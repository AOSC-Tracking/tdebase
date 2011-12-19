/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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

#ifndef __konq_actions_h__
#define __konq_actions_h__

#include <konq_historymgr.h>
#include <kaction.h>
#include <tqptrlist.h>

class HistoryEntry;
class TQPopupMenu;

/**
 * Plug this action into a menu to get a bidirectional history
 * (both back and forward, including current location)
 */
class KonqBidiHistoryAction : public KAction
{
  Q_OBJECT
public:
    KonqBidiHistoryAction( const TQString & text, TQObject* parent = 0, const char* name = 0 );

    virtual ~KonqBidiHistoryAction() {};

    virtual int plug( TQWidget *widget, int index = -1 );
    //virtual void unplug( TQWidget *widget );

    void fillGoMenu( const TQPtrList<HistoryEntry> &history );

    // Used by KonqHistoryAction and KonqBidiHistoryAction
    static void fillHistoryPopup( const TQPtrList<HistoryEntry> &history,
                           TQPopupMenu * popup,
                           bool onlyBack = false,
                           bool onlyForward = false,
                           bool checkCurrentItem = false,
                           uint startPos = 0 );

protected slots:
    void slotActivated( int );

signals:
    void menuAboutToShow();
    // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
    void activated( int );
private:
    uint m_firstIndex; // first index in the Go menu
    int m_startPos;
    int m_currentPos; // == history.at()
    TQPopupMenu *m_goMenu; // hack
};

/////

class KonqLogoAction : public KAction
{
  Q_OBJECT
public:
    KonqLogoAction( const TQString& text, int accel = 0, TQObject* parent = 0, const char* name = 0 );
    KonqLogoAction( const TQString& text, int accel,
	            TQObject* receiver, const char* slot, TQObject* parent, const char* name = 0 );
    KonqLogoAction( const TQString& text, const TQIconSet& pix, int accel = 0,
	            TQObject* parent = 0, const char* name = 0 );
    KonqLogoAction( const TQString& text, const TQIconSet& pix, int accel,
	            TQObject* receiver, const char* slot, TQObject* parent, const char* name = 0 );
    // text missing !
    KonqLogoAction( const TQStringList& icons, TQObject* receiver,
                    const char* slot, TQObject* parent, const char* name = 0 );

    virtual int plug( TQWidget *widget, int index = -1 );
    virtual void updateIcon(int id);

    void start();
    void stop();

private:
    TQStringList iconList;
};

class KonqViewModeAction : public KRadioAction
{
    Q_OBJECT
public:
    KonqViewModeAction( const TQString &text, const TQString &icon,
                        TQObject *parent, const char *name );
    virtual ~KonqViewModeAction();

    virtual int plug( TQWidget *widget, int index = -1 );

    TQPopupMenu *popupMenu() const { return m_menu; }

private slots:
    void slotPopupAboutToShow();
    void slotPopupActivated();
    void slotPopupAboutToHide();

private:
    bool m_popupActivated;
    TQPopupMenu *m_menu;
};

class MostOftenList : public KonqBaseHistoryList
{
protected:
    /**
     * Ensures that the items are sorted by numberOfTimesVisited
     */
    virtual int compareItems( TQPtrCollection::Item, TQPtrCollection::Item );
};

class KonqMostOftenURLSAction : public KActionMenu
{
    Q_OBJECT

public:
    KonqMostOftenURLSAction( const TQString& text, TQObject *parent,
			     const char *name );
    virtual ~KonqMostOftenURLSAction();

signals:
    void activated( const KURL& );

private slots:
    void slotHistoryCleared();
    void slotEntryAdded( const KonqHistoryEntry *entry );
    void slotEntryRemoved( const KonqHistoryEntry *entry );

    void slotFillMenu();
    //void slotClearMenu();

    void slotActivated( int );

private:
    void init();
    void parseHistory();

    static MostOftenList *s_mostEntries;
    static uint s_maxEntries;
    KURL::List m_popupList;
};

#endif
