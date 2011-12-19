/*****************************************************************

   Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.
   Copyright (c) 2006 Debajyoti Bera <dbera.web@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

******************************************************************/

#ifndef __k_new_mnu_h__
#define __k_new_mnu_h__

#include <dcopobject.h>
#include <tqintdict.h>
#include <tqpixmap.h>
#include <tqframe.h>
#include <tqtoolbutton.h>
#include <tqscrollview.h>
#include <tqtimer.h>
#include <tqbitmap.h>
#include <tqvbox.h>
#include <tqregexp.h>

#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>
#include "../interfaces/kickoff-search-plugin.h"

// #include "kmenubase.h"
#include "../core/kmenubase.h"
#include "service_mnu.h"
#include "query.h"

class KickerClientMenu;
class KickoffTabBar;
class KBookmarkMenu;
class KActionCollection;
class KBookmarkOwner;
class Panel;
class TQWidgetStack;
class KHistoryCombo;
class TQScrollView;
class PopupMenuTitle;
class MediaWatcher;
class KURIFilterData;
class KBookmarkGroup;
class KBookmarkManager;
class ItemView;
class FlipScrollView;
class TQListViewItem;
class KMenuItem;
class TQListView;
class TQTabBar;
class TQTab;

static TQString categories[14] = {I18N_NOOP("Actions"), I18N_NOOP("Applications"), I18N_NOOP("Bookmarks"),
                                 I18N_NOOP("Notes"), I18N_NOOP("Emails"),  I18N_NOOP("Files"), I18N_NOOP("Music"),
                                 I18N_NOOP("Browsing History"), I18N_NOOP("Chat Logs"), I18N_NOOP("Feeds"),
                                 I18N_NOOP("Pictures"), I18N_NOOP("Videos"), I18N_NOOP("Documentation"),
                                 I18N_NOOP("Others")};

static TQString kerry_categories[14] = {"contacts", "applications", "webpages", "everything", "conversations",
                                       "everything", "media", "webpages", "conversations", "webpages", "images",
                                       "media", "everything", "everything"};

enum MenuOrientation { BottomUp, TopDown, UnDetermined };
enum OverflowCategoryState { None, Filling, NotNeeded };

class KMenu : public KMenuBase
{
    Q_OBJECT
    Q_PROPERTY (bool KStyleMenuDropShadow READ useKStyleMenuDropShadow )

public:
    KMenu();
    ~KMenu();

    int insertClientMenu(KickerClientMenu *p);
    void removeClientMenu(int id);

    bool useKStyleMenuDropShadow() const { return true; }

    virtual void showMenu();
    virtual bool eventFilter(TQObject*, TQEvent*);

    void clearRecentAppsItems();
    void clearRecentDocsItems();
    bool highlightMenuItem(const TQString& /*id*/) { return false;}

    void selectFirstItem() {}
    void popup(const TQPoint&, int indexAtPoint);

    enum MaskEffect { Plain, Dissolve };

    virtual TQSize sizeHint() const;
    virtual TQSize minimumSizeHint() const;

    void searchOver();
    void initCategoryTitlesUpdate();
    bool anotherHitMenuItemAllowed(int cat, bool count=true);
    void addHitMenuItem(HitMenuItem*);
    void insertSearchResult(HitMenuItem* item);

    void updateCategoryTitles();

signals:
    void aboutToHide();
    void aboutToShow();

public slots:
    virtual void initialize();

    virtual void hide();
    virtual void show();

    void stackWidgetRaised(TQWidget*);

protected slots:
    void slotLock();
    void slotOpenHomepage();
    void slotLogout();
    void slotPopulateSessions();
    void slotSessionActivated( int );
    void slotGoSubMenu(const TQString& relPath);
    void slotGoBack();
    void slotGoExitMainMenu();
    void slotGoExitSubMenu(const TQString& url);
    void tabClicked(TQTab*);

    void paletteChanged();
    virtual void configChanged();
    void updateRecent();

    void initSearch();
    void searchAccept();
    void searchChanged(const TQString &);
    // when timeout happens or doQueryNow calls
    void doQuery (bool return_pressed = false);
    void searchActionClicked(TQListViewItem*);

    void slotStartService(KService::Ptr);
    void slotStartURL(const TQString&);
    void slotContextMenuRequested( TQListViewItem * item, const TQPoint & pos, int col );

    void clearedHistory();

    void slotSloppyTimeout();

    void slotContextMenu(int);
    void slotFavoritesMoved( TQListViewItem*, TQListViewItem*, TQListViewItem* );

    void updateMedia();
    void slotFavDropped(TQDropEvent * e, TQListViewItem *after );
    void slotSuspend(int id);

protected:
    virtual void paintEvent(TQPaintEvent *);
    virtual void resizeEvent ( TQResizeEvent * );
    virtual void mousePressEvent ( TQMouseEvent * e );
    virtual void mouseReleaseEvent ( TQMouseEvent * e );
    virtual void mouseMoveEvent ( TQMouseEvent * e );

    void doNewSession(bool lock);
    void createRecentMenuItems();
    void insertStaticItems();
    void insertStaticExitItems();
    void insertSuspendOption( int &id, int &index );
    virtual void clearSubmenus();
//    void raiseStackWidget(TQWidget *view);

    bool runCommand();

    void setupUi();

    void saveConfig();
    void searchProgramList(TQString relPath);
    void searchBookmarks(KBookmarkGroup);
    void searchAddressbook();

