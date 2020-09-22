/*

   This file is part of the KDE project, module kcmbackground.

   Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef _BGWALLPAPER_H_
#define _BGWALLPAPER_H_

#include <tqlistbox.h>
#include <tqstringlist.h>

#include <kdialogbase.h>

class BGMultiWallpaperBase;
class KBackgroundSettings;

class BGMultiWallpaperList : public TQListBox
{
public:
   BGMultiWallpaperList(TQWidget *parent, const char *name);

   void dragEnterEvent(TQDragEnterEvent *ev);
   void dropEvent(TQDropEvent *ev);
   bool hasSelection();
   void ensureSelectionVisible();
};

class BGMultiWallpaperDialog : public KDialogBase
{
   Q_OBJECT
public:
   BGMultiWallpaperDialog(KBackgroundSettings *settings, TQWidget *parent, const char *name=0);

public slots:
   void slotAdd();
   void slotRemove();
   void slotMoveUp();
   void slotMoveDown();
   void slotOk();
   void slotItemSelected( TQListBoxItem * );
private:
   void setEnabledMoveButtons();

   KBackgroundSettings *m_pSettings;

   BGMultiWallpaperBase *dlg;
};

#endif
