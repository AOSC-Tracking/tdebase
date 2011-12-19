/*
    KAppfinder, the KDE application finder

    Copyright (c) 2002-2003 Tobias Koenig <tokoe@kde.org>

    Based on code written by Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <kaccelmanager.h>
#include <kapplication.h>
#include <kbuttonbox.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprogress.h>
#include <kservice.h>
#include <kservicegroup.h>
#include <kstandarddirs.h>
#include <kstdguiitem.h>
#include <kstartupinfo.h>

#include <tqaccel.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqdir.h>
#include <tqregexp.h>

#include "toplevel.h"

TopLevel::TopLevel( const TQString &destDir, TQWidget *parent, const char *name )
  : KDialog( parent, name, true )
{
  setCaption( i18n( "KAppfinder" ) );
  TQVBoxLayout *layout = new TQVBoxLayout( this, marginHint() );

  TQLabel *label = new TQLabel( i18n( "The application finder looks for non-KDE "
                                    "applications on your system and adds "
                                    "them to the KDE menu system. "
                                    "Click 'Scan' to begin, select the desired applications and then click 'Apply'."), this);
  label->setAlignment( AlignAuto | WordBreak );
  layout->addWidget( label );

  layout->addSpacing( 5 );

  mListView = new TQListView( this );
  mListView->addColumn( i18n( "Application" ) );
  mListView->addColumn( i18n( "Description" ) );
  mListView->addColumn( i18n( "Command" ) );
  mListView->setMinimumSize( 300, 300 );
  mListView->setRootIsDecorated( true );
  mListView->setAllColumnsShowFocus( true );
  mListView->setSelectionMode(TQListView::NoSelection);
  layout->addWidget( mListView );

  mProgress = new KProgress( this );
  mProgress->setPercentageVisible( false );
  layout->addWidget( mProgress );

  mSummary = new TQLabel( i18n( "Summary:" ), this );
  layout->addWidget( mSummary );

  KButtonBox* bbox = new KButtonBox( this );
  mScanButton = bbox->addButton( KGuiItem( i18n( "Scan" ), "find"), TQT_TQOBJECT(this), TQT_SLOT( slotScan() ) );
  bbox->addStretch( 5 );
  mSelectButton = bbox->addButton( i18n( "Select All" ), TQT_TQOBJECT(this),
                                   TQT_SLOT( slotSelectAll() ) );
  mSelectButton->setEnabled( false );
  mUnSelectButton = bbox->addButton( i18n( "Unselect All" ), TQT_TQOBJECT(this),
                                     TQT_SLOT( slotUnselectAll() ) );
  mUnSelectButton->setEnabled( false );
  bbox->addStretch( 5 );
  mApplyButton = bbox->addButton( KStdGuiItem::apply(), TQT_TQOBJECT(this), TQT_SLOT( slotCreate() ) );
  mApplyButton->setEnabled( false );
  bbox->addButton( KStdGuiItem::close(), TQT_TQOBJECT(kapp), TQT_SLOT( quit() ) );
  bbox->layout();

  layout->addWidget( bbox );

	connect( kapp, TQT_SIGNAL( lastWindowClosed() ), kapp, TQT_SLOT( quit() ) );

  mAppCache.setAutoDelete( true );

  adjustSize();

  mDestDir = destDir;
  mDestDir = mDestDir.replace( TQRegExp( "^~/" ), TQDir::homeDirPath() + "/" );
	
  KStartupInfo::appStarted();

  TQAccel *accel = new TQAccel( this );
  accel->connectItem( accel->insertItem( Key_Q + CTRL ), kapp, TQT_SLOT( quit() ) );

  KAcceleratorManager::manage( this );
}


TopLevel::~TopLevel()
{
  mAppCache.clear();
}

TQListViewItem* TopLevel::addGroupItem( TQListViewItem *parent, const TQString &relPath,
                                       const TQString &name )
{
  KServiceGroup::Ptr root = KServiceGroup::group( relPath );
  if( !root )
		  return 0L;
  KServiceGroup::List list = root->entries();

  KServiceGroup::List::ConstIterator it;
  for ( it = list.begin(); it != list.end(); ++it ) {
    KSycocaEntry *p = (*it);
    if ( p->isType( KST_KServiceGroup ) ) {
      KServiceGroup* serviceGroup = static_cast<KServiceGroup*>( p );
      if ( TQString( "%1%2/" ).arg( relPath ).arg( name ) == serviceGroup->relPath() ) {
        TQListViewItem* retval;
        if ( parent )
          retval = parent->firstChild();
        else
          retval = mListView->firstChild();

        while ( retval ) {
          if ( retval->text( 0 ) == serviceGroup->caption() )
            return retval;

          retval = retval->nextSibling();
        }

        TQListViewItem *item;
        if ( parent )
          item = new TQListViewItem( parent, serviceGroup->caption() );
        else
          item = new TQListViewItem( mListView, serviceGroup->caption() );

        item->setPixmap( 0, SmallIcon( serviceGroup->icon() ) );
        item->setOpen( true );
        return item;
      }
    }
  }

  return 0;
}

void TopLevel::slotScan()
{
  KIconLoader* loader = KGlobal::iconLoader();

  mTemplates = KGlobal::dirs()->findAllResources( "data", "kappfinder/apps/*.desktop", true );

  mAppCache.clear();

  mFound = 0;
  int count = mTemplates.count();

  mScanButton->setEnabled( false );
  mProgress->setPercentageVisible( true );
  mProgress->setTotalSteps( count );
  mProgress->setValue( 0 );

  mListView->clear();

  TQStringList::Iterator it;
  for ( it = mTemplates.begin(); it != mTemplates.end(); ++it ) {
    // eye candy
    mProgress->setProgress( mProgress->progress() + 1 );

    TQString desktopName = *it;
    int i = desktopName.findRev('/');
    desktopName = desktopName.mid(i+1);
    i = desktopName.findRev('.');
    if (i != -1)
       desktopName = desktopName.left(i);

    bool found;
    found = KService::serviceByDesktopName(desktopName);
    if (found)
       continue;

    found = KService::serviceByMenuId("kde-"+desktopName+".desktop");
    if (found)
       continue; 

    found = KService::serviceByMenuId("gnome-"+desktopName+".desktop");
    if (found)
       continue; 

    KDesktopFile desktop( *it, true );

    // copy over the desktop file, if exists
    if ( scanDesktopFile( mAppCache, *it, mDestDir ) ) {
      TQString relPath = *it;
      int pos = relPath.find( "kappfinder/apps/" );
      relPath = relPath.mid( pos + strlen( "kappfinder/apps/" ) );
      relPath = relPath.left( relPath.findRev( '/' ) + 1 );
      TQStringList dirList = TQStringList::split( '/', relPath );

      TQListViewItem *dirItem = 0;
      TQString tmpRelPath = TQString::null;

      TQStringList::Iterator tmpIt;
      for ( tmpIt = dirList.begin(); tmpIt != dirList.end(); ++tmpIt ) {
        dirItem = addGroupItem( dirItem, tmpRelPath, *tmpIt );
        tmpRelPath += *tmpIt + '/';
      }

      mFound++;

      TQCheckListItem *item;
      if ( dirItem )
        item = new TQCheckListItem( dirItem, desktop.readName(), TQCheckListItem::CheckBox );
      else
        item = new TQCheckListItem( mListView, desktop.readName(), TQCheckListItem::CheckBox );

      item->setPixmap( 0, loader->loadIcon( desktop.readIcon(), KIcon::Small ) );
      item->setText( 1, desktop.readGenericName() );
      item->setText( 2, desktop.readPathEntry( "Exec" ) );
      if ( desktop.readBoolEntry( "X-StandardInstall" ) )
        item->setOn( true );

      AppLnkCache* cache = mAppCache.last();
      if ( cache )
        cache->item = item;
    }

    // update summary
    TQString sum( i18n( "Summary: found %n application",
                       "Summary: found %n applications", mFound ) );
    mSummary->setText( sum );
  }

  // stop scanning
  mProgress->setValue( 0 );
  mProgress->setPercentageVisible( false );

  mScanButton->setEnabled( true );

  if ( mFound > 0 ) {
    mApplyButton->setEnabled( true );
    mSelectButton->setEnabled( true );
    mUnSelectButton->setEnabled( true );
  }
}

void TopLevel::slotSelectAll()
{
  AppLnkCache* cache;
  for ( cache = mAppCache.first(); cache; cache = mAppCache.next() )
    cache->item->setOn( true );
}

void TopLevel::slotUnselectAll()
{
  AppLnkCache* cache;
  for ( cache = mAppCache.first(); cache; cache = mAppCache.next() )
    cache->item->setOn( false );
}

void TopLevel::slotCreate()
{
  // copy template files
  mAdded = 0;
  createDesktopFiles( mAppCache, mAdded );

  // decorate directories
  decorateDirs( mDestDir );

  KService::rebuildKSycoca(this);

  TQString message( i18n( "%n application was added to the KDE menu system.",
                         "%n applications were added to the KDE menu system.", mAdded ) );
  KMessageBox::information( this, message, TQString::null, "ShowInformation" );
}

#include "toplevel.moc"
