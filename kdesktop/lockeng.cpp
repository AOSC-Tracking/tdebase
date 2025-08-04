//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
// Copyright (c) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

#include <config.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <tdeglobal.h>

#ifdef WITH_TDEHWLIB
#include <ksslcertificate.h>
#include <kuser.h>
#include <tdehardwaredevices.h>
#include <tdecryptographiccarddevice.h>
#endif

#include <tdestandarddirs.h>
#include <tdeapplication.h>
#include <kservicegroup.h>
#include <tdesimpleconfig.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <tqfile.h>
#include <tqtimer.h>
#include <tqeventloop.h>
#include <dcopclient.h>
#include <assert.h>

#include <dmctl.h>

#include <dbus/dbus-shared.h>
#include <tqdbusdata.h>
#include <tqdbuserror.h>
#include <tqdbusmessage.h>
#include <tqdbusobjectpath.h>
#include <tqdbusproxy.h>

#include "lockeng.h"
#include "lockeng.moc"
#include "kdesktopsettings.h"

#include "xautolock_c.h"

#define SYSTEMD_LOGIN1_SERVICE       "org.freedesktop.login1"
#define SYSTEMD_LOGIN1_MANAGER_IFACE "org.freedesktop.login1.Manager"
#define SYSTEMD_LOGIN1_SESSION_IFACE "org.freedesktop.login1.Session"
#define SYSTEMD_LOGIN1_SEAT_IFACE    "org.freedesktop.login1.Seat"
#define SYSTEMD_LOGIN1_PATH          "/org/freedesktop/login1"

#define DBUS_CONN_NAME			"kdesktop_lock"

extern xautolock_corner_t xautolock_corners[ 4 ];
bool trinity_lockeng_sak_available = true;

SaverEngineEventHandler *gbl_saverEngineEventHandler = nullptr;

static void sigusr1_handler(int)
{
	if (gbl_saverEngineEventHandler)
	{
		gbl_saverEngineEventHandler->lockCompleted();
	}
}

static void sigusr2_handler(int)
{
	if (gbl_saverEngineEventHandler)
	{
		gbl_saverEngineEventHandler->lockFullyActivated();
	}
}

static void sigttin_handler(int)
{
	if (gbl_saverEngineEventHandler)
	{
		gbl_saverEngineEventHandler->lockReady();
	}
}

