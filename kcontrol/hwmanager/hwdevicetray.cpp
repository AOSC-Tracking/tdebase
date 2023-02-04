/*
 * Copyright 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 *
 * This file is part of hwdevicetray, the TDE Hardware Device Monitor System Tray Application
 *
 * hwdevicetray is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * hwdevicetray is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with cryptocardwatcher. If not, see http://www.gnu.org/licenses/.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <unistd.h>

#include <tqfileinfo.h>
#include <tqimage.h>
#include <tqtimer.h>
#include <tqtooltip.h>

#include <kcmultidialog.h>
#include <kglobalaccel.h>
#include <khelpmenu.h>
#include "kprocess.h"
#include <kpropertiesdialog.h>
#include <krun.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

#include <tdeaction.h>
#include <tdeapplication.h>
#include <tdeglobal.h>
#include <tdehardwaredevices.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include "tdepassivepopupstack.h"
#include <tdepopupmenu.h>

#include <dcopclient.h>
#include <dcopref.h>

#include "hwdevicetray.h"
#include "hwdevicetray_configdialog.h"


typedef TQMap<int, TQString> TQStringMap;

struct KnownDiskDeviceInfo
{
	TQString friendlyName;
	TQString node;
};

class HwDeviceSystemTrayPrivate
{
public:
	HwDeviceSystemTrayPrivate()
	{
	}

	~HwDeviceSystemTrayPrivate()
	{
	}

	// Members
	KHelpMenu *m_help;
	TDEPopupMenu *m_RMBMenu;

	TQStringMap m_openMenuIndexMap;
	TQStringMap m_mountMenuIndexMap;
	TQStringMap m_unmountMenuIndexMap;
	TQStringMap m_unlockMenuIndexMap;
	TQStringMap m_lockMenuIndexMap;
	TQStringMap m_ejectMenuIndexMap;
	TQStringMap m_safeRemoveMenuIndexMap;
	TQStringMap m_propertiesMenuIndexMap;

	TQMap<TQString, KnownDiskDeviceInfo> m_knownDiskDevices;

	TDEPassivePopupStackContainer *m_hardwareNotifierContainer;
};


HwDeviceSystemTray::HwDeviceSystemTray(TQWidget *parent, const char *name)
	: KSystemTray(parent, name), d(new HwDeviceSystemTrayPrivate())
{
	// Create notifier
	d->m_hardwareNotifierContainer = new TDEPassivePopupStackContainer();
	connect(d->m_hardwareNotifierContainer, TQT_SIGNAL(popupClicked(KPassivePopup*, TQPoint, TQString)), this, TQT_SLOT(devicePopupClicked(KPassivePopup*, TQPoint, TQString)));

	// Create menus
	d->m_help = new KHelpMenu(this, TDEGlobal::instance()->aboutData(), false, actionCollection());
	d->m_help->menu()->connectItem(KHelpMenu::menuHelpContents, this, TQT_SLOT(slotHelpContents()));

	d->m_RMBMenu = contextMenu();

	setPixmap(KSystemTray::loadIcon("hwinfo"));
	setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	connect(this, TQT_SIGNAL(quitSelected()), this, TQT_SLOT(quitApp()));
	TQToolTip::add(this, i18n("Device monitor"));

	globalKeys = new TDEGlobalAccel(TQT_TQOBJECT(this));
	TDEGlobalAccel* keys = globalKeys;
	#include "hwdevicetray_bindings.cpp"
	// the keys need to be read from tdeglobals, not kickerrc
	globalKeys->readSettings();
	globalKeys->setEnabled(true);
	globalKeys->updateConnections();

	connect(kapp, TQT_SIGNAL(settingsChanged(int)), TQT_SLOT(slotSettingsChanged(int)));

	new TDEActionMenu(i18n("Open"), SmallIcon("window-new", TQIconSet::Automatic), actionCollection(), "open_menu");
	new TDEActionMenu(i18n("Mount"), SmallIcon("drive-harddisk-mounted", TQIconSet::Automatic), actionCollection(), "mount_menu");
	new TDEActionMenu(i18n("Unmount"), SmallIcon("drive-harddisk-unmounted", TQIconSet::Automatic), actionCollection(), "unmount_menu");
	new TDEActionMenu(i18n("Unlock"), SmallIcon("decrypted", TQIconSet::Automatic), actionCollection(), "unlock_menu");
	new TDEActionMenu(i18n("Lock"), SmallIcon("encrypted", TQIconSet::Automatic), actionCollection(), "lock_menu");
	new TDEActionMenu(i18n("Eject"), SmallIcon("player_eject", TQIconSet::Automatic), actionCollection(), "eject_menu");
	new TDEActionMenu(i18n("Safe remove"), SmallIcon("player_safe_removal", TQIconSet::Automatic), actionCollection(), "safe_remove_menu");
	new TDEActionMenu(i18n("Properties"), SmallIcon("edit", TQIconSet::Automatic), actionCollection(), "properties_menu");

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	doDiskNotifications(true);
	connect(hwdevices, TQT_SIGNAL(hardwareAdded(TDEGenericDevice*)), this, TQT_SLOT(deviceAdded(TDEGenericDevice*)));
	connect(hwdevices, TQT_SIGNAL(hardwareRemoved(TDEGenericDevice*)), this, TQT_SLOT(deviceRemoved(TDEGenericDevice*)));
	connect(hwdevices, TQT_SIGNAL(hardwareUpdated(TDEGenericDevice*)), this, TQT_SLOT(deviceChanged(TDEGenericDevice*)));
}

HwDeviceSystemTray::~HwDeviceSystemTray()
{
	delete d->m_hardwareNotifierContainer;
  delete d;
}

/*!
 * \b TQT_SLOT which called if hwdevicetray is exited by the user. In this case the user
 * is asked through a yes/no box if "HwDeviceTray should start automatically on log in" and the
 * result is written to the KDE configfile.
 */
