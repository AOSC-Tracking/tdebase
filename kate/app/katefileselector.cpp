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
#include "katefileselector.h"
#include "katefileselector.moc"

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "kbookmarkhandler.h"

#include "kactionselector.h"

#include <tqlayout.h>
#include <tqtoolbutton.h>
#include <tqhbox.h>
#include <tqvbox.h>
#include <tqlabel.h>
#include <tqstrlist.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>
#include <tqapplication.h>
#include <tqlistbox.h>
#include <tqscrollbar.h>
#include <tqspinbox.h>
#include <tqgroupbox.h>
#include <tqcheckbox.h>
#include <tqregexp.h>
#include <tqdockarea.h>
#include <tqtimer.h>
#include <tqdir.h>

#include <kapplication.h>
#include <kiconloader.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kprotocolinfo.h>
#include <kdiroperator.h>
#include <kconfig.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <ktoolbarbutton.h>
#include <tqtoolbar.h>
#include <kpopupmenu.h>
#include <kdialog.h>
#include <kdebug.h>
//END Includes

//BEGIN Toolbar
 // from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
static void silenceQToolBar(TQtMsgType, const char *){}

// helper classes to be able to have a toolbar without move handle
KateFileSelectorToolBar::KateFileSelectorToolBar(TQWidget *parent)
  : KToolBar( parent, "Kate FileSelector Toolbar", true )
{
	setMinimumWidth(10);
}

KateFileSelectorToolBar::~KateFileSelectorToolBar(){}

void KateFileSelectorToolBar::setMovingEnabled( bool)
{
	KToolBar::setMovingEnabled(false);
}


KateFileSelectorToolBarParent::KateFileSelectorToolBarParent(TQWidget *parent)
	:TQFrame(parent),m_tb(0){}
KateFileSelectorToolBarParent::~KateFileSelectorToolBarParent(){}
void KateFileSelectorToolBarParent::setToolBar(KateFileSelectorToolBar *tb)
{
	m_tb=tb;
}

void KateFileSelectorToolBarParent::resizeEvent ( TQResizeEvent * )
{
	if (m_tb)
	{
		setMinimumHeight(m_tb->sizeHint().height());
		m_tb->resize(width(),height());
	}
}
//END

//BEGIN Constructor/destructor

