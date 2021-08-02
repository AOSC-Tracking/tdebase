/**
 * hwmanager.cpp
 *
 * Copyright (c) 2009-2010 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqpushbutton.h>

#include <dcopclient.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdepopupmenu.h>
#include <kinputdialog.h>
#include <kurlrequester.h>
#include <kgenericfactory.h>

#include <unistd.h>
#include <kpassdlg.h>
#include <ksimpleconfig.h>
#include <string>
#include <stdio.h>
#include <tqstring.h>

#include <tdecryptographiccarddevice.h>

#include "hwmanager.h"

using namespace std;

/**** DLL Interface ****/
typedef KGenericFactory<TDEHWManager, TQWidget> TDEHWManagerFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_hwmanager, TDEHWManagerFactory("kcmhwmanager") )

KSimpleConfig *config;
KSimpleConfig *systemconfig;

/**** TDEHWManager ****/

TDEHWManager::TDEHWManager(TQWidget *parent, const char *name, const TQStringList &)
  : TDECModule(TDEHWManagerFactory::instance(), parent, name)
{
	TQVBoxLayout *layout = new TQVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
	config = new KSimpleConfig( TQString::fromLatin1( "hwmanagerrc" ));
	systemconfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdehw/hwmanagerrc" ));

	TDEAboutData *about =
	new TDEAboutData(I18N_NOOP("kcmhwmanager"), I18N_NOOP("TDE Device Manager"), 0,
	I18N_NOOP("Device Manager\n\n"
	          "Can be used to get all kind of informations about your devices on your system,\n"
	          "shows which drivers are used by them and allows to change device settings."),
	TDEAboutData::License_GPL,
	I18N_NOOP("(c) 2012 Timothy Pearson\n"
	          "(c) 2019 The Trinity Desktop Project"));
	about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
	setAboutData( about );

	base = new TDEHWManagerBase(this);
	layout->add(base);

	base->deviceFilter->setListView(base->deviceTree);

	setRootOnlyMsg(i18n("<b>Device settings are system wide, and therefore require administrator access</b><br>To alter the system's device settings, click on the \"Administrator Mode\" button below."));
	setUseRootOnlyMsg(true);

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	hwdevices->setTriggerlessHardwareUpdatesEnabled(true);

	connect(base->showByConnection, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
	connect(base->showByConnection, TQT_SIGNAL(clicked()), TQT_SLOT(populateTreeView()));

	connect(hwdevices, TQT_SIGNAL(hardwareAdded(TDEGenericDevice*)), this, TQT_SLOT(populateTreeView()));
	connect(hwdevices, TQT_SIGNAL(hardwareRemoved(TDEGenericDevice*)), this, TQT_SLOT(populateTreeView()));
	connect(hwdevices, TQT_SIGNAL(hardwareUpdated(TDEGenericDevice*)), this, TQT_SLOT(deviceChanged(TDEGenericDevice*)));

	load();

	populateTreeView();
}

TDEHWManager::~TDEHWManager()
{
	delete config;
	delete systemconfig;
}

void TDEHWManager::load()
{
	load( false );
}

void TDEHWManager::load(bool useDefaults )
{
	emit changed(useDefaults);
}

void TDEHWManager::save()
{
	emit changed(false);
}

void TDEHWManager::defaults()
{
	load( true );
}

void TDEHWManager::populateTreeView()
{
	bool show_by_connection = base->showByConnection->isChecked();

	// Figure out which device, if any, was selected
	TQString selected_syspath;
	DeviceIconItem* selItem = dynamic_cast<DeviceIconItem*>(base->deviceTree->selectedItem());
	if (selItem) {
		if (selItem->device()) {
			selected_syspath = selItem->device()->systemPath();
		}
	}

	base->deviceTree->clear();

	if (show_by_connection) {
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList hwlist = hwdevices->listByDeviceClass(TDEGenericDeviceType::RootSystem);
		TDEGenericDevice *hwdevice;
		for ( hwdevice = hwlist.first(); hwdevice; hwdevice = hwlist.next() ) {
			if (hwdevice->type() == TDEGenericDeviceType::CryptographicCard) {
				TDECryptographicCardDevice* cdevice = static_cast<TDECryptographicCardDevice*>(hwdevice);
				connect(cdevice, SIGNAL(pinRequested(TQString,TDECryptographicCardDevice*)), this, SLOT(cryptographicCardPinRequested(TQString,TDECryptographicCardDevice*)));
				cdevice->enableCardMonitoring(true);
				cdevice->enablePINEntryCallbacks(true);
			}
			DeviceIconItem* item = new DeviceIconItem(base->deviceTree, hwdevice->detailedFriendlyName(), hwdevice->icon(base->deviceTree->iconSize()), hwdevice);
			if ((!selected_syspath.isNull()) && (hwdevice->systemPath() == selected_syspath)) {
				base->deviceTree->ensureItemVisible(item);
				base->deviceTree->setSelected(item, true);
			}
			populateTreeViewLeaf(item, show_by_connection, selected_syspath);
		}
	}
	else {
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		for (int i=0;i<=TDEGenericDeviceType::Last;i++) {
			if (i != TDEGenericDeviceType::Root) {
				DeviceIconItem* rootitem = new DeviceIconItem(base->deviceTree, hwdevices->getFriendlyDeviceTypeStringFromType((TDEGenericDeviceType::TDEGenericDeviceType)i), hwdevices->getDeviceTypeIconFromType((TDEGenericDeviceType::TDEGenericDeviceType)i, base->deviceTree->iconSize()), 0);
				TDEGenericDevice *hwdevice;
				TDEGenericHardwareList hwlist = hwdevices->listByDeviceClass((TDEGenericDeviceType::TDEGenericDeviceType)i);
				for ( hwdevice = hwlist.first(); hwdevice; hwdevice = hwlist.next() ) {
					if (hwdevice->type() == TDEGenericDeviceType::CryptographicCard) {
						TDECryptographicCardDevice* cdevice = static_cast<TDECryptographicCardDevice*>(hwdevice);
						connect(cdevice, SIGNAL(pinRequested(TQString,TDECryptographicCardDevice*)), this, SLOT(cryptographicCardPinRequested(TQString,TDECryptographicCardDevice*)));
						cdevice->enableCardMonitoring(true);
						cdevice->enablePINEntryCallbacks(true);
					}
					DeviceIconItem* item = new DeviceIconItem(rootitem, hwdevice->detailedFriendlyName(), hwdevice->icon(base->deviceTree->iconSize()), hwdevice);
					if ((!selected_syspath.isNull()) && (hwdevice->systemPath() == selected_syspath)) {
						base->deviceTree->ensureItemVisible(item);
						base->deviceTree->setSelected(item, true);
					}
				}
			}
		}
	}
}

void TDEHWManager::populateTreeViewLeaf(DeviceIconItem *parent, bool show_by_connection, TQString selected_syspath) {
	if (show_by_connection) {
		TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
		TDEGenericHardwareList hwlist = hwdevices->listAllPhysicalDevices();
		TDEGenericDevice *hwdevice;
		for ( hwdevice = hwlist.first(); hwdevice; hwdevice = hwlist.next() ) {
			if (hwdevice->type() == TDEGenericDeviceType::CryptographicCard) {
				TDECryptographicCardDevice* cdevice = static_cast<TDECryptographicCardDevice*>(hwdevice);
				connect(cdevice, SIGNAL(pinRequested(TQString,TDECryptographicCardDevice*)), this, SLOT(cryptographicCardPinRequested(TQString,TDECryptographicCardDevice*)));
				cdevice->enableCardMonitoring(true);
				cdevice->enablePINEntryCallbacks(true);
			}
			if (hwdevice->parentDevice() == parent->device()) {
				DeviceIconItem* item = new DeviceIconItem(parent, hwdevice->detailedFriendlyName(), hwdevice->icon(base->deviceTree->iconSize()), hwdevice);
				if ((!selected_syspath.isNull()) && (hwdevice->systemPath() == selected_syspath)) {
					base->deviceTree->ensureItemVisible(item);
					base->deviceTree->setSelected(item, true);
				}
				populateTreeViewLeaf(item, show_by_connection, selected_syspath);
			}
		}
	}
}

void TDEHWManager::deviceChanged(TDEGenericDevice* device) {
	TQListViewItemIterator it(base->deviceTree);
	while (it.current()) {
		DeviceIconItem* item = dynamic_cast<DeviceIconItem*>(it.current());
		if (item) {
			TDEGenericDevice* candidate = item->device();
			if (candidate) {
				if (candidate->systemPath() == device->systemPath()) {
					if (item->text(0) != device->detailedFriendlyName()) {
						item->setText(0, device->detailedFriendlyName());
					}
				}
			}
		}
		++it;
	}
}

void TDEHWManager::cryptographicCardPinRequested(TQString prompt, TDECryptographicCardDevice* cdevice) {
	TQString password;
	int result = KPasswordDialog::getPassword(password, prompt);
	if (result == KPasswordDialog::Accepted) {
		cdevice->setProvidedPin(password);
	}
	else {
		cdevice->setProvidedPin(TQString::null);
	}
}

TQString TDEHWManager::quickHelp() const
{
  return i18n("<h1>TDE Device Manager</h1> This module allows you to get all kind of informations about the devices on your system, the drivers which are used by them and to configure them as well.");
}

#include "hwmanager.moc"