SaverEngine::SaverEngine()
	: TQObject(),
	  KScreensaverIface(),
	  mBlankOnly(false),
	  mNewVTAfterLockEngage(false),
	  mValidCryptoCardInserted(false),
	  mSwitchVTAfterLockEngage(-1),
	  dBusLocal(0),
	  dBusWatch(0),
	  systemdSession(0)
{
	// handle SIGUSR1
	mSignalAction.sa_handler= sigusr1_handler;
	sigemptyset(&(mSignalAction.sa_mask));
	sigaddset(&(mSignalAction.sa_mask), SIGUSR1);
	mSignalAction.sa_flags = 0;
	sigaction(SIGUSR1, &mSignalAction, 0L);

	// handle SIGUSR2
	mSignalAction.sa_handler= sigusr2_handler;
	sigemptyset(&(mSignalAction.sa_mask));
	sigaddset(&(mSignalAction.sa_mask), SIGUSR2);
	mSignalAction.sa_flags = 0;
	sigaction(SIGUSR2, &mSignalAction, 0L);

	// handle SIGTTIN (as custom user signal rather than its inherent meaning)
	mSignalAction.sa_handler= sigttin_handler;
	sigemptyset(&(mSignalAction.sa_mask));
	sigaddset(&(mSignalAction.sa_mask), SIGTTIN);
	mSignalAction.sa_flags = 0;
	sigaction(SIGTTIN, &mSignalAction, 0L);

	// Save X screensaver parameters
	XGetScreenSaver(tqt_xdisplay(), &mXTimeout, &mXInterval, &mXBlanking, &mXExposures);

	// Create event handler thread, event loop and object
	m_eventHandlerThread = new TQEventLoopThread;
	m_eventHandlerThread->start();
	m_saverEngineEventHandler = new SaverEngineEventHandler(this);
	gbl_saverEngineEventHandler = m_saverEngineEventHandler;
	m_saverEngineEventHandler->moveToThread(m_eventHandlerThread);
	connect(this, TQ_SIGNAL(terminateEventHandlerThread()), m_saverEngineEventHandler, TQ_SLOT(terminateThread()));
	connect(this, TQ_SIGNAL(lockScreenSignal(bool)), m_saverEngineEventHandler, TQ_SLOT(lockScreen(bool)));
	connect(this, TQ_SIGNAL(activateSaverOrLockSignal(LockType)),
	        m_saverEngineEventHandler, TQ_SLOT(activateSaverOrLock(LockType)));

	mXAutoLock = nullptr;
	mEnabled = false;

	configure();

	// Prevent kdesktop_lock signals from being handled by the main GUI thread.
	// Those signals will be handled by m_eventHandlerThread instead
	//
	// Make sure to keep this code after the constructor of `m_eventHandlerThread`, so that
	// the new thread starts with the signals unblocked.
	sigset_t sigBlockMask;
	sigemptyset(&sigBlockMask);
	sigaddset(&sigBlockMask, SIGUSR1);
	sigaddset(&sigBlockMask, SIGUSR2);
	sigaddset(&sigBlockMask, SIGTTIN);
	sigaddset(&sigBlockMask, SIGCHLD);
	pthread_sigmask(SIG_BLOCK, &sigBlockMask, NULL);

	// Start SAK and lock processes
	TQTimer::singleShot(0, m_saverEngineEventHandler, TQ_SLOT(restartLockProcess()));

#ifdef WITH_TDEHWLIB
	// Initialize SmartCard readers
	TDEGenericDevice *hwdevice;
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList cardReaderList = hwdevices->listByDeviceClass(TDEGenericDeviceType::CryptographicCard);
	for (hwdevice = cardReaderList.first(); hwdevice; hwdevice = cardReaderList.next())
	{
		TDECryptographicCardDevice *cdevice = static_cast<TDECryptographicCardDevice*>(hwdevice);
		connect(cdevice, TQ_SIGNAL(certificateListAvailable(TDECryptographicCardDevice*)),
		        this, TQ_SLOT(cryptographicCardInserted(TDECryptographicCardDevice*)));
		connect(cdevice, TQ_SIGNAL(cardRemoved(TDECryptographicCardDevice*)),
		        this, TQ_SLOT(cryptographicCardRemoved(TDECryptographicCardDevice*)));
		cdevice->enableCardMonitoring(true);
	}

	// Check card login status
	KUser userinfo;
	TQString fileName = userinfo.homeDir() + "/.tde_card_login_state";
	TQFile flagFile(fileName);
	if (flagFile.open(IO_ReadOnly))
	{
		TQTextStream stream(&flagFile);
		if (stream.readLine().startsWith("1"))
		{
			// Card was likely used to log in
			TQTimer::singleShot(5000, this, TQ_SLOT(cardStartupTimeout()));
		}
		flagFile.close();
	}
#endif

	dBusConnect();
}

SaverEngine::~SaverEngine()
{
	m_saverEngineEventHandler->terminateLockProcess();
	delete mXAutoLock;
	dBusClose();

	// Restore X screensaver parameters
	XSetScreenSaver(tqt_xdisplay(), mXTimeout, mXInterval, mXBlanking, mXExposures);
	emit terminateEventHandlerThread();
	m_eventHandlerThread->wait();
	delete m_saverEngineEventHandler;
	delete m_eventHandlerThread;
}

void SaverEngine::cardStartupTimeout()
{
	if (!mValidCryptoCardInserted)
	{
		configure();  // Restore saver timeout
		lockScreen(); // Force lock
	}
}

