/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999 - 2002 Chris Schlaeger <cs@kde.org>

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

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
    Please do not commit any changes without consulting me first. Thanks!

*/

#include <tqlineedit.h>
#include <tqspinbox.h>
#include <tqwhatsthis.h>

#include <kdebug.h>
#include <tdefiledialog.h>
#include <tdeio/netaccess.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kstandarddirs.h>
#include <tdeaccelmanager.h>

#include "WorkSheet.h"
#include "WorkSheetSettings.h"
#include "ProcessController.h"

#include "Workspace.h"

Workspace::Workspace( TQWidget* parent, const char* name )
  : TQTabWidget( parent, name )
{
  TDEAcceleratorManager::setNoAccel(this);
 
  mSheetList.setAutoDelete( true );
  mAutoSave = true;

  connect( this, TQT_SIGNAL( currentChanged( TQWidget* ) ),
           TQT_SLOT( updateCaption( TQWidget* ) ) );

  TQWhatsThis::add( this, i18n( "This is your work space. It holds your worksheets. You need "
                               "to create a new worksheet (Menu File->New) before "
                               "you can drag sensors here." ) );
}

Workspace::~Workspace()
{
  /* This workaround is necessary to prevent a crash when the last
   * page is not the current page. It seems like the the signal/slot
   * administration data is already deleted but slots are still
   * being triggered. TODO: I need to ask the Trolls about this. */

  disconnect( this, TQT_SIGNAL( currentChanged( TQWidget* ) ), this,
              TQT_SLOT( updateCaption( TQWidget* ) ) );
}

void Workspace::saveProperties( TDEConfig *cfg )
{
  cfg->writePathEntry( "WorkDir", mWorkDir );
  cfg->writeEntry( "CurrentSheet", tabLabel( currentPage() ) );

  TQPtrListIterator<WorkSheet> it( mSheetList);

  TQStringList list;
  for ( int i = 0; it.current(); ++it, ++i )
    if ( !(*it)->fileName().isEmpty() )
      list.append( (*it)->fileName() );

  cfg->writePathEntry( "Sheets", list );
}

void Workspace::readProperties( TDEConfig *cfg )
{
  TQString currentSheet;

  mWorkDir = cfg->readPathEntry( "WorkDir" );

  if ( mWorkDir.isEmpty() ) {
    /* If workDir is not specified in the config file, it's
     * probably the first time the user has started KSysGuard. We
     * then "restore" a special default configuration. */
    TDEStandardDirs* kstd = TDEGlobal::dirs();
    kstd->addResourceType( "data", "share/apps/ksysguard" );

    mWorkDir = kstd->saveLocation( "data", "ksysguard" );

    TQString origFile = kstd->findResource( "data", "SystemLoad.sgrd" );
    TQString newFile = mWorkDir + "/" + i18n( "System Load" ) + ".sgrd";
    if ( !origFile.isEmpty() )
      restoreWorkSheet( origFile, newFile );

    origFile = kstd->findResource( "data", "ProcessTable.sgrd" );
    newFile = mWorkDir + "/" + i18n( "Process Table" ) + ".sgrd";
    if ( !origFile.isEmpty() )
      restoreWorkSheet( origFile, newFile );

    currentSheet = i18n( "System Load" );
  } else {
    currentSheet = cfg->readEntry( "CurrentSheet" );
    TQStringList list = cfg->readPathListEntry( "Sheets" );
    for ( TQStringList::Iterator it = list.begin(); it != list.end(); ++it )
      restoreWorkSheet( *it );
  }

  // Determine visible sheet.
  TQPtrListIterator<WorkSheet> it( mSheetList );
  for ( ; it.current(); ++it )
    if ( currentSheet == tabLabel(*it) ) {
      showPage( *it );
      break;
    }
}

void Workspace::newWorkSheet()
{
  /* Find a name of the form "Sheet %d" that is not yet used by any
   * of the existing worksheets. */
  TQString sheetName;
  bool found;

  int i = 1;
  do {
    sheetName = i18n( "Sheet %1" ).arg( i++ );
    TQPtrListIterator<WorkSheet> it( mSheetList );
    found = false;
    for ( ; it.current() && !found; ++it )
      if ( tabLabel(*it) == sheetName )
        found = true;
  } while ( found );

  WorkSheetSettings dlg( this );
  dlg.setSheetTitle( sheetName );
  if ( dlg.exec() ) {
    WorkSheet* sheet = new WorkSheet( dlg.rows(), dlg.columns(), dlg.interval(), this );
    sheet->setTitle( dlg.sheetTitle() );
    insertTab( sheet, dlg.sheetTitle() );
    mSheetList.append( sheet );
    showPage( sheet );
    connect( sheet, TQT_SIGNAL( sheetModified( TQWidget* ) ),
             TQT_SLOT( updateCaption( TQWidget* ) ) );
    connect( sheet, TQT_SIGNAL( titleChanged( TQWidget* ) ),
             TQT_SLOT( updateSheetTitle( TQWidget* ) ) );
  }
}

