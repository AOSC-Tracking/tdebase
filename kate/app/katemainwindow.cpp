/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katemainwindow.h"
#include "katemainwindow.moc"

#include "kateconfigdialog.h"
#include "kateconsole.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katefileselector.h"
#include "katefilelist.h"
#include "katesessionpanel.h"
#include "kategrepdialog.h"
#include "katemailfilesdialog.h"
#include "katemainwindowiface.h"
#include "kateexternaltools.h"
#include "katesavemodifieddialog.h"
#include "katemwmodonhddialog.h"
#include "katesession.h"
#include "katetabwidget.h"

#include "../interfaces/mainwindow.h"
#include "../interfaces/toolviewmanager.h"

#include <dcopclient.h>
#include <tdeinstance.h>
#include <tdeaboutdata.h>
#include <tdeaction.h>
#include <tdecmdlineargs.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <tdediroperator.h>
#include <kdockwidget.h>
#include <kedittoolbar.h>
#include <tdefiledialog.h>
#include <tdeglobalaccel.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kmimetype.h>
#include <kopenwith.h>
#include <tdepopupmenu.h>
#include <tdesimpleconfig.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <tdestandarddirs.h>
#include <ktrader.h>
#include <kurldrag.h>
#include <tdedesktopfile.h>
#include <khelpmenu.h>
#include <tdemultitabbar.h>
#include <ktip.h>
#include <tdemenubar.h>
#include <kstringhandler.h>
#include <tqlayout.h>
#include <tqptrvector.h>

#include <assert.h>
#include <unistd.h>
//END

uint KateMainWindow::uniqueID = 1;

KateMainWindow::KateMainWindow (TDEConfig *sconfig, const TQString &sgroup)
  : KateMDI::MainWindow (0,(TQString(TQString("__KateMainWindow#%1").arg(uniqueID))).latin1())
{
  // first the very important id
  myID = uniqueID;
  uniqueID++;

  m_modignore = false;

  console = 0;
  greptool = 0;

  // here we go, set some usable default sizes
  if (!initialGeometrySet())
  {
    int scnum = TQApplication::desktop()->screenNumber(parentWidget());
    TQRect desk = TQApplication::desktop()->screenGeometry(scnum);

    TQSize size;

    // try to load size
    if (sconfig)
    {
      sconfig->setGroup (sgroup);
      size.setWidth (sconfig->readNumEntry( TQString::fromLatin1("Width %1").arg(desk.width()), 0 ));
      size.setHeight (sconfig->readNumEntry( TQString::fromLatin1("Height %1").arg(desk.height()), 0 ));
    }

    // if thats fails, try to reuse size
    if (size.isEmpty())
    {
      // first try to reuse size known from current or last created main window ;=)
      if (KateApp::self()->mainWindows () > 0)
      {
        KateMainWindow *win = KateApp::self()->activeMainWindow ();

        if (!win)
          win = KateApp::self()->mainWindow (KateApp::self()->mainWindows ()-1);

        size = win->size();
      }
      else // now fallback to hard defaults ;)
      {
        // first try global app config
        KateApp::self()->config()->setGroup ("MainWindow");
        size.setWidth (KateApp::self()->config()->readNumEntry( TQString::fromLatin1("Width %1").arg(desk.width()), 0 ));
        size.setHeight (KateApp::self()->config()->readNumEntry( TQString::fromLatin1("Height %1").arg(desk.height()), 0 ));

        if (size.isEmpty())
          size = TQSize (kMin (700, desk.width()), kMin(480, desk.height()));
      }

      resize (size);
    }
  }

  // start session restore if needed
  startRestore (sconfig, sgroup);

  m_mainWindow = new Kate::MainWindow (this);
  m_toolViewManager = new Kate::ToolViewManager (this);

  m_dcop = new KateMainWindowDCOPIface (this);

  m_mainWindow->installEventFilter(this);

  // setup the most important widgets
  setupMainWindow();

  // setup the actions
  setupActions();

  setStandardToolBarMenuEnabled( true );
  setXMLFile( "kateui.rc" );
  createShellGUI ( true );

  KatePluginManager::self()->enableAllPluginsGUI (this);

  if ( KateApp::self()->authorize("shell_access") )
    Kate::Document::registerCommand(KateExternalToolsCommand::self());

  // connect documents menu aboutToshow
  documentMenu = (TQPopupMenu*)factory()->container("documents", this);
  connect(documentMenu, TQ_SIGNAL(aboutToShow()), this, TQ_SLOT(documentMenuAboutToShow()));

  // caption update
  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
    slotDocumentCreated (KateDocManager::self()->document(i));

  connect(KateDocManager::self(),TQ_SIGNAL(documentCreated(Kate::Document *)),this,TQ_SLOT(slotDocumentCreated(Kate::Document *)));

  readOptions();

  if (sconfig)
    m_viewManager->restoreViewConfiguration (sconfig, sgroup);

  finishRestore ();

  setAcceptDrops(true);
}

