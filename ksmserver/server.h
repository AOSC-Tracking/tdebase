/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#ifndef SERVER_H
#define SERVER_H

#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqsocketnotifier.h>
#include <tqptrlist.h>
#include <tqvaluelist.h>
#include <tqcstring.h>
#include <tqdict.h>
#include <tqptrqueue.h>
#include <tqptrdict.h>
#include <tdeapplication.h>
#include <tqtimer.h>
#include <dcopobject.h>

#ifdef __TDE_HAVE_TDEHWLIB
#include <tdehardwaredevices.h>
#endif

#include "server2.h"

#include "KSMServerInterface.h"

#define SESSION_PREVIOUS_LOGOUT "saved at previous logout"
#define SESSION_BY_USER  "saved by user"

typedef TQValueList<TQCString> QCStringList;
class KSMListener;
class KSMConnection;
class KSMClient;

enum SMType { SM_ERROR, SM_WMCOMMAND, SM_WMSAVEYOURSELF };
struct SMData
    {
    SMType type;
    TQStringList wmCommand;
    TQString wmClientMachine;
    TQString wmclass1, wmclass2;
    };
typedef TQMap<WId,SMData> WindowMap;

class KSMServer : public TQObject, public KSMServerInterface
{
Q_OBJECT
K_DCOP
k_dcop:
    void notifySlot(TQString,TQString,TQString,TQString,TQString,int,int,int,int);
    void logoutSoundFinished(int,int);
    void autoStart0Done();
    void autoStart1Done();
    void autoStart2Done();
    void kcmPhase1Done();
    void kcmPhase2Done();
    bool startupCompleted();
public:
    KSMServer( const TQString& windowManager, const TQString& windowManagerAddArgs, bool only_local );
    ~KSMServer();

    static KSMServer* self();

    void* watchConnection( IceConn iceConn );
    void removeConnection( KSMConnection* conn );

    KSMClient* newClient( SmsConn );
    void  deleteClient( KSMClient* client );

    // callbacks
    void saveYourselfDone( KSMClient* client, bool success );
    void interactRequest( KSMClient* client, int dialogType );
    void interactDone( KSMClient* client, bool cancelShutdown );
    void phase2Request( KSMClient* client );

    // error handling
    void ioError( IceConn iceConn );

    // notification
    void clientSetProgram( KSMClient* client );
    void clientRegistered( const char* previousId );

    // public API
    void restoreSession( TQString sessionName );
    void startDefaultSession();

    void shutdown( TDEApplication::ShutdownConfirm confirm,
                   TDEApplication::ShutdownType sdtype,
                   TDEApplication::ShutdownMode sdmode );

    virtual void suspendStartup( TQCString app );
    virtual void resumeStartup( TQCString app );

    bool checkStatus( bool &logoutConfirmed, bool &maysd, bool &mayrb,
		      TDEApplication::ShutdownConfirm confirm,
		      TDEApplication::ShutdownType sdtype,
		      TDEApplication::ShutdownMode sdmode );

public slots:
    void cleanUp();

private slots:
    void newConnection( int socket );
    void processData( int socket );
    void restoreSessionInternal();
    void restoreSessionDoneInternal();

    void notificationTimeout();
    void protectionTimerTick();
    void protectionTimeout();
    void timeoutQuit();
    void timeoutWMQuit();
    void knotifyTimeout();
    void kcmPhase1Timeout();
    void kcmPhase2Timeout();
    void pendingShutdownTimeout();

    void autoStart0();
    void autoStart1();
    void autoStart2();
    void tryRestoreNext();
    void startupSuspendTimeout();

    void cancelShutdown();
    void forceSkipSaveYourself();

private:
    void handlePendingInteractions();
    void completeShutdownOrCheckpoint();
    void startKilling();
    void performStandardKilling();
    void completeKilling();
    void killWM();
    void completeKillingWM();
    void cancelShutdown( TQString cancellationText );
    void cancelShutdown( KSMClient* c );
    void killingCompleted();

    void discardSession();
    void storeSession();