KateFileSelector::KateFileSelector( KateMainWindow *mainWindow,
                                    KateViewManager *viewManager,
                                    TQWidget * parent, const char * name )
    : TQVBox (parent, name),
      mainwin(mainWindow),
      viewmanager(viewManager)
{
  mActionCollection = new KActionCollection( this );

  TQtMsgHandler oldHandler = tqInstallMsgHandler( silenceQToolBar );

  KateFileSelectorToolBarParent *tbp=new KateFileSelectorToolBarParent(this);
  toolbar = new KateFileSelectorToolBar(tbp);
  tbp->setToolBar(toolbar);
  toolbar->setMovingEnabled(false);
  toolbar->setFlat(true);
  tqInstallMsgHandler( oldHandler );

  cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
  cmbPath->setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Fixed ));
  KURLCompletion* cmpl = new KURLCompletion(KURLCompletion::DirCompletion);
  cmbPath->setCompletionObject( cmpl );
  cmbPath->setAutoDeleteCompletionObject( true );
  cmbPath->listBox()->installEventFilter( this );

  dir = new KDirOperator(KURL(), this, "operator");
  dir->setView(KFile::/* Simple */Detail);
  dir->view()->setSelectionMode(KFile::Extended);
  connect ( dir, TQT_SIGNAL( viewChanged(KFileView *) ),
                   this, TQT_SLOT( selectorViewChanged(KFileView *) ) );
  setStretchFactor(dir, 2);

  KActionCollection *coll = dir->actionCollection();
  // some shortcuts of diroperator that clashes with Kate
  coll->action( "delete" )->setShortcut( KShortcut( ALT + Key_Delete ) );
  coll->action( "reload" )->setShortcut( KShortcut( ALT + Key_F5 ) );
  coll->action( "back" )->setShortcut( KShortcut( ALT + SHIFT + Key_Left ) );
  coll->action( "forward" )->setShortcut( KShortcut( ALT + SHIFT + Key_Right ) );
  // some consistency - reset up for dir too
  coll->action( "up" )->setShortcut( KShortcut( ALT + SHIFT + Key_Up ) );
  coll->action( "home" )->setShortcut( KShortcut( CTRL + ALT + Key_Home ) );

  // bookmarks action!
  KActionMenu *acmBookmarks = new KActionMenu( i18n("Bookmarks"), "bookmark",
        mActionCollection, "bookmarks" );
  acmBookmarks->setDelayed( false );
  bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->popupMenu() );
  TQHBox* filterBox = new TQHBox(this);

  btnFilter = new TQToolButton( filterBox );
  btnFilter->setIconSet( SmallIconSet("filter" ) );
  btnFilter->setToggleButton( true );
  filter = new KHistoryCombo( true, filterBox, "filter");
  filter->setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Fixed ));
  filterBox->setStretchFactor(filter, 2);
  connect( btnFilter, TQT_SIGNAL( clicked() ), this, TQT_SLOT( btnFilterClick() ) );

  connect( filter, TQT_SIGNAL( activated(const TQString&) ),
                   TQT_SLOT( slotFilterChange(const TQString&) ) );
  connect( filter, TQT_SIGNAL( returnPressed(const TQString&) ),
           filter, TQT_SLOT( addToHistory(const TQString&) ) );

  // kaction for the dir sync method
  acSyncDir = new KAction( i18n("Current Document Folder"), "curfiledir", 0,
        TQT_TQOBJECT(this), TQT_SLOT( setActiveDocumentDir() ), mActionCollection, "sync_dir" );
  toolbar->setIconText( KToolBar::IconOnly );
  toolbar->setIconSize( 16 );
  toolbar->setEnableContextMenu( false );

  connect( cmbPath, TQT_SIGNAL( urlActivated( const KURL&  )),
             this,  TQT_SLOT( cmbPathActivated( const KURL& ) ));
  connect( cmbPath, TQT_SIGNAL( returnPressed( const TQString&  )),
             this,  TQT_SLOT( cmbPathReturnPressed( const TQString& ) ));
  connect(dir, TQT_SIGNAL(urlEntered(const KURL&)),
             this, TQT_SLOT(dirUrlEntered(const KURL&)) );

  connect(dir, TQT_SIGNAL(finishedLoading()),
             this, TQT_SLOT(dirFinishedLoading()) );

  // enable dir sync button if current doc has a valid URL
  connect ( viewmanager, TQT_SIGNAL( viewChanged() ),
              this, TQT_SLOT( kateViewChanged() ) );

  // Connect the bookmark handler
  connect( bookmarkHandler, TQT_SIGNAL( openURL( const TQString& )),
           this, TQT_SLOT( setDir( const TQString& ) ) );

  waitingUrl = TQString::null;

  // whatsthis help
  TQWhatsThis::add( cmbPath,
       i18n("<p>Here you can enter a path for a folder to display."
            "<p>To go to a folder previously entered, press the arrow on "
            "the right and choose one. <p>The entry has folder "
            "completion. Right-click to choose how completion should behave.") );
  TQWhatsThis::add( filter,
        i18n("<p>Here you can enter a name filter to limit which files are displayed."
             "<p>To clear the filter, toggle off the filter button to the left."
             "<p>To reapply the last filter used, toggle on the filter button." ) );
  TQWhatsThis::add( btnFilter,
        i18n("<p>This button clears the name filter when toggled off, or "
             "reapplies the last filter used when toggled on.") );

}

KateFileSelector::~KateFileSelector()
{
}
//END Constroctor/Destrctor

//BEGIN Public Methods

void KateFileSelector::readConfig(KConfig *config, const TQString & name)
{
  dir->setViewConfig( config, name + ":view" );
  dir->readConfig(config, name + ":dir");
  dir->setView( KFile::Default );
  dir->view()->setSelectionMode(KFile::Extended);
  config->setGroup( name );

  // set up the toolbar
  setupToolbar( config );

  cmbPath->setMaxItems( config->readNumEntry( "pathcombo history len", 9 ) );
  cmbPath->setURLs( config->readPathListEntry( "dir history" ) );
  // if we restore history
  if ( config->readBoolEntry( "restore location", true ) || kapp->isRestored() ) {
    TQString loc( config->readPathEntry( "location" ) );
    if ( ! loc.isEmpty() ) {
//       waitingDir = loc;
//       TQTimer::singleShot(0, this, TQT_SLOT(initialDirChangeHack()));
      setDir( loc );
    }
  }

  // else is automatic, as cmpPath->setURL is called when a location is entered.

  filter->setMaxCount( config->readNumEntry( "filter history len", 9 ) );
  filter->setHistoryItems( config->readListEntry("filter history"), true );
  lastFilter = config->readEntry( "last filter" );
  TQString flt("");
  if ( config->readBoolEntry( "restore last filter", true ) || kapp->isRestored() )
    flt = config->readEntry("current filter");
  filter->lineEdit()->setText( flt );
  slotFilterChange( flt );

  autoSyncEvents = config->readNumEntry( "AutoSyncEvents", 0 );
}