bool Workspace::saveOnQuit()
{
  TQPtrListIterator<WorkSheet> it( mSheetList );
  for ( ; it.current(); ++it )
    if ( (*it)->modified() ) {
      if ( !mAutoSave || (*it)->fileName().isEmpty() ) {
        int res = KMessageBox::warningYesNoCancel( this,
                  i18n( "The worksheet '%1' contains unsaved data.\n"
                        "Do you want to save the worksheet?")
                  .arg( tabLabel( *it ) ), TQString::null, KStdGuiItem::save(), KStdGuiItem::discard() );
        if ( res == KMessageBox::Yes )
          saveWorkSheet( *it );
        else if ( res == KMessageBox::Cancel )
          return false; // abort quit
      } else
        saveWorkSheet(*it);
    }

  return true;
}

void Workspace::loadWorkSheet()
{
  KFileDialog dlg( 0, i18n( "*.sgrd|Sensor Files" ), this,
                   "LoadFileDialog", true );

  KURL url = dlg.getOpenURL( mWorkDir, "*.sgrd", 0, i18n( "Select Worksheet to Load" ) );

  loadWorkSheet( url );
}

void Workspace::loadWorkSheet( const KURL &url )
{
  if ( url.isEmpty() )
    return;

  /* It's probably not worth the effort to make this really network
   * transparent. Unless s/o beats me up I use this pseudo transparent
   * code. */
  TQString tmpFile;
  TDEIO::NetAccess::download( url, tmpFile, this );
  mWorkDir = tmpFile.left( tmpFile.findRev( '/' ) );

  // Load sheet from file.
  if ( !restoreWorkSheet( tmpFile ) )
    return;

  /* If we have loaded a non-local file we clear the file name so that
   * the users is prompted for a new name for saving the file. */
  KURL tmpFileUrl;
  tmpFileUrl.setPath( tmpFile );
  if ( tmpFileUrl != url.url() )
    mSheetList.last()->setFileName( TQString::null );
  TDEIO::NetAccess::removeTempFile( tmpFile );

  emit announceRecentURL( KURL( url ) );
}

void Workspace::saveWorkSheet()
{
  saveWorkSheet( (WorkSheet*)currentPage() );
}

void Workspace::saveWorkSheetAs()
{
  saveWorkSheetAs( (WorkSheet*)currentPage() );
}

void Workspace::saveWorkSheet( WorkSheet *sheet )
{
  if ( !sheet ) {
    KMessageBox::sorry( this, i18n( "You do not have a worksheet that could be saved." ) );
    return;
  }

  TQString fileName = sheet->fileName();
  if ( fileName.isEmpty() ) {
    KFileDialog dlg( 0, i18n( "*.sgrd|Sensor Files" ), this,
                     "LoadFileDialog", true );
    fileName = dlg.getSaveFileName( mWorkDir + "/" + tabLabel( sheet ) +
                                    ".sgrd", "*.sgrd", 0,
                                    i18n( "Save Current Worksheet As" ) );
    if ( fileName.isEmpty() )
      return;

    mWorkDir = fileName.left( fileName.findRev( '/' ) );

    // extract filename without path
    TQString baseName = fileName.right( fileName.length() - fileName.findRev( '/' ) - 1 );

    // chop off extension (usually '.sgrd')
    baseName = baseName.left( baseName.findRev( '.' ) );
    changeTab( sheet, baseName );
  }

  /* If we cannot save the file is probably write protected. So we need
   * to ask the user for a new name. */
  if ( !sheet->save( fileName ) ) {
    saveWorkSheetAs( sheet );
    return;
  }

  /* Add file to recent documents menue. */
  KURL url;
  url.setPath( fileName );
  emit announceRecentURL( url );
}

void Workspace::saveWorkSheetAs( WorkSheet *sheet )
{
  if ( !sheet ) {
    KMessageBox::sorry( this, i18n( "You do not have a worksheet that could be saved." ) );
    return;
  }

  TQString fileName;
  do {
    KFileDialog dlg( 0, "*.sgrd", this, "LoadFileDialog", true );
    fileName = dlg.getSaveFileName( mWorkDir + "/" + tabLabel( currentPage() ) +
                                    ".sgrd", "*.sgrd" );
    if ( fileName.isEmpty() )
      return;

    mWorkDir = fileName.left( fileName.findRev( '/' ) );

    // extract filename without path
    TQString baseName = fileName.right( fileName.length() - fileName.findRev( '/' ) - 1 );

    // chop off extension (usually '.sgrd')
    baseName = baseName.left( baseName.findRev( '.' ) );
    changeTab( sheet, baseName );
  } while ( !sheet->save( fileName ) );

  /* Add file to recent documents menue. */
  KURL url;
  url.setPath( fileName );
  emit announceRecentURL( url );
}

