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

*/

#include <stdlib.h>

#include <kdebug.h>
#include <tdelocale.h>
#include <kpassdlg.h> 

#include "SensorClient.h"
#include "SensorManager.h"

#include "SensorSocketAgent.h"

using namespace KSGRD;

SensorSocketAgent::SensorSocketAgent( SensorManager *sm )
  : SensorAgent( sm )
{
  connect( &mSocket, TQT_SIGNAL( gotError( int ) ), TQT_SLOT( error( int ) ) );
  connect( &mSocket, TQT_SIGNAL( bytesWritten( int ) ), TQT_SLOT( msgSent( int ) ) );
  connect( &mSocket, TQT_SIGNAL( readyRead() ), TQT_SLOT( msgRcvd() ) );
  connect( &mSocket, TQT_SIGNAL( closed() ), TQT_SLOT( connectionClosed() ) );
}

SensorSocketAgent::~SensorSocketAgent()
{
  mSocket.writeBlock( "quit\n", strlen( "quit\n" ) );
  mSocket.flush();
}
	
bool SensorSocketAgent::start( const TQString &host, const TQString&,
                               const TQString&, int port )
{
  if ( port <= 0 )
    kdDebug(1215) << "SensorSocketAgent::start: Illegal port " << port << endl;

  setHostName( host );
  mPort = port;

  mSocket.connect( hostName(), TQString::number(mPort) );

  return true;
}

void SensorSocketAgent::hostInfo( TQString &shell, TQString &command, int &port ) const
{
  shell = TQString::null;
  command = TQString::null;
  port = mPort;
}

void SensorSocketAgent::msgSent( int )
{
  if ( mSocket.bytesToWrite() != 0 )
    return;

  setTransmitting( false );

  // Try to send next request if available.
  executeCommand();
}

void SensorSocketAgent::msgRcvd()
{
  int buflen = mSocket.bytesAvailable();
  char* buffer = new char[ buflen ];

  mSocket.readBlock( buffer, buflen );
  TQString buf = TQString::fromLocal8Bit( buffer, buflen );
  delete [] buffer;

  processAnswer( buf );
}

void SensorSocketAgent::connectionClosed()
{
  setDaemonOnLine( false );
  sensorManager()->hostLost( this );
  sensorManager()->requestDisengage( this );
}

void SensorSocketAgent::error( int id )
{
  switch ( id ) {
    case KNetwork::TDESocketBase::ConnectionRefused:
      SensorMgr->notify( i18n( "Connection to %1 refused" )
                         .arg( hostName() ) );
      break;
    case  KNetwork::TDESocketBase::LookupFailure:
      SensorMgr->notify( i18n( "Host %1 not found" )
                         .arg( hostName() ) );
      break;
    case  KNetwork::TDESocketBase::Timeout:
      SensorMgr->notify( i18n( "Timeout at host %1")
                         .arg( hostName() ) );
      break;
    case  KNetwork::TDESocketBase::NetFailure:
      SensorMgr->notify( i18n( "Network failure host %1")
                         .arg( hostName() ) );
      break;
    default:
      kdDebug(1215) << "SensorSocketAgent::error() unknown error " << id << endl;
  }

  setDaemonOnLine( false );
  sensorManager()->requestDisengage( this );
}

bool SensorSocketAgent::writeMsg( const char *msg, int len )
{
  return ( mSocket.writeBlock( msg, len ) == len );
}

bool SensorSocketAgent::txReady()
{
  return !transmitting();
}

#include "SensorSocketAgent.moc"
