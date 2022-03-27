/* This file is part of the TDE Project
   Copyright (c) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "tdehardwarebackend.h"

#include <tqfile.h>
#include <tqfileinfo.h>
#include <tqeventloop.h>
#include <tqstylesheet.h>

#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <tdeio/job.h>
#include <kprocess.h>
#include <kmimetype.h>
#include <kmountpoint.h>
#include <tdemessagebox.h>
#include <tdeapplication.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>

#include "dialog.h"

#define MOUNT_MEDIA_SUFFIX (medium->isEncrypted() ? \
	    (TQString("_encrypted") + (sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt) ? "_unlocked" : "_locked")) : \
      (medium->isMounted() ? TQString("_mounted") : TQString("_unmounted")))

#define MOUNTED_ICON_SUFFIX (medium->isEncrypted() ? \
	    (sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt) ? "-unlocked" : "-locked") : \
      (medium->isMounted() ? TQString("-mounted") : TQString("-unmounted")))

/* Constructor */
TDEBackend::TDEBackend(MediaList &list, TQObject* parent)
    : TQObject()
    , BackendBase(list)
    , m_parent(parent)
{
	// Initialize the TDE device manager
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	// Connect device monitoring signals/slots
	connect(hwdevices, TQT_SIGNAL(hardwareAdded(TDEGenericDevice*)), this, TQT_SLOT(AddDeviceHandler(TDEGenericDevice*)));
	connect(hwdevices, TQT_SIGNAL(hardwareRemoved(TDEGenericDevice*)), this, TQT_SLOT(RemoveDeviceHandler(TDEGenericDevice*)));
	connect(hwdevices, TQT_SIGNAL(hardwareUpdated(TDEGenericDevice*)), this, TQT_SLOT(ModifyDeviceHandler(TDEGenericDevice*)));

	// List devices at startup
	ListDevices();
}

/* Destructor */
TDEBackend::~TDEBackend()
{
	// Remove all media from the media list
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList hwlist = hwdevices->listAllPhysicalDevices();
	TDEGenericDevice *hwdevice;
	for ( hwdevice = hwlist.first(); hwdevice; hwdevice = hwlist.next() ) {
		if (hwdevice->type() == TDEGenericDeviceType::Disk) {
			TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			RemoveDevice(sdevice);
		}
	}
}

void TDEBackend::AddDeviceHandler(TDEGenericDevice *device) {
	if (device->type() == TDEGenericDeviceType::Disk) {
		TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(device);
		AddDevice(sdevice);
	}
}

void TDEBackend::RemoveDeviceHandler(TDEGenericDevice *device) {
	if (device->type() == TDEGenericDeviceType::Disk) {
		TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(device);
		RemoveDevice(sdevice);
	}
}

void TDEBackend::ModifyDeviceHandler(TDEGenericDevice *device) {
	if (device->type() == TDEGenericDeviceType::Disk) {
		TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(device);
		ModifyDevice(sdevice);
	}
}

// List devices (at startup)
bool TDEBackend::ListDevices()
{
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList hwlist = hwdevices->listAllPhysicalDevices();
	TDEGenericDevice *hwdevice;
	for ( hwdevice = hwlist.first(); hwdevice; hwdevice = hwlist.next() ) {
		if (hwdevice->type() == TDEGenericDeviceType::Disk) {
			TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			AddDevice(sdevice, false);
		}
	}

	return true;
}

// Create a media instance for a new storage device
void TDEBackend::AddDevice(TDEStorageDevice * sdevice, bool allowNotification)
{
	kdDebug(1219) << "TDEBackend::AddDevice for " << sdevice->uniqueID() << endl;

	// If the device is already listed, do not process it
	// This should not happen, but who knows...
	/** @todo : refresh properties instead ? */
	if (m_mediaList.findById(sdevice->uniqueID())) {
		kdDebug(1219) << "TDEBackend::AddDevice for " << sdevice->uniqueID() << " found existing entry in media list" << endl;
		return;
	}

	// Respect the device's hidden flag--we will still make the device available via the tdeioslave
	// but we will not generate popups or other notifications on device insertion
	if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hidden)) {
		allowNotification = false;
	}

	// Check if user wants the full popup to be suppressed
	bool allowDialogNotification = allowNotification;
	TDEConfig config("mediamanagerrc");
	config.setGroup("Global");
	if (!config.readBoolEntry("NotificationPopupsEnabled", true)) {
		allowDialogNotification = false;
	}

	// Add volume block devices
	if (sdevice->isDiskOfType(TDEDiskDeviceType::HDD)) {
		/* We only list volumes that...
		*  - are encrypted or
		*  - have a filesystem or
		*  - have an audio track
		*/
		if (!sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		    !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::ContainsFilesystem) &&
			  !sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)
			) {
			//
		}
		// We also don't display devices that underlie other devices, unless they are encrypted devices
		else if ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsedByDevice) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted)) ||
			       sdevice->fileSystemUsage().upper() == "RAID") {
			//
		}
		else {
			// Create medium
			Medium* medium = new Medium(sdevice->uniqueID(), driveUDIFromDeviceUID(sdevice->uniqueID()), "");
			setVolumeProperties(medium);

			// Hide udev hidden devices by default but allow the user to override if desired via Show Hidden Files
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hidden)) {
				medium->setSoftHidden(true);
			}
			else {
				medium->setSoftHidden(false);
			}

			// Insert medium into list
			m_mediaList.addMedium(medium, allowDialogNotification);

			kdDebug(1219) << "TDEBackend::AddDevice inserted hard medium for " << sdevice->uniqueID() << endl;

			// Automount if enabled
			TQMap<TQString,TQString> options = MediaManagerUtils::splitOptions(mountoptions(sdevice->uniqueID()));
			kdDebug(1219) << "automount " << options["automount"] << endl;
			if (options["automount"] == "true" && allowNotification ) {
				TQStringVariantMap mountResult = mount(medium);
				if (!mountResult["result"].toBool()) {
					TQString error = mountResult.contains("errStr") ? mountResult["errStr"].toString() : i18n("Unknown mount error.");
					kdDebug(1219) << "error " << error << endl;
				}
			}
		}
	}

	// Add CD drives
	if ((sdevice->isDiskOfType(TDEDiskDeviceType::CDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMO))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRWW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRAM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRWDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRWDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDVideo))
		) {

		// Create medium
		Medium* medium = new Medium(sdevice->uniqueID(), driveUDIFromDeviceUID(sdevice->uniqueID()), "");
		setVolumeProperties(medium);

		// Insert medium into list
		m_mediaList.addMedium(medium, allowDialogNotification);

		kdDebug(1219) << "TDEBackend::AddDevice inserted optical medium for " << sdevice->uniqueID() << endl;

		// Automount if enabled
		TQMap<TQString,TQString> options = MediaManagerUtils::splitOptions(mountoptions(sdevice->uniqueID()));
		kdDebug(1219) << "automount " << options["automount"] << endl;
		if (options["automount"] == "true" && allowNotification ) {
			TQStringVariantMap mountResult = mount(medium);
			if (!mountResult["result"].toBool()) {
				TQString error = mountResult.contains("errStr") ? mountResult["errStr"].toString() : i18n("Unknown mount error.");
				kdDebug(1219) << "error " << error << endl;
			}
		}
	}

	// Floppy & zip drives
	if ((sdevice->isDiskOfType(TDEDiskDeviceType::Floppy)) ||
		(sdevice->isDiskOfType(TDEDiskDeviceType::Zip)) ||
		(sdevice->isDiskOfType(TDEDiskDeviceType::Jaz))
		) {
		if ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable)) && (!(sdevice->checkDiskStatus(TDEDiskDeviceStatus::Inserted)))) {
			allowNotification = false;
			allowDialogNotification = false;
		}

		/* We only list volumes that...
		*  - are encrypted or
		*  - have a filesystem or
		*  - have an audio track
		*/
		if (!sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		    !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::ContainsFilesystem) &&
			  !sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)
			) {
			//
		}
		// We also don't display devices that underlie other devices, unless they are encrypted devices
		else if ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsedByDevice) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted)) ||
			       sdevice->fileSystemUsage().upper() == "RAID") {
			//
		}
		else {
			// Create medium
			Medium* medium = new Medium(sdevice->uniqueID(), driveUDIFromDeviceUID(sdevice->uniqueID()), "");
			setFloppyProperties(medium);
			m_mediaList.addMedium(medium, allowDialogNotification);

			kdDebug(1219) << "TDEBackend::AddDevice inserted floppy medium for " << sdevice->uniqueID() << endl;

			return;
		}
	}

	// PTP camera
	if (sdevice->isDiskOfType(TDEDiskDeviceType::Camera)) {
		// PTP cameras are handled by the "camera" tdeioslave
		if (KProtocolInfo::isKnownProtocol( TQString("camera") ) )
		{
			// Create medium
			Medium* medium = new Medium(sdevice->uniqueID(), driveUDIFromDeviceUID(sdevice->uniqueID()), "");
			setCameraProperties(medium);
			m_mediaList.addMedium(medium, allowDialogNotification);

			kdDebug(1219) << "TDEBackend::AddDevice inserted camera medium for " << sdevice->uniqueID() << endl;

			return;
		}
	}
}

