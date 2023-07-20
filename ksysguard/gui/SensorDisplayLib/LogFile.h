/*
    KSysGuard, the KDE System Guard

	Copyright (c) 2001 Tobias Koenig <tokoe@kde.org>

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

*/

#ifndef _LogFile_h
#define _LogFile_h

#define MAXLINES 500

class TQFile;
class TQListBox;

#include <tqdom.h>
#include <tqpopupmenu.h>
#include <tqstring.h>
#include <tqstringlist.h>

#include <SensorDisplay.h>

#include "LogFileSettings.h"

class LogFile : public KSGRD::SensorDisplay
{
	TQ_OBJECT
public:
	LogFile(TQWidget *parent = 0, const char *name = 0, const TQString& title = 0);
	~LogFile(void);

	bool addSensor(const TQString& hostName, const TQString& sensorName,
				   const TQString& sensorType, const TQString& sensorDescr);
	void answerReceived(int id, const TQString& answer);
	void resizeEvent(TQResizeEvent*);

	bool restoreSettings(TQDomElement& element);
	bool saveSettings(TQDomDocument& doc, TQDomElement& element, bool save = true);

	void updateMonitor(void);

	void configureSettings(void);

	virtual void timerEvent(TQTimerEvent*)
	{
		updateMonitor();
	}

	virtual bool hasSettingsDialog() const
	{
		return (true);
	}

public slots:
	void applySettings();
	void applyStyle();

	void settingsFontSelection();
	void settingsAddRule();
	void settingsDeleteRule();
	void settingsChangeRule();
	void settingsRuleListSelected(int index);

private:
	LogFileSettings* lfs;
	TQListBox* monitor;
	TQStringList filterRules;

	unsigned long logFileID;
};

#endif // _LogFile_h
