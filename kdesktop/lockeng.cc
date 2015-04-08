//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
// Copyright (c) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//


#include <config.h>

#include <stdlib.h>

#include <kstandarddirs.h>
#include <tdeapplication.h>
#include <kservicegroup.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <tqfile.h>
#include <tqtimer.h>
#include <dcopclient.h>
#include <assert.h>

#include <dbus/dbus-shared.h>
#include <tqdbusdata.h>
#include <tqdbuserror.h>
#include <tqdbusmessage.h>
#include <tqdbusobjectpath.h>
#include <tqdbusproxy.h>

#include "lockeng.h"
#include "lockeng.moc"
#include "kdesktopsettings.h"

#define SYSTEMD_LOGIN1_SERVICE		"org.freedesktop.login1"
#define SYSTEMD_LOGIN1_PATH		"/org/freedesktop/login1"
#define SYSTEMD_LOGIN1_MANAGER_IFACE	"org.freedesktop.login1.Manager"
#define SYSTEMD_LOGIN1_SESSION_IFACE	"org.freedesktop.login1.Session"
#define SYSTEMD_LOGIN1_SEAT_IFACE	"org.freedesktop.login1.Seat"

#define DBUS_CONN_NAME			"kdesktop_lock"

#include "xautolock_c.h"
extern xautolock_corner_t xautolock_corners[ 4 ];

bool trinity_lockeng_sak_available = TRUE;

SaverEngine* m_masterSaverEngine = NULL;
static void sigusr1_handler(int)
{
    if (m_masterSaverEngine) {
        m_masterSaverEngine->slotLockProcessWaiting();
    }
}
static void sigusr2_handler(int)
{
    if (m_masterSaverEngine) {
        m_masterSaverEngine->slotLockProcessFullyActivated();
    }
}
static void sigttin_handler(int)
{
    if (m_masterSaverEngine) {
        m_masterSaverEngine->slotLockProcessReady();
    }
}

//===========================================================================
//
// Screen saver engine. Doesn't handle the actual screensaver window,
// starting screensaver hacks, or password entry. That's done by
// a newly started process.
//
SaverEngine::SaverEngine()
    : TQWidget(),
      KScreensaverIface(),
      mBlankOnly(false),
      mSAKProcess(NULL),
      mTerminationRequested(false),
      mSaverProcessReady(false),
      dBusLocal(0),
      dBusWatch(0),
      systemdSession(0)
{
    // handle SIGUSR1
    m_masterSaverEngine = this;
    mSignalAction.sa_handler= sigusr1_handler;
    sigemptyset(&(mSignalAction.sa_mask));
    sigaddset(&(mSignalAction.sa_mask), SIGUSR1);
    mSignalAction.sa_flags = 0;
    sigaction(SIGUSR1, &mSignalAction, 0L);

    // handle SIGUSR2
    m_masterSaverEngine = this;
    mSignalAction.sa_handler= sigusr2_handler;
    sigemptyset(&(mSignalAction.sa_mask));
    sigaddset(&(mSignalAction.sa_mask), SIGUSR2);
    mSignalAction.sa_flags = 0;
    sigaction(SIGUSR2, &mSignalAction, 0L);

    // handle SIGTTIN
    m_masterSaverEngine = this;
    mSignalAction.sa_handler= sigttin_handler;
    sigemptyset(&(mSignalAction.sa_mask));
    sigaddset(&(mSignalAction.sa_mask), SIGTTIN);
    mSignalAction.sa_flags = 0;
    sigaction(SIGTTIN, &mSignalAction, 0L);

    // Save X screensaver parameters
    XGetScreenSaver(tqt_xdisplay(), &mXTimeout, &mXInterval,
                    &mXBlanking, &mXExposures);

    mState = Waiting;
    mXAutoLock = 0;
    mEnabled = false;

    connect(&mLockProcess, TQT_SIGNAL(processExited(TDEProcess *)),
                        TQT_SLOT(lockProcessExited()));

    mSAKProcess = new TDEProcess;
    *mSAKProcess << "tdmtsak";
    connect(mSAKProcess, TQT_SIGNAL(processExited(TDEProcess*)), this, TQT_SLOT(slotSAKProcessExited()));

    TQTimer::singleShot( 0, this, TQT_SLOT(handleSecureDialog()) );

    configure();

    mLockProcess.clearArguments();
    TQString path = TDEStandardDirs::findExe( "kdesktop_lock" );
    if( path.isEmpty())
    {
	kdDebug( 1204 ) << "Can't find kdesktop_lock!" << endl;
    }
    mLockProcess << path;
    mLockProcess << TQString( "--internal" ) << TQString( "%1" ).arg(getpid());
    if (mLockProcess.start() == false )
    {
	kdDebug( 1204 ) << "Failed to start kdesktop_lock!" << endl;
    }

    dBusConnect();
}

