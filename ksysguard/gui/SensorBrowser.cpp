/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>

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

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>. Please do
    not commit any changes without consulting me first. Thanks!

*/

#include <tqdragobject.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <ksgrd/SensorManager.h>

#include "SensorBrowser.h"

class HostItem : public TQListViewItem
{
  public:
    HostItem( SensorBrowser *parent, const TQString &text, int id,
              KSGRD::SensorAgent *host)
      : TQListViewItem( parent, text ), mInited( false ), mId( id ),
        mHost( host ), mParent( parent )
    {
      setExpandable( true );
    }

    void setOpen( bool open )
    {
      if ( open && !mInited ) {
        mInited = true;
        // request sensor list from host
        mHost->sendRequest( "monitors", mParent, mId );
      }

      TQListViewItem::setOpen( open );
    }

  private:
    bool mInited;
    int mId;
    KSGRD::SensorAgent* mHost;
    SensorBrowser* mParent;
};

SensorBrowser::SensorBrowser( TQWidget* parent, KSGRD::SensorManager* sm,
                              const char* name)
  : TDEListView( parent, name ), mSensorManager( sm )
{
  mHostInfoList.setAutoDelete(true);

  connect( mSensorManager, TQT_SIGNAL( update() ), TQT_SLOT( update() ) );
  connect( this, TQT_SIGNAL( clicked( TQListViewItem* ) ),
           TQT_SLOT( newItemSelected( TQListViewItem* ) ) );
  connect( this, TQT_SIGNAL( returnPressed( TQListViewItem* ) ),
           TQT_SLOT( newItemSelected( TQListViewItem* ) ) );

  addColumn( i18n( "Sensor Browser" ) );
  addColumn( i18n( "Sensor Type" ) );
  setFullWidth( true );

  TQToolTip::add( this, i18n( "Drag sensors to empty cells of a worksheet "
                             "or the panel applet." ) );
  setRootIsDecorated( true );

  // The sensor browser can be completely hidden.
  setMinimumWidth( 1 );

  TQWhatsThis::add( this, i18n( "The sensor browser lists the connected hosts and the sensors "
                               "that they provide. Click and drag sensors into drop zones "
                               "of a worksheet or the panel applet. A display will appear "
                               "that visualizes the "
                               "values provided by the sensor. Some sensor displays can "
                               "display values of multiple sensors. Simply drag other "
                               "sensors on to the display to add more sensors." ) );
}

SensorBrowser::~SensorBrowser()
{
}

void SensorBrowser::disconnect()
{
  TQPtrListIterator<HostInfo> it( mHostInfoList );

  for ( ; it.current(); ++it )
    if ( (*it)->listViewItem()->isSelected() ) {
      kdDebug(1215) << "Disconnecting " <<  (*it)->hostName() << endl;
      KSGRD::SensorMgr->requestDisengage( (*it)->sensorAgent() );
    }
}

void SensorBrowser::hostReconfigured( const TQString& )
{
  // TODO: not yet implemented.
}

void SensorBrowser::update()
{
  static int id = 0;

  KSGRD::SensorManagerIterator it( mSensorManager );

  mHostInfoList.clear();
  clear();

  KSGRD::SensorAgent* host;
  for ( int i = 0 ; ( host = it.current() ); ++it, ++i ) {
    TQString hostName = mSensorManager->hostName( host );
    HostItem* lvi = new HostItem( this, hostName, id, host );

    TQPixmap pix = TDEGlobal::iconLoader()->loadIcon( "computer", TDEIcon::Desktop, TDEIcon::SizeSmall );
    lvi->setPixmap( 0, pix );

    HostInfo* hostInfo = new HostInfo( id, host, hostName, lvi );
    mHostInfoList.append( hostInfo );
    ++id;
  }

  setMouseTracking( false );
}

