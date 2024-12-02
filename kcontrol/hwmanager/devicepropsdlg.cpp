/* This file is part of TDE
   Copyright (C) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include <config.h>

#include <tqvalidator.h>
#include <tqpushbutton.h>
#include <tqlineedit.h>
#include <tqlabel.h>
#include <tqtabwidget.h>
#include <tqgroupbox.h>
#include <tqlayout.h>
#include <tqslider.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <private/tqinternal_p.h>
#include <tdeprocess.h>
#include <dcopclient.h>
#include <dcopref.h>
#undef Unsorted // Required for --enable-final (tqdir.h)
#include <tqfiledialog.h>

#include <kpassdlg.h>
#include <kactivelabel.h>
#include <kbuttonbox.h>
#include <kcombobox.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <tdemessagebox.h>
#include <ksslcertificate.h>

#include "cryptpassworddlg.h"
#include "passworddlg.h"

#include "devicepropsdlg.h"

SensorDisplayLabelsWidget::SensorDisplayLabelsWidget(TQWidget *parent)
	: TQWidget(parent)
{
	m_nameLabel = new TQLabel(this);
	m_valueLabel = new TQLabel(this);

	TQGridLayout *mainGrid = new TQGridLayout(this, 1, 2, 0, 1);
	mainGrid->setRowStretch(1, 0);
	mainGrid->addWidget(m_nameLabel, 0, 0);
	mainGrid->addWidget(m_valueLabel, 0, 1);
}

SensorDisplayLabelsWidget::~SensorDisplayLabelsWidget()
{
}

void SensorDisplayLabelsWidget::setSensorName(TQString name) {
	m_nameLabel->setText(name);
}

void SensorDisplayLabelsWidget::setSensorValue(TQString value) {
	m_valueLabel->setText(value);
}

bool SensorBar::setIndicator(TQString & progress_str, int progress, int totalSteps) {
	Q_UNUSED(progress);
	Q_UNUSED(totalSteps);

	if (progress_str != m_currentValueString) {
		progress_str = m_currentValueString;
		return true;
	}
	else {
		return false;
	}
}

void SensorBar::drawContents(TQPainter *p) {
	// Draw warn/crit/value bars

	TQRect bar = contentsRect();
	TQSharedDoubleBuffer buffer( p, bar.x(), bar.y(), bar.width(), bar.height() );
	buffer.painter()->fillRect(bar, TQt::white);

	TQStyle::SFlags flags = TQStyle::Style_Default;
	if (isEnabled()) {
		flags |= TQStyle::Style_Enabled;
	}
	if (hasFocus()) {
		flags |= TQStyle::Style_HasFocus;
	}
	style().drawControl(TQStyle::CE_ProgressBarGroove, buffer.painter(), this, TQStyle::visualRect(style().subRect(TQStyle::SR_ProgressBarGroove, this), this ), colorGroup(), flags);

	if (m_warningLocation > 0) {
		bar = contentsRect();
		bar.setX((bar.width()*((m_warningLocation*1.0)/totalSteps()))-2);
		bar.setWidth(5);
		bar.setHeight(3);
		buffer.painter()->fillRect(bar, TQt::yellow);
		bar = contentsRect();
		bar.setX((bar.width()*((m_warningLocation*1.0)/totalSteps()))-2);
		bar.setWidth(5);
		bar.setY(bar.height()-3);
		bar.setHeight(3);
		buffer.painter()->fillRect(bar, TQt::yellow);
		bar = contentsRect();
		bar.setX((bar.width()*((m_warningLocation*1.0)/totalSteps()))-0);
		bar.setWidth(1);
		buffer.painter()->fillRect(bar, TQt::yellow);
	}

	if (m_criticalLocation > 0) {
		bar = contentsRect();
		bar.setX((bar.width()*((m_criticalLocation*1.0)/totalSteps()))-2);
		bar.setWidth(5);
		bar.setHeight(3);
		buffer.painter()->fillRect(bar, TQt::red);
		bar = contentsRect();
		bar.setX((bar.width()*((m_criticalLocation*1.0)/totalSteps()))-2);
		bar.setWidth(5);
		bar.setY(bar.height()-3);
		bar.setHeight(3);
		buffer.painter()->fillRect(bar, TQt::red);
		bar = contentsRect();
		bar.setX((bar.width()*((m_criticalLocation*1.0)/totalSteps()))-0);
		bar.setWidth(1);
		buffer.painter()->fillRect(bar, TQt::red);
	}

	if (m_currentLocation > 0) {
		bar = contentsRect();
		bar.setX((bar.width()*((m_currentLocation*1.0)/totalSteps()))-2);
		bar.setWidth(5);
		bar.setHeight(3);
		buffer.painter()->fillRect(bar, TQt::green);
		bar = contentsRect();
		bar.setX((bar.width()*((m_currentLocation*1.0)/totalSteps()))-2);
		bar.setWidth(5);
		bar.setY(bar.height()-3);
		bar.setHeight(3);
		buffer.painter()->fillRect(bar, TQt::green);
		bar = contentsRect();
		bar.setX((bar.width()*((m_currentLocation*1.0)/totalSteps()))-0);
		bar.setWidth(1);
		buffer.painter()->fillRect(bar, TQt::green);
	}

	bar = contentsRect();
	buffer.painter()->setPen(TQt::black);
	buffer.painter()->drawText(bar.x(), bar.y(), bar.width()/3, bar.height(), TQt::AlignVCenter | TQt::AlignLeft, m_minimumValueString);
	buffer.painter()->drawText(bar.x()+(bar.width()/3), bar.y(), bar.width()/3, bar.height(), TQt::AlignVCenter | TQt::AlignHCenter, m_currentValueString);
	buffer.painter()->drawText(bar.x()+((bar.width()/3)*2), bar.y(), bar.width()/3, bar.height(), TQt::AlignVCenter | TQt::AlignRight, m_maximumValueString);
}

SensorDisplayWidget::SensorDisplayWidget(TQWidget *parent)
	: TQWidget(parent)
{
	m_nameLabel = new TQLabel(this);
	m_progressBar = new SensorBar(this);

	TQGridLayout *mainGrid = new TQGridLayout(this, 1, 2, 0, 1);
	mainGrid->setRowStretch(1, 0);
	mainGrid->addWidget(m_nameLabel, 0, 0);
	mainGrid->addWidget(m_progressBar, 0, 1);
}

SensorDisplayWidget::~SensorDisplayWidget()
{
}

void SensorDisplayWidget::setSensorName(TQString name) {
	m_nameLabel->setText(name);
}

void SensorDisplayWidget::setSensorCurrentValue(double value) {
	m_current = value;
}

void SensorDisplayWidget::setSensorMinimumValue(double value) {
	m_minimum = value;
}

void SensorDisplayWidget::setSensorMaximumValue(double value) {
	m_maximum = value;
}

void SensorDisplayWidget::setSensorWarningValue(double value) {
	m_warning = value;
}

void SensorDisplayWidget::setSensorCriticalValue(double value) {
	m_critical = value;
}

void SensorDisplayWidget::updateDisplay() {
	double minimum = m_minimum;
	double maximum = m_maximum;
	double current = m_current;
	double warning = m_warning;
	double critical = m_critical;

	if (minimum < 0) {
		minimum = 0;
	}
	if (maximum < 0) {
		if (critical < 0) {
			maximum = warning;
		}
		else {
			maximum = critical;
		}
	}
	if (warning > maximum) {
		maximum = warning;
	}
	if (critical > maximum) {
		maximum = critical;
	}

	m_progressBar->setTotalSteps(maximum);
	m_progressBar->m_currentLocation = current - minimum;
	m_progressBar->setProgress(0);

	if (warning < 0) {
		m_progressBar->m_warningLocation = -1;
	}
	else {
		m_progressBar->m_warningLocation = warning - minimum;
	}
	if (critical < 0) {
		m_progressBar->m_criticalLocation = -1;
	}
	else {
		m_progressBar->m_criticalLocation = critical - minimum;
	}

	m_progressBar->m_minimumValueString = (TQString("%1").arg(minimum));
	m_progressBar->m_maximumValueString = (TQString("%1").arg(maximum));
	m_progressBar->m_currentValueString = TQString("%1").arg(current);
}

DevicePropertiesDialog::DevicePropertiesDialog(TDEGenericDevice* device, TQWidget *parent)
	: KDialogBase(Plain, TQString::null, Ok|Cancel, Ok, parent, 0L, true, true)
{
	m_device = device;
	enableButtonOK( false );

	if (m_device) {
		base = new DevicePropertiesDialogBase(plainPage());

		// Remove all non-applicable tabs
		if (m_device->type() != TDEGenericDeviceType::Disk) {
			base->tabBarWidget->removePage(base->tabDisk);
			base->tabBarWidget->removePage(base->tabDiskCrypt);
		}
		if (m_device->type() != TDEGenericDeviceType::CPU) {
			base->tabBarWidget->removePage(base->tabCPU);
		}
		if ((m_device->type() != TDEGenericDeviceType::OtherSensor) && (m_device->type() != TDEGenericDeviceType::ThermalSensor)) {
			base->tabBarWidget->removePage(base->tabSensor);
		}
		if (m_device->type() != TDEGenericDeviceType::Battery) {
			base->tabBarWidget->removePage(base->tabBattery);
		}
		if (m_device->type() != TDEGenericDeviceType::PowerSupply) {
			base->tabBarWidget->removePage(base->tabPowerSupply);
		}
		if (m_device->type() != TDEGenericDeviceType::Network) {
			base->tabBarWidget->removePage(base->tabNetwork);
		}
		if (m_device->type() != TDEGenericDeviceType::Backlight) {
			base->tabBarWidget->removePage(base->tabBacklight);
		}
		if (m_device->type() != TDEGenericDeviceType::Monitor) {
			base->tabBarWidget->removePage(base->tabMonitor);
		}
		if (m_device->type() != TDEGenericDeviceType::RootSystem) {
			base->tabBarWidget->removePage(base->tabRootSystem);
		}
		if (m_device->type() != TDEGenericDeviceType::Event) {
			base->tabBarWidget->removePage(base->tabEvent);
		}
		if (m_device->type() != TDEGenericDeviceType::CryptographicCard) {
			base->tabBarWidget->removePage(base->tabCryptographicCard);
		}

		if (m_device->type() == TDEGenericDeviceType::CPU) {
			connect(base->comboCPUGovernor, TQ_SIGNAL(activated(const TQString &)), this, TQ_SLOT(setCPUGovernor(const TQString &)));
		}
		if (m_device->type() == TDEGenericDeviceType::Disk) {
			TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);
			connect(base->buttonDiskMount, TQ_SIGNAL(clicked()), this, TQ_SLOT(mountDisk()));
			connect(base->buttonDiskUnmount, TQ_SIGNAL(clicked()), this, TQ_SLOT(unmountDisk()));
			connect(base->buttonDiskUnlock, TQ_SIGNAL(clicked()), this, TQ_SLOT(unlockDisk()));
			connect(base->buttonDiskLock, TQ_SIGNAL(clicked()), this, TQ_SLOT(lockDisk()));
			connect(base->buttonDiskEject, TQ_SIGNAL(clicked()), this, TQ_SLOT(ejectDisk()));
			connect(base->buttonDiskSafeRemove, TQ_SIGNAL(clicked()), this, TQ_SLOT(safeRemoveDisk()));
			if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS)) {
				connect(base->cryptLUKSAddKey, TQ_SIGNAL(clicked()), this, TQ_SLOT(cryptLUKSAddKey()));
				connect(base->cryptLUKSDelKey, TQ_SIGNAL(clicked()), this, TQ_SLOT(cryptLUKSDelKey()));
				connect(base->cryptLUKSKeySlotList, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(processLockouts()));
				base->cryptLUKSKeySlotList->setAllColumnsShowFocus(true);
				base->cryptLUKSKeySlotList->setFullWidth(true);
				cryptLUKSPopulateList();
				processLockouts();
			}
			else {
				base->tabBarWidget->removePage(base->tabDiskCrypt);
			}
		}

		if ((m_device->type() == TDEGenericDeviceType::OtherSensor) || (m_device->type() == TDEGenericDeviceType::ThermalSensor)) {
			base->groupSensors->setColumnLayout(0, TQt::Vertical );
			base->groupSensors->layout()->setSpacing( KDialog::spacingHint() );
			base->groupSensors->layout()->setMargin( KDialog::marginHint() );
			m_sensorDataGrid = new TQGridLayout( base->groupSensors->layout() );
			m_sensorDataGrid->setAlignment( TQt::AlignTop );
			m_sensorDataGridWidgets.setAutoDelete(true);
		}
		if (m_device->type() == TDEGenericDeviceType::Backlight) {
			connect(base->sliderBacklightBrightness, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(setBacklightBrightness(int)));
		}
		if (m_device->type() == TDEGenericDeviceType::RootSystem) {
			connect(base->comboSystemHibernationMethod, TQ_SIGNAL(activated(int)), this, TQ_SLOT(setHibernationMethod(int)));
		}

		TQGridLayout *mainGrid = new TQGridLayout(plainPage(), 1, 1, 0, spacingHint());
		mainGrid->setRowStretch(1, 1);
		mainGrid->addWidget(base, 0, 0);
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	connect(hwdevices, TQ_SIGNAL(hardwareRemoved(TDEGenericDevice*)), this, TQ_SLOT(processHardwareRemoved(TDEGenericDevice*)));
	connect(hwdevices, TQ_SIGNAL(hardwareUpdated(TDEGenericDevice*)), this, TQ_SLOT(processHardwareUpdated(TDEGenericDevice*)));

	populateDeviceInformation();
}

DevicePropertiesDialog::~DevicePropertiesDialog()
{
}

void DevicePropertiesDialog::processHardwareRemoved(TDEGenericDevice* dev) {
	if (dev == m_device) {
		close();
	}
}

void DevicePropertiesDialog::processHardwareUpdated(TDEGenericDevice* dev) {
	if (dev == m_device) {
		populateDeviceInformation();
	}
}

TQString assembleSwitchList(TDESwitchType::TDESwitchType switches) {
	return (TDEEventDevice::friendlySwitchList(switches).join("<br>"));
}

static TQString formatDisplayString(TQString input) {
	return TQStyleSheet::escape(input);
}

bool DevicePropertiesDialog::isMonitoredDevice(TDEStorageDevice *sdevice)
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

void DevicePropertiesDialog::populateDeviceInformation()
{
	if (m_device) {
		base->labelDeviceType->setText(m_device->friendlyDeviceType());
		base->iconDeviceType->setPixmap(m_device->icon(TDEIcon::SizeSmall));
		base->labelDeviceName->setText(formatDisplayString(m_device->friendlyName()));
		base->labelDeviceNode->setText(formatDisplayString((m_device->deviceNode().isNull())?i18n("<none>"):m_device->deviceNode()));
		base->labelSystemPath->setText(formatDisplayString(m_device->systemPath()));
		base->labelSubsytemType->setText(formatDisplayString(m_device->subsystem()));
		base->labelDeviceDriver->setText(formatDisplayString((m_device->deviceDriver().isNull())?i18n("<none>"):m_device->deviceDriver()));
		base->labelDeviceClass->setText(formatDisplayString((m_device->PCIClass().isNull())?i18n("<n/a>"):m_device->PCIClass()));
		base->labelModalias->setText(formatDisplayString((m_device->moduleAlias().isNull())?i18n("<none>"):m_device->moduleAlias()));

		// These might be redundant
		#if 0
		base->labelVendorName->setText((m_device->vendorName().isNull())?i18n("<unknown>"):m_device->vendorName());
		base->labelVendorModel->setText((m_device->vendorModel().isNull())?i18n("<unknown>"):m_device->vendorModel());
		#else
		base->labelVendorName->hide();
		base->stocklabelVendorName->hide();
		base->labelVendorModel->hide();
		base->stocklabelVendorModel->hide();
		#endif
		base->labelSerialNumber->setText(formatDisplayString((m_device->serialNumber().isNull())?i18n("<unknown>"):m_device->serialNumber()));

		if (m_device->subsystem() == "pci") {
			base->labelBusID->setText(formatDisplayString(m_device->busID()));
			base->labelBusID->show();
			base->stocklabelBusID->show();
		}
		else {
			base->labelBusID->hide();
			base->stocklabelBusID->hide();
		}

		if (m_device->type() == TDEGenericDeviceType::Disk) {
			TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

			TQString mountPoint = sdevice->mountPath();
			if (mountPoint == "") mountPoint = i18n("<none>");
			base->labelDiskMountpoint->setText(formatDisplayString(mountPoint));

			TQString fsName = sdevice->fileSystemName();
			if (fsName == "") fsName = i18n("<unknown>");
			base->labelDiskFileSystemType->setText(formatDisplayString(fsName));

			TQString volUUID = sdevice->diskUUID();
			if (volUUID == "") volUUID = i18n("<none>");
			base->labelDiskUUID->setText(formatDisplayString(volUUID));

			TQString mappedName = sdevice->mappedName();
			if (mappedName == "") mappedName = i18n("<none>");
			base->labelMappedName->setText(formatDisplayString(mappedName));

			// Show status
			TQString status_text = "<qt>";
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable)) {
				status_text += i18n("Mountable") + "<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable)) {
				status_text += i18n("Removable") + "<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Inserted)) {
				status_text += i18n("Inserted") + "<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				status_text += i18n("Blank") + "<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsedByDevice)) {
				status_text += i18n("In use") + "<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsesDevice)) {
				status_text += i18n("Uses other device") + "<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::ContainsFilesystem)) {
				status_text += i18n("Contains a filesystem") + "<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hidden)) {
				status_text += i18n("Hidden") + "<br>";
			}
			if (status_text == "<qt>") {
				status_text += "<i>" + i18n("Unknown") + "</i>";
			}
			status_text += "</qt>";
			base->labelDiskStatus->setText(status_text);

			// Update action button status
			base->buttonDiskMount->setEnabled(false);
			base->buttonDiskUnmount->setEnabled(false);
			base->buttonDiskUnlock->setEnabled(false);
			base->buttonDiskLock->setEnabled(false);
			base->buttonDiskEject->setEnabled(false);
			base->buttonDiskSafeRemove->setEnabled(false);
			base->buttonDiskMount->setHidden(true);
			base->buttonDiskUnmount->setHidden(true);
			base->buttonDiskUnlock->setHidden(true);
			base->buttonDiskLock->setHidden(true);
			base->buttonDiskEject->setHidden(true);
			base->buttonDiskSafeRemove->setHidden(true);
			bool showGroup = false;
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable))
			{
				base->buttonDiskMount->setEnabled((sdevice->mountPath() == ""));
				base->buttonDiskUnmount->setEnabled((sdevice->mountPath() != ""));
				base->buttonDiskMount->setHidden(false);
				base->buttonDiskUnmount->setHidden(false);
				showGroup = true;
			}
			else if (sdevice->isDiskOfType(TDEDiskDeviceType::LUKS) ||
					     sdevice->isDiskOfType(TDEDiskDeviceType::OtherCrypted))
			{
				base->buttonDiskUnlock->setEnabled(!sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt));
				base->buttonDiskLock->setEnabled(sdevice->isDiskOfType(TDEDiskDeviceType::UnlockedCrypt));
				base->buttonDiskUnlock->setHidden(false);
				base->buttonDiskLock->setHidden(false);
				showGroup = true;
			}
			if (isMonitoredDevice(sdevice) &&
			    sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable))
			{
				base->buttonDiskEject->setEnabled(true);
				base->buttonDiskSafeRemove->setEnabled(true);
				base->buttonDiskEject->setHidden(false);
				base->buttonDiskSafeRemove->setHidden(false);
				showGroup = true;
			}
			if (showGroup)
			{
				base->groupDiskActions->show();
			}
			else
			{
				base->groupDiskActions->hide();
			}
		}

		if (m_device->type() == TDEGenericDeviceType::CPU) {
			TDECPUDevice* cdevice = static_cast<TDECPUDevice*>(m_device);

			// Show information
			base->labelCPUVendor->setText(cdevice->vendorEncoded());
			base->labelCPUFrequency->setText((cdevice->frequency()<0)?i18n("<unsupported>"):TQString("%1 MHz").arg(cdevice->frequency()));
			base->labelMinCPUFrequency->setText((cdevice->minFrequency()<0)?i18n("<unsupported>"):TQString("%1 MHz").arg(cdevice->minFrequency()));
			base->labelMaxCPUFrequency->setText((cdevice->maxFrequency()<0)?i18n("<unsupported>"):TQString("%1 MHz").arg(cdevice->maxFrequency()));
			base->labelScalingDriver->setText((cdevice->scalingDriver().isNull())?i18n("<none>"):cdevice->scalingDriver());
			TQStringList scalingfreqs = cdevice->availableFrequencies();
			if (scalingfreqs.count() > 0) {
				TQString scalingfreqsstring = "<qt>";
				for ( TQStringList::Iterator it = scalingfreqs.begin(); it != scalingfreqs.end(); ++it ) {
					TQString freq = (*it);
					scalingfreqsstring.append(TQString("%1 MHz<br>").arg(freq.toDouble()/1000));
				}
				scalingfreqsstring.append("</qt>");
				base->labelScalingFrequencies->setText(scalingfreqsstring);
			}
			else {
				base->labelScalingFrequencies->setText(i18n("<none>"));
			}
			TQStringList dependentcpus = cdevice->dependentProcessors();
			if (dependentcpus.count() > 0) {
				TQString dependentcpusstring = "<qt>";
				for ( TQStringList::Iterator it = dependentcpus.begin(); it != dependentcpus.end(); ++it ) {
					TQString proc = (*it);
					dependentcpusstring.append(TQString("CPU %1<br>").arg(proc));
				}
				dependentcpusstring.append("</qt>");
				base->labelDependentCPUs->setText(dependentcpusstring);
			}
			else {
				base->labelDependentCPUs->setText(i18n("<none>"));
			}
			base->comboCPUGovernor->setEnabled(cdevice->canSetGovernor());
			TQStringList governorPolicies = cdevice->availableGovernors();
			if ((uint)governorPolicies.count() != (uint)base->comboCPUGovernor->count()) {
				base->comboCPUGovernor->clear();
				int i=0;
				for (TQStringList::Iterator it = governorPolicies.begin(); it != governorPolicies.end(); ++it) {
					base->comboCPUGovernor->insertItem(*it, i);
					i++;
				}
			}
			base->comboCPUGovernor->setCurrentItem(cdevice->governor(), false);
		}

		if ((m_device->type() == TDEGenericDeviceType::OtherSensor) || (m_device->type() == TDEGenericDeviceType::ThermalSensor)) {
			TDESensorDevice* sdevice = static_cast<TDESensorDevice*>(m_device);

			TDESensorClusterMap map = sdevice->values();
			TDESensorClusterMap::Iterator it;
			unsigned int i;

			if (m_sensorDataGridWidgets.count() != map.count()) {
				m_sensorDataGridWidgets.clear();
				for (i=0;i<map.count();i++) {
					SensorDisplayWidget* sensorWidget = new SensorDisplayWidget(base->groupSensors);
					m_sensorDataGrid->addWidget(sensorWidget, i, 0);
					m_sensorDataGridWidgets.append(sensorWidget);
				}
			}

			i=0;
			for ( it = map.begin(); it != map.end(); ++it ) {
				TQString sensorlabel = it.key();
				TQString sensordatastring;
				TDESensorCluster values = it.data();

				if (!values.label.isNull()) {
					sensorlabel = values.label;
				}
				if (sensorlabel.isNull()) {
					sensorlabel = i18n("<unnamed>");
				}

				m_sensorDataGridWidgets.at(i)->setSensorName(sensorlabel);
				m_sensorDataGridWidgets.at(i)->setSensorCurrentValue(values.current);
				m_sensorDataGridWidgets.at(i)->setSensorMinimumValue(values.minimum);
				m_sensorDataGridWidgets.at(i)->setSensorMaximumValue(values.maximum);
				m_sensorDataGridWidgets.at(i)->setSensorWarningValue(values.warning);
				m_sensorDataGridWidgets.at(i)->setSensorCriticalValue(values.critical);
				m_sensorDataGridWidgets.at(i)->updateDisplay();

				i++;
			}
		}

		if (m_device->type() == TDEGenericDeviceType::Battery) {
			TDEBatteryDevice* bdevice = static_cast<TDEBatteryDevice*>(m_device);

			base->labelCurrentBatteryEnergy->setText((bdevice->energy()<0)?i18n("<unknown>"):TQString("%1 Ah").arg(bdevice->energy()));
			base->labelMaximumBatteryEnergy->setText((bdevice->maximumEnergy()<0)?i18n("<unknown>"):TQString("%1 Ah").arg(bdevice->maximumEnergy()));
			base->labelMaximumBatteryDesignEnergy->setText((bdevice->maximumDesignEnergy()<0)?i18n("<unknown>"):TQString("%1 Ah").arg(bdevice->maximumDesignEnergy()));
			base->labelMinimumBatteryVoltage->setText((bdevice->minimumVoltage()<0)?i18n("<unknown>"):TQString("%1 V").arg(bdevice->minimumVoltage()));
			base->labelCurrentBatteryVoltage->setText((bdevice->voltage()<0)?i18n("<unknown>"):TQString("%1 V").arg(bdevice->voltage()));
			base->labelCurrentBatteryDischargeRate->setText((bdevice->dischargeRate()<0)?i18n("<unknown>"):TQString("%1 A").arg(bdevice->dischargeRate()));
			TQString batteryStatusString = i18n("Unknown");
			TDEBatteryStatus::TDEBatteryStatus batteryStatus = bdevice->status();
			if (batteryStatus == TDEBatteryStatus::Charging) {
				batteryStatusString = i18n("Charging");
			}
			if (batteryStatus == TDEBatteryStatus::Discharging) {
				batteryStatusString = i18n("Discharging");
			}
			if (batteryStatus == TDEBatteryStatus::Full) {
				batteryStatusString = i18n("Full");
			}
			base->labelCurrentBatteryStatus->setText(batteryStatusString);
			base->labelBatteryTechnology->setText((bdevice->technology().isNull())?i18n("<unknown>"):bdevice->technology());
			base->labelBatteryInstalled->setText((bdevice->installed()==0)?i18n("No"):i18n("Yes"));
			base->labelBatteryCharge->setText((bdevice->chargePercent()<0)?i18n("<unknown>"):TQString("%1 %").arg(bdevice->chargePercent()));
			base->labelBatteryTimeRemaining->setText((bdevice->timeRemaining()<0)?i18n("<unknown>"):TQString("%1 seconds").arg(bdevice->timeRemaining()));
		}

		if (m_device->type() == TDEGenericDeviceType::PowerSupply) {
			TDEMainsPowerDevice* pdevice = static_cast<TDEMainsPowerDevice*>(m_device);

			base->labelPowerSupplyOnline->setText((pdevice->online()==0)?i18n("No"):i18n("Yes"));
		}

		if (m_device->type() == TDEGenericDeviceType::Network) {
			TDENetworkDevice* ndevice = static_cast<TDENetworkDevice*>(m_device);

			base->labelNetworkMAC->setText((ndevice->macAddress().isNull())?i18n("<unknown>"):ndevice->macAddress());
			base->labelNetworkState->setText((ndevice->state().isNull())?i18n("<unknown>"):ndevice->state());
			base->labelNetworkCarrierPresent->setText((ndevice->carrierPresent()==0)?i18n("No"):i18n("Yes"));
			base->labelNetworkDormant->setText((ndevice->dormant()==0)?i18n("No"):i18n("Yes"));

			base->labelNetworkIPV4Address->setText((ndevice->ipV4Address().isNull())?i18n("<none>"):ndevice->ipV4Address());
			base->labelNetworkIPV4Netmask->setText((ndevice->ipV4Netmask().isNull())?i18n("<none>"):ndevice->ipV4Netmask());
			base->labelNetworkIPV4Broadcast->setText((ndevice->ipV4Broadcast().isNull())?i18n("<none>"):ndevice->ipV4Broadcast());
			base->labelNetworkIPV4Destination->setText((ndevice->ipV4Destination().isNull())?i18n("<none>"):ndevice->ipV4Destination());

			base->labelNetworkIPV6Address->setText((ndevice->ipV6Address().isNull())?i18n("<none>"):ndevice->ipV6Address());
			base->labelNetworkIPV6Netmask->setText((ndevice->ipV6Netmask().isNull())?i18n("<none>"):ndevice->ipV6Netmask());
			base->labelNetworkIPV6Broadcast->setText((ndevice->ipV6Broadcast().isNull())?i18n("<none>"):ndevice->ipV6Broadcast());
			base->labelNetworkIPV6Destination->setText((ndevice->ipV6Destination().isNull())?i18n("<none>"):ndevice->ipV6Destination());

			base->labelNetworkRXBytes->setText((ndevice->rxBytes()<0)?i18n("<unknown>"):TDEHardwareDevices::bytesToFriendlySizeString(ndevice->rxBytes()));
			base->labelNetworkTXBytes->setText((ndevice->txBytes()<0)?i18n("<unknown>"):TDEHardwareDevices::bytesToFriendlySizeString(ndevice->txBytes()));
			base->labelNetworkRXPackets->setText((ndevice->rxPackets()<0)?i18n("<unknown>"):TQString("%1").arg(ndevice->rxPackets()));
			base->labelNetworkTXPackets->setText((ndevice->txPackets()<0)?i18n("<unknown>"):TQString("%1").arg(ndevice->txPackets()));
		}

		if (m_device->type() == TDEGenericDeviceType::Backlight) {
			TDEBacklightDevice* bdevice = static_cast<TDEBacklightDevice*>(m_device);

			base->labelBacklightStatus->setText((bdevice->powerLevel()==TDEDisplayPowerLevel::On)?i18n("On"):i18n("Off"));
			base->labelBacklightBrightness->setText((bdevice->brightnessPercent()<0)?i18n("<unknown>"):TQString("%1 %").arg(bdevice->brightnessPercent()));
			base->sliderBacklightBrightness->setOrientation(TQt::Horizontal);
			base->sliderBacklightBrightness->setMinValue(0);
			base->sliderBacklightBrightness->setMaxValue(bdevice->brightnessSteps()-1);
			base->sliderBacklightBrightness->setValue(bdevice->rawBrightness());
			base->sliderBacklightBrightness->setEnabled(bdevice->canSetBrightness());
		}

		if (m_device->type() == TDEGenericDeviceType::Monitor) {
			TDEMonitorDevice* mdevice = static_cast<TDEMonitorDevice*>(m_device);

			base->labelDisplayPortType->setText((mdevice->portType().isNull())?i18n("<unknown>"):mdevice->portType());
			base->labelDisplayConnected->setText((mdevice->connected())?i18n("Yes"):i18n("No"));
			base->labelDisplayEnabled->setText((mdevice->enabled())?i18n("Yes"):i18n("No"));

			TQString dpmsLevel;
			TDEDisplayPowerLevel::TDEDisplayPowerLevel dpms = TDEDisplayPowerLevel::On;
			if (dpms == TDEDisplayPowerLevel::On) {
				dpmsLevel = i18n("On");
			}
			else if (dpms == TDEDisplayPowerLevel::Standby) {
				dpmsLevel = i18n("Standby");
			}
			else if (dpms == TDEDisplayPowerLevel::Suspend) {
				dpmsLevel = i18n("Suspend");
			}
			else if (dpms == TDEDisplayPowerLevel::Off) {
				dpmsLevel = i18n("Off");
			}
			base->labelDisplayDPMS->setText(dpmsLevel);

			TDEResolutionList resolutionList = mdevice->resolutions();
			if (resolutionList.count() > 0) {
				TQString resolutionsstring = "<qt>";
				TDEResolutionList::iterator it;
				for (it = resolutionList.begin(); it != resolutionList.end(); ++it) {
					TDEResolutionPair res = *it;
					resolutionsstring += TQString("%1x%2<br>").arg(res.first).arg(res.second);
				}
				resolutionsstring += "</qt>";
				base->labelDisplayResolutions->setText(resolutionsstring);
			}
			else {
				base->labelDisplayResolutions->setText(i18n("<unknown>"));
			}

			// RandR warning
			base->labelRandrWarning->setText(i18n("<qt><b>NOTE: Any further integration of displays into TDE <i>REQUIRES</i> multi GPU support and other features slated for RandR 2.0.</b><p>Development on such features has been sorely lacking for well over a year as of 2012; if you want to see Linux come up to Windows and Macintosh standards in this area <i>please tell the Xorg developers</i> at http://www.x.org/wiki/XorgMailingLists<p>The TDE project badly needs these features before it can proceed with graphical monitor configuration tools:<br> * GPU object support<br> * The ability to query the active driver name for any Xorg output<p><b>To recap, this is <i>not a TDE shortcoming</i>, but rather is the result of a lack of fundamental Linux support for graphics configuration!</b></qt>"));
		}

		if (m_device->type() == TDEGenericDeviceType::RootSystem) {
			TDERootSystemDevice* rdevice = static_cast<TDERootSystemDevice*>(m_device);

			TQString formFactorString;
			TDESystemFormFactor::TDESystemFormFactor formFactor = rdevice->formFactor();
			if (formFactor == TDESystemFormFactor::Unclassified) {
				formFactorString = i18n("Unknown");
			}
			else if (formFactor == TDESystemFormFactor::Desktop) {
				formFactorString = i18n("Desktop");
			}
			else if (formFactor == TDESystemFormFactor::Laptop) {
				formFactorString = i18n("Laptop");
			}
			else if (formFactor == TDESystemFormFactor::Server) {
				formFactorString = i18n("Server");
			}
			base->labelSystemFormFactor->setText(formFactorString);

			TQString powerStatesString;
			TDESystemPowerStateList powerStates = rdevice->powerStates();
			if (powerStates.count() > 0) {
				powerStatesString = "<qt>";
				TDESystemPowerStateList::iterator it;
				for (it = powerStates.begin(); it != powerStates.end(); ++it) {
					if ((*it) == TDESystemPowerState::Active) {
						powerStatesString += i18n("Active<br>");
					}
					else if ((*it) == TDESystemPowerState::Standby) {
						powerStatesString += i18n("Standby<br>");
					}
					else if ((*it) == TDESystemPowerState::Freeze) {
						powerStatesString += i18n("Freeze<br>");
					}
					else if ((*it) == TDESystemPowerState::Suspend) {
						powerStatesString += i18n("Suspend<br>");
					}
					else if ((*it) == TDESystemPowerState::Hibernate) {
						powerStatesString += i18n("Hibernate<br>");
					}
					else if ((*it) == TDESystemPowerState::HybridSuspend) {
						powerStatesString += i18n("Hybrid Suspend<br>");
					}
					else if ((*it) == TDESystemPowerState::PowerOff) {
						powerStatesString += i18n("Power Off<br>");
					}
				}
				powerStatesString += "</qt>";
			}
			else {
				powerStatesString += i18n("<unknown>");
			}
			base->labelSystemPowerStates->setText(powerStatesString);

			base->comboSystemHibernationMethod->setEnabled(rdevice->canSetHibernationMethod());
			TDESystemHibernationMethodList hibernationMethods = rdevice->hibernationMethods();
			if ((uint)hibernationMethods.count() != (uint)base->comboSystemHibernationMethod->count()) {
				base->comboSystemHibernationMethod->clear();
				m_hibernationComboMap.clear();
				int i=0;
				TQString label;
				for (TDESystemHibernationMethodList::Iterator it = hibernationMethods.begin(); it != hibernationMethods.end(); ++it) {
					if ((*it) == TDESystemHibernationMethod::Unsupported) {
						label = i18n("<none>");
					}
					else if ((*it) == TDESystemHibernationMethod::Platform) {
						label = i18n("Platform");
					}
					else if ((*it) == TDESystemHibernationMethod::Suspend) {
						label = i18n("Suspend");
					}
					else if ((*it) == TDESystemHibernationMethod::Shutdown) {
						label = i18n("Shutdown");
					}
					else if ((*it) == TDESystemHibernationMethod::Reboot) {
						label = i18n("Reboot");
					}
					else if ((*it) == TDESystemHibernationMethod::TestProc) {
						label = i18n("Test Procedure");
					}
					else if ((*it) == TDESystemHibernationMethod::Test) {
						label = i18n("Test");
					}
					base->comboSystemHibernationMethod->insertItem(label, i);
					m_hibernationComboMap[*it] = i;
					i++;
				}
			}
			base->comboSystemHibernationMethod->setCurrentItem(m_hibernationComboMap[rdevice->hibernationMethod()]);

			base->labelSystemUserCanStandby->setText((rdevice->canStandby())?i18n("Yes"):i18n("No"));
			base->labelSystemUserCanFreeze->setText((rdevice->canFreeze())?i18n("Yes"):i18n("No"));
			base->labelSystemUserCanSuspend->setText((rdevice->canSuspend())?i18n("Yes"):i18n("No"));
			base->labelSystemUserCanHibernate->setText((rdevice->canHibernate())?i18n("Yes"):i18n("No"));
			base->labelSystemUserCanHybridSuspend->setText((rdevice->canHybridSuspend())?i18n("Yes"):i18n("No"));
			base->labelSystemUserCanPowerOff->setText((rdevice->canPowerOff())?i18n("Yes"):i18n("No"));

			base->labelSystemHibernationSpace->setText((rdevice->diskSpaceNeededForHibernation()<0)?i18n("<unknown>"):TDEHardwareDevices::bytesToFriendlySizeString(rdevice->diskSpaceNeededForHibernation()));
		}

		if (m_device->type() == TDEGenericDeviceType::Event) {
			TDEEventDevice* edevice = static_cast<TDEEventDevice*>(m_device);

			TQString availableSwitches;
			if (edevice->providedSwitches() == TDESwitchType::Null) {
				availableSwitches = i18n("<none>");
			}
			else {
				availableSwitches = "<qt>";
				availableSwitches += assembleSwitchList(edevice->providedSwitches());
				availableSwitches += "</qt>";
			}
			base->labelEventSwitchTypes->setText(availableSwitches);

			TQString activeSwitches;
			if (edevice->activeSwitches() == TDESwitchType::Null) {
				activeSwitches = i18n("<none>");
			}
			else {
				activeSwitches = "<qt>";
				activeSwitches += assembleSwitchList(edevice->activeSwitches());
				activeSwitches += "</qt>";
			}
			base->labelEventSwitchActive->setText(activeSwitches);
		}

		if (m_device->type() == TDEGenericDeviceType::CryptographicCard) {
			TDECryptographicCardDevice* cdevice = static_cast<TDECryptographicCardDevice*>(m_device);

			connect(cdevice, TQ_SIGNAL(cardInserted(TDECryptographicCardDevice*)), this, TQ_SLOT(cryptographicCardInserted()));
			connect(cdevice, TQ_SIGNAL(cardRemoved(TDECryptographicCardDevice*)), this, TQ_SLOT(cryptographicCardRemoved()));

			updateCryptographicCardStatusDisplay();
		}
	}
}

void DevicePropertiesDialog::cryptographicCardInserted() {
	updateCryptographicCardStatusDisplay();
}

void DevicePropertiesDialog::cryptographicCardRemoved() {
	updateCryptographicCardStatusDisplay();
}

void DevicePropertiesDialog::updateCryptographicCardStatusDisplay() {
	TDECryptographicCardDevice* cdevice = static_cast<TDECryptographicCardDevice*>(m_device);

	int status = cdevice->cardPresent();
	if ((status < 0) ||(status > 1)) {
		base->labelCardStatus->setText(i18n("Unknown"));
		base->labelCardCertificates->setText("");
		base->groupCardCerts->hide();
	}
	else if (status == 0) {
		base->labelCardStatus->setText(i18n("Empty"));
		base->labelCardCertificates->setText("");
		base->groupCardCerts->hide();
	}
	else if (status == 1) {
		base->labelCardStatus->setText(i18n("Inserted") + TQString("<br>") + i18n("ATR: %1").arg(cdevice->cardATR()));

		X509CertificatePtrList certList = cdevice->cardX509Certificates();

		if (certList.count() > 0) {
			// Assemble list of certificates on card
			unsigned int certificate_number = 1;
			TQString certInfo = "<qt>";
			X509CertificatePtrList::iterator it;
			for (it = certList.begin(); it != certList.end(); ++it) {
				KSSLCertificate* tdeCert = KSSLCertificate::fromX509(*it);
				KSSLCertificate::KSSLValidation validationStatus = tdeCert->validate();
				certInfo += i18n("Certificate #%1").arg(certificate_number) + ":<br>";
				certInfo += i18n("Subject") + ": " + tdeCert->getSubject() + "<br>";
				certInfo += i18n("Issuer") + ": " + tdeCert->getIssuer() + "<br>";
				certInfo += i18n("Status") + ": " + KSSLCertificate::verifyText(validationStatus) + "<br>";
				certInfo += i18n("Valid From") + ": " + tdeCert->getNotBefore() + "<br>";
				certInfo += i18n("Valid Until") + ": " + tdeCert->getNotAfter() + "<br>";
				certInfo += i18n("Serial Number") + ": " + tdeCert->getSerialNumber() + "<br>";
				certInfo += i18n("MD5 Digest") + ": " + tdeCert->getMD5DigestText() + "<br>";
				certInfo += "<p>";
				delete tdeCert;
				certificate_number++;
			}
			certInfo += "</qt>";
			base->labelCardCertificates->setText(certInfo);
			base->groupCardCerts->show();
		}
		else {
			base->labelCardCertificates->setText("");
			base->groupCardCerts->hide();
		}
	}
}

void DevicePropertiesDialog::setCPUGovernor(const TQString &governor) {
	TDECPUDevice* cdevice = static_cast<TDECPUDevice*>(m_device);

	cdevice->setGovernor(governor);
	populateDeviceInformation();
}

void DevicePropertiesDialog::setBacklightBrightness(int value) {
	TDEBacklightDevice* bdevice = static_cast<TDEBacklightDevice*>(m_device);

	bdevice->setRawBrightness(value);
}

void DevicePropertiesDialog::setHibernationMethod(int value) {
	TDERootSystemDevice* rdevice = static_cast<TDERootSystemDevice*>(m_device);

	rdevice->setHibernationMethod(m_hibernationComboMap.keys()[value]);
	populateDeviceInformation();
}

void DevicePropertiesDialog::mountDisk() {
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	TDEProcess proc;
	proc << "tdeio_media_mounthelper" << "-m" << sdevice->deviceNode();
	if (!proc.start(TDEProcess::DontCare))
	{
		KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
				i18n("Device monitor"));
	}
}

void DevicePropertiesDialog::unmountDisk() {
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	TDEProcess proc;
	proc << "tdeio_media_mounthelper" << "-u" << sdevice->deviceNode();
	if (!proc.start(TDEProcess::DontCare))
	{
		KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
				i18n("Device monitor"));
	}
}

void DevicePropertiesDialog::unlockDisk() {
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	TDEProcess proc;
	proc << "tdeio_media_mounthelper" << "-k" << sdevice->deviceNode();
	if (!proc.start(TDEProcess::DontCare))
	{
		KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
				i18n("Device monitor"));
	}
}

void DevicePropertiesDialog::lockDisk() {
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	TDEProcess proc;
	proc << "tdeio_media_mounthelper" << "-l" << sdevice->deviceNode();
	if (!proc.start(TDEProcess::DontCare))
	{
		KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
				i18n("Device monitor"));
	}
}

void DevicePropertiesDialog::ejectDisk() {
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	TDEProcess proc;
	proc << "tdeio_media_mounthelper" << "-e" << sdevice->deviceNode();
	if (!proc.start(TDEProcess::DontCare))
	{
		KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
				i18n("Device monitor"));
	}
}

void DevicePropertiesDialog::safeRemoveDisk() {
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	TDEProcess proc;
	proc << "tdeio_media_mounthelper" << "-s" << sdevice->deviceNode();
	if (!proc.start(TDEProcess::DontCare))
	{
		KMessageBox::error(this, i18n("Could not start tdeio_media_mounthelper process."),
				i18n("Device monitor"));
	}
}

void DevicePropertiesDialog::cryptLUKSAddKey() {
	int retcode;

	if (m_device->type() == TDEGenericDeviceType::Disk) {
		TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

		TQListViewItem* lvi = base->cryptLUKSKeySlotList->selectedItem();
		if (lvi) {
			TDECryptographicCardDevice* cdevice = NULL;
			unsigned int key_slot = lvi->text(0).toUInt();
			bool allow_card = false;
			bool use_card = false;
			bool luks_card_key_modified = false;
			KSSLCertificate* card_cert = NULL;
			X509* card_cert_x509;
			TQString disk_uuid = sdevice->diskUUID();
			TDEGenericDevice *hwdevice;
			TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
			TDEGenericHardwareList cardReaderList = hwdevices->listByDeviceClass(TDEGenericDeviceType::CryptographicCard);
			for (hwdevice = cardReaderList.first(); hwdevice; hwdevice = cardReaderList.next()) {
				cdevice = static_cast<TDECryptographicCardDevice*>(hwdevice);
				X509CertificatePtrList certList = cdevice->cardX509Certificates();
				if (certList.count() > 0) {
					allow_card = true;
					card_cert_x509 = certList[0];
					card_cert = KSSLCertificate::fromX509(certList[0]);
				}
			}
			TQByteArray new_password;
			CryptPasswordDialog* passDlg = new CryptPasswordDialog(this, i18n("Enter the new LUKS password for key slot %1").arg(key_slot), TQString::null, allow_card, card_cert, &use_card);
			if (passDlg->exec() == TQDialog::Accepted) {
				new_password = passDlg->password();
				if (allow_card && use_card) {
					// Create new private key for disk device
					if (!TQDir("/etc/trinity/luks").exists()) {
						TQDir directory;
						if (!directory.mkdir("/etc/trinity/luks", true)) {
							KMessageBox::error(this, i18n("<qt><b>Key creation failed</b><br>Please check that you have write access to /etc/trinity and try again</qt>"), i18n("Key creation failure"));
							delete card_cert;
							return;
						}
					}
					if (!TQDir("/etc/trinity/luks/card").exists()) {
						TQDir directory;
						if (!directory.mkdir("/etc/trinity/luks/card", true)) {
							KMessageBox::error(this, i18n("<qt><b>Key creation failed</b><br>Please check that you have write access to /etc/trinity/luks and try again</qt>"), i18n("Key creation failure"));
							delete card_cert;
							return;
						}
					}
					TQString cryptoFileName = TQString("/etc/trinity/luks/card/%1_slot%2").arg(disk_uuid).arg(key_slot);
					TQFile file(cryptoFileName);
					if (file.exists()) {
						if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to overwrite an existing card key for LUKS key slot %1</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(key_slot), i18n("Confirmation Required")) != KMessageBox::Yes) {
							delete card_cert;
							return;
						}
					}
					if (file.open(IO_WriteOnly)) {
						TQByteArray randomKey;
						TQByteArray encryptedRandomKey;

						// Create a new secret key using the public key from the card certificate
						if (TDECryptographicCardDevice::createNewSecretRSAKeyFromCertificate(randomKey, encryptedRandomKey, card_cert_x509) < 0) {
							KMessageBox::error(this, i18n("<qt><b>Key creation failed</b><br>Unable to create new secret key using the provided X509 certificate</qt>"), i18n("Key creation failure"));
							delete card_cert;
							return;
						}

						// Write the encrypted key file to disk
						file.writeBlock(encryptedRandomKey, encryptedRandomKey.size());
						file.close();

						// Use the secret key as the LUKS passcode
						new_password = randomKey;
						luks_card_key_modified = true;
					}
					else {
						KMessageBox::error(this, i18n("<qt><b>Key creation failed</b><br>Please check that you have write access to /etc/trinity/luks/card and try again</qt>"), i18n("Key creation failure"));
						delete card_cert;
						return;
					}
				}
				delete passDlg;
				if (!sdevice->cryptOperationsUnlockPasswordSet()) {
					TQCString password;
					passDlg = new CryptPasswordDialog(this, i18n("Enter the LUKS device unlock password"), TQString::null, allow_card, card_cert, &use_card);
					if (passDlg->exec() == TQDialog::Accepted) {
						TQByteArray unlockPassword = passDlg->password();
						if (use_card) {
							// List all matching keys in directory and try each in turn...
							TQDir luksKeyDir("/etc/trinity/luks/card/");
							luksKeyDir.setFilter(TQDir::Files);
							luksKeyDir.setSorting(TQDir::Unsorted);

							TQValueList<TQByteArray> luksCryptedList;
							TQValueList<TQByteArray> luksDecryptedList;
							TQValueList<int> luksSlotNumberList;

							const TQFileInfoList *luksKeyDirList = luksKeyDir.entryInfoList();
							TQFileInfoListIterator it(*luksKeyDirList);
							TQFileInfo *luksKeyFileInfo;
							TQString errstr;
							while ((luksKeyFileInfo = it.current()) != 0) {
								if (luksKeyFileInfo->fileName().startsWith(disk_uuid) && luksKeyFileInfo->fileName().contains("_slot")) {
									// Found candidate, try decryption
									TQFile luksKeyFile(luksKeyFileInfo->absFilePath());
									if (luksKeyFile.open(IO_ReadOnly)) {
										TQByteArray keycrypted = luksKeyFile.readAll();
										luksCryptedList.append(keycrypted);

										// Parse the file name and find the matching key slot
										int current_card_keyslot = -1;
										TQString fileName = luksKeyFile.name();
										int pos = fileName.find("_slot");
										if (pos >= 0) {
											fileName.remove(0, pos + strlen("_slot"));
											current_card_keyslot = fileName.toInt();
											luksSlotNumberList.append(current_card_keyslot);
										}
									}
								}
								++it;
							}

							// Decrypt LUKS keys
							TQValueList<int> retCodeList;
							retcode = cdevice->decryptDataEncryptedWithCertPublicKey(luksCryptedList, luksDecryptedList, retCodeList, &errstr);
							TQValueList<TQByteArray>::iterator it2;
							TQValueList<int>::iterator it3;
							TQValueList<int>::iterator it4;
							for (it2 = luksDecryptedList.begin(), it3 = retCodeList.begin(), it4 = luksSlotNumberList.begin(); it2 != luksDecryptedList.end(); ++it2, ++it3, ++it4) {
								TQByteArray luksKeyData = *it2;
								retcode = *it3;
								int current_card_keyslot = *it4;
								if (retcode == -3) {
									// User cancelled
									break;
								}
								if (retcode < 0) {
									// ERROR
								}
								else {
									// Key decryption successful, try to open LUKS device...
									sdevice->cryptSetOperationsUnlockPassword(luksKeyData);
									if (sdevice->cryptCheckKey(current_card_keyslot) == TDELUKSResult::Success) {
										break;
									}
									else {
										sdevice->cryptClearOperationsUnlockPassword();
									}
								}
							}
							if (!sdevice->cryptOperationsUnlockPasswordSet()) {
								KMessageBox::error(this, i18n("<qt><b>Key write failed</b><br>Please check the LUKS password and try again</qt>"), i18n("Key write failure"));
							}
						}
						else {
							sdevice->cryptSetOperationsUnlockPassword(unlockPassword);
						}
					}
					delete passDlg;
				}
				if (sdevice->cryptOperationsUnlockPasswordSet()) {
					if ((lvi->text(1) == sdevice->cryptKeySlotFriendlyName(TDELUKSKeySlotStatus::Inactive)) || (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to overwrite the key in key slot %1</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(key_slot), i18n("Confirmation Required")) == KMessageBox::Yes)) {
						if (sdevice->cryptAddKey(key_slot, new_password) != TDELUKSResult::Success) {
							sdevice->cryptClearOperationsUnlockPassword();
							KMessageBox::error(this, i18n("<qt><b>Key write failed</b><br>Please check the LUKS password and try again</qt>"), i18n("Key write failure"));
						}
						else {
							if (luks_card_key_modified) {
								if (KMessageBox::warningYesNo(this, i18n("<qt><b>You have created a new card-dependent key</b><br>Card-dependent keys work in conjunction with an encrypted key file stored on the host system.<br>When a card is used to boot, card-dependent keys must be updated in the initramfs image to become usable.<p>Would you like to update the initramfs image now?</qt>"), i18n("Update Required")) == KMessageBox::Yes) {
									// Update the initramfs
									if (system("update-initramfs -u -k all") != 0) {
										KMessageBox::error(this, i18n("<qt><b>Initramfs update failed</b><br>Card-dependent keys may not be available for use until the root storage device is available / unlocked</qt>"), i18n("Initramfs update failure"));
									}
								}
							}
						}
					}
				}
			}
			else {
				delete passDlg;
			}
			delete card_cert;
		}
	}

	cryptLUKSPopulateList();
}

void DevicePropertiesDialog::cryptLUKSDelKey() {
	if (m_device->type() == TDEGenericDeviceType::Disk) {
		TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

		TQListViewItem* lvi = base->cryptLUKSKeySlotList->selectedItem();
		if (lvi) {
			unsigned int key_slot = lvi->text(0).toUInt();
			if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to purge the key in key slot %1</b><br>This action cannot be undone<p>Are you sure you want to proceed?</qt>").arg(lvi->text(0)), i18n("Confirmation Required")) == KMessageBox::Yes) {
				if (sdevice->cryptKeySlotStatus()[key_slot] & TDELUKSKeySlotStatus::Last) {
					if (KMessageBox::warningYesNo(this, i18n("<qt><b>You are about to purge the last active key from the device!</b><p>This action will render the contents of the encrypted device permanently inaccessable and cannot be undone<p>Are you sure you want to proceed?</qt>"), i18n("Confirmation Required")) != KMessageBox::Yes) {
						cryptLUKSPopulateList();
						return;
					}
				}
				if (sdevice->cryptDelKey(key_slot) != TDELUKSResult::Success) {
					sdevice->cryptClearOperationsUnlockPassword();
					KMessageBox::error(this, i18n("<qt><b>Key purge failed</b><br>The key in key slot %1 is still active</qt>").arg(lvi->text(0)), i18n("Key purge failure"));
				}
				else {
					// See if there was a cryptographic card key associated with this device and slot
					TQString disk_uuid = sdevice->diskUUID();
					TQDir luksKeyDir("/etc/trinity/luks/card/");
					luksKeyDir.setFilter(TQDir::Files);
					luksKeyDir.setSorting(TQDir::Unsorted);

					const TQFileInfoList *luksKeyDirList = luksKeyDir.entryInfoList();
					TQFileInfoListIterator it(*luksKeyDirList);
					TQFileInfo *luksKeyFileInfo;
					TQString errstr;
					while ((luksKeyFileInfo = it.current()) != 0) {
						if (luksKeyFileInfo->fileName().startsWith(disk_uuid) && luksKeyFileInfo->fileName().contains("_slot")) {
							// Parse the file name and find the matching key slot
							int current_card_keyslot = -1;
							TQString fileName = luksKeyFileInfo->absFilePath();
							TQString fileNameSlot = fileName;
							int pos = fileNameSlot.find("_slot");
							if (pos >= 0) {
								fileNameSlot.remove(0, pos + strlen("_slot"));
								current_card_keyslot = fileNameSlot.toInt();
								if (current_card_keyslot >= 0) {
									if ((unsigned int)current_card_keyslot == key_slot) {
										if (!TQFile(fileName).remove()) {
											KMessageBox::error(this, i18n("<qt><b>Card key purge failed</b><br>The card key for slot %1 has been fully deactivated but is still present on your system<br>This does not present a significant security risk</qt>").arg(lvi->text(0)), i18n("Key purge failure"));
										}
										break;
									}
								}
							}
						}
						++it;
					}
				}
			}
		}
	}

	cryptLUKSPopulateList();
}

void DevicePropertiesDialog::cryptLUKSPopulateList() {
	unsigned int i;
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	base->cryptLUKSKeySlotList->clear();
	unsigned int count = sdevice->cryptKeySlotCount();
	TDELUKSKeySlotStatusList status = sdevice->cryptKeySlotStatus();
	for (i = 0; i < count; i++) {
		new TQListViewItem(base->cryptLUKSKeySlotList, TQString("%1").arg(i), sdevice->cryptKeySlotFriendlyName(status[i]));
	}

	processLockouts();
}

void DevicePropertiesDialog::processLockouts() {
	if (m_device->type() == TDEGenericDeviceType::Disk) {
		TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

		TQListViewItem* lvi = base->cryptLUKSKeySlotList->selectedItem();
		if (lvi) {
			if (lvi->text(1) == sdevice->cryptKeySlotFriendlyName(TDELUKSKeySlotStatus::Active)) {
				base->cryptLUKSAddKey->setEnabled(true);
				base->cryptLUKSDelKey->setEnabled(true);
			}
			else {
				base->cryptLUKSAddKey->setEnabled(true);
				base->cryptLUKSDelKey->setEnabled(false);
			}
		}
		else {
			base->cryptLUKSAddKey->setEnabled(false);
			base->cryptLUKSDelKey->setEnabled(false);
		}
	}
}

void DevicePropertiesDialog::virtual_hook( int id, void* data )
{ KDialogBase::virtual_hook( id, data ); }

#include "devicepropsdlg.moc"
