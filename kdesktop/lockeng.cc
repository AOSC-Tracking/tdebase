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
#include <kapplication.h>
#include <kservicegroup.h>
#include <kdebug.h>
#include <klocale.h>
#include <tqfile.h>
#include <tqtimer.h>
#include <dcopclient.h>
#include <assert.h>

#include "lockeng.h"
#include "lockeng.moc"
#include "kdesktopsettings.h"

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
      mTerminationRequested(false)
{
    struct sigaction act;

    // handle SIGUSR1
    m_masterSaverEngine = this;
    act.sa_handler= sigusr1_handler;
    sigemptyset(&(act.sa_mask));
    sigaddset(&(act.sa_mask), SIGUSR1);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, 0L);

    // handle SIGUSR2
    m_masterSaverEngine = this;
    act.sa_handler= sigusr2_handler;
    sigemptyset(&(act.sa_mask));
    sigaddset(&(act.sa_mask), SIGUSR2);
    act.sa_flags = 0;
    sigaction(SIGUSR2, &act, 0L);

    // Save X screensaver parameters
    XGetScreenSaver(qt_xdisplay(), &mXTimeout, &mXInterval,
                    &mXBlanking, &mXExposures);

    mState = Waiting;
    mXAutoLock = 0;
    mEnabled = false;

    connect(&mLockProcess, TQT_SIGNAL(processExited(KProcess *)),
                        TQT_SLOT(lockProcessExited()));

    mSAKProcess = new KProcess;
    *mSAKProcess << "kdmtsak";
    connect(mSAKProcess, TQT_SIGNAL(processExited(KProcess*)), this, TQT_SLOT(slotSAKProcessExited()));

    TQTimer::singleShot( 0, this, TQT_SLOT(handleSecureDialog()) );

    configure();

    mLockProcess.clearArguments();
    TQString path = KStandardDirs::findExe( "kdesktop_lock" );
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

    // Restore X screensaver parameters
    XSetScreenSaver(qt_xdisplay(), mXTimeout, mXInterval, mXBlanking,
                    mXExposures);
}

//---------------------------------------------------------------------------

// This should be called only using DCOP.
void SaverEngine::lock()
{
    bool ok = true;
    if (mState != Saving)
    {
        mSAKProcess->kill(SIGTERM);
        ok = startLockProcess( ForceLock );
        // It takes a while for kdesktop_lock to start and lock the screen.
        // Therefore delay the DCOP call until it tells kdesktop that the locking is in effect.
        // This is done only for --forcelock .
        if( ok && mState != Saving )
        {
            DCOPClientTransaction* trans = kapp->dcopClient()->beginTransaction();
            mLockTransactions.append( trans );
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
    if( mState != Preparing )
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
        mSAKProcess->kill(SIGTERM);
        startLockProcess( DefaultLock );
    }
}

//---------------------------------------------------------------------------
void SaverEngine::quit()
{
    if (mState == Saving || mState == Preparing)
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
        XSetScreenSaver(qt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
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

        XForceScreenSaver(qt_xdisplay(), ScreenSaverReset );
        XSetScreenSaver(qt_xdisplay(), 0, mXInterval,  PreferBlanking, DontAllowExposures);
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
	int screen_number = DefaultScreen(qt_xdisplay());
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
	// FIXME: if running, stop  and restart?  What about security
	// implications of this?
}

//---------------------------------------------------------------------------
//
// Start the screen saver.
//
bool SaverEngine::startLockProcess( LockType lock_type )
{
    if (mState == Saving)
        return true;

    enableExports();

    kdDebug(1204) << "SaverEngine: starting saver" << endl;
    emitDCOPSignal("KDE_start_screensaver()", TQByteArray());

    if (!mLockProcess.isRunning()) {
	mLockProcess.clearArguments();
	TQString path = KStandardDirs::findExe( "kdesktop_lock" );
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

    mLockProcess.kill(SIGTTOU);			// Start lock
    XSetScreenSaver(qt_xdisplay(), 0, mXInterval,  PreferBlanking, mXExposures);

    mState = Preparing;
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
        XForceScreenSaver(qt_xdisplay(), ScreenSaverReset );
        XSetScreenSaver(qt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
    }
    processLockTransactions();
    mState = Waiting;
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
    else {
        // Restart the lock process
        if (!mLockProcess.isRunning()) {
            mLockProcess.clearArguments();
            TQString path = KStandardDirs::findExe( "kdesktop_lock" );
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
        }
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
        XForceScreenSaver(qt_xdisplay(), ScreenSaverReset );
        XSetScreenSaver(qt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
    }
    processLockTransactions();
    mState = Waiting;
}

//---------------------------------------------------------------------------
//
// XAutoLock has detected the required idle time.
//
void SaverEngine::idleTimeout()
{
    // disable X screensaver
    XForceScreenSaver(qt_xdisplay(), ScreenSaverReset );
    XSetScreenSaver(qt_xdisplay(), 0, mXInterval, PreferBlanking, DontAllowExposures);
    mSAKProcess->kill(SIGTERM);
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
