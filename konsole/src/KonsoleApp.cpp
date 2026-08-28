/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1996 by Matthias Ettrich <ettrich@kde.org>
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/
/* The material contained in here more or less directly orginates from    */
/* kvt, which is copyright (c) 1996 by Matthias Ettrich <ettrich@kde.org> */
/*                                                                        */

/*! \class KonsoleApp

    \brief Konsole's main class and program

    The class KonsoleApp handles the application level. Mainly, it is responsible
    for the configuration, taken from several files, from the command line
    and from the user. It hardly does anything interesting.
    Everything is created as late as possible to make it startup fast.
*/

/*TODO:
  - allow to set codec
  - officially declare this file to be hacked to death. ;^)
  - merge into konsole_part.
*/

/*STATE:

  konsole/twin session management, parts stuff, config, menues
  are all in bad need for a complete rewrite.

  While the emulation core (TEmulation, TEVt102, TEScreen, TEWidget)
  are pretty stable, the upper level material has certainly drifted.

  Everything related to Sessions, Configuration has to be redesigned.
  It seems that the konsole now falls apart into individual sessions
  and a session manager.

Time to start a requirement list.

  - Rework the Emulation::setConnect logic.
    Together with session changing (Shift-Left/Right, Ctrl-D) it allows
    key events to propagate to other sessions.

  - Get rid of the unconfigurable, uncallable initial "Konsole" session.
    Leads to code replication in konsole_part and other issues. Related
    to the booting of konsole, thus matter of redesign.
*/

/*FIXME:
  - All the material in here badly sufferes from the fact that the
    configuration can originate from many places, so all is duplicated
    and falls out of service. Especially the command line is badly broken.
    The sources are:
    - command line
    - menu
    - configuration files
    - other events (e.g. resizing)
    We have to find a single-place method to better maintain this.
  - In general, the material contained in here and in session.C
    should be rebalanced. Much more material now comes from configuration
    files and the overall routines should better respect this.
  - Font+Size list should go to a configuration file, too.
  - Controlling the widget is currently done by individual attributes.
    This lead to quite some amount of flicker when a whole bunch of
    attributes has to be set, e.g. in session swapping.
*/

#include <config.h>

#include <tqspinbox.h>
#include <tqcheckbox.h>
#include <tqimage.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqhbox.h>
#include <tqtoolbutton.h>
#include <tqtooltip.h>

#include <stdio.h>
#include <stdlib.h>

#include <tdefiledialog.h>
#include <kurlrequesterdlg.h>

#include <tdefontdialog.h>
#include <kkeydialog.h>
#include <tdestandarddirs.h>
#include <tqpainter.h>
#include <tdemenubar.h>
#include <tdemessagebox.h>
#include <krootpixmap.h>
#include <krun.h>
#include <kstdaction.h>
#include <kinputdialog.h>
#include <kdebug.h>
#include <kipc.h>
#include <dcopclient.h>
#include <tdeglobalsettings.h>
#include <knotifydialog.h>
#undef B0
#include <kprinter.h>
#include <tdeaccelmanager.h>
#include <kurifilter.h>

#include <tdeaction.h>
#include <kshell.h>
#include <tqlabel.h>
#include <tdepopupmenu.h>
#include <tdelocale.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>

#include <kiconloader.h>
#include <kstringhandler.h>
#include <ktip.h>
#include <tdeprocctrl.h>
#include <ktabwidget.h>
#include <kregexpeditorinterface.h>
#include <tdeparts/componentfactory.h>
#include <kcharsets.h>
#include <kcolordialog.h>
#include <tdeio/netaccess.h>

#include "KonsoleApp.h"
#include <netwm.h>
#include "printsettings.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define KONSOLEDEBUG    kdDebug(1211)

#define POPUP_NEW_SESSION_ID 121
#define POPUP_SETTINGS_ID 212

#define SESSION_NEW_WINDOW_ID 1
#define SESSION_NEW_SHELL_ID 100

#define DEFAULT_HISTORY_SIZE 1000

extern bool argb_visual; // declared in main.cpp and konsole_part.cpp

namespace Konsole {

// KonsoleFontSelectAction is now also used for selectSize!
class KonsoleFontSelectAction : public TDESelectAction {
public:
    KonsoleFontSelectAction(const TQString &text, int accel,
                            const TQObject* receiver, const char* slot,
                            TQObject* parent, const char* name = 0 )
        : TDESelectAction(text, accel, receiver, slot, parent, name) {}
    KonsoleFontSelectAction( const TQString &text, const TQIconSet& pix,
                             int accel, const TQObject* receiver,
                             const char* slot, TQObject* parent,
                             const char* name = 0 )
        : TDESelectAction(text, pix, accel, receiver, slot, parent, name) {}

