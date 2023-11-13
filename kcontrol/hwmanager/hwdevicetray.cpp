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
#include <tqvaluevector.h>

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


// Storage Device Action
namespace SDActions
{
	// use 'int' as underlying type to avoid exposing a bunch of unnecessary
	// enums/structs in the class header file private methods' signature
	// Note: must start from 0 because the Type value is also used as index
	// of a TQValueVector
	enum Type : int
	{
		Open = 0,
		Mount,
		Unmount,
		Unlock,
		Lock,
		Eject,
		SafeRemove,
		Properties
	};

	struct Details
	{
		const char *iconName;
		const char *actionName;
		const char *collectionName;
	};

	// Allows to use a for loop to iterate over all action types
	static const Type All[] = { Open, Mount, Unmount, Unlock, Lock, Eject, SafeRemove, Properties };

	static const TQMap<Type, Details> populateMap()
	{
		TQMap<Type, Details> map;
		map[Open] = { "window-new", I18N_NOOP("Open"), "open_menu" };
		map[Mount] = { "drive-harddisk-mounted", I18N_NOOP("Mount"), "mount_menu" };
		map[Unmount] = { "drive-harddisk-unmounted", I18N_NOOP("Unmount"), "unmount_menu" };
		map[Unlock] = { "decrypted", I18N_NOOP("Unlock"), "unlock_menu" };
		map[Lock] = { "encrypted", I18N_NOOP("Lock"), "lock_menu" };
		map[Eject] = { "player_eject", I18N_NOOP("Eject"), "eject_menu" };
		map[SafeRemove] = { "player_safe_removal", I18N_NOOP("Safe remove"), "safe_remove_menu" };
		map[Properties] = { "edit", I18N_NOOP("Properties"), "properties_menu" };
		return map;
	}

	static const TQMap<Type, Details> Data = populateMap();
}

// Storage Device Action Menu Entry, representing an action
// and the storage device on which to perform it
struct SDActionMenuEntry
{
	SDActions::Type actionType;
	TQString uuid;
};

struct KnownDiskDeviceInfo
{
	TQString deviceLabel;
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
	TDEAction *m_deviceManagerAction;
	TDEAction *m_quitAction;
	TDEAction *m_shortcutKeysAction;
	KHelpMenu *m_helpMenu;
	TDEPopupMenu *m_LMBMenu;
	TDEPopupMenu *m_RMBMenu;

	TQMap<int, SDActionMenuEntry> m_actionMenuEntryMap;
	TQMap<SDActions::Type, TDEActionMenu*> m_RMBActionMenuMap;

	TQMap<TQString, KnownDiskDeviceInfo> m_knownDiskDevices;

	TDEPassivePopupStackContainer *m_hardwareNotifierContainer;
};


