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

*/

#ifndef KSG_SENSORMANAGER_H
#define KSG_SENSORMANAGER_H

#include <tdeconfig.h>

#include <tqdict.h>
#include <tqobject.h>

#include <SensorAgent.h>

class HostConnector;

namespace KSGRD {

class SensorManagerIterator;

/**
  The SensorManager handles all interaction with the connected
  hosts. Connections to a specific hosts are handled by
  SensorAgents. Use engage() to establish a connection and
  disengage() to terminate the connection. If you don't know if a
  certain host is already connected use engageHost(). If there is no
  connection yet or the hostname is empty, a dialog will be shown to
  enter the connections details.
 */
class KDE_EXPORT SensorManager : public TQObject
{
  TQ_OBJECT

  friend class SensorManagerIterator;

  public:
    SensorManager();
    ~SensorManager();

    bool engageHost( const TQString &hostName );
    bool engage( const TQString &hostName, const TQString &shell = "ssh",
                 const TQString &command = "", int port = -1 );

    void requestDisengage( const SensorAgent *agent );
    bool disengage( const SensorAgent *agent );
    bool disengage( const TQString &hostName );
    bool resynchronize( const TQString &hostName );
    void hostLost( const SensorAgent *agent );
    void notify( const TQString &msg ) const;

    void setBroadcaster( TQWidget *wdg );

    virtual bool event( TQEvent *event );

    bool sendRequest( const TQString &hostName, const TQString &request,
                      SensorClient *client, int id = 0 );

    const TQString hostName( const SensorAgent *sensor ) const;
    bool hostInfo( const TQString &host, TQString &shell,
                   TQString &command, int &port );

    const TQString& translateUnit( const TQString &unit ) const;
    const TQString& translateSensorPath( const TQString &path ) const;
    const TQString& translateSensorType( const TQString &type ) const;
    TQString translateSensor(const TQString& u) const;

    void readProperties( TDEConfig *cfg );
    void saveProperties( TDEConfig *cfg );

    void disconnectClient( SensorClient *client );
	
  public slots:
    void reconfigure( const SensorAgent *agent );

  signals:
    void update();
    void hostConnectionLost( const TQString &hostName );

  protected:
    TQDict<SensorAgent> mAgents;

  private:
    /**
      These dictionary stores the localized versions of the sensor
      descriptions and units.
     */
    TQDict<TQString> mDescriptions;
    TQDict<TQString> mUnits;
    TQDict<TQString> mDict;
    TQDict<TQString> mTypes;

    TQWidget* mBroadcaster;

    HostConnector* mHostConnector;
};

KDE_EXPORT extern SensorManager* SensorMgr;

class KDE_EXPORT SensorManagerIterator : public TQDictIterator<SensorAgent>
{
  public:
    SensorManagerIterator( const SensorManager *sm )
      : TQDictIterator<SensorAgent>( sm->mAgents ) { }

    ~SensorManagerIterator() { }
};

}

#endif