void TDEBackend::RemoveDevice(TDEStorageDevice * sdevice)
{
	kdDebug(1219) << "TDEBackend::RemoveDevice for " << sdevice->uniqueID() << endl;

	if (!m_mediaList.findById(sdevice->uniqueID())) {
		kdDebug(1219) << "TDEBackend::RemoveDevice for " << sdevice->uniqueID() << " existing entry in media list was not found" << endl;
		return;
	}

	m_mediaList.removeMedium(sdevice->uniqueID(), true);
}

void TDEBackend::ModifyDevice(TDEStorageDevice * sdevice)
{
	kdDebug(1219) << "TDEBackend::ModifyDevice for " << sdevice->uniqueID() << endl;

	ResetProperties(sdevice, false);
}

void TDEBackend::ResetProperties(TDEStorageDevice * sdevice, bool allowNotification, bool overrideIgnoreList)
{
	kdDebug(1219) << "TDEBackend::ResetProperties for " << sdevice->uniqueID() << " allowNotification: " << allowNotification << " overrideIgnoreList: " << overrideIgnoreList << endl;

	if (!m_mediaList.findById(sdevice->uniqueID())) {
		// This device is not currently in the device list, so add it and exit
		kdDebug(1219) << "TDEBackend::ResetProperties for " << sdevice->uniqueID() << " existing entry in media list was not found" << endl;
		AddDevice(sdevice);
		return;
	}

	// If we should ignore device change events for this device, do so
	if (overrideIgnoreList == false) {
		if (m_ignoreDeviceChangeEvents.contains(sdevice->uniqueID())) {
			return;
		}
	}

	Medium* m = new Medium(sdevice->uniqueID(), driveUDIFromDeviceUID(sdevice->uniqueID()), "");

	// Keep these conditions in sync with ::AddDevice above, OR ELSE!!!
	if (sdevice->isDiskOfType(TDEDiskDeviceType::HDD)) {
		/* We only list volumes that...
		*  - are encrypted or
		*  - have a filesystem or
		*  - have an audio track
		*/
		if (!sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		    !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::ContainsFilesystem) &&
			  !sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)
			) {
			//
		}
		// We also don't display devices that underlie other devices, unless they are encrypted devices
		else if ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsedByDevice) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted)) ||
			       sdevice->fileSystemUsage().upper() == "RAID") {
			//
		}
		else {
			setVolumeProperties(m);
		}
	}

	if ((sdevice->isDiskOfType(TDEDiskDeviceType::CDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMO))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRWW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRAM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRWDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRWDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDVideo))
		) {
		setVolumeProperties(m);
	}

	// Floppy & zip drives
	if ((sdevice->isDiskOfType(TDEDiskDeviceType::Floppy)) ||
		(sdevice->isDiskOfType(TDEDiskDeviceType::Zip)) ||
		(sdevice->isDiskOfType(TDEDiskDeviceType::Jaz))
		) {
		/* We only list volumes that...
		*  - are encrypted or
		*  - have a filesystem or
		*  - have an audio track
		*/
		if (!sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		    !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::ContainsFilesystem) &&
			  !sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio) &&
			  !sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)
			) {
			//
		}
		// We also don't display devices that underlie other devices, unless they are encrypted devices
		else if ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsedByDevice) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) &&
		         !sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted)) ||
			       sdevice->fileSystemUsage().upper() == "RAID") {
			//
		}
		else {
			setFloppyProperties(m);
		}
	}

	if (sdevice->isDiskOfType(TDEDiskDeviceType::Camera)) {
		setCameraProperties(m);
	}

	if ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable)) && (!(sdevice->checkDiskStatus(TDEDiskDeviceStatus::Inserted)))) {
		kdDebug(1219) << "TDEBackend::ResetProperties for " << sdevice->uniqueID() << " device was removed from system" << endl;
		RemoveDevice(sdevice);
		return;
	}

	m_mediaList.changeMediumState(*m, allowNotification);

	delete m;
}

