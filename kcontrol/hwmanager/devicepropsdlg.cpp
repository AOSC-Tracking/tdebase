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
#include <tqinternal_p.h>
#undef Unsorted // Required for --enable-final (tqdir.h)
#include <tqfiledialog.h>

#include <kactivelabel.h>
#include <kbuttonbox.h>
#include <kcombobox.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <klineedit.h>
#include <kstdguiitem.h>
#include <tdemessagebox.h>

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

		if (m_device->type() == TDEGenericDeviceType::CPU) {
			connect(base->comboCPUGovernor, TQT_SIGNAL(activated(const TQString &)), this, TQT_SLOT(setCPUGovernor(const TQString &)));
		}
		if (m_device->type() == TDEGenericDeviceType::Disk) {
			connect(base->buttonDiskMount, TQT_SIGNAL(clicked()), this, TQT_SLOT(mountDisk()));
			connect(base->buttonDiskUnmount, TQT_SIGNAL(clicked()), this, TQT_SLOT(unmountDisk()));
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
			connect(base->sliderBacklightBrightness, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(setBacklightBrightness(int)));
		}
		if (m_device->type() == TDEGenericDeviceType::RootSystem) {
			connect(base->comboSystemHibernationMethod, TQT_SIGNAL(activated(int)), this, TQT_SLOT(setHibernationMethod(int)));
		}

		TQGridLayout *mainGrid = new TQGridLayout(plainPage(), 1, 1, 0, spacingHint());
		mainGrid->setRowStretch(1, 1);
		mainGrid->addWidget(base, 0, 0);
	}

	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();

	connect(hwdevices, TQT_SIGNAL(hardwareRemoved(TDEGenericDevice*)), this, TQT_SLOT(processHardwareRemoved(TDEGenericDevice*)));
	connect(hwdevices, TQT_SIGNAL(hardwareUpdated(TDEGenericDevice*)), this, TQT_SLOT(processHardwareUpdated(TDEGenericDevice*)));

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

void DevicePropertiesDialog::populateDeviceInformation() {
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

			// Show status
			TQString status_text = "<qt>";
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable)) {
				status_text += "Mountable<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Removable)) {
				status_text += "Removable<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Inserted)) {
				status_text += "Inserted<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Blank)) {
				status_text += "Blank<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsedByDevice)) {
				status_text += "In use<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::UsesDevice)) {
				status_text += "Uses other device<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::ContainsFilesystem)) {
				status_text += "Contains a filesystem<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hotpluggable)) {
				status_text += "Hotpluggable<br>";
			}
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Hidden)) {
				status_text += "Hidden<br>";
			}
			if (status_text == "<qt>") {
				status_text += "<i>Unavailable</i>";
			}
			status_text += "</qt>";
			base->labelDiskStatus->setText(status_text);

			// Update mount/unmount button status
			if (sdevice->checkDiskStatus(TDEDiskDeviceStatus::Mountable)) {
				base->groupDiskActions->show();
				base->buttonDiskMount->setEnabled((sdevice->mountPath() == ""));
				base->buttonDiskUnmount->setEnabled((sdevice->mountPath() != ""));
			}
			else {
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
			base->labelRandrWarning->setText("<qt><b>NOTE: Any further integration of displays into TDE <i>REQUIRES</i> multi GPU support and other features slated for RandR 2.0.</b><p>Development on such features has been sorely lacking for well over a year as of 2012; if you want to see Linux come up to Windows and Macintosh standards in this area <i>please tell the Xorg developers</i> at http://www.x.org/wiki/XorgMailingLists<p>The TDE project badly needs these features before it can proceed with graphical monitor configuration tools:<br> * GPU object support<br> * The ability to query the active driver name for any Xorg output<p><b>To recap, this is <i>not a TDE shortcoming</i>, but rather is the result of a lack of fundamental Linux support for graphics configuration!</b></qt>");
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
					if ((*it) == TDESystemPowerState::Standby) {
						powerStatesString += i18n("Standby<br>");
					}
					if ((*it) == TDESystemPowerState::Freeze) {
						powerStatesString += i18n("Freeze<br>");
					}
					if ((*it) == TDESystemPowerState::Suspend) {
						powerStatesString += i18n("Suspend<br>");
					}
					if ((*it) == TDESystemPowerState::Hibernate) {
						powerStatesString += i18n("Hibernate<br>");
					}
					if ((*it) == TDESystemPowerState::PowerOff) {
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
					if ((*it) == TDESystemHibernationMethod::Platform) {
						label = i18n("Platform");
					}
					if ((*it) == TDESystemHibernationMethod::Shutdown) {
						label = i18n("Shutdown");
					}
					if ((*it) == TDESystemHibernationMethod::Reboot) {
						label = i18n("Reboot");
					}
					if ((*it) == TDESystemHibernationMethod::TestProc) {
						label = i18n("Test Procedure");
					}
					if ((*it) == TDESystemHibernationMethod::Test) {
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

	// FIXME
	// This can only mount normal volumes
	TQString qerror;
	TQString diskLabel = sdevice->diskLabel();
	if (diskLabel.isNull()) {
		diskLabel = i18n("%1 Removable Device").arg(sdevice->deviceFriendlySize());
	}
	TDEStorageMountOptions mountOptions;
	TQString mountMessages;
	TQString mountedPath = sdevice->mountDevice(diskLabel, mountOptions, &mountMessages);
	if (mountedPath.isNull()) {
		qerror = i18n("<qt>Unable to mount this device.<p>Potential reasons include:<br>Improper device and/or user privilege level<br>Corrupt data on storage device");
		if (!mountMessages.isNull()) {
			qerror.append(i18n("<p>Technical details:<br>").append(mountMessages));
		}
		qerror.append("</qt>");
	}
	else {
		qerror = "";
	}

	if (qerror != "") KMessageBox::error(this, qerror, i18n("Mount Failed"));

	populateDeviceInformation();
}

void DevicePropertiesDialog::unmountDisk() {
	TDEStorageDevice* sdevice = static_cast<TDEStorageDevice*>(m_device);

	TQString qerror;
	TQString unmountMessages;
	int unmountRetcode = 0;
	if (!sdevice->unmountDevice(&unmountMessages, &unmountRetcode)) {
		// Unmount failed!
		qerror = "<qt>" + i18n("Unfortunately, the device could not be unmounted.");
		if (!unmountMessages.isNull()) {
			qerror.append(i18n("<p>Technical details:<br>").append(unmountMessages));
		}
		qerror.append("</qt>");
	}

	if (qerror != "") KMessageBox::error(this, qerror, i18n("Unmount Failed"));

	populateDeviceInformation();
}

void DevicePropertiesDialog::virtual_hook( int id, void* data )
{ KDialogBase::virtual_hook( id, data ); }

#include "devicepropsdlg.moc"
