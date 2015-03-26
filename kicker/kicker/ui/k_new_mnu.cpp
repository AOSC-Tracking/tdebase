/*****************************************************************

   Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.
   Copyright (c) 2006 Debajyoti Bera <dbera.web@gmail.com>
   Copyright (c) 2006 Dirk Mueller <mueller@kde.org>

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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <dmctl.h>
#include <inttypes.h>

#include <tqimage.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <tqwidgetstack.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqregexp.h>
#include <tqfile.h>
#include <tqstylesheet.h>
#include <tqaccel.h>
#include <tqcursor.h>
#include <tqdir.h>
#include <tqsimplerichtext.h>
#include <tqtooltip.h>
#include <tqtabbar.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <tdeabouttde.h>
#include <tdeaction.h>
#include <kbookmarkmenu.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#ifdef __TDE_HAVE_TDEHWLIB
#include <tdehardwaredevices.h>
#endif
#include <kiconloader.h>
#include <klineedit.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kstandarddirs.h>
#include <kcombobox.h>
#include <twin.h>
#include <kdebug.h>
#include <kuser.h>
#include <kurllabel.h>
#include <krun.h>
#include <kmimetype.h>
#include <tderecentdocument.h>
#include <tdecompletionbox.h>
#include <kurifilter.h>
#include <kbookmarkmanager.h>
#include <kbookmark.h>
#include <kprocess.h>
#include <tdeio/jobclasses.h>
#include <tdeio/job.h>
#include <dcopref.h>
#include <konq_popupmenu.h>
#include <konqbookmarkmanager.h>
#include <tdeparts/componentfactory.h>

#include "client_mnu.h"
#include "container_base.h"
#include "global.h"
#include "knewbutton.h"
#include "kicker.h"
#include "kickerSettings.h"
#include "konqbookmarkmanager.h"
#include "menuinfo.h"
#include "menumanager.h"
#include "popupmenutitle.h"
#include "quickbrowser_mnu.h"
#include "recentapps.h"
#include "flipscrollview.h"
#include "itemview.h"
#include <dmctl.h>
#ifdef __OpenBSD__
#include <sys/statvfs.h>
#include <sys/param.h>
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif
#include <mykickoffsearchinterface.h>

#include "media_watcher.h"
#include "k_mnu.h"
#include "k_new_mnu.h"
#include "k_new_mnu.moc"
#include "kickoff_bar.h"

#include "config.h"

#ifdef COMPILE_HALBACKEND
#ifndef NO_QT3_DBUS_SUPPORT
/* We acknowledge the the dbus API is unstable */
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/connection.h>
#endif // NO_QT3_DBUS_SUPPORT

#include <hal/libhal.h>
#endif // COMPILE_HALBACKEND

#define WAIT_BEFORE_QUERYING 700

#define IDS_PER_CATEGORY 20
#define ACTIONS_ID_BASE 10
#define APP_ID_BASE 10 + IDS_PER_CATEGORY
#define BOOKMARKS_ID_BASE 10 + (IDS_PER_CATEGORY * 2)
#define NOTES_ID_BASE 10 + (IDS_PER_CATEGORY * 3)
#define MAIL_ID_BASE 10 + (IDS_PER_CATEGORY * 4)
#define FILE_ID_BASE 10 + (IDS_PER_CATEGORY * 5)
#define MUSIC_ID_BASE 10 + (IDS_PER_CATEGORY * 6)
#define WEBHIST_ID_BASE 10 + (IDS_PER_CATEGORY * 7)
#define CHAT_ID_BASE 10 + (IDS_PER_CATEGORY * 8)
#define FEED_ID_BASE 10 + (IDS_PER_CATEGORY * 9)
#define PIC_ID_BASE 10 + (IDS_PER_CATEGORY * 10)
#define VIDEO_ID_BASE 10 + (IDS_PER_CATEGORY * 11)
#define DOC_ID_BASE 10 + (IDS_PER_CATEGORY * 12)
#define OTHER_ID_BASE 10 + (IDS_PER_CATEGORY * 13)

static TQString calculate(const TQString &exp)
{
   TQString result, cmd;
   const TQString bc = TDEStandardDirs::findExe("bc");
   if ( !bc.isEmpty() )
      cmd = TQString("echo %1 | %2").arg(TDEProcess::quote(exp), TDEProcess::quote(bc));
   else
      cmd = TQString("echo $((%1))").arg(exp);
   FILE *fs = popen(TQFile::encodeName(cmd).data(), "r");
   if (fs)
   {
      TQTextStream ts(fs, IO_ReadOnly);
      result = ts.read().stripWhiteSpace();
      pclose(fs);
   }
   return result;
}

static TQString workaroundStringFreeze(const TQString& str)
{
    TQString s = str;

    s.replace("<u>","&");
    TQRegExp re("<[^>]+>");
    re.setMinimal(true);
    re.setCaseSensitive(false);

    s.replace(re, "");
    s = s.simplifyWhiteSpace();

    return s;
}

int base_category_id[] = {ACTIONS_ID_BASE, APP_ID_BASE, BOOKMARKS_ID_BASE, NOTES_ID_BASE, MAIL_ID_BASE,
                          FILE_ID_BASE, MUSIC_ID_BASE, WEBHIST_ID_BASE, CHAT_ID_BASE, FEED_ID_BASE,
                          PIC_ID_BASE, VIDEO_ID_BASE, DOC_ID_BASE, OTHER_ID_BASE};

#include <assert.h>

static int used_size( TQLabel *label, int oldsize )
{
    TQSimpleRichText st( label->text(), TDEGlobalSettings::toolBarFont() );
    st.setWidth( oldsize );
    return TQMAX( st.widthUsed(), oldsize );
}

KMenu::KMenu()
  : KMenuBase(0, "SUSE::Kickoff::KMenu")
  , m_sloppyTimer(0, "KNewMenu::sloppyTimer"), m_mediaFreeTimer(0, "KNewMenu::mediaFreeTimer"),
    m_iconName(TQString()),  m_orientation(UnDetermined), m_search_plugin( 0 )
{
    setMouseTracking(true);
    connect(&m_sloppyTimer, TQT_SIGNAL(timeout()), TQT_SLOT(slotSloppyTimeout()));

    // set the first client id to some arbitrarily large value.
    client_id = 10000;
    // Don't automatically clear the main menu.
    actionCollection = new TDEActionCollection(this);

    connect(Kicker::the(), TQT_SIGNAL(configurationChanged()),
            this, TQT_SLOT(configChanged()));

    KUser * user = new KUser();

    char hostname[256];
    hostname[0] = '\0';
    if (!gethostname( hostname, sizeof(hostname) ))
      hostname[sizeof(hostname)-1] = '\0';

    m_userInfo->setText( i18n( "User&nbsp;<b>%1</b>&nbsp;on&nbsp;<b>%2</b>" )
                         .arg( user->loginName() ).arg( hostname ) );
    setupUi();

    m_userInfo->setBackgroundMode( PaletteBase );
    TQColor userInfoColor = TQApplication::palette().color( TQPalette::Normal, TQColorGroup::Mid );
    if ( tqGray( userInfoColor.rgb() ) > 120 )
        userInfoColor = userInfoColor.dark( 200 );
    else
        userInfoColor = userInfoColor.light( 200 );
    m_userInfo->setPaletteForegroundColor( userInfoColor );

    m_tabBar = new KickoffTabBar(this, "m_tabBar");
    connect(m_tabBar, TQT_SIGNAL(tabClicked(TQTab*)), TQT_SLOT(tabClicked(TQTab*)));

    const int tab_icon_size = 32;

    m_tabs[FavoriteTab] = new TQTab;
    m_tabBar->addTab(m_tabs[FavoriteTab]);
    m_tabBar->setToolTip(FavoriteTab, "<qt>" + i18n( "Most commonly used applications and documents" )  + "</qt>" );
    m_tabs[ApplicationsTab] = new TQTab;
    m_tabBar->addTab(m_tabs[ApplicationsTab]);
    m_tabBar->setToolTip(ApplicationsTab, "<qt>" + i18n( "List of installed applications" ) +
            "</qt>" );

   m_tabs[ComputerTab] = new TQTab;
    m_tabBar->addTab(m_tabs[ComputerTab]);
    m_tabBar->setToolTip(ComputerTab, "<qt>" + i18n( "Information and configuration of your "
                "system, access to personal files, network resources and connected disk drives")
            + "</qt>");
#if 0
    m_tabs[SearchTab] = new TQTab;
    m_tabBar->addTab(m_tabs[SearchTab]);
#endif
    m_tabs[HistoryTab] = new TQTab;
    m_tabBar->addTab(m_tabs[HistoryTab]);
    m_tabBar->setToolTip(HistoryTab, "<qt>" + i18n( "Recently used applications and documents" ) +
            "</qt>" );
    m_tabs[LeaveTab] = new TQTab;
    m_tabBar->addTab(m_tabs[LeaveTab]);
    m_tabBar->setToolTip(LeaveTab, i18n("<qt>Logout, switch user, switch off or reset,"
               " suspend of the system" ) + "</qt>" );

    if (KickerSettings::kickoffTabBarFormat() != KickerSettings::IconOnly) {
	m_tabs[FavoriteTab]->setText(workaroundStringFreeze(i18n("<p align=\"center\"> <u>F</u>avorites</p>")));
	m_tabs[HistoryTab]->setText(workaroundStringFreeze(i18n("<p align=\"center\"><u>H</u>istory</p>")));
	m_tabs[ComputerTab]->setText(
        workaroundStringFreeze(i18n("<p align=\"center\"> <u>C</u>omputer</p>")));
	m_tabs[ApplicationsTab]->setText(workaroundStringFreeze(i18n("<p align=\"center\"><u>A</u>pplications</p>")));
	m_tabs[LeaveTab]->setText(
        workaroundStringFreeze(i18n("<p align=\"center\"><u>L</u>eave</p>")));
    }

    if (KickerSettings::kickoffTabBarFormat() != KickerSettings::LabelOnly) {
	m_tabs[FavoriteTab]->setIconSet(BarIcon("bookmark", tab_icon_size));
	m_tabs[HistoryTab]->setIconSet(BarIcon("recently_used", tab_icon_size));
	m_tabs[ComputerTab]->setIconSet(BarIcon("computer", tab_icon_size));
	m_tabs[ApplicationsTab]->setIconSet(BarIcon("player_playlist", tab_icon_size));
	m_tabs[LeaveTab]->setIconSet(BarIcon("leave", tab_icon_size));
    }

    connect(m_tabBar, TQT_SIGNAL(selected(int)), m_stacker, TQT_SLOT(raiseWidget(int)));
    connect(m_stacker, TQT_SIGNAL(aboutToShow(int)), m_tabBar, TQT_SLOT(setCurrentTab(int)));

    m_favoriteView = new FavoritesItemView (m_stacker, "m_favoriteView");
    m_favoriteView->setAcceptDrops(true);
    m_favoriteView->setItemsMovable(true);
    m_stacker->addWidget(m_favoriteView, FavoriteTab);

    m_recentlyView = new ItemView (m_stacker, "m_recentlyView");
    m_stacker->addWidget(m_recentlyView, HistoryTab);

    m_systemView = new ItemView(m_stacker, "m_systemView");
    m_stacker->addWidget(m_systemView, ComputerTab );

    m_browserView = new FlipScrollView(m_stacker, "m_browserView");
    m_stacker->addWidget(m_browserView, ApplicationsTab);
    connect( m_browserView, TQT_SIGNAL( backButtonClicked() ), TQT_SLOT( slotGoBack() ) );

    m_exitView = new FlipScrollView(m_stacker, "m_exitView");
    m_stacker->addWidget(m_exitView, LeaveTab);
    connect( m_exitView, TQT_SIGNAL( backButtonClicked() ), TQT_SLOT( slotGoExitMainMenu() ) );

    m_searchWidget = new TQVBox (m_stacker, "m_searchWidget");
    m_searchWidget->setSpacing(0);
    m_stacker->addWidget(m_searchWidget, 5);

    // search provider icon
    TQPixmap icon;
    KURIFilterData data;
    TQStringList list;
    data.setData( TQString("some keyword") );
    list << "kurisearchfilter" << "kuriikwsfilter";

    if ( KURIFilter::self()->filterURI(data, list) ) {
      TQString iconPath = locate("cache", KMimeType::favIconForURL(data.uri()) + ".png");
      if ( iconPath.isEmpty() )
        icon = SmallIcon("enhanced_browsing");
      else
        icon = TQPixmap( iconPath );
    }
    else
      icon = SmallIcon("enhanced_browsing");

    m_searchResultsWidget = new ItemView (m_searchWidget, "m_searchResultsWidget");
    m_searchResultsWidget->setItemMargin(4);
//    m_searchResultsWidget->setIconSize(16);
    m_searchActions = new ItemView (m_searchWidget, "m_searchActions");
    m_searchActions->setFocusPolicy(TQ_NoFocus);
    m_searchActions->setItemMargin(4);
    m_searchInternet = new TQListViewItem(m_searchActions, i18n("Search Internet"));
    m_searchInternet->setPixmap(0,icon);
    setTabOrder(m_kcommand, m_searchResultsWidget);

    m_kerryInstalled = !TDEStandardDirs::findExe(TQString::fromLatin1("kerry")).isEmpty();
    m_isShowing = false;

    if (!m_kerryInstalled) {
       m_searchIndex = 0;
       m_searchActions->setMaximumHeight(5+m_searchInternet->height());
    }
    else {
       m_searchIndex = new TQListViewItem(m_searchActions, i18n("Search Index"));
       m_searchIndex->setPixmap(0,SmallIcon("kerry"));
       m_searchActions->setMaximumHeight(5+m_searchIndex->height()*2);
    }
    connect(m_searchActions, TQT_SIGNAL(clicked(TQListViewItem*)), TQT_SLOT(searchActionClicked(TQListViewItem*)));
    connect(m_searchActions, TQT_SIGNAL(returnPressed(TQListViewItem*)), TQT_SLOT(searchActionClicked(TQListViewItem*)));
    connect(m_searchActions, TQT_SIGNAL(spacePressed(TQListViewItem*)), TQT_SLOT(searchActionClicked(TQListViewItem*)));

    connect(m_searchResultsWidget, TQT_SIGNAL(startService(KService::Ptr)), TQT_SLOT(slotStartService(KService::Ptr)));
    connect(m_searchResultsWidget, TQT_SIGNAL(startURL(const TQString&)), TQT_SLOT(slotStartURL(const TQString&)));
    connect(m_searchResultsWidget, TQT_SIGNAL(rightButtonPressed( TQListViewItem*, const TQPoint &, int )), TQT_SLOT(slotContextMenuRequested( TQListViewItem*, const TQPoint &, int )));

    connect(m_recentlyView, TQT_SIGNAL(startService(KService::Ptr)), TQT_SLOT(slotStartService(KService::Ptr)));
    connect(m_recentlyView, TQT_SIGNAL(startURL(const TQString&)), TQT_SLOT(slotStartURL(const TQString&)));
    connect(m_recentlyView, TQT_SIGNAL(rightButtonPressed( TQListViewItem*, const TQPoint &, int  )), TQT_SLOT(slotContextMenuRequested( TQListViewItem*, const TQPoint &, int )));

    connect(m_favoriteView, TQT_SIGNAL(startService(KService::Ptr)), TQT_SLOT(slotStartService(KService::Ptr)));
    connect(m_favoriteView, TQT_SIGNAL(startURL(const TQString&)), TQT_SLOT(slotStartURL(const TQString&)));
    connect(m_favoriteView, TQT_SIGNAL(rightButtonPressed( TQListViewItem*, const TQPoint &, int  )), TQT_SLOT(slotContextMenuRequested( TQListViewItem*, const TQPoint &, int )));
    connect(m_favoriteView, TQT_SIGNAL(moved(TQListViewItem*, TQListViewItem*, TQListViewItem*)), TQT_SLOT(slotFavoritesMoved( TQListViewItem*, TQListViewItem*, TQListViewItem* )));

    connect(m_systemView, TQT_SIGNAL(startURL(const TQString&)), TQT_SLOT(slotStartURL(const TQString&)));
    connect(m_systemView, TQT_SIGNAL(startService(KService::Ptr)), TQT_SLOT(slotStartService(KService::Ptr)));
    connect(m_systemView, TQT_SIGNAL(rightButtonPressed( TQListViewItem*, const TQPoint &, int )), TQT_SLOT(slotContextMenuRequested( TQListViewItem*, const TQPoint &, int )));

    connect(m_browserView, TQT_SIGNAL(startURL(const TQString&)), TQT_SLOT(slotGoSubMenu(const TQString&)));
    connect(m_browserView, TQT_SIGNAL(startService(KService::Ptr)), TQT_SLOT(slotStartService(KService::Ptr)));
    connect(m_browserView, TQT_SIGNAL(rightButtonPressed( TQListViewItem*, const TQPoint &, int )), TQT_SLOT(slotContextMenuRequested( TQListViewItem*, const TQPoint &, int )));

    connect(m_exitView, TQT_SIGNAL(startURL(const TQString&)), TQT_SLOT(slotStartURL(const TQString&)));
    connect(m_exitView, TQT_SIGNAL(rightButtonPressed( TQListViewItem*, const TQPoint &, int )), TQT_SLOT(slotContextMenuRequested( TQListViewItem*, const TQPoint &, int )));

    m_kcommand->setDuplicatesEnabled( false );
    m_kcommand->setLineEdit(new KLineEdit(m_kcommand, "m_kcommand-lineedit"));
    m_kcommand->setCompletionMode( TDEGlobalSettings::CompletionAuto );
    connect(m_kcommand, TQT_SIGNAL(cleared()), TQT_SLOT(clearedHistory()));
    connect(m_kcommand->lineEdit(), TQT_SIGNAL(returnPressed()), TQT_SLOT(searchAccept()));
    connect(m_kcommand->lineEdit(), TQT_SIGNAL(textChanged(const TQString &)), TQT_SLOT(searchChanged(const TQString &)));

    // URI Filter meta object...
    m_filterData = new KURIFilterData();

    max_category_id = new int [num_categories];
    categorised_hit_total = new int [num_categories];

    input_timer = new TQTimer (this, "input_timer");
    connect( input_timer, TQT_SIGNAL(timeout()), this, TQT_SLOT(doQuery()) );

    init_search_timer = new TQTimer (this, "init_search_timer");
    connect( init_search_timer, TQT_SIGNAL(timeout()), this, TQT_SLOT(initSearch()) );
    init_search_timer->start(2000, true);

    connect( m_favoriteView, TQT_SIGNAL( dropped (TQDropEvent *, TQListViewItem * ) ),
             TQT_SLOT( slotFavDropped( TQDropEvent *, TQListViewItem * ) ) );

    this->installEventFilter(this);
    m_tabBar->installEventFilter(this);
    m_favoriteView->installEventFilter(this);
    m_recentlyView->installEventFilter(this);
    m_browserView->leftView()->installEventFilter(this);
    m_browserView->rightView()->installEventFilter(this);
    m_systemView->installEventFilter(this);
    m_exitView->leftView()->installEventFilter(this);
    m_exitView->rightView()->installEventFilter(this);
    m_kcommand->lineEdit()->installEventFilter(this);
    m_searchLabel->installEventFilter(this);
    m_searchPixmap->installEventFilter(this);
    m_stacker->installEventFilter(this);

    emailRegExp = TQRegExp("^([\\w\\-]+\\.)*[\\w\\-]+@([\\w\\-]+\\.)*[\\w\\-]+$");
    authRegExp = TQRegExp("^[a-zA-Z]+://\\w+(:\\w+)?@([\\w\\-]+\\.)*[\\w\\-]+(:\\d+)?(/.*)?$");
    uriRegExp = TQRegExp("^[a-zA-Z]+://([\\w\\-]+\\.)*[\\w\\-]+(:\\d+)?(/.*)?$");
    uri2RegExp = TQRegExp("^([\\w\\-]+\\.)+[\\w\\-]+(:\\d+)?(/.*)?$");

    m_resizeHandle = new TQLabel(this);
    m_resizeHandle->setBackgroundOrigin( TQLabel::ParentOrigin );
    m_resizeHandle->setScaledContents(true);
    m_resizeHandle->setFixedSize( 16, 16 );
    m_searchFrame->stackUnder( m_resizeHandle );
    m_isresizing = false;

    m_searchPixmap->setPixmap( BarIcon( "edit-find", 32 ) );

    TQFont f = font();
    f.setPointSize( kMax( 7, (f.pointSize() * 4 / 5 ) + KickerSettings::kickoffFontPointSizeOffset() ) );
    m_tabBar->setFont ( f );
    f.setPointSize( kMax( 7, (f.pointSize() * 3 / 2 ) + KickerSettings::kickoffFontPointSizeOffset() ) );
    m_searchLabel->setFont( f );

    static_cast<KLineEdit*>(m_kcommand->lineEdit())->setClickMessage(i18n( "Applications, Contacts and Documents" ) );

    bookmarkManager = 0;
    m_addressBook = 0;
    m_popupMenu = 0;

    main_border_tl.load( locate("data", "kicker/pics/main_corner_tl.png" ) );
    main_border_tr.load( locate("data", "kicker/pics/main_corner_tr.png" ) );

    search_tab_left.load( locate("data", "kicker/pics/search-tab-left.png" ) );
    search_tab_right.load( locate("data", "kicker/pics/search-tab-right.png" ) );
    search_tab_center.load( locate("data", "kicker/pics/search-tab-center.png" ) );

    search_tab_top_left.load( locate("data", "kicker/pics/search-tab-top-left.png" ) );
    search_tab_top_right.load( locate("data", "kicker/pics/search-tab-top-right.png" ) );
    search_tab_top_center.load( locate("data", "kicker/pics/search-tab-top-center.png" ) );

#ifdef COMPILE_HALBACKEND
    m_halCtx = NULL;
    m_halCtx = libhal_ctx_new();

    DBusError error;
    dbus_error_init(&error);
    m_dbusConn = dbus_connection_open_private(DBUS_SYSTEM_BUS, &error);
    if (!m_dbusConn) {
        dbus_error_free(&error);
        libhal_ctx_free(m_halCtx);
        m_halCtx = NULL;
    } else {
        dbus_bus_register(m_dbusConn, &error);
        if (dbus_error_is_set(&error)) {
            dbus_error_free(&error);
            libhal_ctx_free(m_halCtx);
            m_dbusConn = NULL;
            m_halCtx = NULL;
        } else {
            libhal_ctx_set_dbus_connection(m_halCtx, m_dbusConn);
            if (!libhal_ctx_init(m_halCtx, &error)) {
                if (dbus_error_is_set(&error)) {
                    dbus_error_free(&error);
                }
                libhal_ctx_free(m_halCtx);
                m_dbusConn = NULL;
                m_halCtx = NULL;
            }
        }
    }
#endif
}

