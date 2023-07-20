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

#ifndef KSG_SENSORBROWSER_H
#define KSG_SENSORBROWSER_H

#include <tqdict.h>

#include <tdelistview.h>
#include <ksgrd/SensorClient.h>

class TQMouseEvent;

namespace KSGRD {
class SensorManager;
class SensorAgent;
}

class SensorInfo;
class HostInfo;

/**
 * The SensorBrowser is the graphical front-end of the SensorManager. It
 * displays the currently available hosts and their sensors.
 */
class SensorBrowser : public TDEListView, public KSGRD::SensorClient
{
  TQ_OBJECT

  public:
    SensorBrowser( TQWidget* parent, KSGRD::SensorManager* sm, const char* name = 0 );
    ~SensorBrowser();

    TQStringList listHosts();
    TQStringList listSensors( const TQString &hostName );

  public slots:
    void disconnect();
    void hostReconfigured( const TQString &hostName );
    void update();
    void newItemSelected( TQListViewItem *item );

  protected:
    virtual void viewportMouseMoveEvent( TQMouseEvent* );

  private:
    void answerReceived( int id, const TQString& );

    KSGRD::SensorManager* mSensorManager;

    TQPtrList<HostInfo> mHostInfoList;
    TQString mDragText;

};

/**
 Helper classes
 */
class SensorInfo
{
  public:
    SensorInfo( TQListViewItem *lvi, const TQString &name, const TQString &desc,
                const TQString &type );
    ~SensorInfo() {}

    /**
      Returns a pointer to the list view item of the sensor.
     */
    TQListViewItem* listViewItem() const;

    /**
      Returns the name of the sensor.
     */
    const TQString& name() const;

    /**
      Returns the description of the sensor.
     */
    const TQString& description() const;

    /**
      Returns the type of the sensor.
     */
    const TQString& type() const;

  private:
    TQListViewItem* mLvi;
    TQString mName;
    TQString mDesc;
    TQString mType;
};

class HostInfo
{
  public:
    HostInfo( int id, const KSGRD::SensorAgent *agent, const TQString &name,
              TQListViewItem *lvi );
    ~HostInfo() { }

    /**
      Returns the unique id of the host.
     */
    int id() const;

    /**
      Returns a pointer to the sensor agent of the host.
     */
    const KSGRD::SensorAgent* sensorAgent() const;

    /**
      Returns the name of the host.
     */
    const TQString& hostName() const;

    /**
      Returns the a pointer to the list view item of the host.
     */
    TQListViewItem* listViewItem() const;

    /**
      Returns the sensor name of a special list view item.
     */
    const TQString& sensorName( const TQListViewItem *lvi ) const;

    /**
      Returns all sensor names of the host.
     */
    TQStringList allSensorNames() const;

    /**
      Returns the type of a special list view item.
     */
    const TQString& sensorType( const TQListViewItem *lvi ) const;

    /**
      Returns the description of a special list view item.
     */
    const TQString& sensorDescription( const TQListViewItem *lvi ) const;

    /**
      Adds a new Sensor to the host.
      
      @param lvi  The list view item.
      @param name The sensor name.
      @param desc A description.
      @param type The type of the sensor.
     */
    void addSensor( TQListViewItem *lvi, const TQString& name,
                    const TQString& desc, const TQString& type );

    /**
      Returns whether the sensor with @ref name
      is registered at the host.
     */
    bool isRegistered( const TQString& name ) const;

    /**
      Returns whether the sensor with @ref lvi
      is registered at the host.
     */
    bool isRegistered( TQListViewItem *lvi ) const;

  private:
    int mId;

    const KSGRD::SensorAgent* mSensorAgent;
    const TQString mHostName;
    TQListViewItem* mLvi;

    TQPtrList<SensorInfo> mSensorList;
};

#endif