void TDEBackend::setVolumeProperties(Medium* medium)
{
	kdDebug(1219) << "TDEBackend::setVolumeProperties for " << medium->id() << endl;

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	TDEStorageDevice *sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		return;
	}

	medium->setName(generateName(sdevice->deviceNode()));
	if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) || sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted)) {
		medium->setEncrypted(true);
		medium->setLocked(!sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt));
	}
	else {
		medium->setEncrypted(false);
	}

	if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) || sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted) ||
			sdevice->fileSystemUsage().upper() == "RAID") {
		// Encrypted disks or device underlying other devices are not mountable
		medium->setMountable(false);
	}
	else {
		medium->setMountable(true);
	}

	medium->setDeviceNode(sdevice->deviceNode());
	medium->setMountPoint(sdevice->mountPath());
	medium->setFsType(sdevice->fileSystemName());
	medium->setMounted(!sdevice->mountPath().isEmpty());

	TQString diskLabel = sdevice->diskLabel();
	bool useDefaultLabel = diskLabel.isNull();
	if (useDefaultLabel) {
		diskLabel = i18n("%1 Removable Device").arg(sdevice->deviceFriendlySize());
	}

	TQString mimeType;

	if ((sdevice->isDiskOfType(TDEDiskDeviceType::CDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMO))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRWW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRAM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRWDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRWDL))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDROM))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDR))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDRW))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDVideo))
		) {
		// This device is a CD drive of some sort

		// Default
		mimeType = "media/cdrom" + MOUNT_MEDIA_SUFFIX;
		if (useDefaultLabel) {
			diskLabel = i18n("%1 Removable Device").arg(sdevice->deviceFriendlySize());
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDROM)) {
			mimeType = "media/cdrom" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankcd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank CD-ROM");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDR)) {
			mimeType = "media/cd-r" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankcd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank CD-R");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDRW)) {
			mimeType = "media/cd-rw" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankcd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank CD-RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDMO)) {
			mimeType = "media/cd-rw" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankcd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank Magneto-Optical CD");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRW)) {
			mimeType = "media/cd-rw" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankcd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank Mount Ranier CD-RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRWW)) {
			mimeType = "media/cd-rw" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankcd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank Mount Ranier CD-RW-W");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDROM)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank DVD-ROM");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRAM)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank DVD-RAM");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDR)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank DVD-R");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRW)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank DVD-RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRDL)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank Dual Layer DVD-R");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDRWDL)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank Dual Layer DVD-RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSR)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank DVD+R");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRW)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank DVD+RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRDL)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank Dual Layer DVD+R");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRWDL)) {
			mimeType = "media/dvd" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankdvd";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank Dual Layer DVD+RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::BDROM)) {
			mimeType = "media/bluray" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankbluray";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank BLURAY-ROM");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::BDR)) {
			mimeType = "media/bluray" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankbluray";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank BLURAY-R");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::BDRW)) {
			mimeType = "media/bluray" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankbluray";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank BLURAY-RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDROM)) {
			mimeType = "media/bluray" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankbluray";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank HDDVD-ROM");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDR)) {
			mimeType = "media/bluray" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankbluray";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank HDDVD-R");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDRW)) {
			mimeType = "media/bluray" + MOUNT_MEDIA_SUFFIX;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				mimeType = "media/blankbluray";
				medium->setMountable(false);
				medium->setBaseURL(TQString::null);
				diskLabel = i18n("Blank HDDVD-RW");
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio)) {
			mimeType = "media/audiocd";
			medium->setMountable(false);
			medium->setBaseURL("audiocd:/?device=" + sdevice->deviceNode());
			diskLabel = i18n("Audio CD");
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CDVideo)) {
			mimeType = "media/vcd";
		}
		if (sdevice->isDiskOfType(TDEDiskDeviceType::DVDVideo)) {
			mimeType = "media/dvdvideo";
		}
		if (sdevice->isDiskOfType(TDEDiskDeviceType::BDVideo)) {
			mimeType = "media/blurayvideo";
		}

		medium->setIconName(TQString::null);
	}
	else {
		// This device is a hard or flash disk of some kind

		// Default
		mimeType = "media/hdd" + MOUNT_MEDIA_SUFFIX;
		if (useDefaultLabel) {
			diskLabel = i18n("%1 Fixed Disk (%2)").arg(sdevice->deviceFriendlySize(), sdevice->deviceNode());
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::USB)
		    || sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable)
		    || sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hotpluggable)) {
			mimeType = "media/removable" + MOUNT_MEDIA_SUFFIX;
			if (useDefaultLabel) {
				diskLabel = i18n("%1 Removable Device").arg(sdevice->deviceFriendlySize());
			}
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::CompactFlash)) {
			medium->setIconName("media-flash-compact_flash" + MOUNTED_ICON_SUFFIX);
		}
		if (sdevice->isDiskOfType(TDEDiskDeviceType::MemoryStick)) {
			medium->setIconName("media-flash-memory_stick" + MOUNTED_ICON_SUFFIX);
		}
		if (sdevice->isDiskOfType(TDEDiskDeviceType::SmartMedia)) {
			medium->setIconName("media-flash-smart_media" + MOUNTED_ICON_SUFFIX);
		}
		if (sdevice->isDiskOfType(TDEDiskDeviceType::SDMMC)) {
			medium->setIconName("media-flash-sd_mmc" + MOUNTED_ICON_SUFFIX);
		}
		if (sdevice->isDiskOfType(TDEDiskDeviceType::MediaDevice)) {
			medium->setIconName("ipod" + MOUNTED_ICON_SUFFIX);
			if (sdevice->vendorModel().upper().contains("IPOD") && KProtocolInfo::isKnownProtocol(TQString("ipod"))) {
				medium->setBaseURL("ipod:/");
				medium->setMountable(true);
				medium->setMounted(!sdevice->mountPath().isEmpty());
			}
		}
		if (sdevice->isDiskOfType(TDEDiskDeviceType::Tape)) {
			medium->setIconName("media-tape" + MOUNTED_ICON_SUFFIX);
		}
		if (medium->isMounted() && TQFile::exists(medium->mountPoint() + "/dcim")) {
			mimeType = "media/camera" + MOUNT_MEDIA_SUFFIX;
		}
	}

	medium->setLabel(diskLabel);
	medium->setMimeType(mimeType);
}