void KMenu::setupUi()
{
    m_stacker = new TQWidgetStack( this, "m_stacker" );
    m_stacker->setGeometry( TQRect( 90, 260, 320, 220 ) );
    m_stacker->setSizePolicy( TQSizePolicy( (TQSizePolicy::SizeType)3, (TQSizePolicy::SizeType)3, 1, 1, m_stacker->sizePolicy().hasHeightForWidth() ) );
    m_stacker->setPaletteBackgroundColor( TQColor( 255, 255, 255 ) );
   // m_stacker->setFocusPolicy( TQ_StrongFocus );
    m_stacker->setLineWidth( 0 );
    m_stacker->setFocusPolicy(TQ_NoFocus);
    connect(m_stacker, TQT_SIGNAL(aboutToShow(TQWidget*)), TQT_SLOT(stackWidgetRaised(TQWidget*)));

    m_kcommand->setName("m_kcommand");
}

KMenu::~KMenu()
{
    saveConfig();

    clearSubmenus();
    delete m_filterData;

#ifdef COMPILE_HALBACKEND
    if (m_halCtx) {
        DBusError error;
        dbus_error_init(&error);
        libhal_ctx_shutdown(m_halCtx, &error);
        libhal_ctx_free(m_halCtx);
    }
#endif
}

bool KMenu::eventFilter ( TQObject * receiver, TQEvent* e)
{
//kdDebug() << "eventFilter receiver=" << receiver->name() << "  type=" << e->type() << endl;
    TQWidget* raiseWidget = 0;
    TQRect raiseRect;

    if (e->type() ==  TQEvent::KeyPress ||
        e->type() == TQEvent::MouseButtonPress ||
        e->type() == TQEvent::MouseMove
        || e->type() == TQEvent::FocusIn
        || e->type() == TQEvent::Wheel) {
        TQPoint p;

        if (e->type() == TQEvent::MouseMove || e->type() == TQEvent::MouseButtonPress) {
            TQMouseEvent* me = TQT_TQMOUSEEVENT(e);
            p = me->globalPos();
        }
        else if (e->type() == TQEvent::Wheel) {
            TQWheelEvent* we = TQT_TQWHEELEVENT(e);
            p = we->globalPos();
        }

        while (receiver) {
            if (TQT_BASE_OBJECT(receiver) == TQT_BASE_OBJECT(m_tabBar) && (e->type()!=TQEvent::MouseMove || KickerSettings::kickoffSwitchTabsOnHover() ) ) {
                TQTab* s = m_tabBar->selectTab(m_tabBar->mapFromGlobal(p));
                if (s && s->identifier() == ApplicationsTab)
                    raiseWidget = m_browserView;
                if (s && s->identifier() == FavoriteTab)
                    raiseWidget = m_favoriteView;
                if (s && s->identifier() == HistoryTab)
                    raiseWidget = m_recentlyView;
                if (s && s->identifier() == ComputerTab)
                    raiseWidget = m_systemView;
                if (s && s->identifier() == LeaveTab)
                    raiseWidget = m_exitView;

                if (raiseWidget)
                    raiseRect = TQRect( m_tabBar->mapToGlobal(s->rect().topLeft()),
                                s->rect().size());
            }

            /* we do not want hover activation for the search line edit as this can be
             * pretty disturbing */
            if ( (TQT_BASE_OBJECT(receiver) == TQT_BASE_OBJECT(m_searchPixmap) ||
                 ( ( TQT_BASE_OBJECT(receiver) == TQT_BASE_OBJECT(m_searchLabel) || TQT_BASE_OBJECT(receiver)==TQT_BASE_OBJECT(m_kcommand->lineEdit()) ) &&
                 ( e->type() == TQEvent::KeyPress || e->type() == TQEvent::Wheel
                   || e->type() == TQEvent::MouseButtonPress ) ) ) &&
                    !m_isShowing) {
                raiseWidget = m_searchWidget;
                raiseRect = TQRect( m_searchFrame->mapToGlobal(m_searchFrame->rect().topLeft()),
                        m_searchFrame->size());
            }

            if(raiseWidget)
                break;
            if(receiver->isWidgetType())
                receiver = TQT_TQOBJECT(TQT_TQWIDGET(receiver)->parentWidget(true));
            else
                break;
        }

        if (e->type() == TQEvent::FocusIn && receiver && raiseWidget) {
            m_searchResultsWidget->setFocusPolicy(TQ_StrongFocus);
            m_searchActions->setFocusPolicy(raiseWidget == m_searchWidget ?
                    TQ_StrongFocus : TQ_NoFocus);
            setTabOrder(raiseWidget, m_searchResultsWidget);
            if (raiseWidget != m_stacker->visibleWidget()
                && TQT_TQWIDGET(receiver)->focusPolicy() == TQ_NoFocus
                && m_stacker->id(raiseWidget) >= 0) {

                m_stacker->raiseWidget(raiseWidget);
                return true;
            }

            if (raiseWidget->focusPolicy() != TQ_NoFocus)
                return false;
        }

	if (m_sloppyRegion.contains(p)) {
            if (e->type() ==  TQEvent::MouseButtonPress /*&& m_sloppyTimer.isActive()*/)
                m_sloppySourceClicked = true;

            if (!m_sloppyTimer.isActive() || m_sloppySource != raiseRect) {
                int timeout= style().styleHint(TQStyle::SH_PopupMenu_SubMenuPopupDelay);
                if (m_sloppySourceClicked)
                    timeout = 3000;
                m_sloppyTimer.start(timeout);
            }

	    m_sloppyWidget = raiseWidget;
	    m_sloppySource = raiseRect;
	    return false;
	}
    }

    if(e->type() == TQEvent::Enter && receiver->isWidgetType()) {
	TQT_TQWIDGET(receiver)->setMouseTracking(true);
        TQToolTip::hide();
    }

    if ( ( e->type() == TQEvent::DragEnter || e->type() == TQEvent::DragMove ) &&
         raiseWidget == m_favoriteView )
    {
        m_stacker->raiseWidget(m_favoriteView);

	return false;
    }

    // This is a nightmare of a hack, look away. Logic needs
    // to be moved to the stacker and all widgets in the stacker
    // must have focusNextPrevChild() overwritten to do nothing
    if (e->type() == TQEvent::KeyPress && !raiseRect.isNull()) {
        ItemView* view;
        if (m_browserView==m_stacker->visibleWidget())
            view = m_browserView->currentView();
        else if (m_exitView==m_stacker->visibleWidget())
            view = m_exitView->currentView();
        else
            view = dynamic_cast<ItemView*>(m_stacker->visibleWidget());

        if (view)
        {
            bool handled = true;
            switch (TQT_TQKEYEVENT(e)->key()) {
                case Key_Up:
                    if (view->selectedItem()) {
                        view->setSelected(view->selectedItem()->itemAbove(),true);
                    }
                    else {
                        view->setSelected(view->lastItem(),true);
                    }
                    break;
                case Key_Down:
                    if (view->selectedItem()) {
                        view->setSelected(view->selectedItem()->itemBelow(),true);
                    }
                    else {
                        if (view->firstChild() && view->firstChild()->isSelectable())
                           view->setSelected(view->firstChild(),true);
                        else if (view->childCount()>2)
                           view->setSelected(view->firstChild()->itemBelow(),true);
                    }
                    break;
                case Key_Right:
                    if (view->selectedItem() && !static_cast<KMenuItem*>(view->selectedItem())->hasChildren())
                        break;
                    // nobreak
                case Key_Enter:
                case Key_Return:
                    if (view->selectedItem())
                        view->slotItemClicked(view->selectedItem());

                    break;
                case Key_Left:
                    if (m_browserView == m_stacker->visibleWidget() || m_exitView == m_stacker->visibleWidget()) {
                       FlipScrollView* flip = dynamic_cast<FlipScrollView*>(m_stacker->visibleWidget());
                       if (flip->showsBackButton()) {
                          if (m_browserView == m_stacker->visibleWidget())
                             goSubMenu( m_browserView->currentView()->backPath(), true );
                          else
                             view->slotItemClicked(view->firstChild());
                       }
                       break;
                    }
                    // nobreak
                case Key_Backspace:
                    if (m_browserView == m_stacker->visibleWidget() || m_exitView == m_stacker->visibleWidget()) {
                       FlipScrollView* flip = dynamic_cast<FlipScrollView*>(m_stacker->visibleWidget());
                       if (flip->showsBackButton()) {
                          if (m_browserView == m_stacker->visibleWidget())
                             goSubMenu( m_browserView->currentView()->backPath(), true );
                          else
                             view->slotItemClicked(view->firstChild());
                       }
                    }

                    break;
                default:
                    handled = false;
            }

            if (handled)
                view->ensureItemVisible(view->selectedItem());

            return handled;
        }
    }

    bool r = KMenuBase::eventFilter(receiver, e);

    if (!r && raiseWidget)
        m_stacker->raiseWidget(raiseWidget);

    if (e->type() == TQEvent::Wheel && raiseWidget )
    {
        // due to an ugly TQt bug we have to kill wheel events
        // that cause focus switches
        r = true;
    }

    if (e->type() == TQEvent::Enter && TQT_BASE_OBJECT(receiver) == TQT_BASE_OBJECT(m_stacker))
    {
        TQRect r(m_stacker->mapToGlobal(TQPoint(-8,-32)), m_stacker->size());
        r.setSize(r.size()+TQSize(16,128));

        m_sloppyRegion = TQRegion(r);
    }

    // redo the sloppy region
    if (e->type() == TQEvent::MouseMove && !r && raiseWidget)
    {
        TQPointArray points(4);

        // hmm, eventually this should be mouse position + 10px, not
        // just worst case. but worst case seems to work fine enough.
        TQPoint edge(raiseRect.topLeft());
        edge.setX(edge.x()+raiseRect.center().x());

        if (m_orientation == BottomUp)
        {
            points.setPoint(0, m_stacker->mapToGlobal(m_stacker->rect().bottomLeft()));
            points.setPoint(1, m_stacker->mapToGlobal(m_stacker->rect().bottomRight()));

            edge.setY(edge.y()+raiseRect.height());
            points.setPoint(2, edge+TQPoint(+raiseRect.width()/4,0));
            points.setPoint(3, edge+TQPoint(-raiseRect.width()/4,0));
        }
        else
        {
            points.setPoint(0, m_stacker->mapToGlobal(m_stacker->rect().topLeft()));
            points.setPoint(1, m_stacker->mapToGlobal(m_stacker->rect().topRight()));
            points.setPoint(2, edge+TQPoint(-raiseRect.width()/4,0));
            points.setPoint(3, edge+TQPoint(+raiseRect.width()/4,0));
        }

        m_sloppyRegion = TQRegion(points);
    }

    return r;
}

void KMenu::slotSloppyTimeout()
{
    if (m_sloppyRegion.contains(TQCursor::pos()) && !m_sloppySource.isNull())
    {
        if ( m_sloppySource.contains(TQCursor::pos()))
        {
            m_stacker->raiseWidget(m_sloppyWidget);

            m_sloppyWidget = 0;
            m_sloppySource = TQRect();
            m_sloppyRegion = TQRegion();
            m_sloppySourceClicked = false;
        }
    }
    m_sloppyTimer.stop();
}

void KMenu::paintSearchTab( bool active )
{
    TQPixmap canvas( m_searchFrame->size() );
    TQPainter p( &canvas );

    TQPixmap pix;

    if ( m_orientation == BottomUp )
        pix.load( locate("data", "kicker/pics/search-gradient.png" ) );
    else
        pix.load( locate("data", "kicker/pics/search-gradient-topdown.png" ) );

    pix.convertFromImage( pix.convertToImage().scale(pix.width(), m_searchFrame->height()));
    p.drawTiledPixmap( 0, 0, m_searchFrame->width(), m_searchFrame->height(), pix );

    if ( active ) {

        m_tabBar->deactivateTabs(true);

        p.setBrush( Qt::white );
        p.setPen( Qt::NoPen );

        if ( m_orientation == BottomUp ) {
            search_tab_center.convertFromImage( search_tab_center.convertToImage().scale(search_tab_center.width(), m_searchFrame->height()));
            p.drawTiledPixmap( search_tab_left.width(), 0, m_searchFrame->width()-search_tab_left.width()-search_tab_right.width(), m_searchFrame->height(), search_tab_center );

            search_tab_left.convertFromImage( search_tab_left.convertToImage().scale(search_tab_left.width(), m_searchFrame->height()));
            p.drawPixmap( 0, 0, search_tab_left );

            search_tab_right.convertFromImage( search_tab_right.convertToImage().scale(search_tab_right.width(), m_searchFrame->height()));
            p.drawPixmap( m_searchFrame->width()-search_tab_right.width(), 0, search_tab_right );
        }
        else {
            search_tab_top_center.convertFromImage( search_tab_top_center.convertToImage().scale(search_tab_top_center.width(), m_searchFrame->height()));
            p.drawTiledPixmap( search_tab_top_left.width(), 0, m_searchFrame->width()-search_tab_top_left.width()-search_tab_top_right.width(), m_searchFrame->height(), search_tab_top_center );

            search_tab_top_left.convertFromImage( search_tab_top_left.convertToImage().scale(search_tab_top_left.width(), m_searchFrame->height()));
            p.drawPixmap( 0, 0, search_tab_top_left );

            search_tab_top_right.convertFromImage( search_tab_top_right.convertToImage().scale(search_tab_top_right.width(), m_searchFrame->height()));
            p.drawPixmap( m_searchFrame->width()-search_tab_top_right.width(), 0, search_tab_top_right );
         }
    }
    else
        m_tabBar->deactivateTabs(false);

    p.end();
    m_searchFrame->setPaletteBackgroundPixmap( canvas );
}