void KateFileSelector::initialDirChangeHack()
{
  setDir( waitingDir );
}

void KateFileSelector::setupToolbar( KConfig *config )
{
  toolbar->clear();
  TQStringList tbactions = config->readListEntry( "toolbar actions", ',' );
  if ( tbactions.isEmpty() ) {
    // reasonable collection for default toolbar
    tbactions << "up" << "back" << "forward" << "home" <<
                "short view" << "detailed view" <<
                "bookmarks" << "sync_dir";
  }
  KAction *ac;
  for ( TQStringList::Iterator it=tbactions.begin(); it != tbactions.end(); ++it ) {
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = mActionCollection->action( (*it).latin1() );
    else
      ac = dir->actionCollection()->action( (*it).latin1() );
    if ( ac )
      ac->plug( toolbar );
  }
}

void KateFileSelector::writeConfig(KConfig *config, const TQString & name)
{
  dir->writeConfig(config,name + ":dir");

  config->setGroup( name );
  config->writeEntry( "pathcombo history len", cmbPath->maxItems() );
  TQStringList l;
  for (int i = 0; i < cmbPath->count(); i++) {
    l.append( cmbPath->text( i ) );
  }
  config->writePathEntry( "dir history", l );
  config->writePathEntry( "location", cmbPath->currentText() );

  config->writeEntry( "filter history len", filter->maxCount() );
  config->writeEntry( "filter history", filter->historyItems() );
  config->writeEntry( "current filter", filter->currentText() );
  config->writeEntry( "last filter", lastFilter );
  config->writeEntry( "AutoSyncEvents", autoSyncEvents );
}

void KateFileSelector::setView(KFile::FileView view)
{
  dir->setView(view);
  dir->view()->setSelectionMode(KFile::Extended);
}

//END Public Methods

//BEGIN Public Slots

void KateFileSelector::slotFilterChange( const TQString & nf )
{
  TQString f = nf.stripWhiteSpace();
  bool empty = f.isEmpty() || f == "*";
  TQToolTip::remove( btnFilter );
  if ( empty ) {
    dir->clearFilter();
    filter->lineEdit()->setText( TQString::null );
    TQToolTip::add( btnFilter,
        TQString( i18n("Apply last filter (\"%1\")") ).arg( lastFilter ) );
  }
  else {
    dir->setNameFilter( f );
    lastFilter = f;
    TQToolTip::add( btnFilter, i18n("Clear filter") );
  }
  btnFilter->setOn( !empty );
  dir->updateDir();
  // this will be never true after the filter has been used;)
  btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );

}

bool kateFileSelectorIsReadable ( const KURL& url )
{
  if ( !url.isLocalFile() )
    return true; // what else can we say?

  TQDir dir (url.path());
  return dir.exists ();
}

void KateFileSelector::setDir( KURL u )
{
  KURL newurl;

  if ( !u.isValid() )
    newurl.setPath( TQDir::homeDirPath() );
  else
    newurl = u;

  TQString pathstr = newurl.path(+1);
  newurl.setPath(pathstr);

  if ( !kateFileSelectorIsReadable ( newurl ) )
    newurl.cd(TQString::fromLatin1(".."));

  if ( !kateFileSelectorIsReadable (newurl) )
     newurl.setPath( TQDir::homeDirPath() );

  dir->setURL(newurl, true);
}

//END Public Slots

//BEGIN Private Slots

void KateFileSelector::cmbPathActivated( const KURL& u )
{
   cmbPathReturnPressed( u.url() );
}

void KateFileSelector::cmbPathReturnPressed( const TQString& u )
{
  KURL typedURL( u );
  if ( typedURL.hasPass() )
    typedURL.setPass( TQString::null );

  TQStringList urls = cmbPath->urls();
  urls.remove( typedURL.url() );
  urls.prepend( typedURL.url() );
  cmbPath->setURLs( urls, KURLComboBox::RemoveBottom );
  dir->setFocus();
  dir->setURL( KURL(u), true );
}

