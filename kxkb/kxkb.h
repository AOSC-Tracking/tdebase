/*
    Copyright (C) 2001, S.R.Haque <srhaque@iee.org>. Derived from an
    original by Matthias H�zer-Klpfel released under the QPL.
    This file is part of the KDE project

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

DESCRIPTION

    KDE Keyboard Tool. Manages XKB keyboard mappings.
*/
#ifndef __K_XKB_H__
#define __K_XKB_H__


#include <tqstring.h>
#include <tqstringlist.h>
#include <tqdict.h>
#include <tqptrqueue.h>

#include <tdeuniqueapplication.h>
#ifdef WITH_TDEHWLIB
#include <tdegenericdevice.h>
#else
#define TDEGenericDevice void
#endif

#include "kxkbtraywindow.h"
#include "kxkbconfig.h"


class XKBExtension;
class XkbRules;
class TDEGlobalAccel;
class TWinModule;
class LayoutMap;

/* This is the main Kxkb class responsible for reading options
    and switching layouts
*/

class KXKBApp : public TDEUniqueApplication
{
    TQ_OBJECT
    K_DCOP

public:
    KXKBApp(bool allowStyles=true, bool GUIenabled=true);
    ~KXKBApp();

    virtual int newInstance();

    bool setLayout(const LayoutUnit& layoutUnit);
    bool setLayout(const uint group);
k_dcop:
    bool setLayout(const TQString& layoutPair);
    TQString getCurrentLayout() { return m_currentLayout.toPair(); }
    TQStringList getLayoutsList() { return kxkbConfig.getLayoutStringList(); }

public slots:
    void nextLayout();
    void prevLayout();

protected slots:
    void menuActivated(int id);
    void windowChanged(WId winId);
    void slotGroupChanged(uint group);

    void slotSettingsChanged(int category);
    void showLayoutNotification();
    void showErrorNotification(TQString layout);

    void hardwareChanged(TDEGenericDevice *dev);

    void readSettings();
    void applyXkbOptions();
    void slotSyncXkbOptions();
    void syncXkbOptions();

private:
    void initTray();
    bool x11EventFilter(XEvent *e);
    bool isKMiloAvailable();

private:
    KxkbConfig kxkbConfig;

    WId m_prevWinId;	// for tricky part of saving xkb group
    LayoutMap* m_layoutOwnerMap;

    LayoutUnit m_currentLayout;

    XKBExtension *m_extension;
    XkbRules *m_rules;
    KxkbSystemTray *m_tray;
    TDEGlobalAccel *keys;
    TWinModule* tWinModule;
    bool m_forceSetXKBMap;
};

#endif