KateMainWindow::~KateMainWindow()
{
  // first, save our fallback window size ;)
  KateApp::self()->config()->setGroup ("MainWindow");
  saveWindowSize (KateApp::self()->config());

  // save other options ;=)
  saveOptions();

  KateApp::self()->removeMainWindow (this);

  KatePluginManager::self()->disableAllPluginsGUI (this);

  delete m_dcop;
}

void KateMainWindow::setupMainWindow ()
{
  setToolViewStyle( KMultiTabBar::KDEV3ICON );

  m_tabWidget = new KateTabWidget (centralWidget());

  m_viewManager = new KateViewManager (this);

  KateMDI::ToolView *ft = createToolView("kate_filelist", KMultiTabBar::Left, SmallIcon("application-vnd.tde.tdemultiple"), i18n("Documents"));
  filelist = new KateFileList (this, m_viewManager, ft, "filelist");
  filelist->readConfig(KateApp::self()->config(), "Filelist");
  filelist->viewport()->installEventFilter(this);

  KateMDI::ToolView *t = createToolView("kate_fileselector", KMultiTabBar::Left, SmallIcon("document-open"), i18n("Filesystem Browser"));
  fileselector = new KateFileSelector( this, m_viewManager, t, "operator");
  connect(fileselector->dirOperator(),TQ_SIGNAL(fileSelected(const KFileItem*)),this,TQ_SLOT(fileSelected(const KFileItem*)));

  KateMDI::ToolView *st = createToolView("kate_sessionpanel", KMultiTabBar::Left, SmallIcon("view_choose"), i18n("Sessions"));
  m_sessionpanel = new KateSessionPanel( this, m_viewManager, st, "sessionpanel");

  // ONLY ALLOW SHELL ACCESS IF ALLOWED ;)
  if (KateApp::self()->authorize("shell_access"))
  {
    t = createToolView("kate_greptool", KMultiTabBar::Bottom, SmallIcon("filefind"), i18n("Find in Files") );
    greptool = new GrepTool( t, "greptool" );
    connect(greptool, TQ_SIGNAL(itemSelected(const TQString &,int)), this, TQ_SLOT(slotGrepToolItemSelected(const TQString &,int)));
    connect(t,TQ_SIGNAL(visibleChanged(bool)),this, TQ_SLOT(updateGrepDir (bool)));
    // WARNING HACK - anders: showing the greptool seems to make the menu accels work
    greptool->show();

    t = createToolView("kate_console", KMultiTabBar::Bottom, SmallIcon("konsole"), i18n("Terminal"));
    console = new KateConsole (this, t);
  }

  // make per default the filelist visible, if we are in session restore, katemdi will skip this ;)
  showToolView (ft);
}

