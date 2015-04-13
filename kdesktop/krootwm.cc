/*
 * krootwm.cc Part of the KDE project.
 *
 * Copyright (C) 1997 Matthias Ettrich
 *           (C) 1997 Torben Weis, weis@kde.org
 *           (C) 1998 S.u.S.E. weis@suse.de

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <kprocess.h>
#include <kstandarddirs.h>
#include <tdepopupmenu.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kcmultidialog.h>
#include <kbookmarkmenu.h>
#include <konqbookmarkmanager.h>
#include <tdelocale.h>
#include <knewmenu.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <khelpmenu.h>
#include <kdebug.h>
#include <twindowlistmenu.h>
#include <twin.h>
#include <tdemenubar.h>
#include <tdemessagebox.h>
#include <kuser.h>
#include <tqfile.h>
#include <ntqthread.h>
#include <tqeventloop.h>

#include "krootwm.h"
#include "kdiconview.h"
#include "desktop.h"
#include "kcustommenu.h"
#include "kdesktopsettings.h"
#include "lockeng.h"

#include <netwm.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <dmctl.h>

KRootWm * KRootWm::s_rootWm = 0;

extern TQCString kdesktop_name, kicker_name, twin_name;

KRootWm::KRootWm(SaverEngine* _saver, KDesktop* _desktop) : TQObject(_desktop), startup(FALSE)
{
  s_rootWm = this;
  m_actionCollection = new TDEActionCollection(_desktop, this, "KRootWm::m_actionCollection");
  m_pSaver = _saver;
  m_pDesktop = _desktop;
  m_bDesktopEnabled = (m_pDesktop->iconView() != 0);
  customMenu1 = 0;
  customMenu2 = 0;
  m_configDialog = 0;

  // Creates the new menu
  menuBar = 0; // no menubar yet
  menuNew = 0;
  if (m_bDesktopEnabled && kapp->authorize("editable_desktop_icons"))
  {
     menuNew = new KNewMenu( m_actionCollection, "new_menu" );
     connect(menuNew->popupMenu(), TQT_SIGNAL( aboutToShow() ),
             this, TQT_SLOT( slotFileNewAboutToShow() ) );
     connect( menuNew, TQT_SIGNAL( activated() ),
              m_pDesktop->iconView(), TQT_SLOT( slotNewMenuActivated() ) );
  }

  if (kapp->authorizeTDEAction("bookmarks"))
  {
     bookmarks = new TDEActionMenu( i18n("Bookmarks"), "bookmark", m_actionCollection, "bookmarks" );
     // The KBookmarkMenu is needed to fill the Bookmarks menu in the desktop menubar.
     bookmarkMenu = new KBookmarkMenu( KonqBookmarkManager::self(), new KBookmarkOwner(),
                                    bookmarks->popupMenu(),
                                    m_actionCollection,
                                    true, false );
  }
  else
  {
     bookmarks = 0;
     bookmarkMenu = 0;
  }
  
  // The windowList and desktop menus can be part of a menubar (Mac style)
  // so we create them here
  desktopMenu = new TQPopupMenu;
  windowListMenu = new KWindowListMenu;
  connect( windowListMenu, TQT_SIGNAL( aboutToShow() ),
           this, TQT_SLOT( slotWindowListAboutToShow() ) );

  // Create the actions
#if 0
  if (m_bDesktopEnabled)
  {
      // Don't do that! One action in two parent collections means some invalid write
      // during the second ~TDEActionCollection.
     TDEAction *action = m_pDesktop->actionCollection()->action( "paste" );
     if (action)
        m_actionCollection->insert( action );
     action = m_pDesktop->actionCollection()->action( "undo" );
     if (action)
        m_actionCollection->insert( action );
  }
#endif

  if (kapp->authorize("run_command"))
  {
     new TDEAction(i18n("Run Command..."), "system-run", 0, TQT_TQOBJECT(m_pDesktop), TQT_SLOT( slotExecuteCommand() ), m_actionCollection, "exec" );
     new TDEAction(i18n("Open Terminal Here..." ), "terminal", CTRL+Key_T, this, TQT_SLOT( slotOpenTerminal() ),
	m_actionCollection, "open_terminal" );
  }

  if (!TDEGlobal::config()->isImmutable())
  {
     new TDEAction(i18n("Configure Desktop..."), "configure", 0, this, TQT_SLOT( slotConfigureDesktop() ),
                 m_actionCollection, "configdesktop" );
     new TDEAction(i18n("Disable Desktop Menu"), 0, this, TQT_SLOT( slotToggleDesktopMenu() ),
                 m_actionCollection, "togglemenubar" );
  }

  new TDEAction(i18n("Unclutter Windows"), 0, this, TQT_SLOT( slotUnclutterWindows() ),
              m_actionCollection, "unclutter" );
  new TDEAction(i18n("Cascade Windows"), 0, this, TQT_SLOT( slotCascadeWindows() ),
              m_actionCollection, "cascade" );

  // arrange menu actions
  if (m_bDesktopEnabled && kapp->authorize("editable_desktop_icons"))
  {
     new TDEAction(i18n("By Name (Case Sensitive)"), 0, this, TQT_SLOT( slotArrangeByNameCS() ),
                 m_actionCollection, "sort_ncs");
     new TDEAction(i18n("By Name (Case Insensitive)"), 0, this, TQT_SLOT( slotArrangeByNameCI() ),
                 m_actionCollection, "sort_nci");
     new TDEAction(i18n("By Size"), 0, this, TQT_SLOT( slotArrangeBySize() ),
                 m_actionCollection, "sort_size");
     new TDEAction(i18n("By Type"), 0, this, TQT_SLOT( slotArrangeByType() ),
                 m_actionCollection, "sort_type");
     new TDEAction(i18n("By Date"), 0, this, TQT_SLOT( slotArrangeByDate() ),
                 m_actionCollection, "sort_date");

     TDEToggleAction *aSortDirsFirst = new TDEToggleAction( i18n("Directories First"), 0, m_actionCollection, "sort_directoriesfirst" );
     connect( aSortDirsFirst, TQT_SIGNAL( toggled( bool ) ),
              this, TQT_SLOT( slotToggleDirFirst( bool ) ) );
     new TDEAction(i18n("Line Up Horizontally"), 0,
                 this, TQT_SLOT( slotLineupIconsHoriz() ),
                 m_actionCollection, "lineupHoriz" );
     new TDEAction(i18n("Line Up Vertically"), 0,
                 this, TQT_SLOT( slotLineupIconsVert() ),
                 m_actionCollection, "lineupVert" );
     TDEToggleAction *aAutoAlign = new TDEToggleAction(i18n("Align to Grid"), 0,
                 m_actionCollection, "realign" );
     connect( aAutoAlign, TQT_SIGNAL( toggled( bool ) ),
              this, TQT_SLOT( slotToggleAutoAlign( bool ) ) );
     TDEToggleAction *aLockIcons = new TDEToggleAction(i18n("Lock in Place"), 0, m_actionCollection, "lock_icons");
     connect( aLockIcons, TQT_SIGNAL( toggled( bool ) ),
              this, TQT_SLOT( slotToggleLockIcons( bool ) ) );
  }
  if (m_bDesktopEnabled)
  {
     new TDEAction(i18n("Refresh Desktop"), "desktop", 0, this, TQT_SLOT( slotRefreshDesktop() ),
                 m_actionCollection, "refresh" );
  }
  // Icons in sync with kicker
  if (kapp->authorize("lock_screen"))
  {
      new TDEAction(i18n("Lock Session"), "system-lock-screen", 0, this, TQT_SLOT( slotLock() ),
                  m_actionCollection, "lock" );
  }
  if (kapp->authorize("logout"))
  {
      new TDEAction(i18n("Log Out \"%1\"...").arg(KUser().loginName()), "system-log-out", 0,
                  this, TQT_SLOT( slotLogout() ), m_actionCollection, "logout" );
  }

  if (kapp->authorize("start_new_session") && DM().isSwitchable())
  {
      new TDEAction(i18n("Start New Session"), "fork", 0, this,
                  TQT_SLOT( slotNewSession() ), m_actionCollection, "newsession" );
      if (kapp->authorize("lock_screen"))
      {
          new TDEAction(i18n("Lock Current && Start New Session"), "system-lock-screen", 0, this,
                      TQT_SLOT( slotLockNNewSession() ), m_actionCollection, "lockNnewsession" );
      }
  }

  initConfig();
}

KRootWm::~KRootWm()
{
  delete m_actionCollection;
  delete desktopMenu;
  delete windowListMenu;
}

void KRootWm::initConfig()
{
//  kdDebug() << "KRootWm::initConfig" << endl;

  // parse the configuration
  m_bGlobalMenuBar = KDesktopSettings::macStyle();
  m_bShowMenuBar = m_bGlobalMenuBar || KDesktopSettings::showMenubar();

  static const int choiceCount = 7;
  // read configuration for clicks on root window
  static const char * const s_choices[choiceCount] = { "", "WindowListMenu", "DesktopMenu", "AppMenu", "CustomMenu1", "CustomMenu2", "BookmarksMenu" };
  leftButtonChoice = middleButtonChoice = rightButtonChoice = NOTHING;
  TQString s = KDesktopSettings::left();
  for ( int c = 0 ; c < choiceCount ; c ++ )
    if (s == s_choices[c])
      { leftButtonChoice = (menuChoice) c; break; }
  s = KDesktopSettings::middle();
  for ( int c = 0 ; c < choiceCount ; c ++ )
    if (s == s_choices[c])
      { middleButtonChoice = (menuChoice) c; break; }
  s = KDesktopSettings::right();
  for ( int c = 0 ; c < choiceCount ; c ++ )
    if (s == s_choices[c])
      { rightButtonChoice = (menuChoice) c; break; }

  // Read configuration for icons alignment
  if ( m_bDesktopEnabled ) {
    m_pDesktop->iconView()->setAutoAlign( KDesktopSettings::autoLineUpIcons() );
    if ( kapp->authorize( "editable_desktop_icons" ) ) {
        m_pDesktop->iconView()->setItemsMovable( !KDesktopSettings::lockIcons() );
        TDEToggleAction *aLockIcons = static_cast<TDEToggleAction*>(m_actionCollection->action("lock_icons"));
        if (aLockIcons)
            aLockIcons->setChecked( KDesktopSettings::lockIcons()  );
    }
    TDEToggleAction *aAutoAlign = static_cast<TDEToggleAction*>(m_actionCollection->action("realign"));
    if (aAutoAlign)
        aAutoAlign->setChecked( KDesktopSettings::autoLineUpIcons() );
    TDEToggleAction *aSortDirsFirst = static_cast<TDEToggleAction*>(m_actionCollection->action("sort_directoriesfirst"));
    if (aSortDirsFirst)
        aSortDirsFirst->setChecked( KDesktopSettings::sortDirectoriesFirst() );
  }

  buildMenus();
}

void KRootWm::buildMenus()
{
//    kdDebug() << "KRootWm::buildMenus" << endl;

    delete menuBar;
    menuBar = 0;

    delete customMenu1;
    customMenu1 = 0;
    delete customMenu2;
    customMenu2 = 0;

    if (m_bShowMenuBar)
    {
//        kdDebug() << "showMenuBar" << endl;
        menuBar = new KMenuBar;
        menuBar->setCaption("TDE Desktop");
    }

    // create Arrange menu
    TQPopupMenu *pArrangeMenu = 0;
    TQPopupMenu *pLineupMenu = 0;
    TDEAction *action;
    help = new KHelpMenu(0, 0, false);
    help->menu()->removeItem( KHelpMenu::menuAboutApp );

    if (m_bDesktopEnabled && m_actionCollection->action("realign"))
    {
        pArrangeMenu = new TQPopupMenu;
        m_actionCollection->action("sort_ncs")->plug( pArrangeMenu );
        m_actionCollection->action("sort_nci")->plug( pArrangeMenu );
        m_actionCollection->action("sort_size")->plug( pArrangeMenu );
        m_actionCollection->action("sort_type")->plug( pArrangeMenu );
        m_actionCollection->action("sort_date" )->plug( pArrangeMenu );
        pArrangeMenu->insertSeparator();
        m_actionCollection->action("sort_directoriesfirst")->plug( pArrangeMenu );

        pLineupMenu = new TQPopupMenu;
        m_actionCollection->action( "lineupHoriz" )->plug( pLineupMenu );
        m_actionCollection->action( "lineupVert" )->plug( pLineupMenu );
        pLineupMenu->insertSeparator();
        m_actionCollection->action( "realign" )->plug( pLineupMenu );
    }

    sessionsMenu = 0;
    if (m_actionCollection->action("newsession"))
    {
        sessionsMenu = new TQPopupMenu;
        connect( sessionsMenu, TQT_SIGNAL(aboutToShow()), TQT_SLOT(slotPopulateSessions()) );
        connect( sessionsMenu, TQT_SIGNAL(activated(int)), TQT_SLOT(slotSessionActivated(int)) );
    }

    if (menuBar) {
        bool needSeparator = false;
        file = new TQPopupMenu;

        action = m_actionCollection->action("exec");
        if (action)
        {
            action->plug( file );
            file->insertSeparator();
        }

        action = m_actionCollection->action("open_terminal");
        if (action)
        {
            action->plug( file );
        }

        action = m_actionCollection->action("lock");
        if (action)
            action->plug( file );

        action = m_actionCollection->action("logout");
        if (action)
            action->plug( file );

        desk = new TQPopupMenu;

        if (m_bDesktopEnabled)
        {
            m_actionCollection->action("unclutter")->plug( desk );
            m_actionCollection->action("cascade")->plug( desk );
            desk->insertSeparator();

            if (pArrangeMenu)
                desk->insertItem(i18n("Sort Icons"), pArrangeMenu);
            if (pLineupMenu)
                desk->insertItem(i18n("Line Up Icons"), pLineupMenu );

            m_actionCollection->action("refresh")->plug( desk );
            needSeparator = true;
        }
        action = m_actionCollection->action("configdesktop");
        if (action)
        {
           if (needSeparator)
              desk->insertSeparator();
           action->plug( desk );
           needSeparator = true;
        }

        action = m_actionCollection->action("togglemenubar");
        if (action)
        {
           if (needSeparator)
              desk->insertSeparator();
           action->plug( desk );
           action->setText(i18n("Disable Desktop Menu"));
        }
    }
    else
    {
        action = m_actionCollection->action("togglemenubar");
        if (action)
           action->setText(i18n("Enable Desktop Menu"));
    }

    desktopMenu->clear();
    desktopMenu->disconnect( this );
    bool needSeparator = false;

    if (menuNew)
    {
       menuNew->plug( desktopMenu );
       needSeparator = true;
    }

#if 0
    if (bookmarks)
    {
       bookmarks->plug( desktopMenu );
       needSeparator = true;
    }
#endif

    action = m_actionCollection->action("exec");
    if (action)
    {
       action->plug( desktopMenu );
       needSeparator = true;
    }

    action = m_actionCollection->action("open_terminal");
    if (action)
	action->plug( desktopMenu );

    if (needSeparator)
    {
        desktopMenu->insertSeparator();
        needSeparator = false;
    }

    if (m_bDesktopEnabled)
    {
        action = m_pDesktop->actionCollection()->action( "undo" );
        if (action)
           action->plug( desktopMenu );
        action = m_pDesktop->actionCollection()->action( "paste" );
        if (action)
           action->plug( desktopMenu );
        desktopMenu->insertSeparator();
    }

    if (m_bDesktopEnabled && m_actionCollection->action("realign"))
    {
       TQPopupMenu* pIconOperationsMenu = new TQPopupMenu;

       pIconOperationsMenu->insertItem(i18n("Sort Icons"), pArrangeMenu);
       pIconOperationsMenu->insertSeparator();
       m_actionCollection->action( "lineupHoriz" )->plug( pIconOperationsMenu );
       m_actionCollection->action( "lineupVert" )->plug( pIconOperationsMenu );
       pIconOperationsMenu->insertSeparator();
       m_actionCollection->action( "realign" )->plug( pIconOperationsMenu );
       TDEAction *aLockIcons = m_actionCollection->action( "lock_icons" );
       if ( aLockIcons )
           aLockIcons->plug( pIconOperationsMenu );

       desktopMenu->insertItem(SmallIconSet("icons"), i18n("Icons"), pIconOperationsMenu);
    }

    TQPopupMenu* pWindowOperationsMenu = new TQPopupMenu;
    m_actionCollection->action("cascade")->plug( pWindowOperationsMenu );
    m_actionCollection->action("unclutter")->plug( pWindowOperationsMenu );
    desktopMenu->insertItem(SmallIconSet("window_list"), i18n("Windows"), pWindowOperationsMenu);

    if (m_bDesktopEnabled)
    {
       m_actionCollection->action("refresh")->plug( desktopMenu );
    }

    action = m_actionCollection->action("configdesktop");
    if (action)
    {
       action->plug( desktopMenu );
    }
    int lastSep = desktopMenu->insertSeparator();

    if (sessionsMenu && kapp->authorize("switch_user"))
    {
        desktopMenu->insertItem(SmallIconSet("switchuser" ), i18n("Switch User"), sessionsMenu);
        needSeparator = true;
    }

    action = m_actionCollection->action("lock");
    if (action)
    {
        action->plug( desktopMenu );
        needSeparator = true;
    }

    action = m_actionCollection->action("logout");
    if (action)
    {
        action->plug( desktopMenu );
        needSeparator = true;
    }

    if (!needSeparator)
    {
        desktopMenu->removeItem(lastSep);
    }

    connect( desktopMenu, TQT_SIGNAL( aboutToShow() ), this, TQT_SLOT( slotFileNewAboutToShow() ) );

    if (menuBar) {
        menuBar->insertItem(i18n("File"), file);
        if (sessionsMenu)
        {
            menuBar->insertItem(i18n("Sessions"), sessionsMenu);
        }
        if (menuNew)
        {
            menuBar->insertItem(i18n("New"), menuNew->popupMenu());
        }
        if (bookmarks)
        {
            menuBar->insertItem(i18n("Bookmarks"), bookmarks->popupMenu());
        }
        menuBar->insertItem(i18n("Desktop"), desk);
        menuBar->insertItem(i18n("Windows"), windowListMenu);
        menuBar->insertItem(i18n("Help"), help->menu());

        menuBar->setTopLevelMenu( true );
        menuBar->show(); // we need to call show() as we delayed the creation with the timer
    }
}

void KRootWm::slotToggleDirFirst( bool b )
{
    KDesktopSettings::setSortDirectoriesFirst( b );
    KDesktopSettings::writeConfig();
}

void KRootWm::slotToggleAutoAlign( bool b )
{
    KDesktopSettings::setAutoLineUpIcons( b );
    KDesktopSettings::writeConfig();

    // Also save it globally...
    int desktop = TDEApplication::desktop()->primaryScreen();
    TQCString cfilename;
    if (desktop == 0)
        cfilename = "kdesktoprc";
    else
        cfilename.sprintf("kdesktop-screen-%drc", desktop);

    TDEConfig *kdg_config = new TDEConfig(cfilename, false, false);
    kdg_config->setGroup( "General" );
    kdg_config->writeEntry( "AutoLineUpIcons", b );
    kdg_config->sync();
    delete kdg_config;

    // Auto line-up icons
    m_pDesktop->iconView()->setAutoAlign( b );
}

void KRootWm::slotFileNewAboutToShow()
{
  if (menuNew)
  {
//  kdDebug() << " KRootWm:: (" << this << ") slotFileNewAboutToShow() menuNew=" << menuNew << endl;
     // As requested by KNewMenu :
     menuNew->slotCheckUpToDate();
     // And set the files that the menu apply on :
     menuNew->setPopupFiles( m_pDesktop->url() );
  }
}

void KRootWm::slotWindowListAboutToShow()
{
  windowListMenu->init();
}

void KRootWm::activateMenu( menuChoice choice, const TQPoint& global )
{
  switch ( choice )
  {
    case SESSIONSMENU:
      if (sessionsMenu)
          sessionsMenu->popup(global);
      break;
    case WINDOWLISTMENU:
      windowListMenu->popup(global);
      break;
    case DESKTOPMENU:
      m_desktopMenuPosition = global; // for KDIconView::slotPaste
      desktopMenu->popup(global);
      break;
    case BOOKMARKSMENU:
      if (bookmarks)
        bookmarks->popup(global);
      break;
    case APPMENU:
    {
      // This allows the menu to disappear when clicking on the background another time
      XUngrabPointer(tqt_xdisplay(), CurrentTime);
      XSync(tqt_xdisplay(), False);

      // Ask kicker to showup the menu
      DCOPRef( kicker_name, kicker_name ).send( "popupKMenu", global );
      break;
    }
    case CUSTOMMENU1:
      if (!customMenu1)
         customMenu1 = new KCustomMenu("kdesktop_custom_menu1");
      customMenu1->popup(global);
      break;
    case CUSTOMMENU2:
      if (!customMenu2)
         customMenu2 = new KCustomMenu("kdesktop_custom_menu2");
      customMenu2->popup(global);
      break;
    case NOTHING:
    default:
      break;
  }
}

void KRootWm::mousePressed( const TQPoint& _global, int _button )
{
    if (!desktopMenu) return; // initialisation not yet done
    switch ( _button ) {
    case Qt::LeftButton:
        if ( m_bShowMenuBar && menuBar )
            menuBar->raise();
        activateMenu( leftButtonChoice, _global );
        break;
    case Qt::MidButton:
        activateMenu( middleButtonChoice, _global );
        break;
    case Qt::RightButton:
        if (!kapp->authorize("action/kdesktop_rmb")) return;
        activateMenu( rightButtonChoice, _global );
        break;
    default:
        // nothing
        break;
    }
}

void KRootWm::slotWindowList() {
//  kdDebug() << "KRootWm::slotWindowList" << endl;
// Popup at the center of the screen, this is from keyboard shortcut.
  TQDesktopWidget* desktop = TDEApplication::desktop();
  TQRect r;
  if (desktop->numScreens() < 2)
      r = desktop->geometry();
  else
      r = desktop->screenGeometry( desktop->screenNumber(TQCursor::pos()));
  windowListMenu->init();
  disconnect( windowListMenu, TQT_SIGNAL( aboutToShow() ),
           this, TQT_SLOT( slotWindowListAboutToShow() ) ); // avoid calling init() twice
  // windowListMenu->rect() is not valid before showing, use sizeHint()
  windowListMenu->popup(r.center() - TQRect( TQPoint( 0, 0 ), windowListMenu->sizeHint()).center());
  windowListMenu->selectActiveWindow(); // make the popup more useful
  connect( windowListMenu, TQT_SIGNAL( aboutToShow() ),
           this, TQT_SLOT( slotWindowListAboutToShow() ) );
}

void KRootWm::slotSwitchUser() {
//  kdDebug() << "KRootWm::slotSwitchUser" << endl;
  if (!sessionsMenu)
    return;
  TQDesktopWidget* desktop = TDEApplication::desktop();
  TQRect r;
  if (desktop->numScreens() < 2)
      r = desktop->geometry();
  else
      r = desktop->screenGeometry( desktop->screenNumber(TQCursor::pos()));
  slotPopulateSessions();
  disconnect( sessionsMenu, TQT_SIGNAL( aboutToShow() ),
           this, TQT_SLOT( slotPopulateSessions() ) ); // avoid calling init() twice
  sessionsMenu->popup(r.center() - TQRect( TQPoint( 0, 0 ), sessionsMenu->sizeHint()).center());
  connect( sessionsMenu, TQT_SIGNAL( aboutToShow() ),
           TQT_SLOT( slotPopulateSessions() ) );
}

void KRootWm::slotArrangeByNameCS()
{
    if (m_bDesktopEnabled)
    {
        bool b = static_cast<TDEToggleAction *>(m_actionCollection->action("sort_directoriesfirst"))->isChecked();
        m_pDesktop->iconView()->rearrangeIcons( KDIconView::NameCaseSensitive, b);
    }
}

void KRootWm::slotArrangeByNameCI()
{
    if (m_bDesktopEnabled)
    {
        bool b = static_cast<TDEToggleAction *>(m_actionCollection->action("sort_directoriesfirst"))->isChecked();
        m_pDesktop->iconView()->rearrangeIcons( KDIconView::NameCaseInsensitive, b);
    }
}

void KRootWm::slotArrangeBySize()
{
    if (m_bDesktopEnabled)
    {
        bool b = static_cast<TDEToggleAction *>(m_actionCollection->action("sort_directoriesfirst"))->isChecked();
        m_pDesktop->iconView()->rearrangeIcons( KDIconView::Size, b);
    }
}

void KRootWm::slotArrangeByDate()
{
    if (m_bDesktopEnabled)
    {
        bool b = static_cast<TDEToggleAction *>( m_actionCollection->action( "sort_directoriesfirst" ) )->isChecked();
        m_pDesktop->iconView()->rearrangeIcons( KDIconView::Date, b );
    }
}

void KRootWm::slotArrangeByType()
{
    if (m_bDesktopEnabled)
    {
        bool b = static_cast<TDEToggleAction *>(m_actionCollection->action("sort_directoriesfirst"))->isChecked();
        m_pDesktop->iconView()->rearrangeIcons( KDIconView::Type, b);
    }
}

void KRootWm::slotLineupIconsHoriz() {
    if (m_bDesktopEnabled)
    {
        m_pDesktop->iconView()->lineupIcons(TQIconView::LeftToRight);
    }
}

void KRootWm::slotLineupIconsVert()  {
    if (m_bDesktopEnabled)
    {
        m_pDesktop->iconView()->lineupIcons(TQIconView::TopToBottom);
    }
}

void KRootWm::slotLineupIcons() {
    if (m_bDesktopEnabled)
    {
        m_pDesktop->iconView()->lineupIcons();
    }
}

void KRootWm::slotToggleLockIcons( bool lock )
{
    if (m_bDesktopEnabled)
    {
        m_pDesktop->iconView()->setItemsMovable( !lock );
        KDesktopSettings::setLockIcons( lock );
        KDesktopSettings::writeConfig();
    }
}

void KRootWm::slotRefreshDesktop() {
    if (m_bDesktopEnabled)
    {
        m_pDesktop->refresh();
    }
}

TQStringList KRootWm::configModules() {
  TQStringList args;
  args << "tde-background.desktop" << "tde-desktopbehavior.desktop"  << "tde-desktop.desktop"
	  << "tde-screensaver.desktop" << "tde-display.desktop";
  return args;
}

void KRootWm::slotOpenTerminal()
{
    // kdDebug() << "KRootWm::slotOpenTerminal" << endl;
    TDEProcess* p = new TDEProcess;
    TQ_CHECK_PTR(p);

    TDEConfigGroupSaver gs(TDEGlobal::config(), "General");
    TQString terminal = TDEGlobal::config()->readPathEntry("TerminalApplication", "konsole");

    *p << terminal << "--workdir=" + TDEGlobalSettings::desktopPath() + "/";

    p->start(TDEProcess::DontCare);

    delete p;
}

void KRootWm::slotConfigureDesktop() {
  if (!m_configDialog)
  {
    m_configDialog = new KCMultiDialog( (TQWidget*)0, "configureDialog" );
    connect(m_configDialog, TQT_SIGNAL(finished()), this, TQT_SLOT(slotConfigClosed()));

    TQStringList modules = configModules();
    for (TQStringList::const_iterator it = modules.constBegin(); it != modules.constEnd(); ++it)
    {
      if (kapp->authorizeControlModule(*it))
      {
          m_configDialog->addModule(*it);
      }
    }
  }

  KWin::setOnDesktop(m_configDialog->winId(), KWin::currentDesktop());
  m_configDialog->show();
  m_configDialog->raise();
}

void KRootWm::slotConfigClosed()
{
    m_configDialog->delayedDestruct();
    m_configDialog = 0;
}

void KRootWm::slotToggleDesktopMenu()
{
    KDesktopSettings::setShowMenubar( !(m_bShowMenuBar && menuBar) );
    KDesktopSettings::writeConfig();

    TQByteArray data;
    kapp->dcopClient()->send( kdesktop_name, "KDesktopIface", "configure()", data);
    // for the standalone menubar setting
    kapp->dcopClient()->send( "menuapplet*", "menuapplet", "configure()", data );
    kapp->dcopClient()->send( kicker_name, kicker_name, "configureMenubar()", data );
    kapp->dcopClient()->send( "twin*", "", "reconfigure()", data );
}


void KRootWm::slotUnclutterWindows()
{
    kapp->dcopClient()->send(twin_name, "KWinInterface", "unclutterDesktop()", TQString(""));
}


void KRootWm::slotCascadeWindows() {
    kapp->dcopClient()->send(twin_name, "KWinInterface", "cascadeDesktop()", TQString(""));
}


void KRootWm::slotLock() {
	m_pSaver->lockScreen();
}


void KRootWm::slotSave() {
    kapp->dcopClient()->send(kdesktop_name, "KScreensaverIface", "save()", TQString(""));
}


void KRootWm::slotLogout() {
    m_pDesktop->logout(TDEApplication::ShutdownConfirmDefault, TDEApplication::ShutdownTypeDefault);
}

void KRootWm::slotPopulateSessions()
{
    TDEAction *action;
    int p;
    DM dm;

    sessionsMenu->clear();
    action = m_actionCollection->action("newsession");
    if (action && (p = dm.numReserve()) >= 0)
    {
        action->plug( sessionsMenu );
        action->setEnabled( p );
        action = m_actionCollection->action("lockNnewsession");
        if (action)
        {
            action->plug( sessionsMenu );
            action->setEnabled( p );
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

void KRootWm::slotSessionActivated( int ent )
{
    if (ent > 0 && !sessionsMenu->isItemChecked( ent )) {
        m_pSaver->lockScreenAndSwitchSession(ent);
    }
}

void KRootWm::slotNewSession()
{
    doNewSession( false );
}

void KRootWm::slotLockNNewSession()
{
    doNewSession( true );
}

void KRootWm::doNewSession( bool lock )
{
    int result = KMessageBox::warningContinueCancel(
        m_pDesktop,
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

    if (lock) {
        m_pSaver->lockScreenAndDoNewSession();
    }
    else {
        DM().startReserve();
    }
}

void KRootWm::slotMenuItemActivated(int /* item */ )
{
}

#include "krootwm.moc"