// Handle floppies and zip drives
bool TDEBackend::setFloppyProperties(Medium* medium)
{
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	TDEStorageDevice * sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		return false;
	}

	medium->setName(generateName(sdevice->deviceNode()));
	medium->setLabel(i18n("Unknown Drive"));

	// Certain disks have a lot in common with hard drives
	// FIXME
	// Any more?
	if ((sdevice->isDiskOfType(TDEDiskDeviceType::Zip)) || (sdevice->isDiskOfType(TDEDiskDeviceType::Jaz))) {
		medium->setName(generateName(sdevice->deviceNode()));
		if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) || sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted)) {
			medium->setEncrypted(true);
			medium->setLocked(!sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt));
		}
		else {
			medium->setEncrypted(false);
		}

		if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) || sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted) ||
				sdevice->fileSystemUsage().upper() == "RAID") {
			// Encrypted disks or device underlying other devices are not mountable
			medium->setMountable(false);
		}
		else {
			medium->setMountable(true);
		}

		medium->setDeviceNode(sdevice->deviceNode());
		medium->setMountPoint(sdevice->mountPath());
		medium->setFsType(sdevice->fileSystemName());
		medium->setMounted(!sdevice->mountPath().isEmpty());
	}

	if (sdevice->isDiskOfType(TDEDiskDeviceType::Floppy)) {
		setFloppyMountState(medium);

		// We don't use the routine above as floppy disks are extremely slow (we don't want them accessed at all during media listing)
		medium->setMountable(true);
		medium->setDeviceNode(sdevice->deviceNode());
		medium->setMountPoint(sdevice->mountPath());
		medium->setFsType(sdevice->fileSystemName());
		medium->setMounted(!sdevice->mountPath().isEmpty());

		if (sdevice->mountPath().isNull()) {
			medium->setMimeType("media/floppy_unmounted");
		}
		else {
			medium->setMimeType("media/floppy_mounted" );
		}
		medium->setLabel(i18n("Floppy Drive"));
	}

	if (sdevice->isDiskOfType(TDEDiskDeviceType::Zip)) {
		if (sdevice->mountPath().isNull()) {
			medium->setMimeType("media/zip_unmounted");
		}
		else {
			medium->setMimeType("media/zip_mounted" );
		}

		// Set label
		TQString diskLabel = sdevice->diskLabel();
		if (diskLabel.isNull()) {
			diskLabel = i18n("%1 Zip Disk").arg(sdevice->deviceFriendlySize());
		}
		medium->setLabel(diskLabel);
	}

	/** @todo Mimetype for JAZ drives ? */

	medium->setIconName(TQString::null);

	return true;
}

void TDEBackend::setCameraProperties(Medium* medium)
{
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	TDEStorageDevice * sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		return;
	}

	TQString cameraName = sdevice->friendlyName();
	cameraName.replace("/", "_");
	medium->setName(cameraName);

	TQString device = "camera:/";

	TQStringList devNodeList = TQStringList::split("/", sdevice->deviceNode(), TRUE);
	TQString devNode0 = devNodeList[devNodeList.count()-2];
	TQString devNode1 = devNodeList[devNodeList.count()-1];

	if ((devNode0 != "") && (devNode1 != "")) {
		device.sprintf("camera://@[usb:%s,%s]/", devNode0.ascii(), devNode1.ascii());
	}

	medium->setMountable(false);
	medium->setBaseURL(device);
	medium->setMimeType("media/gphoto2camera");
	medium->setIconName(TQString::null);

	if (sdevice->friendlyName() != "") {
		medium->setLabel(sdevice->friendlyName());
	}
	else {
		medium->setLabel(i18n("Camera"));
	}
}

void TDEBackend::setFloppyMountState( Medium *medium )
{
	KMountPoint::List mtab = KMountPoint::currentMountPoints();
	KMountPoint::List::iterator it = mtab.begin();
	KMountPoint::List::iterator end = mtab.end();

	TQString fstype;
	TQString mountpoint;
	for (; it!=end; ++it) {
		if ((*it)->mountedFrom() == medium->deviceNode() ) {
			fstype = (*it)->mountType().isNull() ? (*it)->mountType() : "auto";
			mountpoint = (*it)->mountPoint();
			medium->setMountable(true);
			medium->setDeviceNode(medium->deviceNode());
			medium->setMountPoint(mountpoint);
			medium->setFsType(fstype);
			medium->setMounted(true);
			return;
		}
	}
}