void KMenu::stackWidgetRaised(TQWidget* raiseWidget)
{
    paintSearchTab(raiseWidget == m_searchWidget);

    if (raiseWidget == m_browserView) {
        if ( m_tabBar->currentTab() == ApplicationsTab)
            slotGoSubMenu(TQString());
        if (m_browserDirty ) {
          createNewProgramList();
          m_browserView->prepareRightMove();
          m_browserView->currentView()->clear();
          fillSubMenu(TQString(), m_browserView->currentView());
          m_browserDirty = false;
        }
    }
    else if (raiseWidget == m_recentlyView) {
        if (m_recentDirty)
            updateRecent();
    }
    else if (raiseWidget == m_exitView) {
        if (m_tabBar->currentTab() == LeaveTab)
            slotGoExitMainMenu();
    }


#warning TQtab fixme
#if 0
    else if (raiseWidget == m_systemView)
	frame = m_system;
    else if (raiseWidget == m_favoriteView)
	frame = m_btnFavorites;
    if (!frame)
      return;

    if ( m_activeTab == frame )
        return;

    paintTab( m_activeTab, false );
    paintTab( frame, true );

   // if (dynamic_cast<TQScrollView*>(raiseWidget))
   //     m_activeTab->setFocusProxy(static_cast<TQScrollView*>(raiseWidget)->viewport());

    if (0 && /*raiseWidget == m_stacker->visibleWidget() &&*/ !raiseWidget->hasFocus()) {

        if (dynamic_cast<TQScrollView*>(raiseWidget))
            static_cast<TQScrollView*>(raiseWidget)->viewport()->setFocus();
        else
            raiseWidget->setFocus();
    }

    m_activeTab = frame;

    m_sloppyRegion = TQRegion();
    m_sloppyTimer.stop();

    ItemView* view;
    if (raiseWidget == m_browserView)
        view = m_browserView->currentView();
    else if (raiseWidget == m_exitView)
        view = m_exitView->currentView();
    else
        view = dynamic_cast<ItemView*>(m_stacker->visibleWidget());
    if (view && !view->selectedItem()) {
	if (view->firstChild() && view->firstChild()->isSelectable()) {
     	    view->setSelected(view->firstChild(),true);
        }
        else if (view->childCount()>1) {
       	    view->setSelected(view->firstChild()->itemBelow(),true);
        }
    }
#endif
}

void KMenu::paletteChanged()
{
}

void KMenu::tabClicked(TQTab* t)
{
    if (t==m_tabs[ApplicationsTab])
	slotGoSubMenu(TQString());
    else if (t==m_tabs[LeaveTab])
	slotGoExitMainMenu();
}

void KMenu::slotGoBack()
{
    goSubMenu( m_browserView->currentView()->backPath() );
}

void KMenu::slotGoExitMainMenu()
{
    if (m_exitView->currentView()==m_exitView->rightView()) {
      m_exitView->prepareLeftMove(false);
      m_exitView->showBackButton(false);
      m_exitView->flipScroll(TQString());
    }
}

void KMenu::slotGoExitSubMenu(const TQString& url)
{
    m_exitView->prepareRightMove();
    m_exitView->showBackButton(true);

    int nId = serviceMenuEndId() + 1;
    int index = 1;

    if (url=="kicker:/restart/") {
      TQStringList rebootOptions;
      int def, cur;
      if ( DM().bootOptions( rebootOptions, def, cur ) )
      {
        if ( cur == -1 )
            cur = def;

        int boot_index = 0;
        TQStringList::ConstIterator it = rebootOptions.begin();
        for (; it != rebootOptions.end(); ++it, ++boot_index)
        {

            TQString option = i18n( "Start '%1'" ).arg( *it );
            if (boot_index == cur)
                option = i18n("Start '%1' (current)").arg( *it );
            m_exitView->rightView()->insertItem( "reload", option,
                    i18n( "Restart and boot directly into '%1'").arg( *it ),
                    TQString( "kicker:/restart_%1" ).arg( boot_index ), nId++, index++ );
        }
        m_exitView->rightView()->insertHeader( nId++, "kicker:/restart/" );
      }
    }
    else /*if (url=="kicker:/switchuser/") */{
        m_exitView->rightView()->insertItem( "switchuser", i18n( "Start New Session" ),
                     i18n( "Start a parallel session" ), "kicker:/switchuser", nId++, index++ );

        m_exitView->rightView()->insertItem( "system-lock-screen", i18n( "Lock Current && Start New Session").replace("&&","&"),
                     i18n( "Lock screen and start a parallel session" ), "kicker:/switchuserafterlock", nId++, index++ );

       SessList sess;
       if (DM().localSessions( sess )) {
          if (sess.count()>1)
              m_exitView->rightView()->insertSeparator( nId++, TQString(), index++ );
          for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
              if ((*it).vt && !(*it).self) {
                  TQString user, loc;
                  DM().sess2Str2( *it, user, loc );
                  TQStringList list = TQStringList::split(":", user);
                  m_exitView->rightView()->insertItem( "switchuser", i18n( "Switch to Session of User '%1'").arg(list[0]),
                     i18n("Session: %1").arg(list[1].mid(1)+", "+loc) , TQString("kicker:/switchuser_%1").arg((*it).vt), nId++, index++ );
              }
          }
        }

        m_exitView->rightView()->insertHeader( nId++, "kicker:/switchuser/" );
    }
    m_exitView->flipScroll(TQString());
}

void KMenu::slotGoSubMenu(const TQString& relPath)
{
     goSubMenu(relPath);
}

void KMenu::goSubMenu(const TQString& relPath, bool keyboard)
{
    if ( relPath.startsWith( "kicker:/goup/" ) )
    {
        TQString rel = relPath.mid( strlen( "kicker:/goup/" ) );
        int index = rel.length() - 1;
        if ( rel.endsWith( "/" ) )
            index--;
        index = rel.findRev( '/', index );
        kdDebug() << "goup, rel '" << rel << "' " << index << endl;
        TQString currel = rel;
        rel = rel.left( index + 1 );
        if ( rel == "/" )
            rel = TQString();

        kdDebug() << "goup, rel '" << rel << "' " << rel.isEmpty() << endl;
        fillSubMenu( rel, m_browserView->prepareLeftMove() );
        m_browserView->flipScroll(keyboard ? currel : TQString());
        return;
    } else if (relPath.isEmpty())
    {
	if (m_browserView->currentView()->path.isEmpty())
	    return;
	fillSubMenu( relPath, m_browserView->prepareLeftMove() );
    } else if ( relPath.startsWith( "kicker:/new/" ) )
    {
        ItemView* view = m_browserView->prepareRightMove();
        m_browserView->showBackButton( true );

        int nId = serviceMenuEndId() + 1;
        view->insertHeader( nId++, "new/" );
        int index = 2;
        for (TQStringList::ConstIterator it = m_newInstalledPrograms.begin();
            it != m_newInstalledPrograms.end(); ++it) {
            KService::Ptr p = KService::serviceByStorageId((*it));
            view->insertMenuItem(p, nId++, index++);
         }
    } else
    {
        //m_browserView->clear();
        fillSubMenu(relPath, m_browserView->prepareRightMove());
    }
    m_browserView->flipScroll(keyboard ? "kicker:/goup/": TQString());
}

void KMenu::fillSubMenu(const TQString& relPath, ItemView *view)
{
    kdDebug() << "fillSubMenu() " << relPath << endl;
    KServiceGroup::Ptr root = KServiceGroup::group(relPath);
    Q_ASSERT( root );

    KServiceGroup::List list = root->entries(true, true, true, KickerSettings::
            menuEntryFormat() == KickerSettings::DescriptionAndName || KickerSettings::menuEntryFormat()
            == KickerSettings::DescriptionOnly);

    int nId = serviceMenuStartId();
    m_browserView->showBackButton( !relPath.isEmpty() );
    if ( !relPath.isEmpty() )
    {
        view->insertHeader( nId++, relPath );
    }
    else if ( m_newInstalledPrograms.count() ) {
        KMenuItem *item = view->insertItem( "clock", i18n( "New Applications" ),
                              TQString(), "kicker:/new/", nId++, -1 );
        item->setHasChildren( true );
        view->insertSeparator( nId++, TQString(), -1 );
    }

    view->path = relPath;

    fillMenu (root, list, relPath, view, nId);
}

void KMenu::fillMenu(KServiceGroup::Ptr&
#ifdef KDELIBS_SUSE
    _root
#endif
                     , KServiceGroup::List& _list,
                     const TQString& _relPath,
                     ItemView* view,
                     int& id)
{
    bool separatorNeeded = false;
    KServiceGroup::List::ConstIterator it = _list.begin();
#ifdef KDELIBS_SUSE
    KSortableValueList<TDESharedPtr<KSycocaEntry>,TQCString> slist;
    KSortableValueList<TDESharedPtr<KSycocaEntry>,TQCString> glist;
    TQMap<TQString,TQString> specialTitle;
    TQMap<TQString,TQString> categoryIcon;
    TQMap<TQString,TQString> shortenedMenuPath;
#endif

    for (; it != _list.end(); ++it)
    {
        KSycocaEntry * e = *it;

        if (e->isType(KST_KServiceGroup))
        {
           KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
#ifdef KDELIBS_SUSE
           if ( true /*KickerSettings::reduceMenuDepth()*/ && g->SuSEshortMenu() ){
              KServiceGroup::List l = g->entries(true, true /*excludeNoDisplay_*/ );
              if ( l.count() == 1 ) {
                 // the special case, we want to short the menu.
                 // TOFIX? : this works only for one level
                 KServiceGroup::List::ConstIterator _it=l.begin();
                 KSycocaEntry *_e = *_it;
                 if (_e->isType(KST_KService)) {
                     KService::Ptr s(static_cast<KService *>(_e));
		     TQString key;
                     if ( g->SuSEgeneralDescription() ) {
			// we use the application name
                        key = s->name();
                     }
		     else {
			// we use the normal menu description
			key = s->name();
                        if( !s->genericName().isEmpty() && g->caption()!=s->genericName()) {
                           if (KickerSettings::menuEntryFormat() == KickerSettings::NameAndDescription)
                               key = s->name() + " (" + g->caption() + ")";
			   else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName)
                               key = g->caption() + " (" + s->name() + ")";
			   else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly)
                             key = g->caption();
                        }
		     }
		     specialTitle.insert( _e->name(), key );
		     categoryIcon.insert( _e->name(), g->icon() );
                     slist.insert( key.local8Bit(), _e );
		     shortenedMenuPath.insert( _e->name(), g->relPath() );
                     // and escape from here
                     continue;
                  }
               }
            }
            glist.insert( g->caption().local8Bit(), e );
        }else if( e->isType(KST_KService)) {
            KService::Ptr s(static_cast<KService *>(e));
            slist.insert( s->name().local8Bit(), e );
        } else
            slist.insert( e->name().local8Bit(), e );
    }

    _list = _root->SuSEsortEntries( slist, glist, true /*excludeNoDisplay_*/, true );
    it = _list.begin();

    for (; it != _list.end(); ++it) {

        KSycocaEntry * e = *it;

        if (e->isType(KST_KServiceGroup)) {

           KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
           if ( true /*KickerSettings::reduceMenuDepth()*/ && g->SuSEshortMenu() ){
              KServiceGroup::List l = g->entries(true, true /*excludeNoDisplay_*/ );
              if ( l.count() == 1 ) {
                    continue;
              }
           }
           // standard sub menu
#endif
            TQString groupCaption = g->caption();

           // Avoid adding empty groups.
            KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(g->relPath());

            int nbChildCount = subMenuRoot->childCount();
            if (nbChildCount == 0 && !g->showEmptyMenu())
            {
                continue;
            }

            bool is_description = KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName ||
                                  KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly;

            TQString inlineHeaderName = g->showInlineHeader() ? groupCaption : "";

            if ( nbChildCount == 1 && g->allowInline() && g->inlineAlias())
            {
                KServiceGroup::Ptr element = KServiceGroup::group(g->relPath());
                if ( element )
                {
                    //just one element

                    KServiceGroup::List listElement = element->entries(true, true, true, is_description );
                    KSycocaEntry * e1 = *( listElement.begin() );
                    if ( e1->isType( KST_KService ) )
                    {
                        KService::Ptr s(static_cast<KService *>(e1));
                        view->insertMenuItem(s, id++, -1, 0);
                        continue;
                    }
                }
            }

            if (g->allowInline() && ((nbChildCount <= g->inlineValue() ) ||   (g->inlineValue() == 0)))
            {
                //inline all entries
                KServiceGroup::Ptr rootElement = KServiceGroup::group(g->relPath());

                if (!rootElement || !rootElement->isValid())
                {
                    break;
                }


                KServiceGroup::List listElement = rootElement->entries(true, true, true, is_description );

#if 0
                if ( !g->inlineAlias() && !inlineHeaderName.isEmpty() )
                {
                    int mid = view->insertItem(new PopupMenuTitle(inlineHeaderName, font()), id++, id, 0);
                    m_browserView->setItemEnabled( mid, false );
                }
#endif

                fillMenu( rootElement, listElement, g->relPath(), 0, id );
                continue;
            }

            // Ignore dotfiles.
            if ((g->name().at(0) == '.'))
            {
                continue;
            }

            KMenuItem *item = view->insertItem(g->icon(), groupCaption, TQString(), g->relPath(), id++, -1);
	    item->setMenuPath(g->relPath());
            item->setHasChildren( true );

#warning FIXME
#if 0
            PanelServiceMenu * m =
                newSubMenu(g->name(), g->relPath(), this, g->name().utf8(), inlineHeaderName);
            m->setCaption(groupCaption);

            TQIconSet iconset = KickerLib::menuIconSet(g->icon());

            if (separatorNeeded)
            {
                insertSeparator();
                separatorNeeded = false;
            }

            int newId = insertItem(iconset, groupCaption, m, id++);
            entryMap_.insert(newId, static_cast<KSycocaEntry*>(g));
            // We have to delete the sub menu our selves! (See TQt docs.)
            subMenus.append(m);
#endif
        }
        if (e->isType(KST_KService))
        {
            KService::Ptr s(static_cast<KService *>(e));
            if (_relPath.isEmpty()) {
                TQStringList favs = KickerSettings::favorites();
                if (favs.find(s->storageId())!=favs.end())
                  continue;
            }
#ifdef KDELIBS_SUSE
            KMenuItem *item = view->insertMenuItem(s, id++, -1, 0, TQString(), specialTitle[s->name()], categoryIcon[s->name()] );
            if (shortenedMenuPath[s->name()].isEmpty())
	       item->setMenuPath(_relPath+s->menuId());
            else
	       item->setMenuPath(shortenedMenuPath[s->name()]+s->menuId());
#else
            KMenuItem *item = view->insertMenuItem(s, id++, -1);
	    item->setMenuPath(_relPath+s->menuId());
#endif
        }
        else if (e->isType(KST_KServiceSeparator))
        {
            separatorNeeded = true;
        }
    }

    view->slotMoveContent();
}

void KMenu::initialize()
{
    static bool m_initialized=false;
    if (m_initialized)
        return;
    m_initialized = true;

    kdDebug(1210) << "KMenu::initialize()" << endl;

    // in case we've been through here before, let's disconnect
    disconnect(kapp, TQT_SIGNAL(tdedisplayPaletteChanged()),
            this, TQT_SLOT(paletteChanged()));
    connect(kapp, TQT_SIGNAL(tdedisplayPaletteChanged()),
            this, TQT_SLOT(paletteChanged()));

   /*
       If  the user configured ksmserver to
     */
    TDEConfig ksmserver("ksmserverrc", false, false);
    ksmserver.setGroup("General");
    connect( m_branding, TQT_SIGNAL(clicked()), TQT_SLOT(slotOpenHomepage()));
    m_tabBar->setTabEnabled(LeaveTab, kapp->authorize("logout"));

    // load search field history
    TQStringList histList = KickerSettings::history();
    int maxHistory = KickerSettings::historyLength();

    bool block = m_kcommand->signalsBlocked();
    m_kcommand->blockSignals( true );
    m_kcommand->setMaxCount( maxHistory );
    m_kcommand->setHistoryItems( histList );
    m_kcommand->blockSignals( block );

    TQStringList compList = KickerSettings::completionItems();
    if( compList.isEmpty() )
        m_kcommand->completionObject()->setItems( histList );
    else
        m_kcommand->completionObject()->setItems( compList );

    TDECompletionBox* box = m_kcommand->completionBox();
    if (box)
        box->setActivateOnSelect( false );

    m_finalFilters = KURIFilter::self()->pluginNames();
    m_finalFilters.remove("kuriikwsfilter");

    m_middleFilters = m_finalFilters;
    m_middleFilters.remove("localdomainurifilter");

    TQStringList favs = KickerSettings::favorites();
    if (favs.isEmpty()) {
      TQFile f(locate("data", "kicker/default-favs"));
      if (f.open(IO_ReadOnly)) {
        TQTextStream is(&f);

        while (!is.eof())
            favs << is.readLine();

        f.close();
      }
      KickerSettings::setFavorites(favs);
      KickerSettings::writeConfig();
    }

    int nId = serviceMenuEndId() + 1;
    int index = 1;
    for (TQStringList::ConstIterator it = favs.begin(); it != favs.end(); ++it)
    {
       if ((*it)[0]=='/') {
          KDesktopFile df((*it),true);
          TQString url = df.readURL();
          if (!KURL(url).isLocalFile() || TQFile::exists(url.replace("file://",TQString())))
            m_favoriteView->insertItem(df.readIcon(),df.readName(),df.readGenericName(), url, nId++, index++);
       }
       else {
          KService::Ptr p = KService::serviceByStorageId((*it));
          m_favoriteView->insertMenuItem(p, nId++, index++);
       }
    }

    //nId = m_favoriteView->insertSeparator( nId, TQString(), index++ );
//    m_favoriteView->insertDocument(KURL("help:/khelpcenter/userguide/index.html"), nId++);

    insertStaticItems();

    m_stacker->raiseWidget (m_favoriteView);
}

