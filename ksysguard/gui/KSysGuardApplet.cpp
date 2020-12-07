/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999 - 2001 Chris Schlaeger <cs@kde.org>

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

    KSysGuard is currently maintained by Chris Schlaeger
    <cs@kde.org>. Please do not commit any changes without consulting
    me first. Thanks!

*/

#include <tqcursor.h>
#include <tqdom.h>
#include <tqdragobject.h>
#include <tqfile.h>
#include <tqpushbutton.h>
#include <tqspinbox.h>
#include <tqtooltip.h>

#include <kdebug.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <tdepopupmenu.h>

#include <ksgrd/SensorClient.h>
#include <ksgrd/SensorManager.h>
#include <ksgrd/StyleEngine.h>

#include "DancingBars.h"
#include "FancyPlotter.h"
#include "KSGAppletSettings.h"
#include "MultiMeter.h"

#include "KSysGuardApplet.h"

extern "C"
{
  KDE_EXPORT KPanelApplet* init( TQWidget *parent, const TQString& configFile )
  {
    TDEGlobal::locale()->insertCatalogue( "ksysguard" );
    return new KSysGuardApplet( configFile, KPanelApplet::Normal,
                                KPanelApplet::Preferences, parent,
                                "ksysguardapplet" );
  }
}

KSysGuardApplet::KSysGuardApplet( const TQString& configFile, Type type,
                                  int actions, TQWidget *parent,
                                  const char *name )
  : KPanelApplet( configFile, type, actions, parent, name)
{
  mSettingsDlg = 0;

  KSGRD::SensorMgr = new KSGRD::SensorManager();

  KSGRD::Style = new KSGRD::StyleEngine();

  mDockCount = 1;
  mDockList = new TQWidget*[ mDockCount ];

  mSizeRatio = 1.0;
  addEmptyDisplay( mDockList, 0 );

  updateInterval( 2 );

  load();

  setAcceptDrops( true );
}

KSysGuardApplet::~KSysGuardApplet()
{
  save();

  delete [] mDockList;
  mDockList = 0;

  delete mSettingsDlg;
  mSettingsDlg = 0;

	delete KSGRD::Style;
	delete KSGRD::SensorMgr;
	KSGRD::SensorMgr = 0;
}

int KSysGuardApplet::widthForHeight( int height ) const
{
  return ( (int) ( height * mSizeRatio + 0.5 ) * mDockCount );
}

int KSysGuardApplet::heightForWidth( int width ) const
{
  return ( (int) ( width * mSizeRatio + 0.5 ) * mDockCount );
}

void KSysGuardApplet::resizeEvent( TQResizeEvent* )
{
  layout();
}

void KSysGuardApplet::preferences()
{
  if(mSettingsDlg) {
    return;
  }
  mSettingsDlg = new KSGAppletSettings( this );

  connect( mSettingsDlg, TQT_SIGNAL( applyClicked() ), TQT_SLOT( applySettings() ) );
  connect( mSettingsDlg, TQT_SIGNAL( okClicked() ), TQT_SLOT( applySettings() ) );
  connect( mSettingsDlg, TQT_SIGNAL( finished() ), TQT_SLOT( preferencesFinished() ) );

  mSettingsDlg->setNumDisplay( mDockCount );
  mSettingsDlg->setSizeRatio( (int) ( mSizeRatio * 100.0 + 0.5 ) );
  mSettingsDlg->setUpdateInterval( updateInterval() );

  mSettingsDlg->show();
}
void KSysGuardApplet::preferencesFinished()
{
  mSettingsDlg->delayedDestruct();
  mSettingsDlg = 0;
}
void KSysGuardApplet::applySettings()
{
  updateInterval( mSettingsDlg->updateInterval() );
  mSizeRatio = mSettingsDlg->sizeRatio() / 100.0;
  resizeDocks( mSettingsDlg->numDisplay() );

  for ( uint i = 0; i < mDockCount; ++i )
    if ( !mDockList[ i ]->isA( TQFRAME_OBJECT_NAME_STRING ) )
      ((KSGRD::SensorDisplay*)mDockList[ i ])->setUpdateInterval( updateInterval() );

  save();
}

