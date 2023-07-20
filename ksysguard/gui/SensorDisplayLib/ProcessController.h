/*
    KSysGuard, the KDE System Guard

	Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

	KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
	Please do not commit any changes without consulting me first. Thanks!

*/

#ifndef _ProcessController_h_
#define _ProcessController_h_

#include <tqdict.h>
#include <tqwidget.h>

#include <tdeapplication.h>

#include <SensorDisplay.h>

#include "ProcessList.h"

class TQVBoxLayout;
class TQHBoxLayout;
class TQCheckBox;
class TQComboBox;
class KPushButton;
class TDEListViewSearchLineWidget;

extern TDEApplication* Kapp;

/**
 * This widget implements a process list page. Besides the process
 * list which is implemented as a ProcessList, it contains two
 * comboxes and two buttons.  The combo boxes are used to set the
 * update rate and the process filter.  The buttons are used to force
 * an immediate update and to kill a process.
 */
class ProcessController : public KSGRD::SensorDisplay
{
	TQ_OBJECT

public:
	ProcessController(TQWidget* parent = 0, const char* name = 0, const TQString &title = TQString::null, bool nf = false);
	virtual ~ProcessController() { }

	void resizeEvent(TQResizeEvent*);

	bool restoreSettings(TQDomElement& element);

	bool saveSettings(TQDomDocument& doc, TQDomElement& element, bool save = true);

	void refreshList(void)
	{
		updateList();
	}

	virtual void timerEvent(TQTimerEvent*)
	{
		updateList();
	}

	virtual bool addSensor(const TQString&, const TQString&, const TQString&, const TQString&);

	virtual void answerReceived(int id, const TQString& answer);

	virtual void sensorError(int, bool err);

	void configureSettings() { }

	virtual bool hasSettingsDialog() const
	{
		return (false);
	}

public slots:
	void setSearchFocus();
	void fixTabOrder();
	void filterModeChanged(int filter)
	{
		pList->setFilterMode(filter);
		updateList();
		setModified(true);
	}

	void setTreeView(bool tv)
	{
		pList->setTreeView(tv);
		updateList();
		setModified(true);
	}

	virtual void setModified(bool mfd)
	{
		if (mfd != modified())
		{
			SensorDisplay::setModified( mfd );
			if (!mfd)
				pList->setModified(0);
			emit modified(modified());
		}
	}

	void killProcess();
	void killProcess(int pid, int sig);

	void reniceProcess(const TQValueList<int> &pids, int niceValue);

	void updateList();

signals:
	void setFilterMode(int);

private:
	TQVBoxLayout* gm;

	bool killSupported;

	/// The process list.
	ProcessList* pList;
	///Layout for the search line and process filter combo box
	TQHBoxLayout* gmSearch;
	TDEListViewSearchLineWidget *pListSearchLine;
	
	TQHBoxLayout* gm1;

	/// Checkbox to switch between tree and list view
	TQCheckBox* xbTreeView;

	/// This combo boxes control the process filter.
	TQComboBox* cbFilter;

	/// These buttons force an immedeate refresh or kill a process.
	KPushButton* bRefresh;
	KPushButton* bKill;

	/// Dictionary for header translations.
	TQDict<TQString> dict;
};

#endif