HwDeviceSystemTray::HwDeviceSystemTray(TQWidget *parent, const char *name)
	: KSystemTray(parent, name), d(new HwDeviceSystemTrayPrivate())
{
	// Create notifier
	d->m_hardwareNotifierContainer = new TDEPassivePopupStackContainer();
	connect(d->m_hardwareNotifierContainer, TQT_SIGNAL(popupClicked(KPassivePopup*, TQPoint, TQString)), this, TQT_SLOT(devicePopupClicked(KPassivePopup*, TQPoint, TQString)));

	initMenus();

	setPixmap(KSystemTray::loadIcon("hwinfo"));
	setAlignment(TQt::AlignHCenter | TQt::AlignVCenter);
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
		case TQt::LeftButton:
			populateLMBMenu();
			d->m_LMBMenu->popup(e->globalPos());
			break;

		case TQt::MidButton:
			TQTimer::singleShot(0, this, TQT_SLOT(slotHardwareConfig()));
			break;

		case TQt::RightButton:
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

void HwDeviceSystemTray::initMenus()
{
	// RMB menu
	d->m_RMBMenu = contextMenu();

	// Device actions
	for (const SDActions::Type &actionType : SDActions::All)
	{
		SDActions::Details ad = SDActions::Data[actionType];
		d->m_RMBActionMenuMap.insert(actionType, new TDEActionMenu(i18n(ad.actionName),
	        SmallIcon(ad.iconName, TQIconSet::Automatic), actionCollection(), ad.collectionName));
	}

	// Global Configuration
	d->m_deviceManagerAction = new TDEAction(i18n("Show Device Manager..."), SmallIconSet("kcmpci"),
	        TDEShortcut(), this, TQT_SLOT(slotHardwareConfig()), actionCollection());
	d->m_shortcutKeysAction = new TDEAction(i18n("Configure Shortcut Keys..."), SmallIconSet("configure"),
	        TDEShortcut(), this, TQT_SLOT(slotEditShortcutKeys()), actionCollection());

	// Help & Quit
	d->m_helpMenu = new KHelpMenu(this, TDEGlobal::instance()->aboutData(), false, actionCollection());
	d->m_helpMenu->menu()->connectItem(KHelpMenu::menuHelpContents, this, TQT_SLOT(slotHelpContents()));
	d->m_quitAction = actionCollection()->action(KStdAction::name(KStdAction::Quit));

	// LMB menu
	d->m_LMBMenu = new TDEPopupMenu(this);
}

TQString HwDeviceSystemTray::getDeviceLabel(TDEStorageDevice *sdevice)
{
	if (!sdevice)
	{
		return TQString::null;
	}

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("properties", sdevice->deviceNode());
	TQString deviceLabel = TQString::null;
	if (reply.isValid())
	{
		// TODO R14.2.0: make sure the reply is a valid Medium
		// once the media library is part of tdelibs
		TQStringList properties = reply;
		if (properties.size() >= 4)
		{
			deviceLabel = properties[3]; // medium label
		}
	}

	if (deviceLabel.isEmpty())
	{
		deviceLabel = !sdevice->diskLabel().isEmpty() ? sdevice->diskLabel() : sdevice->friendlyName();
		deviceLabel += " (" + sdevice->deviceNode() + ")";
	}

	return deviceLabel;
}

void HwDeviceSystemTray::addDeviceToLMBMenu(TDEStorageDevice *sdevice, const int type,
        TDEActionMenu *actionMenu, int &actionMenuIdx)
{
	TQString uuid = !sdevice->diskUUID().isEmpty() ? sdevice->diskUUID() : sdevice->systemPath();
	SDActions::Type actionType = (SDActions::Type)type;
	SDActions::Details ad = SDActions::Data[actionType];
	actionMenu->popupMenu()->insertItem(SmallIcon(ad.iconName), i18n(ad.actionName), actionMenuIdx);
	actionMenu->popupMenu()->connectItem(actionMenuIdx, this,
	        TQT_SLOT(slotExecuteDeviceAction(int)));
	d->m_actionMenuEntryMap[actionMenuIdx++] = { actionType, uuid };
}

void HwDeviceSystemTray::addDeviceToRMBMenu(TDEStorageDevice *sdevice, const int type, int &actionMenuIdx)
{
	TQString uuid = !sdevice->diskUUID().isEmpty() ? sdevice->diskUUID() : sdevice->systemPath();
	SDActions::Type actionType = (SDActions::Type)type;
	TDEActionMenu *actionMenu = d->m_RMBActionMenuMap[actionType];
	actionMenu->popupMenu()->insertItem(sdevice->icon(TDEIcon::SizeSmall),
	        getDeviceLabel(sdevice), actionMenuIdx);
	actionMenu->popupMenu()->connectItem(actionMenuIdx, this,
	        TQT_SLOT(slotExecuteDeviceAction(int)));
	actionMenu->setEnabled(true);
	d->m_actionMenuEntryMap[actionMenuIdx++] = { actionType, uuid };
}

void HwDeviceSystemTray::contextMenuAboutToShow(TDEPopupMenu *menu)
{
	d->m_RMBMenu = menu;
	menu->clear();

	for (const SDActions::Type &actionType : SDActions::All)
	{
		TDEActionMenu *actionMenu = d->m_RMBActionMenuMap[actionType];
		actionMenu->popupMenu()->clear();
		actionMenu->setEnabled(false);
		actionMenu->unplug(d->m_RMBMenu);
	}

	d->m_actionMenuEntryMap.clear();

	// Find all storage devices, sort them by label and add them to the popup menus
	TQValueVector<TQMap<TQString, TDEStorageDevice*>*> rmbMenuEntries(sizeof(SDActions::All) / sizeof(SDActions::Type), nullptr);
	for (size_t idx = 0; idx < rmbMenuEntries.size(); ++idx)
	{
		rmbMenuEntries[idx] = new TQMap<TQString, TDEStorageDevice*>();
	}
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
	for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
	{
		TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
		if (isMonitoredDevice(sdevice))
		{
			TQString deviceLabel = getDeviceLabel(sdevice);
			if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) ||
			    sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted))
			{
				if (sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt))
				{
					(*rmbMenuEntries[SDActions::Lock])[deviceLabel] = sdevice;
				}
				else
				{
					(*rmbMenuEntries[SDActions::Unlock])[deviceLabel] = sdevice;
				}
			}

			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable))
			{
				if (sdevice->mountPath().isEmpty())
				{
					(*rmbMenuEntries[SDActions::Mount])[deviceLabel] = sdevice;
				}
				else
				{
					(*rmbMenuEntries[SDActions::Unmount])[deviceLabel] = sdevice;
				}

				// Mounted and unmounted disks can also be opened
				(*rmbMenuEntries[SDActions::Open])[deviceLabel] = sdevice;
			}

			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable) ||
			    sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hotpluggable))
			{
				(*rmbMenuEntries[SDActions::Eject])[deviceLabel] = sdevice;

				(*rmbMenuEntries[SDActions::SafeRemove])[deviceLabel] = sdevice;
			}

			(*rmbMenuEntries[SDActions::Properties])[deviceLabel] = sdevice;
		}
	}

	// Plug in meaningful action menus
	d->m_RMBMenu->insertTitle(SmallIcon("drive-harddisk-unmounted"), i18n("Storage Device Actions"), 0);
	int actionMenuIdx = 0;
	for (const SDActions::Type &actionType : SDActions::All)
	{
		TDEActionMenu *actionMenu = d->m_RMBActionMenuMap[actionType];
		for (TDEStorageDevice *sdevice : *rmbMenuEntries[actionType])
		{
			addDeviceToRMBMenu(sdevice, actionType, actionMenuIdx);
		}
		if (actionMenu->isEnabled())
		{
			actionMenu->plug(d->m_RMBMenu);
		}
		delete rmbMenuEntries[actionType];
		rmbMenuEntries[actionType] = nullptr;
	}

	// Global Configuration
	menu->insertTitle(SmallIcon("configure"), i18n("Global Configuration"));

	d->m_deviceManagerAction->plug(menu);
	d->m_shortcutKeysAction->plug(menu);

	// Help & Quit
	menu->insertSeparator();
	menu->insertItem(SmallIcon("help"), KStdGuiItem::help().text(), d->m_helpMenu->menu());
	d->m_quitAction->plug(menu);
}

