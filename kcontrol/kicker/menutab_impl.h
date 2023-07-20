/*
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */

#ifndef __menutab_impl_h__
#define __menutab_impl_h__

#include <tqlistview.h>
#include <stdlib.h>

#include <tdeshortcut.h>
#include <kpushbutton.h>

#include "menutab.h"

class kSubMenuItem : public TQObject, public TQCheckListItem
{
    TQ_OBJECT

    public:
        kSubMenuItem(TQListView* parent, 
                     const TQString& visibleName,
                     const TQString& desktopFile,
                     const TQPixmap& icon,
                     bool checked);
        ~kSubMenuItem() {}

        TQString desktopFile();

    signals:
        void toggled(bool);

    protected:
        void stateChange(bool state);

        TQString m_desktopFile;
};

class MenuTab : public MenuTabBase
{
    TQ_OBJECT

public:
    MenuTab( TQWidget *parent=0, const char* name=0 );

    void load();
    void load( bool useDefaults );
    void save();
    void defaults();

signals:
    void changed();

public slots:
    void launchMenuEditor();
    void menuStyleChanged();
    void launchIconEditor();
    void kmenuChanged();
    void setSearchShortcut(const TDEShortcut &cut);

protected:
    kSubMenuItem *m_bookmarkMenu;
    kSubMenuItem *m_quickBrowserMenu;
    TQString m_kmenu_icon;
    bool m_kmenu_button_changed;

};

#endif