void SaverEngine::cryptographicCardInserted(TDECryptographicCardDevice* cdevice)
{
#ifdef WITH_TDEHWLIB
	TQString login_name = TQString::null;
	X509CertificatePtrList certList = cdevice->cardX509Certificates();
	if (certList.count() > 0)
	{
		KSSLCertificate* card_cert = NULL;
		card_cert = KSSLCertificate::fromX509(certList[0]);
		TQStringList cert_subject_parts = TQStringList::split("/", card_cert->getSubject(), false);
		for (TQStringList::Iterator it = cert_subject_parts.begin(); it != cert_subject_parts.end(); ++it)
		{
			TQString lcpart = (*it).lower();
			if (lcpart.startsWith("cn="))
			{
				login_name = lcpart.right(lcpart.length() - strlen("cn="));
			}
		}
		delete card_cert;
	}

	if (login_name != "")
	{
		KUser user;
		if (login_name == user.loginName())
		{
			mValidCryptoCardInserted = true;
		}
	}
#endif
}

void SaverEngine::cryptographicCardRemoved(TDECryptographicCardDevice* cdevice)
{
#ifdef WITH_TDEHWLIB
	if (mValidCryptoCardInserted)
	{
		mValidCryptoCardInserted = false;

		// Restore saver timeout
		configure();

		// Force lock
		lockScreen();
	}
#endif
}

// DCOP interface method
void SaverEngine::lock()
{
	lockScreen(true);
}

void SaverEngine::lockScreen(bool dcop)
{
	if (mValidCryptoCardInserted)
	{
		kdDebug(1204) << "SaverEngine: crypto card inserted, ignore lock request" << endl;
		return;
	}
	emit lockScreenSignal(dcop);
}

void SaverEngine::lockScreenGUI()
{
	DCOPClientTransaction *trans = tdeApp->dcopClient()->beginTransaction();
	if (trans)
	{
		mLockTransactions.append(trans);
	}
}

void SaverEngine::processLockTransactions()
{
	TQValueVector<DCOPClientTransaction*>::ConstIterator it = mLockTransactions.begin();
	for (; it != mLockTransactions.end(); ++it)
	{
		TQCString replyType = "void";
		TQByteArray arr;
		tdeApp->dcopClient()->endTransaction(*it, replyType, arr);
	}
	mLockTransactions.clear();
}

void SaverEngine::saverLockReady()
{
	if (m_saverEngineEventHandler->getState() != SaverState::Engaging)
	{
		kdDebug(1204) << "Got unexpected saverLockReady()" << endl;
	}

	kdDebug(1204) << "Saver Lock Ready" << endl;
	processLockTransactions();
}

// DCOP interface method
void SaverEngine::save()
{
	if (mValidCryptoCardInserted)
	{
		kdDebug(1204) << "SaverEngine: crypto card inserted, ignore save request" << endl;
		return;
	}
	TQTimer::singleShot(0, m_saverEngineEventHandler, TQ_SLOT(saveScreen()));
}

// DCOP interface method
void SaverEngine::quit()
{
	TQTimer::singleShot(0, m_saverEngineEventHandler, TQ_SLOT(stopLockProcess()));
}

// DCOP interface method
bool SaverEngine::isEnabled()
{
	return mEnabled;
}

