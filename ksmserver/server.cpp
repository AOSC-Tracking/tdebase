/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2005 Lubos Lunak <l.lunak@kde.org>

relatively small extensions by Oswald Buddenhagen <ob6@inf.tu-dresden.de>

some code taken from the dcopserver (part of the KDE libraries), which is
Copyright (c) 1999 Matthias Ettrich <ettrich@kde.org>
Copyright (c) 1999 Preston Brown <pbrown@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <tqfile.h>
#include <tqtextstream.h>
#include <tqdatastream.h>
#include <tqptrstack.h>
#include <tqpushbutton.h>
#include <tqmessagebox.h>
#include <tqguardedptr.h>
#include <tqtimer.h>
#include <tqregexp.h>

#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <kstandarddirs.h>
#include <unistd.h>
#include <tdeapplication.h>
#include <kstaticdeleter.h>
#include <tdetempfile.h>
#include <kprocess.h>
#include <dcopclient.h>
#include <dcopref.h>

#include "server.h"
#include "global.h"
#include "client.h"

#include "server.moc"

#include <kdebug.h>

#include <dmctl.h>

KSMServer* the_server = 0;

KSMServer* KSMServer::self()
{
    return the_server;
}

/*! Utility function to execute a command on the local machine. Used
 * to restart applications.
 */
void KSMServer::startApplication( TQStringList command, const TQString& clientMachine,
    const TQString& userId )
{
    if ( command.isEmpty() )
        return;
    if ( !userId.isEmpty()) {
        struct passwd* pw = getpwuid( getuid());
        if( pw != NULL && userId != TQString::fromLocal8Bit( pw->pw_name )) {
            command.prepend( "--" );
            command.prepend( userId );
            command.prepend( "-u" );
            command.prepend( "tdesu" );
        }
    }
    if ( !clientMachine.isEmpty() && clientMachine != "localhost" ) {
        command.prepend( clientMachine );
	command.prepend( xonCommand ); // "xon" by default
    }
    int n = command.count();
    TQCString app = command[0].latin1();
    TQValueList<TQCString> argList;
    for ( int i=1; i < n; i++)
       argList.append( TQCString(command[i].latin1()));
    DCOPRef( launcher ).send( "exec_blind", app, DCOPArg( argList, "TQValueList<TQCString>" ) );
}

/*! Utility function to execute a command on the local machine. Used
 * to discard session data
 */
void KSMServer::executeCommand( const TQStringList& command )
{
    if ( command.isEmpty() )
        return;
    TDEProcess proc;
    for ( TQStringList::ConstIterator it = command.begin();
          it != command.end(); ++it )
        proc << (*it).latin1();
    proc.start( TDEProcess::Block );
}

IceAuthDataEntry *authDataEntries = 0;
static KTempFile *remAuthFile = 0;

static IceListenObj *listenObjs = 0;
int numTransports = 0;
static bool only_local = 0;

static Bool HostBasedAuthProc ( char* /*hostname*/)
{
    if (only_local)
        return true;
    else
        return false;
}


Status KSMRegisterClientProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    char *              previousId
)
{
    KSMClient* client = (KSMClient*) managerData;
    client->registerClient( previousId );
    return 1;
}

void KSMInteractRequestProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    int                 dialogType
)
{
    the_server->interactRequest( (KSMClient*) managerData, dialogType );
}

void KSMInteractDoneProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    Bool                        cancelShutdown
)
{
    the_server->interactDone( (KSMClient*) managerData, cancelShutdown );
}

void KSMSaveYourselfRequestProc (
    SmsConn             smsConn ,
    SmPointer           /* managerData */,
    int                 saveType,
    Bool                shutdown,
    int                 interactStyle,
    Bool                fast,
    Bool                global
)
{
    if ( shutdown ) {
        the_server->shutdown( fast ?
                              TDEApplication::ShutdownConfirmNo :
                              TDEApplication::ShutdownConfirmDefault,
                              TDEApplication::ShutdownTypeDefault,
                              TDEApplication::ShutdownModeDefault );
    } else if ( !global ) {
        SmsSaveYourself( smsConn, saveType, false, interactStyle, fast );
        SmsSaveComplete( smsConn );
    }
    // else checkpoint only, ksmserver does not yet support this
    // mode. Will come for KDE 3.1
}