TQStringList TDEBackend::mountoptions(const TQString &name)
{
	const Medium* medium = m_mediaList.findById(name);
	if (!medium) {
		return TQStringList(); // we know nothing about that device
	}
	if (!isInFstab(medium).isNull()) {
		return TQStringList(); // device is listed in fstab, therefore is not handled by us
	}

	if (medium->isEncrypted()) {
		// if not unlocked yet then there are no mountoptions
		return TQStringList();
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	TDEStorageDevice * sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		return TQStringList(); // we can't get the information needed in order to process mount options
	}

	TQStringList result;

	// Allow only those options which are valid for the given device
	// FIXME: Is there a better way to determine options by the file system?
	TQMap<TQString,bool> valids;
	valids["ro"] = true;
	//valids["quiet"] = false;
	//valids["flush"] = false;
	//valids["uid"] = false;
	//valids["utf8"] = false;
	//valids["shortname"] = false;
	//valids["locale"] = false;
	valids["sync"] = true;
	valids["noatime"] = true;
	//valids["data"] = false;

	if ((sdevice->fileSystemName().endsWith("fat"))
	 || (sdevice->fileSystemName().endsWith("dos"))
	) {
		valids["utf8"] = true;
		valids["shortname"] = true;
	}
	if ((sdevice->fileSystemName() == "ext3")
	 || (sdevice->fileSystemName() == "ext4")
	) {
		valids["data"] = true;
	}
	if (sdevice->fileSystemName() == "ntfs") {
		valids["utf8"] = true;
	}
	if (sdevice->fileSystemName() == "ntfs-3g") {
		valids["locale"] = true;
	}
	if (sdevice->fileSystemName() == "iso9660") {
		valids["utf8"] = true;
		valids.remove("ro");
		valids.remove("quiet");
		valids.remove("sync");
		valids.remove("noatime");
	}
	if (sdevice->fileSystemName() == "jfs") {
		valids["utf8"] = true;
	}

	TQString drive_udi = driveUDIFromDeviceUID(medium->id());

	TDEConfig config("mediamanagerrc");

	bool use_defaults = true;
	if (config.hasGroup(drive_udi)) {
		config.setGroup(drive_udi);
		use_defaults = config.readBoolEntry("use_defaults", false);
	}
	if (use_defaults) {
		config.setGroup("DefaultOptions");
	}
	result << TQString("use_defaults=%1").arg(use_defaults ? "true" : "false");

	bool removable = false;
	if (!drive_udi.isNull()) {
		removable = ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable)) || (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hotpluggable)));
	}

	TQString tmp;
	bool value;
	if (use_defaults) {
		value = config.readBoolEntry("automount", false);
	}
	else {
		TQString current_group = config.group();
		config.setGroup(drive_udi);
		value = config.readBoolEntry("automount", false);
		config.setGroup(current_group);
	}

	if ((sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::CDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::DVDVideo))
		|| (sdevice->isDiskOfType(TDEDiskDeviceType::BDVideo))
		) {
		value = false;
	}

	result << TQString("automount=%1").arg(value ? "true" : "false");

	if (valids.contains("ro")) {
		value = config.readBoolEntry("ro", false);
		tmp = TQString("ro=%1").arg(value ? "true" : "false");
		result << tmp;
	}

	if (valids.contains("quiet")) {
		value = config.readBoolEntry("quiet", false);
		tmp = TQString("quiet=%1").arg(value ? "true" : "false");
		result << tmp;
	}

	if (valids.contains("flush")) {
		value = config.readBoolEntry("flush", sdevice->fileSystemName().endsWith("fat"));
		tmp = TQString("flush=%1").arg(value ? "true" : "false");
		result << tmp;
	}

	if (valids.contains("uid")) {
		value = config.readBoolEntry("uid", true);
		tmp = TQString("uid=%1").arg(value ? "true" : "false");
		result << tmp;
	}

	if (valids.contains("utf8")) {
		value = config.readBoolEntry("utf8", true);
		tmp = TQString("utf8=%1").arg(value ? "true" : "false");
		result << tmp;
	}

	if (valids.contains("shortname")) {
		TQString svalue = config.readEntry("shortname", "lower").lower();
		// If the user choose 'default', no shortname is added to the mount option list
		if (svalue == "windows nt") {
			result << "shortname=winnt";
		}
		else if (svalue == "windows 95") {
			result << "shortname=win95";
		}
		else if (svalue == "mixed") {
			result << "shortname=mixed";
		}
		else if (svalue == "lower"){
			result << "shortname=lower";
		}
	}

	// pass our locale to the ntfs-3g driver so it can translate local characters
	if (valids.contains("locale")) {
		// have to obtain LC_CTYPE as returned by the `locale` command
		// check in the same order as `locale` does
		char *cType;
		if ( (cType = getenv("LC_ALL")) || (cType = getenv("LC_CTYPE")) || (cType = getenv("LANG")) ) {
			result << TQString("locale=%1").arg(cType);
		}
	}

	if (valids.contains("sync")) {
		value = config.readBoolEntry("sync", ( valids.contains("flush") && !sdevice->fileSystemName().endsWith("fat") ) && removable);
		tmp = TQString("sync=%1").arg(value ? "true" : "false");
		result << tmp;
	}

	if (valids.contains("noatime")) {
		value = config.readBoolEntry("atime", !sdevice->fileSystemName().endsWith("fat"));
		tmp = TQString("atime=%1").arg(value ? "true" : "false");
		result << tmp;
	}

	TQString mount_point;
	mount_point = config.readEntry("mountpoint", TQString::null);

	if (!mount_point.startsWith("/")) {
		mount_point = "/media/" + mount_point;
	}
	if (mount_point != "") {
		result << TQString("mountpoint=%1").arg(mount_point);
	}

	TQString file_system_name;
	file_system_name = sdevice->fileSystemName();
	if (file_system_name != "") {
		result << TQString("filesystem=%1").arg(file_system_name);
	}

	if (valids.contains("data")) {
		TQString svalue = config.readEntry("journaling").lower();
		if (svalue == "ordered") {
			result << "journaling=ordered";
		}
		else if (svalue == "writeback") {
			result << "journaling=writeback";
		}
		else if (svalue == "data") {
			result << "journaling=data";
		}
		else {
			result << "journaling=ordered";
		}
	}

	return result;
}

bool TDEBackend::setMountoptions(const TQString &name, const TQStringList &options )
{
	const Medium* medium = m_mediaList.findById(name);
	if (!medium) {
		return false; // we know nothing about that device
	}
	if (!isInFstab(medium).isNull()) {
		return false; // device is listed in fstab, therefore is not handled by us
	}

	TQString drive_udi = driveUDIFromDeviceUID(medium->id());

	kdDebug(1219) << "setMountoptions " << name << " " << options << endl;

	TDEConfig config("mediamanagerrc");
	config.setGroup(drive_udi);

	TQMap<TQString,TQString> valids = MediaManagerUtils::splitOptions(options);

	const char *names[] = { "use_defaults", "ro", "quiet", "atime", "uid", "utf8", "flush", "sync", 0 };
	for (int index = 0; names[index]; ++index) {
		if (valids.contains(names[index])) {
			config.writeEntry(names[index], valids[names[index]] == "true");
		}
	}

	if (valids.contains("shortname")) {
		config.writeEntry("shortname", valids["shortname"]);
	}

	if (valids.contains("journaling")) {
		config.writeEntry("journaling", valids["journaling"]);
	}

	if (!mountoptions(name).contains(TQString("mountpoint=%1").arg(valids["mountpoint"]))) {
		config.writeEntry("mountpoint", valids["mountpoint"]);
	}

	if (valids.contains("automount")) {
		config.setGroup(drive_udi);
		config.writeEntry("automount", valids["automount"]);
	}

	return true;
}