void HwDeviceSystemTray::quitApp()
{
	KSimpleConfig *config = new KSimpleConfig("tdehwdevicetrayrc");

	TQString tmp1 = i18n("Start device monitor automatically when you log in?");
	int tmp2 = KMessageBox::questionYesNo(0, tmp1, i18n("Question"), i18n("Start Automatically"), i18n("Do Not Start"));
	config->setGroup("General");
	config->writeEntry("Autostart", tmp2 == KMessageBox::Yes);
	config->sync();

	exit(0);
}

void HwDeviceSystemTray::resizeTrayIcon () {
	// Honor Free Desktop specifications that allow for arbitrary system tray icon sizes
	TQPixmap origpixmap;
	TQPixmap scaledpixmap;
	TQImage newIcon;
	origpixmap = KSystemTray::loadSizedIcon("hwinfo", width());
	newIcon = origpixmap;
	newIcon = newIcon.smoothScale(width(), height());
	scaledpixmap = newIcon;
	setPixmap(scaledpixmap);
}

void HwDeviceSystemTray::resizeEvent (TQResizeEvent *) {
	// Honor Free Desktop specifications that allow for arbitrary system tray icon sizes
	resizeTrayIcon();
}

void HwDeviceSystemTray::showEvent (TQShowEvent *) {
	// Honor Free Desktop specifications that allow for arbitrary system tray icon sizes
	resizeTrayIcon();
}

void HwDeviceSystemTray::mousePressEvent(TQMouseEvent* e)
{
	switch (e->button())
	{
		case Qt::LeftButton:

			break;

		case Qt::MidButton:
			TQTimer::singleShot(0, this, TQT_SLOT(slotHardwareConfig()));
			break;

		case Qt::RightButton:
			contextMenuAboutToShow(d->m_RMBMenu);
			d->m_RMBMenu->popup(e->globalPos());
			break;

		default:
			// do nothing
			break;
	}
}