void KSysGuardApplet::sensorDisplayModified( bool modified )
{
  if ( modified )
    save();
}

void KSysGuardApplet::layout()
{
  if ( orientation() == Qt::Horizontal ) {
    int h = height();
    int w = (int) ( h * mSizeRatio + 0.5 );
    for ( uint i = 0; i < mDockCount; ++i )
      if ( mDockList[ i ] )
        mDockList[ i ]->setGeometry( i * w, 0, w, h );
  } else {
    int w = width();
    int h = (int) ( w * mSizeRatio + 0.5 );
    for ( uint i = 0; i < mDockCount; ++i )
      if ( mDockList[ i ] )
        mDockList[ i ]->setGeometry( 0, i * h, w, h );
  }
}

int KSysGuardApplet::findDock( const TQPoint& point )
{
  if ( orientation() == Qt::Horizontal )
    return ( point.x() / (int) ( height() * mSizeRatio + 0.5 ) );
  else
    return ( point.y() / (int) ( width() * mSizeRatio + 0.5 ) );
}

void KSysGuardApplet::dragEnterEvent( TQDragEnterEvent *e )
{
  e->accept( TQTextDrag::canDecode( e ) );
}

void KSysGuardApplet::dropEvent( TQDropEvent *e )
{
  TQString dragObject;

  if ( TQTextDrag::decode( e, dragObject ) ) {
    // The host name, sensor name and type are seperated by a ' '.
    TQStringList parts = TQStringList::split( ' ', dragObject );

    TQString hostName = parts[ 0 ];
    TQString sensorName = parts[ 1 ];
    TQString sensorType = parts[ 2 ];
    TQString sensorDescr = parts[ 3 ];

    if ( hostName.isEmpty() || sensorName.isEmpty() || sensorType.isEmpty() )
      return;

    int dock = findDock( e->pos() );
    if ( mDockList[ dock ]->isA( TQFRAME_OBJECT_NAME_STRING ) ) {
      if ( sensorType == "integer" || sensorType == "float" ) {
        TDEPopupMenu popup;
        TQWidget *wdg = 0;

        popup.insertTitle( i18n( "Select Display Type" ) );
        popup.insertItem( i18n( "&Signal Plotter" ), 1 );
        popup.insertItem( i18n( "&Multimeter" ), 2 );
        popup.insertItem( i18n( "&Dancing Bars" ), 3 );
        switch ( popup.exec( TQCursor::pos() ) ) {
          case 1:
            wdg = new FancyPlotter( this, "FancyPlotter", sensorDescr,
                                    100.0, 100.0, true );
						break;

          case 2:
            wdg = new MultiMeter( this, "MultiMeter", sensorDescr,
                                  100.0, 100.0, true );
            break;

          case 3:
            wdg = new DancingBars( this, "DancingBars", sensorDescr,
                                   100, 100, true );
            break;
				}

        if ( wdg ) {
          delete mDockList[ dock ];
          mDockList[ dock ] = wdg;
          layout();

          connect( wdg, TQT_SIGNAL( modified( bool ) ),
                   TQT_SLOT( sensorDisplayModified( bool ) ) );

          mDockList[ dock ]->show();
        }
      } else {
        KMessageBox::sorry( this,
          i18n( "The KSysGuard applet does not support displaying of "
                "this type of sensor. Please choose another sensor." ) );

        layout();
      }
    }

    if ( !mDockList[ dock ]->isA( TQFRAME_OBJECT_NAME_STRING ) )
      ((KSGRD::SensorDisplay*)mDockList[ dock ])->
                  addSensor( hostName, sensorName, sensorType, sensorDescr );
  }

  save();
}

void KSysGuardApplet::customEvent( TQCustomEvent *e )
{
  if ( e->type() == TQEvent::User ) {
    // SensorDisplays send out this event if they want to be removed.
    removeDisplay( (KSGRD::SensorDisplay*)e->data() );
    save();
  }
}

