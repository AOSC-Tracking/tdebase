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

#ifndef _SensorLogger_h
#define _SensorLogger_h

#include <tqdom.h>
#include <tqlabel.h>
#include <tqlineedit.h>
#include <tqlistview.h>
#include <tqpopupmenu.h>
#include <tqspinbox.h>
#include <tqstring.h>

#include <SensorDisplay.h>

#include "SensorLoggerDlg.h"

#define NONE -1

class SensorLoggerSettings;

class SLListViewItem : public TQListViewItem
{
public:
	SLListViewItem(TQListView *parent = 0);

	void setTextColor(const TQColor& color) { textColor = color; }

	void paintCell(TQPainter *p, const TQColorGroup &cg, int column, int width, int alignment) {
		TQColorGroup cgroup(cg);
		cgroup.setColor(TQColorGroup::Text, textColor);
		TQListViewItem::paintCell(p, cgroup, column, width, alignment);
	
	}

	void paintFocus(TQPainter *, const TQColorGroup, const TQRect) {}

private:
	TQColor textColor;
};	

class LogSensor : public TQObject, public KSGRD::SensorClient
{
	Q_OBJECT
public:
	LogSensor(TQListView *parent);
	~LogSensor(void);

	void answerReceived(int id, const TQString& answer);

	void setHostName(const TQString& name) { hostName = name; lvi->setText(3, name); }
	void setSensorName(const TQString& name) { sensorName = name; lvi->setText(2, name); }
	void setFileName(const TQString& name)
	{
		fileName = name;
		lvi->setText(4, name);
	}
	void setUpperLimitActive(bool value) { upperLimitActive = value; }
	void setLowerLimitActive(bool value) { lowerLimitActive = value; }
	void setUpperLimit(double value) { upperLimit = value; }
	void setLowerLimit(double value) { lowerLimit = value; }

	void setTimerInterval(int interval) {
		timerInterval = interval;

		if (timerID != NONE)
		{
			timerOff();
			timerOn();
		}

		lvi->setText(1, TQString("%1").arg(interval));
	}

	TQString getSensorName(void) { return sensorName; }
	TQString getHostName(void) { return hostName; }
	TQString getFileName(void) { return fileName; }
	int getTimerInterval(void) { return timerInterval; }
	bool getUpperLimitActive(void) { return upperLimitActive; }
	bool getLowerLimitActive(void) { return lowerLimitActive; }
	double getUpperLimit(void) { return upperLimit; }
	double getLowerLimit(void) { return lowerLimit; }
	TQListViewItem* getListViewItem(void) { return lvi; }

public slots:
	void timerOff()
	{
		killTimer(timerID);
		timerID = NONE;
	}

	void timerOn()
	{
		timerID = startTimer(timerInterval * 1000);
	}

	bool isLogging() { return timerID != NONE; }

	void startLogging(void);
	void stopLogging(void);

protected:
	virtual void timerEvent(TQTimerEvent*);

private:
	TQListView* monitor;
	SLListViewItem* lvi;
	TQPixmap pixmap_running;
	TQPixmap pixmap_waiting;
	TQString sensorName;
	TQString hostName;
	TQString fileName;

	int timerInterval;
	int timerID;

	bool lowerLimitActive;
	bool upperLimitActive;

	double lowerLimit;
	double upperLimit;
};

class SensorLogger : public KSGRD::SensorDisplay
{
	Q_OBJECT
public:
	SensorLogger(TQWidget *parent = 0, const char *name = 0, const TQString& title = 0);
	~SensorLogger(void);

	bool addSensor(const TQString& hostName, const TQString& sensorName, const TQString& sensorType,
				   const TQString& sensorDescr);

	bool editSensor(LogSensor*);

	void answerReceived(int id, const TQString& answer);
	void resizeEvent(TQResizeEvent*);

	bool restoreSettings(TQDomElement& element);
	bool saveSettings(TQDomDocument& doc, TQDomElement& element, bool save = true);

	void configureSettings(void);

	virtual bool hasSettingsDialog() const
	{
		return (true);
	}

public slots:
	void applySettings();
	void applyStyle();
	void RMBClicked(TQListViewItem*, const TQPoint&, int);

protected:
	LogSensor* getLogSensor(TQListViewItem*);

private:
	TQListView* monitor;

	TQPtrList<LogSensor> logSensors;

	SensorLoggerDlg *sld;
	SensorLoggerSettings *sls;
};

#endif // _SensorLogger_h