TQStringVariantMap TDEBackend::mount(const Medium *medium)
{
	kdDebug(1219) << "TDEBackend::mount for medium " << medium->name() << endl;

	TQStringVariantMap result;
	if (!medium->isMountable()) {
		result["errStr"] = i18n("%1 is not a mountable media.").arg(medium->deviceNode());
		result["result"] = false;
		return result;
	}
	else if (medium->isMounted()) {
		result["errStr"] = i18n("%1 is already mounted to %2.").arg(medium->deviceNode()).arg(medium->mountPoint());
		result["result"] = false;
		return result;
	}

	TQString mountPoint = isInFstab(medium);
	if (!mountPoint.isEmpty())
	{
		struct mount_job_data data;
		data.completed = false;
		data.medium = medium;

		TDEIO::Job *job = TDEIO::mount(false, 0, medium->deviceNode(), mountPoint);
		connect(job, TQT_SIGNAL(result(TDEIO::Job*)), TQT_SLOT(slotResult(TDEIO::Job*)));
		mount_jobs[job] = &data;
		// The caller expects the device to be mounted when the function
		// completes. Thus block until the job completes.
		while (!data.completed) {
			kapp->eventLoop()->enterLoop();
		}
		if (!data.error) {
			result["result"] = true;
			return result;
		}
		else {
			result["errStr"] = data.errorMessage; // Return the error message (if any) to the caller
			result["result"] = false;
			return result;
		}
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEStorageDevice *sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		result["errStr"] = i18n("Internal error. Couldn't find medium id %1.").arg(medium->id());
		result["result"] = false;
		return result;
	}

	TQMap<TQString,TQString> valids = MediaManagerUtils::splitOptions(mountoptions(medium->id()));
	TQString diskLabel;
	TQString mount_point = valids["mountpoint"];
	if (mount_point.startsWith("/media/")) {
		diskLabel = mount_point.mid(7);
	}
	if (diskLabel.isEmpty()) {
		// Try to use a pretty mount point if possible
		TQStringList pieces = TQStringList::split("/", sdevice->deviceNode(), FALSE);
		TQString node = pieces[pieces.count()-1];
		diskLabel = medium->label() + " (" + node + ")";
		diskLabel.replace("/", "_");
	}

	TQString qerror;
	TQStringVariantMap mountResult = sdevice->mountDevice(diskLabel, valids);
	TQString mountedPath = mountResult.contains("mountPath") ? mountResult["mountPath"].toString() : TQString::null;
	if (mountedPath.isEmpty()) {
		qerror = i18n("<b>Unable to mount this device.</b>");
		TQString errStr = mountResult.contains("errStr") ? mountResult["errStr"].toString() : TQString::null;
		if (!errStr.isEmpty()) {
			qerror.append(i18n("<p>Technical details:<br>").append(errStr));
		}
	}

	if (!qerror.isEmpty()) {
		result["errStr"] = qerror;
		result["result"] = false;
		return result;
	}

	ResetProperties(sdevice, false, true);

	if (m_ignoreDeviceChangeEvents.contains(sdevice->uniqueID())) {
		m_ignoreDeviceChangeEvents.remove(sdevice->uniqueID());
	}

	result["result"] = true;
	return result;
}

TQStringVariantMap TDEBackend::mount(const TQString &id)
{
	kdDebug(1219) << "TDEBackend::mount for id " << id << endl;

	const Medium *medium = m_mediaList.findById(id);
	if (!medium) {
		TQStringVariantMap result;
		result["errStr"] = i18n("No such medium: %1").arg(id);
		result["result"] = false;
		return result;
	}
	return mount(medium);
}

TQStringVariantMap TDEBackend::unmount(const TQString &id)
{
	kdDebug(1219) << "TDEBackend::unmount for id " << id << endl;

	TQStringVariantMap result;
	const Medium *medium = m_mediaList.findById(id);
	if (!medium) {
		result["errStr"] = i18n("No such medium: %1").arg(id);
		result["result"] = false;
		return result;
	}
	else if (!medium->isMountable()) {
		result["errStr"] = i18n("%1 is not a mountable media.").arg(medium->deviceNode());
		result["result"] = false;
		return result;
	}
	else if (!medium->isMounted()) {
		result["errStr"] = i18n("%1 is already unmounted.").arg(medium->deviceNode());
		result["result"] = false;
		return result;
	}

	TQString mountPoint = isInFstab(medium);
	if (!mountPoint.isNull())
	{
		struct mount_job_data data;
		data.completed = false;
		data.medium = medium;

		TDEIO::Job *job = TDEIO::unmount( medium->mountPoint(), false );
		connect(job, TQT_SIGNAL(result(TDEIO::Job*)), TQT_SLOT(slotResult(TDEIO::Job*)));
		mount_jobs[job] = &data;
		// The caller expects the device to be unmounted when the function
		// completes. Thus block until the job completes.
		while (!data.completed) {
			kapp->eventLoop()->enterLoop();
		}
		if (!data.error) {
			result["result"] = true;
			return result;
		}
		else {
			result["errStr"] = data.errorMessage; // Return the error message (if any) to the caller
			result["result"] = false;
			return result;
		}
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEStorageDevice *sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		result["errStr"] = i18n("Internal error. Couldn't find medium id %1.").arg(medium->id());
		result["result"] = false;
		return result;
	}

	// Save these for later
	TQString uid = sdevice->uniqueID();
	TQString node = sdevice->deviceNode();

	TQString qerror;
	TQStringVariantMap unmountResult = sdevice->unmountDevice();
	if (unmountResult["result"].toBool() == false) {
		// Unmount failed!
		qerror = i18n("The device <b>%1</b> (%2) named <b>'%3'</b> and currently mounted at "
		    "<b>%4</b> could not be unmounted. ").arg("system:/media/" + medium->name(), medium->deviceNode(),
		    medium->prettyLabel(), medium->prettyBaseURL().pathOrURL());
		TQString errStr = unmountResult.contains("errStr") ? unmountResult["errStr"].toString() : TQString::null;
		if (!errStr.isEmpty()) {
			qerror.append(i18n("<p>Technical details:<br>").append(errStr));
		}
	}

	if (unmountResult.contains("retCode") && unmountResult["retCode"].toInt() == 1280) {
		// Failed as BUSY
		TQString processesUsingDev = listUsingProcesses(medium);
		if (!processesUsingDev.isNull()) {
			if (KMessageBox::warningYesNo(0, i18n("The device <b>%1</b> (%2) named <b>'%3'</b> and currently "
			    "mounted at <b>%4</b> can not be unmounted at this time.<p>%5<p><b>Would you like to forcibly "
			    "terminate these processes?</b><br><i>All unsaved data would be lost</i>").arg("system:/media/" +
			    medium->name()).arg(medium->deviceNode()).arg(medium->prettyLabel()).arg(medium->prettyBaseURL().pathOrURL())
			    .arg(processesUsingDev)) == KMessageBox::Yes) {
				killUsingProcesses(medium);
				unmountResult = sdevice->unmountDevice();
				if (unmountResult["result"].toBool() == false) {
					// Unmount failed!
					qerror = i18n("The device <b>%1</b> (%2) named <b>'%3'</b> and currently mounted at "
					    "<b>%4</b> could not be unmounted. ").arg("system:/media/" + medium->name(), medium->deviceNode(),
					    medium->prettyLabel(), medium->prettyBaseURL().pathOrURL());
					TQString errStr = unmountResult.contains("errStr") ? unmountResult["errStr"].toString() : TQString::null;
					if (!errStr.isEmpty()) {
						qerror.append(i18n("<p>Technical details:<br>").append(errStr));
					}
				}
			}
		}
	}

	if (!qerror.isEmpty()) {
		result["errStr"] = qerror;
		result["result"] = false;
		return result;
	}

	// There is a possibility that the storage device was unceremoniously removed from the system immediately
	// after it was unmounted. There is no reliable way to know if this happened either!
	// For now, see if the device node still exists
	TQFileInfo checkDN(node);
	if (!checkDN.exists()) {
		m_mediaList.removeMedium(uid, true);
	}

	ResetProperties(sdevice, false, true);
	result["result"] = true;
	return result;
}