bool HwDeviceSystemTray::isMonitoredDevice(TDEStorageDevice* sdevice)
{
	// Type selection logic largely duplicated from the media manager tdeioslave
	return ((sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) ||
	         sdevice->checkDiskStatus(TDEDiskDeviceStatus::ContainsFilesystem) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio) ||
	         sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) &&
	        !sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hidden) &&
	        (sdevice->isDiskOfType(TDEDiskDeviceType::HDD) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDROM) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDR) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDRW) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDMO) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRW) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDMRRWW) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDROM) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDRAM) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDR) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDRW) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDRDL) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDRWDL) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSR) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRW) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRDL) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDPLUSRWDL) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::BDROM) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::BDR) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::BDRW) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDROM) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDR) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::HDDVDRW) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDAudio) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::CDVideo) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::DVDVideo) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::BDVideo) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::Floppy) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::Zip) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::Jaz) ||
	         sdevice->isDiskOfType(TDEDiskDeviceType::Camera)));
}

void HwDeviceSystemTray::configChanged() {
	//
}

void HwDeviceSystemTray::contextMenuAboutToShow(TDEPopupMenu *menu)
{
	menu->clear();
	menu->setCheckable(true);

	// Device actions
	menu->insertTitle(SmallIcon("drive-harddisk-unmounted"), i18n("Storage Device Actions"));

	TDEActionMenu *openDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("open_menu"));
	TDEActionMenu *mountDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("mount_menu"));
	TDEActionMenu *unmountDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("unmount_menu"));
	TDEActionMenu *unlockDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("unlock_menu"));
	TDEActionMenu *lockDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("lock_menu"));
	TDEActionMenu *ejectDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("eject_menu"));
	TDEActionMenu *safeRemoveDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("safe_remove_menu"));
	TDEActionMenu *propertiesDeviceActionMenu = static_cast<TDEActionMenu*>(actionCollection()->action("properties_menu"));

	openDeviceActionMenu->popupMenu()->clear();
	mountDeviceActionMenu->popupMenu()->clear();
	unmountDeviceActionMenu->popupMenu()->clear();
	unlockDeviceActionMenu->popupMenu()->clear();
	lockDeviceActionMenu->popupMenu()->clear();
	ejectDeviceActionMenu->popupMenu()->clear();
	safeRemoveDeviceActionMenu->popupMenu()->clear();
	propertiesDeviceActionMenu->popupMenu()->clear();

	d->m_openMenuIndexMap.clear();
	d->m_mountMenuIndexMap.clear();
	d->m_unmountMenuIndexMap.clear();
	d->m_unlockMenuIndexMap.clear();
	d->m_lockMenuIndexMap.clear();
	d->m_ejectMenuIndexMap.clear();
	d->m_safeRemoveMenuIndexMap.clear();
	d->m_propertiesMenuIndexMap.clear();

	// Find all storage devices and add them to the popup menus
	int lastOpenIndex = -1;
	int lastMountIndex = -1;
	int lastUnmountIndex = -1;
	int lastUnlockIndex = -1;
	int lastLockIndex = -1;
	int lastEjectIndex = -1;
	int lastSafeRemoveIndex = -1;
	int lastPropertiesIndex = -1;

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
	for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
	{
		TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(hwdevice);
		if (isMonitoredDevice(sdevice))
		{
			TQString friendlyName = sdevice->diskLabel();
			if (friendlyName.isEmpty())
			{
				friendlyName = sdevice->friendlyName();
			}

			if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) || sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted))
			{
				if (sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt))
				{
					lastLockIndex = lockDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
					        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
					lockDeviceActionMenu->popupMenu()->connectItem(lastLockIndex, this, TQT_SLOT(slotLockDevice(int)));
					d->m_lockMenuIndexMap[lastLockIndex] = sdevice->diskUUID();
					if (d->m_lockMenuIndexMap[lastLockIndex] == "")
					{
						d->m_lockMenuIndexMap[lastLockIndex] = sdevice->systemPath();
					}
				}
				else
				{
					lastUnlockIndex = unlockDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
					        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
					unlockDeviceActionMenu->popupMenu()->connectItem(lastUnlockIndex, this, TQT_SLOT(slotUnlockDevice(int)));
					d->m_unlockMenuIndexMap[lastUnlockIndex] = sdevice->diskUUID();
					if (d->m_unlockMenuIndexMap[lastUnlockIndex] == "")
					{
						d->m_unlockMenuIndexMap[lastUnlockIndex] = sdevice->systemPath();
					}
				}
			}

			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable))
			{
				if (sdevice->mountPath().isEmpty())
				{
					lastMountIndex = mountDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
					        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
					mountDeviceActionMenu->popupMenu()->connectItem(lastMountIndex, this, TQT_SLOT(slotMountDevice(int)));
					d->m_mountMenuIndexMap[lastMountIndex] = sdevice->diskUUID();
					if (d->m_mountMenuIndexMap[lastMountIndex] == "")
					{
						d->m_mountMenuIndexMap[lastMountIndex] = sdevice->systemPath();
					}
				}
				else
				{
					lastUnmountIndex = unmountDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
					        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
					unmountDeviceActionMenu->popupMenu()->connectItem(lastUnmountIndex, this, TQT_SLOT(slotUnmountDevice(int)));
					d->m_unmountMenuIndexMap[lastUnmountIndex] = sdevice->diskUUID();
					if (d->m_unmountMenuIndexMap[lastMountIndex] == "")
					{
						d->m_unmountMenuIndexMap[lastMountIndex] = sdevice->systemPath();
					}
				}

				// Both mounted and unmounted disks can be opened
				lastOpenIndex = openDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
				        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
				openDeviceActionMenu->popupMenu()->connectItem(lastOpenIndex, this, TQT_SLOT(slotOpenDevice(int)));
				d->m_openMenuIndexMap[lastOpenIndex] = sdevice->diskUUID();
				if (d->m_openMenuIndexMap[lastOpenIndex] == "")
				{
					d->m_openMenuIndexMap[lastOpenIndex] = sdevice->systemPath();
				}
			}

			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable) ||
					sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hotpluggable))
			{
				lastEjectIndex = ejectDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
				        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
				ejectDeviceActionMenu->popupMenu()->connectItem(lastEjectIndex, this, TQT_SLOT(slotEjectDevice(int)));
				d->m_ejectMenuIndexMap[lastEjectIndex] = sdevice->diskUUID();
				if (d->m_ejectMenuIndexMap[lastEjectIndex] == "")
				{
					d->m_ejectMenuIndexMap[lastEjectIndex] = sdevice->systemPath();
				}

				lastSafeRemoveIndex = safeRemoveDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
				        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
				safeRemoveDeviceActionMenu->popupMenu()->connectItem(lastSafeRemoveIndex, this, TQT_SLOT(slotSafeRemoveDevice(int)));
				d->m_safeRemoveMenuIndexMap[lastSafeRemoveIndex] = sdevice->diskUUID();
				if (d->m_safeRemoveMenuIndexMap[lastSafeRemoveIndex] == "")
				{
					d->m_safeRemoveMenuIndexMap[lastSafeRemoveIndex] = sdevice->systemPath();
				}
			}

			lastPropertiesIndex = propertiesDeviceActionMenu->popupMenu()->insertItem(hwdevice->icon(TDEIcon::SizeSmall),
			        i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()));
			propertiesDeviceActionMenu->popupMenu()->connectItem(lastPropertiesIndex, this, TQT_SLOT(slotPropertiesDevice(int)));
			d->m_propertiesMenuIndexMap[lastPropertiesIndex] = sdevice->diskUUID();
			if (d->m_propertiesMenuIndexMap[lastPropertiesIndex] == "")
			{
				d->m_propertiesMenuIndexMap[lastPropertiesIndex] = sdevice->systemPath();
			}
		}
	}

	openDeviceActionMenu->setEnabled(lastOpenIndex != -1);
	mountDeviceActionMenu->setEnabled(lastMountIndex != -1);
	unmountDeviceActionMenu->setEnabled(lastUnmountIndex != -1);
	unlockDeviceActionMenu->setEnabled(lastUnlockIndex != -1);
	lockDeviceActionMenu->setEnabled(lastLockIndex != -1);
	ejectDeviceActionMenu->setEnabled(lastEjectIndex != -1);
	safeRemoveDeviceActionMenu->setEnabled(lastSafeRemoveIndex != -1);
	propertiesDeviceActionMenu->setEnabled(lastPropertiesIndex != -1);

	if (lastOpenIndex != -1)
	{
		openDeviceActionMenu->plug(menu);
	}
	if (lastMountIndex != -1)
	{
		mountDeviceActionMenu->plug(menu);
	}
	if (lastUnmountIndex != -1)
	{
		unmountDeviceActionMenu->plug(menu);
	}
	if (lastUnlockIndex != -1)
	{
		unlockDeviceActionMenu->plug(menu);
	}
	if (lastLockIndex != -1)
	{
		lockDeviceActionMenu->plug(menu);
	}
	if (lastEjectIndex != -1)
	{
		ejectDeviceActionMenu->plug(menu);
	}
	if (lastSafeRemoveIndex != -1)
	{
		safeRemoveDeviceActionMenu->plug(menu);
	}
	if (lastPropertiesIndex != -1)
	{
		propertiesDeviceActionMenu->plug(menu);
	}

	// Global Configuration
	menu->insertTitle(SmallIcon("configure"), i18n("Global Configuration"));

	TDEAction *actHardwareConfig = new TDEAction(i18n("Show Device Manager..."), SmallIconSet("kcmpci"), TDEShortcut(), this, TQT_SLOT(slotHardwareConfig()), actionCollection());
	actHardwareConfig->plug(menu);

	TDEAction *actShortcutKeys = new TDEAction(i18n("Configure Shortcut Keys..."), SmallIconSet("configure"), TDEShortcut(), this, TQT_SLOT(slotEditShortcutKeys()), actionCollection());
	actShortcutKeys->plug(menu);

	// Help & Quit
	menu->insertSeparator();
	menu->insertItem(SmallIcon("help"), KStdGuiItem::help().text(), d->m_help->menu());
	TDEAction *quitAction = actionCollection()->action(KStdAction::name(KStdAction::Quit));
	quitAction->plug(menu);
}