void SensorBrowser::newItemSelected( TQListViewItem *item )
{
  static bool showAnnoyingPopup = true;
  if ( item && item->pixmap( 0 ) && showAnnoyingPopup)
  {
    showAnnoyingPopup = false;
    KMessageBox::information( this, i18n( "Drag sensors to empty fields in a worksheet." ),
                              TQString::null, "ShowSBUseInfo" );
  }
}

void SensorBrowser::answerReceived( int id, const TQString &answer )
{
  /* An answer has the following format:

     cpu/idle integer
     cpu/sys  integer
     cpu/nice integer
     cpu/user integer
     ps       table
  */

  TQPtrListIterator<HostInfo> it( mHostInfoList );

  /* Check if id is still valid. It can get obsolete by rapid calls
   * of update() or when the sensor died. */
  for ( ; it.current(); ++it )
    if ( (*it)->id() == id )
      break;

  if ( !it.current() )
    return;

  KSGRD::SensorTokenizer lines( answer, '\n' );

  for ( uint i = 0; i < lines.count(); ++i ) {
    if ( lines[ i ].isEmpty() )
      break;

    KSGRD::SensorTokenizer words( lines[ i ], '\t' );
    TQString sensorName = words[ 0 ];
    TQString sensorType = words[ 1 ];

    /* Calling update() a rapid sequence will create pending
     * requests that do not get erased by calling
     * clear(). Subsequent updates will receive the old pending
     * answers so we need to make sure that we register each
     * sensor only once. */
    if ( (*it)->isRegistered( sensorName ) )
      return;

    /* The sensor browser can display sensors in a hierachical order.
     * Sensors can be grouped into nodes by seperating the hierachical
     * nodes through slashes in the sensor name. E. g. cpu/user is
     * the sensor user in the cpu node. There is no limit for the
     * depth of nodes. */
    KSGRD::SensorTokenizer absolutePath( sensorName, '/' );

    TQListViewItem* parent = (*it)->listViewItem();
    for ( uint j = 0; j < absolutePath.count(); ++j ) {
      // Localize the sensor name part by part.
      TQString name = KSGRD::SensorMgr->translateSensorPath( absolutePath[ j ] );

      bool found = false;
      TQListViewItem* sibling = parent->firstChild();
      while ( sibling && !found ) {
        if (sibling->text( 0 ) == name ) {
          // The node or sensor is already known.
          found = true;
        } else
          sibling = sibling->nextSibling();
      }
      if ( !found ) {
        TQListViewItem* lvi = new TQListViewItem( parent, name );
        if ( j == absolutePath.count() - 1 ) {
          TQPixmap pix = TDEGlobal::iconLoader()->loadIcon( "ksysguardd", TDEIcon::Desktop,
                                               TDEIcon::SizeSmall );
          lvi->setPixmap( 0, pix );
          lvi->setText( 1, KSGRD::SensorMgr->translateSensorType( sensorType ) );

          // add sensor info to internal data structure
          (*it)->addSensor( lvi, sensorName, name, sensorType );
        } else
          parent = lvi;

        // The child indicator might need to be updated.
        repaintItem( parent );
      } else
        parent = sibling;
    }
  }

  repaintItem( (*it)->listViewItem() );
}

void SensorBrowser::viewportMouseMoveEvent( TQMouseEvent *e )
{
  /* setMouseTracking(false) seems to be broken. With current Qt
   * mouse tracking cannot be turned off. So we have to check each event
   * whether the LMB is really pressed. */

  if ( !( e->state() & Qt::LeftButton ) )
    return;

  TQListViewItem* item = itemAt( e->pos() );
  if ( !item ) // no item under cursor
    return;

  // Make sure that a sensor and not a node or hostname has been picked.
  TQPtrListIterator<HostInfo> it( mHostInfoList );
  for ( ; it.current() && !(*it)->isRegistered( item ); ++it );
  if ( !it.current() )
    return;

  // Create text drag object as
  // "<hostname> <sensorname> <sensortype> <sensordescription>".
  // Only the description may contain blanks.
  mDragText = (*it)->hostName() + " " +
              (*it)->sensorName( item ) + " " +
              (*it)->sensorType( item ) + " " +
              (*it)->sensorDescription( item );

  TQDragObject* dragObject = new TQTextDrag( mDragText, this );
  dragObject->dragCopy();
}

