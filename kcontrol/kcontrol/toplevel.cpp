/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA  02110-1301, USA.
*/

#include <tdeapplication.h>
#include <tdestandarddirs.h>
#include <tdeconfig.h>
#include <kbugreport.h>
#include <tdeaboutapplication.h>
#include <tdemessagebox.h>
#include <kinstance.h>
#include <tdeaction.h>
#include <twin.h>
#include <kiconloader.h>
#include <klineedit.h>

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqtabwidget.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>

#include "indexwidget.h"
#include "searchwidget.h"
#include "helpwidget.h"
#include "aboutwidget.h"
#include "proxywidget.h"
#include "moduletreeview.h"
#include <stdio.h>

#include "toplevel.h"
#include "toplevel.moc"

TopLevel::TopLevel(const char* name)
  : TDEMainWindow( 0, name, (WFlags)WStyle_ContextHelp  )
  , _active(0), dummyAbout(0)
{
  setCaption(TQString::null);

  report_bug = 0;

  // read settings
  TDEConfig *config = TDEGlobal::config();
  config->setGroup("Index");
  TQString viewmode = config->readEntry("ViewMode", "Tree");

  if (viewmode == "Tree")
    KCGlobal::setViewMode(Tree);
  else
    KCGlobal::setViewMode(Icon);

  TQString size = config->readEntry("IconSize", "Medium");
  if (size == "Small")
    KCGlobal::setIconSize(TDEIcon::SizeSmall);
  else if (size == "Large")
    KCGlobal::setIconSize(TDEIcon::SizeLarge);
  else if (size == "Huge")
    KCGlobal::setIconSize(TDEIcon::SizeHuge);
  else
    KCGlobal::setIconSize(TDEIcon::SizeMedium);

  // initialize the entries
  _modules = new ConfigModuleList();
  _modules->readDesktopEntries();

  for ( ConfigModule* m = _modules->first(); m; m = _modules->next() ) {
      connect( m, TQ_SIGNAL( handbookRequest() ), this, TQ_SLOT( slotHandbookRequest() ) );
      connect( m, TQ_SIGNAL( helpRequest() ), this, TQ_SLOT( slotHelpRequest() ) );
  }

  // create the layout box
  _splitter = new TQSplitter( TQt::Horizontal, this );

  TQFrame* leftFrame = new TQFrame ( _splitter );
  TQBoxLayout *leftFrameLayout = new TQVBoxLayout( leftFrame );

  TQFrame* mSearchFrame = new TQFrame( leftFrame );
  leftFrameLayout->addWidget( mSearchFrame );

  TQBoxLayout *searchLayout = new TQHBoxLayout( mSearchFrame );
  searchLayout->setSpacing( KDialog::spacingHint() );
  searchLayout->setMargin( 6 );

  TQPushButton *clearButton = new TQPushButton( mSearchFrame );
  clearButton->setIconSet( TDEApplication::reverseLayout() ?
    SmallIconSet( "clear_left" ) : SmallIconSet("locationbar_erase") );
  searchLayout->addWidget( clearButton );
  TQToolTip::add( clearButton, i18n("Clear search") );

  TQLabel *label = new TQLabel(i18n("Search:"), mSearchFrame );
  searchLayout->addWidget( label );
  
  KLineEdit *searchEdit = new KLineEdit( mSearchFrame );
  clearButton->setFixedHeight( searchEdit->height() );
  connect( clearButton, TQ_SIGNAL( clicked() ), searchEdit, TQ_SLOT( clear() ) );
  label->setBuddy( searchEdit );
  searchLayout->addWidget( searchEdit );
  connect( searchEdit, TQ_SIGNAL( textChanged( const TQString & ) ),
           TQ_SLOT( slotSearchChanged(const TQString &) ) );

  // create the left hand side under search
  _stack = new TQWidgetStack( leftFrame );
  leftFrameLayout->addWidget( _stack );

  // index tab
  _index = new IndexWidget(_modules, this);
  connect(_index, TQ_SIGNAL(moduleActivated(ConfigModule*)),
                  this, TQ_SLOT(activateModule(ConfigModule*)));
  _stack->addWidget( _index );

  connect(_index, TQ_SIGNAL(categorySelected(TQListViewItem*)),
                  this, TQ_SLOT(categorySelected(TQListViewItem*)));

  // search tab
  _search = new SearchWidget(this);
  _search->populateKeywordList(_modules);
  connect(_search, TQ_SIGNAL(moduleSelected(ConfigModule *)),
                  this, TQ_SLOT(activateModule(ConfigModule *)));

  _stack->addWidget( _search );

  // set up the right hand side (the docking area)
  _dock = new DockContainer( _splitter );

  // help widget
  _help = new HelpWidget(_dock);

  _stack->setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Expanding ) );

 // Restore sizes
  config->setGroup("General");
  TQValueList<int> sizes = config->readIntListEntry(  "SplitterSizes" );
  if (!sizes.isEmpty())
     _splitter->setSizes(sizes);

  // That one does the trick ...
  _splitter->setResizeMode( leftFrame, TQSplitter::KeepSize );

  connect(_dock, TQ_SIGNAL(newModule(const TQString&, const TQString&, const TQString&)),
                  this, TQ_SLOT(newModule(const TQString&, const TQString&, const TQString&)));
  connect(_dock, TQ_SIGNAL(changedModule(ConfigModule*)),
          TQ_SLOT(changedModule(ConfigModule*)));

  // set the main view
  setCentralWidget( _splitter );

  // initialize the GUI actions
  setupActions();

  // activate defaults
  if (KCGlobal::viewMode() == Tree)   {
    activateTreeView();
    tree_view->setChecked(true);
  }
  else {
    activateIconView();
    icon_view->setChecked(true);
  }

  // insert the about widget
  if (KCGlobal::isInfoCenter())
  {
      AboutWidget *aw = new AboutWidget( this, 0, _index->firstTreeViewItem());
      connect( aw, TQ_SIGNAL( moduleSelected( ConfigModule * ) ),
               TQ_SLOT( activateModule( ConfigModule * ) ) );
      _dock->setBaseWidget( aw );
      KWin::setIcons(  winId(),
		       TDEGlobal::iconLoader()->loadIcon("hwinfo",  TDEIcon::NoGroup,  32 ),
		       TDEGlobal::iconLoader()->loadIcon("hwinfo",  TDEIcon::NoGroup,  16 ) );
  }
  else
  {
      AboutWidget *aw = new AboutWidget(this);
      connect( aw, TQ_SIGNAL( moduleSelected( ConfigModule * ) ),
                   TQ_SLOT( activateModule( ConfigModule * ) ) );
      _dock->setBaseWidget(aw);
  }
}