void KMenu::insertStaticExitItems()
{
    int nId = serviceMenuEndId() + 1;
    int index = 1;

    m_exitView->leftView()->insertSeparator( nId++, i18n("Session"), index++ );
    if (kapp->authorize("logout"))
       m_exitView->leftView()->insertItem( "edit-undo", i18n( "Logout" ),
                                   i18n( "End session" ), "kicker:/logout", nId++, index++ );
    if (kapp->authorize("lock_screen"))
       m_exitView->leftView()->insertItem( "system-lock-screen", i18n( "Lock" ),
                                   i18n( "Lock screen" ), "kicker:/lock", nId++, index++ );

    TDEConfig ksmserver("ksmserverrc", false, false);
    ksmserver.setGroup("General");
    if (ksmserver.readEntry( "loginMode" ) == "restoreSavedSession")
    {
        m_exitView->leftView()->insertItem("document-save", i18n("Save Session"),
                               i18n("Save current Session for next login"),
                               "kicker:/savesession", nId++, index++ );
    }
    if (DM().isSwitchable() && kapp->authorize("switch_user"))
    {
        KMenuItem *switchuser = m_exitView->leftView()->insertItem( "switchuser", i18n( "Switch User" ),
                                                                    i18n( "Manage parallel sessions" ), "kicker:/switchuser/", nId++, index++ );
        switchuser->setHasChildren(true);
    }

    bool maysd = false;
#if defined(COMPILE_HALBACKEND)
    if (ksmserver.readBoolEntry( "offerShutdown", true ) && DM().canShutdown())
        maysd = true;
#elif defined(__TDE_HAVE_TDEHWLIB)
    TDERootSystemDevice* rootDevice = TDEGlobal::hardwareDevices()->rootSystemDevice();
    if( rootDevice ) {
        maysd = rootDevice->canPowerOff();
    }
#endif

    if ( maysd )
    {
        m_exitView->leftView()->insertSeparator( nId++, i18n("System"), index++ );
        m_exitView->leftView()->insertItem( "system-log-out", i18n( "Shutdown Computer" ),
                                   i18n( "Turn off computer" ), "kicker:/shutdown", nId++, index++ );

        m_exitView->leftView()->insertItem( "reload", i18n( "&Restart Computer" ).replace("&",""),
                                            i18n( "Restart and boot the default system" ),
                                            "kicker:/restart", nId++, index++ );

        insertSuspendOption(nId, index);

        int def, cur;
        TQStringList dummy_opts;
        if ( DM().bootOptions( dummy_opts, def, cur ) )
        {

            KMenuItem *restart = m_exitView->leftView()->insertItem( "reload", i18n( "Start Operating System" ),
                                                                     i18n( "Restart and boot another operating system" ),
                                                                     "kicker:/restart/", nId++, index++ );
            restart->setHasChildren(true);
        }
    }
}

void KMenu::insertStaticItems()
{
    insertStaticExitItems();

    int nId = serviceMenuEndId() + 10;
    int index = 1;

    m_systemView->insertSeparator( nId++, i18n("Applications"), index++);

#ifdef KICKOFF_DIST_CONFIG_SHORTCUT1
    KService::Ptr kdcs1 = KService::serviceByStorageId(KICKOFF_DIST_CONFIG_SHORTCUT1);
    m_systemView->insertMenuItem(kdcs1, nId++, index++);
#endif
#ifdef KICKOFF_DIST_CONFIG_SHORTCUT2
    KService::Ptr kdcs2 = KService::serviceByStorageId(KICKOFF_DIST_CONFIG_SHORTCUT2);
    m_systemView->insertMenuItem(kdcs2, nId++, index++);
#endif

    KService::Ptr p = KService::serviceByStorageId("KControl.desktop");
    m_systemView->insertMenuItem(p, nId++, index++);

    // run command
    if (kapp->authorize("run_command"))
    {
        m_systemView->insertItem( "system-run", i18n("Run Command..."),
                   "", "kicker:/runusercommand", nId++, index++ );
    }

    m_systemView->insertSeparator( nId++, i18n("System Folders"), index++ );

    m_systemView->insertItem( "folder_home", i18n( "Home Folder" ),
                              TQDir::homeDirPath(), "file://"+TQDir::homeDirPath(), nId++, index++ );

    if ( TDEStandardDirs::exists( TDEGlobalSettings::documentPath() + "/" ) )
    {
        TQString documentPath = TDEGlobalSettings::documentPath();
        if ( documentPath.endsWith( "/" ) )
            documentPath = documentPath.left( documentPath.length() - 1 );
        if (documentPath!=TQDir::homeDirPath())
           m_systemView->insertItem( "folder_man", i18n( "My Documents" ), documentPath, documentPath, nId++, index++ );
    }

    if ( TDEStandardDirs::exists( TDEGlobalSettings::picturesPath() + "/" ) )
    {
        TQString picturesPath = TDEGlobalSettings::picturesPath();
        if ( picturesPath.endsWith( "/" ) )
            picturesPath = picturesPath.left( picturesPath.length() - 1 );
        if (picturesPath!=TQDir::homeDirPath())
           m_systemView->insertItem( "folder_image", i18n( "My Images" ), picturesPath, picturesPath, nId++, index++ );
   }

    if ( TDEStandardDirs::exists( TDEGlobalSettings::musicPath() + "/" ) )
    {
        TQString musicPath = TDEGlobalSettings::musicPath();
        if ( musicPath.endsWith( "/" ) )
            musicPath = musicPath.left( musicPath.length() - 1 );
        if (musicPath!=TQDir::homeDirPath())
           m_systemView->insertItem( "folder_sound", i18n( "My Music" ), musicPath, musicPath, nId++, index++ );
    }

    if ( TDEStandardDirs::exists( TDEGlobalSettings::videosPath() + "/" ) )
    {
        TQString videosPath = TDEGlobalSettings::videosPath();
        if ( videosPath.endsWith( "/" ) )
            videosPath = videosPath.left( videosPath.length() - 1 );
        if (videosPath!=TQDir::homeDirPath())
           m_systemView->insertItem( "folder_video", i18n( "My Videos" ), videosPath, videosPath, nId++, index++ );
    }

    if ( TDEStandardDirs::exists( TDEGlobalSettings::downloadPath() + "/" ) )
    {
        TQString downloadPath = TDEGlobalSettings::downloadPath();
        if ( downloadPath.endsWith( "/" ) )
            downloadPath = downloadPath.left( downloadPath.length() - 1 );
        if (downloadPath!=TQDir::homeDirPath())
           m_systemView->insertItem( "folder_inbox", i18n( "My Downloads" ), downloadPath, downloadPath, nId++, index++ );
    }

    m_systemView->insertItem( "network", i18n( "Network Folders" ),
                              "remote:/", "remote:/", nId++, index++ );

    m_mediaWatcher = new MediaWatcher( TQT_TQOBJECT(this) );
    connect( m_mediaWatcher, TQT_SIGNAL( mediumChanged() ), TQT_SLOT( updateMedia() ) );
    m_media_id = 0;

    connect(&m_mediaFreeTimer, TQT_SIGNAL(timeout()), TQT_SLOT( updateMedia()));
}

int KMenu::insertClientMenu(KickerClientMenu *)
{
#if 0
    int id = client_id;
    clients.insert(id, p);
    return id;
#endif
    return 0;
}

void KMenu::removeClientMenu(int)
{
#if 0
    clients.remove(id);
    slotClear();
#endif
}

extern int kicker_screen_number;

void KMenu::slotLock()
{
    kdDebug() << "slotLock " << endl;
    accept();
    TQCString appname( "kdesktop" );
    if ( kicker_screen_number )
        appname.sprintf("kdesktop-screen-%d", kicker_screen_number);
    kapp->dcopClient()->send(appname, "KScreensaverIface", "lock()", TQString(""));
}

void KMenu::slotOpenHomepage()
{
    accept();
    kapp->invokeBrowser("http://www.trinitydesktop.org");
}

void KMenu::slotLogout()
{
    kapp->requestShutDown();
}

void KMenu::slotPopulateSessions()
{
    int p = 0;
    DM dm;

    sessionsMenu->clear();
    if (kapp->authorize("start_new_session") && (p = dm.numReserve()) >= 0)
    {
        if (kapp->authorize("lock_screen"))
	  sessionsMenu->insertItem(/*SmallIconSet("lockfork"),*/ i18n("Lock Current && Start New Session"), 100 );
        sessionsMenu->insertItem(SmallIconSet("fork"), i18n("Start New Session"), 101 );
        if (!p) {
            sessionsMenu->setItemEnabled( 100, false );
            sessionsMenu->setItemEnabled( 101, false );
        }
        sessionsMenu->insertSeparator();
    }
    SessList sess;
    if (dm.localSessions( sess ))
        for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
            int id = sessionsMenu->insertItem( DM::sess2Str( *it ), (*it).vt );
            if (!(*it).vt)
                sessionsMenu->setItemEnabled( id, false );
            if ((*it).self)
                sessionsMenu->setItemChecked( id, true );
        }
}

void KMenu::slotSessionActivated( int ent )
{
    if (ent == 100)
        doNewSession( true );
    else if (ent == 101)
        doNewSession( false );
    else if (!sessionsMenu->isItemChecked( ent ))
        DM().lockSwitchVT( ent );
}

void KMenu::doNewSession( bool lock )
{
    int result = KMessageBox::warningContinueCancel(
        TQT_TQWIDGET(kapp->desktop()->screen(kapp->desktop()->screenNumber(this))),
        i18n("<p>You have chosen to open another desktop session.<br>"
               "The current session will be hidden "
               "and a new login screen will be displayed.<br>"
               "An F-key is assigned to each session; "
               "F%1 is usually assigned to the first session, "
               "F%2 to the second session and so on. "
               "You can switch between sessions by pressing "
               "Ctrl, Alt and the appropriate F-key at the same time. "
               "Additionally, the TDE Panel and Desktop menus have "
               "actions for switching between sessions.</p>")
                           .arg(7).arg(8),
        i18n("Warning - New Session"),
        KGuiItem(i18n("&Start New Session"), "fork"),
        ":confirmNewSession",
        KMessageBox::PlainCaption | KMessageBox::Notify);

    if (result==KMessageBox::Cancel)
        return;

    if (lock)
        slotLock();

    DM().startReserve();
}

void KMenu::searchAccept()
{
    TQString cmd = m_kcommand->currentText().stripWhiteSpace();

    bool logout = (cmd == "logout");
    bool lock = (cmd == "lock");

    addToHistory();

    if ( !logout && !lock )
    {
        // first try if we have any search action
        if (m_searchResultsWidget->currentItem()) {
            m_searchResultsWidget->slotItemClicked(m_searchResultsWidget->currentItem());
            return;
        }
    }

    accept();
    saveConfig();

    if ( logout )
    {
        kapp->propagateSessionManager();
        kapp->requestShutDown();
    }
    if ( lock )
    {
        TQCString appname( "kdesktop" );
        int kicker_screen_number = tqt_xscreen();
        if ( kicker_screen_number )
            appname.sprintf("kdesktop-screen-%d", kicker_screen_number);
        kapp->dcopClient()->send(appname, "KScreensaverIface", "lock()", TQString(""));
    }
}

bool KMenu::runCommand()
{
    kdDebug() << "runCommand() " << m_kcommand->lineEdit()->text() << endl;
    // Ignore empty commands...
    if ( m_kcommand->lineEdit()->text().isEmpty() )
      return true;

    accept();

    if (input_timer->isActive ())
        input_timer->stop ();

    // Make sure we have an updated data
    parseLine( true );

    bool block = m_kcommand->signalsBlocked();
    m_kcommand->blockSignals( true );
    m_kcommand->clearEdit();
    m_kcommand->setFocus();
    m_kcommand->reset();
    m_kcommand->blockSignals( block );


    TQString cmd;
    KURL uri = m_filterData->uri();
    if ( uri.isLocalFile() && !uri.hasRef() && uri.query().isEmpty() )
      cmd = uri.path();
    else
      cmd = uri.url();

    TQString exec;

    switch( m_filterData->uriType() )
    {
      case KURIFilterData::LOCAL_FILE:
      case KURIFilterData::LOCAL_DIR:
      case KURIFilterData::NET_PROTOCOL:
      case KURIFilterData::HELP:
      {
        // No need for kfmclient, KRun does it all (David)
        (void) new KRun( m_filterData->uri(), parentWidget());
        return false;
      }
      case KURIFilterData::EXECUTABLE:
      {
        if( !m_filterData->hasArgsAndOptions() )
        {
          // Look for desktop file
          KService::Ptr service = KService::serviceByDesktopName(cmd);
          if (service && service->isValid() && service->type() == "Application")
          {
            notifyServiceStarted(service);
            KRun::run(*service, KURL::List());
            return false;
          }
        }
      }
      // fall-through to shell case
      case KURIFilterData::SHELL:
      {
        if (kapp->authorize("shell_access"))
        {
          exec = cmd;

          if( m_filterData->hasArgsAndOptions() )
            cmd += m_filterData->argsAndOptions();

          break;
        }
        else
        {
          KMessageBox::sorry( this, i18n("<center><b>%1</b></center>\n"
                                    "You do not have permission to execute "
                                    "this command.")
                                    .arg( TQStyleSheet::convertFromPlainText(cmd) ));
          return true;
        }
      }
      case KURIFilterData::UNKNOWN:
      case KURIFilterData::ERROR:
      default:
      {
        // Look for desktop file
        KService::Ptr service = KService::serviceByDesktopName(cmd);
        if (service && service->isValid() && service->type() == "Application")
        {
          notifyServiceStarted(service);
          KRun::run(*service, KURL::List(), this);
          return false;
        }

        service = KService::serviceByName(cmd);
        if (service && service->isValid() && service->type() == "Application")
        {
          notifyServiceStarted(service);
          KRun::run(*service, KURL::List(), this);
          return false;
      }

        KMessageBox::sorry( this, i18n("<center><b>%1</b></center>\n"
                                  "Could not run the specified command.")
                                  .arg( TQStyleSheet::convertFromPlainText(cmd) ));
        return true;
      }
    }

    if ( KRun::runCommand( cmd, exec, m_iconName ) )
      return false;

    KMessageBox::sorry( this, i18n("<center><b>%1</b></center>\n"
                                "The specified command does not exist.").arg(cmd) );
    return true; // Let the user try again...
}

void KMenu::show()
{
    m_isShowing = true;
    emit aboutToShow();

    initialize();

    PanelPopupButton *kButton = MenuManager::the()->findKButtonFor( this );
    if (kButton)
    {
        TQPoint center = kButton->center();
        TQRect screen = TQApplication::desktop()->screenGeometry( center );
        setOrientation((center.y()-screen.y()<screen.height()/2)
                ? TopDown : BottomUp);
    }

    m_browserDirty=true;
    m_recentDirty=true;

    updateMedia();
    m_mediaFreeTimer.start(10 * 1000); // refresh all 10s

    m_stacker->raiseWidget(FavoriteTab);
    m_kcommand->clear();
    current_query.clear();
    m_kcommand->setFocus();

    // we need to reenable it
    m_toolTipsEnabled = TQToolTip::isGloballyEnabled();
    TQToolTip::setGloballyEnabled(KickerSettings::showToolTips());

    KMenuBase::show();
    m_isShowing = false;
}

void KMenu::setOrientation(MenuOrientation orientation)
{
    if (m_orientation == orientation)
        return;

    m_orientation=orientation;

    m_resizeHandle->setCursor(m_orientation == BottomUp ? tqsizeBDiagCursor : tqsizeFDiagCursor);

    TQPixmap pix;
    if ( m_orientation == BottomUp )
        pix.load( locate("data", "kicker/pics/search-gradient.png" ) );
    else
        pix.load( locate("data", "kicker/pics/search-gradient-topdown.png" ) );

    pix.convertFromImage( pix.convertToImage().scale(pix.width(), m_searchFrame->height()));
    m_search->mainWidget()->setPaletteBackgroundPixmap( pix );
    m_resizeHandle->setPaletteBackgroundPixmap( pix );

    m_tabBar->setShape( m_orientation == BottomUp
            ? TQTabBar::RoundedBelow : TQTabBar::RoundedAbove);

    TQPixmap respix = TQPixmap( locate("data", "kicker/pics/resize_handle.png" ) );
    if ( m_orientation == TopDown ) {
      TQWMatrix m;
      m.rotate( 90.0 );
      respix=respix.xForm(m);
    }
    m_resizeHandle->setPixmap(respix);

    {
        TQWidget *footer = m_footer->mainWidget();
        TQPixmap pix( 64, footer->height() );
        TQPainter p( &pix );
        p.fillRect( 0, 0, 64, footer->height(), m_branding->colorGroup().brush( TQColorGroup::Base ) );
        p.end();
        footer->setPaletteBackgroundPixmap( pix );
    }

    resizeEvent(new TQResizeEvent(sizeHint(), sizeHint()));
}

void KMenu::showMenu()
{
    kdDebug() << "KMenu::showMenu()" << endl;
    PanelPopupButton *kButton = MenuManager::the()->findKButtonFor(this);
    if (kButton)
    {
	adjustSize();
        kButton->showMenu();
    }
    else
    {
        show();
    }
    kdDebug() << "end KMenu::showMenu()" << endl;
}

void KMenu::hide()
{
    //kdDebug() << "KMenu::hide() from " << kdBacktrace() << endl;

    // TODO: hide popups

    emit aboutToHide();

    if (m_popupMenu) {
        m_popupMenu->deleteLater();
        m_popupMenu=0;
    }
    m_mediaFreeTimer.stop();

    m_isresizing = false;

    KickerSettings::setKMenuWidth(width());
    KickerSettings::setKMenuHeight(height());
    KickerSettings::writeConfig();

    TQToolTip::setGloballyEnabled(m_toolTipsEnabled);

    // remove focus from lineedit again, otherwise it doesn't kill its timers
    m_stacker->raiseWidget(FavoriteTab);

    TQWidget::hide();
}