void KateMainWindow::setupActions()
{
  TDEAction *a;

  KStdAction::openNew( m_viewManager, TQ_SLOT( slotDocumentNew() ), actionCollection(), "file_new" )->setWhatsThis(i18n("Create a new document"));
  KStdAction::open( m_viewManager, TQ_SLOT( slotDocumentOpen() ), actionCollection(), "file_open" )->setWhatsThis(i18n("Open an existing document for editing"));

  fileOpenRecent = KStdAction::openRecent (m_viewManager, TQ_SLOT(openURL (const KURL&)), actionCollection());
  fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a=new TDEAction( i18n("Save A&ll"),"save_all", CTRL+Key_L, KateDocManager::self(), TQ_SLOT( saveAll() ), actionCollection(), "file_save_all" );
  a->setWhatsThis(i18n("Save all open, modified documents to disk."));

  KStdAction::close( m_viewManager, TQ_SLOT( slotDocumentClose() ), actionCollection(), "file_close" )->setWhatsThis(i18n("Close the current document."));

  a=new TDEAction( i18n( "Clos&e All" ), 0, this, TQ_SLOT( slotDocumentCloseAll() ), actionCollection(), "file_close_all" );
  a->setWhatsThis(i18n("Close all open documents."));

  KStdAction::mail( this, TQ_SLOT(slotMail()), actionCollection() )->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));

  KStdAction::quit( this, TQ_SLOT( slotFileQuit() ), actionCollection(), "file_quit" )->setWhatsThis(i18n("Close this window"));

  a=new TDEAction(i18n("&New Window"), "window-new", 0, this, TQ_SLOT(newWindow()), actionCollection(), "view_new_view");
  a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

  if ( KateApp::self()->authorize("shell_access") )
  {
    externalTools = new KateExternalToolsMenuAction( i18n("External Tools"), actionCollection(), "tools_external", this );
    externalTools->setWhatsThis( i18n("Launch external helper applications") );
  }

  TDEToggleAction* showFullScreenAction = KStdAction::fullScreen( 0, 0, actionCollection(),this);
  connect( showFullScreenAction,TQ_SIGNAL(toggled(bool)), this,TQ_SLOT(slotFullScreen(bool)));

  documentOpenWith = new TDEActionMenu(i18n("Open W&ith"), actionCollection(), "file_open_with");
  documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
  connect(documentOpenWith->popupMenu(), TQ_SIGNAL(aboutToShow()), this, TQ_SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->popupMenu(), TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotOpenWithMenuAction(int)));

  a=KStdAction::keyBindings(this, TQ_SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=KStdAction::configureToolbars(this, TQ_SLOT(slotEditToolbars()), actionCollection());
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  TDEAction* settingsConfigure = KStdAction::preferences(this, TQ_SLOT(slotConfigure()), actionCollection(), "settings_configure");
  settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

  // pipe to terminal action
  if (KateApp::self()->authorize("shell_access"))
    new TDEAction(i18n("&Pipe to Console"), "pipe", 0, console, TQ_SLOT(slotPipeToConsole()), actionCollection(), "tools_pipe_to_terminal");

  // tip of the day :-)
  KStdAction::tipOfDay( this, TQ_SLOT( tipOfTheDay() ), actionCollection() )->setWhatsThis(i18n("This shows useful tips on the use of this application."));

  if (KatePluginManager::self()->pluginList().count() > 0)
  {
    a=new TDEAction(i18n("&Plugins Handbook"), 0, this, TQ_SLOT(pluginHelp()), actionCollection(), "help_plugins_contents");
    a->setWhatsThis(i18n("This shows help files for various available plugins."));
  }

  connect(m_viewManager,TQ_SIGNAL(viewChanged()),this,TQ_SLOT(slotWindowActivated()));
  connect(m_viewManager,TQ_SIGNAL(viewChanged()),this,TQ_SLOT(slotUpdateOpenWith()));

  slotWindowActivated ();

  // session actions
  new TDEAction(i18n("&New"), "list-add", 0,
      m_sessionpanel, TQ_SLOT(slotNewSession()), actionCollection(), "session_new");
  new TDEAction(i18n("&Save"), "document-save", 0,
      m_sessionpanel, TQ_SLOT(slotSaveSession()), actionCollection(), "session_save");
  new TDEAction(i18n("Save &As..."), "document-save-as", 0,
      m_sessionpanel, TQ_SLOT(slotSaveSessionAs()), actionCollection(), "session_save_as");
  new TDEAction(i18n("&Rename"), "edit_user", 0,
      m_sessionpanel, TQ_SLOT(slotRenameSession()), actionCollection(), "session_rename");
  new TDEAction(i18n("&Delete"), "edit-delete", 0,
      m_sessionpanel, TQ_SLOT(slotDeleteSession()), actionCollection(), "session_delete");
  new TDEAction(i18n("Re&load"), "reload", 0,
      m_sessionpanel, TQ_SLOT(slotReloadSession()), actionCollection(), "session_reload");
  new TDEAction(i18n("Acti&vate"), "forward", 0,
      m_sessionpanel, TQ_SLOT(slotActivateSession()), actionCollection(), "session_activate");
  new TDEToggleAction(i18n("Toggle read &only"), "encrypted", 0,
      m_sessionpanel, TQ_SLOT(slotSessionToggleReadOnly()), actionCollection(), "session_toggle_read_only");
  new TDEAction(i18n("Move &Up"), "go-up", 0,
      m_sessionpanel, TQ_SLOT(slotSessionMoveUp()), actionCollection(), "session_move_up");
  new TDEAction(i18n("Move Do&wn"), "go-down", 0,
      m_sessionpanel, TQ_SLOT(slotSessionMoveDown()), actionCollection(), "session_move_down");
  new KateSessionListActionMenu(this, i18n("Sele&ct session"), actionCollection(), "session_list");

  connect(m_sessionpanel, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(slotSelectionChanged()));
}