void KSMSaveYourselfPhase2RequestProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData
)
{
    the_server->phase2Request( (KSMClient*) managerData );
}

void KSMSaveYourselfDoneProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    Bool                success
)
{
    the_server->saveYourselfDone( (KSMClient*) managerData, success );
}

void KSMCloseConnectionProc (
    SmsConn             smsConn,
    SmPointer           managerData,
    int                 count,
    char **             reasonMsgs
)
{
    the_server->deleteClient( ( KSMClient* ) managerData );
    if ( count )
        SmFreeReasons( count, reasonMsgs );
    IceConn iceConn = SmsGetIceConnection( smsConn );
    SmsCleanUp( smsConn );
    IceSetShutdownNegotiation (iceConn, False);
    IceCloseConnection( iceConn );
}

void KSMSetPropertiesProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    int                 numProps,
    SmProp **           props
)
{
    KSMClient* client = ( KSMClient* ) managerData;
    for ( int i = 0; i < numProps; i++ ) {
        SmProp *p = client->property( props[i]->name );
        if ( p ) {
            client->properties.removeRef( p );
            SmFreeProperty( p );
        }
        client->properties.append( props[i] );
        if ( !qstrcmp( props[i]->name, SmProgram ) )
            the_server->clientSetProgram( client );
    }

    if ( numProps )
        free( props );

}

void KSMDeletePropertiesProc (
    SmsConn             /* smsConn */,
    SmPointer           managerData,
    int                 numProps,
    char **             propNames
)
{
    KSMClient* client = ( KSMClient* ) managerData;
    for ( int i = 0; i < numProps; i++ ) {
        SmProp *p = client->property( propNames[i] );
        if ( p ) {
            client->properties.removeRef( p );
            SmFreeProperty( p );
        }
    }
}

void KSMGetPropertiesProc (
    SmsConn             smsConn,
    SmPointer           managerData
)
{
    KSMClient* client = ( KSMClient* ) managerData;
    SmProp** props = new SmProp*[client->properties.count()];
    int i = 0;
    for ( SmProp* prop = client->properties.first(); prop; prop = client->properties.next() )
        props[i++] = prop;

    SmsReturnProperties( smsConn, i, props );
    delete [] props;
}


class KSMListener : public TQSocketNotifier
{
public:
    KSMListener( IceListenObj obj )
        : TQSocketNotifier( IceGetListenConnectionNumber( obj ),
                           TQSocketNotifier::Read, 0, 0)
{
    listenObj = obj;
}

    IceListenObj listenObj;
};

class KSMConnection : public TQSocketNotifier
{
 public:
  KSMConnection( IceConn conn )
    : TQSocketNotifier( IceConnectionNumber( conn ),
                       TQSocketNotifier::Read, 0, 0 )
    {
        iceConn = conn;
    }

    IceConn iceConn;
};


/* for printing hex digits */
static void fprintfhex (FILE *fp, unsigned int len, char *cp)
{
    static const char hexchars[] = "0123456789abcdef";

    for (; len > 0; len--, cp++) {
        unsigned char s = *cp;
        putc(hexchars[s >> 4], fp);
        putc(hexchars[s & 0x0f], fp);
    }
}

/*
 * We use temporary files which contain commands to add/remove entries from
 * the .ICEauthority file.
 */
static void write_iceauth (FILE *addfp, FILE *removefp, IceAuthDataEntry *entry)
{
    fprintf (addfp,
             "add %s \"\" %s %s ",
             entry->protocol_name,
             entry->network_id,
             entry->auth_name);
    fprintfhex (addfp, entry->auth_data_length, entry->auth_data);
    fprintf (addfp, "\n");

    fprintf (removefp,
             "remove protoname=%s protodata=\"\" netid=%s authname=%s\n",
             entry->protocol_name,
             entry->network_id,
             entry->auth_name);
}


#define MAGIC_COOKIE_LEN 16

