/*
  Copyright (C) 2005 Ivor Hewitt <ivor@ivor.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef FILTEROPTS_H
#define FILTEROPTS_H

#include <tdecmodule.h>

class TQListBox;
class TQPushButton;
class TQLineEdit;
class TQListBoxItem;
class TQCheckBox;

class TDEConfig;

class KCMFilter : public TDECModule
{
    TQ_OBJECT
public:
    KCMFilter( TDEConfig* config, TQString group, TQWidget* parent = 0, const char* name = 0 );
    ~KCMFilter();
    
    void load();
    void load( bool useDefaults );
    void save();
    void defaults();
    TQString quickHelp() const;
    
public slots:

protected slots:
    void insertFilter();
    void updateFilter();
    void removeFilter();
    void slotItemSelected();
    void slotEnableChecked();
    void slotKillChecked();

    void exportFilters();
    void importFilters();

private:
    void updateButton();
    TQListBox *mListBox;
    TQLineEdit *mString;
    TQCheckBox *mEnableCheck;
    TQCheckBox *mKillCheck;
    TQPushButton *mInsertButton;
    TQPushButton *mUpdateButton;
    TQPushButton *mRemoveButton;
    TQPushButton *mImportButton;
    TQPushButton *mExportButton;

    TDEConfig *mConfig;
    TQString mGroupname;
    int mSelCount;
};

#endif