TopLevel::~TopLevel()
{
  TDEConfig *config = TDEGlobal::config();
  config->setGroup("Index");
  if (KCGlobal::viewMode() == Tree)
    config->writeEntry("ViewMode", "Tree");
  else
    config->writeEntry("ViewMode", "Icon");

  switch (KCGlobal::iconSize())
    {
    case TDEIcon::SizeSmall:
      config->writeEntry("IconSize", "Small");
      break;
    case TDEIcon::SizeLarge:
      config->writeEntry("IconSize", "Large");
      break;
    case TDEIcon::SizeHuge:
      config->writeEntry("IconSize", "Huge");
      break;
    default:
      config->writeEntry("IconSize", "Medium");
      break;
    }

  config->setGroup("General");
  config->writeEntry("SplitterSizes", _splitter->sizes());

  config->sync();

  delete _modules;
}

bool TopLevel::queryClose()
{
  return _dock->dockModule(0);
}

void TopLevel::setupActions()
{
  KStdAction::quit(this, TQ_SLOT(close()), actionCollection());
  KStdAction::keyBindings(guiFactory(), TQ_SLOT(configureShortcuts()),
actionCollection());
  icon_view = new TDERadioAction
    (i18n("&Icon View"), 0, this, TQ_SLOT(activateIconView()),
     actionCollection(), "activate_iconview");
  icon_view->setExclusiveGroup( "viewmode" );

  tree_view = new TDERadioAction
    (i18n("&Tree View"), 0, this, TQ_SLOT(activateTreeView()),
     actionCollection(), "activate_treeview");
  tree_view->setExclusiveGroup( "viewmode" );

  icon_small = new TDERadioAction
    (i18n("&Small"), 0, this, TQ_SLOT(activateSmallIcons()),
     actionCollection(), "activate_smallicons");
  icon_small->setExclusiveGroup( "iconsize" );

  icon_medium = new TDERadioAction
    (i18n("&Medium"), 0, this, TQ_SLOT(activateMediumIcons()),
     actionCollection(), "activate_mediumicons");
  icon_medium->setExclusiveGroup( "iconsize" );

  icon_large = new TDERadioAction
    (i18n("&Large"), 0, this, TQ_SLOT(activateLargeIcons()),
     actionCollection(), "activate_largeicons");
  icon_large->setExclusiveGroup( "iconsize" );

  icon_huge = new TDERadioAction
    (i18n("&Huge"), 0, this, TQ_SLOT(activateHugeIcons()),
     actionCollection(), "activate_hugeicons");
  icon_huge->setExclusiveGroup( "iconsize" );

  show_hidden_modules = new TDEToggleAction
    (i18n("Show hidden modules"), 0, this, TQ_SLOT(toggleHiddenModules()),
     actionCollection(), "show_hidden_modules");

  about_module = new TDEAction(i18n("About Current Module"), 0, this, TQ_SLOT(aboutModule()), actionCollection(), "help_about_module");
  about_module->setEnabled(false);

  createGUI("kcontrolui.rc");

  report_bug = actionCollection()->action("help_report_bug");
  report_bug->setText(i18n("&Report Bug..."));
  report_bug->disconnect();
  connect(report_bug, TQ_SIGNAL(activated()), TQ_SLOT(reportBug()));
}