KateTabWidget *KateMainWindow::tabWidget ()
{
  return m_tabWidget;
}

void KateMainWindow::slotDocumentCloseAll() {
  if (queryCloseAllDocuments())
    KateDocManager::self()->closeAllDocuments(false);
}

// Check if all documents can be closed safely
bool KateMainWindow::queryCloseAllDocuments() {
  uint documentCount=KateDocManager::self()->documents();

  if ( !showModOnDiskPrompt() )
  {
    return false;
  }

  TQPtrList<Kate::Document> modifiedDocuments=KateDocManager::self()->modifiedDocumentList();
  bool closeOK = (modifiedDocuments.count() == 0);

  if (!closeOK)
  {
    closeOK = KateSaveModifiedDialog::queryClose(this,modifiedDocuments);
  }

  if ( KateDocManager::self()->documents() > documentCount ) {
    KMessageBox::information (this,
                              i18n ("New file opened while trying to close all other documents, closing aborted."),
                              i18n ("Closing Aborted"));
    return false;
  }

  return closeOK;
}

bool KateMainWindow::queryClose_internal() {
  if (!queryCloseAllDocuments())
    return false;

  return KateApp::self()->query_session_close();
}

/**
 * queryClose(), take care that after the last mainwindow the stuff is closed
 */
bool KateMainWindow::queryClose()
{
  // session saving, can we close all views ?
  // just test, not close them actually
  if (KateApp::self()->sessionSaving())
  {
    return queryClose_internal();
  }

  // normal closing of window
  // allow to close all windows until the last without restrictions
  if (KateApp::self()->mainWindows() > 1)
  {
    return true;
  }

  // last one: check if we can close all documents and sessions, try run
  // and save docs if we really close down !
  if (queryClose_internal())
  {
    // detach the dcopClient
    KateApp::self()->dcopClient()->detach();
    return true;
  }

  return false;
}

void KateMainWindow::newWindow ()
{
  KateApp::self()->newMainWindow ();
}

void KateMainWindow::slotEditToolbars()
{
  saveMainWindowSettings( KateApp::self()->config(), "MainWindow" );
  KEditToolbar dlg( factory() );
  connect( &dlg, TQ_SIGNAL(newToolbarConfig()), this, TQ_SLOT(slotNewToolbarConfig()) );
  dlg.exec();
}

void KateMainWindow::slotNewToolbarConfig()
{
  applyMainWindowSettings( KateApp::self()->config(), "MainWindow" );
}

void KateMainWindow::slotFileQuit()
{
  KateApp::self()->shutdownKate(this);
}

void KateMainWindow::readOptions ()
{
  TDEConfig *config = KateApp::self()->config ();

  config->setGroup("General");
  showSessionName = config->readBoolEntry("Show session name", false);
  syncKonsole = config->readBoolEntry("Sync Konsole", true);
  useInstance = config->readBoolEntry("UseInstance", false);
  modNotification = config->readBoolEntry("Modified Notification", false);
  KateDocManager::self()->setSaveMetaInfos(config->readBoolEntry("Save Meta Infos", true));
  KateDocManager::self()->setDaysMetaInfos(config->readNumEntry("Days Meta Infos", 30));

  m_viewManager->setShowFullPath(config->readBoolEntry("Show Full Path in Title", false));

  fileOpenRecent->setMaxItems( config->readNumEntry("Number of recent files", fileOpenRecent->maxItems() ) );
  fileOpenRecent->loadEntries(config, "Recent Files");

  fileselector->readConfig(config, "fileselector");
}

