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

#include "kwritemain.h"
#include "kwritemain.moc"

#include <kate/document.h>
#include <kate/view.h>

#include <tdetexteditor/configinterface.h>
#include <tdetexteditor/sessionconfiginterface.h>
#include <tdetexteditor/viewcursorinterface.h>
#include <tdetexteditor/printinterface.h>
#include <tdetexteditor/encodinginterface.h>
#include <tdetexteditor/editorchooser.h>
#include <tdetexteditor/popupmenuinterface.h>

#include <tdeio/netaccess.h>

#include <dcopclient.h>
#include <kurldrag.h>
#include <kencodingfiledialog.h>
#include <tdediroperator.h>
#include <kiconloader.h>
#include <tdeaboutdata.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <tdeaction.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdeapplication.h>
#include <tdelocale.h>
#include <kurl.h>
#include <tdeconfig.h>
#include <tdecmdlineargs.h>
#include <tdemessagebox.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <tdeparts/event.h>
#include <tdemenubar.h>

#include <tqdropsite.h>
#include <tqdragobject.h>
#include <tqvbox.h>
#include <tqtextcodec.h>
#include <tqlayout.h>

// StatusBar field IDs
#define KWRITE_ID_GEN 1

TQPtrList<KTextEditor::Document> KWrite::docList;
TQPtrList<KWrite> KWrite::winList;

KWrite::KWrite (KTextEditor::Document *doc)
    : m_view(0),
      m_recentFiles(0),
      m_paShowPath(0),
      m_paShowStatusBar(0)
{
  if ( !doc )
  {
    if ( !(doc = KTextEditor::EditorChooser::createDocument(0,"KTextEditor::Document")) )
    {
      KMessageBox::error(this, i18n("A TDE text-editor component could not be found;\n"
                                    "please check your TDE installation."));
      tdeApp->exit(1);
    }

    docList.append(doc);
  }

  m_view = doc->createView (this, 0L);

  setCentralWidget(m_view);

  setupActions();
  setupStatusBar();

  setAcceptDrops(true);

  connect(m_view,TQ_SIGNAL(newStatus()),this,TQ_SLOT(newCaption()));
  connect(m_view,TQ_SIGNAL(viewStatusMsg(const TQString &)),this,TQ_SLOT(newStatus(const TQString &)));
  connect(m_view->document(),TQ_SIGNAL(fileNameChanged()),this,TQ_SLOT(newCaption()));
  connect(m_view->document(),TQ_SIGNAL(fileNameChanged()),this,TQ_SLOT(slotFileNameChanged()));
  connect(m_view,TQ_SIGNAL(dropEventPass(TQDropEvent *)),this,TQ_SLOT(slotDropEvent(TQDropEvent *)));

  setXMLFile( "kwriteui.rc" );
  createShellGUI( true );
  guiFactory()->addClient( m_view );

  // install a working kate part popup dialog thingy
  if (static_cast<Kate::View*>(m_view->tqt_cast("Kate::View")))
    static_cast<Kate::View*>(m_view->tqt_cast("Kate::View"))->installPopup ((TQPopupMenu*)(factory()->container("tdetexteditor_popup", this)) );

  // init with more usefull size, stolen from konq :)
  if (!initialGeometrySet())
    resize( TQSize(700, 480).expandedTo(minimumSizeHint()));

  // call it as last thing, must be sure everything is already set up ;)
  setAutoSaveSettings ();

  readConfig ();

  winList.append (this);

  show ();
}

KWrite::~KWrite()
{
  winList.remove (this);

  if (m_view->document()->views().count() == 1)
  {
    docList.remove(m_view->document());
    delete m_view->document();
  }

  tdeApp->config()->sync ();
}

