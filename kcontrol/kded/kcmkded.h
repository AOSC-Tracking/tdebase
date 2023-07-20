/* This file is part of the KDE project
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */
#ifndef KCMKDED_H
#define KCMKDED_H

#include <tqlistview.h>

#include <tdecmodule.h>

class TDEListView;

class TQStringList;
class TQPushButton;

class KDEDConfig : public TDECModule
{
TQ_OBJECT
public:
	KDEDConfig(TQWidget* parent, const char* name= 0L, const TQStringList& foo = TQStringList());
	~KDEDConfig() {};

	void       load();
	void       load( bool useDefaults );
	void       save();
	void       defaults();

protected slots:
	void slotReload();
	void slotStartService();
	void slotStopService();
	void slotServiceRunningToggled();
	void slotEvalItem(TQListViewItem *item);
	void slotItemChecked(TQCheckListItem *item);
	void getServiceStatus();

        bool autoloadEnabled(TDEConfig *config, const TQString &filename);
        void setAutoloadEnabled(TDEConfig *config, const TQString &filename, bool b);

private:
	TDEListView *_lvLoD;
	TDEListView *_lvStartup;
	TQPushButton *_pbStart;
	TQPushButton *_pbStop;
	
	TQString RUNNING;
	TQString NOT_RUNNING;
};

class CheckListItem : public TQObject, public TQCheckListItem
{
	TQ_OBJECT
public:
	CheckListItem(TQListView* parent, const TQString &text);
	~CheckListItem() { }
signals:
	void changed(TQCheckListItem*);
protected:
	virtual void stateChange(bool);
};

#endif // KCMKDED_H