void HwDeviceSystemTray::slotOpenDevice(int parameter)
{
	TQString uuid = d->m_openMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				if (sdevice->isDiskOfType(TDEDiskDeviceType::Camera))
				{
					new KRun(TQString("media:/%1").arg(sdevice->friendlyName()));
				}
				else
				{
					new KRun(TQString("system:/media/%1").arg(TQFileInfo(sdevice->deviceNode()).baseName(true)));
				}
				return;
			}
		}
	}
}

void HwDeviceSystemTray::slotMountDevice(int parameter)
{
	TQString uuid = d->m_mountMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				if (sdevice->mountPath().isEmpty())
				{
					TDEProcess proc;
					proc << "tdeio_media_mounthelper" << "-m" << sdevice->deviceNode();
					if (!proc.start(TDEProcess::DontCare))
					{
						KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
								i18n("Device monitor"));
					}
				}
			}
		}
	}
}

void HwDeviceSystemTray::slotUnmountDevice(int parameter)
{
	TQString uuid = d->m_unmountMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				if (!sdevice->mountPath().isEmpty())
				{
					TDEProcess proc;
					proc << "tdeio_media_mounthelper" << "-u" << sdevice->deviceNode();
					if (!proc.start(TDEProcess::DontCare))
					{
						KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
								i18n("Device monitor"));
					}
				}
			}
		}
	}
}

