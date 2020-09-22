/*
 *  searchwidget.h - part of the TDE Help Center
 *
 *  Copyright (C) 1999 Matthias Elter (me@kde.org)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __searchwidget_h__
#define __searchwidget_h__

#include <tqwidget.h>

#include <dcopobject.h>

#include "docmetainfo.h"

class TQCheckBox;
class TQListView;
class TQListViewItem;
class TQComboBox;

class KLanguageCombo;
class TDEConfig;

class KCMHelpCenter;

namespace KHC {

class ScopeItem;
class SearchEngine;

class SearchWidget : public TQWidget, public DCOPObject
{
    Q_OBJECT
    K_DCOP

  k_dcop:
    ASYNC searchIndexUpdated(); // called from kcmhelpcenter

  public:
    SearchWidget ( SearchEngine *, TQWidget *parent = 0 );
    ~SearchWidget();

    TQString method();
    int pages();
    TQString scope();

    TQListView *listView() { return mScopeListView; }

    enum { ScopeDefault, ScopeAll, ScopeNone, ScopeCustom, ScopeNum };

    TQString scopeSelectionLabel( int ) const;

    void readConfig( TDEConfig * );
    void writeConfig( TDEConfig * );

    int scopeCount() const;

    SearchEngine *engine() const { return mEngine; }

  signals:
    void searchResult( const TQString &url );
    void scopeCountChanged( int );
    void showIndexDialog();

  public slots:
    void slotSwitchBoxes();
    void scopeSelectionChanged( int );
    void updateScopeList();

  protected:
    void checkScope();

  protected slots:
    void scopeDoubleClicked( TQListViewItem * );
    void scopeClicked( TQListViewItem * );

  private:
    void loadLanguages();

    SearchEngine *mEngine;

    TQComboBox *mMethodCombo;
    TQComboBox *mPagesCombo;
    TQComboBox *mScopeCombo;
    TQListView *mScopeListView;

    int mScopeCount;
};

}

#endif