void Workspace::deleteWorkSheet()
{
  WorkSheet *current = (WorkSheet*)currentPage();

  if ( current ) {
    if ( current->modified() ) {
      if ( !mAutoSave || current->fileName().isEmpty() ) {
        int res = KMessageBox::warningYesNoCancel( this,
                            i18n( "The worksheet '%1' contains unsaved data.\n"
                                  "Do you want to save the worksheet?" )
                            .arg( tabLabel( current ) ), TQString::null, KStdGuiItem::save(), KStdGuiItem::discard() );
        if ( res == KMessageBox::Cancel )
          return;

        if ( res == KMessageBox::Yes )
          saveWorkSheet( current );
      } else
        saveWorkSheet( current );
    }

    removePage( current );
    mSheetList.remove( current );
  } else {
    TQString msg = i18n( "There are no worksheets that could be deleted." );
    KMessageBox::error( this, msg );
  }
}

void Workspace::removeAllWorkSheets()
{
  WorkSheet *sheet;
  while ( ( sheet = (WorkSheet*)currentPage() ) != 0 ) {
    removePage( sheet );
    mSheetList.remove( sheet );
  }
}

void Workspace::deleteWorkSheet( const TQString &fileName )
{
  TQPtrListIterator<WorkSheet> it( mSheetList );
  for ( ; it.current(); ++it )
    if ( (*it)->fileName() == fileName ) {
      removePage( *it );
      mSheetList.remove( *it );
      return;
    }
}

WorkSheet *Workspace::restoreWorkSheet( const TQString &fileName, const TQString &newName )
{
  /* We might want to save the worksheet under a different name later. This
   * name can be specified by newName. If newName is empty we use the
   * original name to save the work sheet. */
  TQString tmpStr;
  if ( newName.isEmpty() )
    tmpStr = fileName;
  else
    tmpStr = newName;

  // extract filename without path
  TQString baseName = tmpStr.right( tmpStr.length() - tmpStr.findRev( '/' ) - 1 );

  // chop off extension (usually '.sgrd')
  baseName = baseName.left( baseName.findRev( '.' ) );

  WorkSheet *sheet = new WorkSheet( this );
  sheet->setTitle( baseName );
  insertTab( sheet, baseName );
  showPage( sheet );

  if ( !sheet->load( fileName ) ) {
    delete sheet;
    return NULL;
  }
  
  mSheetList.append( sheet );
  connect( sheet, TQT_SIGNAL( sheetModified( TQWidget* ) ),
           TQT_SLOT( updateCaption( TQWidget* ) ) );

  /* Force the file name to be the new name. This also sets the modified
   * flag, so that the file will get saved on exit. */
  if ( !newName.isEmpty() )
    sheet->setFileName( newName );

  return sheet;
}

void Workspace::cut()
{
  WorkSheet *current = (WorkSheet*)currentPage();

  if ( current )
    current->cut();
}

void Workspace::copy()
{
  WorkSheet *current = (WorkSheet*)currentPage();

  if ( current )
    current->copy();
}

void Workspace::paste()
{
  WorkSheet *current = (WorkSheet*)currentPage();

  if ( current )
    current->paste();
}

void Workspace::configure()
{
  WorkSheet *current = (WorkSheet*)currentPage();

  if ( !current )
    return;

  current->settings();
}

void Workspace::updateCaption( TQWidget* wdg )
{
  if ( wdg )
    emit setCaption( tabLabel( wdg ), ((WorkSheet*)wdg)->modified() );
  else
    emit setCaption( TQString::null, false );

  for ( WorkSheet* s = mSheetList.first(); s != 0; s = mSheetList.next() )
    ((WorkSheet*)s)->setIsOnTop( s == wdg );
}

void Workspace::updateSheetTitle( TQWidget* wdg )
{
  if ( wdg )
    changeTab( wdg, static_cast<WorkSheet*>( wdg )->title() );
}

void Workspace::applyStyle()
{
  if ( currentPage() )
    ((WorkSheet*)currentPage())->applyStyle();
}

void Workspace::showProcesses()
{
  TDEStandardDirs* kstd = TDEGlobal::dirs();
  kstd->addResourceType( "data", "share/apps/ksysguard" );

  TQString file = kstd->findResource( "data", "ProcessTable.sgrd" );
  if ( file.isEmpty() ) {
    KMessageBox::error( this, i18n( "Cannot find file ProcessTable.sgrd." ) );
    return;
  }
  WorkSheet *processSheet = restoreWorkSheet( file );
  if(!processSheet) return;

  //Set the focus of the search line.  This is nasty I know, but I don't know how better to do this :(
  KSGRD::SensorDisplay *processSensor = processSheet->display( 0,0 );
  if(!processSensor || !processSensor->isA("ProcessController")) return;
  ProcessController *controller = dynamic_cast<ProcessController *>(processSensor);
  if(!controller) return;
  controller->setSearchFocus();

}

#include "Workspace.moc"