void TopLevel::activateIconView()
{
  KCGlobal::setViewMode(Icon);
  _index->activateView(Icon);

  icon_small->setEnabled(true);
  icon_medium->setEnabled(true);
  icon_large->setEnabled(true);
  icon_huge->setEnabled(true);

  switch(KCGlobal::iconSize())
    {
    case TDEIcon::SizeSmall:
      icon_small->setChecked(true);
      break;
    case TDEIcon::SizeLarge:
      icon_large->setChecked(true);
      break;
    case TDEIcon::SizeHuge:
      icon_huge->setChecked(true);
      break;
    default:
      icon_medium->setChecked(true);
      break;
    }
}

void TopLevel::activateTreeView()
{
  KCGlobal::setViewMode(Tree);
  _index->activateView(Tree);

  icon_small->setEnabled(false);
  icon_medium->setEnabled(false);
  icon_large->setEnabled(false);
  icon_huge->setEnabled(false);
}

void TopLevel::activateSmallIcons()
{
  KCGlobal::setIconSize(TDEIcon::SizeSmall);
  _index->reload();
}

void TopLevel::activateMediumIcons()
{
  KCGlobal::setIconSize(TDEIcon::SizeMedium);
  _index->reload();
}

void TopLevel::activateLargeIcons()
{
  KCGlobal::setIconSize(TDEIcon::SizeLarge);
  _index->reload();
}

void TopLevel::activateHugeIcons()
{
  KCGlobal::setIconSize(TDEIcon::SizeHuge);
  _index->reload();
}

void TopLevel::toggleHiddenModules()
{
  KCGlobal::setShowHiddenModules(show_hidden_modules->isChecked());
  _index->reload();
}

void TopLevel::newModule(const TQString &name, const TQString& docPath, const TQString &quickhelp)
{
    setCaption(name, false);

  _help->setText( docPath, quickhelp );

  if (!report_bug) return;

  if(name.isEmpty())
    report_bug->setText(i18n("&Report Bug..."));
  else
    report_bug->setText(i18n("Report Bug on Module %1...").arg( handleAmpersand( name)));
}

void TopLevel::changedModule(ConfigModule *changed)
{
    if (!changed)
        return;
    setCaption(changed->moduleName(), changed->isChanged() );
}

