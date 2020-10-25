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
#ifndef __devicepropsdlg_h__
#define __devicepropsdlg_h__

#include <tqprogressbar.h>
#include <kdialogbase.h>
#include <tdehardwaredevices.h>

#include "devicepropsdlgbase.h"

class PasswordDlg;

/**
 *
 * Simple sensor name and text label value display widget
 *
 * @version 0.1
 * @author Timothy Pearson <kb9vqf@pearsoncomputing.net>
 */

class TDEUI_EXPORT SensorDisplayLabelsWidget : public TQWidget
{
	Q_OBJECT
public:
	/**
	* Create a simple sensor name and value display widget
	* @param parent     Parent widget for the display widget
	*/
	SensorDisplayLabelsWidget(TQWidget* parent);
	virtual ~SensorDisplayLabelsWidget();

	/**
	* Set sensor name
	* @param name A TQString with the name of the sensor
	*/
	void setSensorName(TQString name);

	/**
	* Set sensor value
	* @param value A TQString with the value of the sensor
	*/
	void setSensorValue(TQString value);

private:
	TQLabel* m_nameLabel;
	TQLabel* m_valueLabel;
};

class TDEUI_EXPORT SensorBar : public TQProgressBar
{
	Q_OBJECT
public:
	SensorBar(TQWidget* parent=0, const char* name=0, WFlags f=0) : TQProgressBar(parent, name, f) {}
	SensorBar(int totalSteps, TQWidget* parent=0, const char* name=0, WFlags f=0): TQProgressBar(totalSteps, parent, name, f) {}

protected:
	virtual bool setIndicator(TQString & progress_str, int progress, int totalSteps);
	virtual void drawContents(TQPainter *p);

public:
	TQString m_currentValueString;
	TQString m_maximumValueString;
	TQString m_minimumValueString;
	int m_currentLocation;
	int m_warningLocation;
	int m_criticalLocation;
};

/**
 *
 * Simple sensor information display widget
 *
 * @version 0.1
 * @author Timothy Pearson <kb9vqf@pearsoncomputing.net>
 */

class TDEUI_EXPORT SensorDisplayWidget : public TQWidget
{
	Q_OBJECT
public:
	/**
	* Simple sensor information display widget
	* @param parent     Parent widget for the display widget
	*/
	SensorDisplayWidget(TQWidget* parent);
	virtual ~SensorDisplayWidget();

	/**
	* Set sensor name
	* @param name A TQString with the name of the sensor
	*/
	void setSensorName(TQString name);

	/**
	* Set current sensor value
	* @param value A double with the current value of the sensor
	*/
	void setSensorCurrentValue(double value);

	/**
	* Set minimum sensor value
	* @param value A double with the minimum value of the sensor, < 0 if not supported
	*/
	void setSensorMinimumValue(double value);

	/**
	* Set maximum sensor value
	* @param value A double with the maximum value of the sensor, < 0 if not supported
	*/
	void setSensorMaximumValue(double value);

	/**
	* Set warning sensor value
	* @param value A double with the warning value of the sensor, < 0 if not supported
	*/
	void setSensorWarningValue(double value);

	/**
	* Set critical sensor value
	* @param value A double with the critical value of the sensor, < 0 if not supported
	*/
	void setSensorCriticalValue(double value);

	/**
	* Updates the sensor value display
	*/
	void updateDisplay();

private:
	TQLabel* m_nameLabel;
	SensorBar* m_progressBar;

	double m_current;
	double m_minimum;
	double m_maximum;
	double m_warning;
	double m_critical;
};

typedef TQPtrList<SensorDisplayWidget> SensorDisplayWidgetList;
typedef TQMap<TDESystemHibernationMethod::TDESystemHibernationMethod, int> HibernationComboMap;

/**
 *
 * Dialog to view and edit hardware device properties
 *
 * @version 0.1
 * @author Timothy Pearson <kb9vqf@pearsoncomputing.net>
 */

class TDEUI_EXPORT DevicePropertiesDialog : public KDialogBase
{
	Q_OBJECT
public:
	/**
	* Create a dialog that allows a user to view and edit hardware device properties
	* @param parent     Parent widget
	*/
	DevicePropertiesDialog(TDEGenericDevice* device, TQWidget *parent);
	virtual ~DevicePropertiesDialog();

protected:
	virtual void virtual_hook( int id, void* data );

private slots:
	void processHardwareRemoved(TDEGenericDevice*);
	void processHardwareUpdated(TDEGenericDevice*);
	void populateDeviceInformation();

	void setCPUGovernor(const TQString &);
	void setBacklightBrightness(int);
	void setHibernationMethod(int);

	void mountDisk();
	void unmountDisk();
	void unlockDisk();
	void doUnlockDisk();
	void lockDisk();

	void cryptLUKSAddKey();
	void cryptLUKSDelKey();
	void cryptLUKSPopulateList();

	void cryptographicCardInserted();
	void cryptographicCardRemoved();
	void updateCryptographicCardStatusDisplay();

	void processLockouts();

private:
	TDEGenericDevice* m_device;
	DevicePropertiesDialogBase* base;
	PasswordDlg *m_passDlg;

	TQGridLayout* m_sensorDataGrid;
	SensorDisplayWidgetList m_sensorDataGridWidgets;

	HibernationComboMap m_hibernationComboMap;
};

#endif
