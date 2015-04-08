//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//

#ifndef __LOCKENG_H__
#define __LOCKENG_H__

#include <tqwidget.h>
#include <kprocess.h>
#include <tqvaluevector.h>
#include "KScreensaverIface.h"
#include "xautolock.h"
#include "xautolock_c.h"

#include <tqdbusconnection.h>

class DCOPClientTransaction;
class TQT_DBusMessage;
class TQT_DBusProxy;

//===========================================================================
/**
 * Screen saver engine.  Handles screensaver window, starting screensaver
 * hacks, and password entry.
 */
class SaverEngine : public TQWidget, public KScreensaverIface
{
	Q_OBJECT
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

	/**
	 * @internal
	 */
	void lockScreen(bool DCOP = false);

	/**
	 * Called by KDesktop to wait for saver engage
	 * @internal
	 */
	void waitForLockEngage();

public slots:
	void slotLockProcessWaiting();
	void slotLockProcessFullyActivated();
	void slotLockProcessReady();
	void handleDBusSignal(const TQT_DBusMessage&);

protected slots:
	void idleTimeout();
	void lockProcessExited();
	void lockProcessWaiting();

private slots:
	void handleSecureDialog();
	void slotSAKProcessExited();

	/**
	 * Enable wallpaper exports
	 */
	void enableExports();
	void recoverFromHackingAttempt();

	bool dBusReconnect();

private:
	bool restartDesktopLockProcess();
	void dBusClose();
	bool dBusConnect();
	void onDBusServiceRegistered(const TQString&);
	void onDBusServiceUnregistered(const TQString&);

protected:
	enum SaverState { Waiting, Preparing, Engaging, Saving };
	enum LockType { DontLock, DefaultLock, ForceLock, SecureDialog };
	bool startLockProcess( LockType lock_type );
	void stopLockProcess();
	bool handleKeyPress(XKeyEvent *xke);
	void processLockTransactions();
	xautolock_corner_t applyManualSettings(int);

protected:
	bool mEnabled;

	SaverState mState;
	XAutoLock *mXAutoLock;
	TDEProcess mLockProcess;
	int mTimeout;

	// the original X screensaver parameters
	int mXTimeout;
	int mXInterval;
	int mXBlanking;
	int mXExposures;

	bool mBlankOnly;  // only use the blanker, not the defined saver
	TQValueVector< DCOPClientTransaction* > mLockTransactions;

private:
	TDEProcess* mSAKProcess;
	bool mTerminationRequested;
	bool mSaverProcessReady;
	struct sigaction mSignalAction;
	TQT_DBusConnection dBusConn;
	TQT_DBusProxy* dBusLocal;
	TQT_DBusProxy* dBusWatch;
	TQT_DBusProxy* systemdSession;
};

#endif