void TopLevel::categorySelected(TQListViewItem *category)
{
  if (_active)
  {
    if (_active->isChanged())
      {
        int res = KMessageBox::warningYesNoCancel(this, _active ?
             i18n("There are unsaved changes in the active module.\n"
                  "Do you want to apply the changes before running "
                  "the new module or discard the changes?") :
             i18n("There are unsaved changes in the active module.\n"
                  "Do you want to apply the changes before exiting "
                  "the Control Center or discard the changes?"),
                            i18n("Unsaved Changes"),
                            KStdGuiItem::apply(),
                            KStdGuiItem::discard());
        if (res == KMessageBox::Yes)
          _active->module()->applyClicked();
        else if (res == KMessageBox::Cancel)
          return;
      }
  }
  _dock->removeModule();
  about_module->setText( i18n( "About Current Module" ) );
  about_module->setIconSet( TQIconSet() );
  about_module->setEnabled( false );

  // insert the about widget
  TQListViewItem *firstItem = category->firstChild();
  TQString caption = static_cast<ModuleTreeItem*>(category)->caption();
  TQString icon = static_cast<ModuleTreeItem*>(category)->icon();
  if( _dock->baseWidget()->isA( "AboutWidget" ) )
  {
    static_cast<AboutWidget *>( _dock->baseWidget() )->setCategory( firstItem, icon, caption);
  }
  else
  {
    AboutWidget *aw = new AboutWidget( this, 0, firstItem, caption );
    connect( aw, TQ_SIGNAL( moduleSelected( ConfigModule * ) ),
             TQ_SLOT( activateModule( ConfigModule * ) ) );
    _dock->setBaseWidget( aw );
  }
}


void TopLevel::activateModule(ConfigModule *mod)
{
  if ( _dock->module() == mod )
     return;

  // tell the index to display the module
  _index->makeVisible(mod);

  // tell the index to mark this module as loaded
  _index->makeSelected(mod);

  // dock it
  if (!_dock->dockModule(mod))
  {
     if ( _dock->module() )
     {
       _index->makeVisible(_active);
       _index->makeSelected(_active);
     }
     return;
  }

  _active=mod;

  if (mod->aboutData())
  {
     about_module->setText(i18n("Help menu->about <modulename>", "About %1").arg(
                             handleAmpersand( mod->moduleName())));
     about_module->setIcon(mod->icon());
     about_module->setEnabled(true);
  }
  else
  {
     about_module->setText(i18n("About Current Module"));
     about_module->setIconSet(TQIconSet());
     about_module->setEnabled(false);
  }
}

void TopLevel::deleteDummyAbout()
{
  delete dummyAbout;
  dummyAbout = 0;
}

void TopLevel::slotSearchChanged(const TQString& search)
{
    if (search.isEmpty())
       _stack->raiseWidget(_index);
    else
    {
       _stack->raiseWidget(_search);
       _search->searchTextChanged(search);
    }
}

void TopLevel::slotHelpRequest()
{
    _help->display(_help->text(), TQCursor::pos ( ), _dock);
}

void TopLevel::slotHandbookRequest()
{
    _help->handbookRequest();
}

void TopLevel::reportBug()
{
    // this assumes the user only opens one bug report at a time
    static char buffer[128];

    dummyAbout = 0;
    bool deleteit = false;

    if (!_active) // report against kcontrol
        dummyAbout = const_cast<TDEAboutData*>(TDEGlobal::instance()->aboutData());
    else
    {
        if (_active->aboutData())
            dummyAbout = const_cast<TDEAboutData*>(_active->aboutData());
        else
        {
            snprintf(buffer, sizeof(buffer), "kcm%s", _active->library().latin1());
            dummyAbout = new TDEAboutData(buffer, _active->moduleName().utf8(), "2.0");
            deleteit = true;
        }
    }
    KBugReport *br = new KBugReport(this, false, dummyAbout);
    if (deleteit)
        connect(br, TQ_SIGNAL(finished()), TQ_SLOT(deleteDummyAbout()));
    else
        dummyAbout = 0;
    br->show();
}

void TopLevel::aboutModule()
{
    TDEAboutApplication dlg(_active->aboutData());
    dlg.exec();
}

TQString TopLevel::handleAmpersand( TQString modulename ) const
{
   if( modulename.contains( '&' )) // double it
   {
      for( int i = modulename.length();
           i >= 0;
           --i )
         if( modulename[ i ] == '&' )
             modulename.insert( i, "&" );
   }
   return modulename;
}