    void createNewProgramList();
    void createNewProgramList(TQString relPath);

    void paintSearchTab( bool active );

    void goSubMenu(const TQString& relPath, bool keyboard = false);
    void setOrientation(MenuOrientation orientation);

private:
    int serviceMenuStartId() { return 5242; }
    int serviceMenuEndId() { return 5242; }

    void fillMenu( KServiceGroup::Ptr &_root, KServiceGroup::List &_list,
		   const TQString &_relPath, ItemView* view, int & id );

    void fillSubMenu(const TQString& relPath, ItemView *view);

    TQPopupMenu                 *sessionsMenu;
    int                         client_id;
    bool                        delay_init;
    TQIntDict<KickerClientMenu>  clients;
    KActionCollection          *actionCollection;
    PopupMenuList               dynamicSubMenus;

    TQTimer                       m_sloppyTimer;
    TQTimer                       m_mediaFreeTimer;
    MediaWatcher               * m_mediaWatcher;
    TQRegion                      m_sloppyRegion;
    TQRect                        m_sloppySource;
    bool                         m_sloppySourceClicked;
    TQWidget                    * m_sloppyWidget;
    ItemView                   * m_recentlyView;
    ItemView                   * m_favoriteView;
    ItemView                   * m_searchResultsWidget;
    TQListView                  * m_searchActions;
    FlipScrollView             * m_browserView;
    ItemView                   * m_systemView;
    FlipScrollView             * m_exitView;
    TQVBox                      * m_searchWidget;
    TQLabel                     * m_resizeHandle;

    bool                       m_isresizing;
    // timer for search without pressing enter feature
    TQTimer *input_timer, *init_search_timer;

    Query current_query;

    bool dontQueryNow(const TQString &);

    // start timeout timer to call doQuery is enough time has passed since last keypress
    void checkToDoQuery (const TQString &);
    // when return is pressed
    void doQueryNow ();
    void clearSearchResults(bool showHelp = true);

    int *max_category_id; // maximum id in this category: max_category_id - base_category_id gives the current number of hits displayed in this category
    int *categorised_hit_total; // current number of hits returned in each category

    bool ensureServiceRunning(const TQString & service);

    TQString iconForHitMenuItem(HitMenuItem *hit_item);

    int getHitMenuItemPosition (HitMenuItem *hit_item);
    TQMap<TQString, TQString> mimetype_iconstore;
    TQMap<TQString, TQString> media_mimetypes;
    // report error as a menu item
    void reportError (TQString err);
    void addToHistory();

    int max_items(int category) const;
    TQString TOP_CATEGORY_STRING;
    bool *already_added;

    void notifyServiceStarted(KService::Ptr service);
    void parseLine( bool final );
    TQString m_iconName;
    TQStringList m_middleFilters;
    TQStringList m_finalFilters;
    KURIFilterData* m_filterData;
    TQPtrList<HitMenuItem> m_current_menu_items;
    TQListViewItem *m_searchIndex, *m_searchInternet;

    bool checkUriInMenu(const KURL &uri);

    TQRegExp emailRegExp,uriRegExp,uri2RegExp,authRegExp;

    KBookmarkManager *bookmarkManager;
    KABC::AddressBook* m_addressBook;

    enum ContextMenuEntry { AddItemToPanel, EditItem, AddMenuToPanel, EditMenu,
                            AddItemToDesktop, AddMenuToDesktop, PutIntoRunDialog,
                            AddToFavorites, RemoveFromFavorites, ClearRecentlyUsedApps, 
                            ClearRecentlyUsedDocs };
    struct PopupPath
    {
        TQString title, description, icon, path, menuPath; 
    };

    enum KickoffTabEntry { FavoriteTab, ApplicationsTab, ComputerTab,
        HistoryTab, LeaveTab, SearchTab, NumTabs };

    KPopupMenu* m_popupMenu;
    KService* m_popupService;
    PopupPath m_popupPath;

    KickoffTabBar* m_tabBar;
    TQTab* m_tabs[NumTabs];

    TQString newDesktopFile(const KURL& url, const TQString &directory);
    void updateRecentlyUsedApps(KService::Ptr &service);

    TQPixmap main_border_lc;
    TQPixmap main_border_rc;
    TQPixmap main_border_tl;
    TQPixmap main_border_tr;
    TQPixmap button_box_left;

    TQPixmap search_tab_left;
    TQPixmap search_tab_right;
    TQPixmap search_tab_center;

    TQPixmap search_tab_top_left;
    TQPixmap search_tab_top_right;
    TQPixmap search_tab_top_center;

    TQWidgetStack *m_stacker;

    TQStringList m_programsInMenu;
    TQStringList m_newInstalledPrograms, m_seenPrograms;
    bool m_seenProgramsChanged;
    TQString m_currentDate;

    MenuOrientation m_orientation;
    bool m_toolTipsEnabled;
    int m_media_id;

    bool m_recentDirty, m_browserDirty, m_kerryInstalled, m_isShowing;

    KickoffSearch::Plugin* m_search_plugin;
    TQObject* m_search_plugin_interface;

    OverflowCategoryState m_overflowCategoryState;
    TQPtrList<HitMenuItem> m_overflowList;
    int m_overflowCategory;

    void resetOverflowCategory();
    void fillOverflowCategory();

    TQString insertBreaks(const TQString& text, TQFontMetrics fm, int width, TQString leadInsert = TQString::null);
};

#endif