// DCOP interface method
bool SaverEngine::enable(bool e)
{
	if (e == mEnabled)
		return true;

	// If we aren't in a suitable state, we will not reconfigure.
	if (m_saverEngineEventHandler->getState() != SaverState::Waiting)
		return false;

	mEnabled = e;

	if (mEnabled)
	{
		if (!mXAutoLock)
		{
			mXAutoLock = new XAutoLock();
			connect(mXAutoLock, TQ_SIGNAL(timeout()), TQ_SLOT(idleTimeout()));
		}
		mXAutoLock->setTimeout(mTimeout);
		mXAutoLock->setDPMS(true);
	
		// We'll handle blanking
		XSetScreenSaver(tqt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
		mXAutoLock->start();
		kdDebug(1204) << "Saver engine started, timeout: " << mTimeout << endl;
	}
	else
	{
		if (mXAutoLock)
		{
			delete mXAutoLock;
			mXAutoLock = nullptr;
		}
	
		XForceScreenSaver(tqt_xdisplay(), ScreenSaverReset);
		XSetScreenSaver(tqt_xdisplay(), 0, mXInterval,  PreferBlanking, DontAllowExposures);
		kdDebug(1204) << "Saver engine disabled" << endl;
	}

	return true;
}

// DCOP interface method
bool SaverEngine::isBlanked()
{
	return (m_saverEngineEventHandler->getState() != SaverState::Waiting);
}

void SaverEngine::enableExports()
{
#ifdef TQ_WS_X11
	kdDebug(270) << k_lineinfo << "activating background exports" << endl;
	DCOPClient *client = tdeApp->dcopClient();
	if (!client->isAttached())
	{
		client->attach();
	}
	TQByteArray data;
	TQDataStream args(data, IO_WriteOnly);
	args << 1;
	
	TQCString appname("kdesktop");
	int screen_number = DefaultScreen(tqt_xdisplay());
	if (screen_number)
	{
		appname.sprintf("kdesktop-screen-%d", screen_number);
	}
	
	client->send(appname, "KBackgroundIface", "setExport(int)", data);
#endif
}

// Read and apply configuration.
void SaverEngine::configure()
{
	// If we aren't in a suitable state, we will not reconfigure.
	if (m_saverEngineEventHandler->getState() != SaverState::Waiting)
	{
		return;
	}

	// create a new config obj to ensure we read the latest options
	KDesktopSettings::self()->readConfig();

	mTimeout = KDesktopSettings::timeout();
	bool e = KDesktopSettings::screenSaverEnabled();
	mEnabled = !e;   // enable the screensaver by forcibly toggling it
	enable(e);

	int action;
	action = KDesktopSettings::actionTopLeft();
	xautolock_corners[0] = applyManualSettings(action);
	action = KDesktopSettings::actionTopRight();
	xautolock_corners[1] = applyManualSettings(action);
	action = KDesktopSettings::actionBottomLeft();
	xautolock_corners[2] = applyManualSettings(action);
	action = KDesktopSettings::actionBottomRight();
	xautolock_corners[3] = applyManualSettings(action);
}

// DCOP interface method
// Set a variable to indicate only to blank the screen and not use the saver
void SaverEngine::setBlankOnly(bool blankOnly)
{
	mBlankOnly = blankOnly;
}

void SaverEngine::activateSaverOrLockGUI()
{
	XSetScreenSaver(tqt_xdisplay(), 0, mXInterval,  PreferBlanking, mXExposures);
	if (mXAutoLock)
	{
		mXAutoLock->stop();
	}
	emitDCOPSignal("KDE_start_screensaver()", TQByteArray());
}

void SaverEngine::stopLockProcessGUI()
{
	emitDCOPSignal("KDE_stop_screensaver()", TQByteArray());

	if (mEnabled)
	{
		if (mXAutoLock)
		{
			mXAutoLock->start();
		}
		XForceScreenSaver(tqt_xdisplay(), ScreenSaverReset);
		XSetScreenSaver(tqt_xdisplay(), mTimeout + 10, mXInterval, PreferBlanking, mXExposures);
	}
	processLockTransactions();

	if (systemdSession && systemdSession->canSend())
	{
		TQValueList<TQT_DBusData> params;
		params << TQT_DBusData::fromBool(false);
		TQT_DBusMessage reply = systemdSession->sendWithReply("SetIdleHint", params);
	}
}

void SaverEngine::terminateTDESession()
{
	// Terminate the TDE session ASAP!
	// Values are explained at http://lists.kde.org/?l=kde-linux&m=115770988603387
	TQByteArray data;
	TQDataStream arg(data, IO_WriteOnly);
	arg << (int)0 << (int)0 << (int)2;
	if (!tdeApp->dcopClient()->send("ksmserver", "default", "logout(int,int,int)", data))
	{
		// Someone got to DCOP before we did. Try an emergency system logout
		system("logout");
	}
}

void SaverEngine::lockProcessFullyActivatedGUI()
{
	if (systemdSession && systemdSession->canSend())
	{
		TQValueList<TQT_DBusData> params;
		params << TQT_DBusData::fromBool(true);
		TQT_DBusMessage reply = systemdSession->sendWithReply("SetIdleHint", params);
	}

	if (mNewVTAfterLockEngage)
	{
		DM().startReserve();
		mNewVTAfterLockEngage = false;
	}
	else if (mSwitchVTAfterLockEngage != -1)
	{
		DM().switchVT(mSwitchVTAfterLockEngage);
		mSwitchVTAfterLockEngage = -1;
	}
}

// XAutoLock has detected the required idle time.
void SaverEngine::idleTimeout()
{
	if (!mValidCryptoCardInserted)
	{
		// disable X screensaver
		XForceScreenSaver(tqt_xdisplay(), ScreenSaverReset);
		XSetScreenSaver(tqt_xdisplay(), 0, mXInterval, PreferBlanking, DontAllowExposures);
		emit activateSaverOrLockSignal(DefaultLock);
	}
}

xautolock_corner_t SaverEngine::applyManualSettings(int action)
{
	if (action == 0)
	{
		kdDebug() << "no lock" << endl;
		return ca_nothing;
	}
	else if (action == 1)
	{
		kdDebug() << "lock screen" << endl;
		return ca_forceLock;
	}
	else if (action == 2)
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

/*
 * This function try to reconnect to D-Bus.
 * \return boolean with the result of the operation
 * \retval true if successful reconnected to D-Bus
 * \retval false if unsuccessful
 */
bool SaverEngine::dBusReconnect()
{
	dBusClose();            // close D-Bus connection
	return (dBusConnect()); // init D-Bus conntection
} 

// This function is used to close D-Bus connection.
void SaverEngine::dBusClose()
{
	if (dBusConn.isConnected())
	{
		if (dBusLocal)
		{
			delete dBusLocal;
			dBusLocal = nullptr;
		}
		if (dBusWatch)
		{
			delete dBusWatch;
			dBusWatch = nullptr;
		}
		if (systemdSession)
		{
			delete systemdSession;
			systemdSession = nullptr;
		}
	}
	dBusConn.closeConnection(DBUS_CONN_NAME);
}

// This function is used to connect to D-Bus.
bool SaverEngine::dBusConnect()
{
	dBusConn = TQT_DBusConnection::addConnection(TQT_DBusConnection::SystemBus, DBUS_CONN_NAME);
	if (!dBusConn.isConnected())
	{
		kdError() << "Failed to open connection to system message bus: " << dBusConn.lastError().message() << endl;
		TQTimer::singleShot(4000, this, TQ_SLOT(dBusReconnect()));
		return false;
	}

	// watcher for Disconnect signal
	dBusLocal = new TQT_DBusProxy(DBUS_SERVICE_DBUS, DBUS_PATH_LOCAL, DBUS_INTERFACE_LOCAL, dBusConn);
	TQObject::connect(dBusLocal, TQ_SIGNAL(dbusSignal(const TQT_DBusMessage&)),
	        this, TQ_SLOT(handleDBusSignal(const TQT_DBusMessage&)));

	// watcher for NameOwnerChanged signals
	dBusWatch = new TQT_DBusProxy(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, dBusConn);
	TQObject::connect(dBusWatch, TQ_SIGNAL(dbusSignal(const TQT_DBusMessage&)),
	        this, TQ_SLOT(handleDBusSignal(const TQT_DBusMessage&)));

	// find already running SystemD
	TQT_DBusProxy checkSystemD(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, dBusConn);
	if (checkSystemD.canSend())
	{
		TQValueList<TQT_DBusData> params;
		params << TQT_DBusData::fromString(SYSTEMD_LOGIN1_SERVICE);
		TQT_DBusMessage reply = checkSystemD.sendWithReply("NameHasOwner", params);
		if (reply.type() == TQT_DBusMessage::ReplyMessage && reply.count() == 1 && reply[0].toBool())
		{
			onDBusServiceRegistered(SYSTEMD_LOGIN1_SERVICE);
		}
	}
	return true;
}

// This function handles D-Bus service registering
void SaverEngine::onDBusServiceRegistered(const TQString& service)
{
	if (service == SYSTEMD_LOGIN1_SERVICE)
	{
		// get current systemd session
		TQT_DBusProxy managerIface(SYSTEMD_LOGIN1_SERVICE, SYSTEMD_LOGIN1_PATH, SYSTEMD_LOGIN1_MANAGER_IFACE, dBusConn);
		TQT_DBusObjectPath systemdSessionPath = TQT_DBusObjectPath();
		if (managerIface.canSend())
		{
			TQValueList<TQT_DBusData> params;
			params << TQT_DBusData::fromUInt32(getpid());
			TQT_DBusMessage reply = managerIface.sendWithReply("GetSessionByPID", params);
			if (reply.type() == TQT_DBusMessage::ReplyMessage && reply.count() == 1)
			{
				systemdSessionPath = reply[0].toObjectPath();
			}
		}
		// wather for systemd session signals
		if (systemdSessionPath.isValid())
		{
			systemdSession = new TQT_DBusProxy(SYSTEMD_LOGIN1_SERVICE, systemdSessionPath, SYSTEMD_LOGIN1_SESSION_IFACE, dBusConn);
			TQObject::connect(systemdSession, TQ_SIGNAL(dbusSignal(const TQT_DBusMessage&)),
					  this, TQ_SLOT(handleDBusSignal(const TQT_DBusMessage&)));
		}
		return;
	}
}

// This function handles D-Bus service unregistering
void SaverEngine::onDBusServiceUnregistered(const TQString& service)
{
	if (service == SYSTEMD_LOGIN1_SERVICE)
	{
		if (systemdSession)
		{
			delete systemdSession;
			systemdSession = nullptr;
		}
		return;
	}
}

// This function handles signals from the D-Bus daemon.
void SaverEngine::handleDBusSignal(const TQT_DBusMessage& msg)
{
	// dbus terminated
	if (msg.path() == DBUS_PATH_LOCAL && msg.interface() == DBUS_INTERFACE_LOCAL &&
	        msg.member() == "Disconnected")
	{
		dBusClose();
		TQTimer::singleShot(1000, this, TQ_SLOT(dBusReconnect()));
		return;
	}

	// service registered / unregistered
	if (msg.path() == DBUS_PATH_DBUS && msg.interface() == DBUS_INTERFACE_DBUS &&
	        msg.member() == "NameOwnerChanged")
	{
		if (msg[1].toString().isEmpty())
		{
			// old-owner is empty
			onDBusServiceRegistered(msg[0].toString());
		}
		if (msg[2].toString().isEmpty())
		{
			// new-owner is empty
			onDBusServiceUnregistered(msg[0].toString());
		}
		return;
	}

	// systemd signal Lock()
	if (systemdSession && systemdSession->canSend() && msg.path() == systemdSession->path() &&
	        msg.interface() == SYSTEMD_LOGIN1_SESSION_IFACE && msg.member() == "Lock")
	{
		lockScreen();
		return;
	}

	// systemd signal Unlock()
	if (systemdSession && systemdSession->canSend() && msg.path() == systemdSession->path() &&
	        msg.interface() == SYSTEMD_LOGIN1_SESSION_IFACE && msg.member() == "Unlock")
	{
		// unlock?
		return;
	}
}

void SaverEngine::lockScreenAndDoNewSession()
{
	mNewVTAfterLockEngage = true;
	lockScreen();
}

void SaverEngine::lockScreenAndSwitchSession(int vt)
{
	mSwitchVTAfterLockEngage = vt;
	lockScreen();
}

SaverEngineEventHandler::SaverEngineEventHandler(SaverEngine *engine) :
        m_state(Waiting), m_saverProcessReady(false), m_lockProcessRestarting(false),
        m_terminationRequest(false), m_saverEngine(engine), m_SAKProcess(nullptr)
{
	connect(&m_lockProcess, TQ_SIGNAL(processExited(TDEProcess*)),
	        this, TQ_SLOT(slotLockProcessExited()));
}

void SaverEngineEventHandler::terminateLockProcess()
{
	if (m_state == Waiting)
	{
		kill(m_lockProcess.pid(), SIGKILL);
	}
	m_lockProcess.detach(); // don't kill it if we crash
}
											
void SaverEngineEventHandler::lockCompleted()
{
	kdDebug(1204) << "SaverEngineEventHandler: lock completed" << endl;

	if (m_state == Waiting)
	{
		return;
	}

	m_state = Waiting;
	if (trinity_lockeng_sak_available)
	{
		startSAKProcess();
	}
	TQTimer::singleShot(0, m_saverEngine, TQ_SLOT(stopLockProcessGUI()));
}

void SaverEngineEventHandler::lockFullyActivated()
{
	m_state = Saving;
	TQTimer::singleShot(0, m_saverEngine, TQ_SLOT(lockProcessFullyActivatedGUI()));
}

void SaverEngineEventHandler::lockReady()
{
	m_saverProcessReady = true;
}

void SaverEngineEventHandler::lockScreen(bool dcop)
{
	if (m_lockProcessRestarting)
	{
		kdDebug(1204) << "SaverEngineEventHandler: lock process is restarting, can't handle lock request" << endl;
		return;
	}

	bool ok = true;
	if (m_state == Waiting)
	{
		ok = activateSaverOrLock(ForceLock);
		// It takes a while for kdesktop_lock to start and lock the screen.
		// Therefore delay the DCOP call until it tells kdesktop that the locking is in effect.
		// This is done only for --forcelock .
		if (ok && m_state != Saving)
		{
			if (dcop)
			{
				TQTimer::singleShot(0, m_saverEngine, TQ_SLOT(lockScreenGUI()));
			}
		}
	}
}

void SaverEngineEventHandler::saveScreen()
{
	if (m_lockProcessRestarting)
	{
		kdDebug(1204) << "SaverEngineEventHandler: lock process is restarting, can't handle save request" << endl;
		return;
	}

	if (m_state == Waiting)
	{
		activateSaverOrLock(DefaultLock);
	}
}

void SaverEngineEventHandler::slotLockProcessExited()
{
	// Clean up status after the lock process has exited
	lockCompleted();

	m_lockProcessRestarting = true;

	bool abnormalExit = false;
	if (!m_lockProcess.normalExit())
	{
		abnormalExit = true;
	}
	else if (m_lockProcess.exitStatus() != 0)
	{
		abnormalExit = true;
	}
	if (m_terminationRequest)
	{
		abnormalExit = false;
		m_terminationRequest = false;
	}

	// Restart the lock process. This call blocks till
	// the lock process has restarted.
	restartLockProcess();

	if (abnormalExit)
	{
		// Possible hacking attempt detected, try to relaunch the saver with force lock
		m_state = Waiting;
		if (!activateSaverOrLock(ForceLock))
		{
			TQTimer::singleShot(0, m_saverEngine, TQ_SLOT(terminateTDESession()));
		}
	}
	m_lockProcessRestarting = false;
}

/*
 * Start or restart the lock process.
 * On the very first invocation, launch the SAK process if required and
 * auto lock the screen if the option has been enabled in the configuration.
 */
bool SaverEngineEventHandler::restartLockProcess()
{
	static bool firstStart = true;

	bool autoLoginEnable = false;
	bool autoLoginLocked = false;
	if (firstStart)
	{
		firstStart = false;

		// Create SAK process only if SAK is enabled
		struct stat st;
		TDESimpleConfig *config;
		if (stat(KDE_CONFDIR "/tdm/tdmdistrc" , &st) == 0)
		{
			config = new TDESimpleConfig(TQString::fromLatin1(KDE_CONFDIR "/tdm/tdmdistrc"));
		}
		else
		{
			config = new TDESimpleConfig(TQString::fromLatin1(KDE_CONFDIR "/tdm/tdmrc"));
		}
		config->setGroup("X-:*-Greeter");
		bool useSAKProcess = false;
#ifdef BUILD_TSAK
		useSAKProcess = config->readBoolEntry("UseSAK", false) && KDesktopSettings::useTDESAK();
#endif
		if (useSAKProcess)
		{
			startSAKProcess();
		}

		// autolock the desktop if required
		config->setGroup("X-:0-Core");
		autoLoginEnable = config->readBoolEntry("AutoLoginEnable", false);
		autoLoginLocked = config->readBoolEntry("AutoLoginLocked", false);
		delete config;
	}

	// (Re)start the lock process
	if (!m_lockProcess.isRunning())
	{
		m_lockProcess.clearArguments();
		TQString path = TDEStandardDirs::findExe("kdesktop_lock");
		if (path.isEmpty())
		{
			kdDebug(1204) << "Can't find kdesktop_lock!" << endl;
			return false;
		}
		m_lockProcess << path;
		m_lockProcess << TQString("--internal") << TQString("%1").arg(getpid());

		m_saverProcessReady = false;
		if (!m_lockProcess.start())
		{
			kdDebug(1204) << "Failed to start kdesktop_lock!" << endl;
			return false;
		}
		// Wait for the lock process to signal that it is ready
		sigset_t empty_mask;
		sigemptyset(&empty_mask);
		while (!m_saverProcessReady)
		{
			sigsuspend(&empty_mask);
		}
		if (!m_lockProcess.isRunning())
		{
			kdDebug(1204) << "Failed to initialize kdesktop_lock (unexpected termination)!" << endl;
			return false;
		}
	}

	if (autoLoginEnable && autoLoginLocked)
	{
		m_lockProcess.kill(SIGTTOU);
		m_lockProcess.kill(SIGUSR1);
	}
	return true;
}

// Start the screen saver or lock screen
bool SaverEngineEventHandler::activateSaverOrLock(LockType lock_type)
{
	if (m_state == Saving)
	{
		return true;
	}

	kdDebug(1204) << "SaverEngineEventHandler: starting saver" << endl;
	m_state = Preparing;
	if (m_SAKProcess)
	{
		m_SAKProcess->kill(SIGTERM);
	}

	m_saverEngine->enableExports();
	if (!restartLockProcess())
	{
		m_state = Waiting;
		return false;
	}

	switch (lock_type)
	{
		case ForceLock:
			m_lockProcess.kill(SIGUSR1);        // Request forcelock
			break;
		case DontLock:
			m_lockProcess.kill(SIGUSR2);        // Request dontlock
			break;
		case SecureDialog:
			m_lockProcess.kill(SIGWINCH);       // Request secure dialog
			break;
		default:
			break;
	}

	if (m_saverEngine->mBlankOnly)
	{
		m_lockProcess.kill(SIGTTIN);          // Request blanking
	}

	int ret = m_lockProcess.kill(SIGTTOU);  // Start lock
	if (!ret)
	{
		m_state = Waiting;
		return false;
	}
	m_state = Engaging;

	// Ask to the GUI thread to activate X11 saver
	TQTimer::singleShot(0, m_saverEngine, TQ_SLOT(activateSaverOrLockGUI()));

	return true;
}

// Stop the screen saver.
void SaverEngineEventHandler::stopLockProcess()
{
	if (m_state == Waiting)
	{
		return;
	}

	kdDebug(1204) << "SaverEngineEventHandler: stopping lock process" << endl;

	m_terminationRequest = true;
	m_lockProcess.kill();
	m_state = Waiting;

	// Ask to the GUI thread to stop the X11 saver
	TQTimer::singleShot(0, m_saverEngine, TQ_SLOT(stopLockProcessGUI()));
}

void SaverEngineEventHandler::startSAKProcess()
{
	if (!m_SAKProcess)
	{
		m_SAKProcess = new TDEProcess;
		*m_SAKProcess << "tdmtsak";
		connect(m_SAKProcess, TQ_SIGNAL(processExited(TDEProcess*)), this, TQ_SLOT(slotSAKProcessExited()));
	}
	if (!m_SAKProcess->isRunning())
	{
		m_SAKProcess->start();
	}
}

void SaverEngineEventHandler::slotSAKProcessExited()
{
	if (!m_SAKProcess)
	{
		tqWarning("[kdesktop] SAK process does not exist. Something went wrong. Ignoring.");
		return;
	}

	int retcode = m_SAKProcess->exitStatus();
	if (retcode && m_SAKProcess->normalExit())
	{
		trinity_lockeng_sak_available = false;
		tqWarning("[kdesktop] SAK driven secure dialog is not available for use (retcode %d). "
		          "Check tdmtsak for proper functionality.", retcode);
	}

	if (m_state == Preparing)
	{
		return;
	}

	if (m_SAKProcess->normalExit() && trinity_lockeng_sak_available)
	{
		if (m_state == Waiting)
		{
			activateSaverOrLock(SecureDialog);
		}
		else
		{
			m_lockProcess.kill(SIGHUP);
		}
	}
}

void SaverEngineEventHandler::terminateThread()
{
	TQEventLoop *eventLoop = TQApplication::eventLoop();
	if (eventLoop)
	{
		eventLoop->exit(0);
	}
}