void KateFileSelector::dirUrlEntered( const KURL& u )
{
  cmbPath->setURL( u );
}

void KateFileSelector::dirFinishedLoading()
{
}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void KateFileSelector::btnFilterClick()
{
  if ( !btnFilter->isOn() ) {
    slotFilterChange( TQString::null );
  }
  else {
    filter->lineEdit()->setText( lastFilter );
    slotFilterChange( lastFilter );
  }
}

//FIXME crash on shutdown
void KateFileSelector::setActiveDocumentDir()
{
//   kdDebug(13001)<<"KateFileSelector::setActiveDocumentDir()"<<endl;
  KURL u = mainwin->activeDocumentUrl();
//   kdDebug(13001)<<"URL: "<<u.prettyURL()<<endl;
  if (!u.isEmpty())
    setDir( u.upURL() );
//   kdDebug(13001)<<"... setActiveDocumentDir() DONE!"<<endl;
}

void KateFileSelector::kateViewChanged()
{
  if ( autoSyncEvents & DocumentChanged )
  {
//     kdDebug(13001)<<"KateFileSelector::do a sync ()"<<endl;
    // if visible, sync
    if ( isVisible() ) {
      setActiveDocumentDir();
      waitingUrl = TQString::null;
    }
    // else set waiting url
    else {
      KURL u = mainwin->activeDocumentUrl();
      if (!u.isEmpty())
        waitingUrl = u.directory();
    }
  }

  // TODO: make sure the button is disabled if the directory is unreadable, eg
  //       the document URL has protocol http
  acSyncDir->setEnabled( ! mainwin->activeDocumentUrl().directory().isEmpty() );
}

void KateFileSelector::selectorViewChanged( KFileView * newView )
{
  newView->setSelectionMode(KFile::Extended);
}

//END Private Slots

//BEGIN Protected

void KateFileSelector::focusInEvent( TQFocusEvent * )
{
   dir->setFocus();
}

void KateFileSelector::showEvent( TQShowEvent * )
{
    // sync if we should
    if ( autoSyncEvents & GotVisible ) {
//     kdDebug(13001)<<"syncing fs on show"<<endl;
      setActiveDocumentDir();
      waitingUrl = TQString::null;
    }
    // else, if we have a waiting URL set it
    else if ( ! waitingUrl.isEmpty() ) {
      setDir( waitingUrl );
      waitingUrl = TQString::null;
   }
}

bool KateFileSelector::eventFilter( TQObject* o, TQEvent *e )
{
  /*
      This is rather unfortunate, but:
      TQComboBox does not support setting the size of the listbox to something
      reasonable. Even using listbox->setVariableWidth() does not yield a
      satisfying result, something is wrong with the handling of the sizehint.
      And the popup is rather useless, if the paths are only partly visible.
  */
  TQListBox *lb = cmbPath->listBox();
  if ( TQT_BASE_OBJECT(o) == TQT_BASE_OBJECT(lb) && e->type() == TQEvent::Show ) {
    int add = lb->height() < lb->contentsHeight() ? lb->verticalScrollBar()->width() : 0;
    int w = QMIN( mainwin->width(), lb->contentsWidth() + add );
    lb->resize( w, lb->height() );
    // TODO - move the listbox to a suitable place if nessecary
    // TODO - decide if it is worth caching the size while untill the contents
    //        are changed.
  }
  // TODO - same thing for the completion popup?
  return TQWidget::eventFilter( o, e );
}

//END Protected

//BEGIN ACtionLBItem
/*
   QListboxItem that can store and return a string,
   used for the toolbar action selector.
*/
class ActionLBItem : public TQListBoxPixmap {
  public:
  ActionLBItem( TQListBox *lb=0,
                const TQPixmap &pm = TQPixmap(),
                const TQString &text=TQString::null,
                const TQString &str=TQString::null ) :
    TQListBoxPixmap( lb, pm, text ),
    _str(str) {};
  TQString idstring() { return _str; };
  private:
    TQString _str;
};
//END ActionLBItem

