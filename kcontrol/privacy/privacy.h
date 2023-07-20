/**
  * privacy.h
  *
  * Copyright (c) 2003 Ralf Hoelzer <ralf@well.com>
  *
  *  This program is free software; you can redistribute it and/or modify
  *  it under the terms of the GNU Lesser General Public License as published
  *  by the Free Software Foundation; either version 2.1 of the License, or
  *  (at your option) any later version.
  *
  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU Lesser General Public License for more details.
  *
  *  You should have received a copy of the GNU Lesser General Public License
  *  along with this program; if not, write to the Free Software
  *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  */

#ifndef _PRIVACY_H_
#define _PRIVACY_H_

#include <tdecmodule.h>
#include <tdelistview.h>

#include "kcmprivacydialog.h"
#include "kprivacymanager.h"
#include "kprivacysettings.h"

class Privacy: public TDECModule
{
    TQ_OBJECT

public:
    Privacy( TQWidget *parent=0, const char *name=0 );
    ~Privacy();

    virtual void load();
    virtual void load(bool useDefaults);
    virtual void save();
    virtual void defaults();

public slots:
    void cleanup();
    void selectAll();
    void selectNone();

private:
    KCMPrivacyDialog  *cleaningDialog;
    KPrivacySettings  *p3pSettings;
    KPrivacyManager *m_privacymanager;

    TQPtrList<TQCheckListItem> checklist;

    TDEListViewItem *generalCLI;
    TDEListViewItem *webbrowsingCLI;

    TQCheckListItem *clearThumbnails;	
    TQCheckListItem *clearRunCommandHistory;
    TQCheckListItem *clearAllCookies;
    TQCheckListItem *clearSavedClipboardContents;
    TQCheckListItem *clearWebHistory;
    TQCheckListItem *clearWebCache;
    TQCheckListItem *clearFormCompletion;
    TQCheckListItem *clearRecentDocuments;
    TQCheckListItem *clearQuickStartMenu;
    TQCheckListItem *clearFavIcons;
    TQCheckListItem *clearKPDFDocData;
    //TQCheckListItem *clearFileDialogHistory;


};

#endif