void KWrite::setupActions()
{
  KStdAction::close( this, TQ_SLOT(slotFlush()), actionCollection(), "file_close" )->setWhatsThis(i18n("Use this to close the current document"));

  // setup File menu
  KStdAction::print(this, TQ_SLOT(printDlg()), actionCollection())->setWhatsThis(i18n("Use this command to print the current document"));
  KStdAction::openNew( this, TQ_SLOT(slotNew()), actionCollection(), "file_new" )->setWhatsThis(i18n("Use this command to create a new document"));
  KStdAction::open( this, TQ_SLOT( slotOpen() ), actionCollection(), "file_open" )->setWhatsThis(i18n("Use this command to open an existing document for editing"));

  m_recentFiles = KStdAction::openRecent(this, TQ_SLOT(slotOpen(const KURL&)),
                                         actionCollection());
  m_recentFiles->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  TDEAction *a=new TDEAction(i18n("&New Window"), "window-new", 0, this, TQ_SLOT(newView()),
              actionCollection(), "view_new_view");
  a->setWhatsThis(i18n("Create another view containing the current document"));

  a=new TDEAction(i18n("Choose Editor Component..."),0,this,TQ_SLOT(changeEditor()),
		actionCollection(),"settings_choose_editor");
  a->setWhatsThis(i18n("Override the system wide setting for the default editing component"));

  KStdAction::quit(this, TQ_SLOT(close()), actionCollection())->setWhatsThis(i18n("Close the current document view"));

  // setup Settings menu
  setStandardToolBarMenuEnabled(true);

  m_paShowStatusBar = KStdAction::showStatusbar(this, TQ_SLOT(toggleStatusBar()), actionCollection(), "settings_show_statusbar");
  m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));

  m_paShowPath = new TDEToggleAction(i18n("Sho&w Path"), 0, this, TQ_SLOT(newCaption()),
                    actionCollection(), "set_showPath");
  m_paShowPath->setCheckedState(i18n("Hide Path"));
  m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));
  a=KStdAction::keyBindings(this, TQ_SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=KStdAction::configureToolbars(this, TQ_SLOT(editToolbars()), actionCollection());
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));
}

void KWrite::setupStatusBar()
{
  statusBar()->insertItem("", KWRITE_ID_GEN);
}

// load on url
void KWrite::loadURL(const KURL &url)
{
  m_view->document()->openURL(url);
}

// is closing the window wanted by user ?
bool KWrite::queryClose()
{
  if (m_view->document()->views().count() > 1)
    return true;

  if (m_view->document()->queryClose())
  {
    writeConfig();

    return true;
  }

  return false;
}

void KWrite::changeEditor()
{
  KWriteEditorChooser choose(this);
  choose.exec();
}

void KWrite::slotFlush ()
{
   m_view->document()->closeURL();
}

void KWrite::slotNew()
{
  new KWrite();
}

void KWrite::slotOpen()
{
  if (KTextEditor::encodingInterface(m_view->document()))
  {
	KEncodingFileDialog::Result r=KEncodingFileDialog::getOpenURLsAndEncoding(
		KTextEditor::encodingInterface(m_view->document())->encoding(),
	m_view->document()->url().url(),TQString::null,this,i18n("Open File"));

    for (KURL::List::Iterator i=r.URLs.begin(); i != r.URLs.end(); ++i)
    {
      encoding = r.encoding;
      slotOpen ( *i );
    }
  }
  else
  {
    KURL::List l=KFileDialog::getOpenURLs(m_view->document()->url().url(),TQString::null,this,TQString::null);
    for (KURL::List::Iterator i=l.begin(); i != l.end(); ++i)
    {
      slotOpen ( *i );
    }
  }
}

void KWrite::slotOpen( const KURL& url )
{
  if (url.isEmpty()) return;

  if (!TDEIO::NetAccess::exists(url, true, this))
  {
    KMessageBox::error (this, i18n("The given file could not be read, check if it exists or if it is readable for the current user."));
    return;
  }

  if (m_view->document()->isModified() || !m_view->document()->url().isEmpty())
  {
    KWrite *t = new KWrite();
    if (KTextEditor::encodingInterface(t->m_view->document())) KTextEditor::encodingInterface(t->m_view->document())->setEncoding(encoding);
    t->loadURL(url);
  }
  else
  {
    if (KTextEditor::encodingInterface(m_view->document())) KTextEditor::encodingInterface(m_view->document())->setEncoding(encoding);
    loadURL(url);
  }
}

void KWrite::slotFileNameChanged()
{
  if ( ! m_view->document()->url().isEmpty() )
    m_recentFiles->addURL( m_view->document()->url() );
}