//BEGIN KFSConfigPage
////////////////////////////////////////////////////////////////////////////////
// KFSConfigPage implementation
////////////////////////////////////////////////////////////////////////////////
KFSConfigPage::KFSConfigPage( TQWidget *parent, const char *name, KateFileSelector *kfs )
  : Kate::ConfigPage( parent, name ),
    fileSelector( kfs ),
    m_changed( false )
{
  TQVBoxLayout *lo = new TQVBoxLayout( this );
  int spacing = KDialog::spacingHint();
  lo->setSpacing( spacing );

  // Toolbar - a lot for a little...
  TQGroupBox *gbToolbar = new TQGroupBox( 1, Qt::Vertical, i18n("Toolbar"), this );
  acSel = new KActionSelector( gbToolbar );
  acSel->setAvailableLabel( i18n("A&vailable actions:") );
  acSel->setSelectedLabel( i18n("S&elected actions:") );
  lo->addWidget( gbToolbar );
  connect( acSel, TQT_SIGNAL( added( TQListBoxItem * ) ), this, TQT_SLOT( slotMyChanged() ) );
  connect( acSel, TQT_SIGNAL( removed( TQListBoxItem * ) ), this, TQT_SLOT( slotMyChanged() ) );
  connect( acSel, TQT_SIGNAL( movedUp( TQListBoxItem * ) ), this, TQT_SLOT( slotMyChanged() ) );
  connect( acSel, TQT_SIGNAL( movedDown( TQListBoxItem * ) ), this, TQT_SLOT( slotMyChanged() ) );

  // Sync
  TQGroupBox *gbSync = new TQGroupBox( 1, Qt::Horizontal, i18n("Auto Synchronization"), this );
  cbSyncActive = new TQCheckBox( i18n("When a docu&ment becomes active"), gbSync );
  cbSyncShow = new TQCheckBox( i18n("When the file selector becomes visible"), gbSync );
  lo->addWidget( gbSync );
  connect( cbSyncActive, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotMyChanged() ) );
  connect( cbSyncShow, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotMyChanged() ) );

  // Histories
  TQHBox *hbPathHist = new TQHBox ( this );
  TQLabel *lbPathHist = new TQLabel( i18n("Remember &locations:"), hbPathHist );
  sbPathHistLength = new TQSpinBox( hbPathHist );
  lbPathHist->setBuddy( sbPathHistLength );
  lo->addWidget( hbPathHist );
  connect( sbPathHistLength, TQT_SIGNAL( valueChanged ( int ) ), this, TQT_SLOT( slotMyChanged() ) );

  TQHBox *hbFilterHist = new TQHBox ( this );
  TQLabel *lbFilterHist = new TQLabel( i18n("Remember &filters:"), hbFilterHist );
  sbFilterHistLength = new TQSpinBox( hbFilterHist );
  lbFilterHist->setBuddy( sbFilterHistLength );
  lo->addWidget( hbFilterHist );
  connect( sbFilterHistLength, TQT_SIGNAL( valueChanged ( int ) ), this, TQT_SLOT( slotMyChanged() ) );

  // Session
  TQGroupBox *gbSession = new TQGroupBox( 1, Qt::Horizontal, i18n("Session"), this );
  cbSesLocation = new TQCheckBox( i18n("Restore loca&tion"), gbSession );
  cbSesFilter = new TQCheckBox( i18n("Restore last f&ilter"), gbSession );
  lo->addWidget( gbSession );
  connect( cbSesLocation, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotMyChanged() ) );
  connect( cbSesFilter, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotMyChanged() ) );

  // make it look nice
  lo->addStretch( 1 );

  // be helpfull
  /*
  TQWhatsThis::add( lbAvailableActions, i18n(
        "<p>Available actions for the toolbar. To add an action, select it here "
        "and press the add (<strong>-&gt;</strong>) button" ) );
  TQWhatsThis::add( lbUsedActions, i18n(
        "<p>Actions used in the toolbar. To remove an action, select it and "
        "press the remove (<strong>&lt;-</strong>) button."
        "<p>To change the order of the actions, use the Up and Down buttons to "
        "move the selected action.") );
  */
  TQString lhwt( i18n(
        "<p>Decides how many locations to keep in the history of the location "
        "combo box.") );
  TQWhatsThis::add( lbPathHist, lhwt );
  TQWhatsThis::add( sbPathHistLength, lhwt );
  TQString fhwt( i18n(
        "<p>Decides how many filters to keep in the history of the filter "
        "combo box.") );
  TQWhatsThis::add( lbFilterHist, fhwt );
  TQWhatsThis::add( sbFilterHistLength, fhwt );
  TQString synwt( i18n(
        "<p>These options allow you to have the File Selector automatically "
        "change location to the folder of the active document on certain "
        "events."
        "<p>Auto synchronization is <em>lazy</em>, meaning it will not take "
        "effect until the file selector is visible."
        "<p>None of these are enabled by default, but you can always sync the "
        "location by pressing the sync button in the toolbar.") );
  TQWhatsThis::add( gbSync, synwt );
  TQWhatsThis::add( cbSesLocation, i18n(
        "<p>If this option is enabled (default), the location will be restored "
        "when you start Kate.<p><strong>Note</strong> that if the session is "
        "handled by the KDE session manager, the location is always restored.") );
  TQWhatsThis::add( cbSesFilter, i18n(
        "<p>If this option is enabled (default), the current filter will be "
        "restored when you start Kate.<p><strong>Note</strong> that if the "
        "session is handled by the KDE session manager, the filter is always "
        "restored."
        "<p><strong>Note</strong> that some of the autosync settings may "
        "override the restored location if on.") );

  init();

}

