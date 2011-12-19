/*
   This file is part of the Kate text editor of the KDE project.

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

   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/
// TODO
// Icons
// Direct shortcut setting
//BEGIN Includes
#include "kateexternaltools.h"
#include "kateexternaltools.moc"
#include "katedocmanager.h"
#include "kateviewmanager.h"
#include "kateapp.h"

#include "katemainwindow.h"

#include <kate/view.h>
#include <kate/document.h>

#include <klistbox.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kmimetypechooser.h>
#include <kconfig.h>
#include <krun.h>
#include <kicondialog.h>
#include <kpopupmenu.h>
#include <kdebug.h>

#include <tqbitmap.h>
#include <tqcombobox.h>
#include <tqfile.h>
#include <tqpushbutton.h>
#include <tqlineedit.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqlistbox.h>
#include <tqmap.h>
#include <tqregexp.h>
#include <tqtextedit.h>
#include <tqtoolbutton.h>
#include <tqwhatsthis.h>

#include <stdlib.h>
#include <unistd.h>
//END Includes

KateExternalToolsCommand *KateExternalToolsCommand::s_self=0;

//BEGIN KateExternalTool
KateExternalTool::KateExternalTool( const TQString &name,
                      const TQString &command,
                      const TQString &icon,
                      const TQString &tryexec,
                      const TQStringList &mimetypes,
                      const TQString &acname,
                      const TQString &cmdname,
                      int save )
  : name ( name ),
    command ( command ),
    icon ( icon ),
    tryexec ( tryexec ),
    mimetypes ( mimetypes ),
    acname ( acname ),
    cmdname ( cmdname ),
    save ( save )
{
  //if ( ! tryexec.isEmpty() )
    hasexec = checkExec();
}

bool KateExternalTool::checkExec()
{
  // if tryexec is empty, it is the first word of command
  if ( tryexec.isEmpty() )
    tryexec = command.section( " ", 0, 0, TQString::SectionSkipEmpty );

  // NOTE this code is modified taken from kdesktopfile.cpp, from KDesktopFile::tryExec()
  if (!tryexec.isEmpty()) {
    if (tryexec[0] == '/') {
      if (::access(TQFile::encodeName(tryexec), R_OK | X_OK))
	return false;

      m_exec = tryexec;
    } else {
      // !!! Sergey A. Sukiyazov <corwin@micom.don.ru> !!!
      // Environment PATH may contain filenames in 8bit locale cpecified
      // encoding (Like a filenames).
      TQStringList dirs = TQStringList::split(':', TQFile::decodeName(::getenv("PATH")));
      TQStringList::Iterator it(dirs.begin());
      bool match = false;
      for (; it != dirs.end(); ++it)
      {
	TQString fName = *it + "/" + tryexec;
	if (::access(TQFile::encodeName(fName), R_OK | X_OK) == 0)
	{
	  match = true;
          m_exec = fName;
	  break;
	}
      }
      // didn't match at all
      if (!match)
        return false;
    }
    return true;
  }
  return false;
}

bool KateExternalTool::valid( const TQString &mt ) const
{
  return mimetypes.isEmpty() || mimetypes.contains( mt );
}
//END KateExternalTool

//BEGIN KateExternalToolsCommand
KateExternalToolsCommand::KateExternalToolsCommand() : Kate::Command() {
	m_inited=false;
	reload();
}

TQStringList KateExternalToolsCommand::cmds () {
	return m_list;
}

KateExternalToolsCommand *KateExternalToolsCommand::self () {
	if (s_self) return s_self;
	s_self=new KateExternalToolsCommand;
	return s_self;
}

void KateExternalToolsCommand::reload () {
  m_list.clear();
  m_map.clear();

  KConfig config("externaltools", false, false, "appdata");
  config.setGroup("Global");
  TQStringList tools = config.readListEntry("tools");


  for( TQStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
      continue;


    config.setGroup( *it );

    KateExternalTool t = KateExternalTool(
        config.readEntry( "name", "" ),
        config.readEntry( "command", ""),
        config.readEntry( "icon", ""),
        config.readEntry( "executable", ""),
        config.readListEntry( "mimetypes" ),
        config.readEntry( "acname", "" ),
        config.readEntry( "cmdname", "" ) );
    // FIXME test for a command name first!
	if ( t.hasexec && (!t.cmdname.isEmpty())) {
		m_list.append("exttool-"+t.cmdname);
		m_map.insert("exttool-"+t.cmdname,t.acname);
	}
  }
  if (m_inited) {
	  Kate::Document::unregisterCommand(this);
	  Kate::Document::registerCommand(this);
   }
  else m_inited=true;
}

bool KateExternalToolsCommand::exec (Kate::View *view, const TQString &cmd, TQString &) {
	TQWidget *wv=tqt_dynamic_cast<TQWidget*>(view);
	if (!wv) {
// 		kdDebug(13001)<<"KateExternalToolsCommand::exec: Could not get view widget"<<endl;
		return false;
	}
  KateMDI::MainWindow *dmw=tqt_dynamic_cast<KateMDI::MainWindow*>(wv->topLevelWidget());
	if (!dmw) {
// 		kdDebug(13001)<<"KateExternalToolsCommand::exec: Could not get main window"<<endl;
		return false;
	}
// 	kdDebug(13001)<<"cmd="<<cmd.stripWhiteSpace()<<endl;
	TQString actionName=m_map[cmd.stripWhiteSpace()];
	if (actionName.isEmpty()) return false;
// 	kdDebug(13001)<<"actionName is not empty:"<<actionName<<endl;
	KateExternalToolsMenuAction *a=
		tqt_dynamic_cast<KateExternalToolsMenuAction*>(dmw->action("tools_external"));
	if (!a) return false;
// 	kdDebug(13001)<<"trying to find action"<<endl;
	KAction *a1=a->actionCollection()->action(static_cast<const char *>(actionName.utf8()));
	if (!a1) return false;
// 	kdDebug(13001)<<"activating action"<<endl;
	a1->activate();
	return true;
}

bool KateExternalToolsCommand::help (Kate::View *, const TQString &, TQString &) {
	return false;
}
//END KateExternalToolsCommand

//BEGIN KateExternalToolAction
KateExternalToolAction::KateExternalToolAction( TQObject *parent,
             const char *name, KateExternalTool *t)
  : KAction( parent, name ),
    tool ( t )
{
  setText( t->name );
  if ( ! t->icon.isEmpty() )
    setIconSet( SmallIconSet( t->icon ) );

  connect( this ,TQT_SIGNAL(activated()), this, TQT_SLOT(slotRun()) );
}

bool KateExternalToolAction::expandMacro( const TQString &str, TQStringList &ret )
{
  KateMainWindow *mw = (KateMainWindow*)parent()->parent();

  Kate::View *view = mw->viewManager()->activeView();
  if ( ! view ) return false;


  if ( str == "URL" )
    ret += mw->activeDocumentUrl().url();
  else if ( str == "directory" ) // directory of current doc
    ret += mw->activeDocumentUrl().directory();
  else if ( str == "filename" )
    ret += mw->activeDocumentUrl().fileName();
  else if ( str == "line" ) // cursor line of current doc
    ret += TQString::number( view->cursorLine() );
  else if ( str == "col" ) // cursor col of current doc
    ret += TQString::number( view->cursorColumn() );
  else if ( str == "selection" ) // selection of current doc if any
    ret += view->getDoc()->selection();
  else if ( str == "text" ) // text of current doc
    ret += view->getDoc()->text();
  else if ( str == "URLs" ) {
    for( Kate::Document *doc = KateDocManager::self()->firstDocument(); doc; doc = KateDocManager::self()->nextDocument() )
      if ( ! doc->url().isEmpty() )
        ret += doc->url().url();
  } else
    return false;
  return true;
}

KateExternalToolAction::~KateExternalToolAction() {
	delete(tool);
}

void KateExternalToolAction::slotRun()
{
  // expand the macros in command if any,
  // and construct a command with an absolute path
  TQString cmd = tool->command;

  if ( ! expandMacrosShellQuote( cmd ) )
  {
    KMessageBox::sorry( (KateMainWindow*)parent()->parent(),
                         i18n("Failed to expand the command '%1'.").arg( cmd ),
                         i18n( "Kate External Tools") );
    return;
  }
  kdDebug(13001)<<"externaltools: Running command: "<<cmd<<endl;

  // save documents if requested
  KateMainWindow *mw = (KateMainWindow*)parent()->parent();
  if ( tool->save == 1 )
    mw->viewManager()->activeView()->document()->save();
  else if ( tool->save == 2 )
    mw->actionCollection()->action("file_save_all")->activate();

  KRun::runCommand( cmd, tool->tryexec, tool->icon );
}
//END KateExternalToolAction

//BEGIN KateExternalToolsMenuAction
KateExternalToolsMenuAction::KateExternalToolsMenuAction( const TQString &text,
                                               TQObject *parent,
                                               const char* name,
                                               KateMainWindow *mw )
    : KActionMenu( text, parent, name ),
      mainwindow( mw )
{

  m_actionCollection = new KActionCollection( mainwindow );

  connect(KateDocManager::self(),TQT_SIGNAL(documentChanged()),this,TQT_SLOT(slotDocumentChanged()));

  reload();
}

void KateExternalToolsMenuAction::reload()
{
  m_actionCollection->clear ();
  popupMenu()->clear();

  // load all the tools, and create a action for each of them
  KConfig *config = new KConfig( "externaltools", false, false, "appdata" );
  config->setGroup( "Global" );
  TQStringList tools = config->readListEntry( "tools" );

  // if there are tools that are present but not explicitly removed,
  // add them to the end of the list
  config->setReadDefaults( true );
  TQStringList dtools = config->readListEntry( "tools" );
  int gver = config->readNumEntry( "version", 1 );
  config->setReadDefaults( false );

  int ver = config->readNumEntry( "version" );
  if ( ver <= gver )
  {
    TQStringList removed = config->readListEntry( "removed" );
    bool sepadded = false;
    for (TQStringList::iterator itg = dtools.begin(); itg != dtools.end(); ++itg )
    {
      if ( ! tools.contains( *itg ) &&
            ! removed.contains( *itg ) )
      {
        if ( ! sepadded )
        {
          tools << "---";
          sepadded = true;
        }
        tools << *itg;
      }
    }

    config->writeEntry( "tools", tools );
    config->sync();
    config->writeEntry( "version", gver );
  }

  for( TQStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
    {
      popupMenu()->insertSeparator();
      // a separator
      continue;
    }

    config->setGroup( *it );

    KateExternalTool *t = new KateExternalTool(
        config->readEntry( "name", "" ),
        config->readEntry( "command", ""),
        config->readEntry( "icon", ""),
        config->readEntry( "executable", ""),
        config->readListEntry( "mimetypes" ),
        config->readEntry( "acname", "" ),
        config->readEntry( "cmdname", "" ),
        config->readNumEntry( "save", 0 ) );

    if ( t->hasexec )
      insert( new KateExternalToolAction( m_actionCollection, t->acname.ascii(), t ) );
  }

  m_actionCollection->readShortcutSettings( "Shortcuts", config );
  slotDocumentChanged();
  delete config;
}

void KateExternalToolsMenuAction::slotDocumentChanged()
{
  // try to enable/disable to match current mime type
  Kate::DocumentExt *de = documentExt( KateDocManager::self()->activeDocument() );
  if ( de )
  {
    TQString mt = de->mimeType();
    TQStringList l;
    bool b;

    KActionPtrList actions = m_actionCollection->actions();
    for (KActionPtrList::iterator it = actions.begin(); it != actions.end(); ++it )
    {
      KateExternalToolAction *action = tqt_dynamic_cast<KateExternalToolAction*>(*it);
      if ( action )
      {
        l = action->tool->mimetypes;
        b = ( ! l.count() || l.contains( mt ) );
        action->setEnabled( b );
      }
    }
  }
}
//END KateExternalToolsMenuAction

//BEGIN ToolItem
/**
 * This is a TQListBoxItem, that has a KateExternalTool. The text is the Name
 * of the tool.
 */