void KWrite::newView()
{
  new KWrite(m_view->document());
}

void KWrite::toggleStatusBar()
{
  if( m_paShowStatusBar->isChecked() )
    statusBar()->show();
  else
    statusBar()->hide();
}

void KWrite::editKeys()
{
  KKeyDialog dlg;
  dlg.insert(actionCollection());
  if( m_view )
    dlg.insert(m_view->actionCollection());
  dlg.configure();
}

void KWrite::editToolbars()
{
  saveMainWindowSettings( tdeApp->config(), "MainWindow" );
  KEditToolbar *dlg = new KEditToolbar(guiFactory());
  connect( dlg, TQ_SIGNAL(newToolbarConfig()), this, TQ_SLOT(slotNewToolbarConfig()) );
  dlg->exec();
  delete dlg;
}

void KWrite::slotNewToolbarConfig()
{
  applyMainWindowSettings( tdeApp->config(), "MainWindow" );
}


void KWrite::printNow()
{
  KTextEditor::printInterface(m_view->document())->print ();
}

void KWrite::printDlg()
{
  KTextEditor::printInterface(m_view->document())->printDialog ();
}

void KWrite::newStatus(const TQString &msg)
{
  newCaption();

  statusBar()->changeItem(msg,KWRITE_ID_GEN);
}

void KWrite::newCaption()
{
  if (m_view->document()->url().isEmpty()) {
    setCaption(i18n("Untitled"),m_view->document()->isModified());
  }
  else
  {
    TQString c;
    if (!m_paShowPath->isChecked())
    {
      c = m_view->document()->url().fileName();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = c.left(64) + "...";
    }
    else
    {
      c = m_view->document()->url().prettyURL();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = "..." + c.right(64);
    }

    setCaption (c, m_view->document()->isModified());
  }
}

void KWrite::dragEnterEvent( TQDragEnterEvent *event )
{
  event->accept(KURLDrag::canDecode(event));
}

void KWrite::dropEvent( TQDropEvent *event )
{
  slotDropEvent(event);
}

void KWrite::slotDropEvent( TQDropEvent *event )
{
  KURL::List textlist;

  if (!KURLDrag::decode(event, textlist))
    return;

  for (KURL::List::Iterator i=textlist.begin(); i != textlist.end(); ++i)
    slotOpen (*i);
}

void KWrite::slotEnableActions( bool enable )
{
  TQValueList<TDEAction *> actions = actionCollection()->actions();
  TQValueList<TDEAction *>::ConstIterator it = actions.begin();
  TQValueList<TDEAction *>::ConstIterator end = actions.end();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );

  actions = m_view->actionCollection()->actions();
  it = actions.begin();
  end = actions.end();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );
}

//common config
void KWrite::readConfig(TDEConfig *config)
{
  config->setGroup("General Options");

  m_paShowStatusBar->setChecked( config->readBoolEntry("ShowStatusBar") );
  m_paShowPath->setChecked( config->readBoolEntry("ShowPath") );

  m_recentFiles->loadEntries(config, "Recent Files");

  if (m_view && KTextEditor::configInterface(m_view->document()))
    KTextEditor::configInterface(m_view->document())->readConfig(config);

  if( m_paShowStatusBar->isChecked() )
    statusBar()->show();
  else
    statusBar()->hide();
}

void KWrite::writeConfig(TDEConfig *config)
{
  config->setGroup("General Options");

  config->writeEntry("ShowStatusBar",m_paShowStatusBar->isChecked());
  config->writeEntry("ShowPath",m_paShowPath->isChecked());

  m_recentFiles->saveEntries(config, "Recent Files");

  if (m_view && KTextEditor::configInterface(m_view->document()))
    KTextEditor::configInterface(m_view->document())->writeConfig(config);

  config->sync ();
}

//config file
void KWrite::readConfig()
{
  TDEConfig *config = tdeApp->config();
  readConfig(config);
}

void KWrite::writeConfig()
{
  TDEConfig *config = tdeApp->config();
  writeConfig(config);
}

// session management
void KWrite::restore(TDEConfig *config, int n)
{
  readPropertiesInternal(config, n);
}