void KateMainWindow::saveOptions ()
{
  TDEConfig *config = KateApp::self()->config ();
  config->setGroup("General");

  if (console)
    config->writeEntry("Show Console", console->isVisible());
  else
    config->writeEntry("Show Console", false);

  config->writeEntry("Show session name", showSessionName);
  config->writeEntry("Save Meta Infos", KateDocManager::self()->getSaveMetaInfos());
  config->writeEntry("Days Meta Infos", KateDocManager::self()->getDaysMetaInfos());
  config->writeEntry("Show Full Path in Title", m_viewManager->getShowFullPath());
  config->writeEntry("Sync Konsole", syncKonsole);
  config->writeEntry("UseInstance", useInstance);

  fileOpenRecent->saveEntries(config, "Recent Files");
  fileselector->writeConfig(config, "fileselector");
  filelist->writeConfig(config, "Filelist");

  config->sync();
}

void KateMainWindow::slotWindowActivated ()
{
  if (m_viewManager->activeView())
  {
    if (console && syncKonsole)
    {
      static TQString path;
      TQString newPath = m_viewManager->activeView()->getDoc()->url().directory();

      if ( newPath != path )
      {
        path = newPath;
        console->cd (KURL( path ));
      }
    }

    updateCaption (m_viewManager->activeView()->getDoc());
  }

  // update proxy
  centralWidget()->setFocusProxy (m_viewManager->activeView());
}

void KateMainWindow::slotUpdateOpenWith()
{
  if (m_viewManager->activeView())
    documentOpenWith->setEnabled(!m_viewManager->activeView()->document()->url().isEmpty());
  else
    documentOpenWith->setEnabled(false);
}

void KateMainWindow::documentMenuAboutToShow()
{
  // remove documents
  while (documentMenu->count() > 3)
    documentMenu->removeItemAt (3);

  TQListViewItem * item = filelist->firstChild();
  while( item ) {
    // would it be saner to use the screen width as a limit that some random number??
    TQString name = KStringHandler::rsqueeze( ((KateFileListItem *)item)->document()->docName(), 150 );
    Kate::Document* doc = ((KateFileListItem *)item)->document();
    documentMenu->insertItem (
          doc->isModified() ? i18n("'document name [*]', [*] means modified", "%1 [*]").arg(name) : name,
          m_viewManager, TQ_SLOT (activateView (int)), 0,
          ((KateFileListItem *)item)->documentNumber () );

    item = item->nextSibling();
  }
  if (m_viewManager->activeView())
    documentMenu->setItemChecked ( m_viewManager->activeView()->getDoc()->documentNumber(), true);
}

void KateMainWindow::slotGrepToolItemSelected(const TQString &filename,int linenumber)
{
  KURL fileURL;
  fileURL.setPath( filename );
  m_viewManager->openURL( fileURL );
  if ( m_viewManager->activeView() == 0 ) return;
  m_viewManager->activeView()->gotoLineNumber( linenumber );
  raise();
  setActiveWindow();
}

void KateMainWindow::dragEnterEvent( TQDragEnterEvent *event )
{
  event->accept(KURLDrag::canDecode(event));
}

void KateMainWindow::dropEvent( TQDropEvent *event )
{
  slotDropEvent(event);
}

void KateMainWindow::slotDropEvent( TQDropEvent * event )
{
  KURL::List textlist;
  if (!KURLDrag::decode(event, textlist)) return;

  for (KURL::List::Iterator i=textlist.begin(); i != textlist.end(); ++i)
  {
    m_viewManager->openURL (*i);
  }
}

void KateMainWindow::editKeys()
{
  KKeyDialog dlg ( false, this );

  TQPtrList<KXMLGUIClient> clients = guiFactory()->clients();

  for( TQPtrListIterator<KXMLGUIClient> it( clients ); it.current(); ++it )
    dlg.insert ( (*it)->actionCollection(), (*it)->instance()->aboutData()->programName() );

  dlg.insert( externalTools->actionCollection(), i18n("External Tools") );

  dlg.configure();

  TQPtrList<Kate::Document>  l=KateDocManager::self()->documentList();
  for (uint i=0;i<l.count();i++) {
//     kdDebug(13001)<<"reloading Keysettings for document "<<i<<endl;
    l.at(i)->reloadXML();
    TQPtrList<class KTextEditor::View> l1=l.at(i)->views ();//KTextEditor::Document
    for (uint i1=0;i1<l1.count();i1++) {
      l1.at(i1)->reloadXML();
//       kdDebug(13001)<<"reloading Keysettings for view "<<i<<"/"<<i1<<endl;
    }
  }

  externalTools->actionCollection()->writeShortcutSettings( "Shortcuts", new TDEConfig("externaltools", false, false, "appdata") );
}