void KSysGuardApplet::removeDisplay( KSGRD::SensorDisplay *display )
{
  for ( uint i = 0; i < mDockCount; ++i )
    if ( display == mDockList[i] ) {
      delete mDockList[ i ];

      addEmptyDisplay( mDockList, i );
      return;
    }
}

void KSysGuardApplet::resizeDocks( uint newDockCount )
{
  /* This function alters the number of available docks. The number of
   * docks can be increased or decreased. We try to preserve existing
   * docks if possible. */

  if ( newDockCount == mDockCount ) {
    emit updateLayout();
    return;
  }

  // Create and initialize new dock array.
  TQWidget** tmp = new TQWidget*[ newDockCount ];

  uint i;
  for ( i = 0; ( i < newDockCount ) && ( i < mDockCount ); ++i )
    tmp[ i ] = mDockList[ i ];

  for ( i = newDockCount; i < mDockCount; ++i )
    if ( mDockList[ i ] )
      delete mDockList[ i ];

  for ( i = mDockCount; i < newDockCount; ++i )
    addEmptyDisplay( tmp, i );

  delete [] mDockList;

  mDockList = tmp;
  mDockCount = newDockCount;

  emit updateLayout();
}

bool KSysGuardApplet::load()
{
  TDEStandardDirs* kstd = TDEGlobal::dirs();
  kstd->addResourceType( "data", "share/apps/ksysguard" );
  TQString fileName = kstd->findResource( "data", "KSysGuardApplet.xml" );

  TQFile file( fileName );
  if ( !file.open( IO_ReadOnly ) ) {
    KMessageBox::sorry( this, i18n( "Cannot open the file %1." ).arg( fileName ) );
    return false;
  }

  // Parse the XML file.
  TQDomDocument doc;

  // Read in file and check for a valid XML header.
  if ( !doc.setContent( &file ) ) {
    KMessageBox::sorry( this, i18n( "The file %1 does not contain valid XML." )
                        .arg( fileName ) );
    return false;
  }

	// Check for proper document type.
  if ( doc.doctype().name() != "KSysGuardApplet" ) {
    KMessageBox::sorry( this, i18n( "The file %1 does not contain a valid applet "
                        "definition, which must have a document type 'KSysGuardApplet'." )
                        .arg( fileName ) );
    return false;
  }

  TQDomElement element = doc.documentElement();
  bool ok;
  uint count = element.attribute( "dockCnt" ).toUInt( &ok );
  if ( !ok )
		count = 1;

  mSizeRatio = element.attribute( "sizeRatio" ).toDouble( &ok );
  if ( !ok )
    mSizeRatio = 1.0;

  updateInterval( element.attribute( "interval" ).toUInt( &ok ) );
  if ( !ok )
    updateInterval( 2 );

  resizeDocks( count );

  /* Load lists of hosts that are needed for the work sheet and try
   * to establish a connection. */
  TQDomNodeList dnList = element.elementsByTagName( "host" );
  uint i;
  for ( i = 0; i < dnList.count(); ++i ) {
    TQDomElement element = dnList.item( i ).toElement();
    int port = element.attribute( "port" ).toInt( &ok );
    if ( !ok )
      port = -1;

    KSGRD::SensorMgr->engage( element.attribute( "name" ),
                              element.attribute( "shell" ),
                              element.attribute( "command" ), port );
  }
  //if no hosts are specified, at least connect to localhost
  if(dnList.count() == 0)
    KSGRD::SensorMgr->engage( "localhost", "", "ksysguardd", -1);

  // Load the displays and place them into the work sheet.
  dnList = element.elementsByTagName( "display" );
  for ( i = 0; i < dnList.count(); ++i ) {
    TQDomElement element = dnList.item( i ).toElement();
    uint dock = element.attribute( "dock" ).toUInt();
    if ( i >= mDockCount ) {
      kdDebug (1215) << "Dock number " << i << " out of range "
                     << mDockCount << endl;
      return false;
    }

    TQString classType = element.attribute( "class" );
    KSGRD::SensorDisplay* newDisplay;
    if ( classType == "FancyPlotter" )
      newDisplay = new FancyPlotter( this, "FancyPlotter", "Dummy", 100.0, 100.0, true /*no frame*/, true /*run ksysguard menu*/);
    else if ( classType == "MultiMeter" )
      newDisplay = new MultiMeter( this, "MultiMeter", "Dummy", 100.0, 100.0, true /*no frame*/, true /*run ksysguard menu*/ );
    else if ( classType == "DancingBars" )
      newDisplay = new DancingBars( this, "DancingBars", "Dummy", 100, 100, true /*no frame*/, true /*run ksysguard menu*/);
    else {
      KMessageBox::sorry( this, i18n( "The KSysGuard applet does not support displaying of "
                          "this type of sensor. Please choose another sensor." ) );
      return false;
    }

    newDisplay->setUpdateInterval( updateInterval() );

    // load display specific settings
    if ( !newDisplay->restoreSettings( element ) )
      return false;

    delete mDockList[ dock ];
    mDockList[ dock ] = newDisplay;

    connect( newDisplay, TQT_SIGNAL( modified( bool ) ),
             TQT_SLOT( sensorDisplayModified( bool ) ) );
  }

  return true;
}