void KWrite::readProperties(TDEConfig *config)
{
  readConfig(config);

  if (KTextEditor::sessionConfigInterface(m_view))
    KTextEditor::sessionConfigInterface(m_view)->readSessionConfig(config);
}

void KWrite::saveProperties(TDEConfig *config)
{
  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.find(m_view->document()) + 1);

  if (KTextEditor::sessionConfigInterface(m_view))
    KTextEditor::sessionConfigInterface(m_view)->writeSessionConfig(config);
}

void KWrite::saveGlobalProperties(TDEConfig *config) //save documents
{
  config->setGroup("Number");
  config->writeEntry("NumberOfDocuments",docList.count());

  for (uint z = 1; z <= docList.count(); z++)
  {
     TQString buf = TQString("Document %1").arg(z);
     config->setGroup(buf);

     KTextEditor::Document *doc = docList.at(z - 1);

     if (KTextEditor::configInterface(doc))
       KTextEditor::configInterface(doc)->writeSessionConfig(config);
  }

  for (uint z = 1; z <= winList.count(); z++)
  {
     TQString buf = TQString("Window %1").arg(z);
     config->setGroup(buf);

     config->writeEntry("DocumentNumber",docList.find(winList.at(z-1)->view()->document()) + 1);
  }
}

//restore session
void KWrite::restore()
{
  TDEConfig *config = tdeApp->sessionConfig();

  if (!config)
    return;

  int docs, windows;
  TQString buf;
  KTextEditor::Document *doc;
  KWrite *t;

  config->setGroup("Number");
  docs = config->readNumEntry("NumberOfDocuments");
  windows = config->readNumEntry("NumberOfWindows");

  for (int z = 1; z <= docs; z++)
  {
     buf = TQString("Document %1").arg(z);
     config->setGroup(buf);
     doc=KTextEditor::EditorChooser::createDocument(0,"KTextEditor::Document");

     if (KTextEditor::configInterface(doc))
       KTextEditor::configInterface(doc)->readSessionConfig(config);
     docList.append(doc);
  }

  for (int z = 1; z <= windows; z++)
  {
    buf = TQString("Window %1").arg(z);
    config->setGroup(buf);
    t = new KWrite(docList.at(config->readNumEntry("DocumentNumber") - 1));
    t->restore(config,z);
  }
}

static TDECmdLineOptions options[] =
{
  { "stdin",    I18N_NOOP("Read the contents of stdin"), 0},
  { "encoding <argument>",      I18N_NOOP("Set encoding for the file to open"), 0 },
  { "line <argument>",      I18N_NOOP("Navigate to this line"), 0 },
  { "column <argument>",      I18N_NOOP("Navigate to this column"), 0 },
  { "+[URL]",   I18N_NOOP("Document to open"), 0 },
  TDECmdLineLastOption
};