class ToolItem : public TQListBoxPixmap
{
  public:
    ToolItem( TQListBox *lb, const TQPixmap &icon, KateExternalTool *tool )
        : TQListBoxPixmap( lb, icon, tool->name ),
          tool ( tool )
    {;}

    ~ToolItem() {};

    KateExternalTool *tool;
};
//END ToolItem

//BEGIN KateExternalToolServiceEditor
KateExternalToolServiceEditor::KateExternalToolServiceEditor( KateExternalTool *tool,
                                TQWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Edit External Tool"), KDialogBase::Ok|KDialogBase::Cancel ),
      tool( tool )
{
    // create a entry for each property
    // fill in the values from the service if available
  TQWidget *w = new TQWidget( this );
  setMainWidget( w );
  TQGridLayout *lo = new TQGridLayout( w );
  lo->setSpacing( KDialogBase::spacingHint() );

  TQLabel *l;

  leName = new TQLineEdit( w );
  lo->addWidget( leName, 1, 2 );
  l = new TQLabel( leName, i18n("&Label:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 1, 1 );
  if ( tool ) leName->setText( tool->name );
  TQWhatsThis::add( leName, i18n(
      "The name will be displayed in the 'Tools->External' menu") );

  btnIcon = new KIconButton( w );
  btnIcon->setIconSize( KIcon::SizeSmall );
  lo->addWidget( btnIcon, 1, 3 );
  if ( tool && !tool->icon.isEmpty() )
    btnIcon->setIcon( tool->icon );

  teCommand = new TQTextEdit( w );
  lo->addMultiCellWidget( teCommand, 2, 2, 2, 3 );
  l = new TQLabel( teCommand, i18n("S&cript:"), w );
  l->setAlignment( Qt::AlignTop|Qt::AlignRight );
  lo->addWidget( l, 2, 1 );
  if ( tool ) teCommand->setText( tool->command );
  TQWhatsThis::add( teCommand, i18n(
      "<p>The script to execute to invoke the tool. The script is passed "
      "to /bin/sh for execution. The following macros "
      "will be expanded:</p>"
      "<ul><li><code>%URL</code> - the URL of the current document."
      "<li><code>%URLs</code> - a list of the URLs of all open documents."
      "<li><code>%directory</code> - the URL of the directory containing "
      "the current document."
      "<li><code>%filename</code> - the filename of the current document."
      "<li><code>%line</code> - the current line of the text cursor in the "
      "current view."
      "<li><code>%column</code> - the column of the text cursor in the "
      "current view."
      "<li><code>%selection</code> - the selected text in the current view."
      "<li><code>%text</code> - the text of the current document.</ul>" ) );


  leExecutable = new TQLineEdit( w );
  lo->addMultiCellWidget( leExecutable, 3, 3, 2, 3 );
  l = new TQLabel( leExecutable, i18n("&Executable:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 3, 1 );
  if ( tool ) leExecutable->setText( tool->tryexec );
  TQWhatsThis::add( leExecutable, i18n(
      "The executable used by the command. This is used to check if a tool "
      "should be displayed; if not set, the first word of <em>command</em> "
      "will be used.") );

  leMimetypes = new TQLineEdit( w );
  lo->addWidget( leMimetypes, 4, 2 );
  l = new TQLabel( leMimetypes, i18n("&Mime types:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 4, 1 );
  if ( tool ) leMimetypes->setText( tool->mimetypes.join("; ") );
  TQWhatsThis::add( leMimetypes, i18n(
      "A semicolon-separated list of mime types for which this tool should "
      "be available; if this is left empty, the tool is always available. "
      "To choose from known mimetypes, press the button on the right.") );

  TQToolButton *btnMTW = new TQToolButton(w);
  lo->addWidget( btnMTW, 4, 3 );
  btnMTW->setIconSet(TQIconSet(SmallIcon("wizard")));
  connect(btnMTW, TQT_SIGNAL(clicked()), this, TQT_SLOT(showMTDlg()));
  TQWhatsThis::add( btnMTW, i18n(
      "Click for a dialog that can help you creating a list of mimetypes.") );

  cmbSave = new TQComboBox(w);
  lo->addMultiCellWidget( cmbSave, 5, 5, 2, 3 );
  l = new TQLabel( cmbSave, i18n("&Save:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 5, 1 );
  TQStringList sl;
  sl << i18n("None") << i18n("Current Document") << i18n("All Documents");
  cmbSave->insertStringList( sl );
  if ( tool ) cmbSave->setCurrentItem( tool->save );
  TQWhatsThis::add( cmbSave, i18n(
      "You can elect to save the current or all [modified] documents prior to "
      "running the command. This is helpful if you want to pass URLs to "
      "an application like, for example, an FTP client.") );


  leCmdLine = new TQLineEdit( w );
  lo->addMultiCellWidget( leCmdLine, 6, 6, 2, 3 );
  l = new TQLabel( leCmdLine, i18n("&Command line name:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 6, 1 );
  if ( tool ) leCmdLine->setText( tool->cmdname );
  TQWhatsThis::add( leCmdLine, i18n(
      "If you specify a name here, you can invoke the command from the view "
      "command lines with exttool-the_name_you_specified_here. "
      "Please do not use spaces or tabs in the name."));

}

void KateExternalToolServiceEditor::slotOk()
{
  if ( leName->text().isEmpty() ||
       teCommand->text().isEmpty() )
  {
    KMessageBox::information( this, i18n("You must specify at least a name and a command") );
    return;
  }

  KDialogBase::slotOk();
}

void KateExternalToolServiceEditor::showMTDlg()
{
  TQString text = i18n("Select the MimeTypes for which to enable this tool.");
  TQStringList list = TQStringList::split( TQRegExp("\\s*;\\s*"), leMimetypes->text() );
  KMimeTypeChooserDialog d( i18n("Select Mime Types"), text, list, "text", this );
  if ( d.exec() == KDialogBase::Accepted ) {
    leMimetypes->setText( d.chooser()->mimeTypes().join(";") );
  }
}
//END KateExternalToolServiceEditor

//BEGIN KateExternalToolsConfigWidget
KateExternalToolsConfigWidget::KateExternalToolsConfigWidget( TQWidget *parent, const char* name )
  : Kate::ConfigPage( parent, name ),
    m_changed( false )
{
  TQGridLayout *lo = new TQGridLayout( this, 5, 5, 0, KDialog::spacingHint() );

  lbTools = new KListBox( this );
  lo->addMultiCellWidget( lbTools, 1, 4, 0, 3 );
  connect( lbTools, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(slotSelectionChanged()) );

  btnNew = new TQPushButton( i18n("&New..."), this );
  lo->addWidget( btnNew, 5, 0 );
  connect( btnNew, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotNew()) );

  btnRemove = new TQPushButton( i18n("&Remove"), this );
  lo->addWidget( btnRemove, 5, 2 );
  connect( btnRemove, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotRemove()) );

  btnEdit = new TQPushButton( i18n("&Edit..."), this );
  lo->addWidget( btnEdit, 5, 1 );
  connect( btnEdit, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotEdit()) );

  TQPushButton *b = new TQPushButton( i18n("Insert &Separator"), this );
  lo->addWidget( b, 5, 3 );
  connect( b, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotInsertSeparator()) );

  btnMoveUp = new TQPushButton( SmallIconSet("up"), "", this );
  lo->addWidget( btnMoveUp, 2, 4 );
  connect( btnMoveUp, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotMoveUp()) );

  btnMoveDwn = new TQPushButton( SmallIconSet("down"), "", this );
  lo->addWidget( btnMoveDwn, 3, 4 );
  connect( btnMoveDwn, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotMoveDown()) );

  connect( lbTools, TQT_SIGNAL( doubleClicked ( TQListBoxItem * ) ), this,  TQT_SLOT( slotEdit() ) );

  lo->setRowStretch( 1, 1 );
  lo->setRowStretch( 4, 1 );
  lo->setColStretch( 0, 1 );
  lo->setColStretch( 1, 1 );
  lo->setColStretch( 2, 1 );


  TQWhatsThis::add( lbTools, i18n(
      "This list shows all the configured tools, represented by their menu text.") );

  config = new KConfig("externaltools", false, false, "appdata");
  reload();
  slotSelectionChanged();
}

KateExternalToolsConfigWidget::~KateExternalToolsConfigWidget()
{
  delete config;
}

void KateExternalToolsConfigWidget::reload()
{
  //m_tools.clear();
  lbTools->clear();

  // load the files from a KConfig
  config->setGroup( "Global" );
  TQStringList tools = config->readListEntry("tools");

  for( TQStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
    {
      new TQListBoxText( lbTools, "---" );
    }
    else
    {
      config->setGroup( *it );

      KateExternalTool *t = new KateExternalTool(
          config->readEntry( "name", "" ),
          config->readEntry( "command", ""),
          config->readEntry( "icon", ""),
          config->readEntry( "executable", ""),
          config->readListEntry( "mimetypes" ),
          config->readEntry( "acname" ),
	  config->readEntry( "cmdname"),
          config->readNumEntry( "save", 0 ) );

      if ( t->hasexec ) // we only show tools that are also in the menu.
        new ToolItem( lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t );
    }
  }
  m_changed = false;
}

TQPixmap KateExternalToolsConfigWidget::blankIcon()
{
  TQPixmap pm( KIcon::SizeSmall, KIcon::SizeSmall );
  pm.fill();
  pm.setMask( pm.createHeuristicMask() );
  return pm;
}

void KateExternalToolsConfigWidget::apply()
{
  if ( ! m_changed )
    return;
  m_changed = false;

  // save a new list
  // save each item
  TQStringList tools;
  for ( uint i = 0; i < lbTools->count(); i++ )
  {
    if ( lbTools->text( i ) == "---" )
    {
      tools << "---";
      continue;
    }
    KateExternalTool *t = ((ToolItem*)lbTools->item( i ))->tool;
//     kdDebug(13001)<<"adding tool: "<<t->name<<endl;
    tools << t->acname;

    config->setGroup( t->acname );
    config->writeEntry( "name", t->name );
    config->writeEntry( "command", t->command );
    config->writeEntry( "icon", t->icon );
    config->writeEntry( "executable", t->tryexec );
    config->writeEntry( "mimetypes", t->mimetypes );
    config->writeEntry( "acname", t->acname );
    config->writeEntry( "cmdname", t->cmdname );
    config->writeEntry( "save", t->save );
  }

  config->setGroup("Global");
  config->writeEntry( "tools", tools );

  // if any tools was removed, try to delete their groups, and
  // add the group names to the list of removed items.
  if ( m_removed.count() )
  {
    for ( TQStringList::iterator it = m_removed.begin(); it != m_removed.end(); ++it )
    {
      if ( config->hasGroup( *it ) )
        config->deleteGroup( *it  );
    }
    TQStringList removed = config->readListEntry( "removed" );
    removed += m_removed;

    // clean up the list of removed items, so that it does not contain
    // non-existing groups (we can't remove groups from a non-owned global file).
    config->sync();
    TQStringList::iterator it1 = removed.begin();
    while( it1 != removed.end() )
    {
      if ( ! config->hasGroup( *it1 ) )
        it1 = removed.remove( it1 );
      else
        ++it1;
    }
    config->writeEntry( "removed", removed );
  }

  config->sync();
}

void KateExternalToolsConfigWidget::slotSelectionChanged()
{
  // update button state
  bool hs =  lbTools->selectedItem() != 0;
  btnEdit->setEnabled( hs && dynamic_cast<ToolItem*>(lbTools->selectedItem()) );
  btnRemove->setEnabled( hs );
  btnMoveUp->setEnabled( ( lbTools->currentItem() > 0 ) && hs );
  btnMoveDwn->setEnabled( ( lbTools->currentItem() < (int)lbTools->count()-1 )&&hs );
}

void KateExternalToolsConfigWidget::slotNew()
{
  // display a editor, and if it is OK'd, create a new tool and
  // create a listbox item for it
  KateExternalToolServiceEditor editor( 0, this );

  if ( editor.exec() )
  {
    KateExternalTool *t = new KateExternalTool(
      editor.leName->text(),
      editor.teCommand->text(),
      editor.btnIcon->icon(),
      editor.leExecutable->text(),
      TQStringList::split( TQRegExp("\\s*;\\s*"), editor.leMimetypes->text() ) );

    // This is sticky, it does not change again, so that shortcuts sticks
    // TODO check for dups
    t->acname = "externaltool_" + TQString(t->name).replace( TQRegExp("\\W+"), "" );

    new ToolItem ( lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t );

    slotChanged();
    m_changed = true;
  }
}

void KateExternalToolsConfigWidget::slotRemove()
{
  // add the tool action name to a list of removed items,
  // remove the current listbox item
  if ( lbTools->currentItem() > -1 ) {
    ToolItem *i = dynamic_cast<ToolItem*>(lbTools->selectedItem());
    if ( i )
      m_removed << i->tool->acname;

    lbTools->removeItem( lbTools->currentItem() );
    slotChanged();
    m_changed = true;
  }
}

void KateExternalToolsConfigWidget::slotEdit()
{
  if( !dynamic_cast<ToolItem*>(lbTools->selectedItem()) ) return;
  // show the item in an editor
  KateExternalTool *t = ((ToolItem*)lbTools->selectedItem())->tool;
  KateExternalToolServiceEditor editor( t, this);
  config->setGroup( "Editor" );
  editor.resize( config->readSizeEntry( "Size" ) );
  if ( editor.exec() /*== KDialogBase::Ok*/ )
  {

      bool elementChanged = ( ( editor.btnIcon->icon() != t->icon ) || (editor.leName->text() != t->name ) ) ;

    t->name = editor.leName->text();
    t->cmdname = editor.leCmdLine->text();
    t->command = editor.teCommand->text();
    t->icon = editor.btnIcon->icon();
    t->tryexec = editor.leExecutable->text();
    t->mimetypes = TQStringList::split( TQRegExp("\\s*;\\s*"), editor.leMimetypes->text() );
    t->save = editor.cmbSave->currentItem();

    //if the icon has changed or name changed, I have to renew the listbox item :S
    if ( elementChanged )
    {
      int idx = lbTools->index( lbTools->selectedItem() );
      lbTools->removeItem( idx );
      lbTools->insertItem( new ToolItem( 0, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t ), idx );
    }

    slotChanged();
    m_changed = true;
  }

  config->setGroup( "Editor" );
  config->writeEntry( "Size", editor.size() );
  config->sync();
}

void KateExternalToolsConfigWidget::slotInsertSeparator()
{
  lbTools->insertItem( "---", lbTools->currentItem()+1 );
  slotChanged();
  m_changed = true;
}

void KateExternalToolsConfigWidget::slotMoveUp()
{
  // move the current item in the listbox upwards if possible
  TQListBoxItem *item = lbTools->selectedItem();
  if ( ! item ) return;

  int idx = lbTools->index( item );

  if ( idx < 1 ) return;

  if ( dynamic_cast<ToolItem*>(item) )
  {
    KateExternalTool *tool = ((ToolItem*)item)->tool;
    lbTools->removeItem( idx );
    lbTools->insertItem( new ToolItem( 0, tool->icon.isEmpty() ? blankIcon() : SmallIcon( tool->icon ), tool ), idx-1 );
  }
  else // a separator!
  {
    lbTools->removeItem( idx );
    lbTools->insertItem( new TQListBoxText( 0, "---" ), idx-1 );
  }

  lbTools->setCurrentItem( idx - 1 );
  slotSelectionChanged();
  slotChanged();
  m_changed = true;
}

void KateExternalToolsConfigWidget::slotMoveDown()
{
  // move the current item in the listbox downwards if possible
  TQListBoxItem *item = lbTools->selectedItem();
  if ( ! item ) return;

  uint idx = lbTools->index( item );

  if ( idx > lbTools->count()-1 ) return;

  if ( dynamic_cast<ToolItem*>(item) )
  {
    KateExternalTool *tool = ((ToolItem*)item)->tool;
    lbTools->removeItem( idx );
    lbTools->insertItem( new ToolItem( 0, tool->icon.isEmpty() ? blankIcon() : SmallIcon( tool->icon ), tool ), idx+1 );
  }
  else // a separator!
  {
    lbTools->removeItem( idx );
    lbTools->insertItem( new TQListBoxText( 0, "---" ), idx+1 );
  }

  lbTools->setCurrentItem( idx+1 );
  slotSelectionChanged();
  slotChanged();
  m_changed = true;
}
//END KateExternalToolsConfigWidget
// kate: space-indent on; indent-width 2; replace-tabs on;