Status SetAuthentication_local (int count, IceListenObj *listenObjs)
{
    int i;
    for (i = 0; i < count; i ++) {
        char *prot = IceGetListenConnectionString(listenObjs[i]);
        if (!prot) continue;
        char *host = strchr(prot, '/');
        char *sock = 0;
        if (host) {
            *host=0;
            host++;
            sock = strchr(host, ':');
            if (sock) {
                *sock = 0;
                sock++;
            }
        }
        kdDebug( 1218 ) << "KSMServer: SetAProc_loc: conn " << (unsigned)i << ", prot=" << prot << ", file=" << sock << endl;
        if (sock && !strcmp(prot, "local")) {
            chmod(sock, 0700);
        }
        IceSetHostBasedAuthProc (listenObjs[i], HostBasedAuthProc);
        free(prot);
    }
    return 1;
}

Status SetAuthentication (int count, IceListenObj *listenObjs,
                          IceAuthDataEntry **authDataEntries)
{
    KTempFile addAuthFile;
    addAuthFile.setAutoDelete(true);

    remAuthFile = new KTempFile;
    remAuthFile->setAutoDelete(true);

    if ((addAuthFile.status() != 0) || (remAuthFile->status() != 0))
        return 0;

    if ((*authDataEntries = (IceAuthDataEntry *) malloc (
                         count * 2 * sizeof (IceAuthDataEntry))) == NULL)
        return 0;

    for (int i = 0; i < numTransports * 2; i += 2) {
        (*authDataEntries)[i].network_id =
            IceGetListenConnectionString (listenObjs[i/2]);
        (*authDataEntries)[i].protocol_name = (char *) "ICE";
        (*authDataEntries)[i].auth_name = (char *) "MIT-MAGIC-COOKIE-1";

        (*authDataEntries)[i].auth_data =
            IceGenerateMagicCookie (MAGIC_COOKIE_LEN);
        (*authDataEntries)[i].auth_data_length = MAGIC_COOKIE_LEN;

        (*authDataEntries)[i+1].network_id =
            IceGetListenConnectionString (listenObjs[i/2]);
        (*authDataEntries)[i+1].protocol_name = (char *) "XSMP";
        (*authDataEntries)[i+1].auth_name = (char *) "MIT-MAGIC-COOKIE-1";

        (*authDataEntries)[i+1].auth_data =
            IceGenerateMagicCookie (MAGIC_COOKIE_LEN);
        (*authDataEntries)[i+1].auth_data_length = MAGIC_COOKIE_LEN;

        write_iceauth (addAuthFile.fstream(), remAuthFile->fstream(), &(*authDataEntries)[i]);
        write_iceauth (addAuthFile.fstream(), remAuthFile->fstream(), &(*authDataEntries)[i+1]);

        IceSetPaAuthData (2, &(*authDataEntries)[i]);

        IceSetHostBasedAuthProc (listenObjs[i/2], HostBasedAuthProc);
    }
    addAuthFile.close();
    remAuthFile->close();

    TQString iceAuth = TDEGlobal::dirs()->findExe("iceauth");
    if (iceAuth.isEmpty())
    {
        tqWarning("[KSMServer] could not find iceauth");
        return 0;
    }

    TDEProcess p;
    p << iceAuth << "source" << addAuthFile.name();
    p.start(TDEProcess::Block);

    return (1);
}

/*
 * Free up authentication data.
 */
void FreeAuthenticationData(int count, IceAuthDataEntry *authDataEntries)
{
    /* Each transport has entries for ICE and XSMP */
    if (only_local)
        return;

    for (int i = 0; i < count * 2; i++) {
        free (authDataEntries[i].network_id);
        free (authDataEntries[i].auth_data);
    }

    free (authDataEntries);

    TQString iceAuth = TDEGlobal::dirs()->findExe("iceauth");
    if (iceAuth.isEmpty())
    {
        tqWarning("[KSMServer] could not find iceauth");
        return;
    }

    TDEProcess p;
    p << iceAuth << "source" << remAuthFile->name();
    p.start(TDEProcess::Block);

    delete remAuthFile;
    remAuthFile = 0;
}

static int Xio_ErrorHandler( Display * )
{
    tqWarning("[KSMServer] Fatal IO error: client killed");

    // Don't do anything that might require the X connection
    if (the_server)
    {
       KSMServer *server = the_server;
       the_server = 0;
       server->cleanUp();
       // Don't delete server!!
    }

    exit(0); // Don't report error, it's not our fault.
}