void KFSConfigPage::apply()
{
  if ( ! m_changed )
    return;

  m_changed = false;

  KConfig *config = kapp->config();
  config->setGroup( "fileselector" );
  // toolbar
  TQStringList l;
  TQListBoxItem *item = acSel->selectedListBox()->firstItem();
  ActionLBItem *aItem;
  while ( item )
  {
    aItem = (ActionLBItem*)item;
    if ( aItem )
    {
      l << aItem->idstring();
    }
    item = item->next();
  }
  config->writeEntry( "toolbar actions", l );
  fileSelector->setupToolbar( config );
  // sync
  int s = 0;
  if ( cbSyncActive->isChecked() )
    s |= KateFileSelector::DocumentChanged;
  if ( cbSyncShow->isChecked() )
    s |= KateFileSelector::GotVisible;
  fileSelector->autoSyncEvents = s;

  // histories
  fileSelector->cmbPath->setMaxItems( sbPathHistLength->value() );
  fileSelector->filter->setMaxCount( sbFilterHistLength->value() );
  // session - theese are read/written directly to the app config,
  //           as they are not needed during operation.
  config->writeEntry( "restore location", cbSesLocation->isChecked() );
  config->writeEntry( "restore last filter", cbSesFilter->isChecked() );
}

void KFSConfigPage::reload()
{
  // hmm, what is this supposed to do, actually??
  init();
  m_changed = false;
}
void KFSConfigPage::init()
{
  KConfig *config = kapp->config();
  config->setGroup( "fileselector" );
  // toolbar
  TQStringList l = config->readListEntry( "toolbar actions", ',' );
  if ( l.isEmpty() ) // default toolbar
    l << "up" << "back" << "forward" << "home" <<
                "short view" << "detailed view" <<
                "bookmarks" << "sync_dir";

  // actions from diroperator + two of our own
  TQStringList allActions;
  allActions << "up" << "back" << "forward" << "home" <<
                "reload" << "mkdir" << "delete" <<
                "short view" << "detailed view" /*<< "view menu" <<
                "show hidden" << "properties"*/ <<
                "bookmarks" << "sync_dir";
  TQRegExp re("&(?=[^&])");
  KAction *ac;
  TQListBox *lb;
  for ( TQStringList::Iterator it=allActions.begin(); it != allActions.end(); ++it ) {
    lb = l.contains( *it ) ? acSel->selectedListBox() : acSel->availableListBox();
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = fileSelector->actionCollection()->action( (*it).latin1() );
    else
      ac = fileSelector->dirOperator()->actionCollection()->action( (*it).latin1() );
    if ( ac )
      new ActionLBItem( lb, SmallIcon( ac->icon() ), ac->text().replace( re, "" ), *it );
  }

  // sync
  int s = fileSelector->autoSyncEvents;
  cbSyncActive->setChecked( s & KateFileSelector::DocumentChanged );
  cbSyncShow->setChecked( s & KateFileSelector::GotVisible );
  // histories
  sbPathHistLength->setValue( fileSelector->cmbPath->maxItems() );
  sbFilterHistLength->setValue( fileSelector->filter->maxCount() );
  // session
  cbSesLocation->setChecked( config->readBoolEntry( "restore location", true ) );
  cbSesFilter->setChecked( config->readBoolEntry( "restore last filter", true ) );
}

void KFSConfigPage::slotMyChanged()
{
  m_changed = true;
  slotChanged();
}
//END KFSConfigPage
// kate: space-indent on; indent-width 2; replace-tabs on;