void HwDeviceSystemTray::slotUnlockDevice(int parameter)
{
	TQString uuid = d->m_unlockMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				TDEProcess proc;
				proc << "tdeio_media_mounthelper" << "-k" << sdevice->deviceNode();
				if (!proc.start(TDEProcess::DontCare))
				{
					KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
							i18n("Device monitor"));
				}
			}
		}
	}
}

void HwDeviceSystemTray::slotLockDevice(int parameter)
{
	TDEGenericDevice *hwdevice;
	TQString uuid = d->m_lockMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next()) {
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				TDEProcess proc;
				proc << "tdeio_media_mounthelper" << "-l" << sdevice->deviceNode();
				if (!proc.start(TDEProcess::DontCare))
				{
					KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
							    i18n("Device monitor"));
				}
		  }
		}
	}
}

void HwDeviceSystemTray::slotEjectDevice(int parameter)
{
	TQString uuid = d->m_ejectMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				TDEProcess proc;
				proc << "tdeio_media_mounthelper" << "-e" << sdevice->deviceNode();
				if (!proc.start(TDEProcess::DontCare))
				{
					KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
							    i18n("Device monitor"));
				}
			}
		}
	}
}

void HwDeviceSystemTray::slotSafeRemoveDevice(int parameter)
{
	TQString uuid = d->m_safeRemoveMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				TDEProcess proc;
				proc << "tdeio_media_mounthelper" << "-s" << sdevice->deviceNode();
				if (!proc.start(TDEProcess::DontCare))
				{
					KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
							    i18n("Device monitor"));
				}
			}
		}
	}
}