void KSMServer::setupXIOErrorHandler()
{
    XSetIOErrorHandler(Xio_ErrorHandler);
}

static void sighandler(int sig)
{
    if (sig == SIGHUP) {
        signal(SIGHUP, sighandler);
        return;
    }

    if (the_server)
    {
       KSMServer *server = the_server;
       the_server = 0;
       server->cleanUp();
       delete server;
    }

    if (kapp) {
        kapp->quit();
    }
    //::exit(0);
}


void KSMWatchProc ( IceConn iceConn, IcePointer client_data, Bool opening, IcePointer* watch_data)
{
    KSMServer* ds = ( KSMServer*) client_data;

    if (opening) {
        *watch_data = (IcePointer) ds->watchConnection( iceConn );
    }
    else  {
        ds->removeConnection( (KSMConnection*) *watch_data );
    }
}

static Status KSMNewClientProc ( SmsConn conn, SmPointer manager_data,
                                 unsigned long* mask_ret, SmsCallbacks* cb, char** failure_reason_ret)
{
    *failure_reason_ret = 0;

    void* client =  ((KSMServer*) manager_data )->newClient( conn );

    cb->register_client.callback = KSMRegisterClientProc;
    cb->register_client.manager_data = client;
    cb->interact_request.callback = KSMInteractRequestProc;
    cb->interact_request.manager_data = client;
    cb->interact_done.callback = KSMInteractDoneProc;
    cb->interact_done.manager_data = client;
    cb->save_yourself_request.callback = KSMSaveYourselfRequestProc;
    cb->save_yourself_request.manager_data = client;
    cb->save_yourself_phase2_request.callback = KSMSaveYourselfPhase2RequestProc;
    cb->save_yourself_phase2_request.manager_data = client;
    cb->save_yourself_done.callback = KSMSaveYourselfDoneProc;
    cb->save_yourself_done.manager_data = client;
    cb->close_connection.callback = KSMCloseConnectionProc;
    cb->close_connection.manager_data = client;
    cb->set_properties.callback = KSMSetPropertiesProc;
    cb->set_properties.manager_data = client;
    cb->delete_properties.callback = KSMDeletePropertiesProc;
    cb->delete_properties.manager_data = client;
    cb->get_properties.callback = KSMGetPropertiesProc;
    cb->get_properties.manager_data = client;

    *mask_ret = SmsRegisterClientProcMask |
                SmsInteractRequestProcMask |
                SmsInteractDoneProcMask |
                SmsSaveYourselfRequestProcMask |
                SmsSaveYourselfP2RequestProcMask |
                SmsSaveYourselfDoneProcMask |
                SmsCloseConnectionProcMask |
                SmsSetPropertiesProcMask |
                SmsDeletePropertiesProcMask |
                SmsGetPropertiesProcMask;
    return 1;
}


#ifdef HAVE__ICETRANSNOLISTEN
extern "C" int _IceTransNoListen(const char * protocol);
#endif