void KateMainWindow::openURL (const TQString &name)
{
  m_viewManager->openURL (KURL(name));
}

void KateMainWindow::slotConfigure()
{
  if (!m_viewManager->activeView())
    return;

  KateConfigDialog* dlg = new KateConfigDialog (this, m_viewManager->activeView());
  dlg->exec();

  delete dlg;

  // Inform Kate that options may have been changed
  KateApp::self()->reparse_config();
}

KURL KateMainWindow::activeDocumentUrl()
{
  // anders: i make this one safe, as it may be called during
  // startup (by the file selector)
  Kate::View *v = m_viewManager->activeView();
  if ( v )
    return v->getDoc()->url();
  return KURL();
}

void KateMainWindow::fileSelected(const KFileItem * /*file*/)
{
  const KFileItemList *list=fileselector->dirOperator()->selectedItems();
  KFileItem *tmp;
  for (KFileItemListIterator it(*list); (tmp = it.current()); ++it)
  {
    m_viewManager->openURL(tmp->url());
    fileselector->dirOperator()->view()->setSelected(tmp,false);
  }
}

// TODO make this work
void KateMainWindow::mSlotFixOpenWithMenu()
{
  //kdDebug(13001)<<"13000"<<"fixing open with menu"<<endl;
  documentOpenWith->popupMenu()->clear();
  // get a list of appropriate services.
  KMimeType::Ptr mime = KMimeType::findByURL( m_viewManager->activeView()->getDoc()->url() );
  //kdDebug(13001)<<"13000"<<"url: "<<m_viewManager->activeView()->getDoc()->url().prettyURL()<<"mime type: "<<mime->name()<<endl;
  // some checking goes here...
  TDETrader::OfferList offers = TDETrader::self()->query(mime->name(), "Type == 'Application'");
  // for each one, insert a menu item...
  for(TDETrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it) {
    if ((*it)->name() == "Kate") continue;
    documentOpenWith->popupMenu()->insertItem( SmallIcon( (*it)->icon() ), (*it)->name() );
  }
  // append "Other..." to call the TDE "open with" dialog.
  documentOpenWith->popupMenu()->insertItem(i18n("&Other..."));
}

void KateMainWindow::slotOpenWithMenuAction(int idx)
{
  KURL::List list;
  list.append( m_viewManager->activeView()->getDoc()->url() );
  TQString appname = documentOpenWith->popupMenu()->text(idx);

  appname = appname.remove('&'); //Remove a possible accelerator ... otherwise the application might not get found.
  if ( appname.compare(i18n("Other...")) == 0 ) {
    // display "open with" dialog
    KOpenWithDlg dlg(list);
    if (dlg.exec())
      KRun::run(*dlg.service(), list);
    return;
  }

  TQString qry = TQString("((Type == 'Application') and (Name == '%1'))").arg( appname.latin1() );
  KMimeType::Ptr mime = KMimeType::findByURL( m_viewManager->activeView()->getDoc()->url() );
  TDETrader::OfferList offers = TDETrader::self()->query(mime->name(), qry);

  if (!offers.isEmpty()) {
    KService::Ptr app = offers.first();
    KRun::run(*app, list);
  }
  else
    KMessageBox::error(this, i18n("Application '%1' not found!").arg(appname.latin1()), i18n("Application Not Found!"));
}

void KateMainWindow::pluginHelp()
{
  KateApp::self()->invokeHelp (TQString::null, "kate-plugins");
}

