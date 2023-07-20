/*
 *   Copyright (C) 2000 Matthias Elter <elter@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef __basictab_h__
#define __basictab_h__

#include <tqwidget.h>
#include <tqstring.h>

#include <klineedit.h>

class KKeyButton;
class KLineEdit;
class TDEIconButton;
class TQCheckBox;
class TQGroupBox;
class TQLabel;
class KURLRequester;
class KComboBox;
class KService;

class MenuFolderInfo;
class MenuEntryInfo;

class BasicTab : public TQWidget
{
    TQ_OBJECT

public:
    BasicTab( TQWidget *parent=0, const char *name=0 );

    void apply();
signals:
    void changed( MenuFolderInfo * );
    void changed( MenuEntryInfo * );
    void findServiceShortcut(const TDEShortcut&, KService::Ptr &);

public slots:
    void setFolderInfo(MenuFolderInfo *folderInfo);
    void setEntryInfo(MenuEntryInfo *entryInfo);
    void slotDisableAction();
protected slots:
    void slotChanged();
    void launchcb_clicked();
    void systraycb_clicked();
    void termcb_clicked();
    void uidcb_clicked();
    void slotCapturedShortcut(const TDEShortcut&);
    void slotExecSelected();

protected:
    void enableWidgets(bool isDF, bool isDeleted);

protected:
    KLineEdit    *_nameEdit, *_commentEdit;
    KLineEdit	 *_descriptionEdit;
    KKeyButton   *_keyEdit;
    KURLRequester *_execEdit, *_pathEdit;
    KLineEdit    *_termOptEdit, *_uidEdit;
    TQCheckBox    *_terminalCB, *_uidCB, *_launchCB, *_systrayCB;
    TDEIconButton  *_iconButton;
    TQGroupBox    *_path_group, *_term_group, *_uid_group, *general_group_keybind;
    TQLabel *_termOptLabel, *_uidLabel, *_pathLabel, *_nameLabel, *_commentLabel, *_execLabel;
    TQLabel	*_descriptionLabel;

    MenuFolderInfo *_menuFolderInfo;
    MenuEntryInfo  *_menuEntryInfo;
    bool _isDeleted;
};

#endif