void KMenu::paintEvent(TQPaintEvent * e)
{
    KMenuBase::paintEvent(e);

    TQPainter p(this);
    p.setClipRegion(e->region());

    const BackgroundMode bgmode = backgroundMode();
    const TQColorGroup::ColorRole crole = TQPalette::backgroundRoleFromMode( bgmode );
    p.setBrush( colorGroup().brush( crole ) );

    p.drawRect( 0, 0, width(), height() );
    int ypos = m_search->mainWidget()->geometry().bottom();

    p.drawPixmap( 0, ypos, main_border_tl );
    p.drawPixmap( width() - main_border_tr.width(), ypos, main_border_tr );
 //   p.drawPixmap( 0, ->y(), button_box_left );
}


void KMenu::configChanged()
{
    RecentlyLaunchedApps::the().m_bNeedToUpdate = false;
    RecentlyLaunchedApps::the().configChanged();

    m_exitView->leftView()->clear();
    insertStaticExitItems();
}

// create and fill "recent" section at first
void KMenu::createRecentMenuItems()
{
    RecentlyLaunchedApps::the().init();

    if (!KickerSettings::numVisibleEntries())
      KickerSettings::setNumVisibleEntries(5);

    int nId = serviceMenuEndId() + 1;
    m_recentlyView->insertSeparator( nId++, i18n( "Applications" ), -1 );

    TQStringList RecentApps;

    if (!KickerSettings::recentVsOften()) {
        KickerSettings::setRecentVsOften(true);
        RecentlyLaunchedApps::the().configChanged();
        RecentlyLaunchedApps::the().getRecentApps(RecentApps);
        KickerSettings::setRecentVsOften(false);
        RecentlyLaunchedApps::the().configChanged();
    }
    else
        RecentlyLaunchedApps::the().getRecentApps(RecentApps);


    if (RecentApps.count() > 0)
    {
//        bool bSeparator = KickerSettings::showMenuTitles();
        int nIndex = 0;

        for (TQValueList<TQString>::ConstIterator it =
             RecentApps.begin(); it!=RecentApps.end(); ++it)
        {
            KService::Ptr s = KService::serviceByStorageId(*it);
            if (!s)
            {
                RecentlyLaunchedApps::the().removeItem(*it);
            }
            else
                m_recentlyView->insertMenuItem(s, nIndex++);
        }

    }

    m_recentlyView->insertSeparator( nId++, i18n( "Documents" ), -1 );

    TQStringList fileList = TDERecentDocument::recentDocuments();
    kdDebug() << "createRecentMenuItems=" << fileList << endl;
    for (TQStringList::ConstIterator it = fileList.begin();
         it != fileList.end();
         ++it)
        m_recentlyView->insertRecentlyItem(*it, nId++);

}

void KMenu::clearSubmenus()
{
    // we don't need to delete these on the way out since the libloader
    // handles them for us
    if (TQApplication::closingDown())
    {
        return;
    }

    for (PopupMenuList::const_iterator it = dynamicSubMenus.constBegin();
            it != dynamicSubMenus.constEnd();
            ++it)
    {
        delete *it;
    }
    dynamicSubMenus.clear();
}

void KMenu::updateRecent()
{
    m_recentlyView->clear();

    createRecentMenuItems();

    m_recentDirty = false;
}

void KMenu::popup(const TQPoint&, int)
{
   showMenu();
}

void KMenu::clearRecentAppsItems()
{
    RecentlyLaunchedApps::the().clearRecentApps();
    RecentlyLaunchedApps::the().save();
    RecentlyLaunchedApps::the().m_bNeedToUpdate = true;
    updateRecent();
}

void KMenu::clearRecentDocsItems()
{
    TDERecentDocument::clear();
    updateRecent();
}

void KMenu::searchChanged(const TQString & text)
{
  if (!text.isEmpty()) {
    const TQColor on = TQColor( 244, 244, 244 );
    const TQColor off = TQColor( 181, 181, 181 );
    m_stacker->raiseWidget(m_searchWidget);
    paintSearchTab(true);
  }

  m_searchActions->clearSelection();
  m_searchResultsWidget->clearSelection();

  if (input_timer->isActive ())
    input_timer->stop ();
  input_timer->start (WAIT_BEFORE_QUERYING, TRUE);
}

bool KMenu::dontQueryNow (const TQString& str)
{
    if (str.isEmpty ())
        return true;
    if (str == current_query.get())
	return true;
    int length = str.length ();
    int last_whitespace = str.findRev (' ', -1);
    if (last_whitespace == length-1)
        return false; // if the user typed a space, search
    if (last_whitespace >= length-2)
        return true; // dont search if the user only typed one character
    TQChar lastchar = str[length-1];
    if (lastchar == ':' || lastchar == '=')
        return true;
    return false;
}

void KMenu::createNewProgramList()
{
    m_seenProgramsChanged = false;
    m_seenPrograms = KickerSettings::firstSeenApps();
    m_newInstalledPrograms.clear();

    m_currentDate = TQDate::currentDate().toString(Qt::ISODate);

    bool initialize = (m_seenPrograms.count() == 0);

    createNewProgramList(TQString());

    if (initialize) {
       for (TQStringList::Iterator it = m_seenPrograms.begin(); it != m_seenPrograms.end(); ++it)
           *(++it)="-";

        m_newInstalledPrograms.clear();
    }

    if (m_seenProgramsChanged) {
      KickerSettings::setFirstSeenApps(m_seenPrograms);
      KickerSettings::writeConfig();
    }
}

void KMenu::createNewProgramList(TQString relPath)
{
    KServiceGroup::Ptr group = KServiceGroup::group(relPath);
    if (!group || !group->isValid())
      return;

    KServiceGroup::List list = group->entries();
    if (list.isEmpty())
      return;

    KServiceGroup::List::ConstIterator it = list.begin();
    for(; it != list.end(); ++it) {
	KSycocaEntry *e = *it;

	if(e != 0) {
		if(e->isType(KST_KServiceGroup)) {
			KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
			if(!g->noDisplay())
				createNewProgramList(g->relPath());
		} else if(e->isType(KST_KService)) {
			KService::Ptr s(static_cast<KService *>(e));
			if(s->type() == "Application" && !s->noDisplay() ) {
                            TQString shortStorageId = s->storageId().replace(".desktop",TQString());
                            TQStringList::Iterator it_find = m_seenPrograms.begin();
                            TQStringList::Iterator it_end = m_seenPrograms.end();
 			    bool found = false;
			    for (; it_find != it_end; ++it_find) {
				if (*(it_find)==shortStorageId) {
				   found = true;
				   break;
                                }
                                ++it_find;
                            }
                            if (!found) {
                                m_seenProgramsChanged=true;
                                m_seenPrograms+=shortStorageId;
                                m_seenPrograms+=m_currentDate;
                                if (m_newInstalledPrograms.find(s->storageId())==m_newInstalledPrograms.end())
                                  m_newInstalledPrograms+=s->storageId();
                            }
                            else {
                                ++it_find;
                                if (*(it_find)!="-") {
                                   TQDate date = TQDate::fromString(*(it_find),Qt::ISODate);
                                   if (date.daysTo(TQDate::currentDate())<3) {
                                      if (m_newInstalledPrograms.find(s->storageId())==m_newInstalledPrograms.end())
                                         m_newInstalledPrograms+=s->storageId();
                                   }
                                   else {
                                      m_seenProgramsChanged=true;
                                      (*it_find)="-";
                                   }
                                }
                            }
                        }
		}
	}
    }
}

void KMenu::searchProgramList(TQString relPath)
{
    KServiceGroup::Ptr group = KServiceGroup::group(relPath);
    if (!group || !group->isValid())
      return;

    KServiceGroup::List list = group->entries();
    if (list.isEmpty())
      return;

    KServiceGroup::List::ConstIterator it = list.begin();
    for(; it != list.end(); ++it) {
	KSycocaEntry *e = *it;

	if(e != 0) {
		if(e->isType(KST_KServiceGroup)) {
			KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
			if(!g->noDisplay())
				searchProgramList(g->relPath());
		} else if(e->isType(KST_KService)) {
			KService::Ptr s(static_cast<KService *>(e));
			if(s->type() == "Application" && !s->noDisplay() && !checkUriInMenu(s->desktopEntryPath())) {
				if (!current_query.matches(s->name()+' '+s->genericName()+' '+s->exec()+' '+
				    s->keywords().join(",")+' '+s->comment()+' '+group->caption()+' '+
				    s->categories().join(",")) || !anotherHitMenuItemAllowed(APPS))
					continue;

				TQString input = current_query.get();
				int score = 0;
				if (s->exec()==input)
					score = 100;
				else if (s->exec().find(input)==0)
					score = 50;
				else if (s->exec().find(input)!=-1)
					score = 10;
				else if (s->name().lower()==input)
					score = 100;
				else if (s->genericName().lower()==input)
					score = 100;
				else if (s->name().lower().find(input)==0)
					score = 50;
				else if (s->genericName().lower().find(input)==0)
					score = 50;
				else if (s->name().lower().find(input)!=-1)
					score = 10;
				else if (s->genericName().lower().find(input)!=-1)
					score = 10;

				if (s->exec().find(' ')==-1)
					score+=1;

				if (s->substituteUid())
					score-=1;

				if (s->noDisplay())
				    score -= 100;
				else if (s->terminal())
				    score -= 50;
				else
				    score += kMin(10, s->initialPreference());

				TQString firstLine, secondLine;
				if ((KickerSettings::DescriptionAndName || KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly) && !s->genericName().isEmpty()) {
									firstLine = s->genericName();
					secondLine = s->name();
				}
				else {
					firstLine = s->name();
					secondLine = s->genericName();
				}

        			HitMenuItem *hit_item = new HitMenuItem (firstLine, secondLine,
                                s->desktopEntryPath(), TQString(), 0, APPS, s->icon(), score);
        			if (hit_item == NULL)
					continue;

				hit_item->service = s;
        			insertSearchResult(hit_item);

                                TQString exe = s->exec();
                                int pos = exe.find(' ');
                                if (pos>0)
                                  exe=exe.left(pos);
				m_programsInMenu+=TDEGlobal::dirs()->findExe(exe);
			}
		}
	}
    }
}

void KMenu::searchBookmarks(KBookmarkGroup group)
{
	KBookmark bookmark = group.first();
	while(!bookmark.isNull()) {
		if (bookmark.isGroup()) {
			searchBookmarks(bookmark.toGroup());
		} else if (!bookmark.isSeparator() && !bookmark.isNull()) {
				if (!current_query.matches(bookmark.fullText()+' '+bookmark.url().url()) || !anotherHitMenuItemAllowed(BOOKMARKS)) {
                                        bookmark = group.next(bookmark);
	    				continue;
                                }

        			HitMenuItem *hit_item = new HitMenuItem (bookmark.fullText(), bookmark.fullText(),
                                bookmark.url(), TQString(), 0, BOOKMARKS, bookmark.icon());

        			insertSearchResult(hit_item);
		}
		bookmark = group.next(bookmark);
	}
}

void KMenu::initSearch()
{
    if (!m_addressBook && KickerSettings::kickoffSearchAddressBook())
       m_addressBook = TDEABC::StdAddressBook::self( false );

    if (!bookmarkManager)
      bookmarkManager = KBookmarkManager::userBookmarksManager();

    if (!m_search_plugin) {
      m_search_plugin_interface = new TQObject( this, "m_search_plugin_interface" );
      new MyKickoffSearchInterface( this, m_search_plugin_interface, "kickoffsearch interface" );
      TDETrader::OfferList offers = TDETrader::self()->query("KickoffSearch/Plugin");

      KService::Ptr service = *offers.begin();
      if (service) {
        int errCode = 0;
        m_search_plugin = KParts::ComponentFactory::createInstanceFromService<KickoffSearch::Plugin>
            ( service, m_search_plugin_interface, 0, TQStringList(), &errCode);
      }
    }
}

void KMenu::searchAddressbook()
{
    if (!KickerSettings::kickoffSearchAddressBook())
      return;

    if (!m_addressBook)
      m_addressBook = TDEABC::StdAddressBook::self( false );

    TDEABC::AddressBook::ConstIterator it = m_addressBook->begin();
    while (it!=m_addressBook->end()) {
        if (!current_query.matches((*it).assembledName()+' '+(*it).fullEmail())) {
            it++;
            continue;
        }

        HitMenuItem *hit_item;
        TQString realName = (*it).realName();
        if (realName.isEmpty())
            realName=(*it).preferredEmail();

        if (!(*it).preferredEmail().isEmpty()) {
	    if (!anotherHitMenuItemAllowed(ACTIONS)) {
               it++;
               continue;
            }

            hit_item = new HitMenuItem (i18n("Send Email to %1").arg(realName), (*it).preferredEmail(),
                                "mailto:"+(*it).preferredEmail(), TQString(), 0, ACTIONS, "mail-message-new");

        			insertSearchResult(hit_item);
        }

	if (!anotherHitMenuItemAllowed(ACTIONS)) {
               it++;
               continue;
        }

        hit_item = new HitMenuItem (i18n("Open Addressbook at %1").arg(realName), (*it).preferredEmail(),
                                "kaddressbook:/"+(*it).uid(), TQString(), 0, ACTIONS, "kaddressbook");

        			insertSearchResult(hit_item);

       it++;
    }
}

TQString KMenu::insertBreaks(const TQString& text, TQFontMetrics fm, int width, TQString leadInsert)
{
    TQString result, line;
    TQStringList words = TQStringList::split(' ', text);

    for(TQStringList::Iterator it = words.begin(); it != words.end(); ++it) {
       if (fm.width(line+' '+*it) >= width) {
          if (!result.isEmpty())
             result = result + '\n';
          result = result + line;
          line = leadInsert + *it;
       }
       else
          line = line + ' ' + *it;
    }
    if (!result.isEmpty())
       result = result + '\n';

    return result + line;
}

void KMenu::clearSearchResults(bool showHelp)
{
    m_searchResultsWidget->clear();
    m_searchResultsWidget->setFocusPolicy(showHelp ? TQ_NoFocus : TQ_StrongFocus);
    setTabOrder(m_kcommand, m_searchResultsWidget);

    if (showHelp) {
        const int width = m_searchResultsWidget->width()-10;
        TQFontMetrics fm = m_searchResultsWidget->fontMetrics();

        TQListViewItem* item;
        item = new TQListViewItem( m_searchResultsWidget, insertBreaks(i18n("- Add ext:type to specify a file extension."), fm, width, "   ") );
        item->setSelectable(false);
        item->setMultiLinesEnabled(true);
        item = new TQListViewItem( m_searchResultsWidget, insertBreaks(i18n("- When searching for a phrase, add quotes."), fm, width, "   " ) );
        item->setSelectable(false);
        item->setMultiLinesEnabled(true);
        item = new TQListViewItem( m_searchResultsWidget, insertBreaks(i18n("- To exclude search terms, use the minus symbol in front."), fm, width, "   " ) );
        item->setSelectable(false);
        item->setMultiLinesEnabled(true);
        item = new TQListViewItem( m_searchResultsWidget, insertBreaks(i18n("- To search for optional terms, use OR."), fm, width, "   ")  );
        item->setSelectable(false);
        item->setMultiLinesEnabled(true);
        item = new TQListViewItem( m_searchResultsWidget, insertBreaks(i18n("- You can use upper and lower case."), fm, width, "   ")  );
        item->setSelectable(false);
        item->setMultiLinesEnabled(true);
        item = new TQListViewItem( m_searchResultsWidget, i18n("Search Quick Tips"));
        item->setSelectable(false);
    }

    for (int i=0; i<num_categories; ++i) {
	categorised_hit_total [i] = 0;
	max_category_id [i] = base_category_id [i];
    }
}