TQStringVariantMap TDEBackend::unlock(const TQString &id, const TQString &password)
{
	kdDebug(1219) << "TDEBackend::unlock for id " << id << endl;

	TQStringVariantMap result;
	const Medium *medium = m_mediaList.findById(id);
	if (!medium) {
		result["errStr"] = i18n("No such medium: %1").arg(id);
		result["result"] = false;
		return result;
	}
	else if (!medium->isEncrypted()) {
		result["errStr"] = i18n("%1 is not an encrypted media.").arg(medium->deviceNode());
		result["result"] = false;
		return result;
	}
	else if (!medium->needUnlocking()) {
		result["errStr"] = i18n("%1 is already unlocked.").arg(medium->deviceNode());
		result["result"] = false;
		return result;
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEStorageDevice *sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		result["errStr"] = i18n("Internal error. Couldn't find medium id %1.").arg(medium->id());
		result["result"] = false;
		return result;
	}

	TQStringVariantMap unlockResult = sdevice->unlockDevice(password);
	if (unlockResult["result"].toBool() == false) {
		TQString qerror = i18n("<b>Unable to unlock the device.</b>");
		TQString errStr = unlockResult.contains("errStr") ? unlockResult["errStr"].toString() : TQString::null;
		if (!errStr.isEmpty()) {
			qerror.append(i18n("<p>Technical details:<br>").append(errStr));
		result["errStr"] = qerror;
		result["result"] = false;
		return result;
		}
	}

	ResetProperties(sdevice, false, true);
	result["result"] = true;
	return result;
}

TQStringVariantMap TDEBackend::lock(const TQString &id, bool releaseHolders)
{
	kdDebug(1219) << "TDEBackend::lock for id " << id << ", release holders "
		            << releaseHolders << endl;

	TQStringVariantMap result;

	const Medium *medium = m_mediaList.findById(id);
	if (!medium) {
		result["errStr"] = i18n("No such medium: %1").arg(id);
		result["result"] = false;
		return result;
	}
	else if (!medium->isEncrypted()) {
		result["errStr"] = i18n("%1 is not an encrypted media.").arg(medium->deviceNode());
		result["result"] = false;
		return result;
	}
	else if (medium->needUnlocking()) {
		result["errStr"] = i18n("%1 is already locked.").arg(medium->deviceNode());
		result["result"] = false;
		return result;
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEStorageDevice *sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice) {
		result["errStr"] = i18n("Internal error. Couldn't find medium id %1.").arg(medium->id());
		result["result"] = false;
		return result;
	}

	// Release device holders if requested
	if (releaseHolders)
	{
		releaseHolderDevices(medium->deviceNode(), false);
	}

	TQStringVariantMap lockResult = sdevice->lockDevice();
	if (lockResult["result"].toBool() == false) {
		TQString qerror = i18n("<b>Unable to lock the device.</b>");
		TQString errStr = lockResult.contains("errStr") ? lockResult["errStr"].toString() : TQString::null;
		if (!errStr.isEmpty()) {
			qerror.append(i18n("<p>Technical details:<br>").append(errStr));
		result["errStr"] = qerror;
		result["result"] = false;
		return result;
		}
	}

	ResetProperties(sdevice, false, true);
	result["result"] = true;
	return result;
}