void HwDeviceSystemTray::populateLMBMenu()
{
	d->m_LMBMenu->clear();
	d->m_LMBMenu->insertTitle(SmallIcon("drive-harddisk-unmounted"), i18n("Storage Devices"), 0);

	d->m_actionMenuEntryMap.clear();
	int actionMenuIdx = 0;

	// Find all storage devices and add them to the popup menus
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
	TQMap<TQString, TDEActionMenu*> lmbMenuEntries;
	for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
	{
		TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
		if (isMonitoredDevice(sdevice) &&
		    (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) ||
		     sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted) ||
		     sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable) ||
		     sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable) ||
		     sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hotpluggable)))
		{
			TQString deviceLabel = getDeviceLabel(sdevice);
			TDEActionMenu *actionMenu = new TDEActionMenu(deviceLabel,
					sdevice->icon(TDEIcon::SizeSmall));

			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable))
			{
				// Mounted and unmounted disks can also be opened
				addDeviceToLMBMenu(sdevice, SDActions::Open, actionMenu, actionMenuIdx);

				if (sdevice->mountPath().isEmpty())
				{
					addDeviceToLMBMenu(sdevice, SDActions::Mount, actionMenu, actionMenuIdx);
				}
				else
				{
					addDeviceToLMBMenu(sdevice, SDActions::Unmount, actionMenu, actionMenuIdx);
				}
			}

			if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) ||
					sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted))
			{
				if (sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt))
				{
					addDeviceToLMBMenu(sdevice, SDActions::Lock, actionMenu, actionMenuIdx);
				}
				else
				{
					addDeviceToLMBMenu(sdevice, SDActions::Unlock, actionMenu, actionMenuIdx);
				}
			}


			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable) ||
					sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hotpluggable))
			{
				addDeviceToLMBMenu(sdevice, SDActions::Eject, actionMenu, actionMenuIdx);

				addDeviceToLMBMenu(sdevice, SDActions::SafeRemove, actionMenu, actionMenuIdx);
			}

			addDeviceToLMBMenu(sdevice, SDActions::Properties, actionMenu, actionMenuIdx);

			lmbMenuEntries[deviceLabel] = actionMenu;
		}
	}
	// Insert menu entries in sorted order
	for (TDEActionMenu *am : lmbMenuEntries)
	{
		am->plug(d->m_LMBMenu);
	}
}