void KMenu::doQuery (bool return_pressed)
{
    TQString query_str = m_kcommand->lineEdit()->text ().simplifyWhiteSpace ();
    if (! return_pressed && dontQueryNow (query_str)) {
        if (query_str.length()<3)
            clearSearchResults();
        else {
            if (m_searchResultsWidget->firstChild() && m_searchResultsWidget->firstChild()->isSelectable()) {
                m_searchResultsWidget->setSelected(m_searchResultsWidget->firstChild(),true);
            }
            else if (m_searchResultsWidget->childCount()>1) {
                m_searchResultsWidget->setSelected(m_searchResultsWidget->firstChild()->itemBelow(),true);
            }
        }
        return;
    }
    kdDebug() << "Querying for [" << query_str << "]" << endl;
    current_query.set(query_str);

    // reset search results
    HitMenuItem *hit_item;
    while ((hit_item = m_current_menu_items.take ()) != NULL) {
        //kndDebug () << " (" << hit_item->id << "," << hit_item->category << ")" << endl;
        delete hit_item;
    }

    clearSearchResults(false);
    m_searchPixmap->setMovie(TQMovie(locate( "data", "kicker/pics/search-running.mng" )));

    resetOverflowCategory();

    initCategoryTitlesUpdate();

    // calculate ?
    TQString cmd = query_str.stripWhiteSpace();
    if (!cmd.isEmpty() && (cmd[0].isNumber() || (cmd[0] == '(')) &&
            (TQRegExp("[a-zA-Z\\]\\[]").search(cmd) == -1))
    {
        TQString result = calculate(cmd);
        if (!result.isEmpty())
        {
            categorised_hit_total[ACTIONS] ++;
            HitMenuItem *hit_item = new HitMenuItem (i18n("%1 = %2").arg(query_str, result), TQString(),
                    "kcalc", TQString(), (++max_category_id [ACTIONS]), ACTIONS, "kcalc");
            int index = getHitMenuItemPosition (hit_item);
            m_searchResultsWidget->insertItem(iconForHitMenuItem(hit_item), hit_item->display_name,
                    hit_item->display_info, TDEGlobal::dirs()->findExe("kcalc"), max_category_id [ACTIONS], index);
        }
    }

    // detect email address
    if (emailRegExp.exactMatch(query_str)) {
      categorised_hit_total[ACTIONS] ++;
      HitMenuItem *hit_item = new HitMenuItem (i18n("Send Email to %1").arg(query_str), TQString(),
                                "mailto:"+query_str, TQString(), (++max_category_id [ACTIONS]), ACTIONS, "mail-message-new");
      int index = getHitMenuItemPosition (hit_item);
      m_searchResultsWidget->insertItem(iconForHitMenuItem(hit_item), hit_item->display_name, hit_item->display_info, "mailto:"+query_str, max_category_id [ACTIONS], index);
    }

    // quick own application search
    m_programsInMenu.clear();
    searchProgramList(TQString());

    KURIFilterData filterData;
    filterData.setData(query_str);
    filterData.setCheckForExecutables(true);

    if (KURIFilter::self()->filterURI(filterData)) {

        TQString description;
        TQString exe;

        switch (filterData.uriType()) {
        case KURIFilterData::LOCAL_FILE:
            description = i18n("Open Local File: %1").arg(filterData.uri().url());
            break;
        case KURIFilterData::LOCAL_DIR:
            description = i18n("Open Local Dir: %1").arg(filterData.uri().url());
            break;
        case KURIFilterData::NET_PROTOCOL:
            description = i18n("Open Remote Location: %1").arg(filterData.uri().url());
            break;
        case KURIFilterData::SHELL:
        case KURIFilterData::EXECUTABLE:
        {
            exe = TDEGlobal::dirs()->findExe(filterData.uri().url());
#ifdef KDELIBS_SUSE
            bool gimp_hack = false;
            if (exe.endsWith("/bin/gimp")) {
               TQStringList::ConstIterator it = m_programsInMenu.begin();
               for (; it != m_programsInMenu.end(); ++it)
                  if ((*it).find("bin/gimp-remote-")!=-1) {
                    gimp_hack = true;
                    break;
                  }
            }
#endif
            if (m_programsInMenu.find(exe)!=m_programsInMenu.end()
#ifdef KDELIBS_SUSE
                || gimp_hack
#endif
                )
                exe = TQString();
            else if (kapp->authorize("shell_access"))
            {
                if( filterData.hasArgsAndOptions() )
                    exe += filterData.argsAndOptions();

                description = i18n("Run '%1'").arg(exe);
                exe = "kicker:/runcommand";
            }
        }
        default:
            break;
        }

        if (!description.isEmpty()) {
            categorised_hit_total[ACTIONS] ++;
            HitMenuItem *hit_item = new HitMenuItem (description, TQString(),
                    exe.isEmpty() ? filterData.uri() : exe, TQString(),
                    (++max_category_id [ACTIONS]), ACTIONS, exe.isEmpty() ? "document-open": "run");
            int index = getHitMenuItemPosition (hit_item);
            m_searchResultsWidget->insertItem(iconForHitMenuItem(hit_item), hit_item->display_name,
                    hit_item->display_info,
                    exe.isEmpty() ? filterData.uri().url() : exe, max_category_id [ACTIONS], index);
        }
    }

    // search Konqueror bookmarks;
    if (!bookmarkManager)
      bookmarkManager = KBookmarkManager::userBookmarksManager();

    if (query_str.length()>=3)
      searchBookmarks(bookmarkManager->root());

    // search TDE addressbook
    if (query_str.length()>=3)
      searchAddressbook();

    updateCategoryTitles();

    if (m_searchResultsWidget->childCount()>1)
      m_searchResultsWidget->setSelected(m_searchResultsWidget->firstChild()->itemBelow(),true);
    m_searchActions->clearSelection();

    if (!m_search_plugin)
      initSearch();

    // start search plugin only with at least 3 characters
    if (query_str.length()<3 || !m_search_plugin || (m_search_plugin && !m_search_plugin->daemonRunning()) ) {
      m_searchPixmap->setPixmap( BarIcon( "edit-find", 32 ) );
      fillOverflowCategory();
      if (query_str.length()>2 && m_current_menu_items.isEmpty())
	  reportError (i18n("No matches found"));
      return;
    }

    if (m_search_plugin) {
      m_search_plugin->query(current_query.get(), KickerSettings::DescriptionAndName || KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly);
    }
}

bool KMenu::anotherHitMenuItemAllowed(int cat, bool count)
{
	// update number of hits in this category
        if (count)
	    categorised_hit_total [cat] ++;

	// if number of hits in this category is more than allowed, dont process this
	if (max_category_id [cat] - base_category_id [cat] < max_items(cat))
            return true;

        if (m_overflowCategoryState==None || (m_overflowCategoryState==Filling && m_overflowCategory==cat &&
            max_category_id [cat] + m_overflowList.count() - base_category_id [cat] < max_items(cat) * 2.0))
            return true;

        return false;
}

void KMenu::addHitMenuItem(HitMenuItem* item)
{
        if (checkUriInMenu(item->uri))
            return;

	// if number of hits in this category is more than allowed, dont process this
	if (!anotherHitMenuItemAllowed(item->category, false))
	    return;

        insertSearchResult(item);
}

void KMenu::insertSearchResult(HitMenuItem* item)
{
        if (m_overflowCategoryState==None) {
            m_overflowCategoryState = Filling;
            m_overflowCategory = item->category;
        }
        else if (m_overflowCategoryState==Filling && m_overflowCategory!=item->category)
            m_overflowCategoryState = NotNeeded;

        if (max_category_id [item->category] - base_category_id [item->category] < max_items(item->category)) {
	    max_category_id [item->category]++;
            item->id=max_category_id [item->category];

            int index = getHitMenuItemPosition (item);

            kdDebug () << "Adding " << item->uri
    		    << "(" << item->mimetype << ") with id="
    		    << max_category_id [item->category] << " at " << index << endl;

            KMenuItem *hit_item = m_searchResultsWidget->insertItem(iconForHitMenuItem(item), item->display_name, item->display_info, item->uri.url(), max_category_id [item->category], index);
            hit_item->setService(item->service);

            kdDebug () << "Done inserting ... " << endl;
        }
        else if (m_overflowCategoryState==Filling && m_overflowCategory==item->category &&
            max_category_id [item->category] - base_category_id [item->category] < max_items(item->category) * 2)
               m_overflowList.append(item);
}

void KMenu::searchOver()
{
    m_searchPixmap->setPixmap( BarIcon( "edit-find", 32 ) );
    fillOverflowCategory();
    if (m_current_menu_items.isEmpty()) {
        kdDebug() << "No matches found" << endl;
	reportError (i18n("No matches found"));
    }
    if (!m_searchResultsWidget->selectedItem() && !m_searchActions->selectedItem() && m_searchResultsWidget->childCount()>1) {
            m_searchResultsWidget->setSelected(m_searchResultsWidget->firstChild()->itemBelow(),true);
        }
}

void KMenu::initCategoryTitlesUpdate()
{
    // Need to know if each category was updated with hits or had the first hit
    // That way we know if we need to changetitle or inserttitle
    already_added = new bool [num_categories];
    for (int i=0; i<num_categories; ++i)
	already_added [i] = (max_category_id [i] != base_category_id [i]);
}

void KMenu::updateCategoryTitles()
{
    // update category title
    for (int i=0; i<num_categories; ++i) {
	if (i == OTHER)
	    continue;
	// nothing is in this category
	if (max_category_id [i] == base_category_id [i])
	    continue;

        KMenuItemSeparator *sep = 0;

	// if nothing was in this category before but now there is
	if (! already_added [i]) {
	    // insert a new title for this category
	    int index = getHitMenuItemPosition (new HitMenuItem (
						    base_category_id[i],
						    i));
            TQString title = TQString ("%1").arg (i18n(categories [i].utf8()));
            sep = m_searchResultsWidget->insertSeparator(base_category_id [i], title, index);
	    kdDebug () << "Inserting heading with id=" << base_category_id[i] << " for " << categories[i] << " at " << index << endl;
	} else {
	    // something was already displayed in this category
	    // update the title to reflect the total
            sep = dynamic_cast<KMenuItemSeparator*>( m_searchResultsWidget->findItem(base_category_id [i]) );
            if ( !sep )
                continue;
            kdDebug () << "Changing heading of id=" << base_category_id[i] << " for " << categories[i] << endl;
	}

        int max = max_items(i);
        if (m_overflowCategoryState == Filling && m_overflowCategory == i)
            max *= 2;

        if ( categorised_hit_total [i] > max ) {
            if (m_kerryInstalled)
                sep->setLink( i18n( "top %1 of %2" ).arg( max ).arg( categorised_hit_total [i] ), TQString( "kerry:/%1" ).arg( i ) );
            else
                sep->setText( 0, i18n( "%1 (top %2 of %3)" ).arg( i18n(categories [i].utf8()) ).arg( max ).arg( categorised_hit_total [i] ) );
        }
        else {
            sep->setLink( TQString() );
        }
    }
    delete[] already_added;
    already_added = 0;
}

TQString KMenu::iconForHitMenuItem(HitMenuItem *hit_item)
{
    // get the icon
    if (!hit_item->icon.isEmpty())
        return hit_item->icon;

    if (hit_item->category == WEBHIST) {
        TQString favicon = KMimeType::favIconForURL (hit_item->uri);
        if (! favicon.isEmpty ())
	    return favicon;
    }

    if (mimetype_iconstore.contains (hit_item->mimetype))
        return (mimetype_iconstore [hit_item->mimetype]);
    else {
        KMimeType::Ptr mimetype_ptr = KMimeType::mimeType (hit_item->mimetype);
        TQString mimetype_icon = mimetype_ptr->icon(TQString(), FALSE);
        mimetype_iconstore [hit_item->mimetype] = mimetype_icon;
        return mimetype_icon;
    }
    return TQString();
}

void KMenu::slotStartService(KService::Ptr ptr)
{
    accept();

    addToHistory();
    TDEApplication::startServiceByDesktopPath(ptr->desktopEntryPath(),
                                            TQStringList(), 0, 0, 0, "", true);
    updateRecentlyUsedApps(ptr);
}


void KMenu::slotStartURL(const TQString& u)
{
    if ( u == "kicker:/goup/" ) {
        // only m_exitView is connected to this slot, not m_browserView
        slotGoExitMainMenu();
        return;
    }

    if ( u == "kicker:/restart/" || u=="kicker:/switchuser/") {
        slotGoExitSubMenu(u);
        return;
    }

    accept();

    if ( u == "kicker:/lock" ) {
        slotLock();
    }
    else if ( u == "kicker:/logout" ) {
        TQByteArray params;
        TQDataStream stream(params, IO_WriteOnly);
        stream << 0 << -1 << "";

        kapp->dcopClient()->send("ksmserver", "default", "logoutTimed(int,int,TQString)", params);
    }
    else if ( u == "kicker:/runcommand" )
    {
         runCommand();
    }
    else if ( u == "kicker:/runusercommand" )
    {
         runUserCommand();
    }
    else if ( u == "kicker:/shutdown" ) {
        TQByteArray params;
        TQDataStream stream(params, IO_WriteOnly);
        stream << 2 << -1 << "";

        kapp->dcopClient()->send("ksmserver", "default", "logoutTimed(int,int,TQString)", params);
    }
    else if ( u == "kicker:/restart" ) {
        TQByteArray params;
        TQDataStream stream(params, IO_WriteOnly);
        stream << 1 << -1 << TQString();

        kapp->dcopClient()->send("ksmserver", "default", "logoutTimed(int,int,TQString)", params);
    }
    else if ( u == "kicker:/suspend_disk" ) {
        slotSuspend( 1 );
    }
    else if ( u == "kicker:/suspend_ram" ) {
        slotSuspend( 2 );
    }
    else if ( u == "kicker:/suspend_freeze" ) {
        slotSuspend( 4 );
    }
    else if ( u == "kicker:/standby" ) {
        slotSuspend( 3 );
    }
    else if ( u == "kicker:/savesession" ) {
        TQByteArray data;
        kapp->dcopClient()->send( "ksmserver", "default",
                "saveCurrentSession()", data );
    }
    else if ( u == "kicker:/switchuser" ) {
        DM().startReserve();
    }
    else if ( u == "kicker:/switchuserafterlock" ) {
        slotLock();
        DM().startReserve();
    }
    else if ( u.startsWith("kicker:/switchuser_") )
        DM().lockSwitchVT( u.mid(19).toInt() );
    else if ( u.startsWith("kicker:/restart_") ) {
        TQStringList rebootOptions;
        int def, cur;
        DM().bootOptions( rebootOptions, def, cur );

        TQByteArray params;
        TQDataStream stream(params, IO_WriteOnly);
        stream << 1 << -1 << rebootOptions[u.mid(16).toInt()];

        kapp->dcopClient()->send("ksmserver", "default", "logoutTimed(int,int,TQString)", params);
    }
#warning restart entry not supported
#if 0
    else if ( u == "kicker:/restart_windows" ) {
        if (KMessageBox::Continue==KMessageBox::warningContinueCancel(this, i18n("Do you really want to reset the computer and boot Microsoft Windows"), i18n("Start Windows Confirmation"), KGuiItem(i18n("Start Windows"),"reload")))
            KMessageBox::error( this, TQString( "kicker:/restart_windows is not yet implemented " ) );
    }
#endif
    else if ( u.startsWith("kerry:/"))
    {
       TQByteArray data;
       TQDataStream arg(data, IO_WriteOnly);
       arg << m_kcommand->currentText() << kerry_categories[u.mid(7).toInt()];
       if (ensureServiceRunning("kerry"))
           kapp->dcopClient()->send("kerry","search","search(TQString,TQString)", data);
    }
    else {
        addToHistory();
        if (u.startsWith("kaddressbook:/")) {
            TDEProcess *proc = new TDEProcess;
            *proc << "kaddressbook" << "--uid" << u.mid(14);
            proc->start();
            accept();
            return;
        } else if (u.startsWith("note:/")) {
            TDEProcess *proc = new TDEProcess;
            *proc << "tomboy";
            *proc << "--open-note" << u;
            if (!proc->start())
               KMessageBox::error(0,i18n("Could not start Tomboy."));
            return;
        }
        else if (u.startsWith("knotes:/") ) {
            if (ensureServiceRunning("knotes")) {
                TQByteArray data;
                TQDataStream arg(data, IO_WriteOnly);
                arg << u.mid(9,22);

                kapp->dcopClient()->send("knotes","KNotesIface","showNote(TQString)", data);
            }
            return;
        }

        kapp->propagateSessionManager();
        (void) new KRun( u, parentWidget());
    }
}