TQStringVariantMap TDEBackend::eject(const TQString &id)
{
	kdDebug(1219) << "TDEBackend::eject for id " << id << endl;

	TQStringVariantMap result;

	const Medium *medium = m_mediaList.findById(id);
	if (!medium)
	{
		result["errStr"] = i18n("No such medium: %1").arg(id);
		result["result"] = false;
		return result;
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEStorageDevice *sdevice = hwdevices->findDiskByUID(medium->id());
	if (!sdevice)
	{
		result["errStr"] = i18n("Internal error. Couldn't find medium id %1.").arg(medium->id());
		result["result"] = false;
		return result;
	}

	TQStringVariantMap ejectResult = sdevice->ejectDrive();
	if (ejectResult["result"].toBool() == false)
	{
		TQString qerror = i18n("<b>Unable to eject the device.</b>");
		TQString errStr = ejectResult.contains("errStr") ? ejectResult["errStr"].toString() : TQString::null;
		if (!errStr.isEmpty())
		{
			qerror.append(i18n("<p>Technical details:<br>").append(errStr));
			result["errStr"] = qerror;
			result["result"] = false;
			return result;
		}
	}

	result["result"] = true;
	return result;
}

TQStringVariantMap TDEBackend::safeRemove(const TQString &id)
{
	kdDebug(1219) << "TDEBackend::safeRemove for id " << id << endl;

	TQStringVariantMap result;

	const Medium *medium = m_mediaList.findById(id);
	if (!medium)
	{
		result["errStr"] = i18n("No such medium: %1").arg(id);
		result["result"] = false;
		return result;
	}

	releaseHolderDevices(medium->deviceNode(), true);
	return eject(id);
}

void TDEBackend::releaseHolderDevices(const TQString &deviceNode, bool handleThis)
{
	kdDebug(1219) << "TDEBackend::releaseHolderDevices for node " << deviceNode
	              << ", handle this " << (handleThis ? "yes" : "no") << endl;

	const Medium *medium = m_mediaList.findByNode(deviceNode);
	if (!medium)
	{
		return;
	}

	// Scan the holding devices and unmount/lock them if possible
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEStorageDevice *sdevice = hwdevices->findDiskByUID(medium->id());
	if (sdevice)
	{
		TQStringList holdingDeviceList = sdevice->holdingDevices();
		for (TQStringList::Iterator holdingDevIt = holdingDeviceList.begin(); holdingDevIt != holdingDeviceList.end(); ++holdingDevIt)
		{
			TDEGenericDevice *hwHolderDevice = hwdevices->findBySystemPath(*holdingDevIt);
			if (hwHolderDevice->type() == TDEGenericDeviceType::Disk)
			{
				TDEStorageDevice *holderSDevice = static_cast<TDEStorageDevice*>(hwHolderDevice);
				const Medium *holderMedium = m_mediaList.findByNode(holderSDevice->deviceNode());
				if (holderMedium && !holderMedium->id().isEmpty())
				{
					releaseHolderDevices(holderMedium->deviceNode(), true);
				}
			}
		}
	}

	if (handleThis)
	{
		// Unmount if necessary
		if (medium->isMountable() && medium->isMounted())
		{
			unmount(medium->id());
			// Must process udev events before continuing, to make sure all
			// affected devices are properly updated
			tqApp->processEvents();
		}
		// Lock if necessary.
		if (medium->isEncrypted() && !medium->isLocked())
		{
			lock(medium->id(), false);
			// Must process udev events before continuing, to make sure all
			// affected devices are properly updated
			tqApp->processEvents();
		}
	}
}

void TDEBackend::slotResult(TDEIO::Job *job)
{
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	struct mount_job_data *data = mount_jobs[job];
	TQString& qerror = data->errorMessage;
	const Medium* medium = data->medium;

	if (job->error() == TDEIO::ERR_COULD_NOT_UNMOUNT) {
		TQString proclist(listUsingProcesses(medium));

		qerror += "<p>" + i18n("The device <b>%1</b> (%2) named <b>'%3'</b> and "
			"currently mounted at <b>%4</b> could not be unmounted. ").arg(
				"system:/media/" + medium->name(),
				medium->deviceNode(),
				medium->prettyLabel(),
				medium->prettyBaseURL().pathOrURL()) + "</p>";
		qerror += "<p>" + i18n("The following error was returned by umount command:");
		qerror += "</p><pre>" + job->errorText() + "</pre>";

		if (!proclist.isEmpty()) {
			qerror += proclist;
		}
	} else if (job->error()) {
		qerror = job->errorText();
	}

	TDEStorageDevice * sdevice = hwdevices->findDiskByUID(medium->id());
	if (sdevice) {
		ResetProperties(sdevice);
	}
	mount_jobs.remove(job);

	/* Job completed. Notify the caller */
	data->error = job->error();
	data->completed = true;
	kapp->eventLoop()->exitLoop();
}

TQString TDEBackend::isInFstab(const Medium *medium)
{
	KMountPoint::List fstab = KMountPoint::possibleMountPoints(KMountPoint::NeedMountOptions|KMountPoint::NeedRealDeviceName);

	KMountPoint::List::iterator it = fstab.begin();
	KMountPoint::List::iterator end = fstab.end();

	for (; it!=end; ++it)
	{
		TQString reald = (*it)->realDeviceName();
		if ( reald.endsWith( "/" ) ) {
			reald = reald.left( reald.length() - 1 );
		}
		if ((*it)->mountedFrom() == medium->deviceNode() || ( !medium->deviceNode().isEmpty() && reald == medium->deviceNode() ) )
		{
			TQStringList opts = (*it)->mountOptions();
			if (opts.contains("user") || opts.contains("users")) {
				return (*it)->mountPoint();
			}
		}
	}

	return TQString::null;
}

TQString TDEBackend::listUsingProcesses(const Medium* medium)
{
	TQString proclist, fullmsg;
	TQString fuserpath = TDEStandardDirs::findExe("fuser", TQString("/sbin:/usr/sbin:") + getenv( "PATH" ));
	FILE *fuser = NULL;

	uint counter = 0;
	if (!fuserpath.isEmpty()) {
		TQString cmdline = TQString("/usr/bin/env %1 -vm %2 2>&1").arg(fuserpath, TDEProcess::quote(medium->mountPoint()));
		fuser = popen(cmdline.latin1(), "r");
	}
	if (fuser) {
		proclist += "<pre>";
		TQTextIStream is(fuser);
		TQString tmp;
		while (!is.atEnd()) {
			tmp = is.readLine();
			tmp = TQStyleSheet::escape(tmp) + "\n";

			proclist += tmp;
			if (counter++ > 10) {
				proclist += "...";
				break;
			}
		}
		proclist += "</pre>";
		(void)pclose( fuser );
	}
	if (counter) {
		fullmsg = i18n("Programs still using the device "
			"have been detected. They are listed below. You have to "
			"close them or change their working directory before "
			"attempting to unmount the device again.");
		fullmsg += "<br>" + proclist;
		return fullmsg;
	}
	else {
		return TQString::null;
	}
}

TQString TDEBackend::killUsingProcesses(const Medium* medium)
{
	TQString proclist, fullmsg;
	TQString fuserpath = TDEStandardDirs::findExe("fuser", TQString("/sbin:/usr/sbin:") + getenv( "PATH" ));
	FILE *fuser = NULL;

	uint counter = 0;
	if (!fuserpath.isEmpty()) {
		TQString cmdline = TQString("/usr/bin/env %1 -vmk %2 2>&1").arg(fuserpath, TDEProcess::quote(medium->mountPoint()));
		fuser = popen(cmdline.latin1(), "r");
	}
	if (fuser) {
		proclist += "<pre>";
		TQTextIStream is(fuser);
		TQString tmp;
		while (!is.atEnd()) {
			tmp = is.readLine();
			tmp = TQStyleSheet::escape(tmp) + "\n";

			proclist += tmp;
			if (counter++ > 10) {
				proclist += "...";
				break;
			}
		}
		proclist += "</pre>";
		(void)pclose( fuser );
	}
	if (counter) {
		fullmsg = i18n("Programs that were still using the device "
			"have been forcibly terminated. They are listed below.");
		fullmsg += "<br>" + proclist;
		return fullmsg;
	}
	else {
		return TQString::null;
	}
}

TQString TDEBackend::generateName(const TQString &devNode)
{
    return KURL(devNode).fileName();
}

TQString TDEBackend::driveUDIFromDeviceUID(TQString uuid) {
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	TDEStorageDevice * sdevice = hwdevices->findDiskByUID(uuid);
	TQString ret;
	if (sdevice) {
		ret = sdevice->diskUUID();
		if (ret != "") {
			ret = "volume_uuid_" + ret;
		}
		else {
			ret = sdevice->deviceNode();
			if (ret != "") {
				ret = "device_node_" + ret;
			}
			else {
				ret = sdevice->uniqueID();
			}
		}
	}
	if (ret == "") {
		return TQString::null;
	}
	else {
		return ret;
	}
}

#include "tdehardwarebackend.moc"