void KateMainWindow::slotMail()
{
  KateMailDialog *d = new KateMailDialog(this, this);
  if ( ! d->exec() )
  {
    delete d;
    return;
  }
  TQPtrList<Kate::Document> attDocs = d->selectedDocs();
  delete d;
  // Check that all selected files are saved (or shouldn't be)
  TQStringList urls; // to atthatch
  Kate::Document *doc;
  TQPtrListIterator<Kate::Document> it(attDocs);
  for ( ; it.current(); ++it ) {
    doc = it.current();
    if (!doc) continue;
    if ( doc->url().isEmpty() ) {
      // unsaved document. back out unless it gets saved
      int r = KMessageBox::questionYesNo( this,
              i18n("<p>The current document has not been saved, and "
              "cannot be attached to an email message."
              "<p>Do you want to save it and proceed?"),
              i18n("Cannot Send Unsaved File"),KStdGuiItem::saveAs(),KStdGuiItem::cancel() );
      if ( r == KMessageBox::Yes ) {
        Kate::View *v = (Kate::View*)doc->views().first();
        int sr = v->saveAs();
        if ( sr == Kate::View::SAVE_OK ) { ;
        }
        else {
          if ( sr != Kate::View::SAVE_CANCEL ) // ERROR or RETRY(?)
            KMessageBox::sorry( this, i18n("The file could not be saved. Please check "
                                        "if you have write permission.") );
          continue;
        }
      }
      else
        continue;
    }
    if ( doc->isModified() ) {
      // warn that document is modified and offer to save it before proceeding.
      int r = KMessageBox::warningYesNoCancel( this,
                i18n("<p>The current file:<br><strong>%1</strong><br>has been "
                "modified. Modifications will not be available in the attachment."
                "<p>Do you want to save it before sending it?").arg(doc->url().prettyURL()),
                i18n("Save Before Sending?"), KStdGuiItem::save(), i18n("Do Not Save") );
      switch ( r ) {
        case KMessageBox::Cancel:
          continue;
        case KMessageBox::Yes:
          doc->save();
          if ( doc->isModified() ) { // read-only docs ends here, if modified. Hmm.
            KMessageBox::sorry( this, i18n("The file could not be saved. Please check "
                                      "if you have write permission.") );
            continue;
          }
          break;
        default:
          break;
      }
    }
    // finally call the mailer
    urls << doc->url().url();
  } // check selected docs done
  if ( ! urls.count() )
    return;
  KateApp::self()->invokeMailer( TQString::null, // to
                      TQString::null, // cc
                      TQString::null, // bcc
                      TQString::null, // subject
                      TQString::null, // body
                      TQString::null, // msgfile
                      urls           // urls to atthatch
                      );
}
void KateMainWindow::tipOfTheDay()
{
  KTipDialog::showTip( /*0*/this, TQString::null, true );
}

void KateMainWindow::slotFullScreen(bool t)
{
  if (t)
    showFullScreen();
  else
    showNormal();
}

void KateMainWindow::updateGrepDir (bool visible)
{
  // grepdlg gets hidden
  if (!visible)
    return;

  if ( m_viewManager->activeView() )
  {
    if ( m_viewManager->activeView()->getDoc()->url().isLocalFile() )
    {
      greptool->updateDirName( m_viewManager->activeView()->getDoc()->url().directory() );
    }
  }
}

bool KateMainWindow::event( TQEvent *e )
{
  uint type = e->type();
  if ( type == TQEvent::WindowActivate && modNotification )
  {
    showModOnDiskPrompt();
  }
  return KateMDI::MainWindow::event( e );
}

bool KateMainWindow::showModOnDiskPrompt()
{
  Kate::Document *doc;

  DocVector list( KateDocManager::self()->documents() );
  uint cnt = 0;
  for( doc = KateDocManager::self()->firstDocument(); doc; doc = KateDocManager::self()->nextDocument() )
  {
    if ( KateDocManager::self()->documentInfo( doc )->modifiedOnDisc )
    {
      list.insert( cnt, doc );
      cnt++;
    }
  }

  if ( cnt && !m_modignore )
  {
    list.resize( cnt );
    KateMwModOnHdDialog mhdlg( list, this );
    m_modignore = true;
    bool res = mhdlg.exec();
    m_modignore = false;

    return res;
  }
  return true;
}

void KateMainWindow::slotDocumentCreated (Kate::Document *doc)
{
  connect(doc,TQ_SIGNAL(modStateChanged(Kate::Document *)),this,TQ_SLOT(updateCaption(Kate::Document *)));
  connect(doc,TQ_SIGNAL(nameChanged(Kate::Document *)),this,TQ_SLOT(slotNameChanged(Kate::Document *)));
  connect(doc,TQ_SIGNAL(nameChanged(Kate::Document *)),this,TQ_SLOT(slotUpdateOpenWith()));

  updateCaption (doc);
}

void KateMainWindow::slotNameChanged(Kate::Document *doc)
{
  updateCaption(doc);
  if (!doc->url().isEmpty())
    fileOpenRecent->addURL(doc->url());
}