void KMenu::slotContextMenuRequested( TQListViewItem * item, const TQPoint & pos, int /*col*/ )
{
    const TQObject* source = TQT_TQOBJECT_CONST(sender());

    if (!item)
        return;

    KMenuItem* kitem = dynamic_cast<KMenuItem*>(item);
    if (!kitem)
        return;

    KFileItemList _items;
    _items.setAutoDelete(true);

    if (dynamic_cast<KMenuItemSeparator*>(item))
        return;

    m_popupService = kitem->service();
    m_popupPath.menuPath = kitem->menuPath();
    if (!m_popupService) {
        m_popupPath.title = kitem->title();
        m_popupPath.description = kitem->description();
        m_popupPath.path = kitem->path();
        m_popupPath.icon = kitem->icon();

        if (m_popupPath.path.startsWith(locateLocal("data", TQString::fromLatin1("RecentDocuments/")))) {
               KDesktopFile df(m_popupPath.path,true);
               m_popupPath.path=df.readURL();
        }
    }

    m_popupMenu = new TDEPopupMenu(this);
    connect(m_popupMenu, TQT_SIGNAL(activated(int)), TQT_SLOT(slotContextMenu(int)));
    bool hasEntries = false;

    m_popupMenu->insertTitle(SmallIcon(kitem->icon()),kitem->title());

     if (TQT_BASE_OBJECT_CONST(source)==TQT_BASE_OBJECT(m_favoriteView))
     {
         hasEntries = true;
         m_popupMenu->insertItem(SmallIconSet("remove"),
             i18n("Remove From Favorites"), RemoveFromFavorites);
     }
     else if (!kitem->hasChildren() && !m_popupPath.path.startsWith("system:/") &&
        !m_popupPath.path.startsWith("kicker:/switchuser_") && !m_popupPath.path.startsWith("kicker:/restart_"))
     {
         hasEntries = true;
         int num = m_popupMenu->insertItem(SmallIconSet("bookmark_add"),
             i18n("Add to Favorites"), AddToFavorites);

         TQStringList favs = KickerSettings::favorites();
         if (m_popupService && favs.find(m_popupService->storageId())!=favs.end())
            m_popupMenu->setItemEnabled(num, false);
         else {
            TQStringList::Iterator it;
            for (it = favs.begin(); it != favs.end(); ++it)
            {
                if ((*it)[0]=='/')
                {
                    KDesktopFile df((*it),true);
                    if (df.readURL().replace("file://",TQString())==m_popupPath.path)
                        break;
                }
            }
            if (it!=favs.end())
                m_popupMenu->setItemEnabled(num, false);
         }
     }

     if (TQT_BASE_OBJECT_CONST(source)!=TQT_BASE_OBJECT(m_exitView)) {
        if (m_popupService || (!m_popupPath.path.startsWith("kicker:/") && !m_popupPath.path.startsWith("system:/") && !m_popupPath.path.startsWith("kaddressbook:/"))) {
            if (hasEntries)
                m_popupMenu->insertSeparator();

            if (kapp->authorize("editable_desktop_icons") )
            {
                hasEntries = true;
                if (m_popupPath.menuPath.endsWith("/"))
                  m_popupMenu->insertItem(SmallIconSet("desktop"),
                    i18n("Add Menu to Desktop"), AddMenuToDesktop);
                else
                  m_popupMenu->insertItem(SmallIconSet("desktop"),
                      i18n("Add Item to Desktop"), AddItemToDesktop);
            }
            if (kapp->authorizeTDEAction("kicker_rmb") && !Kicker::the()->isImmutable())
            {
                hasEntries = true;
                if (m_popupPath.menuPath.endsWith("/"))
                   m_popupMenu->insertItem(SmallIconSet("kicker"),
                      i18n("Add Menu to Main Panel"), AddMenuToPanel);
                else
                   m_popupMenu->insertItem(SmallIconSet("kicker"),
                      i18n("Add Item to Main Panel"), AddItemToPanel);
            }
            if (kapp->authorizeTDEAction("menuedit") && !kitem->menuPath().isEmpty())
            {
                hasEntries = true;
                if (kitem->menuPath().endsWith("/"))
                  m_popupMenu->insertItem(SmallIconSet("kmenuedit"), i18n("Edit Menu"), EditMenu);
                else
                  m_popupMenu->insertItem(SmallIconSet("kmenuedit"), i18n("Edit Item"), EditItem);
            }
            if (kapp->authorize("run_command") && (m_popupService || (!m_popupPath.menuPath.isEmpty() && !m_popupPath.menuPath.endsWith("/"))))
            {
               hasEntries = true;
               m_popupMenu->insertItem(SmallIconSet("system-run"),
               i18n("Put Into Run Dialog"), PutIntoRunDialog);
            }
        }
        if (TQT_BASE_OBJECT_CONST(source)==TQT_BASE_OBJECT(m_searchResultsWidget) || ((TQT_BASE_OBJECT_CONST(source)==TQT_BASE_OBJECT(m_favoriteView) || TQT_BASE_OBJECT_CONST(source)==TQT_BASE_OBJECT(m_recentlyView) || TQT_BASE_OBJECT_CONST(source) == TQT_BASE_OBJECT(m_systemView)) && !m_popupService && !m_popupPath.path.startsWith("kicker:/")) ) {
            TQString uri;
            if (m_popupService)
                uri = locate("apps", m_popupService->desktopEntryPath());
            else
                uri = m_popupPath.path;

            TQString mimetype = TQString();
            if ( m_popupPath.path.startsWith( "system:/media/" ) )
                mimetype = media_mimetypes[m_popupPath.path];

            KFileItem* item = new KFileItem(uri, mimetype, KFileItem::Unknown);
            _items.append( item );

            const KURL kurl(uri);
            TDEActionCollection act(this);

            KonqPopupMenu * konqPopupMenu = new KonqPopupMenu( KonqBookmarkManager::self(), _items,
                                                   kurl, act, (KNewMenu*)NULL, this,
                                                   item->isLocalFile() ? KonqPopupMenu::ShowProperties : KonqPopupMenu::NoFlags,
                                                   KParts::BrowserExtension::DefaultPopupItems );

            if (konqPopupMenu->count()) {
                if (hasEntries) {
                    m_popupMenu->insertSeparator();
                    m_popupMenu->insertItem(SmallIconSet("add"),i18n("Advanced"), konqPopupMenu);
                }
                else {
                    delete m_popupMenu;
                    m_popupMenu = (TDEPopupMenu*)konqPopupMenu;
                    m_popupMenu->insertTitle(SmallIcon(kitem->icon()),kitem->title(),-1,0);
                }
                hasEntries = true;
            }
        }
    }

    if (TQT_BASE_OBJECT_CONST(source)==TQT_BASE_OBJECT(m_recentlyView)) {
       m_popupMenu->insertSeparator();
       if (m_popupService)
         m_popupMenu->insertItem(SmallIconSet("history_clear"),
                 i18n("Clear Recently Used Applications"), ClearRecentlyUsedApps);
       else
         m_popupMenu->insertItem(SmallIconSet("history_clear"),
                 i18n("Clear Recently Used Documents"), ClearRecentlyUsedDocs);
    }

    if (hasEntries) {
       m_isShowing = true;
       m_popupMenu->exec(pos);
       m_isShowing = false;
    }

    delete m_popupMenu;
    m_popupMenu = 0;
}

void KMenu::slotContextMenu(int selected)
{
    KServiceGroup::Ptr g;
    TQByteArray ba;
    TQDataStream ds(ba, IO_WriteOnly);

    KURL src,dest;
    TDEIO::CopyJob *job;

    TDEProcess *proc;

    TQStringList favs = KickerSettings::favorites();

    switch (selected) {
        case AddItemToDesktop:
            accept();
	    if (m_popupService) {
	        src.setPath( TDEGlobal::dirs()->findResource( "apps", m_popupService->desktopEntryPath() ) );
	        dest.setPath( TDEGlobalSettings::desktopPath() );
	        dest.setFileName( src.fileName() );

                job = TDEIO::copyAs( src, dest );
                job->setDefaultPermissions( true );
 	    }
            else {
		KDesktopFile* df = new KDesktopFile( newDesktopFile(KURL(m_popupPath.path), TDEGlobalSettings::desktopPath() ) );
		df->writeEntry("GenericName", m_popupPath.description);
		df->writeEntry( "Icon", m_popupPath.icon );
		df->writePathEntry( "URL", m_popupPath.path );
		df->writeEntry( "Name", m_popupPath.title );
		df->writeEntry( "Type", "Link" );
		df->sync();
		delete df;
            }
            accept();
	    break;

	case AddItemToPanel:
            accept();
	    if (m_popupService)
	    	kapp->dcopClient()->send("kicker", "Panel", "addServiceButton(TQString)", m_popupService->desktopEntryPath());
            else
#warning FIXME special RecentDocuments/foo.desktop handling
	    	kapp->dcopClient()->send("kicker", "Panel", "addURLButton(TQString)", m_popupPath.path);
	    accept();
	    break;

	case EditItem:
        case EditMenu:
	    accept();
            proc = new TDEProcess(TQT_TQOBJECT(this));
            *proc << TDEStandardDirs::findExe(TQString::fromLatin1("kmenuedit"));
            *proc << "/"+m_popupPath.menuPath.section('/',-200,-2) << m_popupPath.menuPath.section('/', -1);
            proc->start();
	    break;

	case PutIntoRunDialog:
	    accept();
	    if (m_popupService)
	      kapp->dcopClient()->send("kdesktop", "default", "popupExecuteCommand(TQString)", m_popupService->exec());
	    else
#warning FIXME special RecentDocuments/foo.desktop handling
              kapp->dcopClient()->send("kdesktop", "default", "popupExecuteCommand(TQString)", m_popupPath.path);
            accept();
	    break;

	case AddMenuToDesktop: {
	    accept();
	    KDesktopFile *df = new KDesktopFile( newDesktopFile(KURL("programs:/"+m_popupPath.menuPath),TDEGlobalSettings::desktopPath()));
            df->writeEntry( "Icon", m_popupPath.icon );
            df->writePathEntry( "URL", "programs:/"+m_popupPath.menuPath );
	    df->writeEntry( "Name", m_popupPath.title );
	    df->writeEntry( "Type", "Link" );
            df->sync();
	    delete df;

	    break;
            }
	case AddMenuToPanel:
	    accept();
            ds << "foo" << m_popupPath.menuPath;
	    kapp->dcopClient()->send("kicker", "Panel", "addServiceMenuButton(TQString,TQString)", ba);
	    break;

        case AddToFavorites:
	    if (m_popupService) {
              if (favs.find(m_popupService->storageId())==favs.end()) {
                KService::Ptr p = KService::serviceByStorageId(m_popupService->storageId());
                m_favoriteView->insertMenuItem(p, serviceMenuEndId()+favs.count()+1);
                favs+=m_popupService->storageId();
              }
            }
            else {
               TQStringList::Iterator it;
               for (it = favs.begin(); it != favs.end(); ++it) {
                  if ((*it)[0]=='/') {
                     KDesktopFile df((*it),true);
                     if (df.readURL().replace("file://",TQString())==m_popupPath.path)
                        break;
                  }
               }
               if (it==favs.end()) {
                 TQString file = KickerLib::newDesktopFile(m_popupPath.path);
                 KDesktopFile df(file);
                 df.writeEntry("Encoding", "UTF-8");
                 df.writeEntry("Type","Link");
                 df.writeEntry("Name", m_popupPath.title);
                 df.writeEntry("GenericName", m_popupPath.description);
                 df.writeEntry("Icon", m_popupPath.icon);
                 df.writeEntry("URL", m_popupPath.path);

                 m_favoriteView->insertItem(m_popupPath.icon, m_popupPath.title, m_popupPath.description,
                    m_popupPath.path, serviceMenuEndId()+favs.count()+1, -1);

                 favs+=file;
               }
            }
            KickerSettings::setFavorites(favs);
            KickerSettings::writeConfig();
            m_browserDirty=true;
            m_stacker->raiseWidget(FavoriteTab);
	    break;

        case RemoveFromFavorites:
	    if (m_popupService) {
              favs.erase(favs.find(m_popupService->storageId()));

              for (TQListViewItemIterator it(m_favoriteView); it.current(); ++it) {
                 KMenuItem* kitem = static_cast<KMenuItem*>(it.current());
	         if (kitem->service() && kitem->service()->storageId() == m_popupService->storageId()) {
                   delete it.current();
                   break;
                 }
              }
            }
            else {
               for (TQStringList::Iterator it = favs.begin(); it != favs.end(); ++it) {
                  if ((*it)[0]=='/') {
                     KDesktopFile df((*it),true);
                     if (df.readURL().replace("file://",TQString())==m_popupPath.path) {
			TQFile::remove((*it));
                        favs.erase(it);
                        break;
                     }
                  }
               }
               for (TQListViewItemIterator it(m_favoriteView); it.current(); ++it) {
                  KMenuItem* kitem = static_cast<KMenuItem*>(it.current());
	          if (!kitem->service() && kitem->path() == m_popupPath.path) {
                     delete it.current();
                     break;
                  }
               }
            }
	    m_favoriteView->slotMoveContent();
            KickerSettings::setFavorites(favs);
            KickerSettings::writeConfig();
            m_browserDirty=true;
            m_stacker->raiseWidget(FavoriteTab);
	    break;

        case ClearRecentlyUsedApps:
            clearRecentAppsItems();
	    break;

        case ClearRecentlyUsedDocs:
            clearRecentDocsItems();
	    break;

	default:
	    break;
	}
}

void KMenu::resizeEvent ( TQResizeEvent * e )
{
    //kdDebug() << "resizeEvent " << size() << endl;
    KMenuBase::resizeEvent(e);
    int ypos = 0;
    // this is the height remaining to fill
    int left_height = height();

    if ( m_orientation == BottomUp )
    {
        m_resizeHandle->move( e->size().width() - 19, 3);

        // put the search widget at the top of the menu and give it its desired
        // height
        m_search->mainWidget()->setGeometry( 0, ypos, width(),
                m_search->minimumSize().height() );
        left_height -= m_search->minimumSize().height();
        ypos += m_search->minimumSize().height();

        // place the footer widget at the bottom of the menu and give it its desired
        // height
        m_footer->mainWidget()->setGeometry( 0, height() - m_footer->minimumSize().height(),
                width(), m_footer->minimumSize().height() );
        left_height -= m_footer->minimumSize().height();

        // place the button box above the footer widget, horizontal placement
        // has the width of the edge graphics subtracted
        m_tabBar->setGeometry(button_box_left.width(),
                height() - m_footer->minimumSize().height() -
                m_tabBar->sizeHint().height(),
                width() - button_box_left.width(),
                m_tabBar->sizeHint().height() );
        left_height -= m_tabBar->sizeHint().height();

        // place the main (stacker) widget below the search widget,
        // in the remaining vertical space
        m_stacker->setGeometry(0, ypos,
                width(),
                left_height );

    }
    else // TopDown orientation
    {
        // place the 'footer' widget at the top of the menu and give it
        // its desired height
        m_footer->mainWidget()->setGeometry( 0,
                ypos /*height() - m_footer->minimumSize().height()*/,
                width(),
                m_footer->minimumSize().height() );
        ypos += m_footer->minimumSize().height();
        left_height -= m_footer->minimumSize().height();

        // place the button box next at the top of the menu.
        // has the width of the edge graphics subtracted
        m_tabBar->setGeometry(button_box_left.width(), ypos, width() - button_box_left.width(),
                m_tabBar->sizeHint().height());

        ypos += m_tabBar->sizeHint().height();
        left_height -= m_tabBar->sizeHint().height();

        // put the search widget above the footer widget
        // height
        m_search->mainWidget()->setGeometry( 0,
                height() - m_search->minimumSize().height(),
                width(),
                m_search->minimumSize().height()
                );
        left_height -= m_search->minimumSize().height();

        // place the main (stacker) widget below the button box,
        // in the remaining vertical space
        m_stacker->setGeometry(0, ypos,
                width(),
                left_height );
        m_resizeHandle->move( e->size().width() - 19, e->size().height() - 19);
    }
    paintSearchTab( false );
}

void KMenu::mousePressEvent ( TQMouseEvent * e )
{
    if ( m_orientation == BottomUp ) {
       if (e->x() > width() - m_resizeHandle->width() &&
          e->y() < m_resizeHandle->height() )
       {
            m_isresizing = true;
       }
    }
    else {
       if (e->x() > width() - m_resizeHandle->width() &&
          e->y() > height() - m_resizeHandle->height() )
       {
            m_isresizing = true;
       }
    }
    KMenuBase::mousePressEvent(e);
}

void KMenu::mouseReleaseEvent ( TQMouseEvent * /*e*/ )
{
    m_isresizing = false;
}

void KMenu::mouseMoveEvent ( TQMouseEvent * e )
{
    if ( hasMouseTracking() && m_isresizing ) {
        m_stacker->setMinimumSize( TQSize(0, 0) );
        m_stacker->setMaximumSize( TQSize(32000, 32000) );
        int newWidth = TQMAX( e->x() - x(), minimumSizeHint().width() );
        if ( m_orientation == BottomUp ) {
          int newHeight = TQMAX( height() - e->y(), minimumSizeHint().height() + 10 );
          int newY = y() + height() - newHeight;
          setGeometry( x(), newY, newWidth, newHeight);
        }
        else {
          setGeometry( x(), y(), newWidth, TQMAX( e->y(), minimumSizeHint().height() + 10 ));
        }
    }
}

void KMenu::clearedHistory()
{
    saveConfig();
}

void KMenu::saveConfig()
{
    KickerSettings::setHistory( m_kcommand->historyItems() );
    KickerSettings::setCompletionItems( m_kcommand->completionObject()->items() );
    KickerSettings::writeConfig();
}

void KMenu::notifyServiceStarted(KService::Ptr service)
{
    // Inform other applications (like the quickstarter applet)
    // that an application was started
    TQByteArray params;
    TQDataStream stream(params, IO_WriteOnly);
    stream << "minicli" << service->storageId();
    kdDebug() << "minicli appLauncher dcop signal: " << service->storageId() << endl;
    TDEApplication::kApplication()->dcopClient()->emitDCOPSignal("appLauncher",
        "serviceStartedByStorageId(TQString,TQString)", params);
}

void KMenu::parseLine( bool final )
{
  TQString cmd = m_kcommand->currentText().stripWhiteSpace();
  m_filterData->setData( cmd );

  if( final )
    KURIFilter::self()->filterURI( *(m_filterData), m_finalFilters );
  else
    KURIFilter::self()->filterURI( *(m_filterData), m_middleFilters );

  m_iconName = m_filterData->iconName();

  kdDebug (1207) << "Command: " << m_filterData->uri().url() << endl;
  kdDebug (1207) << "Arguments: " << m_filterData->argsAndOptions() << endl;
}

// report error as a title in the menu
void KMenu::reportError (TQString error)
{
    int index = 1000; //getHitMenuItemPosition (new HitMenuItem (base_category_id[0], 0));
    kndDebug () << "Inserting error:" << error << " at position " << index << endl;
    m_searchResultsWidget->insertSeparator(OTHER_ID_BASE + 120, error, index);
}

int KMenu::getHitMenuItemPosition ( HitMenuItem *hit_item)
{
    TQPtrListIterator<HitMenuItem> it (m_current_menu_items);
    const HitMenuItem *cur_item;
    int pos = 0;
    while ((cur_item = it.current ()) != NULL) {
	++it;
	if ((cur_item->category!=hit_item->category || !cur_item->display_name.isEmpty()) && (*hit_item) < (*cur_item))
	    break;
	pos++;
    }
    m_current_menu_items.insert (pos, hit_item);

    return pos + 1;
}

bool KMenu::checkUriInMenu( const KURL &uri)
{
    TQPtrListIterator<HitMenuItem> it (m_current_menu_items);
    const HitMenuItem *cur_item;
    while ((cur_item = it.current ()) != NULL) {
	++it;
	if (cur_item->uri == uri )
	    return true;
    }

    return false;
}

void KMenu::searchActionClicked(TQListViewItem* item)
{
   accept();

   addToHistory();
   if (item==m_searchIndex) {
     TQByteArray data;
     TQDataStream arg(data, IO_WriteOnly);
     arg << m_kcommand->currentText();

     if (ensureServiceRunning("kerry"))
       kapp->dcopClient()->send("kerry","search","search(TQString)", data);
   }
   else {
     KURIFilterData data;
     TQStringList list;
     data.setData( m_kcommand->currentText() );
     list << "kurisearchfilter" << "kuriikwsfilter";

     if( !KURIFilter::self()->filterURI(data, list) ) {
         KDesktopFile file("searchproviders/google.desktop", true, "services");
         data.setData(file.readEntry("Query").replace("\\{@}", m_kcommand->currentText()));
     }

     (void) new KRun( data.uri(), parentWidget());
   }
}

void KMenu::addToHistory()
{
  TQString search = m_kcommand->currentText().stripWhiteSpace();

  if (search.length()<4)
    return;

  m_kcommand->addToHistory( search );
}

TQString KMenu::newDesktopFile(const KURL& url, const TQString &directory)
{
   TQString base = url.fileName();
   if (base.endsWith(".desktop"))
      base.truncate(base.length()-8);
   TQRegExp r("(.*)(?=-\\d+)");
   if (r.search(base) > -1)
      base = r.cap(1);

   TQString file = base + ".desktop";

   for(int n = 1; ++n; )
   {
      if (!TQFile::exists(directory+file))
         break;

      file = TQString("%2-%1.desktop").arg(n).arg(base);
   }
   return directory+file;
}