void HwDeviceSystemTray::slotPropertiesDevice(int parameter)
{
	TQString uuid = d->m_propertiesMenuIndexMap[parameter];
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				new KPropertiesDialog(KURL(TQString("media:/%1").arg(sdevice->deviceNode())));
				return;
			}
		}
	}
}

void HwDeviceSystemTray::slotHardwareConfig() {
	KCMultiDialog *kcm = new KCMultiDialog(KDialogBase::Plain, i18n("Configure"), this);

	kcm->addModule("hwmanager");
	kcm->setPlainCaption(i18n("Configure Devices"));
	kcm->exec();
}

void HwDeviceSystemTray::slotSettingsChanged(int category) {
	if (category == (int) TDEApplication::SETTINGS_SHORTCUTS) {
		globalKeys->readSettings();
		globalKeys->updateConnections();
	}
}

void HwDeviceSystemTray::slotEditShortcutKeys() {
	ConfigDialog *dlg = new ConfigDialog(globalKeys, true);

	if (dlg->exec() == TQDialog::Accepted) {
		dlg->commitShortcuts();
		globalKeys->writeSettings(0, true);
		globalKeys->updateConnections();
	}

	delete dlg;
}

void HwDeviceSystemTray::doDiskNotifications(bool scanOnly)
{
	TDEConfig config("mediamanagerrc");
	config.setGroup("Global");
	bool popupEnable = config.readBoolEntry("DeviceMonitorPopupsEnabled", true);

	// Scan devices for changes and notify new devices if needed.
	// This is necessary because the device information may not be available
	// at the time the hardwareAdded signal is emitted
	TQMap<TQString, KnownDiskDeviceInfo> oldKnownDevices = d->m_knownDiskDevices;
	d->m_knownDiskDevices.clear();
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
	for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
	{
		TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
		if (isMonitoredDevice(sdevice))
		{
			TQString sysPath = sdevice->systemPath();
			if (oldKnownDevices.contains(sysPath))
			{
				d->m_knownDiskDevices[sysPath] = oldKnownDevices[sysPath];
				oldKnownDevices.remove(sysPath);
			}
			else
			{
				TQString friendlyName = sdevice->diskLabel();
				if (friendlyName.isEmpty())
				{
					friendlyName = sdevice->friendlyName();
				}
				d->m_knownDiskDevices[sysPath] = { friendlyName, sdevice->deviceNode() };
				if (!scanOnly && popupEnable)
				{
					d->m_hardwareNotifierContainer->displayMessage(
							i18n("A disk device has been added!"),
							i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()),
							SmallIcon("drive-harddisk-unmounted"), 0, 0, "ADD: " + sysPath);
				}
			}
		}
	}
	// Notify devices which have been removed, if necessary
	if (!scanOnly && popupEnable)
	{
		TQMap<TQString, KnownDiskDeviceInfo>::ConstIterator delIt;
		for (delIt = oldKnownDevices.begin(); delIt != oldKnownDevices.end(); delIt++)
		{
			d->m_hardwareNotifierContainer->displayMessage(
					i18n("A disk device has been removed!"),
					i18n("%1 (%2)").arg(delIt.data().friendlyName, delIt.data().node),
					SmallIcon("drive-harddisk-unmounted"), 0, 0, "REMOVE: " + delIt.key());
		}
	}
}