//---------------------------------------------------------------------------
//
// Destructor - usual cleanups.
//
SaverEngine::~SaverEngine()
{
    if (mState == Waiting) {
        kill(mLockProcess.pid(), SIGKILL);
    }

    mLockProcess.detach(); // don't kill it if we crash
    delete mXAutoLock;

    dBusClose();

    // Restore X screensaver parameters
    XSetScreenSaver(tqt_xdisplay(), mXTimeout, mXInterval, mXBlanking,
                    mXExposures);
}

//---------------------------------------------------------------------------
//
// This should be called only using DCOP.
//
void SaverEngine::lock()
{
    lockScreen(true);
}

//---------------------------------------------------------------------------
//
// Lock the screen
//
void SaverEngine::lockScreen(bool DCOP)
{
    bool ok = true;
    if (mState != Saving)
    {
        ok = startLockProcess( ForceLock );
        // It takes a while for kdesktop_lock to start and lock the screen.
        // Therefore delay the DCOP call until it tells kdesktop that the locking is in effect.
        // This is done only for --forcelock .
        if( ok && mState != Saving )
        {
            if (DCOP) {
                DCOPClientTransaction* trans = kapp->dcopClient()->beginTransaction();
                if (trans) {
                    mLockTransactions.append( trans );
                }
            }
        }
    }
    else
    {
        mLockProcess.kill( SIGHUP );
    }
}

void SaverEngine::processLockTransactions()
{
    for( TQValueVector< DCOPClientTransaction* >::ConstIterator it = mLockTransactions.begin();
         it != mLockTransactions.end();
         ++it )
    {
        TQCString replyType = "void";
        TQByteArray arr;
        kapp->dcopClient()->endTransaction( *it, replyType, arr );
    }
    mLockTransactions.clear();
}

void SaverEngine::saverLockReady()
{
    if( mState != Engaging )
    {
	kdDebug( 1204 ) << "Got unexpected saverReady()" << endl;
    }
    kdDebug( 1204 ) << "Saver Lock Ready" << endl;
    processLockTransactions();
}

//---------------------------------------------------------------------------
void SaverEngine::save()
{
    if (mState == Waiting)
    {
        startLockProcess( DefaultLock );
    }
}

//---------------------------------------------------------------------------
void SaverEngine::quit()
{
    if (mState == Saving || mState == Engaging)
    {
        stopLockProcess();
    }
}

//---------------------------------------------------------------------------
bool SaverEngine::isEnabled()
{
  return mEnabled;
}

