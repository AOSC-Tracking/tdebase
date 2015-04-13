//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
// Copyright (c) 2003 Oswald Buddenhagen <ossi@kde.org>
// Copyright (c) 2010-2013 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

#ifndef __LOCKENG_H__
#define __LOCKENG_H__

#include <kgreeterplugin.h>

#include <kprocess.h>
#include <kpixmap.h>
#include <krootpixmap.h>

#include <tqwidget.h>
#include <tqtimer.h>
#include <tqvaluestack.h>
#include <tqmessagebox.h>
#include <tqpixmap.h>
#include <tqdatetime.h>
#include <tqthread.h>

#include <X11/Xlib.h>

class KLibrary;
class KWinModule;
class KSMModalDialog;
class LockProcess;

struct GreeterPluginHandle {
    KLibrary *library;
    kgreeterplugin_info *info;
};

#define FIFO_DIR "/tmp/tdesocket-global"
#define FIFO_FILE "/tmp/tdesocket-global/kdesktoplockcontrol-%d"
#define FIFO_FILE_OUT "/tmp/tdesocket-global/kdesktoplockcontrol_out-%d"

typedef TQValueList<Window> TQXLibWindowList;

//===========================================================================
//
// Control pipe handler
//
class ControlPipeHandlerObject : public TQObject
{
	Q_OBJECT

	public:
		ControlPipeHandlerObject();
		~ControlPipeHandlerObject();

	public slots:
		void run();
		void terminateThread();
	
	signals:
		void processCommand(TQString);

	public:
		LockProcess* mParent;

	private:
		bool mRunning;
		bool mTerminate;
		pthread_t mThreadID;
};

//===========================================================================
//
// Screen saver handling process.  Handles screensaver window,
// starting screensaver hacks, and password entry.
//
class LockProcess
    : public TQWidget
{
    Q_OBJECT
public:
    LockProcess();
    ~LockProcess();

    void init(bool child_saver = false, bool useBlankOnly = false);

    bool lock();

    bool defaultSave();

    bool dontLock();

    bool runSecureDialog();
    bool inSecureDialog();

    void setChildren(TQValueList<int> children) { child_sockets = children; }
    void setParent(int fd) { mParent = fd; }

    void msgBox( TQMessageBox::Icon type, const TQString &txt );
    int execDialog( TQDialog* dlg );

signals:
    void terminateHelperThread();

public slots:
    void quitSaver();
    void preparePopup();
    void cleanupPopup();
    void desktopResized();
    void doDesktopResizeFinish();
    void doFunctionKeyBroadcast();
    void slotPaintBackground(const TQPixmap &pm);
    void slotForcePaintBackground();

protected:
    virtual bool x11Event(XEvent *);
    virtual void timerEvent(TQTimerEvent *);
    virtual void resizeEvent(TQResizeEvent *);

private slots:
    void hackExited(TDEProcess *);
    void signalPipeSignal();
    bool startLock();
    void suspend();
    void checkDPMSActive();
    void slotDeadTimePassed();
    void windowAdded( WId );
    void resumeUnforced();
    void displayLockDialogIfNeeded();
    void closeDialogAndStartHack();
    bool closeCurrentWindow();
    void repaintRootWindowIfNeeded();
    void startSecureDialog();
    void slotMouseActivity(XEvent *event);
    void processInputPipeCommand(TQString command);

private:
    void configure();
    void readSaver();
    void createSaverWindow();
    void hideSaverWindow();
    void saveVRoot();
    void setVRoot(Window win, Window rw);
    void removeVRoot(Window win);
    void setTransparentBackgroundARGB();
    bool grabKeyboard();
    bool grabMouse();
    bool grabInput();
    void ungrabInput();
    void cantLock(const TQString &reason);
    bool startSaver(bool notify_ready = false);
    void stopSaver();
    bool startHack();
    void stopHack();
    void setupSignals();
    bool checkPass();
    void stayOnTop();
    void lockXF86();
    void unlockXF86();
    void showVkbd();
    void hideVkbd();
    void saverReady();
    void saverReadyIfNeeded();
    bool forwardVkbdEvent( XEvent* event );
    void sendVkbdFocusInOut( WId window, Time t );
    void windowAdded( WId window, bool managed );
    void resume( bool force );
    static TQVariant getConf(void *ctx, const char *key, const TQVariant &dflt);
    void generateBackingImages();
    void fullyOnline();

    bool        mLocked;
    int         mLockGrace;
    int         mPriority;
    bool        mBusy;
    TDEProcess    mHackProc;
    int         mRootWidth;
    int         mRootHeight;
    TQString     mSaverExec;
    TQString     mSaver;
    bool        mOpenGLVisual;
    bool        child_saver;
    TQValueList<int> child_sockets;
    int         mParent;
    bool        mUseBlankOnly;
    bool        mShowLockDateTime;
    bool        mSuspended;
    TQTimer      mSuspendTimer;
    bool        mVisibility;
    TQTimer      mCheckDPMS;
    TQValueStack< TQWidget* > mDialogs;
    bool        mRestoreXF86Lock;
    bool        mForbidden;
    TQStringList mPlugins, mPluginOptions;
    TQString     mMethod;
    GreeterPluginHandle greetPlugin;
    TQPixmap     mSavedScreen;
    int         mAutoLogoutTimerId;
    int         mAutoLogoutTimeout;
    bool        mAutoLogout;

    TQTimer      *resizeTimer;
    unsigned int  mkeyCode;

    TQTimer      *hackResumeTimer;

    TDEProcess*   mVkbdProcess;
    KWinModule* mKWinModule;
    struct VkbdWindow
        {
        WId id;
        TQRect rect;
        };
    TQValueList< VkbdWindow > mVkbdWindows;
    WId         mVkbdLastEventWindow;

    bool        mPipeOpen;
    int         mPipe_fd;
    bool        mPipeOpen_out;
    int         mPipe_fd_out;

    bool        mInfoMessageDisplayed;
    bool        mDialogControlLock;
    bool        mForceReject;
    TQDialog     *currentDialog;

    TQTimer*    mEnsureScreenHiddenTimer;
    TQTimer*    mForceContinualLockDisplayTimer;
    TQTimer*    mEnsureVRootWindowSecurityTimer;
    TQTimer*    mHackDelayStartupTimer;

    int         mHackDelayStartupTimeout;
    bool        mHackStartupEnabled;
    bool        mOverrideHackStartupEnabled;
    bool        mResizingDesktopLock;
    bool        mFullyOnlineSent;

    bool        mClosingWindows;
    bool        mInSecureDialog;
    bool        mHackActive;

    TQPixmap    backingPixmap;
    KRootPixmap  *m_rootPixmap;
    int         mBackingStartupDelayTimer;
    TQPixmap    mArgbTransparentBackgroundPixmap;

    KSMModalDialog* m_startupStatusDialog;

    TQDateTime mlockDateTime;

    bool m_mouseDown;
    int m_mousePrevX;
    int m_mousePrevY;
    int m_dialogPrevX;
    int m_dialogPrevY;

    bool m_notifyReadyRequested;

    TQWidget* m_maskWidget;
    Window m_saverRootWindow;

    ControlPipeHandlerObject* mControlPipeHandler;
    TQEventLoopThread*        mControlPipeHandlerThread;

    friend class ControlPipeHandlerObject;
};

#endif