    virtual void slotActivated( int index );
};

void KonsoleFontSelectAction::slotActivated(int index) {
    // emit even if it's already activated
    if (currentItem() == index) {
        TDESelectAction::slotActivated();
        return;
    } else {
        TDESelectAction::slotActivated(index);
    }
}

KonsoleApp::KonsoleApp(const char* name, int histon, bool menubaron, bool tabbaron, bool frameon, bool scrollbaron,
                 TQCString type, bool b_inRestore, const int wanted_tabbar, const TQString &workdir )
:DCOPObject( "konsole" )
,TDEMainWindow(0, name)
,m_defaultSession(0)
,m_defaultSessionFilename("")
,tabwidget(0)
,te(0)
,se(0)
,se_previous(0)
,m_initialSession(0)
,colors(0)
,tWinModule(0)
,menubar(0)
,statusbar(0)
,m_session_string(i18n("&Session"))
,m_edit_string(i18n("&Edit"))
,m_view_string(i18n("&View"))
,m_bookmarks_string(i18n("&Bookmarks"))
,m_options_string(i18n("Se&ttings"))
,m_help_string(i18n("&Help"))
,m_session_id(-1)
,m_edit_id(-1)
,m_view_id(-1)
,m_bookmarks_id(-1)
,m_options_id(-1)
,m_help_id(-1)
,m_session(0)
,m_edit(0)
,m_view(0)
,m_bookmarks(0)
,m_bookmarksSession(0)
,m_options(0)
,m_schema(0)
,m_keytab(0)
,m_tabbarSessionsCommands(0)
,m_signals(0)
,m_help(0)
,m_rightButton(0)
,m_zmodemUpload(0)
,monitorActivity(0)
,monitorSilence(0)
,masterMode(0)
,showMenubar(0)
,m_fullscreen(0)
,selectSize(0)
,selectFont(0)
,selectScrollbar(0)
,selectTabbar(0)
,selectBell(0)
,selectSetEncoding(0)
,m_clearHistory(0)
,m_findHistory(0)
,m_saveHistory(0)
,m_detachSession(0)
,m_moveSessionLeft(0)
,m_moveSessionRight(0)
,bookmarkHandler(0)
,bookmarkHandlerSession(0)
,m_finddialog(0)
,m_find_pattern("")
,cmd_serial(0)
,cmd_first_screen(-1)
,n_keytab(0)
,n_defaultKeytab(0)
,n_render(0)
,curr_schema(0)
,wallpaperSource(0)
,sessionIdCounter(0)
,monitorSilenceSeconds(10)
,s_tdeconfigSchema("")
,m_tabViewMode(ShowIconAndText)
,b_dynamicTabHide(false)
,b_autoResizeTabs(false)
,b_installBitmapFonts(false)
,b_framevis(true)
,b_metaAsAlt(false)
,b_realTransparency(false)
,b_fullscreen(false)
,m_menuCreated(false)
,b_warnQuit(false)
,b_allowResize(true) // Whether application may resize
,b_fixedSize(false) // Whether user may resize
,b_addToUtmp(true)
,b_xonXoff(false)
,b_bidiEnabled(false)
,b_fullScripting(false)
,b_showstartuptip(true)
,b_sessionShortcutsEnabled(false)
,b_sessionShortcutsMapped(false)
,b_matchTabWinTitle(false)
,m_histSize(DEFAULT_HISTORY_SIZE)
,m_separator_id(-1)
,m_newSessionButton(0)
,m_removeSessionButton(0)
,sessionNumberMapper(0)
,sl_sessionShortCuts(0)
,s_workDir(workdir)
,m_filterData(0)
{
  isRestored = b_inRestore;
  connect( &m_closeTimeout, TQ_SIGNAL(timeout()), this, TQ_SLOT(slotCouldNotClose()));

  no2command.setAutoDelete(true);
  menubar = menuBar();

  TDEAcceleratorManager::setNoAccel( menubar );

  sessionNumberMapper = new TQSignalMapper( this );
  connect( sessionNumberMapper, TQ_SIGNAL( mapped( int ) ),
          this, TQ_SLOT( newSessionTabbar( int ) ) );

  colors = new ColorSchemaList();
  colors->checkSchemas();
  colors->sort();

  KeyTrans::loadAll();

  // create applications /////////////////////////////////////////////////////
  // read and apply default values ///////////////////////////////////////////
  resize(321, 321); // Dummy.
  TQSize currentSize = size();
  TDEConfig * config = TDEGlobal::config();
  config->setDesktopGroup();
  applyMainWindowSettings(config);
  if (currentSize != size())
     defaultSize = size();

  TDESimpleConfig *co;
  if (!type.isEmpty())
    setDefaultSession(type+".desktop");
  co = defaultSession();

  co->setDesktopGroup();
  TQString schema = co->readEntry("Schema");
  readProperties(config, schema, false);

  makeBasicGUI();
  setMenuAcceleratos();

  if (isRestored) {
    n_tabbar = wanted_tabbar;
    TDEConfig *c = tdeApp->sessionConfig();
//    c->setDesktopGroup();      // Reads from wrong group
    b_dynamicTabHide = c->readBoolEntry("DynamicTabHide", false);
  }

  if (!tabbaron)
    n_tabbar = TabNone;

  makeTabWidget();
  setCentralWidget(tabwidget);

  if (b_dynamicTabHide || n_tabbar==TabNone)
    tabwidget->setTabBarHidden(true);

  if (!histon)
    b_histEnabled=false;

  if (!menubaron)
    menubar->hide();
  if (!frameon) {
    b_framevis=false;
    if (te) te->setFrameStyle( TQFrame::NoFrame );
  }
  if (!scrollbaron) {
    n_scroll = TEWidget::SCRNONE;
    if (te) te->setScrollbarLocation(TEWidget::SCRNONE);
  }

//  connect(tdeApp, TQ_SIGNAL(tdedisplayFontChanged()), this, TQ_SLOT(slotFontChanged()));

  tdeApp->dcopClient()->setDefaultObject( "konsole" );
}


KonsoleApp::~KonsoleApp()
{
    delete m_filterData;

    sessions.first();
    while(sessions.current())
    {
      sessions.current()->closeSession();
      sessions.next();
    }

    // Wait a bit for all childs to clean themselves up.
    while(sessions.count() && TDEProcessController::theTDEProcessController->waitForProcessExit(1))
        ;

    sessions.setAutoDelete(true);

    resetScreenSessions();
    if (no2command.isEmpty())
       delete m_defaultSession;

    delete colors;
    colors=0;

    delete tWinModule;
    tWinModule = 0;
}

void KonsoleApp::setAutoClose(bool on)
{
    if (sessions.first())
       sessions.first()->setAutoClose(on);
}

void KonsoleApp::showTip()
{
   KTipDialog::showTip(this,TQString::null,true);
}

void KonsoleApp::showTipOnStart()
{
   if (b_showstartuptip)
      KTipDialog::showTip(this);
}

/* ------------------------------------------------------------------------- */
/*  Make menu                                                                */
/* ------------------------------------------------------------------------- */

void KonsoleApp::updateRMBMenu()
{
   if (!m_rightButton) return;
   int index = 0;
   if (!showMenubar->isChecked() && m_options)
   {
      // Only show when menubar is hidden
      if (!showMenubar->isPlugged( m_rightButton ))
      {
         showMenubar->plug ( m_rightButton, index );
         m_rightButton->insertSeparator( index+1 );
      }
      index = 2;
      m_rightButton->setItemVisible(POPUP_NEW_SESSION_ID,true);
      if (m_separator_id != -1)
         m_rightButton->setItemVisible(m_separator_id,true);
      m_rightButton->setItemVisible(POPUP_SETTINGS_ID,true);
   }
   else
   {
      if (showMenubar->isPlugged( m_rightButton ))
      {
         showMenubar->unplug ( m_rightButton );
         m_rightButton->removeItemAt(index);
      }
      index = 0;
      m_rightButton->setItemVisible(POPUP_NEW_SESSION_ID,false);
      m_rightButton->setItemVisible(m_separator_id,false);
      m_rightButton->setItemVisible(POPUP_SETTINGS_ID,false);
   }

   if (!m_fullscreen)
      return;
   if (b_fullscreen)
   {
      if (!m_fullscreen->isPlugged( m_rightButton ))
      {
         m_fullscreen->plug ( m_rightButton, index );
         m_rightButton->insertSeparator( index+1 );
      }
   }
   else
   {
      if (m_fullscreen->isPlugged( m_rightButton ))
      {
         m_fullscreen->unplug ( m_rightButton );
         m_rightButton->removeItemAt(index);
      }
   }
}

// Be carefull !!
// This function consumes a lot of time, that's why it is called delayed on demand.
// Be careful not to introduce function calls which lead to the execution of this
// function when starting konsole
// Be careful not to access stuff which is created in this function before this
// function was called ! you can check this using m_menuCreated, aleXXX
void KonsoleApp::makeGUI()
{
   if (m_menuCreated) return;

   if (m_tabbarSessionsCommands)
      disconnect(m_tabbarSessionsCommands,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   disconnect(m_session,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   if (m_options)
      disconnect(m_options,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   if (m_help)
      disconnect(m_help,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   if (m_rightButton)
      disconnect(m_rightButton,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   disconnect(m_edit,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   disconnect(m_view,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   if (m_bookmarks)
      disconnect(m_bookmarks,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   if (m_bookmarksSession)
      disconnect(m_bookmarksSession,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
   if (m_tabbarSessionsCommands)
      connect(m_tabbarSessionsCommands,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(loadScreenSessions()));
   connect(m_session,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(loadScreenSessions()));
   m_menuCreated=true;

   // Remove the empty separator Qt inserts if the menu is empty on popup,
   // not sure if this will be "fixed" in Qt, for now use this hack (malte)
   if(!(isRestored)) {
     if (sender() && sender()->inherits("TQPopupMenu") &&
       static_cast<const TQPopupMenu *>(sender())->count() == 1)
       const_cast<TQPopupMenu *>(static_cast<const TQPopupMenu *>(sender()))->removeItemAt(0);
       }

   TDEActionCollection* actions = actionCollection();

   // Send Signal Menu -------------------------------------------------------------
   if (tdeApp->authorizeTDEAction("send_signal"))
   {
      m_signals = new TDEPopupMenu(this);
      m_signals->insertItem( i18n( "&Suspend Task" )   + " (STOP)", SIGSTOP);
      m_signals->insertItem( i18n( "&Continue Task" )  + " (CONT)", SIGCONT);
      m_signals->insertItem( i18n( "&Hangup" )         + " (HUP)", SIGHUP);
      m_signals->insertItem( i18n( "&Interrupt Task" ) + " (INT)", SIGINT);
      m_signals->insertItem( i18n( "&Terminate Task" ) + " (TERM)", SIGTERM);
      m_signals->insertItem( i18n( "&Kill Task" )      + " (KILL)", SIGKILL);
      m_signals->insertItem( i18n( "User Signal &1")   + " (USR1)", SIGUSR1);
      m_signals->insertItem( i18n( "User Signal &2")   + " (USR2)", SIGUSR2);
      connect(m_signals, TQ_SIGNAL(activated(int)), TQ_SLOT(sendSignal(int)));
      TDEAcceleratorManager::manage( m_signals );
   }

   // Edit Menu ----------------------------------------------------------------
   m_copyClipboard->plug(m_edit);
   m_pasteClipboard->plug(m_edit);

   m_edit->setCheckable(true);
   if (m_signals)
      m_edit->insertItem( i18n("&Send Signal"), m_signals );

   if ( m_zmodemUpload ) {
      m_edit->insertSeparator();
      m_zmodemUpload->plug( m_edit );
   }

   m_edit->insertSeparator();
   m_clearTerminal->plug(m_edit);

   m_resetClearTerminal->plug(m_edit);

   m_edit->insertSeparator();
   m_findHistory->plug(m_edit);
   m_findNext->plug(m_edit);
   m_findPrevious->plug(m_edit);
   m_saveHistory->plug(m_edit);
   m_edit->insertSeparator();
   m_clearHistory->plug(m_edit);
   m_clearAllSessionHistories->plug(m_edit);

   // View Menu
   m_detachSession->plug(m_view);
   m_renameSession->plug(m_view);

   m_view->insertSeparator();
   monitorActivity->plug ( m_view );
   monitorSilence->plug ( m_view );

   masterMode->plug ( m_view );

   m_view->insertSeparator();
   m_moveSessionLeft->setEnabled( false );
   m_moveSessionLeft->plug(m_view);

   m_moveSessionRight->setEnabled( false );
   m_moveSessionRight->plug(m_view);

   m_view->insertSeparator();
   TDERadioAction *ra = session2action.find(se);
   if (ra!=0) ra->plug(m_view);

   //bookmarks menu
   if (bookmarkHandler)
      connect( bookmarkHandler, TQ_SIGNAL( openURL( const TQString&, const TQString& )),
            TQ_SLOT( enterURL( const TQString&, const TQString& )));
   if (bookmarkHandlerSession)
      connect( bookmarkHandlerSession, TQ_SIGNAL( openURL( const TQString&, const TQString& )),
            TQ_SLOT( newSession( const TQString&, const TQString& )));
   if (m_bookmarks)
      connect(m_bookmarks, TQ_SIGNAL(aboutToShow()), TQ_SLOT(bookmarks_menu_check()));
   if (m_bookmarksSession)
      connect(m_bookmarksSession, TQ_SIGNAL(aboutToShow()), TQ_SLOT(bookmarks_menu_check()));

   // Schema Options Menu -----------------------------------------------------
   m_schema = new TDEPopupMenu(this);
   m_schema->setCheckable(true);
   TDEAcceleratorManager::manage( m_schema );
   connect(m_schema, TQ_SIGNAL(activated(int)), TQ_SLOT(schema_menu_activated(int)));
   connect(m_schema, TQ_SIGNAL(aboutToShow()), TQ_SLOT(schema_menu_check()));

   // Keyboard Options Menu ---------------------------------------------------
   m_keytab = new TDEPopupMenu(this);
   m_keytab->setCheckable(true);
   TDEAcceleratorManager::manage( m_keytab );
   connect(m_keytab, TQ_SIGNAL(activated(int)), TQ_SLOT(keytab_menu_activated(int)));

   //options menu
   if (m_options)
   {
      // Menubar on/off
      showMenubar->plug ( m_options );

      // Tabbar
      selectTabbar = new TDESelectAction(i18n("&Tab Bar"), 0, this,
                                       TQ_SLOT(slotSelectTabbar()), actions, "tabbar" );
      TQStringList tabbaritems;
      tabbaritems << i18n("&Hide") << i18n("&Top") << i18n("&Bottom");
      selectTabbar->setItems(tabbaritems);
      selectTabbar->plug(m_options);

      // Scrollbar
      selectScrollbar = new TDESelectAction(i18n("Sc&rollbar"), 0, this,
                                       TQ_SLOT(slotSelectScrollbar()), actions, "scrollbar" );
      TQStringList scrollitems;
      scrollitems << i18n("&Hide") << i18n("&Left") << i18n("&Right");
      selectScrollbar->setItems(scrollitems);
      selectScrollbar->plug(m_options);

      // Fullscreen
      m_options->insertSeparator();
      if (m_fullscreen)
      {
        m_fullscreen->plug(m_options);
        m_options->insertSeparator();
      }

      // Select Bell
      selectBell = new TDESelectAction(i18n("&Bell"), SmallIconSet( "bell"), 0 , this,
                                  TQ_SLOT(slotSelectBell()), actions, "bell");
      TQStringList bellitems;
      bellitems << i18n("System &Bell")
                << i18n("System &Notification")
                << i18n("&Visible Bell")
                << i18n("N&one");
      selectBell->setItems(bellitems);
      selectBell->plug(m_options);

      checkBitmapFonts();
      TDEActionMenu* m_fontsizes = new TDEActionMenu( i18n( "Font" ),
                                                  SmallIconSet( "text" ),
                                                  actions, 0L );
      m_fontsizes->insert( new TDEAction( i18n( "&Enlarge Font" ),
                           SmallIconSet( "fontsizeup" ), 0, this,
                           TQ_SLOT( biggerFont() ), actions,
                           "enlarge_font" ) );
      m_fontsizes->insert( new TDEAction( i18n( "&Shrink Font" ),
                           SmallIconSet( "fontsizedown" ), 0, this,
                           TQ_SLOT( smallerFont() ), actions,
                           "shrink_font" ) );
      m_fontsizes->insert( new TDEAction( i18n( "Se&lect..." ),
                           SmallIconSet( "font-x-generic" ), 0, this,
                           TQ_SLOT( slotSelectFont() ), actions,
                           "select_font" ) );
      if ( b_installBitmapFonts )
      {
         m_fontsizes->insert( new TDEAction( i18n( "&Install Bitmap..." ),
                              SmallIconSet( "font-x-generic" ), 0, this,
                              TQ_SLOT( slotInstallBitmapFonts() ), actions,
                              "install_fonts" ) );
      }
      m_fontsizes->plug(m_options);

      // encoding menu, start with default checked !
      selectSetEncoding = new TDESelectAction( i18n( "&Encoding" ), SmallIconSet( "charset" ), 0, this, TQ_SLOT(slotSetEncoding()), actions, "set_encoding" );
      TQStringList list = TDEGlobal::charsets()->descriptiveEncodingNames();
      list.prepend( i18n( "Default" ) );
      selectSetEncoding->setItems(list);
      selectSetEncoding->setCurrentItem (0);
      selectSetEncoding->plug(m_options);

      if (tdeApp->authorizeTDEAction("keyboard"))
         m_options->insertItem( SmallIconSet( "key_bindings" ), i18n( "&Keyboard" ), m_keytab );

      // Schema
      if (tdeApp->authorizeTDEAction("schema"))
         m_options->insertItem( SmallIconSet( "colorize" ), i18n( "Sch&ema" ), m_schema);

      // Select size
      if (!b_fixedSize)
      {
         selectSize = new KonsoleFontSelectAction(i18n("S&ize"), 0, this,
                                  TQ_SLOT(slotSelectSize()), actions, "size");
         TQStringList sizeitems;
         sizeitems << i18n("40x15 (&Small)")
            << i18n("80x24 (&VT100)")
            << i18n("80x25 (&IBM PC)")
            << i18n("80x40 (&XTerm)")
            << i18n("80x52 (IBM V&GA)")
            << ""
            << i18n("&Custom...");
         selectSize->setItems(sizeitems);
         selectSize->plug(m_options);
      }

      TDEAction *historyType = new TDEAction(i18n("Hist&ory..."), "history", 0, this,
                                      TQ_SLOT(slotHistoryType()), actions, "history");
      historyType->plug(m_options);

      m_options->insertSeparator();

      TDEAction *save_settings = new TDEAction(i18n("&Save as Default"), "document-save", 0, this,
                                        TQ_SLOT(slotSaveSettings()), actions, "save_default");
      save_settings->plug(m_options);

      m_options->insertSeparator();

      m_saveProfile->plug(m_options);

      m_options->insertSeparator();

      KStdAction::configureNotifications(this, TQ_SLOT(slotConfigureNotifications()), actionCollection())->plug(m_options);
      KStdAction::keyBindings(this, TQ_SLOT(slotConfigureKeys()), actionCollection())->plug(m_options);
      TDEAction *configure = KStdAction::preferences(this, TQ_SLOT(slotConfigure()), actions);
      configure->plug(m_options);

      if (TDEGlobalSettings::insertTearOffHandle())
         m_options->insertTearOffHandle();
   }

   //help menu
   if (m_help)
   {
      m_help->insertSeparator(1);
      m_help->insertItem(SmallIcon( "idea" ), i18n("&Tip of the Day"),
            this, TQ_SLOT(showTip()), 0, -1, 2);
   }

   //the different session menus
   buildSessionMenus();

   connect(m_session, TQ_SIGNAL(activated(int)), TQ_SLOT(newSession(int)));

   // Right mouse button menu
   if (m_rightButton)
   {
      updateRMBMenu(); // show menubar / exit fullscreen

      TDEAction* selectionEnd = new TDEAction(i18n("Set Selection End"), 0, this,
                               TQ_SLOT(slotSetSelectionEnd()), actions, "selection_end");
      selectionEnd->plug(m_rightButton);

      m_copyClipboard->plug(m_rightButton);
      m_pasteClipboard->plug(m_rightButton);

      m_openSelection = new TDEPopupMenu(this);
      m_rightButton->insertItem( i18n("&Open.."), m_openSelection );
      connect(m_openSelection, TQ_SIGNAL(aboutToShow()), TQ_SLOT(slotOpenSelection()));

      if (m_signals)
         m_rightButton->insertItem(i18n("&Send Signal"), m_signals);

      m_rightButton->insertSeparator();
      if (m_tabbarSessionsCommands)
         m_rightButton->insertItem( i18n("New Sess&ion"), m_tabbarSessionsCommands, POPUP_NEW_SESSION_ID );
      m_detachSession->plug(m_rightButton);
      m_renameSession->plug(m_rightButton);

      if (m_bookmarks)
      {
         m_rightButton->insertSeparator();
         m_rightButton->insertItem(i18n("&Bookmarks"), m_bookmarks);
      }

      if (m_options)
      {
         m_separator_id=m_rightButton->insertSeparator();
         m_rightButton->insertItem(i18n("S&ettings"), m_options, POPUP_SETTINGS_ID);
      }
      m_rightButton->insertSeparator();
      m_closeSession->plug(m_rightButton );
      if (TDEGlobalSettings::insertTearOffHandle())
         m_rightButton->insertTearOffHandle();
   }

   delete colors;
   colors = new ColorSchemaList();
   colors->checkSchemas();
   colors->sort();
   updateSchemaMenu();
   ColorSchema *sch=colors->find(s_schema);
   if (sch)
        curr_schema=sch->numb();
   else
        curr_schema = 0;
   for (uint i=0; i<m_schema->count(); i++)
      m_schema->setItemChecked(i,false);

   m_schema->setItemChecked(curr_schema,true);
   while (se == NULL) {}
   se->setSchemaNo(curr_schema);

   // insert keymaps into menu
   // This sorting seems a bit cumbersome; but it is not called often.
   TQStringList kt_titles;
   typedef TQMap<TQString,KeyTrans*> QStringKeyTransMap;
   QStringKeyTransMap kt_map;

   for (int i = 0; i < KeyTrans::count(); i++)
   {
      KeyTrans* ktr = KeyTrans::find(i);
      assert( ktr );
      TQString title=ktr->hdr().lower();
      kt_titles << title;
      kt_map[title] = ktr;
   }
   kt_titles.sort();
   for ( TQStringList::Iterator it = kt_titles.begin(); it != kt_titles.end(); ++it ) {
      KeyTrans* ktr = kt_map[*it];
      assert( ktr );
      TQString title=ktr->hdr();
      m_keytab->insertItem(title.replace('&',"&&"),ktr->numb());
   }

   applySettingsToGUI();
   isRestored = false;


   // Fill tab context menu
   m_tabPopupMenu = new TDEPopupMenu( this );
   TDEAcceleratorManager::manage( m_tabPopupMenu );

   if (!m_detachSession)
   {
     m_detachSession = new TDEAction(i18n("&Detach Session"), SmallIconSet("tab_breakoff"), 0, this,
                                     TQ_SLOT(slotDetachSession()), m_shortcuts, "detach_session");
   }
   m_detachSession->plug(m_tabPopupMenu);

   m_tabPopupMenu->insertItem( i18n("&Rename Session..."), this,
                         TQ_SLOT(slotTabRenameSession()) );
   m_tabPopupMenu->insertSeparator();

   m_tabMonitorActivity = new TDEToggleAction ( i18n( "Monitor for &Activity" ),
      SmallIconSet("activity"), 0, this, TQ_SLOT( slotTabToggleMonitor() ), this );
   m_tabMonitorActivity->setCheckedState( KGuiItem( i18n( "Stop Monitoring for &Activity" ) ) );
   m_tabMonitorActivity->plug(m_tabPopupMenu);

   m_tabMonitorSilence = new TDEToggleAction ( i18n( "Monitor for &Silence" ),
      SmallIconSet("silence"), 0, this, TQ_SLOT( slotTabToggleMonitor() ), this );
   m_tabMonitorSilence->setCheckedState( KGuiItem( i18n( "Stop Monitoring for &Silence" ) ) );
   m_tabMonitorSilence->plug(m_tabPopupMenu);

   m_tabMasterMode = new TDEToggleAction ( i18n( "Send &Input to All Sessions" ), "remote", 0, this,
                                    TQ_SLOT( slotTabToggleMasterMode() ), this);
   m_tabMasterMode->plug(m_tabPopupMenu);

   m_tabPopupMenu->insertSeparator();
   if (!m_moveSessionLeft)
   {
     m_moveSessionLeft = new TDEAction(i18n("&Move Session Left"), TQApplication::reverseLayout() ? "forward" : "back",
                                       TQApplication::reverseLayout() ? TQt::CTRL+TQt::SHIFT+TQt::Key_Right : TQt::CTRL+TQt::SHIFT+TQt::Key_Left, this,
                                       TQ_SLOT(moveSessionLeft()), m_shortcuts, "move_session_left");
   }
   m_moveSessionLeft->plug(m_tabPopupMenu);
   if (!m_moveSessionRight)
   {
     m_moveSessionRight = new TDEAction(i18n("M&ove Session Right"), TQApplication::reverseLayout() ? "back" : "forward",
                                        TQApplication::reverseLayout() ? TQt::CTRL+TQt::SHIFT+TQt::Key_Left : TQt::CTRL+TQt::SHIFT+TQt::Key_Right, this,
                                        TQ_SLOT(moveSessionRight()), m_shortcuts, "move_session_right");
   }
   m_moveSessionRight->plug(m_tabPopupMenu);

   m_tabPopupMenu->insertSeparator();
   m_tabPopupMenu->insertItem( SmallIconSet("colors"), i18n("Select &Tab Color..."), this, TQ_SLOT(slotTabSelectColor()) );
   m_tabPopupMenu->insertItem( i18n("Reset Tab Color"), this, TQ_SLOT(slotTabResetColor()) );

   m_tabPopupMenu->insertSeparator();
   m_tabPopupTabsMenu = new TDEPopupMenu( m_tabPopupMenu );
   m_tabPopupMenu->insertItem( i18n("Switch to Tab" ), m_tabPopupTabsMenu );
   connect( m_tabPopupTabsMenu, TQ_SIGNAL( activated ( int ) ),
            TQ_SLOT( activateSession( int ) ) );

   m_tabPopupMenu->insertSeparator();
   m_tabPopupMenu->insertItem( SmallIcon("window-close"), i18n("C&lose Session"), this,
                          TQ_SLOT(slotTabCloseSession()) );

   if (m_options) {
      // Fill tab bar context menu
      m_tabbarPopupMenu = new TDEPopupMenu( this );
      TDEAcceleratorManager::manage( m_tabbarPopupMenu );
      selectTabbar->plug(m_tabbarPopupMenu);

      TDESelectAction *viewOptions = new TDESelectAction(this);
      viewOptions->setText(i18n("Tab &Options"));
      TQStringList options;
      options << i18n("&Text && Icons") << i18n("Text &Only") << i18n("&Icons Only");
      viewOptions->setItems(options);
      viewOptions->setCurrentItem(m_tabViewMode);
      viewOptions->plug(m_tabbarPopupMenu);
      connect(viewOptions, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotTabSetViewOptions(int)));
      slotTabSetViewOptions(m_tabViewMode);

      TDEToggleAction *dynamicTabHideOption = new TDEToggleAction ( i18n( "&Dynamic Hide" ), 0, this,
                                       TQ_SLOT( slotTabbarToggleDynamicHide() ), this);
      dynamicTabHideOption->setChecked(b_dynamicTabHide);
      dynamicTabHideOption->plug(m_tabbarPopupMenu);

      TDEToggleAction *m_autoResizeTabs = new TDEToggleAction( i18n("&Auto Resize Tabs"),
                 0, this, TQ_SLOT( slotToggleAutoResizeTabs() ), this);
      m_autoResizeTabs->setChecked(b_autoResizeTabs);
      m_autoResizeTabs->plug(m_tabbarPopupMenu);
    }
}

// Called via menu
void KonsoleApp::slotSetEncoding()
{
  if (!se) return;

  TQTextCodec * qtc;
  if (selectSetEncoding->currentItem() == 0)
  {
    qtc = TQTextCodec::codecForLocale();
  }
  else
  {
    bool found;
    TQString enc = TDEGlobal::charsets()->encodingForName(selectSetEncoding->currentText());
    qtc = TDEGlobal::charsets()->codecForName(enc, found);

    // BR114535 : Remove jis7 due to infinite loop.
    if ( enc == "jis7" ) {
      kdWarning()<<"Encoding Japanese (jis7) currently does not work!  BR114535"<<endl;
      qtc = TQTextCodec::codecForLocale();
      selectSetEncoding->setCurrentItem( 0 );
    }

    if(!found)
    {
      kdWarning() << "Codec " << selectSetEncoding->currentText() << " not found!  Using default..." << endl;
      qtc = TQTextCodec::codecForLocale();
      selectSetEncoding->setCurrentItem( 0 );
    }
  }

  se->setEncodingNo(selectSetEncoding->currentItem());
  se->getEmulation()->setCodec(qtc);
}

void KonsoleApp::makeTabWidget()
{
  tabwidget = new KTabWidget(this);
  tabwidget->setTabReorderingEnabled(true);
  tabwidget->setAutomaticResizeTabs(b_autoResizeTabs);
  tabwidget->setTabCloseActivatePrevious(true);
  tabwidget->setMouseWheelScroll(b_mouseWheelScroll);

  if (n_tabbar==TabTop)
    tabwidget->setTabPosition(TQTabWidget::Top);
  else
    tabwidget->setTabPosition(TQTabWidget::Bottom);

  TDEAcceleratorManager::setNoAccel( tabwidget );

  connect(tabwidget, TQ_SIGNAL(movedTab(int,int)), TQ_SLOT(slotMovedTab(int,int)));
  connect(tabwidget, TQ_SIGNAL(mouseDoubleClick(TQWidget*)), TQ_SLOT(slotRenameSession()));
  connect(tabwidget, TQ_SIGNAL(currentChanged(TQWidget*)), TQ_SLOT(activateSession(TQWidget*)));
  connect(tabwidget, TQ_SIGNAL(contextMenu(TQWidget*, const TQPoint &)),
                     TQ_SLOT(slotTabContextMenu(TQWidget*, const TQPoint &)));
  connect(tabwidget, TQ_SIGNAL(contextMenu(const TQPoint &)),
                     TQ_SLOT(slotTabbarContextMenu(const TQPoint &)));

  if (tdeApp->authorize("shell_access")) {
    connect(tabwidget, TQ_SIGNAL(mouseDoubleClick()), TQ_SLOT(newSession()));

    m_newSessionButton = new TQToolButton( tabwidget );
    TQToolTip::add(m_newSessionButton,i18n("Click for new standard session\nClick and hold for session menu"));
    m_newSessionButton->setIconSet( SmallIcon( "tab_new" ) );
    m_newSessionButton->adjustSize();
    m_newSessionButton->setPopup( m_tabbarSessionsCommands );
    connect(m_newSessionButton, TQ_SIGNAL(clicked()), TQ_SLOT(newSession()));
    tabwidget->setCornerWidget( m_newSessionButton, BottomLeft );
    m_newSessionButton->installEventFilter(this);

    m_removeSessionButton = new TQToolButton( tabwidget );
    TQToolTip::add(m_removeSessionButton,i18n("Close the current session"));
    m_removeSessionButton->setIconSet( SmallIconSet( "tab_remove" ) );
    m_removeSessionButton->adjustSize();
    m_removeSessionButton->setEnabled(false);
    connect(m_removeSessionButton, TQ_SIGNAL(clicked()), TQ_SLOT(confirmCloseCurrentSession()));
    tabwidget->setCornerWidget( m_removeSessionButton, BottomRight );

  }
}

bool KonsoleApp::eventFilter( TQObject *o, TQEvent *ev )
{
  if (o == m_newSessionButton)
  {
    // Popup the menu when the left mousebutton is pressed and the mouse
    // is moved by a small distance.
    if (ev->type() == TQEvent::MouseButtonPress)
    {
      TQMouseEvent* mev = static_cast<TQMouseEvent*>(ev);
      m_newSessionButtonMousePressPos = mev->pos();
    }
    else if (ev->type() == TQEvent::MouseMove)
    {
      TQMouseEvent* mev = static_cast<TQMouseEvent*>(ev);
      if ((mev->pos() - m_newSessionButtonMousePressPos).manhattanLength()
            > TDEGlobalSettings::dndEventDelay())
      {
        m_newSessionButton->openPopup();
        return true;
      }
    }
    else if (ev->type() == TQEvent::ContextMenu)
    {
      TQMouseEvent* mev = static_cast<TQMouseEvent*>(ev);
      slotTabbarContextMenu(mev->globalPos());
      return true;
    }
  }
  return TDEMainWindow::eventFilter(o, ev);
}

void KonsoleApp::makeBasicGUI()
{
  if (tdeApp->authorize("shell_access")) {
    m_tabbarSessionsCommands = new TDEPopupMenu( this );
    TDEAcceleratorManager::manage( m_tabbarSessionsCommands );
    connect(m_tabbarSessionsCommands, TQ_SIGNAL(activated(int)), TQ_SLOT(newSessionTabbar(int)));
  }

  m_session = new TDEPopupMenu(this);
  TDEAcceleratorManager::manage( m_session );
  m_edit = new TDEPopupMenu(this);
  TDEAcceleratorManager::manage( m_edit );
  m_view = new TDEPopupMenu(this);
  TDEAcceleratorManager::manage( m_view );
  if (tdeApp->authorizeTDEAction("bookmarks"))
  {
    bookmarkHandler = new KonsoleBookmarkHandler( this, true );
    m_bookmarks = bookmarkHandler->menu();
    // call manually to disable accelerator c-b for add-bookmark initially.
    bookmarks_menu_check();
  }

  if (tdeApp->authorizeTDEAction("settings")) {
     m_options = new TDEPopupMenu(this);
     TDEAcceleratorManager::manage( m_options );
  }

  if (tdeApp->authorizeTDEAction("help"))
     m_help = helpMenu(0, false);

  if (tdeApp->authorizeTDEAction("konsole_rmb")) {
     m_rightButton = new TDEPopupMenu(this);
     TDEAcceleratorManager::manage( m_rightButton );
  }

  if (tdeApp->authorizeTDEAction("bookmarks"))
  {
    // Bookmarks that open new sessions.
    bookmarkHandlerSession = new KonsoleBookmarkHandler( this, false );
    m_bookmarksSession = bookmarkHandlerSession->menu();
  }

  // For those who would like to add shortcuts here, be aware that
  // ALT-key combinations are heavily used by many programs. Thus,
  // activating shortcuts here means deactivating them in the other
  // programs.

  if (m_tabbarSessionsCommands)
     connect(m_tabbarSessionsCommands,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  connect(m_session,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  if (m_options)
     connect(m_options,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  if (m_help)
     connect(m_help,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  if (m_rightButton)
     connect(m_rightButton,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  connect(m_edit,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  connect(m_view,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  if (m_bookmarks)
     connect(m_bookmarks,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));
  if (m_bookmarksSession)
     connect(m_bookmarksSession,TQ_SIGNAL(aboutToShow()),this,TQ_SLOT(makeGUI()));

  m_session_id=menubar->insertItem(m_session_string , m_session);
  m_edit_id=menubar->insertItem(m_edit_string, m_edit);
  m_view_id=menubar->insertItem(m_view_string, m_view);
  if (m_bookmarks)
     m_bookmarks_id=menubar->insertItem(m_bookmarks_string, m_bookmarks);
  if (m_options)
     m_options_id=menubar->insertItem(m_options_string, m_options);
  if (m_help)
     m_help_id=menubar->insertItem(m_help_string, m_help);

  m_shortcuts = new TDEActionCollection(this);

  m_copyClipboard = new TDEAction(i18n("&Copy"), "edit-copy", 0, this,
                                 TQ_SLOT(slotCopyClipboard()), m_shortcuts, "edit_copy");
  m_pasteClipboard = new TDEAction(i18n("&Paste"), "edit-paste", TQt::SHIFT+TQt::Key_Insert, this,
                                 TQ_SLOT(slotPasteClipboard()), m_shortcuts, "edit_paste");
  m_pasteSelection = new TDEAction(i18n("Paste Selection"), TQt::CTRL+TQt::SHIFT+TQt::Key_Insert, this,
                                 TQ_SLOT(slotPasteSelection()), m_shortcuts, "pasteselection");

  m_clearTerminal = new TDEAction(i18n("C&lear Terminal"), 0, this,
                                TQ_SLOT(slotClearTerminal()), m_shortcuts, "clear_terminal");
  m_resetClearTerminal = new TDEAction(i18n("&Reset && Clear Terminal"), 0, this,
                                     TQ_SLOT(slotResetClearTerminal()), m_shortcuts, "reset_clear_terminal");
  m_findHistory = new TDEAction(i18n("&Find in History..."), "edit-find", 0, this,
                              TQ_SLOT(slotFindHistory()), m_shortcuts, "find_history");
  m_findHistory->setEnabled(b_histEnabled);

  m_findNext = new TDEAction(i18n("Find &Next"), "go-next", 0, this,
                           TQ_SLOT(slotFindNext()), m_shortcuts, "find_next");
  m_findNext->setEnabled(b_histEnabled);

  m_findPrevious = new TDEAction(i18n("Find Pre&vious"), "go-previous", 0, this,
                               TQ_SLOT(slotFindPrevious()), m_shortcuts, "find_previous");
  m_findPrevious->setEnabled( b_histEnabled );

  m_saveHistory = new TDEAction(i18n("S&ave History As..."), "document-save-as", 0, this,
                              TQ_SLOT(slotSaveHistory()), m_shortcuts, "save_history");
  m_saveHistory->setEnabled(b_histEnabled );

  m_clearHistory = new TDEAction(i18n("Clear &History"), "history_clear", 0, this,
                               TQ_SLOT(slotClearHistory()), m_shortcuts, "clear_history");
  m_clearHistory->setEnabled(b_histEnabled);

  m_clearAllSessionHistories = new TDEAction(i18n("Clear All H&istories"), "history_clear", 0,
    this, TQ_SLOT(slotClearAllSessionHistories()), m_shortcuts, "clear_all_histories");

  if (!m_detachSession)
  {
    m_detachSession = new TDEAction(i18n("&Detach Session"), SmallIconSet("tab_breakoff"), 0, this,
                                    TQ_SLOT(slotDetachSession()), m_shortcuts, "detach_session");
  }
  m_detachSession->setEnabled(false);

  m_renameSession = new TDEAction(i18n("&Rename Session..."), TQt::CTRL+TQt::SHIFT+TQt::Key_R, this,
                                TQ_SLOT(slotRenameSession()), m_shortcuts, "rename_session");

  if (tdeApp->authorizeTDEAction("zmodem_upload"))
    m_zmodemUpload = new TDEAction( i18n( "&ZModem Upload..." ),
                                  TQt::CTRL+TQt::ALT+TQt::Key_U, this,
                                  TQ_SLOT( slotZModemUpload() ),
                                  m_shortcuts, "zmodem_upload" );

  monitorActivity = new TDEToggleAction ( i18n( "Monitor for &Activity" ),
      SmallIconSet("activity"), 0, this,
      TQ_SLOT( slotToggleMonitor() ), m_shortcuts, "monitor_activity" );
  monitorActivity->setCheckedState( KGuiItem( i18n( "Stop Monitoring for &Activity" ) ) );

  monitorSilence = new TDEToggleAction ( i18n( "Monitor for &Silence" ),
      SmallIconSet("silence"), 0, this,
      TQ_SLOT( slotToggleMonitor() ), m_shortcuts, "monitor_silence" );
  monitorSilence->setCheckedState( KGuiItem( i18n( "Stop Monitoring for &Silence" ) ) );

  masterMode = new TDEToggleAction ( i18n( "Send &Input to All Sessions" ), "remote", 0, this,
                                   TQ_SLOT( slotToggleMasterMode() ), m_shortcuts, "send_input_to_all_sessions" );

  showMenubar = new TDEToggleAction ( i18n( "Show &Menubar" ), "showmenu", TQt::CTRL+TQt::SHIFT+TQt::ALT+TQt::Key_M, this,
                                    TQ_SLOT( slotToggleMenubar() ), m_shortcuts, "show_menubar" );
  showMenubar->setCheckedState( KGuiItem( i18n("Hide &Menubar"), "showmenu", TQString::null, TQString::null ) );

  m_fullscreen = KStdAction::fullScreen(0, 0, m_shortcuts, this );
  connect( m_fullscreen,TQ_SIGNAL(toggled(bool)), this,TQ_SLOT(updateFullScreen(bool)));
  m_fullscreen->setChecked(b_fullscreen);

  m_saveProfile = new TDEAction( i18n( "Save Sessions &Profile..." ), SmallIconSet("document-save-as"), 0, this,
                         TQ_SLOT( slotSaveSessionsProfile() ), m_shortcuts, "save_sessions_profile" );

  //help menu
  if (m_help)
     m_help->setAccel(TQKeySequence(),m_help->idAt(0));
     // Don't steal F1 (handbook) accel (esp. since it not visible in
     // "Configure Shortcuts").

  m_closeSession = new TDEAction(i18n("C&lose Session"), "window-close", 0, this,
                               TQ_SLOT(confirmCloseCurrentSession()), m_shortcuts, "close_session");
  m_print = new TDEAction(i18n("&Print Screen..."), "document-print", 0, this, TQ_SLOT( slotPrint() ), m_shortcuts, "file_print");
  m_quit = new TDEAction(i18n("&Quit"), "system-log-out", 0, this, TQ_SLOT( close() ), m_shortcuts, "file_quit");

  TDEShortcut shortcut(TQt::CTRL+TQt::ALT+TQt::Key_N);
  shortcut.append(TDEShortcut(TQt::CTRL+TQt::SHIFT+TQt::Key_N));
  new TDEAction(i18n("New Session"), shortcut, this, TQ_SLOT(newSession()), m_shortcuts, "new_session");
  new TDEAction(i18n("Activate Menu"), TQt::CTRL+TQt::ALT+TQt::Key_M, this, TQ_SLOT(activateMenu()), m_shortcuts, "activate_menu");
  new TDEAction(i18n("List Sessions"), 0, this, TQ_SLOT(listSessions()), m_shortcuts, "list_sessions");

  if (!m_moveSessionLeft)
  {
    m_moveSessionLeft = new TDEAction(i18n("&Move Session Left"), TQApplication::reverseLayout() ? "forward" : "back",
                                      TQApplication::reverseLayout() ? TQt::CTRL+TQt::SHIFT+TQt::Key_Right : TQt::CTRL+TQt::SHIFT+TQt::Key_Left, this,
                                      TQ_SLOT(moveSessionLeft()), m_shortcuts, "move_session_left");
  }
  if (!m_moveSessionRight)
  {
    m_moveSessionRight = new TDEAction(i18n("M&ove Session Right"), TQApplication::reverseLayout() ? "back" : "forward",
                                       TQApplication::reverseLayout() ? TQt::CTRL+TQt::SHIFT+TQt::Key_Left : TQt::CTRL+TQt::SHIFT+TQt::Key_Right, this,
                                       TQ_SLOT(moveSessionRight()), m_shortcuts, "move_session_right");
  }

  new TDEAction(i18n("Go to Previous Session"), TQApplication::reverseLayout() ? TQt::SHIFT+TQt::Key_Right : TQt::SHIFT+TQt::Key_Left,
              this, TQ_SLOT(prevSession()), m_shortcuts, "previous_session");
  new TDEAction(i18n("Go to Next Session"), TQApplication::reverseLayout() ? TQt::SHIFT+TQt::Key_Left : TQt::SHIFT+TQt::Key_Right,
              this, TQ_SLOT(nextSession()), m_shortcuts, "next_session");

  for (int i=1;i<13;i++) { // Due to 12 function keys?
     new TDEAction(i18n("Switch to Session %1").arg(i), 0, this, TQ_SLOT(switchToSession()), m_shortcuts, TQString(TQString().sprintf("switch_to_session_%02d", i)).latin1());
  }

  new TDEAction(i18n("Enlarge Font"), 0, this, TQ_SLOT(biggerFont()), m_shortcuts, "bigger_font");
  new TDEAction(i18n("Shrink Font"), 0, this, TQ_SLOT(smallerFont()), m_shortcuts, "smaller_font");

  new TDEAction(i18n("Toggle Bidi"), TQt::CTRL+TQt::ALT+TQt::Key_B, this, TQ_SLOT(toggleBidi()), m_shortcuts, "toggle_bidi");

  // Should we load all *.desktop files now?  Required for Session shortcuts.
  if ( TDEConfigGroup(TDEGlobal::config(), "General").readBoolEntry("SessionShortcutsEnabled", false) ) {
    b_sessionShortcutsEnabled = true;
    loadSessionCommands();
    loadScreenSessions();
  }
  m_shortcuts->readShortcutSettings();

  m_sessionList = new TDEPopupMenu(this);
  TDEAcceleratorManager::manage( m_sessionList );
  connect(m_sessionList, TQ_SIGNAL(activated(int)), TQ_SLOT(activateSession(int)));
}

/**
   Make menubar available via escape sequence (Default: Ctrl+Alt+m)
 */
void KonsoleApp::activateMenu()
{
  menubar->activateItemAt(0);
  if ( !showMenubar->isChecked() ) {
    menubar->show();
    showMenubar->setChecked(true);
  }
}

/**
   Ask for Quit confirmation - Martijn Klingens
   Asks for confirmation if there are still open shells when the 'Warn on
   Quit' option is set.
 */
bool KonsoleApp::queryClose()
{
   if(tdeApp->sessionSaving())
     // saving session - do not even think about doing any kind of cleanup here
       return true;

   if (sessions.count() == 0)
       return true;

   if ( b_warnQuit)
   {
        if(sessions.count()>1) {
	    switch (
		KMessageBox::warningYesNoCancel(
	    	    this,
            	    i18n( "You have open sessions (besides the current one). "
                	  "These will be killed if you continue.\n"
                	  "Are you sure you want to quit?" ),
	    	    i18n("Really Quit?"),
	    	    KStdGuiItem::quit(),
	    	    KGuiItem(i18n("C&lose Session"),"window-close")
		)
	    ) {
		case KMessageBox::Yes :
		break;
		case KMessageBox::No :
		    { closeCurrentSession();
		        return false;
		    }
		break;
		case KMessageBox::Cancel : return false;
	    }
	}
    }

    // WABA: Don't close if there are any sessions left.
    // Tell them to go away.
    sessions.first();
    while(sessions.current())
    {
      sessions.current()->closeSession();
      sessions.next();
    }

    m_closeTimeout.start(1500, true);
    return false;
}

void KonsoleApp::slotCouldNotClose()
{
    int result = KMessageBox::warningContinueCancel(this,
             i18n("The application running in Konsole does not respond to the close request. "
                  "Do you want Konsole to close anyway?"),
             i18n("Application Does Not Respond"),
             KStdGuiItem::close());
    if (result == KMessageBox::Continue)
    {
        while(sessions.first())
        {
            doneSession(sessions.current());
        }
    }
}

/**
    sets application window to a size based on columns X lines of the te
    guest widget. Call with (0,0) for setting default size.
*/

void KonsoleApp::setColLin(int columns, int lines)
{
  if ((columns==0) || (lines==0))
  {
    if (b_fixedSize || defaultSize.isEmpty())
    {
      // not in config file : set default value
      columns = 80;
      lines = 24;
    }
  }

  if ((columns==0) || (lines==0))
  {
    resize(defaultSize);
  }
  else
  {
    if (b_fixedSize)
       te->setFixedSize(columns, lines);
    else
       te->setSize(columns, lines);
    adjustSize();
    if (b_fixedSize)
      setFixedSize(sizeHint());
    notifySize(columns, lines);  // set menu items
  }
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void KonsoleApp::configureRequest(TEWidget* _te, int state, int x, int y)
{
   if (!m_menuCreated)
      makeGUI();
  TDEPopupMenu *menu = (state & ControlButton) ? m_session : m_rightButton;
  if (menu)
     menu->popup(_te->mapToGlobal(TQPoint(x,y)));
}

void KonsoleApp::slotTabContextMenu(TQWidget* _te, const TQPoint & pos)
{
  if (!m_menuCreated)
      makeGUI();

  m_contextMenuSession = sessions.at( tabwidget->indexOf( _te ) );
  m_tabMonitorActivity->setChecked( m_contextMenuSession->isMonitorActivity() );
  m_tabMonitorSilence->setChecked( m_contextMenuSession->isMonitorSilence() );
  m_tabMasterMode->setChecked( m_contextMenuSession->isMasterMode() );

  m_tabPopupTabsMenu->clear();
  int counter=0;
  for (TESession *ses = sessions.first(); ses; ses = sessions.next()) {
    TQString title=ses->Title();
    m_tabPopupTabsMenu->insertItem(SmallIcon(ses->IconName()),title.replace('&',"&&"),counter++);
  }

  m_tabPopupMenu->popup( pos );
}

void KonsoleApp::slotTabDetachSession() {
  detachSession( m_contextMenuSession );
}

void KonsoleApp::slotTabRenameSession() {
  renameSession(m_contextMenuSession);
}

void KonsoleApp::slotTabSelectColor()
{
  TQColor color = tabwidget->tabColor( m_contextMenuSession->widget() );
  int result = KColorDialog::getColor( color, tabwidget->colorGroup().foreground() );

  if ( result == KColorDialog::Accepted ) {
    tabwidget->setTabColor(m_contextMenuSession->widget(), color);
  }
}

void KonsoleApp::slotTabResetColor()
{
  tabwidget->resetTabColor(m_contextMenuSession->widget());
}

void KonsoleApp::slotTabToggleMonitor()
{
  m_contextMenuSession->setMonitorActivity( m_tabMonitorActivity->isChecked() );
  m_contextMenuSession->setMonitorSilence( m_tabMonitorSilence->isChecked() );
  notifySessionState( m_contextMenuSession, NOTIFYNORMAL );
  if (m_contextMenuSession==se) {
    monitorActivity->setChecked( m_tabMonitorActivity->isChecked() );
    monitorSilence->setChecked( m_tabMonitorSilence->isChecked() );
  }
}

void KonsoleApp::slotTabToggleMasterMode()
{
  setMasterMode( m_tabMasterMode->isChecked(), m_contextMenuSession );
}

void KonsoleApp::slotTabCloseSession()
{
  confirmCloseCurrentSession(m_contextMenuSession);
}

void KonsoleApp::slotTabbarContextMenu(const TQPoint & pos)
{
   if (!m_menuCreated)
      makeGUI();

  if ( m_tabbarPopupMenu ) m_tabbarPopupMenu->popup( pos );
}

void KonsoleApp::slotTabSetViewOptions(int mode)
{
  m_tabViewMode = TabViewModes(mode);

  for(int i = 0; i < tabwidget->count(); i++) {

    TQWidget *page = tabwidget->page(i);
    TQIconSet icon = iconSetForSession(sessions.at(i));
    TQString title;
    if (b_matchTabWinTitle)
      title = sessions.at(i)->fullTitle();
    else
      title = sessions.at(i)->Title();

    title=title.replace('&',"&&");
    switch(mode) {
      case ShowIconAndText:
        tabwidget->changeTab(page, icon, title);
        break;
      case ShowTextOnly:
        tabwidget->changeTab(page, TQIconSet(), title);
        break;
      case ShowIconOnly:
        tabwidget->changeTab(page, icon, TQString::null);
        break;
    }
  }
}

void KonsoleApp::slotToggleAutoResizeTabs()
{
  b_autoResizeTabs = !b_autoResizeTabs;

  tabwidget->setAutomaticResizeTabs( b_autoResizeTabs );
}

void KonsoleApp::slotTabbarToggleDynamicHide()
{
  b_dynamicTabHide=!b_dynamicTabHide;
  if (b_dynamicTabHide && tabwidget->count()==1)
    tabwidget->setTabBarHidden(true);
  else
    tabwidget->setTabBarHidden(false);
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Configuration                                                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void KonsoleApp::slotSaveSessionsProfile()
{
  bool ok;

  TQString prof = KInputDialog::getText( i18n( "Save Sessions Profile" ),
      i18n( "Enter name under which the profile should be saved:" ),
      TQString::null, &ok, this );
  if ( ok ) {
    TQString path = locateLocal( "data",
        TQString::fromLatin1( "konsole/profiles/" ) + prof,
        TDEGlobal::instance() );

    if ( TQFile::exists( path ) )
      TQFile::remove( path );

    TDESimpleConfig cfg( path );
    savePropertiesInternal(&cfg,1);
    saveMainWindowSettings(&cfg);
  }
}

void KonsoleApp::saveProperties(TDEConfig* config) {
  uint counter=0;
  uint active=0;
  TQString key;

  if (config != TDEGlobal::config())
  {
     // called by the session manager
     config->writeEntry("numSes",sessions.count());
     sessions.first();
     while(counter < sessions.count())
     {
        key = TQString("Title%1").arg(counter);

        config->writeEntry(key, sessions.current()->Title());
        key = TQString("Schema%1").arg(counter);
        config->writeEntry(key, colors->find( sessions.current()->schemaNo() )->relPath());
	key = TQString("Encoding%1").arg(counter);
	config->writeEntry(key, sessions.current()->encodingNo());
        key = TQString("Args%1").arg(counter);
        config->writeEntry(key, sessions.current()->getArgs());
        key = TQString("Pgm%1").arg(counter);
        config->writeEntry(key, sessions.current()->getPgm());
        key = TQString("SessionFont%1").arg(counter);
        config->writeEntry(key, (sessions.current()->widget())->getVTFont());
        key = TQString("Term%1").arg(counter);
        config->writeEntry(key, sessions.current()->Term());
        key = TQString("KeyTab%1").arg(counter);
        config->writeEntry(key, sessions.current()->keymap());
        key = TQString("Icon%1").arg(counter);
        config->writeEntry(key, sessions.current()->IconName());
        key = TQString("MonitorActivity%1").arg(counter);
        config->writeEntry(key, sessions.current()->isMonitorActivity());
        key = TQString("MonitorSilence%1").arg(counter);
        config->writeEntry(key, sessions.current()->isMonitorSilence());
        key = TQString("MasterMode%1").arg(counter);
        config->writeEntry(key, sessions.current()->isMasterMode());
        key = TQString("TabColor%1").arg(counter);
        config->writeEntry(key, tabwidget->tabColor((sessions.current())->widget()));
        key = TQString("History%1").arg(counter);
        config->writeEntry(key, sessions.current()->history().getSize());
        key = TQString("HistoryEnabled%1").arg(counter);
        config->writeEntry(key, sessions.current()->history().isOn());

        TQString cwd=sessions.current()->getCwd();
        if (cwd.isEmpty())
          cwd=sessions.current()->getInitial_cwd();
        key = TQString("Cwd%1").arg(counter);
        config->writePathEntry(key, cwd);

        if (sessions.current()==se)
	  active=counter;
        sessions.next();
        counter++;
     }
  }
  else
  {
     config->setDesktopGroup();
     config->writeEntry("TabColor", tabwidget->tabColor(se->widget()));
  }
  config->writeEntry("Fullscreen",b_fullscreen);
  config->writeEntry("defaultfont", (se->widget())->getVTFont());
  s_tdeconfigSchema = colors->find( se->schemaNo() )->relPath();
  config->writeEntry("schema",s_tdeconfigSchema);
  config->writeEntry("scrollbar",n_scroll);
  config->writeEntry("tabbar",n_tabbar);
  config->writeEntry("bellmode",n_bell);
  config->writeEntry("keytab",KeyTrans::find(n_defaultKeytab)->id());
  config->writeEntry("ActiveSession", active);
  config->writeEntry("DefaultSession", m_defaultSessionFilename);
  config->writeEntry("TabViewMode", int(m_tabViewMode));
  config->writeEntry("DynamicTabHide", b_dynamicTabHide);
  config->writeEntry("AutoResizeTabs", b_autoResizeTabs);

  if (selectSetEncoding)
  {
    TQString encoding = TDEGlobal::charsets()->encodingForName(selectSetEncoding->currentText());
    config->writeEntry("EncodingName", encoding);
  } else {    // This will not always work (ie 'winsami' saves as 'ws2')
    if (se) config->writeEntry("EncodingName", se->encoding());
  }

  if (se) {
    config->writeEntry("history", se->history().getSize());
    config->writeEntry("historyenabled", b_histEnabled);
  }

  config->writeEntry("class",name());
  if (config != TDEGlobal::config())
  {
      saveMainWindowSettings(config);
  }

  if (!s_workDir.isEmpty())
    config->writePathEntry("workdir", s_workDir);

  // Set the new default font
  defaultFont = se->widget()->getVTFont();
}


// Called by constructor (with config = TDEGlobal::config())
// and by session-management (with config = sessionconfig).
// So it has to apply the settings when reading them.
void KonsoleApp::readProperties(TDEConfig* config)
{
    readProperties(config, TQString::null, false);
}

// If --type option was given, load the corresponding schema instead of
// default
//
// When globalConfigOnly is true only the options that are shared among all
// konsoles are being read.
void KonsoleApp::readProperties(TDEConfig* config, const TQString &schema, bool globalConfigOnly)
{
   if (config==TDEGlobal::config())
   {
     config->setDesktopGroup();
     b_warnQuit=config->readBoolEntry( "WarnQuit", true );
     b_allowResize=config->readBoolEntry( "AllowResize", false);
     b_bidiEnabled = config->readBoolEntry("EnableBidi",false);
     s_word_seps= config->readEntry("wordseps",":@-./_~");
     b_framevis = config->readBoolEntry("has frame",true);
     b_metaAsAlt = config->readBoolEntry("metaAsAltMode",false);
     b_realTransparency = config->readBoolEntry("RealTransparency",false);

     Atom kde_wm_transparent_to_desktop;
     kde_wm_transparent_to_desktop = XInternAtom(tqt_xdisplay(), "_TDE_TRANSPARENT_TO_DESKTOP", False);
     if (b_realTransparency)
     {
       XDeleteProperty(tqt_xdisplay(), winId(), kde_wm_transparent_to_desktop);
     }
     else
     {
       // Signal that we want to be transparent to the desktop, not to windows behind us...
       XChangeProperty(tqt_xdisplay(), winId(), kde_wm_transparent_to_desktop, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
     }

     TQPtrList<TEWidget> tes = activeTEs();
     for (TEWidget *_te = tes.first(); _te; _te = tes.next()) {
       _te->setWordCharacters(s_word_seps);
       _te->setTerminalSizeHint( config->readBoolEntry("TerminalSizeHint",false) );
       _te->setFrameStyle( b_framevis?(TQFrame::WinPanel|TQFrame::Sunken):TQFrame::NoFrame );
       _te->setBlinkingCursor(config->readBoolEntry("BlinkingCursor",false));
       _te->setCtrlDrag(config->readBoolEntry("CtrlDrag",true));
       _te->setCutToBeginningOfLine(config->readBoolEntry("CutToBeginningOfLine",false));
       _te->setLineSpacing( config->readUnsignedNumEntry( "LineSpacing", 0 ) );
       _te->setBidiEnabled(b_bidiEnabled);
     }

     monitorSilenceSeconds=config->readUnsignedNumEntry("SilenceSeconds", 10);
     for (TESession *ses = sessions.first(); ses; ses = sessions.next())
     {
       ses->setMetaAsAltMode(b_metaAsAlt);
       ses->setMonitorSilenceSeconds(monitorSilenceSeconds);
     }

     b_xonXoff = config->readBoolEntry("XonXoff",false);
     b_matchTabWinTitle = config->readBoolEntry("MatchTabWinTitle",false);
     b_mouseWheelScroll = config->readBoolEntry("TabsCycleWheel",true);
     b_menuAccelerators = config->readBoolEntry("MenuAccelerators",false);
     config->setGroup("UTMP");
     b_addToUtmp = config->readBoolEntry("AddToUtmp",true);
     config->setDesktopGroup();
   }

   if (!globalConfigOnly)
   {
      n_defaultKeytab=KeyTrans::find(config->readEntry("keytab","default"))->numb(); // act. the keytab for this session
      b_fullscreen = config->readBoolEntry("Fullscreen",false);
      n_scroll   = TQMIN(config->readUnsignedNumEntry("scrollbar",TEWidget::SCRRIGHT),2);
      n_tabbar   = TQMIN(config->readUnsignedNumEntry("tabbar",TabBottom),2);
      n_bell = TQMIN(config->readUnsignedNumEntry("bellmode",TEWidget::BELLSYSTEM),3);

      // Options that should be applied to all sessions /////////////

      // (1) set menu items and Konsole members

      TQFont tmpFont = TDEGlobalSettings::fixedFont();
      defaultFont = config->readFontEntry("defaultfont", &tmpFont);

      //set the schema
      s_tdeconfigSchema=config->readEntry("schema");
      ColorSchema* sch = colors->find(schema.isEmpty() ? s_tdeconfigSchema : schema);
      if (!sch)
      {
         sch = (ColorSchema*)colors->at(0);  //the default one
         kdWarning() << "Could not find schema named " <<s_tdeconfigSchema<<"; using "<<sch->relPath()<<endl;
         s_tdeconfigSchema = sch->relPath();
      }
      if (sch->hasSchemaFileChanged()) sch->rereadSchemaFile();
      s_schema = sch->relPath();
      curr_schema = sch->numb();
      pmPath = sch->imagePath();

      if (te) {
        if (sch->useTransparency())
        {
          if (!argb_visual) {
             if (!rootxpms[te])
               rootxpms.insert( te, new KRootPixmap(te) );
             rootxpms[te]->setFadeEffect(sch->tr_x(), TQColor(sch->tr_r(), sch->tr_g(), sch->tr_b()));
          }
          else {
            te->setBlendColor(tqRgba(sch->tr_r(), sch->tr_g(), sch->tr_b(), int(sch->tr_x() * 255)));
            te->setErasePixmap( TQPixmap() ); // make sure any background pixmap is unset
          }
        }
        else
        {
           if (rootxpms[te]) {
             delete rootxpms[te];
             rootxpms.remove(te);
           }
           pixmap_menu_activated(sch->alignment());
        }

        te->setColorTable(sch->table()); //FIXME: set twice here to work around a bug
        te->setColorTable(sch->table());
        te->setScrollbarLocation(n_scroll);
        te->setBellMode(n_bell);
      }

      // History
      m_histSize = config->readNumEntry("history",DEFAULT_HISTORY_SIZE);
      b_histEnabled = config->readBoolEntry("historyenabled",true);

      // Tab View Mode
      m_tabViewMode = TabViewModes(config->readNumEntry("TabViewMode", ShowIconAndText));
      b_dynamicTabHide = config->readBoolEntry("DynamicTabHide", false);
      b_autoResizeTabs = config->readBoolEntry("AutoResizeTabs", false);

      s_encodingName = config->readEntry( "EncodingName", "" ).lower();

      // The scrollbar location only needs to be changed when the given
      // profile scrollbar entry differs from the konsolerc scrollbar entry.
      TQPtrList<TEWidget> tes = activeTEs();
      for (TEWidget *_te = tes.first(); _te; _te = tes.next()) {
        if (_te->getScrollbarLocation() != n_scroll)
           _te->setScrollbarLocation(n_scroll);
      }
   }

   if (m_menuCreated)
   {
      applySettingsToGUI();
      activateSession();
   };
}

void KonsoleApp::applySettingsToGUI()
{
   if (!m_menuCreated) return;
   if (m_options)
   {
//      setFont();
      notifySize(te->Columns(), te->Lines());
      selectTabbar->setCurrentItem(n_tabbar);
      showMenubar->setChecked(!menuBar()->isHidden());
      selectScrollbar->setCurrentItem(n_scroll);
      selectBell->setCurrentItem(n_bell);
      selectSetEncoding->setCurrentItem( se->encodingNo() );
      updateRMBMenu();
   }
   updateKeytabMenu();
   tabwidget->setAutomaticResizeTabs( b_autoResizeTabs );
}


/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void KonsoleApp::bookmarks_menu_check()
{
  bool state = false;
  if ( se )
      state = !(se->getCwd().isEmpty());

  TDEAction *addBookmark = actionCollection()->action( "add_bookmark" );
  if ( !addBookmark )
  {
      return;
  }
  addBookmark->setEnabled( state );
}

void KonsoleApp::pixmap_menu_activated(int item, TEWidget* tewidget)
{
  if (!tewidget)
    tewidget=te;
  if (item <= 1) pmPath = "";
  TQPixmap pm(pmPath);
  if (pm.isNull()) {
    pmPath = "";
    item = 1;
    tewidget->setBackgroundColor(tewidget->getDefaultBackColor());
    return;
  }
  // FIXME: respect scrollbar (instead of te->size)
  n_render= item;
  switch (item)
  {
    case 1: // none
    case 2: // tile
            tewidget->setBackgroundPixmap(pm);
    break;
    case 3: // center
            { TQPixmap bgPixmap;
              bgPixmap.resize(tewidget->size());
              bgPixmap.fill(tewidget->getDefaultBackColor());
              bitBlt( &bgPixmap, ( tewidget->size().width() - pm.width() ) / 2,
                                ( tewidget->size().height() - pm.height() ) / 2,
                      &pm, 0, 0,
                      pm.width(), pm.height() );

              tewidget->setBackgroundPixmap(bgPixmap);
            }
    break;
    case 4: // full
            {
              float sx = (float)tewidget->size().width() / pm.width();
              float sy = (float)tewidget->size().height() / pm.height();
              TQWMatrix matrix;
              matrix.scale( sx, sy );
              tewidget->setBackgroundPixmap(pm.xForm( matrix ));
            }
    break;
    default: // oops
             n_render = 1;
  }
}

void KonsoleApp::slotSelectBell() {
  n_bell = selectBell->currentItem();
  te->setBellMode(n_bell);
}

void KonsoleApp::slotSelectScrollbar() {
   if (m_menuCreated)
      n_scroll = selectScrollbar->currentItem();

   TQPtrList<TEWidget> tes = activeTEs();
   for (TEWidget *_te = tes.first(); _te; _te = tes.next())
     _te->setScrollbarLocation(n_scroll);
   activateSession(); // maybe helps in bg
}

void KonsoleApp::checkBitmapFonts()
{
    {
        TQFont f;
        f.setRawName("-misc-console-medium-r-normal--16-160-72-72-c-80-iso10646-1");
        TQFontInfo fi( f );
        if ( !fi.exactMatch() )
            b_installBitmapFonts = true;
    }
    {
        TQFont f;
        f.setRawName("-misc-fixed-medium-r-normal--15-140-75-75-c-90-iso10646-1");
        TQFontInfo fi( f );
        if ( !fi.exactMatch() )
            b_installBitmapFonts = true;
    }
}

// In KDE 3.5, Konsole only allows the user to pick a font via
// TDEFontDialog.  This causes problems with the bdf/pcf files
// distributed with Konsole (console8x16 and 9x15).
void KonsoleApp::slotInstallBitmapFonts()
{
    if ( !b_installBitmapFonts )
        return;

    TQStringList sl_installFonts;
    {
        TQFont f;
        f.setRawName("-misc-console-medium-r-normal--16-160-72-72-c-80-iso10646-1");
        TQFontInfo fi( f );
        if ( !fi.exactMatch() )
            sl_installFonts << "console8x16.pcf.gz";
    }
    {
        TQFont f;
        f.setRawName("-misc-fixed-medium-r-normal--15-140-75-75-c-90-iso10646-1");
        TQFontInfo fi( f );
        if ( !fi.exactMatch() )
            sl_installFonts << "9x15.pcf.gz";
    }

    if ( !sl_installFonts.isEmpty() )
    {
        if ( KMessageBox::questionYesNoList( this,
            i18n( "If you want to use the bitmap fonts distributed with Konsole, they must be installed.  After installation, you must restart Konsole to use them.  Do you want to install the fonts listed below into fonts:/Personal?" ),
            sl_installFonts,
            i18n( "Install Bitmap Fonts?" ),
            KGuiItem( i18n("&Install" ) ),
            i18n("Do Not Install") ) == KMessageBox::Yes )
        {
            for ( TQStringList::iterator it = sl_installFonts.begin();
                  it != sl_installFonts.end(); ++it )
            {
                TQString sf = "fonts/" + *it;
                if ( TDEIO::NetAccess::copy( locate( "appdata", sf ),
                                            "fonts:/Personal/", 0 ) )
                {
                    b_installBitmapFonts = false;
                    // TODO: Remove the Install from the Fonts sub-menu.
                } else {
                    KMessageBox::error( this, i18n( "Could not install %1 into fonts:/Personal/" ).arg( *it ), i18n( "Error" ) );
                }
            }
        }
    }

}

void KonsoleApp::slotSelectFont() {
   if ( !se ) return;

   TQFont font = se->widget()->getVTFont();
   if ( TDEFontDialog::getFont( font, true ) != TQDialog::Accepted )
      return;

   se->widget()->setVTFont( font );
//  activateSession(); // activates the current
}

void KonsoleApp::schema_menu_activated(int item)
{
  if (!se) return;
  setSchema(item);
  activateSession(); // activates the current
}

void KonsoleApp::schema_menu_check()
{
        if (colors->checkSchemas())
        {
                colors->sort();
                updateSchemaMenu();
        }
}

void KonsoleApp::updateSchemaMenu()
{
  m_schema->clear();
  for (int i = 0; i < (int) colors->count(); i++)
  {
     ColorSchema* s = (ColorSchema*)colors->at(i);
    assert( s );
    TQString title=s->title();
    m_schema->insertItem(title.replace('&',"&&"),s->numb(),0);
  }

  if (te && se)
  {
        m_schema->setItemChecked(se->schemaNo(),true);
  }

}

void KonsoleApp::updateKeytabMenu()
{
  if (m_menuCreated)
  {
     m_keytab->setItemChecked(n_keytab,false);
     m_keytab->setItemChecked(se->keymapNo(),true);
  };
  n_keytab = se->keymapNo();
}

void KonsoleApp::keytab_menu_activated(int item)
{
  se->setKeymapNo(item);
  n_defaultKeytab = item;
  updateKeytabMenu();
}

/**
     Toggle the Menubar visibility
 */
void KonsoleApp::slotToggleMenubar() {
  if ( showMenubar->isChecked() )
     menubar->show();
  else
     menubar->hide();
  if (b_fixedSize)
  {
     adjustSize();
     setFixedSize(sizeHint());
  }
  if (!showMenubar->isChecked()) {
    setCaption(i18n("Use the right mouse button to bring back the menu"));
    TQTimer::singleShot(5000,this,TQ_SLOT(updateTitle()));
  }
  updateRMBMenu();
}

void KonsoleApp::initTEWidget(TEWidget* new_te, TEWidget* default_te)
{
  new_te->setWordCharacters(default_te->wordCharacters());
  new_te->setTerminalSizeHint(default_te->isTerminalSizeHint());
  new_te->setTerminalSizeStartup(false);
  new_te->setFrameStyle(b_framevis?(TQFrame::WinPanel|TQFrame::Sunken):TQFrame::NoFrame);
  new_te->setBlinkingCursor(default_te->blinkingCursor());
  new_te->setCtrlDrag(default_te->ctrlDrag());
  new_te->setCutToBeginningOfLine(default_te->cutToBeginningOfLine());
  new_te->setLineSpacing(default_te->lineSpacing());
  new_te->setBidiEnabled(b_bidiEnabled);

  new_te->setVTFont(default_te->font());
  new_te->setScrollbarLocation(n_scroll);
  new_te->setBellMode(default_te->bellMode());

  new_te->setMinimumSize(150,70);
}

void KonsoleApp::createSessionTab(TEWidget *widget, const TQIconSet &iconSet,
                               const TQString &text, int index)
{
  switch(m_tabViewMode) {
  case ShowIconAndText:
    tabwidget->insertTab(widget, iconSet, text, index);
    break;
  case ShowTextOnly:
    tabwidget->insertTab(widget, TQIconSet(), text, index);
    break;
  case ShowIconOnly:
    tabwidget->insertTab(widget, iconSet, TQString::null, index);
    break;
  }
}

TQIconSet KonsoleApp::iconSetForSession(TESession *session) const
{
  if (m_tabViewMode == ShowTextOnly)
    return TQIconSet();
  return SmallIconSet(session->isMasterMode() ? "remote" : session->IconName());
}


/**
    Toggle the Tabbar visibility
 */
void KonsoleApp::slotSelectTabbar() {
   if (m_menuCreated)
      n_tabbar = selectTabbar->currentItem();

   if ( n_tabbar == TabNone ) {     // Hide tabbar
      tabwidget->setTabBarHidden( true );
   } else {
      if ( tabwidget->isTabBarHidden() )
         tabwidget->setTabBarHidden( false );
      if ( n_tabbar == TabTop )
         tabwidget->setTabPosition( TQTabWidget::Top );
      else
         tabwidget->setTabPosition( TQTabWidget::Bottom );
   }

/* FIXME: Still necessary ? */
      TQPtrDictIterator<KRootPixmap> it(rootxpms);
      for (;it.current();++it)
        it.current()->repaint(true);

  if (b_fixedSize)
  {
     adjustSize();
     setFixedSize(sizeHint());
  }
}

void KonsoleApp::slotSaveSettings()
{
  TDEConfig *config = TDEGlobal::config();
  config->setDesktopGroup();
  saveProperties(config);
  saveMainWindowSettings(config);
  config->sync();
}

void KonsoleApp::slotConfigureNotifications()
{
   KNotifyDialog::configure(this, "Notification Configuration Dialog");
}

void KonsoleApp::slotConfigureKeys()
{
  KKeyDialog::configure(m_shortcuts);
  m_shortcuts->writeShortcutSettings();

  TQStringList ctrlKeys;

  for ( uint i = 0; i < m_shortcuts->count(); i++ )
  {
    TDEShortcut shortcut = (m_shortcuts->action( i ))->shortcut();
    for( uint j = 0; j < shortcut.count(); j++)
    {
      const KKey &key = shortcut.seq(j).key(0); // First Key of KeySequence
      if (key.modFlags() == KKey::CTRL)
        ctrlKeys += key.toString();
    }

    // Are there any shortcuts for Session Menu entries?
    if ( !b_sessionShortcutsEnabled &&
         m_shortcuts->action( i )->shortcut().count() &&
         TQString(m_shortcuts->action( i )->name()).startsWith("SSC_") ) {
      b_sessionShortcutsEnabled = true;
      TDEConfigGroup group(TDEGlobal::config(), "General");
      group.writeEntry("SessionShortcutsEnabled", true);
    }
  }

  if (!ctrlKeys.isEmpty())
  {
    ctrlKeys.sort();
    KMessageBox::informationList( this, i18n( "You have chosen one or more Ctrl+<key> combinations to be used as shortcuts. "
                                               "As a result these key combinations will no longer be passed to the command shell "
                                               "or to applications that run inside Konsole. "
                                               "This can have the unintended consequence that functionality that would otherwise be "
                                               "bound to these key combinations is no longer accessible."
                                               "\n\n"
                                               "You may wish to reconsider your choice of keys and use Alt+Ctrl+<key> or Ctrl+Shift+<key> instead."
                                               "\n\n"
                                               "You are currently using the following Ctrl+<key> combinations:" ),
                                               ctrlKeys,
                                               i18n( "Choice of Shortcut Keys" ), 0);
  }
}

void KonsoleApp::slotConfigure()
{
  TQStringList args;
  args << "kcmkonsole";
  TDEApplication::tdeinitExec( "tdecmshell", args );
}

void KonsoleApp::reparseConfiguration()
{
  TDEGlobal::config()->reparseConfiguration();
  readProperties(TDEGlobal::config(), TQString::null, true);

  // The .desktop files may have been changed by user...
  b_sessionShortcutsMapped = false;

  // Mappings may have to be changed...get a fresh mapper.
  disconnect( sessionNumberMapper, TQ_SIGNAL( mapped( int ) ),
          this, TQ_SLOT( newSessionTabbar( int ) ) );
  delete sessionNumberMapper;
  sessionNumberMapper = new TQSignalMapper( this );
  connect( sessionNumberMapper, TQ_SIGNAL( mapped( int ) ),
          this, TQ_SLOT( newSessionTabbar( int ) ) );

  sl_sessionShortCuts.clear();
  buildSessionMenus();

  // FIXME: Should be a better way to traverse TDEActionCollection
  uint count = m_shortcuts->count();
  for ( uint i = 0; i < count; i++ )
  {
    TDEAction* action = m_shortcuts->action( i );
    bool b_foundSession = false;
    if ( TQString(action->name()).startsWith("SSC_") ) {
      TQString name = TQString(action->name());

      // Check to see if shortcut's session has been loaded.
      for ( TQStringList::Iterator it = sl_sessionShortCuts.begin(); it != sl_sessionShortCuts.end(); ++it ) {
        if ( TQString::compare( *it, name ) == 0 ) {
          b_foundSession = true;
          break;
        }
      }
      if ( ! b_foundSession ) {
        action->setShortcut( TDEShortcut() );   // Clear shortcut
        m_shortcuts->writeShortcutSettings();
        delete action;           // Remove Action and Accel
        if ( i == 0 ) i = 0;     // Reset index
        else i--;
        count--;                 // = m_shortcuts->count();
      }
    }
  }

  m_shortcuts->readShortcutSettings();

  // User may have changed Schema->Set as default schema
  s_tdeconfigSchema = TDEGlobal::config()->readEntry("schema");
  ColorSchema* sch = colors->find(s_tdeconfigSchema);
  if (!sch)
  {
     sch = (ColorSchema*)colors->at(0);  //the default one
     kdWarning() << "Could not find schema named " <<s_tdeconfigSchema<<"; using "<<sch->relPath()<<endl;
     s_tdeconfigSchema = sch->relPath();
  }
  if (sch->hasSchemaFileChanged()) sch->rereadSchemaFile();
  s_schema = sch->relPath();
  curr_schema = sch->numb();
  pmPath = sch->imagePath();

  for (TESession *_se = sessions.first(); _se; _se = sessions.next())
  {
     _se->setMetaAsAltMode(b_metaAsAlt);
     ColorSchema* s = colors->find( _se->schemaNo() );
     if (s) {
       if (s->hasSchemaFileChanged())
         s->rereadSchemaFile();
       setSchema(s,_se->widget());
     }
  }

  tabwidget->setMouseWheelScroll(b_mouseWheelScroll);
  setMenuAcceleratos();
}

// Called via emulation via session
void KonsoleApp::changeTabTextColor( TESession* ses, int rgb )
{
    if ( !ses ) return;
    TQColor color;
    color.setRgb( rgb );
    if ( !color.isValid() ) {
        kdWarning()<<" Invalid RGB color "<<rgb<<endl;
        return;
    }
    tabwidget->setTabColor( ses->widget(), color );
}

// Called from emulation
void KonsoleApp::changeColLin(int columns, int lines)
{
  if (b_allowResize && !b_fixedSize) {
    setColLin(columns, lines);
    te->update();
  }
}

// Called from emulation
void KonsoleApp::changeColumns(int columns)
{
  if (b_allowResize) {
    setColLin(columns,te->Lines());
    te->update();
  }
}

void KonsoleApp::slotSelectSize() {
    int item = selectSize->currentItem();
    if (b_fullscreen)
       setFullScreen( false );

    switch (item) {
    case 0: setColLin(40,15); break;
    case 1: setColLin(80,24); break;
    case 2: setColLin(80,25); break;
    case 3: setColLin(80,40); break;
    case 4: setColLin(80,52); break;
    case 6: SizeDialog dlg(te->Columns(), te->Lines(), this);
            if (dlg.exec())
              setColLin(dlg.columns(),dlg.lines());
            break;
   }
}

void KonsoleApp::notifySize(int columns, int lines)
{
  if (selectSize)
  {
    selectSize->blockSignals(true);
    selectSize->setCurrentItem(-1);
    if (columns==40&&lines==15)
        selectSize->setCurrentItem(0);
    else if (columns==80&&lines==24)
        selectSize->setCurrentItem(1);
    else if (columns==80&&lines==25)
        selectSize->setCurrentItem(2);
    else if (columns==80&&lines==40)
        selectSize->setCurrentItem(3);
    else if (columns==80&&lines==52)
        selectSize->setCurrentItem(4);
    else
        selectSize->setCurrentItem(5);
    selectSize->blockSignals(false);
  }

  if (n_render >= 3) pixmap_menu_activated(n_render);
}

void KonsoleApp::updateTitle(TESession* _se)
{
  if ( !_se )
    _se = se;

  if (_se == se)
  {
    setCaption( _se->fullTitle() );
    setIconText( _se->IconText() );
  }
  tabwidget->setTabIconSet(_se->widget(), iconSetForSession(_se));
  TQString icon = _se->IconName();
  TDERadioAction *ra = session2action.find(_se);
  if (ra && (ra->icon() != icon))
    ra->setIcon(icon);
  if (m_tabViewMode == ShowIconOnly)
    tabwidget->changeTab( _se->widget(), TQString::null );
  else if (b_matchTabWinTitle)
    tabwidget->setTabLabel( _se->widget(), _se->fullTitle().replace('&',"&&"));
}

void KonsoleApp::initSessionFont(TQFont font) {
  te->setVTFont( font );
}

void KonsoleApp::initSessionKeyTab(const TQString &keyTab) {
  se->setKeymap(keyTab);
  updateKeytabMenu();
}

void KonsoleApp::initFullScreen()
{
  //This function is to be called from main.C to initialize the state of the Konsole (fullscreen or not).  It doesn't appear to work
  //from inside the Konsole constructor
  if (b_fullscreen) {
    setColLin(0,0);
  }
  setFullScreen(b_fullscreen);
}

void KonsoleApp::toggleFullScreen()
{
  setFullScreen(!b_fullscreen);
}

bool KonsoleApp::fullScreen()
{
  return b_fullscreen;
}

void KonsoleApp::setFullScreen(bool on)
{
  if( on )
      showFullScreen();
  else {
      if( isFullScreen()) // showNormal() may also do unminimize, unmaximize etc. :(
          showNormal();
  }
}

// don't call this directly
void KonsoleApp::updateFullScreen( bool on )
{
  b_fullscreen = on;
  if( on )
    showFullScreen();
  else {
    if( isFullScreen()) // showNormal() may also do unminimize, unmaximize etc. :(
        showNormal();
    updateTitle(); // restore caption of window
  }
  updateRMBMenu();
  te->setFrameStyle( b_framevis && !b_fullscreen ?(TQFrame::WinPanel|TQFrame::Sunken):TQFrame::NoFrame );
}

/* --| sessions |------------------------------------------------------------ */

//FIXME: activating sessions creates a lot flicker in the moment.
//       it comes from setting the attributes of a session individually.
//       ONE setImage call should actually be enough to match all cases.
//       These can be quite different:
//       - The screen size might have changed while the session was
//         detached. A propagation of the resize should in this case
//         make the drawEvent.
//       - font, background image and color palette should be set in one go.

void KonsoleApp::disableMasterModeConnections()
{
  TQPtrListIterator<TESession> from_it(sessions);
  for (; from_it.current(); ++from_it) {
    TESession *from = from_it.current();
    if (from->isMasterMode()) {
      TQPtrListIterator<TESession> to_it(sessions);
      for (; to_it.current(); ++to_it) {
        TESession *to = to_it.current();
        if (to!=from)
        {
          disconnect(from->widget(), TQ_SIGNAL(keyPressedSignal(TQKeyEvent*)),
                     to->getEmulation(), TQ_SLOT(onKeyPress(TQKeyEvent*)));
          disconnect(from->widget(), TQ_SIGNAL(keyReleasedSignal(TQKeyEvent*)),
                     to->getEmulation(), TQ_SLOT(onKeyReleased(TQKeyEvent*)));
          disconnect(from->widget(), TQ_SIGNAL(focusInSignal(TQFocusEvent*)),
                     to->getEmulation(), TQ_SLOT(onFocusIn(TQFocusEvent*)));
        }
      }
    }
  }
}

void KonsoleApp::enableMasterModeConnections()
{
  TQPtrListIterator<TESession> from_it(sessions);
  for (; from_it.current(); ++from_it) {
    TESession *from = from_it.current();
    if (from->isMasterMode()) {
      TQPtrListIterator<TESession> to_it(sessions);
      for (; to_it.current(); ++to_it) {
        TESession *to = to_it.current();
        if (to!=from)
        {
          connect(from->widget(), TQ_SIGNAL(keyPressedSignal(TQKeyEvent*)),
                  to->getEmulation(), TQ_SLOT(onKeyPress(TQKeyEvent*)));
          connect(from->widget(), TQ_SIGNAL(keyReleasedSignal(TQKeyEvent*)),
                  to->getEmulation(), TQ_SLOT(onKeyReleased(TQKeyEvent*)));
          connect(from->widget(), TQ_SIGNAL(focusInSignal(TQFocusEvent*)),
                  to->getEmulation(), TQ_SLOT(onFocusIn(TQFocusEvent*)));
        }
      }
    }
    from->setListenToKeyPress(true);
  }
}

void KonsoleApp::feedAllSessions(const TQString &text)
{
  if (!te) return;
  bool oldMasterMode = se->isMasterMode();
  setMasterMode(true);
  te->emitText(text);
  if (!oldMasterMode)
    setMasterMode(false);
}

void KonsoleApp::sendAllSessions(const TQString &text)
{
  TQString newtext=text;
  newtext.append("\r");
  feedAllSessions(newtext);
}

KURL KonsoleApp::baseURL() const
{
   KURL url;
   url.setPath(se->getCwd()+"/");
   return url;
}

void KonsoleApp::enterURL(const TQString& URL, const TQString&)
{
  TQString path, login, host, newtext;

  if (URL.startsWith("file:")) {
    KURL uglyurl(URL);
    newtext=uglyurl.path();
    KRun::shellQuote(newtext);
    te->emitText("cd "+newtext+"\r");
  }
  else if (URL.contains("://", true)) {
    KURL u(URL);
    newtext = u.protocol();
    bool isSSH = (newtext == "ssh");
    if (u.port() && isSSH)
       newtext += " -p " + TQString().setNum(u.port());
    if (u.hasUser())
       newtext += " -l " + u.user();

    /*
     * If we have a host, connect.
     */
    if (u.hasHost()) {
      newtext = newtext + " " + u.host();
      if (u.port() && !isSSH)
         newtext += TQString(" %1").arg(u.port());
      se->setUserTitle(31,"");           // we don't know remote cwd
      te->emitText(newtext + "\r");
    }
  }
  else
    te->emitText(URL);
}

void KonsoleApp::slotClearTerminal()
{
  if (se) {
    se->getEmulation()->clearEntireScreen();
    se->getEmulation()->clearSelection();
  }
}

void KonsoleApp::slotResetClearTerminal()
{
  if (se) {
    se->getEmulation()->reset();
    se->getEmulation()->clearSelection();
  }
}

void KonsoleApp::sendSignal(int sn)
{
  if (se) se->sendSignal(sn);
}

void KonsoleApp::runSession(TESession* s)
{
    TDERadioAction *ra = session2action.find(s);
    ra->setChecked(true);
    activateSession(s);

    // give some time to get through the
    // resize events before starting up.
    TQTimer::singleShot(100,s,TQ_SLOT(run()));
}

void KonsoleApp::addSession(TESession* s)
{
  TQString newTitle = s->Title();

  bool nameOk;
  int count = 1;
  do {
     nameOk = true;
     for (TESession *ses = sessions.first(); ses; ses = sessions.next())
     {
        if (newTitle == ses->Title())
        {
           nameOk = false;
           break;
        }
     }
     if (!nameOk)
     {
       count++;
       newTitle = i18n("abbreviation of number","%1 No. %2").arg(s->Title()).arg(count);
     }
  }
  while (!nameOk);

  s->setTitle(newTitle);

  // create an action for the session
  TDERadioAction *ra = new TDERadioAction(newTitle.replace('&',"&&"),
                                      s->IconName(),
                                      0,
                                      this,
                                      TQ_SLOT(activateSession()),
                                      m_shortcuts);
  ra->setExclusiveGroup("sessions");
  ra->setChecked(true);

  action2session.insert(ra, s);
  session2action.insert(s,ra);
  sessions.append(s);
  if (sessions.count()>1) {
     if (!m_menuCreated)
        makeGUI();
     m_detachSession->setEnabled(true);
  }

  if (m_menuCreated)
     ra->plug(m_view);

  createSessionTab(te, SmallIconSet(s->IconName()), newTitle);
  setSchema(s->schemaNo());
  tabwidget->setCurrentPage(tabwidget->count()-1);
  disableMasterModeConnections(); // no duplicate connections, remove old
  enableMasterModeConnections();
  if( m_removeSessionButton )
    m_removeSessionButton->setEnabled(tabwidget->count()>1);
}

TQString KonsoleApp::currentSession()
{
  return se->SessionId();
}

TQString KonsoleApp::sessionId(const int position)
{
  if (position<=0 || position>(int)sessions.count())
    return "";

  return sessions.at(position-1)->SessionId();
}

void KonsoleApp::listSessions()
{
  int counter=0;
  m_sessionList->clear();
  m_sessionList->insertTitle(i18n("Session List"));
  m_sessionList->setKeyboardShortcutsEnabled(true);
  for (TESession *ses = sessions.first(); ses; ses = sessions.next()) {
    TQString title=ses->Title();
    m_sessionList->insertItem(SmallIcon(ses->IconName()),title.replace('&',"&&"),counter++);
  }
  m_sessionList->adjustSize();
  m_sessionList->popup(mapToGlobal(TQPoint((width()/2)-(m_sessionList->width()/2),(height()/2)-(m_sessionList->height()/2))));
}

void KonsoleApp::switchToSession()
{
  activateSession( TQString( sender()->name() ).right( 2 ).toInt() -1 );
}

void KonsoleApp::activateSession(int position)
{
  if (position<0 || position>=(int)sessions.count())
    return;
  activateSession( sessions.at(position) );
}

void KonsoleApp::activateSession(TQWidget* w)
{
  activateSession(tabwidget->indexOf(w));
  w->setFocus();
}

void KonsoleApp::activateSession(const TQString &sessionId)
{
  TESession* activate=NULL;

  sessions.first();
  while(sessions.current())
  {
    if (sessions.current()->SessionId()==sessionId)
      activate=sessions.current();
    sessions.next();
  }

  if (activate)
    activateSession( activate );
}

/**
   Activates a session from the menu
 */
void KonsoleApp::activateSession()
{
  TESession* s = NULL;
  // finds the session based on which button was activated
  TQPtrDictIterator<TESession> it( action2session ); // iterator for dict
  while ( it.current() )
  {
    TDERadioAction *ra = (TDERadioAction*)it.currentKey();
    if (ra->isChecked()) { s = it.current(); break; }
    ++it;
  }
  if (s!=NULL) activateSession(s);
}

void KonsoleApp::activateSession(TESession *s)
{
  if (se)
  {
     se->setConnect(false);
     se->setListenToKeyPress(true);
     notifySessionState(se,NOTIFYNORMAL);
     // Delete the session if isn't in the session list any longer.
     if (sessions.find(se) == -1)
        delete se;
  }
  if (se != s)
     se_previous = se;
  se = s;

  // Set the required schema variables for the current session
  ColorSchema* cs = colors->find( se->schemaNo() );
  if (!cs)
      cs = (ColorSchema*)colors->at(0);  //the default one
  s_schema = cs->relPath();
  curr_schema = cs->numb();
  pmPath = cs->imagePath();
  n_render = cs->alignment();

// BR 106464 temporary fix...
//  only 2 sessions opened, 2nd session viewable, right-click on 1st tab and
//  select 'Detach', close original Konsole window... crash
//  s is not set properly on original Konsole window
  TDERadioAction *ra = session2action.find(se);
  if (!ra) {
    se=sessions.first();        // Get new/correct TESession
    ra = session2action.find(se);
  }
  ra->setChecked(true);

  TQTimer::singleShot(1,this,TQ_SLOT(allowPrevNext())); // hack, hack, hack

  tabwidget->showPage( se->widget() );
  te = se->widget();
  if (m_menuCreated) {
    if (selectBell) selectBell->setCurrentItem(te->bellMode());
    updateSchemaMenu();
  }

  if (rootxpms[te])
    rootxpms[te]->start();
  notifySize(te->Columns(), te->Lines()); // set menu items
  se->setConnect(true);
  updateTitle();
  if (!m_menuCreated)
     return;

  if (selectSetEncoding) selectSetEncoding->setCurrentItem(se->encodingNo());
  updateKeytabMenu(); // act. the keytab for this session
  if (m_clearHistory) m_clearHistory->setEnabled( se->history().isOn() );
  if (m_findHistory) m_findHistory->setEnabled( se->history().isOn() );
  if (m_findNext) m_findNext->setEnabled( se->history().isOn() );
  if (m_findPrevious) m_findPrevious->setEnabled( se->history().isOn() );
  se->getEmulation()->findTextBegin();
  if (m_saveHistory) m_saveHistory->setEnabled( se->history().isOn() );
  if (monitorActivity) monitorActivity->setChecked( se->isMonitorActivity() );
  if (monitorSilence) monitorSilence->setChecked( se->isMonitorSilence() );
  masterMode->setChecked( se->isMasterMode() );
  sessions.find(se);
  uint position=sessions.at();
  if (m_moveSessionLeft) m_moveSessionLeft->setEnabled(position>0);
  if (m_moveSessionRight) m_moveSessionRight->setEnabled(position<sessions.count()-1);
}

void KonsoleApp::slotUpdateSessionConfig(TESession *session)
{
  if (session == se)
     activateSession(se);
}

void KonsoleApp::slotResizeSession(TESession *session, TQSize size)
{
  TESession *oldSession = se;
  if (se != session)
     activateSession(session);
  setColLin(size.width(), size.height());
  activateSession(oldSession);
}

// Called by newSession and DCOP function below
void KonsoleApp::setSessionEncoding( const TQString &encoding, TESession *session )
{
    if ( encoding.isEmpty() )
        return;

    if ( !session )
        session = se;

    bool found = false;
    TQString enc = TDEGlobal::charsets()->encodingForName(encoding);
    TQTextCodec * qtc = TDEGlobal::charsets()->codecForName(enc, found);
    if ( !found || !qtc )
        return;

    // Encoding was found; now try to figure out which Encoding menu item
    // it corresponds to.
    int i = 0;
    bool found_encoding = false;
    TQStringList encodingNames = TDEGlobal::charsets()->descriptiveEncodingNames();
    TQStringList::ConstIterator it = encodingNames.begin();
    TQString t_encoding = encoding.lower();

    while ( it != encodingNames.end() && !found_encoding )
    {
      if ( TQString::compare( TDEGlobal::charsets()->encodingForName(*it),
                             t_encoding ) == 0 ) {
         found_encoding = true;
      }
      i++; it++;
    }

    // BR114535 : Remove jis7 due to infinite loop.
    if ( enc == "jis7" ) {
      kdWarning()<<"Encoding Japanese (jis7) currently does not work!  BR114535"<<endl;
      return;
    }

    if ( found_encoding )
    {
      session->setEncodingNo( i );
      session->getEmulation()->setCodec(qtc);
      if (se == session)
        activateSession(se);
    }
}

// Called via DCOP only
void KonsoleApp::slotSetSessionEncoding(TESession *session, const TQString &encoding)
{
   setSessionEncoding( encoding, session );
}

void KonsoleApp::slotGetSessionSchema(TESession *session, TQString &schema)
{
  int no = session->schemaNo();
  ColorSchema* s = colors->find( no );
  schema = s->relPath();
}

void KonsoleApp::slotSetSessionSchema(TESession *session, const TQString &schema)
{
  ColorSchema* s = colors->find( schema );
  setSchema(s, session->widget());
}

void KonsoleApp::allowPrevNext()
{
  if (!se) return;
  notifySessionState(se,NOTIFYNORMAL);
}

TDESimpleConfig *KonsoleApp::defaultSession()
{
  if (!m_defaultSession) {
    TDEConfig * config = TDEGlobal::config();
    config->setDesktopGroup();
    setDefaultSession(config->readEntry("DefaultSession","shell.desktop"));
  }
  return m_defaultSession;
}

void KonsoleApp::setDefaultSession(const TQString &filename)
{
  delete m_defaultSession;
  m_defaultSession = new TDESimpleConfig(locate("appdata", filename), true /* read only */);
  m_defaultSession->setDesktopGroup();
  b_showstartuptip = m_defaultSession->readBoolEntry("Tips", true);

  m_defaultSessionFilename=filename;
}

void KonsoleApp::newSession(const TQString &pgm, const TQStrList &args, const TQString &term, const TQString &icon, const TQString &title, const TQString &cwd)
{
  TDESimpleConfig *co = defaultSession();
  newSession(co, pgm, args, term, icon, title, cwd);
}

TQString KonsoleApp::newSession()
{
  TDESimpleConfig *co = defaultSession();
  return newSession(co, TQString::null, TQStrList());
}

void KonsoleApp::newSession(int i)
{
  if (i == SESSION_NEW_WINDOW_ID)
  {
    // TODO: "type" isn't passed properly
    KonsoleApp* konsole = new KonsoleApp(name(), b_histEnabled, !menubar->isHidden(), n_tabbar != TabNone, b_framevis,
                                   n_scroll != TEWidget::SCRNONE, 0, false, 0);
    konsole->newSession();
    konsole->enableFullScripting(b_fullScripting);
    konsole->enableFixedSize(b_fixedSize);
    konsole->setColLin(0,0); // Use defaults
    konsole->initFullScreen();
    konsole->show();
    return;
  }

  TDESimpleConfig* co = no2command.find(i);
  if (co) {
    newSession(co);
    resetScreenSessions();
  }
}

void KonsoleApp::newSessionTabbar(int i)
{
  if (i == SESSION_NEW_WINDOW_ID)
  {
    // TODO: "type" isn't passed properly
    KonsoleApp* konsole = new KonsoleApp(name(), b_histEnabled, !menubar->isHidden(), n_tabbar != TabNone, b_framevis,
                                   n_scroll != TEWidget::SCRNONE, 0, false, 0);
    konsole->newSession();
    konsole->enableFullScripting(b_fullScripting);
    konsole->enableFixedSize(b_fixedSize);
    konsole->setColLin(0,0); // Use defaults
    konsole->initFullScreen();
    konsole->show();
    return;
  }

  TDESimpleConfig* co = no2command.find(i);
  if (co) {
    newSession(co);
    resetScreenSessions();
  }
}

TQString KonsoleApp::newSession(const TQString &type)
{
  TDESimpleConfig *co;
  if (type.isEmpty())
     co = defaultSession();
  else
     co = new TDESimpleConfig(locate("appdata", type + ".desktop"), true /* read only */);
  return newSession(co);
}

TQString KonsoleApp::newSession(TDESimpleConfig *co, TQString program, const TQStrList &args,
                            const TQString &_term,const TQString &_icon,
                            const TQString &_title, const TQString &_cwd)
{
  TQString emu = "xterm";
  TQString icon = "konsole";
  TQString key;
  TQString sch = s_tdeconfigSchema;
  TQString txt;
  TQString cwd;
  TQFont font = defaultFont;
  TQStrList cmdArgs;

  if (co) {
     co->setDesktopGroup();
     emu = co->readEntry("Term", emu);
     key = co->readEntry("KeyTab", key);
     sch = co->readEntry("Schema", sch);
     txt = co->readEntry("Name");
     font = co->readFontEntry("SessionFont", &font);
     icon = co->readEntry("Icon", icon);
     cwd = co->readPathEntry("Cwd");
  }

  if (!_term.isEmpty())
     emu = _term;

  if (!_icon.isEmpty())
     icon = _icon;

  if (!_title.isEmpty())
     txt = _title;

  // apply workdir only when the session config does not have a directory
  if (cwd.isEmpty())
     cwd = s_workDir;
  // bookmarks take precedence over workdir
  // however, --workdir option has precedence in the very first session
  if (!_cwd.isEmpty())
     cwd = _cwd;

  if (!program.isEmpty()) {
     cmdArgs = args;
  }
  else {
     program = TQFile::decodeName(konsole_shell(cmdArgs));

     if (co) {
        co->setDesktopGroup();
        TQString cmd = co->readPathEntry("Exec");
        if (!cmd.isEmpty()) {
          cmdArgs.append("-c");
          cmdArgs.append(TQFile::encodeName(cmd));
        }
     }
  }

  ColorSchema* schema = colors->find(sch);
  if (!schema)
      schema=(ColorSchema*)colors->at(0);  //the default one
  int schmno = schema->numb();

  if (sessions.count()==1 && n_tabbar!=TabNone)
    tabwidget->setTabBarHidden( false );

  TEWidget* te_old = te;
  te=new TEWidget(tabwidget);

  connect( te, TQ_SIGNAL(configureRequest(TEWidget*, int, int, int)),
           this, TQ_SLOT(configureRequest(TEWidget*,int,int,int)) );
  if (te_old) {
    initTEWidget(te, te_old);
  }
  else {
    readProperties(TDEGlobal::config(), "", true);
    te->setVTFont(font);
    te->setScrollbarLocation(n_scroll);
    te->setBellMode(n_bell);
  }

  te->setMinimumSize(150,70);

  TQString sessionId="session-"+TQString::number(++sessionIdCounter);
  TESession* s = new TESession(te, emu,winId(),sessionId,cwd);
  s->setProgram(TQFile::encodeName(program),cmdArgs);
  s->setMonitorSilenceSeconds(monitorSilenceSeconds);
  s->enableFullScripting(b_fullScripting);
  s->setMetaAsAltMode(b_metaAsAlt);

  // If you add any new signal-slot connection below, think about doing it in konsolePart too
  connect( s,TQ_SIGNAL(done(TESession*)),
           this,TQ_SLOT(doneSession(TESession*)));
  connect( s, TQ_SIGNAL(updateTitle(TESession*)),
           this, TQ_SLOT(updateTitle(TESession*)));
  connect( s, TQ_SIGNAL(notifySessionState(TESession*, int)),
           this, TQ_SLOT(notifySessionState(TESession*, int)));
  connect( s, TQ_SIGNAL(disableMasterModeConnections()),
           this, TQ_SLOT(disableMasterModeConnections()));
  connect( s, TQ_SIGNAL(enableMasterModeConnections()),
           this, TQ_SLOT(enableMasterModeConnections()));
  connect( s, TQ_SIGNAL(renameSession(TESession*,const TQString&)),
           this, TQ_SLOT(slotRenameSession(TESession*, const TQString&)));
  connect( s->getEmulation(), TQ_SIGNAL(changeColumns(int)),
           this, TQ_SLOT(changeColumns(int)) );
  connect( s->getEmulation(), TQ_SIGNAL(changeColLin(int,int)),
           this, TQ_SLOT(changeColLin(int,int)));
  connect( s->getEmulation(), TQ_SIGNAL(ImageSizeChanged(int,int)),
           this, TQ_SLOT(notifySize(int,int)));
  connect( s, TQ_SIGNAL(zmodemDetected(TESession*)),
           this, TQ_SLOT(slotZModemDetected(TESession*)));
  connect( s, TQ_SIGNAL(updateSessionConfig(TESession*)),
           this, TQ_SLOT(slotUpdateSessionConfig(TESession*)));
  connect( s, TQ_SIGNAL(resizeSession(TESession*, TQSize)),
           this, TQ_SLOT(slotResizeSession(TESession*, TQSize)));
  connect( s, TQ_SIGNAL(setSessionEncoding(TESession*, const TQString &)),
           this, TQ_SLOT(slotSetSessionEncoding(TESession*, const TQString &)));
  connect( s, TQ_SIGNAL(getSessionSchema(TESession*, TQString &)),
           this, TQ_SLOT(slotGetSessionSchema(TESession*, TQString &)));
  connect( s, TQ_SIGNAL(setSessionSchema(TESession*, const TQString &)),
           this, TQ_SLOT(slotSetSessionSchema(TESession*, const TQString &)));
  connect( s, TQ_SIGNAL(changeTabTextColor(TESession*, int)),
           this,TQ_SLOT(changeTabTextColor(TESession*, int)));

  s->widget()->setVTFont(defaultFont);// Hack to set font again after newSession
  s->setSchemaNo(schmno);
  if (key.isEmpty())
    s->setKeymapNo(n_defaultKeytab);
  else {
    // TODO: Fixes BR77018, see BR83000.
    if (key.endsWith(".keytab"))
      key.remove(".keytab");
    s->setKeymap(key);
  }

  s->setTitle(txt);
  s->setIconName(icon);
  s->setAddToUtmp(b_addToUtmp);
  s->setXonXoff(b_xonXoff);

  if (b_histEnabled && m_histSize)
    s->setHistory(HistoryTypeBuffer(m_histSize));
  else if (b_histEnabled && !m_histSize)
    s->setHistory(HistoryTypeFile());
  else
    s->setHistory(HistoryTypeNone());

  setSessionEncoding( s_encodingName, s );

  addSession(s);
  runSession(s); // activate and run
  return sessionId;
}

/*
 * Starts a new session based on URL.
 */
void KonsoleApp::newSession(const TQString& sURL, const TQString& title)
{
   TQStrList args;
   TQString protocol, path, login, host;

   KURL url = KURL(sURL);
   if ((url.protocol() == "file") && (url.hasPath())) {
     TDESimpleConfig *co = defaultSession();
     path = url.path();
     newSession(co, TQString::null, TQStrList(), TQString::null, TQString::null,
                title.isEmpty() ? path : title, path);
     return;
   }
   else if ((!url.protocol().isEmpty()) && (url.hasHost())) {
     protocol = url.protocol();
     bool isSSH = (protocol == "ssh");
     args.append( protocol.latin1() ); /* argv[0] == command to run. */
     host = url.host();
     if (url.port() && isSSH) {
       args.append("-p");
       args.append(TQCString().setNum(url.port()));
     }
     if (url.hasUser()) {
       login = url.user();
       args.append("-l");
       args.append(login.latin1());
     }
     args.append(host.latin1());
     if (url.port() && !isSSH)
       args.append(TQCString().setNum(url.port()));
     newSession( NULL, protocol.latin1() /* protocol */, args /* arguments */,
                 TQString::null /*term*/, TQString::null /*icon*/,
 	        title.isEmpty() ? path : title /*title*/, TQString::null /*cwd*/);
     return;
   }
   /*
    * We can't create a session without a protocol.
    * We should ideally popup a warning.
    */
}

void KonsoleApp::confirmCloseCurrentSession( TESession* _se )
{
   if ( !_se )
      _se = se;
   if (KMessageBox::warningContinueCancel(this,
     i18n("Are you sure that you want to close the current session?"),
     i18n("Close Confirmation"), KGuiItem(i18n("C&lose Session"),"tab_remove"),
     "ConfirmCloseSession")==KMessageBox::Continue)
       _se->closeSession();
}

void KonsoleApp::closeCurrentSession()
{
  se->closeSession();
}

//FIXME: If a child dies during session swap,
//       this routine might be called before
//       session swap is completed.

void KonsoleApp::doneSession(TESession* s)
{

  if (s == se_previous)
    se_previous = 0;

  if (se_previous)
    activateSession(se_previous);

  TDERadioAction *ra = session2action.find(s);
  ra->unplug(m_view);
  tabwidget->removePage( s->widget() );
  if (rootxpms[s->widget()]) {
    delete rootxpms[s->widget()];
    rootxpms.remove(s->widget());
  }
  delete s->widget();
  if(m_removeSessionButton )
      m_removeSessionButton->setEnabled(tabwidget->count()>1);
  session2action.remove(s);
  action2session.remove(ra);
  int sessionIndex = sessions.findRef(s);
  sessions.remove(s);
  delete ra; // will the toolbar die?

  s->setConnect(false);
  delete s;

  if (s == se_previous)
    se_previous = 0;

  if (s == se)
  { // pick a new session
    se = 0;
    if (sessions.count())
    {
      se = sessions.at(sessionIndex ? sessionIndex - 1 : 0);

      session2action.find(se)->setChecked(true);
      //FIXME: this Timer stupidity originated from the connected
      //       design of Emulations. By this the newly activated
      //       session might get a Ctrl(D) if the session has be
      //       terminated by this keypress. A likely problem
      //       can be found in the CMD_prev/nextSession processing.
      //       Since the timer approach only works at good weather,
      //       the whole construction is not suited to what it
      //       should do. Affected is the TEEmulation::setConnect.
      TQTimer::singleShot(1,this,TQ_SLOT(activateSession()));
    }
    else
      close();
  }
  else {
    sessions.find(se);
    uint position=sessions.at();
    m_moveSessionLeft->setEnabled(position>0);
    m_moveSessionRight->setEnabled(position<sessions.count()-1);
  }
  if (sessions.count()==1) {
    m_detachSession->setEnabled(false);
    if (b_dynamicTabHide && !tabwidget->isTabBarHidden())
       tabwidget->setTabBarHidden(true);
  }
}

/*! Cycle to previous session (if any) */

void KonsoleApp::prevSession()
{
  sessions.find(se); sessions.prev();
  if (!sessions.current()) sessions.last();
  if (sessions.current() && sessions.count() > 1)
    activateSession(sessions.current());
}

/*! Cycle to next session (if any) */

void KonsoleApp::nextSession()
{
  sessions.find(se); sessions.next();
  if (!sessions.current()) sessions.first();
  if (sessions.current() && sessions.count() > 1)
    activateSession(sessions.current());
}

void KonsoleApp::slotMovedTab(int from, int to)
{

  TESession* _se = sessions.take(from);
  sessions.remove(_se);
  sessions.insert(to,_se);

  TDERadioAction *ra = session2action.find(_se);
  ra->unplug(m_view);
  ra->plug(m_view,(m_view->count()-sessions.count()+1)+to);

  if (to==tabwidget->currentPageIndex()) {
    if (!m_menuCreated)
      makeGUI();
    m_moveSessionLeft->setEnabled(to>0);
    m_moveSessionRight->setEnabled(to<(int)sessions.count()-1);
  }
}

/* Move session forward in session list if possible */
void KonsoleApp::moveSessionLeft()
{
  sessions.find(se);
  uint position=sessions.at();
  if (position==0)
    return;

  sessions.remove(position);
  sessions.insert(position-1,se);

  TDERadioAction *ra = session2action.find(se);
  ra->unplug(m_view);
  ra->plug(m_view,(m_view->count()-sessions.count()+1)+position-1);

  TQColor oldcolor = tabwidget->tabColor(se->widget());

  tabwidget->blockSignals(true);
  tabwidget->removePage(se->widget());
  tabwidget->blockSignals(false);
  TQString title = se->Title();
  createSessionTab(se->widget(), iconSetForSession(se),
                   title.replace('&', "&&"), position-1);
  tabwidget->showPage(se->widget());
  tabwidget->setTabColor(se->widget(),oldcolor);

  if (!m_menuCreated)
    makeGUI();
  m_moveSessionLeft->setEnabled(position-1>0);
  m_moveSessionRight->setEnabled(true);
}

/* Move session back in session list if possible */
void KonsoleApp::moveSessionRight()
{
  sessions.find(se);
  uint position=sessions.at();

  if (position==sessions.count()-1)
    return;

  sessions.remove(position);
  sessions.insert(position+1,se);

  TDERadioAction *ra = session2action.find(se);
  ra->unplug(m_view);
  ra->plug(m_view,(m_view->count()-sessions.count()+1)+position+1);

  TQColor oldcolor = tabwidget->tabColor(se->widget());

  tabwidget->blockSignals(true);
  tabwidget->removePage(se->widget());
  tabwidget->blockSignals(false);
  TQString title = se->Title();
  createSessionTab(se->widget(), iconSetForSession(se),
                   title.replace('&', "&&"), position+1);
  tabwidget->showPage(se->widget());
  tabwidget->setTabColor(se->widget(),oldcolor);

  if (!m_menuCreated)
    makeGUI();
  m_moveSessionLeft->setEnabled(true);
  m_moveSessionRight->setEnabled(position+1<sessions.count()-1);
}

void KonsoleApp::initMonitorActivity(bool state)
{
  monitorActivity->setChecked(state);
  slotToggleMonitor();
}

void KonsoleApp::initMonitorSilence(bool state)
{
  monitorSilence->setChecked(state);
  slotToggleMonitor();
}

void KonsoleApp::slotToggleMonitor()
{
  se->setMonitorActivity( monitorActivity->isChecked() );
  se->setMonitorSilence( monitorSilence->isChecked() );
  notifySessionState(se,NOTIFYNORMAL);
}

void KonsoleApp::initMasterMode(bool state)
{
  masterMode->setChecked(state);
  slotToggleMasterMode();
}

void KonsoleApp::initTabColor(TQColor color)
{
  if ( color.isValid() )
    tabwidget->setTabColor( se->widget(), color );
}

void KonsoleApp::initHistory(int lines, bool enable)
{
   // If no History#= is given in the profile, use the history
   // parameter saved in konsolerc.
   if ( lines < 0 ) lines = m_histSize;

   if ( enable && lines > 0 )
      se->setHistory( HistoryTypeBuffer( lines ) );
   else if ( enable )  // Unlimited buffer
      se->setHistory(HistoryTypeFile());
   else
      se->setHistory( HistoryTypeNone() );
}

void KonsoleApp::slotToggleMasterMode()
{
  setMasterMode( masterMode->isChecked() );
}

void KonsoleApp::setMasterMode(bool _state, TESession* _se)
{
  if (!_se)
    _se = se;
  if (_se->isMasterMode() == _state)
    return;

  if (_se==se)
    masterMode->setChecked( _state );

  disableMasterModeConnections();

  _se->setMasterMode( _state );

  if (_state)
    enableMasterModeConnections();

  notifySessionState(_se,NOTIFYNORMAL);
}

void KonsoleApp::notifySessionState(TESession* session, int state)
{
  TQString state_iconname;
  switch(state)
  {
    case NOTIFYNORMAL  : if(session->isMasterMode())
                           state_iconname = "remote";
                         else
                           state_iconname = session->IconName();
                         break;
    case NOTIFYBELL    : state_iconname = "bell";
                         break;
    case NOTIFYACTIVITY: state_iconname = "activity";
                         break;
    case NOTIFYSILENCE : state_iconname = "silence";
			 break;
  }
  if (!state_iconname.isEmpty()
      && session->testAndSetStateIconName(state_iconname)
      && m_tabViewMode != ShowTextOnly) {

    TQPixmap normal = TDEGlobal::instance()->iconLoader()->loadIcon(state_iconname,
                       TDEIcon::Small, 0, TDEIcon::DefaultState, 0L, true);
    TQPixmap active = TDEGlobal::instance()->iconLoader()->loadIcon(state_iconname,
                       TDEIcon::Small, 0, TDEIcon::ActiveState, 0L, true);

    // make sure they are not larger than 16x16
    if (normal.width() > 16 || normal.height() > 16)
      normal.convertFromImage(TQImage(normal.convertToImage()).smoothScale(16,16));
    if (active.width() > 16 || active.height() > 16)
      active.convertFromImage(TQImage(active.convertToImage()).smoothScale(16,16));

    TQIconSet iconset;
    iconset.setPixmap(normal, TQIconSet::Small, TQIconSet::Normal);
    iconset.setPixmap(active, TQIconSet::Small, TQIconSet::Active);

    tabwidget->setTabIconSet(session->widget(), iconset);
  }
}

// --| Session support |-------------------------------------------------------

void KonsoleApp::buildSessionMenus()
{
   m_session->clear();
   if (m_tabbarSessionsCommands)
      m_tabbarSessionsCommands->clear();

   loadSessionCommands();
   loadScreenSessions();

   createSessionMenus();

   if (tdeApp->authorizeTDEAction("file_print"))
   {
      m_session->insertSeparator();
      m_print->plug(m_session);
   }

   m_session->insertSeparator();
   m_closeSession->plug(m_session);

   m_session->insertSeparator();
   m_quit->plug(m_session);
}

static void insertItemSorted(TDEPopupMenu *menu, const TQIconSet &iconSet, const TQString &txt, int id)
{
  const int defaultId = SESSION_NEW_SHELL_ID; // The id of the 'new' item.
  int index = menu->indexOf(defaultId);
  int count = menu->count();
  if (index >= 0)
  {
     index++; // Skip New Window
     index++; // Skip separator
     while(true)
     {
        index++;
        if (index >= count)
        {
           index = -1; // Insert at end
           break;
        }
        if (menu->text(menu->idAt(index)) > txt)
           break; // Insert before this item
     }
  }
  menu->insertItem(iconSet, txt, id, index);
}

void KonsoleApp::addSessionCommand(const TQString &path)
{
  TDESimpleConfig* co;
  if (path.isEmpty())
    co = new TDESimpleConfig(locate("appdata", "shell.desktop"), true /* read only */);
  else
    co = new TDESimpleConfig(path,true);
  co->setDesktopGroup();
  TQString typ = co->readEntry("Type");
  TQString txt = co->readEntry("Name");

  // try to locate the binary
  TQString exec= co->readPathEntry("Exec");
  exec.remove(TQRegExp("^(sudo )?su (- )?-c ?\'"));
  exec.remove(TQRegExp("\'?$"));

  exec = KRun::binaryName(exec, false);
  exec = KShell::tildeExpand(exec);
  TQString pexec = TDEGlobal::dirs()->findExe(exec);

  if (typ.isEmpty() || txt.isEmpty() || typ != "KonsoleApplication"
      || ( !exec.isEmpty() && pexec.isEmpty() ) )
  {
    if (!path.isEmpty())
       delete co;
    kdWarning()<<"Unable to use "<<path.latin1()<<endl;
    return; // ignore
  }

  no2command.insert(++cmd_serial,co);

  // Add shortcuts only once and not for 'New Shell'.
  if ( ( b_sessionShortcutsMapped == true ) || ( cmd_serial == SESSION_NEW_SHELL_ID ) ) return;

  // Add an empty shortcut for each Session.
  TQString comment = co->readEntry("Comment");
  if (comment.isEmpty())
    comment=txt.prepend(i18n("New "));

  TQString name = comment;
  name.prepend("SSC_");  // Allows easy searching for Session ShortCuts
  name.replace(" ", "_");
  sl_sessionShortCuts << name;

  // Is there already this shortcut?
  TDEAction* sessionAction;
  if ( m_shortcuts->action( name.latin1() ) ) {
    sessionAction = m_shortcuts->action( name.latin1() );
  } else {
    sessionAction = new TDEAction( comment, 0, this, 0, m_shortcuts, name.latin1() );
  }
  connect( sessionAction, TQ_SIGNAL( activated() ), sessionNumberMapper, TQ_SLOT( map() ) );
  sessionNumberMapper->setMapping( sessionAction, cmd_serial );

}

void KonsoleApp::loadSessionCommands()
{
  no2command.clear();

  cmd_serial = 99;
  cmd_first_screen = -1;

  if (!tdeApp->authorize("shell_access"))
     return;

  addSessionCommand(TQString::null);

  TQStringList lst = TDEGlobal::dirs()->findAllResources("appdata", "*.desktop", false, true);

  for(TQStringList::Iterator it = lst.begin(); it != lst.end(); ++it )
    if (!(*it).endsWith("/shell.desktop"))
       addSessionCommand(*it);

  b_sessionShortcutsMapped = true;
}

void KonsoleApp::createSessionMenus()
{
  if (no2command.isEmpty()) { // All sessions have been deleted
    m_session->insertItem(SmallIconSet("window-new"),
                          i18n("New &Window"), SESSION_NEW_WINDOW_ID);
    m_tabbarSessionsCommands->insertItem(SmallIconSet("window-new"),
                          i18n("New &Window"), SESSION_NEW_WINDOW_ID);
    return;
  }

  TDESimpleConfig *cfg = no2command[SESSION_NEW_SHELL_ID];
  TQString txt = cfg->readEntry("Name");
  TQString icon = cfg->readEntry("Icon", "konsole");
  insertItemSorted(m_tabbarSessionsCommands, SmallIconSet(icon),
                   txt.replace('&',"&&"), SESSION_NEW_SHELL_ID );

  TQString comment = cfg->readEntry("Comment");
  if (comment.isEmpty())
    comment=txt.prepend(i18n("New "));
  insertItemSorted(m_session, SmallIconSet(icon),
                   comment.replace('&',"&&"), SESSION_NEW_SHELL_ID);
  m_session->insertItem(SmallIconSet("window-new"),
                        i18n("New &Window"), SESSION_NEW_WINDOW_ID);
  m_tabbarSessionsCommands->insertItem(SmallIconSet("window-new"),
                        i18n("New &Window"), SESSION_NEW_WINDOW_ID);
  m_session->insertSeparator();
  m_tabbarSessionsCommands->insertSeparator();

  TQIntDictIterator<TDESimpleConfig> it( no2command );
  for ( ; it.current(); ++it ) {
    if ( it.currentKey() == SESSION_NEW_SHELL_ID )
      continue;

    TQString txt = (*it).readEntry("Name");
    TQString icon = (*it).readEntry("Icon", "konsole");
    insertItemSorted(m_tabbarSessionsCommands, SmallIconSet(icon),
                     txt.replace('&',"&&"), it.currentKey() );
    TQString comment = (*it).readEntry("Comment");
    if (comment.isEmpty())
      comment=txt.prepend(i18n("New "));
    insertItemSorted(m_session, SmallIconSet(icon),
                     comment.replace('&',"&&"), it.currentKey());
  }

  if (m_bookmarksSession)
  {
    m_session->insertSeparator();
    m_session->insertItem(SmallIconSet("keditbookmarks"),
                          i18n("New Shell at Bookmark"), m_bookmarksSession);
    m_tabbarSessionsCommands->insertSeparator();
    m_tabbarSessionsCommands->insertItem(SmallIconSet("keditbookmarks"),
                          i18n("Shell at Bookmark"), m_bookmarksSession);
  }
}

void KonsoleApp::addScreenSession(const TQString &path, const TQString &socket)
{
  KTempFile *tmpFile = new KTempFile();
  tmpFile->setAutoDelete(true);
  TDESimpleConfig *co = new TDESimpleConfig(tmpFile->name());
  co->setDesktopGroup();
  co->writeEntry("Name", socket);
  TQString txt = i18n("Screen is a program controlling screens!", "Screen at %1").arg(socket);
  co->writeEntry("Comment", txt);
  co->writePathEntry("Exec", TQString::fromLatin1("SCREENDIR=%1 screen -r %2")
    .arg(path).arg(socket));
  TQString icon = "konsole";
  cmd_serial++;
  m_session->insertItem( SmallIconSet( icon ), txt, cmd_serial, cmd_serial - 1 );
  m_tabbarSessionsCommands->insertItem( SmallIconSet( icon ), txt, cmd_serial );
  no2command.insert(cmd_serial,co);
}

void KonsoleApp::loadScreenSessions()
{
  if (!tdeApp->authorize("shell_access"))
     return;
  TQCString screenDir = getenv("SCREENDIR");
  if (screenDir.isEmpty())
    screenDir = TQFile::encodeName(TQDir::homeDirPath()) + "/.screen/";
  // Some distributions add a shell function called screen that sets
  // $SCREENDIR to ~/tmp. In this case the variable won't be set here.
  if (!TQFile::exists(screenDir))
    screenDir = TQFile::encodeName(TQDir::homeDirPath()) + "/tmp/";
  TQStringList sessions;
  // Can't use TQDir as it doesn't support FIFOs :(
  DIR *dir = opendir(screenDir);
  if (dir)
  {
    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
      TQCString path = screenDir + "/" + entry->d_name;
      struct stat st;
      if (stat(path, &st) != 0)
        continue;

      int fd;
      if (S_ISFIFO(st.st_mode) && !(st.st_mode & 0111) && // xbit == attached
          (fd = open(path, O_WRONLY | O_NONBLOCK)) != -1)
      {
        ::close(fd);
        sessions.append(TQFile::decodeName(entry->d_name));
      }
    }
    closedir(dir);
  }
  resetScreenSessions();
  for (TQStringList::ConstIterator it = sessions.begin(); it != sessions.end(); ++it)
    addScreenSession(screenDir, *it);
}

void KonsoleApp::resetScreenSessions()
{
  if (cmd_first_screen == -1)
    cmd_first_screen = cmd_serial + 1;
  else
  {
    for (int i = cmd_first_screen; i <= cmd_serial; ++i)
    {
      m_session->removeItem(i);
      if (m_tabbarSessionsCommands)
         m_tabbarSessionsCommands->removeItem(i);
      no2command.remove(i);
    }
    cmd_serial = cmd_first_screen - 1;
  }
}

// --| Schema support |-------------------------------------------------------

void KonsoleApp::setSchema(int numb, TEWidget* tewidget)
{
  ColorSchema* s = colors->find(numb);
  if (!s)
  {
    s = (ColorSchema*)colors->at(0);
    kdWarning() << "No schema with serial #"<<numb<<", using "<<s->relPath()<<" (#"<<s->numb()<<")." << endl;
    s_tdeconfigSchema = s->relPath();
  }

  if (s->hasSchemaFileChanged())
  {
        const_cast<ColorSchema *>(s)->rereadSchemaFile();
  }
  if (s) setSchema(s, tewidget);
}

void KonsoleApp::setSchema(const TQString & path)
{
  ColorSchema* s = colors->find(path);
  if (!s)
  {
     s = (ColorSchema*)colors->at(0);  //the default one
     kdWarning() << "No schema with the name " <<path<<", using "<<s->relPath()<<endl;
     s_tdeconfigSchema = s->relPath();
  }
  if (s->hasSchemaFileChanged())
  {
        const_cast<ColorSchema *>(s)->rereadSchemaFile();
  }
  if (s) setSchema(s);
}

// Called via main.cpp for session manager.
void KonsoleApp::setEncoding(int index)
{
  if ( selectSetEncoding ) {
    selectSetEncoding->setCurrentItem(index);
    slotSetEncoding();
  }
}

void KonsoleApp::setSchema(ColorSchema* s, TEWidget* tewidget)
{
  if (!s) return;
  if (!tewidget) tewidget=te;

  if (tewidget==te) {
    if (m_schema)
    {
      m_schema->setItemChecked(curr_schema,false);
      m_schema->setItemChecked(s->numb(),true);
    }

    s_schema = s->relPath();
    curr_schema = s->numb();
    pmPath = s->imagePath();
  }
  tewidget->setColorTable(s->table()); //FIXME: set twice here to work around a bug

  if (s->useTransparency()) {
    if (!argb_visual) {
      if (!rootxpms[tewidget])
        rootxpms.insert( tewidget, new KRootPixmap(tewidget) );
      rootxpms[tewidget]->setFadeEffect(s->tr_x(), TQColor(s->tr_r(), s->tr_g(), s->tr_b()));
    } else {
      tewidget->setBlendColor(tqRgba(s->tr_r(), s->tr_g(), s->tr_b(), int(s->tr_x() * 255)));
      tewidget->setErasePixmap( TQPixmap() ); // make sure any background pixmap is unset
    }
  } else {
      if (rootxpms[tewidget]) {
        delete rootxpms[tewidget];
        rootxpms.remove(tewidget);
      }
       pixmap_menu_activated(s->alignment(), tewidget);
       tewidget->setBlendColor(tqRgba(0, 0, 0, 0xff));
  }

  tewidget->setColorTable(s->table());
  TQPtrListIterator<TESession> ses_it(sessions);
  for (; ses_it.current(); ++ses_it)
    if (tewidget==ses_it.current()->widget()) {
      ses_it.current()->setSchemaNo(s->numb());
      break;
    }
}

void KonsoleApp::slotDetachSession()
{
  detachSession();
}

void KonsoleApp::detachSession(TESession* _se) {
  if (!_se) _se=se;

  TDERadioAction *ra = session2action.find(_se);
  ra->unplug(m_view);
  TEWidget* se_widget = _se->widget();
  session2action.remove(_se);
  action2session.remove(ra);
  int sessionIndex = sessions.findRef(_se);
  sessions.remove(_se);
  delete ra;

  if ( _se->isMasterMode() ) {
    // Disable master mode when detaching master
    setMasterMode(false);
  } else {
    TQPtrListIterator<TESession> from_it(sessions);
    for(; from_it.current(); ++from_it) {
      TESession *from = from_it.current();
      if(from->isMasterMode())
      {
        disconnect(from->widget(), TQ_SIGNAL(keyPressedSignal(TQKeyEvent*)),
	            _se->getEmulation(), TQ_SLOT(onKeyPress(TQKeyEvent*)));
        disconnect(from->widget(), TQ_SIGNAL(keyReleasedSignal(TQKeyEvent*)),
              _se->getEmulation(), TQ_SLOT(onKeyReleased(TQKeyEvent*)));
        disconnect(from->widget(), TQ_SIGNAL(focusInSignal(TQFocusEvent*)),
              _se->getEmulation(), TQ_SLOT(onFocusIn(TQFocusEvent*)));
      }
    }
  }

  TQColor se_tabtextcolor = tabwidget->tabColor( _se->widget() );

  disconnect( _se,TQ_SIGNAL(done(TESession*)),
              this,TQ_SLOT(doneSession(TESession*)) );

  disconnect( _se->getEmulation(),TQ_SIGNAL(ImageSizeChanged(int,int)), this,TQ_SLOT(notifySize(int,int)));
  disconnect( _se->getEmulation(),TQ_SIGNAL(changeColLin(int, int)), this,TQ_SLOT(changeColLin(int,int)) );
  disconnect( _se->getEmulation(),TQ_SIGNAL(changeColumns(int)), this,TQ_SLOT(changeColumns(int)) );
  disconnect( _se, TQ_SIGNAL(changeTabTextColor(TESession*, int)), this, TQ_SLOT(changeTabTextColor(TESession*, int)) );

  disconnect( _se,TQ_SIGNAL(updateTitle(TESession*)), this,TQ_SLOT(updateTitle(TESession*)) );
  disconnect( _se,TQ_SIGNAL(notifySessionState(TESession*,int)), this,TQ_SLOT(notifySessionState(TESession*,int)) );
  disconnect( _se,TQ_SIGNAL(disableMasterModeConnections()), this,TQ_SLOT(disableMasterModeConnections()) );
  disconnect( _se,TQ_SIGNAL(enableMasterModeConnections()), this,TQ_SLOT(enableMasterModeConnections()) );
  disconnect( _se,TQ_SIGNAL(renameSession(TESession*,const TQString&)), this,TQ_SLOT(slotRenameSession(TESession*,const TQString&)) );

  // TODO: "type" isn't passed properly
  KonsoleApp* konsole = new KonsoleApp(name(), b_histEnabled, !menubar->isHidden(), n_tabbar != TabNone, b_framevis,
                                 n_scroll != TEWidget::SCRNONE, 0, false, 0);
  konsole->enableFullScripting(b_fullScripting);
  // TODO; Make this work: konsole->enableFixedSize(b_fixedSize);
  konsole->resize(size());
  konsole->show();
  konsole->attachSession(_se);
  konsole->activateSession(_se);
  konsole->changeTabTextColor( _se, se_tabtextcolor.rgb() );//restore prev color
  konsole->slotTabSetViewOptions(m_tabViewMode);

  if (_se==se) {
    if (se == se_previous)
      se_previous=NULL;

    // pick a new session
    if (se_previous)
      se = se_previous;
    else
      se = sessions.at(sessionIndex ? sessionIndex - 1 : 0);
    session2action.find(se)->setChecked(true);
    TQTimer::singleShot(1,this,TQ_SLOT(activateSession()));
  }

  if (sessions.count()==1)
    m_detachSession->setEnabled(false);

  tabwidget->removePage( se_widget );
  if (rootxpms[se_widget]) {
    delete rootxpms[se_widget];
    rootxpms.remove(se_widget);
  }
  delete se_widget;
  if (b_dynamicTabHide && tabwidget->count()==1)
    tabwidget->setTabBarHidden(true);

  if( m_removeSessionButton )
    m_removeSessionButton->setEnabled(tabwidget->count()>1);
}

void KonsoleApp::attachSession(TESession* session)
{
  if (b_dynamicTabHide && sessions.count()==1 && n_tabbar!=TabNone)
    tabwidget->setTabBarHidden(false);

  TEWidget* se_widget = session->widget();

  te=new TEWidget(tabwidget);

  connect( te, TQ_SIGNAL(configureRequest(TEWidget*, int, int, int)),
           this, TQ_SLOT(configureRequest(TEWidget*,int,int,int)) );

  te->resize(se_widget->size());
  te->setSize(se_widget->Columns(), se_widget->Lines());
  initTEWidget(te, se_widget);
  session->changeWidget(te);
  te->setFocus();
  createSessionTab(te, SmallIconSet(session->IconName()), session->Title());
  setSchema(session->schemaNo());
  if (session->isMasterMode()) {
    disableMasterModeConnections(); // no duplicate connections, remove old
    enableMasterModeConnections();
  }

  TQString title=session->Title();
  TDERadioAction *ra = new TDERadioAction(title.replace('&',"&&"), session->IconName(),
                                      0, this, TQ_SLOT(activateSession()), m_shortcuts);

  ra->setExclusiveGroup("sessions");
  ra->setChecked(true);

  action2session.insert(ra, session);
  session2action.insert(session,ra);
  sessions.append(session);
  if (sessions.count()>1)
    m_detachSession->setEnabled(true);

  if (m_menuCreated)
    ra->plug(m_view);

  connect( session,TQ_SIGNAL(done(TESession*)),
           this,TQ_SLOT(doneSession(TESession*)) );

  connect( session,TQ_SIGNAL(updateTitle(TESession*)), this,TQ_SLOT(updateTitle(TESession*)) );
  connect( session,TQ_SIGNAL(notifySessionState(TESession*,int)), this,TQ_SLOT(notifySessionState(TESession*,int)) );

  connect( session,TQ_SIGNAL(disableMasterModeConnections()), this,TQ_SLOT(disableMasterModeConnections()) );
  connect( session,TQ_SIGNAL(enableMasterModeConnections()), this,TQ_SLOT(enableMasterModeConnections()) );
  connect( session,TQ_SIGNAL(renameSession(TESession*,const TQString&)), this,TQ_SLOT(slotRenameSession(TESession*,const TQString&)) );
  connect( session->getEmulation(),TQ_SIGNAL(ImageSizeChanged(int,int)), this,TQ_SLOT(notifySize(int,int)));
  connect( session->getEmulation(),TQ_SIGNAL(changeColumns(int)), this,TQ_SLOT(changeColumns(int)) );
  connect( session->getEmulation(),TQ_SIGNAL(changeColLin(int, int)), this,TQ_SLOT(changeColLin(int,int)) );

  connect( session, TQ_SIGNAL(changeTabTextColor(TESession*, int)), this, TQ_SLOT(changeTabTextColor(TESession*, int)) );

  activateSession(session);
}

void KonsoleApp::setSessionTitle( TQString& title, TESession* ses )
{
   if ( !ses )
      ses = se;
   ses->setTitle( title );
   slotRenameSession( ses, title );
}

void KonsoleApp::renameSession(TESession* ses) {
  TQString title = ses->Title();
  bool ok;

  title = KInputDialog::getText( i18n( "Rename Session" ),
      i18n( "Session name:" ), title, &ok, this );

  if (!ok) return;

  ses->setTitle(title);
  slotRenameSession(ses,title);
}

void KonsoleApp::slotRenameSession() {
  renameSession(se);
}

void KonsoleApp::slotRenameSession(TESession* ses, const TQString &name)
{
  TDERadioAction *ra = session2action.find(ses);
  TQString title=name;
  title=title.replace('&',"&&");
  ra->setText(title);
  ra->setIcon( ses->IconName() ); // I don't know why it is needed here
  if (m_tabViewMode!=ShowIconOnly)
    tabwidget->setTabLabel( ses->widget(), title );
  updateTitle();
}


void KonsoleApp::slotClearAllSessionHistories() {
  for (TESession *_se = sessions.first(); _se; _se = sessions.next())
    _se->clearHistory();
}

//////////////////////////////////////////////////////////////////////

HistoryTypeDialog::HistoryTypeDialog(const HistoryType& histType,
                                     unsigned int histSize,
                                     TQWidget *parent)
  : KDialogBase(Plain, i18n("History Configuration"),
                Help | Default | Ok | Cancel, Ok,
                parent, 0, true, true)
{
  TQFrame *mainFrame = plainPage();

  TQHBoxLayout *hb = new TQHBoxLayout(mainFrame);

  m_btnEnable    = new TQCheckBox(i18n("&Enable"), mainFrame);
  connect(m_btnEnable, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotHistEnable(bool)));

  m_label = new TQLabel(i18n("&Number of lines: "), mainFrame);

  m_size = new TQSpinBox(0, 10 * 1000 * 1000, 100, mainFrame);
  m_size->setValue(histSize);
  m_size->setSpecialValueText(i18n("Unlimited (number of lines)", "Unlimited"));

  m_label->setBuddy( m_size );

  m_setUnlimited = new TQPushButton(i18n("&Set Unlimited"), mainFrame);
  connect( m_setUnlimited,TQ_SIGNAL(clicked()), this,TQ_SLOT(slotSetUnlimited()) );

  hb->addWidget(m_btnEnable);
  hb->addSpacing(10);
  hb->addWidget(m_label);
  hb->addWidget(m_size);
  hb->addSpacing(10);
  hb->addWidget(m_setUnlimited);

  if ( ! histType.isOn()) {
    m_btnEnable->setChecked(false);
    slotHistEnable(false);
  } else {
    m_btnEnable->setChecked(true);
    m_size->setValue(histType.getSize());
    slotHistEnable(true);
  }
  setHelp("configure-history");
}

void HistoryTypeDialog::slotDefault()
{
  m_btnEnable->setChecked(true);
  m_size->setValue(DEFAULT_HISTORY_SIZE);
  slotHistEnable(true);
}

void HistoryTypeDialog::slotHistEnable(bool b)
{
  m_label->setEnabled(b);
  m_size->setEnabled(b);
  m_setUnlimited->setEnabled(b);
  if (b) m_size->setFocus();
}

void HistoryTypeDialog::slotSetUnlimited()
{
  m_size->setValue(0);
}

unsigned int HistoryTypeDialog::nbLines() const
{
  return m_size->value();
}

bool HistoryTypeDialog::isOn() const
{
  return m_btnEnable->isChecked();
}

void KonsoleApp::slotHistoryType()
{
  if (!se) return;

  HistoryTypeDialog dlg(se->history(), m_histSize, this);
  if (dlg.exec()) {
    m_clearHistory->setEnabled( dlg.isOn() );
    m_findHistory->setEnabled( dlg.isOn() );
    m_findNext->setEnabled( dlg.isOn() );
    m_findPrevious->setEnabled( dlg.isOn() );
    m_saveHistory->setEnabled( dlg.isOn() );
    if (dlg.isOn()) {
      if (dlg.nbLines() > 0) {
         se->setHistory(HistoryTypeBuffer(dlg.nbLines()));
         m_histSize = dlg.nbLines();
         b_histEnabled = true;

      } else {

         se->setHistory(HistoryTypeFile());
         m_histSize = 0;
         b_histEnabled = true;

      }

    } else {

      se->setHistory(HistoryTypeNone());
      m_histSize = dlg.nbLines();
      b_histEnabled = false;

    }
  }
}

void KonsoleApp::slotClearHistory()
{
  se->clearHistory();
}

void KonsoleApp::slotFindHistory()
{
  if( !m_finddialog ) {
    m_finddialog = new KonsoleFind( this, "konsolefind", false);
    connect(m_finddialog,TQ_SIGNAL(search()),this,TQ_SLOT(slotFind()));
    connect(m_finddialog,TQ_SIGNAL(done()),this,TQ_SLOT(slotFindDone()));
  }

  TQString string;
  string = m_finddialog->getText();
  m_finddialog->setText(string.isEmpty() ? m_find_pattern : string);

  m_find_first = true;
  m_find_found = false;

  m_finddialog->show();
  m_finddialog->result();
}

void KonsoleApp::slotOpenSelection()
{
  delete m_filterData;

  m_openSelection->clear();
  disconnect(m_openSelection, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotOpenURI(int)));

  TQString selection = se->getEmulation()->getSelection();
  TQString curdir = baseURL().path();

  if ( TQFile::exists( curdir + selection ) ) {
     selectedURL = TQString(curdir + selection);
  } else {
     selectedURL = TQString(selection);
  }

  m_filterData = new KURIFilterData( selectedURL );
  KURIFilter::self()->filterURI( *(m_filterData) );
  m_openSelection->insertItem( SmallIconSet( m_filterData->iconName() ),i18n( "%1" ).arg(m_filterData->uri().url()), 1 );

  connect(m_openSelection, TQ_SIGNAL(activated(int)), TQ_SLOT(slotOpenURI(int)));
}

void KonsoleApp::slotOpenURI(int)
{
  (void) new KRun( m_filterData->uri() );
}

void KonsoleApp::slotFindNext()
{
  if( !m_finddialog ) {
    slotFindHistory();
    return;
  }

  TQString string;
  string = m_finddialog->getText();
  m_finddialog->setText(string.isEmpty() ? m_find_pattern : string);

  slotFind();
}

void KonsoleApp::slotFindPrevious()
{
  if( !m_finddialog ) {
    slotFindHistory();
    return;
  }

  TQString string;
  string = m_finddialog->getText();
  m_finddialog->setText(string.isEmpty() ? m_find_pattern : string);

  m_finddialog->setDirection( !m_finddialog->get_direction() );
  slotFind();
  m_finddialog->setDirection( !m_finddialog->get_direction() );
}

void KonsoleApp::slotFind()
{
  if (m_find_first) {
    se->getEmulation()->findTextBegin();
    m_find_first = false;
  }

  bool forward = !m_finddialog->get_direction();
  m_find_pattern = m_finddialog->getText();

  if (se->getEmulation()->findTextNext(m_find_pattern,forward,
                          m_finddialog->case_sensitive(),m_finddialog->reg_exp()))
    m_find_found = true;
  else
    if (m_find_found) {
      if (forward) {
        if ( KMessageBox::questionYesNo( m_finddialog,
             i18n("End of history reached.\n" "Continue from the beginning?"),
  	     i18n("Find"), KStdGuiItem::cont(), KStdGuiItem::cancel() ) == KMessageBox::Yes ) {
          m_find_first = true;
    	  slotFind();
        }
      }
      else {
        if ( KMessageBox::questionYesNo( m_finddialog,
             i18n("Beginning of history reached.\n" "Continue from the end?"),
  	     i18n("Find"), KStdGuiItem::cont(), KStdGuiItem::cancel() ) == KMessageBox::Yes ) {
          m_find_first = true;
   	  slotFind();
        }
      }
    }
  else
    KMessageBox::information( m_finddialog,
    	i18n( "Search string '%1' not found." ).arg(KStringHandler::csqueeze(m_find_pattern)),
	i18n( "Find" ) );
}

void KonsoleApp::slotFindDone()
{
  if (!m_finddialog)
    return;

  se->getEmulation()->clearSelection();
  m_finddialog->hide();
}

void KonsoleApp::slotSaveHistory()
{
  // FIXME - mostLocalURL can't handle non-existing files yet, so this
  //         code doesn't work.
  KURL s_url = KFileDialog::getSaveURL(TQString::null, TQString::null, 0L, i18n("Save History"));
  if( s_url.isEmpty())
      return;
  KURL url = TDEIO::NetAccess::mostLocalURL( s_url, 0 );

  if( !url.isLocalFile() ) {
    KMessageBox::sorry(this, i18n("This is not a local file.\n"));
    return;
  }

  int query = KMessageBox::Continue;
  TQFileInfo info;
  TQString name( url.path() );
  info.setFile( name );
  if( info.exists() )
    query = KMessageBox::warningContinueCancel( this,
      i18n( "A file with this name already exists.\nDo you want to overwrite it?" ), i18n("File Exists"), i18n("Overwrite") );

  if (query==KMessageBox::Continue) {
    TQFile file(url.path());
    if(!file.open(IO_WriteOnly)) {
      KMessageBox::sorry(this, i18n("Unable to write to file."));
      return;
    }

    TQTextStream textStream(&file);
    assert( se && se->getEmulation() );
    se->getEmulation()->streamHistory( &textStream );

    file.close();
    if(file.status()) {
      KMessageBox::sorry(this, i18n("Could not save history."));
      return;
    }
  }
}

void KonsoleApp::slotZModemUpload()
{
  if (se->zmodemIsBusy())
  {
    KMessageBox::sorry(this,
         i18n("<p>The current session already has a ZModem file transfer in progress."));
    return;
  }
  TQString zmodem = TDEGlobal::dirs()->findExe("sz");
  if (zmodem.isEmpty())
    zmodem = TDEGlobal::dirs()->findExe("lsz");
  if (zmodem.isEmpty())
  {
    KMessageBox::sorry(this,
                   i18n("<p>No suitable ZModem software was found on "
                        "the system.\n"
                        "<p>You may wish to install the 'rzsz' or 'lrzsz' package.\n"));
    return;
  }

  TQStringList files = KFileDialog::getOpenFileNames(TQString::null, TQString::null, this,
  	i18n("Select Files to Upload"));
  if (files.isEmpty())
    return;

  se->startZModem(zmodem, TQString::null, files);
}

void KonsoleApp::slotZModemDetected(TESession *session)
{
  if (!tdeApp->authorize("zmodem_download")) return;

  if(se != session)
    activateSession(session);

  TQString zmodem = TDEGlobal::dirs()->findExe("rz");
  if (zmodem.isEmpty())
    zmodem = TDEGlobal::dirs()->findExe("lrz");
  if (zmodem.isEmpty())
  {
    KMessageBox::information(this,
                   i18n("<p>A ZModem file transfer attempt has been detected, "
                        "but no suitable ZModem software was found on "
                        "the system.\n"
                        "<p>You may wish to install the 'rzsz' or 'lrzsz' package.\n"));
    return;
  }
  KURLRequesterDlg dlg(TDEGlobalSettings::documentPath(),
                   i18n("A ZModem file transfer attempt has been detected.\n"
                        "Please specify the folder you want to store the file(s):"),
                   this, "zmodem_dlg");
  dlg.setButtonOK(KGuiItem( i18n("&Download"),
                       i18n("Start downloading file to specified folder."),
                       i18n("Start downloading file to specified folder.")));
  if (!dlg.exec())
  {
     session->cancelZModem();
  }
  else
  {
     const KURL &url = dlg.selectedURL();
     session->startZModem(zmodem, url.path(), TQStringList());
  }
}

void KonsoleApp::slotPrint()
{
  KPrinter printer;
  printer.addDialogPage(new PrintSettings());
  if (printer.setup(this, i18n("Print %1").arg(se->Title())))
  {
    printer.setFullPage(false);
    printer.setCreator("Konsole");
    TQPainter paint;
    paint.begin(&printer);
    se->print(paint, printer.option("app-konsole-printfriendly") == "true",
                     printer.option("app-konsole-printexact") == "true");
    paint.end();
  }
}

void KonsoleApp::toggleBidi()
{
  b_bidiEnabled=!b_bidiEnabled;
  TQPtrList<TEWidget> tes = activeTEs();
  for (TEWidget *_te = tes.first(); _te; _te = tes.next()) {
    _te->setBidiEnabled(b_bidiEnabled);
    _te->repaint();
  }
}

//////////////////////////////////////////////////////////////////////

SizeDialog::SizeDialog(const unsigned int columns,
                       const unsigned int lines,
                       TQWidget *parent)
  : KDialogBase(Plain, i18n("Size Configuration"),
                Help | Default | Ok | Cancel, Ok,
                parent)
{
  TQFrame *mainFrame = plainPage();

  TQHBoxLayout *hb = new TQHBoxLayout(mainFrame);

  m_columns = new TQSpinBox(20,1000,1,mainFrame);
  m_columns->setValue(columns);

  m_lines = new TQSpinBox(4,1000,1,mainFrame);
  m_lines->setValue(lines);

  hb->addWidget(new TQLabel(i18n("Number of columns:"), mainFrame));
  hb->addWidget(m_columns);
  hb->addSpacing(10);
  hb->addWidget(new TQLabel(i18n("Number of lines:"), mainFrame));
  hb->addWidget(m_lines);

  setHelp("configure-size");
}

void SizeDialog::slotDefault()
{
  m_columns->setValue(80);
  m_lines->setValue(24);
}

unsigned int SizeDialog::columns() const
{
  return m_columns->value();
}

unsigned int SizeDialog::lines() const
{
  return m_lines->value();
}

//////////////////////////////////////////////////////////////////////

KonsoleFind::KonsoleFind( TQWidget *parent, const char *name, bool /*modal*/ )
  : KEdFind( parent, name, false ), m_editorDialog(0), m_editRegExp(0)
{
  TQHBox* row = new TQHBox( (TQWidget*)group );
  m_asRegExp = new TQCheckBox( i18n("As &regular expression"), row, "asRegexp" );

  if (!TDETrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty()) {
    m_editRegExp = new TQPushButton( i18n("&Edit..."), row, "editRegExp" );
    connect( m_asRegExp, TQ_SIGNAL( toggled(bool) ), m_editRegExp, TQ_SLOT( setEnabled(bool) ) );
    connect( m_editRegExp, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotEditRegExp() ) );
    m_editRegExp->setEnabled( false );
  }
}

void KonsoleFind::slotEditRegExp()
{
  if ( m_editorDialog == 0 )
    m_editorDialog = KParts::ComponentFactory::createInstanceFromQuery<TQDialog>( "KRegExpEditor/KRegExpEditor", TQString(), this );

  assert( m_editorDialog );

  KRegExpEditorInterface *iface = dynamic_cast<KRegExpEditorInterface *>( m_editorDialog );
  assert( iface );

  iface->setRegExp( getText() );
  bool ret = m_editorDialog->exec();
  if ( ret == TQDialog::Accepted)
    setText( iface->regExp() );
}

bool KonsoleFind::reg_exp() const
{
  return m_asRegExp->isChecked();
}

///////////////////////////////////////////////////////////
// This was to apply changes made to KControl fixed font to all TEs...
//  kvh - 03/10/2005 - We don't do this anymore...
void KonsoleApp::slotFontChanged()
{
  TEWidget *oldTe = te;
  TQPtrList<TEWidget> tes = activeTEs();
  for (TEWidget *_te = tes.first(); _te; _te = tes.next()) {
    te = _te;
//    setFont(n_font);
  }
  te = oldTe;
}

void KonsoleApp::biggerFont(void) {
    if ( !se ) return;
    te->biggerFont();
    activateSession();
}

void KonsoleApp::smallerFont(void) {
    if ( !se ) return;
    te->smallerFont();
    activateSession();
}

bool KonsoleApp::processDynamic(const TQCString &fun, const TQByteArray &data, TQCString& replyType, TQByteArray &replyData)
{
    if (b_fullScripting)
    {
      if (fun == "feedAllSessions(TQString)")
      {
        TQString arg0;
        TQDataStream arg( data, IO_ReadOnly );
        arg >> arg0;
        feedAllSessions(arg0);
        replyType = "void";
        return true;
      }
      else if (fun == "sendAllSessions(TQString)")
      {
        TQString arg0;
        TQDataStream arg( data, IO_ReadOnly );
        arg >> arg0;
        sendAllSessions(arg0);
        replyType = "void";
        return true;
      }
    }
    return KonsoleIface::processDynamic(fun, data, replyType, replyData);
}

QCStringList KonsoleApp::functionsDynamic()
{
    QCStringList funcs = KonsoleIface::functionsDynamic();
    if (b_fullScripting)
    {
      funcs << "void feedAllSessions(TQString text)";
      funcs << "void sendAllSessions(TQString text)";
    }
    return funcs;
}

void KonsoleApp::enableFullScripting(bool b)
{
    b_fullScripting = b;
    for (TESession *_se = sessions.first(); _se; _se = sessions.next())
       _se->enableFullScripting(b);
}

void KonsoleApp::enableFixedSize(bool b)
{
    b_fixedSize = b;
    if (b_fixedSize)
    {
      delete m_fullscreen;
      m_fullscreen = 0;
    }
}

TQPtrList<TEWidget> KonsoleApp::activeTEs()
{
   TQPtrList<TEWidget> ret;
   if (sessions.count()>0)
     for (TESession *_se = sessions.first(); _se; _se = sessions.next())
        ret.append(_se->widget());
   else if (te)  // check for startup initalization case in newSession()
     ret.append(te);
   return ret;
}

void KonsoleApp::setMenuAcceleratos()
{
  if (b_menuAccelerators)
  {
    menubar->changeItem(m_session_id, m_session_string);
    menubar->changeItem(m_edit_id, m_edit_string);
    menubar->changeItem(m_view_id, m_view_string);
    if (m_bookmarks)
      menubar->changeItem(m_bookmarks_id, m_bookmarks_string);
    if (m_options)
      menubar->changeItem(m_options_id, m_options_string);
    if (m_help)
      menubar->changeItem(m_help_id, m_help_string);
  }
  else
  {
    menubar->changeItem(m_session_id, TQString(m_session_string).replace(TQRegExp("&([^&])"), "\\1"));
    menubar->changeItem(m_edit_id, TQString(m_edit_string).replace(TQRegExp("&([^&])"), "\\1"));
    menubar->changeItem(m_view_id, TQString(m_view_string).replace(TQRegExp("&([^&])"), "\\1"));
    if (m_bookmarks)
      menubar->changeItem(m_bookmarks_id, TQString(m_bookmarks_string).replace(TQRegExp("&([^&])"), "\\1"));
    if (m_options)
      menubar->changeItem(m_options_id, TQString(m_options_string).replace(TQRegExp("&([^&])"), "\\1"));
    if (m_help)
      menubar->changeItem(m_help_id, TQString(m_help_string).replace(TQRegExp("&([^&])"), "\\1"));
  }
}

}

#include "KonsoleApp.moc"
