/*
 *  This file is part of the TDE Help Center
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

#ifndef __navigator_h__
#define __navigator_h__

#include "glossary.h"

#include <tdelistview.h>
#include <kurl.h>

#include <tqfile.h>
#include <tqtextstream.h>
#include <tqtimer.h>
#include <tqptrlist.h>
#include <tqtabwidget.h>
#include <tqlistview.h>
#include <tqdict.h>

class TQPushButton;

class TDEListView;
class KService;
class TDEProcess;
class TDEProcIO;

class KCMHelpCenter;

namespace KHC {

class NavigatorItem;
class Navigator;
class View;
class SearchEngine;
class SearchWidget;
class Formatter;

class Navigator : public TQWidget
{
    TQ_OBJECT
  public:
    Navigator(View *, TQWidget *parent=0, const char *name=0);
    virtual ~Navigator();

    KURL homeURL();

    SearchEngine *searchEngine() const;
    Formatter *formatter() const;

    const GlossaryEntry &glossEntry(const TQString &term) const { return mGlossaryTree->entry( term ); }

    void insertParentAppDocs( const TQString &name, NavigatorItem *parent );
    void insertAppletDocs( NavigatorItem *parent );
    NavigatorItem *insertScrollKeeperDocs( NavigatorItem *parentItem,
                                 NavigatorItem *after );
    void insertInfoDocs( NavigatorItem *parentItem );
    void insertIOSlaveDocs(const TQString &, NavigatorItem*parent);
    
    void createItemFromDesktopFile( NavigatorItem *item, const TQString &name );

    bool showMissingDocs() const;

    void clearSelection();

    void showOverview( NavigatorItem *item, const KURL &url );

    void readConfig();
    void writeConfig();

  public slots:
    void openInternalUrl( const KURL &url );
    void slotItemSelected(TQListViewItem* index);
    void slotSearch();
    void slotShowSearchResult( const TQString & );
    void slotSelectGlossEntry( const TQString &id );
    void selectItem( const KURL &url );
    void showIndexDialog();

  signals:
    void itemSelected(const TQString& itemURL);
    void glossSelected(const GlossaryEntry &entry);

  protected slots:
    void slotSearchFinished();
    void slotTabChanged( TQWidget * );
    void checkSearchButton();

    bool checkSearchIndex();

    void clearSearch();

  protected:
    TQString createChildrenList( TQListViewItem *child );

  private:
    void setupContentsTab();
    void setupIndexTab();
    void setupSearchTab();
    void setupGlossaryTab();

    void insertPlugins();
    void hideSearch();

    TDEListView *mContentsTree;
    Glossary *mGlossaryTree;

    SearchWidget *mSearchWidget;
    KCMHelpCenter *mIndexDialog;

    TQTabWidget *mTabWidget;

    TQFrame *mSearchFrame;
    TQLineEdit *mSearchEdit;
    TQPushButton *mSearchButton;

    TQPtrList<NavigatorItem> manualItems, pluginItems;

    bool mShowMissingDocs;
    
    SearchEngine *mSearchEngine;

    View *mView;

    KURL mHomeUrl;
    
    bool mSelected;

    KURL mLastUrl;

    int mDirLevel;
};

}

#endif
