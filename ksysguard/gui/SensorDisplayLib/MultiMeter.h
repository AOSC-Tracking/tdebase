/*
    KSysGuard, the KDE System Guard
   
	Copyright (c) 1999 - 2001 Chris Schlaeger <cs@kde.org>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

	KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
	Please do not commit any changes without consulting me first. Thanks!

*/

#ifndef _MultiMeter_h_
#define _MultiMeter_h_

#include <SensorDisplay.h>

class TQGroupBox;
class TQLCDNumber;
class TQLabel;
class MultiMeterSettings;

class MultiMeter : public KSGRD::SensorDisplay
{
	TQ_OBJECT

public:
	MultiMeter(TQWidget* parent = 0, const char* name = 0,
			   const TQString& = TQString::null, double min = 0, double max = 0, bool nf = false, bool isApplet = false);
	virtual ~MultiMeter()
	{
	}

	bool addSensor(const TQString& hostName, const TQString& sensorName,
				const TQString& sensorType, const TQString& sensorDescr);
	void answerReceived(int id, const TQString& answer);
	void resizeEvent(TQResizeEvent*);

	bool restoreSettings(TQDomElement& element);
	bool saveSettings(TQDomDocument& doc, TQDomElement& element, bool save = true);

	virtual bool hasSettingsDialog() const
	{
		return (true);
	}

	void configureSettings();

public slots:
	void applySettings();
	void applyStyle();

private:
	void setDigitColor(const TQColor& col);
	void setBackgroundColor(const TQColor& col);

	TQLCDNumber* lcd;
	TQColor normalDigitColor;
	TQColor alarmDigitColor;

	MultiMeterSettings* mms;
	bool lowerLimitActive;
	double lowerLimit;
	bool upperLimitActive;
	double upperLimit;
};

#endif