bool KSysGuardApplet::save()
{
  // Parse the XML file.
  TQDomDocument doc( "KSysGuardApplet" );
  doc.appendChild( doc.createProcessingInstruction(
                   "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );

  // save work sheet information
  TQDomElement ws = doc.createElement( "WorkSheet" );
  doc.appendChild( ws );
  ws.setAttribute( "dockCnt", mDockCount );
  ws.setAttribute( "sizeRatio", mSizeRatio );
  ws.setAttribute( "interval", updateInterval() );

  TQStringList hosts;
  uint i;
  for ( i = 0; i < mDockCount; ++i )
    if ( !mDockList[ i ]->isA( TQFRAME_OBJECT_NAME_STRING ) )
      ((KSGRD::SensorDisplay*)mDockList[ i ])->hosts( hosts );

  // save host information (name, shell, etc.)
  TQStringList::Iterator it;
  for ( it = hosts.begin(); it != hosts.end(); ++it ) {
    TQString shell, command;
    int port;

    if ( KSGRD::SensorMgr->hostInfo( *it, shell, command, port ) ) {
      TQDomElement host = doc.createElement( "host" );
      ws.appendChild( host );
      host.setAttribute( "name", *it );
      host.setAttribute( "shell", shell );
      host.setAttribute( "command", command );
      host.setAttribute( "port", port );
    }
  }

  for ( i = 0; i < mDockCount; ++i )
    if ( !mDockList[ i ]->isA( TQFRAME_OBJECT_NAME_STRING ) ) {
      TQDomElement element = doc.createElement( "display" );
      ws.appendChild( element );
      element.setAttribute( "dock", i );
      element.setAttribute( "class", mDockList[ i ]->className() );

      ((KSGRD::SensorDisplay*)mDockList[ i ])->saveSettings( doc, element );
    }

  TDEStandardDirs* kstd = TDEGlobal::dirs();
  kstd->addResourceType( "data", "share/apps/ksysguard" );
  TQString fileName = kstd->saveLocation( "data", "ksysguard" );
  fileName += "/KSysGuardApplet.xml";

  KSaveFile file( fileName, 0644 );

  if ( file.status() == 0 )
  {
    file.textStream()->setEncoding( TQTextStream::UnicodeUTF8 );
    *(file.textStream()) << doc;
    file.close();
  }
  else
  {
    KMessageBox::sorry( this, i18n( "Cannot save file %1" ).arg( fileName ) );
    return false;
  }

  return true;
}

void KSysGuardApplet::addEmptyDisplay( TQWidget **dock, uint pos )
{
  dock[ pos ] = new TQFrame( this );
  ((TQFrame*)dock[ pos ])->setFrameStyle( TQFrame::WinPanel | TQFrame::Sunken );
  TQToolTip::add( dock[ pos ],
                 i18n( "Drag sensors from the TDE System Guard into this cell." ) );

  layout();
  if ( isVisible() )
    dock[ pos ]->show();
}

#include "KSysGuardApplet.moc"