void HwDeviceSystemTray::deviceAdded(TDEGenericDevice* device)
{
	if (device->type() == TDEGenericDeviceType::Disk)
	{
		TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(device);
		// The device information may not be available at the time the hardwareAdded signal is emitted.
		// In such case ignore the event and handle that at the subsequent hardwareUpdate signal emission.
		TQString sysPath = sdevice->systemPath();
		if (isMonitoredDevice(sdevice) && !d->m_knownDiskDevices.contains(sysPath))
		{
			TQString friendlyName = sdevice->diskLabel();
			if (friendlyName.isEmpty())
			{
				friendlyName = sdevice->friendlyName();
			}
			d->m_knownDiskDevices[sysPath] = { friendlyName, sdevice->deviceNode() };
			TDEConfig config("mediamanagerrc");
			config.setGroup("Global");
			if (config.readBoolEntry("DeviceMonitorPopupsEnabled", true))
			{
				d->m_hardwareNotifierContainer->displayMessage(
						i18n("A disk device has been added!"),
						i18n("%1 (%2)").arg(friendlyName, sdevice->deviceNode()),
						SmallIcon("drive-harddisk-unmounted"), 0, 0, "ADD: " + sysPath);
			}
		}
	}
}

void HwDeviceSystemTray::deviceRemoved(TDEGenericDevice* device)
{
	if (device->type() == TDEGenericDeviceType::Disk)
	{
		TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(device);
		TQString sysPath = sdevice->systemPath();
		if (isMonitoredDevice(sdevice) && d->m_knownDiskDevices.contains(sysPath))
		{
			TDEConfig config("mediamanagerrc");
			config.setGroup("Global");
			if (config.readBoolEntry("DeviceMonitorPopupsEnabled", true))
			{
				d->m_hardwareNotifierContainer->displayMessage(
						i18n("A disk device has been removed!"),
						i18n("%1 (%2)").arg(d->m_knownDiskDevices[sysPath].friendlyName, d->m_knownDiskDevices[sysPath].node),
						SmallIcon("drive-harddisk-unmounted"), 0, 0, "REMOVE: " + sysPath);
			}
			d->m_knownDiskDevices.remove(sysPath);
		}
	}
}

void HwDeviceSystemTray::deviceChanged(TDEGenericDevice* device)
{
	doDiskNotifications(false);
}

void HwDeviceSystemTray::devicePopupClicked(KPassivePopup* popup, TQPoint point, TQString uuid) {
	TDEGenericDevice *hwdevice;
	if (uuid.startsWith("ADD: ")) {
		uuid = uuid.right(uuid.length() - strlen("ADD: "));
		if (uuid != "") {
			TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
			TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
			for (hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next()) {
				TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(hwdevice);
				if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid)) {
					// Pop up full media notification dialog
					DCOPClient* dcopClient = TDEApplication::dcopClient();
					TQByteArray data;
					TQDataStream arg(data, IO_WriteOnly);
					bool allowNotification = true;
					if (sdevice->isDiskOfType(TDEDiskDeviceType::Camera)) {
						arg << sdevice->friendlyName();
					}
					else {
						arg << TQFileInfo(sdevice->deviceNode()).baseName(true);
					}
					arg << allowNotification;
					dcopClient->send("kded", "medianotifier", "onMediumChange(TQString, bool)", data);
					return;
				}
			}
		}
	}
}

void HwDeviceSystemTray::slotHelpContents() {
	kapp->invokeHelp(TQString::null, "hwdevicetray");
}

#include "hwdevicetray.moc"