    void startProtection();
    void endProtection();
    void handleProtectionTimeout();
    void updateLogoutStatusDialog();

    void startApplication( TQStringList command,
        const TQString& clientMachine = TQString::null,
        const TQString& userId = TQString::null );
    void executeCommand( const TQStringList& command );

    bool isWM( const KSMClient* client ) const;
    bool isWM( const TQString& program ) const;
    bool isCM( const KSMClient* client ) const;
    bool isCM( const TQString& program ) const;
    bool isDesktop( const KSMClient* client ) const;
    bool isDesktop( const TQString& program ) const;
    bool isNotifier( const KSMClient* client ) const;
    bool isNotifier( const TQString& program ) const;
    bool isCrashHandler( const KSMClient* client ) const;
    bool isCrashHandler( const TQString& program ) const;
    bool defaultSession() const; // empty session
    void setupXIOErrorHandler();

    void shutdownInternal( TDEApplication::ShutdownConfirm confirm,
			   TDEApplication::ShutdownType sdtype,
			   TDEApplication::ShutdownMode sdmode,
			   TQString bootOption = TQString::null );

    void performLegacySessionSave();
    void storeLegacySession( TDEConfig* config );
    void restoreLegacySession( TDEConfig* config );
    void restoreLegacySessionInternal( TDEConfig* config, char sep = ',' );
    TQStringList windowWmCommand(WId w);
    TQString windowWmClientMachine(WId w);
    WId windowWmClientLeader(WId w);
    TQCString windowSessionId(WId w, WId leader);

    bool checkStartupSuspend();
    void finishStartup();
    void resumeStartupInternal();

    // public dcop interface
    void logout( int, int, int );
    virtual void logoutTimed( int, int, TQString );
    TQStringList sessionList();
    TQString currentSession();
    void saveCurrentSession();
    void saveCurrentSessionAs( TQString );

    TQWidget* startupNotifierIPDlg;
    TQWidget* shutdownNotifierIPDlg;

 private:
    TQPtrList<KSMListener> listener;
    TQPtrList<KSMClient> clients;

    enum State
        {
        Idle,
        LaunchingWM, AutoStart0, KcmInitPhase1, AutoStart1, Restoring, FinishingStartup, // startup
        Shutdown, Checkpoint, Killing, KillingWM, WaitingForKNotify // shutdown
        };
    State state;
    bool dialogActive;
    bool saveSession;
    int wmPhase1WaitingCount;
    int saveType;
    TQMap< TQCString, int > startupSuspendCount;

    TDEApplication::ShutdownType shutdownType;
    TDEApplication::ShutdownMode shutdownMode;
    TQString bootOption;

    bool clean;
    KSMClient* clientInteracting;
    TQString wm;
    TQString wmAddArgs;
    TQString sessionGroup;
    TQString sessionName;
    TQCString launcher;
    TQTimer protectionTimer;
    TQTimer notificationTimer;
    TQTimer restoreTimer;
    TQTimer shutdownTimer;
    TQString xonCommand;
    int logoutSoundEvent;
    TQTimer knotifyTimeoutTimer;
    TQTimer startupSuspendTimeoutTimer;
    bool waitAutoStart2;
    bool waitKcmInit2;
    bool m_startupCompleted;
    TQTimer pendingShutdown;
    TDEApplication::ShutdownConfirm pendingShutdown_confirm;
    TDEApplication::ShutdownType pendingShutdown_sdtype;
    TDEApplication::ShutdownMode pendingShutdown_sdmode;

    // ksplash interface
    void upAndRunning( const TQString& msg );
    void publishProgress( int progress, bool max  = false  );

    // sequential startup
    int appsToStart;
    int lastAppStarted;
    TQString lastIdStarted;

    TQStringList excludeApps;

    WindowMap legacyWindows;

#ifdef __TDE_HAVE_TDEHWLIB
    TDEHardwareDevices* hwDevices;
#endif
    int initialClientCount;
    int phase2ClientCount;
    int protectionTimerCounter;
};

#endif