void KMenu::updateRecentlyUsedApps(KService::Ptr &service)
{
    TQString strItem(service->desktopEntryPath());

    // don't add an item from root kmenu level
    if (!strItem.contains('/'))
    {
        return;
    }

    // add it into recent apps list
    RecentlyLaunchedApps::the().appLaunched(strItem);
    RecentlyLaunchedApps::the().save();
    RecentlyLaunchedApps::the().m_bNeedToUpdate = true;
}

TQSize KMenu::sizeHint() const
{
#warning FIXME
    // this should be only for the inner area so layout changes do not break it
    const int width = kMin(KickerSettings::kMenuWidth(), TQApplication::desktop()->screen()->width()-50);

    const int height = kMin(KickerSettings::kMenuHeight(), TQApplication::desktop()->screen()->height()-50);
    TQSize wanted(width, height);
    kdDebug() << "show " << minimumSizeHint() << " " << m_stacker->minimumSizeHint() << " "
              << m_searchFrame->minimumSizeHint() << " " << wanted << endl;
    bool isDefault = wanted.isNull();
    wanted = wanted.expandedTo(minimumSizeHint());
    if ( isDefault )
        wanted.setHeight( wanted.height() + ( m_favoriteView->goodHeight() - m_stacker->minimumSizeHint().height() ) );

    return wanted;
}

TQSize KMenu::minimumSizeHint() const
{
    TQSize minsize;
    minsize.setWidth( minsize.width() + m_tabBar->minimumSizeHint().width()  );
    minsize.setWidth( TQMAX( minsize.width(),
                            m_search->minimumSize().width() ) );
    minsize.setWidth( TQMAX( minsize.width(),
                            m_search->minimumSize().width() ) );

    minsize.setHeight( minsize.height() +
                       m_search->minimumSize().height() +
                       m_footer->minimumSize().height() +
                       180 ); // 180 is a very rough guess for 32 icon size
    return minsize;
}

void KMenu::slotFavoritesMoved( TQListViewItem* item, TQListViewItem* /*afterFirst*/, TQListViewItem* afterNow)
{
    KMenuItem* kitem = dynamic_cast<KMenuItem*>(item);
    KMenuItem* kafterNow = dynamic_cast<KMenuItem*>(afterNow);

    TQStringList favs = KickerSettings::favorites();
    TQStringList::Iterator it;
    TQString addFav = TQString();

    // remove at old position
    if (kitem->service())
    {
        favs.erase(favs.find(kitem->service()->storageId()));
        addFav = kitem->service()->storageId();
    }
    else
    {
        for (it = favs.begin(); it != favs.end(); ++it)
        {
            if ((*it)[0]=='/')
            {
                KDesktopFile df((*it),true);
                if (df.readURL().replace("file://",TQString())==kitem->path())
                {
                    addFav = *it;
                    favs.erase(it);
                    break;
                }
            }
        }
    }

    if (addFav.isEmpty())
      return;

    if (!kafterNow || dynamic_cast<KMenuSpacer*>(afterNow))
    {
        favs.prepend(addFav);
    }
    else
    {
        // add at new position
        for (it = favs.begin(); it != favs.end(); ++it)
        {
            if ((*it)[0]=='/' && !kafterNow->service())
            {
                KDesktopFile df((*it),true);
                if (df.readURL().replace("file://",TQString())==kafterNow->path())
                {
                    kdDebug() << "insert after " << kafterNow->path() << endl;
                    favs.insert(++it,addFav);
                    break;
                }
            }
            else if (kafterNow->service() && *it==kafterNow->service()->storageId())
            {
                kdDebug() << "insert after service " << kafterNow->service() << endl;
                favs.insert(++it,addFav);
                break;
            }
        }
    }
    kdDebug() << "favs " << favs << endl;

    KickerSettings::setFavorites(favs);
    KickerSettings::writeConfig();

    m_favoriteView->slotMoveContent();
}

void KMenu::updateMedia()
{
    TQStringList devices = m_mediaWatcher->devices();
    if ( devices.isEmpty() ) {
        return;
    }

    int nId = serviceMenuStartId();
    if ( m_media_id ) {
        for ( int i = m_media_id + 1 ;; ++i )
        {
            KMenuItem *item = m_systemView->findItem( i );
            if ( !item )
                break;
            if ( !item->path().startsWith( "system:/" ) )
                break;
            media_mimetypes.remove(item->path());
            delete item;
        }
        nId = m_media_id + 1;
    } else {
        m_media_id = nId;
        m_systemView->insertSeparator( nId++, i18n("Media"), -1);
    }

    // WARNING
    // This loop MUST be kept in sync with the data structure listed in libmediacommon/medium.h
    #define SAFE_INCREMENT it++; if (it == devices.constEnd()) { printf("[kicker] Warning: incompatible media device list encountered!\n"); break; }
    for ( TQStringList::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it )
    {
        TQString id = *it;
        SAFE_INCREMENT
        TQString uuid = *it;
        SAFE_INCREMENT
        TQString name = *it;
        SAFE_INCREMENT
        TQString label = *it;
        SAFE_INCREMENT
        TQString userLabel = *it;
        SAFE_INCREMENT
        bool mountable = ( *it == "true" ); // bool
        SAFE_INCREMENT
        TQString deviceNode = ( *it );
        SAFE_INCREMENT
        TQString mountPoint = ( *it );
        SAFE_INCREMENT
        TQString fsType = ( *it );
        SAFE_INCREMENT
        bool mounted = ( *it == "true" ); // bool
        SAFE_INCREMENT
        TQString baseURL = ( *it );
        SAFE_INCREMENT
        TQString mimeType = ( *it );
        SAFE_INCREMENT
        TQString iconName = ( *it );
        SAFE_INCREMENT
        bool encrypted = ( *it == "true" ); // bool
        SAFE_INCREMENT
        TQString clearDeviceUDI = ( *it );
        SAFE_INCREMENT
        bool hidden = ( *it == "true" ); // bool

        media_mimetypes["system:/media/"+name] = mimeType;

        if ( iconName.isEmpty() ) // no user icon, query the MIME type
        {
            KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
            iconName = mime->icon( TQString(), false );
        }

        TQString descr = deviceNode;
        if ( mounted )
        {
            descr = mountPoint;
            // calc the free/total space
            struct statfs sfs;
            if ( statfs( TQFile::encodeName( mountPoint ), &sfs ) == 0 )
            {
                uint64_t total = ( uint64_t )sfs.f_blocks * sfs.f_bsize;
                uint64_t avail = ( uint64_t )( getuid() ? sfs.f_bavail : sfs.f_bfree ) * sfs.f_bsize;
                if ( avail < total && avail > 1024 ) {
                    label += " " + i18n( "(%1 available)" ).arg( TDEIO::convertSize(avail) );
                }
            }
        }
        m_systemView->insertItem( iconName, userLabel.isEmpty() ? label : userLabel,
                                  descr, "system:/media/" + name, nId++, -1 );
        SAFE_INCREMENT // skip separator
    }
    #undef SAFE_INCREMENT
}

bool KMenu::ensureServiceRunning(const TQString & service)
{
    TQStringList URLs;
    TQByteArray data, replyData;
    TQCString replyType;
    TQDataStream arg(data, IO_WriteOnly);
    arg << service << URLs;

    if ( !kapp->dcopClient()->call( "tdelauncher", "tdelauncher", "start_service_by_desktop_name(TQString,TQStringList)",
                      data, replyType, replyData) ) {
        tqWarning( "call to tdelauncher failed.");
        return false;
    }
    TQDataStream reply(replyData, IO_ReadOnly);

    if ( replyType != "serviceResult" )
    {
        tqWarning( "unexpected result '%s' from tdelauncher.", replyType.data());
        return false;
    }
    int result;
    TQCString dcopName;
    TQString error;
    reply >> result >> dcopName >> error;
    if (result != 0)
    {
        tqWarning("Error starting: %s", error.local8Bit().data());
        return false;
    }
    return true;
}

void KMenu::slotFavDropped(TQDropEvent * ev, TQListViewItem *after )
{
    TQStringList favs = KickerSettings::favorites();
    KMenuItem *newItem = 0;

    if (KMenuItemDrag::canDecode(ev))
    {
        KMenuItemInfo item;
        KMenuItemDrag::decode(ev,item);

        if (item.m_s)
        {
            if (favs.find(item.m_s->storageId())==favs.end())
            {
                newItem = m_favoriteView->insertMenuItem(item.m_s, serviceMenuEndId()+favs.count()+1);
                favs += item.m_s->storageId();
            }
        }
        else
        {
            TQString uri = item.m_path;
            if (uri.startsWith(locateLocal("data", TQString::fromLatin1("RecentDocuments/")))) {
               KDesktopFile df(uri,true);
               uri=df.readURL();
            }

            TQStringList::Iterator it;
            for (it = favs.begin(); it != favs.end(); ++it)
            {
                if ((*it)[0]=='/')
                {
                    KDesktopFile df((*it),true);
                    if (df.readURL().replace("file://",TQString())==uri)
                        break;
                }
            }
            if (it==favs.end())
            {
                TQString file = KickerLib::newDesktopFile(uri);
                KDesktopFile df(file);
                df.writeEntry("Encoding", "UTF-8");
                df.writeEntry("Type","Link");
                df.writeEntry("Name", item.m_title);
                df.writeEntry("GenericName", item.m_description);
                df.writeEntry("Icon", item.m_icon);
                df.writeEntry("URL", uri);

                newItem = m_favoriteView->insertItem(item.m_icon, item.m_title, item.m_description,
                                                     uri, serviceMenuEndId()+favs.count()+1, -1);
                favs += file;
            }
        }
    }
    else if (TQTextDrag::canDecode(ev))
    {
        TQString text;
        TQTextDrag::decode(ev,text);

        if (text.endsWith(".desktop"))
        {
            KService::Ptr p = KService::serviceByDesktopPath(text.replace("file://",TQString()));
            if (p && favs.find(p->storageId())==favs.end()) {
                newItem = m_favoriteView->insertMenuItem(p, serviceMenuEndId()+favs.count()+1);
                favs+=p->storageId();
            }
        }
        else
        {
            TQStringList::Iterator it;
            for (it = favs.begin(); it != favs.end(); ++it)
            {
                if ((*it)[0]=='/')
                {
                    KDesktopFile df((*it),true);
                    if (df.readURL().replace("file://",TQString())==text)
                        break;
                }
            }
            if (it==favs.end())
            {
                KFileItem* item = new KFileItem(text, TQString(), KFileItem::Unknown);
                KURL kurl(text);

                TQString file = KickerLib::newDesktopFile(text);
                KDesktopFile df(file);
                df.writeEntry("Encoding", "UTF-8");
                df.writeEntry("Type","Link");
                df.writeEntry("Name", item->name());
                df.writeEntry("GenericName", i18n("Directory: %1").arg(kurl.upURL().path()));
                df.writeEntry("Icon", item->iconName());
                df.writeEntry("URL", text);

                newItem = m_favoriteView->insertItem(item->iconName(), item->name(), i18n("Directory: %1").arg(kurl.upURL().path()), text, serviceMenuEndId()+favs.count()+1, -1);
                favs += file;
            }
        }
    }

    if ( newItem ) {
        if (!after && m_favoriteView->childCount()>0) {
            newItem->moveItem( m_favoriteView->firstChild() );
            m_favoriteView->firstChild()->moveItem( newItem );
        }
        else
            newItem->moveItem( after );
        KickerSettings::setFavorites(favs);
        slotFavoritesMoved( newItem, 0, after );
    }
    m_stacker->raiseWidget(m_favoriteView);
}

void KMenu::resetOverflowCategory()
{
   if (m_overflowCategoryState==NotNeeded)
      m_overflowList.setAutoDelete( true );

   m_overflowList.clear();
   m_overflowList.setAutoDelete( false );
   m_overflowCategoryState = None;
   m_overflowCategory = num_categories;
}

void KMenu::fillOverflowCategory()
{
   if (m_overflowCategoryState==Filling) {
      initCategoryTitlesUpdate();
      for (HitMenuItem * item = m_overflowList.first(); item; item = m_overflowList.next() ) {
	  max_category_id [item->category]++;
          item->id=max_category_id [item->category];

          KMenuItem *hit_item = m_searchResultsWidget->insertItem(iconForHitMenuItem(item), item->display_name, item->display_info, item->uri.url(), max_category_id [item->category], getHitMenuItemPosition (item));
          hit_item->setService(item->service);
      }
      updateCategoryTitles();
  }
}

int KMenu::max_items(int category) const
{
    if (category==ACTIONS)
      return 10;

    return 5;
}

void KMenu::insertSuspendOption( int &nId, int &index )
{
    bool suspend_ram = false;
    bool suspend_freeze = false;
    bool standby = false;
    bool suspend_disk = false;
#if defined(COMPILE_HALBACKEND)
    suspend_ram = libhal_device_get_property_bool(m_halCtx,
        "/org/freedesktop/Hal/devices/computer",
        "power_management.can_suspend",
        NULL);

    standby = libhal_device_get_property_bool(m_halCtx,
        "/org/freedesktop/Hal/devices/computer",
        "power_management.can_standby",
        NULL);

    suspend_disk = libhal_device_get_property_bool(m_halCtx,
        "/org/freedesktop/Hal/devices/computer",
        "power_management.can_hibernate",
        NULL);
#elif defined(__TDE_HAVE_TDEHWLIB) // COMPILE_HALBACKEND
    TDERootSystemDevice* rootDevice = TDEGlobal::hardwareDevices()->rootSystemDevice();
    if (rootDevice) {
        suspend_ram = rootDevice->canSuspend();
        suspend_freeze = rootDevice->canFreeze();
        standby = rootDevice->canStandby();
        suspend_disk = rootDevice->canHibernate();
    }
#endif

    // respect disable suspend/hibernate settings from power-manager
    TDEConfig config("power-managerrc");
    bool disableSuspend = config.readBoolEntry("disableSuspend", false);
    bool disableHibernate = config.readBoolEntry("disableHibernate", false);

    if ( suspend_disk && !disableHibernate ) {
        m_exitView->leftView()->insertItem(
            "suspend2disk",
            i18n( "Suspend to Disk" ),
            i18n( "Pause without logging out" ),
            "kicker:/suspend_disk", nId++, index++ );
    }

    if ( suspend_ram && !disableSuspend ) {
        m_exitView->leftView()->insertItem(
            "suspend2ram",
            i18n( "Suspend to RAM" ),
            i18n( "Pause without logging out" ),
            "kicker:/suspend_ram", nId++, index++ );
    }

    if ( suspend_freeze && !disableSuspend ) {
        m_exitView->leftView()->insertItem(
            "suspend2ram",
            i18n( "Freeze" ),
            i18n( "Pause without logging out" ),
            "kicker:/suspend_freeze", nId++, index++ );
    }

    if ( standby && !disableSuspend ) {
        m_exitView->leftView()->insertItem(
            "media-playback-pause",
            i18n( "Standby" ),
            i18n( "Pause without logging out" ),
            "kicker:/standby", nId++, index++ );
    }
}

void KMenu::slotSuspend(int id)
{
    bool error = true;

    // respect lock on resume settings from power-manager
    TDEConfig config("power-managerrc");
    bool lockOnResume = config.readBoolEntry("lockOnResume", true);
    if (lockOnResume) {
	// Block here until lock is complete
	// If this is not done the desktop of the locked session will be shown after suspend/hibernate until the lock fully engages!
        DCOPRef("kdesktop", "KScreensaverIface").call("lock()");
    }

#if defined(COMPILE_HALBACKEND)
    DBusMessage* msg = NULL;

    if (m_dbusConn) {
        if (id == 1) {
            msg = dbus_message_new_method_call(
                              "org.freedesktop.Hal",
                              "/org/freedesktop/Hal/devices/computer",
                              "org.freedesktop.Hal.Device.SystemPowerManagement",
                              "Hibernate");
        } else if (id == 2) {
            msg = dbus_message_new_method_call(
                              "org.freedesktop.Hal",
                              "/org/freedesktop/Hal/devices/computer",
                              "org.freedesktop.Hal.Device.SystemPowerManagement",
                              "Suspend");
            int wakeup=0;
            dbus_message_append_args(msg, DBUS_TYPE_INT32, &wakeup, DBUS_TYPE_INVALID);
        } else if (id == 3) {
            msg = dbus_message_new_method_call(
                              "org.freedesktop.Hal",
                              "/org/freedesktop/Hal/devices/computer",
                              "org.freedesktop.Hal.Device.SystemPowerManagement",
                              "Standby");
        } else {
            return;
        }

        if(dbus_connection_send(m_dbusConn, msg, NULL)) {
            error = false;
        }
        dbus_message_unref(msg);
    }
#elif defined(__TDE_HAVE_TDEHWLIB) // COMPILE_HALBACKEND
    TDERootSystemDevice* rootDevice = TDEGlobal::hardwareDevices()->rootSystemDevice();
    if (rootDevice) {
        if (id == 1) {
            error = !rootDevice->setPowerState(TDESystemPowerState::Hibernate);
        } else if (id == 2) {
            error = !rootDevice->setPowerState(TDESystemPowerState::Suspend);
        } else if (id == 3) {
            error = !rootDevice->setPowerState(TDESystemPowerState::Standby);
        } else if (id == 4) {
            error = !rootDevice->setPowerState(TDESystemPowerState::Freeze);
        } else {
            return;
        }
    }
#else
    error = false;
#endif
    if (error) {
        KMessageBox::error(this, i18n("Suspend failed"));
    }
}

void KMenu::runUserCommand()
{
    TQByteArray data;
    TQCString appname( "kdesktop" );
    if ( kicker_screen_number )
        appname.sprintf("kdesktop-screen-%d", kicker_screen_number);

    kapp->updateRemoteUserTimestamp( appname );
    kapp->dcopClient()->send( appname, "KDesktopIface",
                              "popupExecuteCommand()", data );
}

// vim:cindent:sw=4:
