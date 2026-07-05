/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef HISTORY_MODULE_H
#define HISTORY_MODULE_H

#include <tqdatetime.h>
#include <tqobject.h>
#include <tqdict.h>
#include <tqpixmap.h>

#include <tdeglobal.h>
#include <tdelocale.h>
#include <konq_sidebartreemodule.h>

#include "history_item.h"

class TDEActionCollection;
class KDialogBase;
class KonqSidebarHistorySettings;
class KonqSidebarTree;
class KonqSidebarTreeItem;

class KonqSidebarHistoryModule : public TQObject, public KonqSidebarTreeModule
{
    TQ_OBJECT

public:
    enum {
        ModuleContextMenu = 1,
        EntryContextMenu  = 2
    };

    KonqSidebarHistoryModule( KonqSidebarTree * parentTree, const char * name = 0 );
    virtual ~KonqSidebarHistoryModule();

    virtual void addTopLevelItem( KonqSidebarTreeTopLevelItem * item );
    virtual bool handleTopLevelContextMenu( KonqSidebarTreeTopLevelItem *item, const TQPoint& pos );

    void showPopupMenu( int which, const TQPoint& pos );

    // called by the items
    void showPopupMenu();
    void groupOpened( KonqSidebarHistoryGroupItem *item, bool open );
    const TQDateTime& currentTime() const { return m_currentTime; }
    bool sortsByName() const { return m_sortsByName; }

    static TQString groupForURL( const KURL& url ) {
	static const TQString& misc = TDEGlobal::staticTQString(i18n("Miscellaneous"));
	return url.host().isEmpty() ? misc : url.host();
    }

public slots:
    void clear();

private slots:
    void slotCreateItems();
    void slotEntryAdded( const KonqHistoryEntry * );
    void slotEntryRemoved( const KonqHistoryEntry * );

    void slotNewWindow();
    void slotRemoveEntry();
    void slotPreferences();
    void slotSettingsChanged();

    void slotItemExpanded( TQListViewItem * );

    void slotSortByName();
    void slotSortByDate();

    void slotClearHistory();

private:
    KonqSidebarHistoryGroupItem *getGroupItem( const KURL& url );

    void sortingChanged();
    typedef TQDictIterator<KonqSidebarHistoryGroupItem> HistoryItemIterator;
    TQDict<KonqSidebarHistoryGroupItem> m_dict;

    KonqSidebarTreeTopLevelItem * m_topLevelItem;

    TDEActionCollection *m_collection;

    KDialogBase *m_dlg;
    TQPixmap m_folderClosed;
    TQPixmap m_folderOpen;
    bool m_initialized;
    bool m_sortsByName;
    TQDateTime m_currentTime; // used for sorting the items by date
    static KonqSidebarHistorySettings *s_settings;
};

#endif // HISTORY_MODULE_H
