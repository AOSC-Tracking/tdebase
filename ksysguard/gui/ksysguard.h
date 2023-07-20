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

#ifndef KSG_KSYSGUARD_H
#define KSG_KSYSGUARD_H

#include <tqevent.h>

#include <dcopclient.h>
#include <dcopobject.h>
#include <tdeapplication.h>
#include <tdemainwindow.h>
#include <dnssd/servicebrowser.h>

#include <ksgrd/SensorClient.h>

class TDERecentFilesAction;
class TDEToggleAction;

class TQSplitter;
class SensorBrowser;
class Workspace;

class TopLevel : public TDEMainWindow, public KSGRD::SensorClient, public DCOPObject
{
  TQ_OBJECT
  K_DCOP

  public:
    TopLevel( const char *name = 0 );

  virtual void saveProperties( TDEConfig* );
  virtual void readProperties( TDEConfig* );

  virtual void answerReceived( int id, const TQString& );

  void beATaskManager();
  void showRequestedSheets();
  void initStatusBar();

  k_dcop:
    // calling ksysguard with twin/kicker hot-key
    ASYNC showProcesses();
    ASYNC showOnCurrentDesktop();
    ASYNC loadWorkSheet( const TQString &fileName );
    ASYNC removeWorkSheet( const TQString &fileName );
    TQStringList listHosts();
    TQStringList listSensors( const TQString &hostName );
    TQString readIntegerSensor( const TQString &sensorLocator );
    TQStringList readListSensor( const TQString &sensorLocator );

  public slots:
    void registerRecentURL( const KURL &url );
    void resetWorkSheets();

  protected:
    virtual void customEvent( TQCustomEvent* );
    virtual void timerEvent( TQTimerEvent* );
    virtual bool queryClose();

  protected slots:
    void connectHost();
    void disconnectHost();
    void updateStatusBar();
    void editToolbars();
    void editStyle();
    void slotNewToolbarConfig();
    void serviceAdded(DNSSD::RemoteService::Ptr srv);

  private:
    void setSwapInfo( long, long, const TQString& );

    TQPtrList<DCOPClientTransaction> mDCopFIFO;

    TQSplitter* mSplitter;
    TDERecentFilesAction* mActionOpenRecent;

    SensorBrowser* mSensorBrowser;
    Workspace* mWorkSpace;
    
    DNSSD::ServiceBrowser* mServiceBrowser;

    bool mDontSaveSession;
    int mTimerId;
};

extern TopLevel* Toplevel;

/*
   since there is only a forward declaration of DCOPClientTransaction
   in dcopclient.h we have to redefine it here, otherwise QPtrList
   causes errors
*/
typedef unsigned long CARD32;

class DCOPClientTransaction
{
  public:
    TQ_INT32 id;
    CARD32 key;
    TQCString senderId;
};

#endif