KSMServer::KSMServer( const TQString& windowManager, const TQString& windowManagerAddArgs, bool _only_local )
  : DCOPObject("ksmserver"), startupNotifierIPDlg(0), shutdownNotifierIPDlg(0), sessionGroup( "" ), protectionTimerCounter(0)
{
    the_server = this;
    clean = false;
    wm = windowManager;
    wmAddArgs = windowManagerAddArgs;

    shutdownType = TDEApplication::ShutdownTypeNone;

    state = Idle;
    dialogActive = false;
    saveSession = false;
    wmPhase1WaitingCount = 0;
    TDEConfig* config = TDEGlobal::config();
    config->setGroup("General" );
    clientInteracting = 0;
    xonCommand = config->readEntry( "xonCommand", "xon" );

#ifdef __TDE_HAVE_TDEHWLIB
    hwDevices = TDEGlobal::hardwareDevices();
#endif

    connect( &knotifyTimeoutTimer, TQT_SIGNAL( timeout()), TQT_SLOT( knotifyTimeout()));
    connect( &startupSuspendTimeoutTimer, TQT_SIGNAL( timeout()), TQT_SLOT( startupSuspendTimeout()));
    connect( &pendingShutdown, TQT_SIGNAL( timeout()), TQT_SLOT( pendingShutdownTimeout()));

    only_local = _only_local;
#ifdef HAVE__ICETRANSNOLISTEN
    if (only_local)
        _IceTransNoListen("tcp");
#else
    only_local = false;
#endif

    launcher = TDEApplication::launcher();

    char        errormsg[256];
    if (!SmsInitialize ( (char*) KSMVendorString, (char*) KSMReleaseString,
                         KSMNewClientProc,
                         (SmPointer) this,
                         HostBasedAuthProc, 256, errormsg ) ) {

        tqWarning("[KSMServer] could not register XSM protocol");
    }

    if (!IceListenForConnections (&numTransports, &listenObjs,
                                  256, errormsg))
    {
        tqWarning("[KSMServer] Error listening for connections: %s", errormsg);
        tqWarning("[KSMServer] Aborting.");
        exit(1);
    }

    {
        // publish available transports.
        TQCString fName = TQFile::encodeName(locateLocal("socket", "KSMserver"));
        TQCString display = ::getenv("DISPLAY");
        // strip the screen number from the display
        display.replace(TQRegExp("\\.[0-9]+$"), "");
        int i;
        while( (i = display.find(':')) >= 0)
           display[i] = '_';

        fName += "_"+display;
        FILE *f;
        f = ::fopen(fName.data(), "w+");
        if (!f)
        {
            tqWarning("[KSMServer] can't open %s: %s", fName.data(), strerror(errno));
            tqWarning("[KSMServer] Aborting.");
            exit(1);
        }
        char* session_manager = IceComposeNetworkIdList(numTransports, listenObjs);
        fprintf(f, "%s\n%i\n", session_manager, getpid());
        fclose(f);
        setenv( "SESSION_MANAGER", session_manager, true  );
       // Pass env. var to tdeinit.
       DCOPRef( launcher ).send( "setLaunchEnv", "SESSION_MANAGER", (const char*) session_manager );
    }

    if (only_local) {
        if (!SetAuthentication_local(numTransports, listenObjs))
            tqFatal("[KSMServer] authentication setup failed.");
    } else {
        if (!SetAuthentication(numTransports, listenObjs, &authDataEntries))
            tqFatal("[KSMServer] authentication setup failed.");
    }

    IceAddConnectionWatch (KSMWatchProc, (IcePointer) this);

    listener.setAutoDelete( true );
    KSMListener* con;
    for ( int i = 0; i < numTransports; i++) {
        con = new KSMListener( listenObjs[i] );
        listener.append( con );
        connect( con, TQT_SIGNAL( activated(int) ), this, TQT_SLOT( newConnection(int) ) );
    }

    signal(SIGHUP, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGPIPE, SIG_IGN);

    connect( &notificationTimer, TQT_SIGNAL( timeout() ), this, TQT_SLOT( notificationTimeout() ) );
    connect( &protectionTimer, TQT_SIGNAL( timeout() ), this, TQT_SLOT( protectionTimerTick() ) );
    connect( &restoreTimer, TQT_SIGNAL( timeout() ), this, TQT_SLOT( tryRestoreNext() ) );
    connect( &shutdownTimer, TQT_SIGNAL( timeout() ), this, TQT_SLOT( timeoutQuit() ) );
    connect( kapp, TQT_SIGNAL( shutDown() ), this, TQT_SLOT( cleanUp() ) );
}

KSMServer::~KSMServer()
{
    the_server = 0;
    cleanUp();
}

void KSMServer::cleanUp()
{
    if (clean) return;
    clean = true;
    IceFreeListenObjs (numTransports, listenObjs);

    TQCString fName = TQFile::encodeName(locateLocal("socket", "KSMserver"));
    TQCString display = ::getenv("DISPLAY");
    // strip the screen number from the display
    display.replace(TQRegExp("\\.[0-9]+$"), "");
    int i;
    while( (i = display.find(':')) >= 0) {
         display[i] = '_';
    }

    fName += "_"+display;
    ::unlink(fName.data());

    FreeAuthenticationData(numTransports, authDataEntries);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    if (DM().canShutdown()) {
        DM().shutdown( shutdownType, shutdownMode, bootOption );
    }
    else {
#ifdef __TDE_HAVE_TDEHWLIB
        TDERootSystemDevice* rootDevice = hwDevices->rootSystemDevice();
        if (rootDevice) {
            if (shutdownType == TDEApplication::ShutdownTypeHalt) {
                rootDevice->setPowerState(TDESystemPowerState::PowerOff);
            }
            if (shutdownType == TDEApplication::ShutdownTypeReboot) {
                rootDevice->setPowerState(TDESystemPowerState::Reboot);
            }
        }
#endif
    }
}



