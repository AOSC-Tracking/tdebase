//
// This file is part of the TDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//

#ifndef __LOCKENG_H__
#define __LOCKENG_H__

#include <tqthread.h>
#include <kprocess.h>
#include <tqvaluevector.h>
#include "KScreensaverIface.h"
#include "xautolock.h"
#include "xautolock_c.h"

#include <tqdbusconnection.h>

#ifdef WITH_TDEHWLIB
class TDECryptographicCardDevice;
#else
#define TDECryptographicCardDevice void
#endif

/**
 * Screen saver engine. Handles communication with the lock process.
 * The engine is split into two parts, the 'SaverEngine' running in the GUI thread and
 * the 'SaverEngineEventHandler' running in a separate thread and eventloop.
 * The 'SaverEngine' handles communication with X11, DCOP and DBUS while the
 * 'SaverEngineEventHandler' handles communication with the actual lock process.
 * Several actions require cooperation of the two parts, so in various methods
 * there will be inter-thread calls (using timers or by emitting signals) to
 * trigger the other side remaining logic.
 * This complex design is necessary to avoid blocking the main GUI application event loop,
 * which has several tasks to manage and therefore can't affort to wait in a suspended state.
 * This was previously leading to deadlock when DCOP calls where executed on the secondary
 * thread/eventloop, for example when changing desktop while the lock process was restarting.
 */

class DCOPClientTransaction;
class TQT_DBusMessage;
class TQT_DBusProxy;
class SaverEngineEventHandler;

// Type of lock screen
enum LockType : int
{
	DontLock = 0,
	DefaultLock,
	ForceLock,
	SecureDialog
};

enum SaverState
{
	Waiting,
	Preparing,
	Engaging,
	Saving
};

class SaverEngine : public TQObject, public KScreensaverIface
{
	friend class SaverEngineEventHandler;

	TQ_OBJECT
public:
	SaverEngine();
	~SaverEngine();

	/**
	 * Lock the screen
	 */
	virtual void lock();

	/**
	 * Save the screen
	 */
	virtual void save();

	/**
	 * Quit the screensaver if running
	 */
	virtual void quit();

	/**
	 * return true if the screensaver is enabled
	 */
	virtual bool isEnabled();

	/**
	 * enable/disable the screensaver
	 */
	virtual bool enable( bool e );

	/**
	 * return true if the screen is currently blanked
	 */
	virtual bool isBlanked();

	/**
	 * Read and apply configuration.
	 */
	virtual void configure();

	/**
	 * Enable or disable "blank only" mode.  This is useful for
	 * laptops where one might not want a cpu thirsty screensaver
	 * draining the battery.
	 */
	virtual void setBlankOnly( bool blankOnly );

	/**
	 * Called by kdesktop_lock when locking is in effect.
	 */
	virtual void saverLockReady();

	void lockScreen(bool DCOP = false);

	void lockScreenAndDoNewSession();
	void lockScreenAndSwitchSession(int vt);

	void enableExports(); // Enable wallpaper exports

signals:
	void activateSaverOrLockSignal(LockType lock_type);
	void lockScreenSignal(bool);
	void terminateEventHandlerThread();

public slots:
	void handleDBusSignal(const TQT_DBusMessage&);
	void terminateTDESession();

protected slots:
	void idleTimeout();

private slots:
	void cryptographicCardInserted(TDECryptographicCardDevice*);
	void cryptographicCardRemoved(TDECryptographicCardDevice*);
	void cardStartupTimeout();
	bool dBusReconnect();

	// The following slots are invoked by corresponding methods named without the 'GUI' suffix
	// in 'SaverEngineEventHandler' to complete the remaining X11 part of the actions
	void activateSaverOrLockGUI();
	void lockProcessFullyActivatedGUI();
	void lockScreenGUI();
	void stopLockProcessGUI();

private:
	void dBusClose();
	bool dBusConnect();
	void onDBusServiceRegistered(const TQString&);
	void onDBusServiceUnregistered(const TQString&);

protected:
	void processLockTransactions();
	xautolock_corner_t applyManualSettings(int);

protected:
	bool mEnabled;

	XAutoLock *mXAutoLock;
	int mTimeout;

	// the original X screensaver parameters
	int mXTimeout;
	int mXInterval;
	int mXBlanking;
	int mXExposures;

	TQValueVector< DCOPClientTransaction* > mLockTransactions;

public:
	bool mBlankOnly;  // only use the blanker, not the defined saver // protected
	SaverEngineEventHandler *m_saverEngineEventHandler;

private:
	TQEventLoopThread* m_eventHandlerThread;
	bool mNewVTAfterLockEngage;
	bool mValidCryptoCardInserted;
	int mSwitchVTAfterLockEngage;
	struct sigaction mSignalAction;
	TQT_DBusConnection dBusConn;
	TQT_DBusProxy *dBusLocal;
	TQT_DBusProxy *dBusWatch;
	TQT_DBusProxy *systemdSession;
};

class SaverEngineEventHandler : public TQObject
{
	TQ_OBJECT
	
public:
	SaverEngineEventHandler(SaverEngine *engine);

	SaverState getState() const { return m_state; }

	void lockProcessExited();
	void lockProcessFullyActivated();
	void lockProcessReady();
	void terminateLockProcess();

public slots:
	bool activateSaverOrLock(LockType lock_type);
	void lockScreen(bool DCOP = false);
	bool restartLockProcess();
	void saveScreen();
	void stopLockProcess();
	void terminateThread();

protected slots:
	void slotLockProcessExited();
	void slotSAKProcessExited();

protected:
	void startSAKProcess();

	bool m_saverProcessReady;
	bool m_lockProcessRestarting;
	bool m_terminationRequest;
	SaverState m_state;
	SaverEngine *m_saverEngine;
	TDEProcess *m_SAKProcess;
	TDEProcess m_lockProcess;
};

#endif

