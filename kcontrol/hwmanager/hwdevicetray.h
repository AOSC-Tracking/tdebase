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

#ifndef TDEHWDEVICETRAY_H
#define TDEHWDEVICETRAY_H

#include <tqptrlist.h>

#include <ksystemtray.h>
#include <kglobalaccel.h>
#include <ksimpleconfig.h>
#include <tdepassivepopupstack.h>

#include <tdehardwaredevices.h>
#include <tdestoragedevice.h>

class KHelpMenu;
class PasswordDlg;
class TDEPopupMenu;

typedef TQMap<int, TQString> TQStringMap;

class HwDeviceSystemTray :  public KSystemTray
{
	Q_OBJECT

public:
	HwDeviceSystemTray(TQWidget* parent = 0, const char *name = 0);
	~HwDeviceSystemTray();
	TDEGlobalAccel *globalKeys;

	virtual void contextMenuAboutToShow(TDEPopupMenu* menu);

	void configChanged();

protected slots:
	void slotHardwareConfig();
	void slotEditShortcutKeys();
	void slotSettingsChanged(int category);
	void slotHelpContents();

	void slotOpenDevice(int parameter);
	void slotMountDevice(int parameter);
	void slotUnmountDevice(int parameter);
	void slotUnlockDevice(int parameter);
	void slotLockDevice(int parameter);
	void slotEjectDevice(int parameter);
	void slotSafeRemoveDevice(int parameter);
	void slotPropertiesDevice(int parameter);

protected:
	void mousePressEvent(TQMouseEvent *e);
	void resizeEvent(TQResizeEvent *);
	void showEvent(TQShowEvent *);

private slots:
	void _quit();
	void deviceAdded(TDEGenericDevice*);
	void deviceRemoved(TDEGenericDevice*);
	void deviceChanged(TDEGenericDevice*);
	void devicePopupClicked(KPassivePopup*, TQPoint, TQString);
	void doDiskNotifications(bool scanOnly);

private:
	bool isMonitoredDevice(TDEStorageDevice* sdevice);

private:
	void populateMenu(TDEPopupMenu* menu);
	void resizeTrayIcon();

	bool m_popupUp;
	KHelpMenu* m_help;

	TQWidget* m_parent;
	TDEPassivePopupStackContainer* m_hardwareNotifierContainer;

	TQStringMap m_openMenuIndexMap;
	TQStringMap m_mountMenuIndexMap;
	TQStringMap m_unmountMenuIndexMap;
	TQStringMap m_unlockMenuIndexMap;
	TQStringMap m_lockMenuIndexMap;
	TQStringMap m_ejectMenuIndexMap;
	TQStringMap m_safeRemoveMenuIndexMap;
	TQStringMap m_propertiesMenuIndexMap;
	TDEPopupMenu* m_menu;
	KSimpleConfig *r_config;

	struct KnownDiskDeviceInfo
	{
		TQString friendlyName;
		TQString node;
	};
	TQMap<TQString, KnownDiskDeviceInfo> m_knownDiskDevices;
};

#endif