//---------------------------------------------------------------------------
bool SaverEngine::enable( bool e )
{
    if ( e == mEnabled )
	return true;

    // If we aren't in a suitable state, we will not reconfigure.
    if (mState != Waiting)
        return false;

    mEnabled = e;

    if (mEnabled)
    {
	if ( !mXAutoLock ) {
	    mXAutoLock = new XAutoLock();
	    connect(mXAutoLock, TQT_SIGNAL(timeout()), TQT_SLOT(idleTimeout()));
	}
        mXAutoLock->setTimeout(mTimeout);
        mXAutoLock->setDPMS(true);
	//mXAutoLock->changeCornerLockStatus( mLockCornerTopLeft, mLockCornerTopRight, mLockCornerBottomLeft, mLockCornerBottomRight);

        // We'll handle blanking
        XSetScreenSaver(tqt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
        kdDebug() << "XSetScreenSaver " << mTimeout + 10 << endl;

        mXAutoLock->start();

        kdDebug(1204) << "Saver Engine started, timeout: " << mTimeout << endl;
    }
    else
    {
	if (mXAutoLock)
	{
	    delete mXAutoLock;
	    mXAutoLock = 0;
	}

        XForceScreenSaver(tqt_xdisplay(), ScreenSaverReset );
        XSetScreenSaver(tqt_xdisplay(), 0, mXInterval,  PreferBlanking, DontAllowExposures);
        kdDebug(1204) << "Saver Engine disabled" << endl;
    }

    return true;
}

//---------------------------------------------------------------------------
bool SaverEngine::isBlanked()
{
  return (mState != Waiting);
}

void SaverEngine::enableExports()
{
#ifdef Q_WS_X11
	kdDebug(270) << k_lineinfo << "activating background exports.\n";
	DCOPClient *client = kapp->dcopClient();
	if (!client->isAttached()) {
		client->attach();
	}
	TQByteArray data;
	TQDataStream args( data, IO_WriteOnly );
	args << 1;
	
	TQCString appname( "kdesktop" );
	int screen_number = DefaultScreen(tqt_xdisplay());
	if ( screen_number )
	appname.sprintf("kdesktop-screen-%d", screen_number );
	
	client->send( appname, "KBackgroundIface", "setExport(int)", data );
#endif
}

//---------------------------------------------------------------------------
void SaverEngine::handleSecureDialog()
{
    // Wait for SAK press
    if (!mSAKProcess->isRunning()) mSAKProcess->start();
}

void SaverEngine::slotSAKProcessExited()
{
    int retcode = mSAKProcess->exitStatus();
    if ((retcode != 0) && (mSAKProcess->normalExit())) {
        trinity_lockeng_sak_available = FALSE;
        printf("[kdesktop] SAK driven secure dialog is not available for use (retcode %d).  Check tdmtsak for proper functionality.\n", retcode); fflush(stdout);
    }

    if (mState == Preparing) {
        return;
    }

    if ((mSAKProcess->normalExit()) && (trinity_lockeng_sak_available == TRUE)) {
        bool ok = true;
        if (mState == Waiting)
        {
            ok = startLockProcess( SecureDialog );
            if( ok && mState != Saving )
            {
            }
        }
        else
        {
            mLockProcess.kill( SIGHUP );
        }
    }
}

//---------------------------------------------------------------------------
//
// Read and apply configuration.
//
void SaverEngine::configure()
{
    // If we aren't in a suitable state, we will not reconfigure.
    if (mState != Waiting)
        return;

    // create a new config obj to ensure we read the latest options
    KDesktopSettings::self()->readConfig();

    bool e  = KDesktopSettings::screenSaverEnabled();
    mTimeout = KDesktopSettings::timeout();

    mEnabled = !e;   // force the enable()

    int action;
    action = KDesktopSettings::actionTopLeft();
    xautolock_corners[0] = applyManualSettings(action);
    action = KDesktopSettings::actionTopRight();
    xautolock_corners[1] = applyManualSettings(action);
    action = KDesktopSettings::actionBottomLeft();
    xautolock_corners[2] = applyManualSettings(action);
    action = KDesktopSettings::actionBottomRight();
    xautolock_corners[3] = applyManualSettings(action);

    enable( e );
}

//---------------------------------------------------------------------------
//
//  Set a variable to indicate only using the blanker and not the saver.
//
void SaverEngine::setBlankOnly( bool blankOnly )
{
	mBlankOnly = blankOnly;
	// FIXME: if running, stop and restart?  What about security
	// implications of this?
}

bool SaverEngine::restartDesktopLockProcess()
{
    if (!mLockProcess.isRunning()) {
        mSaverProcessReady = false;
	mLockProcess.clearArguments();
	TQString path = TDEStandardDirs::findExe( "kdesktop_lock" );
	if( path.isEmpty())
	{
	    kdDebug( 1204 ) << "Can't find kdesktop_lock!" << endl;
	    return false;
	}
	mLockProcess << path;
	mLockProcess << TQString( "--internal" ) << TQString( "%1" ).arg(getpid());
        if (mLockProcess.start() == false )
	{
	    kdDebug( 1204 ) << "Failed to start kdesktop_lock!" << endl;
	    return false;
	}
        // Wait for the saver process to signal ready...
        int count = 0;
        while (!mSaverProcessReady) {
            count++;
            usleep(100);
            if (count > 100) {
                return false;
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------
//
// Start the screen saver.
//
bool SaverEngine::startLockProcess( LockType lock_type )
{
    int ret;

    if (mState == Saving) {
        return true;
    }

    mState = Preparing;
    mSAKProcess->kill(SIGTERM);

    enableExports();

    kdDebug(1204) << "SaverEngine: starting saver" << endl;
    emitDCOPSignal("KDE_start_screensaver()", TQByteArray());

    if (!restartDesktopLockProcess()) {
        mState = Waiting;
        return false;
    }

    switch( lock_type )
    {
	case ForceLock:
    	    mLockProcess.kill(SIGUSR1);		// Request forcelock
	  break;
	case DontLock:
    	    mLockProcess.kill(SIGUSR2);		// Request dontlock
	  break;
	case SecureDialog:
    	    mLockProcess.kill(SIGWINCH);	// Request secure dialog
	  break;
	default:
	  break;
    }
    if (mBlankOnly) {
	    mLockProcess.kill(SIGTTIN);		// Request blanking
    }

    ret = mLockProcess.kill(SIGTTOU);			// Start lock
    if (!ret) {
	    mState = Waiting;
	    return false;
    }
    XSetScreenSaver(tqt_xdisplay(), 0, mXInterval,  PreferBlanking, mXExposures);

    mState = Engaging;
    if (mXAutoLock)
    {
        mXAutoLock->stop();
    }
    return true;
}

//---------------------------------------------------------------------------
//
// Stop the screen saver.
//
void SaverEngine::stopLockProcess()
{
    if (mState == Waiting)
    {
        kdWarning(1204) << "SaverEngine::stopSaver() saver not active" << endl;
        return;
    }
    kdDebug(1204) << "SaverEngine: stopping lock" << endl;
    emitDCOPSignal("KDE_stop_screensaver()", TQByteArray());

    mTerminationRequested=true;
    mLockProcess.kill();

    if (mEnabled)
    {
        if (mXAutoLock)
        {
            mXAutoLock->start();
        }
        XForceScreenSaver(tqt_xdisplay(), ScreenSaverReset );
        XSetScreenSaver(tqt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
    }
    processLockTransactions();
    mState = Waiting;

    if( systemdSession && systemdSession->canSend() ) {
	TQValueList<TQT_DBusData> params;
	params << TQT_DBusData::fromBool(false);
	TQT_DBusMessage reply = systemdSession->sendWithReply("SetIdleHint", params);
    }
}

void SaverEngine::recoverFromHackingAttempt()
{
    // Try to relaunch saver with forcelock
    if (!startLockProcess( ForceLock )) {
        // Terminate the TDE session ASAP!
        // Values are explained at http://lists.kde.org/?l=kde-linux&m=115770988603387
        TQByteArray data;
        TQDataStream arg(data, IO_WriteOnly);
        arg << (int)0 << (int)0 << (int)2;
        if ( ! kapp->dcopClient()->send("ksmserver", "default", "logout(int,int,int)", data) ) {
            // Someone got to DCOP before we did
            // Try an emergency system logout
            system("logout");
        }
    }
}

void SaverEngine::lockProcessExited()
{
    bool abnormalExit = false;
    if (mLockProcess.normalExit() == false) {
        abnormalExit = true;
    }
    else {
        if (mLockProcess.exitStatus() != 0) {
            abnormalExit = true;
        }
    }
    if (mTerminationRequested == true) {
        abnormalExit = false;
        mTerminationRequested = false;
    }
    if (abnormalExit == true) {
        // PROBABLE HACKING ATTEMPT DETECTED
        restartDesktopLockProcess();
        mState = Waiting;
        TQTimer::singleShot( 100, this, SLOT(recoverFromHackingAttempt()) );
    }
    else {
        // Restart the lock process
        restartDesktopLockProcess();
    }
}

void SaverEngine::slotLockProcessWaiting()
{
    // lockProcessWaiting cannot be called directly from a signal handler, as it will hang in certain obscure circumstances
    // Instead we use a single-shot timer to immediately call lockProcessWaiting once control has returned to the Qt main loop
    TQTimer::singleShot(0, this, SLOT(lockProcessWaiting()));
}

void SaverEngine::slotLockProcessFullyActivated()
{
    mState = Saving;

    if( systemdSession && systemdSession->canSend() ) {
	TQValueList<TQT_DBusData> params;
	params << TQT_DBusData::fromBool(true);
	TQT_DBusMessage reply = systemdSession->sendWithReply("SetIdleHint", params);
    }
}

void SaverEngine::slotLockProcessReady()
{
    mSaverProcessReady = true;
}

void SaverEngine::lockProcessWaiting()
{
    kdDebug(1204) << "SaverEngine: lock exited" << endl;
    if (trinity_lockeng_sak_available == TRUE) {
        handleSecureDialog();
    }
    if( mState == Waiting )
	return;
    emitDCOPSignal("KDE_stop_screensaver()", TQByteArray());
    if (mEnabled)
    {
        if (mXAutoLock)
        {
            mXAutoLock->start();
        }
        XForceScreenSaver(tqt_xdisplay(), ScreenSaverReset );
        XSetScreenSaver(tqt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
    }
    processLockTransactions();
    mState = Waiting;

    if( systemdSession && systemdSession->canSend() ) {
	TQValueList<TQT_DBusData> params;
	params << TQT_DBusData::fromBool(false);
	TQT_DBusMessage reply = systemdSession->sendWithReply("SetIdleHint", params);
    }
}

//---------------------------------------------------------------------------
//
// XAutoLock has detected the required idle time.
//
void SaverEngine::idleTimeout()
{
    // disable X screensaver
    XForceScreenSaver(tqt_xdisplay(), ScreenSaverReset );
    XSetScreenSaver(tqt_xdisplay(), 0, mXInterval, PreferBlanking, DontAllowExposures);
    startLockProcess( DefaultLock );
}

xautolock_corner_t SaverEngine::applyManualSettings(int action)
{
	if (action == 0)
	{
		kdDebug() << "no lock" << endl;
		return ca_nothing;
	}
	else
	if (action == 1)
	{
		kdDebug() << "lock screen" << endl;
		return ca_forceLock;
	}
	else
	if (action == 2)
	{
		kdDebug() << "prevent lock" << endl;
		return ca_dontLock;
	}
	else
	{
		kdDebug() << "no lock nothing" << endl;
		return ca_nothing;
	}
}

/*!
 * This function try a reconnect to D-Bus.
 * \return boolean with the result of the operation
 * \retval true if successful reconnected to D-Bus
 * \retval false if unsuccessful
 */
bool SaverEngine::dBusReconnect() {
	// close D-Bus connection
	dBusClose();
	// init D-Bus conntection
	return (dBusConnect());
}

/*!
 * This function is used to close D-Bus connection.
 */
void SaverEngine::dBusClose() {
	if( dBusConn.isConnected() ) {
		if( dBusLocal ) {
			delete dBusLocal;
			dBusLocal = 0;
		}
		if( dBusWatch ) {
			delete dBusWatch;
			dBusWatch = 0;
		}
		if( systemdSession ) {
			delete systemdSession;
			systemdSession = 0;
		}
	}
	dBusConn.closeConnection(DBUS_CONN_NAME);
}

/*!
 * This function is used to connect to D-Bus.
 */
bool SaverEngine::dBusConnect() {
	dBusConn = TQT_DBusConnection::addConnection(TQT_DBusConnection::SystemBus, DBUS_CONN_NAME);
	if( !dBusConn.isConnected() ) {
	    kdError() << "Failed to open connection to system message bus: " << dBusConn.lastError().message() << endl;
	    TQTimer::singleShot(4000, this, TQT_SLOT(dBusReconnect()));
	    return false;
	}

	// watcher for Disconnect signal
	dBusLocal = new TQT_DBusProxy(DBUS_SERVICE_DBUS, DBUS_PATH_LOCAL, DBUS_INTERFACE_LOCAL, dBusConn);
	TQObject::connect(dBusLocal, TQT_SIGNAL(dbusSignal(const TQT_DBusMessage&)),
			  this, TQT_SLOT(handleDBusSignal(const TQT_DBusMessage&)));

	// watcher for NameOwnerChanged signals
	dBusWatch = new TQT_DBusProxy(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, dBusConn);
	TQObject::connect(dBusWatch, TQT_SIGNAL(dbusSignal(const TQT_DBusMessage&)),
			  this, TQT_SLOT(handleDBusSignal(const TQT_DBusMessage&)));

	// find already running SystemD
	TQT_DBusProxy checkSystemD(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, dBusConn);
	if( checkSystemD.canSend() ) {
		TQValueList<TQT_DBusData> params;
		params << TQT_DBusData::fromString(SYSTEMD_LOGIN1_SERVICE);
		TQT_DBusMessage reply = checkSystemD.sendWithReply("NameHasOwner", params);
		if (reply.type() == TQT_DBusMessage::ReplyMessage && reply.count() == 1 && reply[0].toBool() ) {
			onDBusServiceRegistered(SYSTEMD_LOGIN1_SERVICE);
		}
	}
	return true;
}

/*!
 * This function handles D-Bus service registering
 */
void SaverEngine::onDBusServiceRegistered(const TQString& service) {
	if( service == SYSTEMD_LOGIN1_SERVICE ) {
		// get current systemd session
		TQT_DBusProxy managerIface(SYSTEMD_LOGIN1_SERVICE, SYSTEMD_LOGIN1_PATH, SYSTEMD_LOGIN1_MANAGER_IFACE, dBusConn);
		TQT_DBusObjectPath systemdSessionPath = TQT_DBusObjectPath();
		if( managerIface.canSend() ) {
			TQValueList<TQT_DBusData> params;
			params << TQT_DBusData::fromUInt32( getpid() );
			TQT_DBusMessage reply = managerIface.sendWithReply("GetSessionByPID", params);
			if (reply.type() == TQT_DBusMessage::ReplyMessage && reply.count() == 1 ) {
				systemdSessionPath = reply[0].toObjectPath();
			}
		}
		// wather for systemd session signals
		if( systemdSessionPath.isValid() ) {
			systemdSession = new TQT_DBusProxy(SYSTEMD_LOGIN1_SERVICE, systemdSessionPath, SYSTEMD_LOGIN1_SESSION_IFACE, dBusConn);
			TQObject::connect(systemdSession, TQT_SIGNAL(dbusSignal(const TQT_DBusMessage&)),
					  this, TQT_SLOT(handleDBusSignal(const TQT_DBusMessage&)));
		}
		return;
	}
}

/*!
 * This function handles D-Bus service unregistering
 */
void SaverEngine::onDBusServiceUnregistered(const TQString& service) {
	if( service == SYSTEMD_LOGIN1_SERVICE ) {
		if( systemdSession ) {
			delete systemdSession;
			systemdSession = 0;
		}
		return;
	}
}

/*!
 * This function handles signals from the D-Bus daemon.
 */
void SaverEngine::handleDBusSignal(const TQT_DBusMessage& msg) {
	// dbus terminated
	if( msg.path() == DBUS_PATH_LOCAL
	 && msg.interface() == DBUS_INTERFACE_LOCAL
	 && msg.member() == "Disconnected" ) {
		dBusClose();
		TQTimer::singleShot(1000, this, TQT_SLOT(dBusReconnect()));
		return;
	}

	// service registered / unregistered
	if( msg.path() == DBUS_PATH_DBUS
	 && msg.interface() == DBUS_INTERFACE_DBUS
	 && msg.member() == "NameOwnerChanged" ) {
		if( msg[1].toString().isEmpty() ) {
			// old-owner is empty
			onDBusServiceRegistered(msg[0].toString());
		}
		if( msg[2].toString().isEmpty() ) {
			// new-owner is empty
			onDBusServiceUnregistered(msg[0].toString());
		}
		return;
	}

	// systemd signal Lock()
	if( systemdSession && systemdSession->canSend()
	 && msg.path() == systemdSession->path()
	 && msg.interface() == SYSTEMD_LOGIN1_SESSION_IFACE
	 && msg.member() == "Lock") {
		lockScreen();
		return;
	}

	// systemd signal Unlock()
	if( systemdSession && systemdSession->canSend()
	 && msg.path() == systemdSession->path()
	 && msg.interface() == SYSTEMD_LOGIN1_SESSION_IFACE
	 && msg.member() == "Unlock") {
		// unlock?
		return;
	}
}

void SaverEngine::waitForLockEngage() {
	sigset_t new_mask;
	sigset_t orig_mask;

	// wait for SIGUSR1, SIGUSR2, SIGTTIN
	sigemptyset(&new_mask);
	sigaddset(&new_mask, SIGUSR1);
	sigaddset(&new_mask, SIGUSR2);
	sigaddset(&new_mask, SIGTTIN);

	sigprocmask(SIG_BLOCK, &new_mask, &orig_mask);
	while ((mState != Waiting) && (mState != Saving)) {
		sigsuspend(&orig_mask);
	}
	sigprocmask(SIG_UNBLOCK, &new_mask, NULL);
}