void* KSMServer::watchConnection( IceConn iceConn )
{
    KSMConnection* conn = new KSMConnection( iceConn );
    connect( conn, TQT_SIGNAL( activated(int) ), this, TQT_SLOT( processData(int) ) );
    return (void*) conn;
}

void KSMServer::removeConnection( KSMConnection* conn )
{
    delete conn;
}


/*!
  Called from our IceIoErrorHandler
 */
void KSMServer::ioError( IceConn /*iceConn*/  )
{
}

void KSMServer::processData( int /*socket*/ )
{
    IceConn iceConn = ((KSMConnection*)sender())->iceConn;
    IceProcessMessagesStatus status = IceProcessMessages( iceConn, 0, 0 );
    if ( status == IceProcessMessagesIOError ) {
        IceSetShutdownNegotiation( iceConn, False );
        TQPtrListIterator<KSMClient> it ( clients );
        while ( it.current() &&SmsGetIceConnection( it.current()->connection() ) != iceConn )
            ++it;
        if ( it.current() ) {
            SmsConn smsConn = it.current()->connection();
            deleteClient( it.current() );
            SmsCleanUp( smsConn );
            (void) IceCloseConnection( iceConn );
        }
    }
}

KSMClient* KSMServer::newClient( SmsConn conn )
{
    KSMClient* client = new KSMClient( conn );
    clients.append( client );
    return client;
}

void KSMServer::deleteClient( KSMClient* client )
{
    if ( clients.findRef( client ) == -1 ) // paranoia
        return;
    clients.removeRef( client );
    if ( client == clientInteracting ) {
        clientInteracting = 0;
        handlePendingInteractions();
    }
    delete client;
    if ( state == Shutdown || state == Checkpoint )
        completeShutdownOrCheckpoint();
    if ( state == Killing )
        completeKilling();
    if ( state == KillingWM )
        completeKillingWM();
}

void KSMServer::newConnection( int /*socket*/ )
{
    IceAcceptStatus status;
    IceConn iceConn = IceAcceptConnection( ((KSMListener*)sender())->listenObj, &status);
    if (iceConn == NULL) {
        return;
    }
    IceSetShutdownNegotiation( iceConn, False );
    IceConnectStatus cstatus;
    while ((cstatus = IceConnectionStatus (iceConn))==IceConnectPending) {
        (void) IceProcessMessages( iceConn, 0, 0 );
    }

    if (cstatus != IceConnectAccepted) {
        if (cstatus == IceConnectIOError)
            kdDebug( 1218 ) << "IO error opening ICE Connection!" << endl;
        else
            kdDebug( 1218 ) << "ICE Connection rejected!" << endl;
        (void )IceCloseConnection (iceConn);
        return;
    }
}


TQString KSMServer::currentSession()
{
    if ( sessionGroup.startsWith( "Session: " ) )
        return sessionGroup.mid( 9 );
    return ""; // empty, not null, since used for TDEConfig::setGroup
}

void KSMServer::discardSession()
{
    TDEConfig* config = TDEGlobal::config();
    config->setGroup( sessionGroup );
    int count =  config->readNumEntry( "count", 0 );
    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        TQStringList discardCommand = c->discardCommand();
        if ( discardCommand.isEmpty())
            continue;
        // check that non of the old clients used the exactly same
        // discardCommand before we execute it. This used to be the
        // case up to KDE and Qt < 3.1
        int i = 1;
        while ( i <= count &&
                config->readPathListEntry( TQString("discardCommand") + TQString::number(i) ) != discardCommand )
            i++;
        if ( i <= count )
            executeCommand( discardCommand );
    }
}