TQStringList SensorBrowser::listHosts()
{
  TQStringList hostList;

  TQPtrListIterator<HostInfo> it( mHostInfoList );
  for ( ; it.current(); ++it )
    hostList.append( (*it)->hostName() );

  return hostList;
}

TQStringList SensorBrowser::listSensors( const TQString &hostName )
{
  TQPtrListIterator<HostInfo> it( mHostInfoList );
  for ( ; it.current(); ++it ) {
    if ( (*it)->hostName() == hostName ) {
      return (*it)->allSensorNames();
    }
  }

  return TQStringList();
}

/**
 Helper classes
 */
SensorInfo::SensorInfo( TQListViewItem *lvi, const TQString &name,
                        const TQString &desc, const TQString &type )
  : mLvi( lvi ), mName( name ), mDesc( desc ), mType( type )
{
}

TQListViewItem* SensorInfo::listViewItem() const
{
  return mLvi;
}

const TQString& SensorInfo::name() const
{
  return mName;
}

const TQString& SensorInfo::type() const
{
  return mType;
}

const TQString& SensorInfo::description() const
{
  return mDesc;
}

HostInfo::HostInfo( int id, const KSGRD::SensorAgent *agent,
                    const TQString &name, TQListViewItem *lvi )
  : mId( id ), mSensorAgent( agent ), mHostName( name ), mLvi( lvi )
{
  mSensorList.setAutoDelete( true );
}

int HostInfo::id() const
{
  return ( mId );
}

const KSGRD::SensorAgent* HostInfo::sensorAgent() const
{
  return mSensorAgent;
}

const TQString& HostInfo::hostName() const
{
  return mHostName;
}

TQListViewItem* HostInfo::listViewItem() const
{
  return mLvi;
}

const TQString& HostInfo::sensorName( const TQListViewItem *lvi ) const
{
  TQPtrListIterator<SensorInfo> it( mSensorList );
  for ( ; it.current() && (*it)->listViewItem() != lvi; ++it );

  Q_ASSERT( it.current() );
  return ( (*it)->name() );
}

TQStringList HostInfo::allSensorNames() const
{
  TQStringList list;

  TQPtrListIterator<SensorInfo> it( mSensorList );
  for ( ; it.current(); ++it )
    list.append( it.current()->name() );

  return list;
}

const TQString& HostInfo::sensorType( const TQListViewItem *lvi ) const
{
  TQPtrListIterator<SensorInfo> it( mSensorList );
  for ( ; it.current() && (*it)->listViewItem() != lvi; ++it );

  Q_ASSERT( it.current() );
  return ( (*it)->type() );
}

const TQString& HostInfo::sensorDescription( const TQListViewItem *lvi ) const
{
  TQPtrListIterator<SensorInfo> it( mSensorList );
  for ( ; it.current() && (*it)->listViewItem() != lvi; ++it );

  Q_ASSERT( it.current() );
  return ( (*it)->description() );
}

void HostInfo::addSensor( TQListViewItem *lvi, const TQString& name,
                          const TQString& desc, const TQString& type )
{
  SensorInfo* info = new SensorInfo( lvi, name, desc, type );
  mSensorList.append( info );
}

bool HostInfo::isRegistered( const TQString& name ) const
{
  TQPtrListIterator<SensorInfo> it( mSensorList );
  for ( ; it.current(); ++it )
    if ( (*it)->name() == name )
      return true;

  return false;
}

bool HostInfo::isRegistered( TQListViewItem *lvi ) const
{
  TQPtrListIterator<SensorInfo> it( mSensorList );
  for ( ; it.current(); ++it )
    if ( (*it)->listViewItem() == lvi )
      return true;

  return false;
}

#include "SensorBrowser.moc"