extern "C" TDE_EXPORT int kdemain(int argc, char **argv)
{
  Kate::Document::setFileChangedDialogsActivated (true);

  TDELocale::setMainCatalogue("kate");         //lukas: set this to have the kwritepart translated using kate message catalog

  // here we go, construct the KWrite version
  // TQString kWriteVersion  = TQString ("%1.%2.%3").arg(KDE::versionMajor() + 1).arg(KDE::versionMinor()).arg(KDE::versionRelease());
  /** The previous version computation scheme (commented out above) worked fine in the 3.5.x days.
      With the new Trinity Rx.y.z scheme the version number gets weird.
      We now hard-code the first two numbers to match the 3.5.x days and only update the last number. */
  TQString kWriteVersion  = TQString ("4.5.%1").arg(KDE::versionMajor());

  TDEAboutData aboutData ( "kwrite",
                         I18N_NOOP("KWrite"),
                         kWriteVersion.latin1(),
                         I18N_NOOP( "KWrite - Text Editor" ), TDEAboutData::License_LGPL_V2,
                         I18N_NOOP( "(c) 2000-2005 The Kate Authors" ), 0 );

  aboutData.addAuthor ("Christoph Cullmann", I18N_NOOP("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
  aboutData.addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  aboutData.addAuthor ("Hamish Rodda",I18N_NOOP("Core Developer"), "rodda@kde.org");
  aboutData.addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
  aboutData.addAuthor ("Charles Samuels", I18N_NOOP("The Editing Commands"), "charles@kde.org");
  aboutData.addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor ("Michael Bartl", I18N_NOOP("Former Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
  aboutData.addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
  aboutData.addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  aboutData.addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");

  aboutData.addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  aboutData.addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  aboutData.addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  aboutData.addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  aboutData.addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  aboutData.addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  aboutData.addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  aboutData.addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  aboutData.addCredit ("Daniel Naber","","");
  aboutData.addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  aboutData.addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  aboutData.addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  aboutData.addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

  aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  TDECmdLineArgs::init( argc, argv, &aboutData );
  TDECmdLineArgs::addCmdLineOptions( options );

  TDEApplication a;

  TDEGlobal::locale()->insertCatalogue("katepart");

  DCOPClient *client = tdeApp->dcopClient();
  if (!client->isRegistered())
  {
    client->attach();
    client->registerAs("kwrite");
  }

  TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

  if (tdeApp->isRestored())
  {
    KWrite::restore();
  }
  else
  {
    bool nav = false;
    int line = 0, column = 0;

    TQTextCodec *codec = args->isSet("encoding") ? TQTextCodec::codecForName(args->getOption("encoding")) : 0;

    if (args->isSet ("line"))
    {
      line = args->getOption ("line").toInt() - 1;
      if (line < 0)
      {
        line = 0;
      }
      nav = true;
    }

    if (args->isSet ("column"))
    {
      column = args->getOption ("column").toInt() - 1;
      if (column < 0)
      {
        column = 0;
      }
      nav = true;
    }

    if ( args->count() == 0 )
    {
        KWrite *t = new KWrite;

        if( args->isSet( "stdin" ) )
        {
          TQTextIStream input(stdin);

          // set chosen codec
          if (codec)
            input.setCodec (codec);

          TQString line;
          TQString text;

          do
          {
            line = input.readLine();
            text.append( line + "\n" );
          } while( !line.isNull() );


          KTextEditor::EditInterface *doc = KTextEditor::editInterface (t->view()->document());
          if( doc )
              doc->setText( text );
        }

        if (nav && KTextEditor::viewCursorInterface(t->view()))
          KTextEditor::viewCursorInterface(t->view())->setCursorPosition (line, column);
    }
    else
    {
      for ( int z = 0; z < args->count(); z++ )
      {
        KWrite *t = new KWrite();

        // this file is no local dir, open it, else warn
        bool noDir = !args->url(z).isLocalFile() || !TQDir (args->url(z).path()).exists();

        if (noDir)
        {
          if (Kate::document (t->view()->document()))
            Kate::Document::setOpenErrorDialogsActivated (false);

          if (codec && KTextEditor::encodingInterface(t->view()->document()))
            KTextEditor::encodingInterface(t->view()->document())->setEncoding(codec->name());

          t->loadURL( args->url( z ) );

          if (Kate::document (t->view()->document()))
            Kate::Document::setOpenErrorDialogsActivated (true);

          if (nav && KTextEditor::viewCursorInterface(t->view()))
            KTextEditor::viewCursorInterface(t->view())->setCursorPosition (line, column);
        }
        else
          KMessageBox::sorry( t, i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(args->url(z).url()) );
      }
    }
  }

  // no window there, uh, ohh, for example borked session config !!!
  // create at least one !!
  if (KWrite::noWindows())
    new KWrite();

  return a.exec ();
}

KWriteEditorChooser::KWriteEditorChooser(TQWidget *):
	KDialogBase(KDialogBase::Plain,i18n("Choose Editor Component"),KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Cancel)
{
	(new TQVBoxLayout(plainPage()))->setAutoAdd(true);
	m_chooser=new KTextEditor::EditorChooser(plainPage(),"Editor Chooser");
	setMainWidget(m_chooser);
	m_chooser->readAppSetting();
}

KWriteEditorChooser::~KWriteEditorChooser() {
;
}

void KWriteEditorChooser::slotOk() {
	m_chooser->writeAppSetting();
	KDialogBase::slotOk();
}