void KSMServer::storeSession()
{
    TDEConfig* config = TDEGlobal::config();
    config->reparseConfiguration(); // config may have changed in the KControl module
    config->setGroup("General" );
    excludeApps = TQStringList::split( TQRegExp( "[,:]" ), config->readEntry( "excludeApps" ).lower());
    config->setGroup( sessionGroup );
    int count =  config->readNumEntry( "count" );
    for ( int i = 1; i <= count; i++ ) {
        TQStringList discardCommand = config->readPathListEntry( TQString("discardCommand") + TQString::number(i) );
        if ( discardCommand.isEmpty())
            continue;
        // check that non of the new clients uses the exactly same
        // discardCommand before we execute it. This used to be the
        // case up to KDE and Qt < 3.1
        KSMClient* c = clients.first();
        while ( c && discardCommand != c->discardCommand() )
            c = clients.next();
        if ( c )
            continue;
        executeCommand( discardCommand );
    }
    config->deleteGroup( sessionGroup ); //### does not work with global config object...
    config->setGroup( sessionGroup );
    count =  0;

    if ( !wm.isEmpty() ) {
        // put the wm first
        for ( KSMClient* c = clients.first(); c; c = clients.next() )
            if ( c->program() == wm ) {
                clients.prepend( clients.take() );
                break;
            }
    }

    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        int restartHint = c->restartStyleHint();
        if (restartHint == SmRestartNever)
           continue;
        TQString program = c->program();
        TQStringList restartCommand = c->restartCommand();
        if (program.isEmpty() && restartCommand.isEmpty())
           continue;
        if (excludeApps.contains( program.lower()))
            continue;

        count++;
        TQString n = TQString::number(count);
        config->writeEntry( TQString("program")+n, program );
        config->writeEntry( TQString("clientId")+n, c->clientId() );
        config->writeEntry( TQString("restartCommand")+n, restartCommand );
        config->writePathEntry( TQString("discardCommand")+n, c->discardCommand() );
        config->writeEntry( TQString("restartStyleHint")+n, restartHint );
        config->writeEntry( TQString("userId")+n, c->userId() );
        config->writeEntry( TQString("wasWm")+n, isWM( c ));
    }
    config->writeEntry( "count", count );

    config->setGroup("General");
    config->writeEntry( "screenCount", ScreenCount(tqt_xdisplay()));

    storeLegacySession( config );
    config->sync();
}


TQStringList KSMServer::sessionList()
{
    TQStringList sessions = "default";
    TDEConfig* config = TDEGlobal::config();
    TQStringList groups = config->groupList();
    for ( TQStringList::ConstIterator it = groups.begin(); it != groups.end(); it++ )
        if ( (*it).startsWith( "Session: " ) )
            sessions << (*it).mid( 9 );
    return sessions;
}

bool KSMServer::isWM( const KSMClient* client ) const
{
    return isWM( client->program());
}

bool KSMServer::isWM( const TQString& program ) const
{
    // KWin relies on ksmserver's special treatment in phase1,
    // therefore make sure it's recognized even if ksmserver
    // was initially started with different WM, and twin replaced
    // it later
    return ((program == wm) || (program == "twin"));
}

bool KSMServer::isCM( const KSMClient* client ) const
{
    return isCM( client->program());
}

bool KSMServer::isCM( const TQString& program ) const
{
    // Returns true if the program in question is a composition manager
    return (program == TDE_COMPOSITOR_BINARY);
}

bool KSMServer::isDesktop( const KSMClient* client ) const
{
    return isDesktop( client->program());
}

bool KSMServer::isDesktop( const TQString& program ) const
{
    // Returns true if the program in question is a desktop
    return (program == "kdesktop");
}

bool KSMServer::isNotifier( const KSMClient* client ) const
{
    return isNotifier( client->program());
}

bool KSMServer::isNotifier( const TQString& program ) const
{
    return (program == "knotify");
}

bool KSMServer::isCrashHandler( const KSMClient* client ) const
{
    return isNotifier( client->program());
}

bool KSMServer::isCrashHandler( const TQString& program ) const
{
    return (program == "drkonqi");
}

bool KSMServer::defaultSession() const
{
    return sessionGroup.isEmpty();
}