void HwDeviceSystemTray::slotExecuteDeviceAction(int parameter)
{
	TQString uuid = d->m_actionMenuEntryMap[parameter].uuid;
	int actionType = d->m_actionMenuEntryMap[parameter].actionType;
	if (!uuid.isEmpty())
	{
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList diskDeviceList = hwdevices->listByDeviceClass(TDEGenericDeviceType::Disk);
		for (TDEGenericDevice *hwdevice = diskDeviceList.first(); hwdevice; hwdevice = diskDeviceList.next())
		{
			TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
			if ((sdevice->diskUUID() == uuid) || (sdevice->systemPath() == uuid))
			{
				if (actionType == SDActions::Open)
				{
					if (sdevice->isDiskOfType(TDEDiskDeviceType::Camera))
					{
						new KRun(TQString("media:/%1").arg(sdevice->friendlyName()));
					}
					else
					{
						new KRun(TQString("system:/media/%1").arg(TQFileInfo(sdevice->deviceNode()).baseName(true)));
					}
				}
				else if (actionType == SDActions::Properties)
				{
					new KPropertiesDialog(KURL(TQString("media:/%1").arg(sdevice->deviceNode())));
				}
				else
				{
					TQString opType = TQString::null;
					if (actionType == SDActions::Mount)           { opType = "-m"; }
					else if (actionType == SDActions::Unmount)    { opType = "-u"; }
					else if (actionType == SDActions::Unlock)     { opType = "-k"; }
					else if (actionType == SDActions::Lock)       { opType = "-l"; }
					else if (actionType == SDActions::Eject)      { opType = "-e"; }
					else if (actionType == SDActions::SafeRemove) { opType = "-s"; }

					if (!opType.isEmpty())
					{
						TDEProcess proc;
						proc << "tdeio_media_mounthelper" << opType << sdevice->deviceNode();
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
				TQString deviceLabel = getDeviceLabel(sdevice);
				d->m_knownDiskDevices[sysPath] = { deviceLabel, sdevice->deviceNode() };
				if (!scanOnly && popupEnable)
				{
					d->m_hardwareNotifierContainer->displayMessage(
							i18n("A disk device has been added!"), deviceLabel,
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
					i18n("A disk device has been removed!"), delIt.data().deviceLabel,
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
			TQString deviceLabel = getDeviceLabel(sdevice);
			d->m_knownDiskDevices[sysPath] = { deviceLabel, sdevice->deviceNode() };
			TDEConfig config("mediamanagerrc");
			config.setGroup("Global");
			if (config.readBoolEntry("DeviceMonitorPopupsEnabled", true))
			{
				d->m_hardwareNotifierContainer->displayMessage(
						i18n("A disk device has been added!"), deviceLabel,
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
				TQString deviceLabel = getDeviceLabel(sdevice);
				d->m_hardwareNotifierContainer->displayMessage(
						i18n("A disk device has been removed!"), deviceLabel,
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
		TDEConfig config("mediamanagerrc");
		config.setGroup("Global");
		if (config.readBoolEntry("NotificationPopupsEnabled", true))
		{
			return;
		}

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
