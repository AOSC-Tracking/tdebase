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

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
    Please do not commit any changes without consulting me first. Thanks!

*/

#ifndef _ListView_h_
#define _ListView_h_

#include <tqlistview.h>
#include <tqpainter.h>

#include <SensorDisplay.h>

typedef const char* (*KeyFunc)(const char*);

class TQLabel;
class QBoxGroup;
class ListViewSettings;

class PrivateListView : public TQListView
{
	Q_OBJECT
public:
  enum ColumnType { Text, Int, Float, Time, DiskStat };

	PrivateListView(TQWidget *parent = 0, const char *name = 0);
	
	void addColumn(const TQString& label, const TQString& type);
	void removeColumns(void);
	void update(const TQString& answer);
	int columnType( uint pos ) const;

private:
  TQStringList mColumnTypes;
};

class PrivateListViewItem : public TQListViewItem
{
public:
	PrivateListViewItem(PrivateListView *parent = 0);

	void paintCell(TQPainter *p, const TQColorGroup &, int column, int width, int tqalignment) {
		TQColorGroup cgroup = _parent->colorGroup();
		TQListViewItem::paintCell(p, cgroup, column, width, tqalignment);
		p->setPen(cgroup.color(TQColorGroup::Link));
		p->drawLine(0, height() - 1, width - 1, height() - 1);
	}

	void paintFocus(TQPainter *, const TQColorGroup, const TQRect) {}

	virtual int compare( TQListViewItem*, int column, bool ascending ) const;

private:
	TQWidget *_parent;
};	

class ListView : public KSGRD::SensorDisplay
{
	Q_OBJECT
public:
	ListView(TQWidget* parent = 0, const char* name = 0,
			const TQString& = TQString::null, int min = 0, int max = 0);
	~ListView() {}

	bool addSensor(const TQString& hostName, const TQString& sensorName, const TQString& sensorType, const TQString& sensorDescr);
	void answerReceived(int id, const TQString& answer);
	void resizeEvent(TQResizeEvent*);
	void updateList();

	bool restoreSettings(TQDomElement& element);
	bool saveSettings(TQDomDocument& doc, TQDomElement& element, bool save = true);

	virtual bool hasSettingsDialog() const
	{
		return (true);
	}

	virtual void timerEvent(TQTimerEvent*)
	{
		updateList();
	}

	void configureSettings();

public slots:
	void applySettings();
	void applyStyle();

private:
	PrivateListView* monitor;
	ListViewSettings* lvs;
};

#endif