void KateMainWindow::updateCaption(Kate::Document *doc)
{
  if (!m_viewManager->activeView())
  {
    setCaption ("", false);
    return;
  }

  if (!(m_viewManager->activeView()->getDoc() == doc))
    return;

  TQString c;
  if (m_viewManager->activeView()->getDoc()->url().isEmpty() || (!m_viewManager->getShowFullPath()))
  {
    c = m_viewManager->activeView()->getDoc()->docName();
  }
  else
  {
    c = m_viewManager->activeView()->getDoc()->url().pathOrURL();
  }

  if (showSessionName)
  {
    TQString sessName = KateApp::self()->sessionManager()->getActiveSessionName();
    if (!sessName.isEmpty())
    {
      sessName = TQString("%1: ").arg(sessName);
    }
    setCaption(KStringHandler::lsqueeze(sessName,32) + KStringHandler::lsqueeze(c,64),
        m_viewManager->activeView()->getDoc()->isModified());
  }
  else
  {
    setCaption(KStringHandler::lsqueeze(c,64), m_viewManager->activeView()->getDoc()->isModified());
  }
}

void KateMainWindow::saveProperties(TDEConfig *config)
{
  TQString grp=config->group();

  saveSession(config, grp);
  m_viewManager->saveViewConfiguration (config, grp);

  config->setGroup(grp);
}

void KateMainWindow::readProperties(TDEConfig *config)
{
  TQString grp=config->group();

  startRestore(config, grp);
  finishRestore ();
  m_viewManager->restoreViewConfiguration (config, grp);

  config->setGroup(grp);
}

bool KateMainWindow::eventFilter(TQObject *obj, TQEvent *ev)
{
    if (ev->type() == TQEvent::MouseButtonRelease)
    {
        TQMouseEvent *mouseEvent = static_cast<TQMouseEvent *>(ev);
        switch (mouseEvent->button())
        {
            case TQMouseEvent::HistoryBackButton:
                filelist->slotPrevDocument();
                return true;
            case TQMouseEvent::HistoryForwardButton:
                filelist->slotNextDocument();
                return true;
        }
    }
    return false;
}

//-------------------------------------------
void KateMainWindow::slotSelectionChanged()
{
  TDEActionCollection *mwac = actionCollection();  // Main Window Action Collection
  TDEActionPtrList actionList = m_sessionpanel->m_actionCollection->actions();
  TDEActionPtrList::ConstIterator spa_it;
  for (spa_it = actionList.begin(); spa_it != actionList.end(); ++spa_it)
  {
    TDEAction *a = mwac->action((*spa_it)->name());
    TDEToggleAction *ta = dynamic_cast<TDEToggleAction*>(a);
    if (ta)
    {
      ta->setChecked((dynamic_cast<TDEToggleAction*>(*spa_it))->isChecked());
    }
    if (a)
    {
      a->setEnabled((*spa_it)->isEnabled());
    }
  }
}

//-------------------------------------------
void KateMainWindow::activateSession(int sessionId)
{
  if (sessionId < 0 || sessionId == KateApp::self()->sessionManager()->getActiveSessionId())
  {
    return;
  }

  // Select the required session in the session panel's listview
  TQListViewItem *item = m_sessionpanel->m_listview->firstChild();
  int idx = 0;
  while (item && idx < sessionId)
  {
    item = item->nextSibling();
    ++idx;
  }
  if (idx == sessionId && item)
  {
    // Required session item found, switch session with consistent behavior
    m_sessionpanel->m_listview->setSelected(item, true);
    m_sessionpanel->slotActivateSession();
  }
}

//-------------------------------------------
KateSessionListActionMenu::KateSessionListActionMenu(KateMainWindow *mw, const TQString &text, TQObject *parent, const char *name)
  : TDEActionMenu(text, parent, name), m_mainWindow(mw)
{
  connect(popupMenu(), TQ_SIGNAL(aboutToShow()), this, TQ_SLOT(slotAboutToShow()));
}

//-------------------------------------------
void KateSessionListActionMenu::slotAboutToShow()
{
  popupMenu()->clear();

  TQPtrList<KateSession> &sessions = KateApp::self()->sessionManager()->getSessionsList();
  for (int idx = 0;  idx < (int)sessions.count();  ++idx)
  {
    popupMenu()->insertItem(sessions[idx]->getSessionName(), m_mainWindow, TQ_SLOT(activateSession(int)), 0, idx);
  }
}
