//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
// Copyright (c) 2003 Oswald Buddenhagen <ossi@kde.org>
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

#include <X11/Xlib.h>

class KLibrary;
class KWinModule;
class KSMModalDialog;

struct GreeterPluginHandle {
    KLibrary *library;
    kgreeterplugin_info *info;
};

#define FIFO_DIR "/tmp/ksocket-global"
#define FIFO_FILE "/tmp/ksocket-global/kdesktoplockcontrol"
#define FIFO_FILE_OUT "/tmp/ksocket-global/kdesktoplockcontrol_out"
#define PIPE_CHECK_INTERVAL 50

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
    LockProcess(bool child_saver = false, bool useBlankOnly = false);
    ~LockProcess();

    bool lock();

    bool defaultSave();

    bool dontLock();

    bool runSecureDialog();

    void setChildren(TQValueList<int> children) { child_sockets = children; }
    void setParent(int fd) { mParent = fd; }

    void msgBox( TQMessageBox::Icon type, const TQString &txt );
    int execDialog( TQDialog* dlg );

public slots:
    void quitSaver();
    void preparePopup();
    void cleanupPopup();
    void checkPipe();
    void desktopResized();
    void doDesktopResizeFinish();
    void doFunctionKeyBroadcast();
    void slotPaintBackground(const TQPixmap &pm);

protected:
    virtual bool x11Event(XEvent *);
    virtual void timerEvent(TQTimerEvent *);

private slots:
    void hackExited(KProcess *);
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

private:
    void configure();
    void readSaver();
    void createSaverWindow();
    void hideSaverWindow();
    void saveVRoot();
    void setVRoot(Window win, Window rw);
    void removeVRoot(Window win);
    bool grabKeyboard();
    bool grabMouse();
    bool grabInput();
    void ungrabInput();
    void cantLock(const TQString &reason);
    bool startSaver();
    void stopSaver();
    bool startHack();
    void stopHack();
    void setupSignals();
    void setupPipe();
    bool checkPass();
    void stayOnTop();
    void lockXF86();
    void unlockXF86();
    void showVkbd();
    void hideVkbd();
    bool forwardVkbdEvent( XEvent* event );
    void sendVkbdFocusInOut( WId window, Time t );
    void windowAdded( WId window, bool managed );
    void resume( bool force );
    static TQVariant getConf(void *ctx, const char *key, const TQVariant &dflt);

    bool        mLocked;
    int         mLockGrace;
    int         mPriority;
    bool        mBusy;
    KProcess    mHackProc;
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
    bool        mDPMSDepend;
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

    KProcess*   mVkbdProcess;
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

    TQTimer*    mForceContinualLockDisplayTimer;
    TQTimer*    mEnsureVRootWindowSecurityTimer;
    TQTimer*    mHackDelayStartupTimer;

    int         mHackDelayStartupTimeout;
    bool        mHackStartupEnabled;

    TQPixmap    backingPixmap;
    KRootPixmap  *m_rootPixmap;
    int         mBackingStartupDelayTimer;

    KSMModalDialog* m_startupStatusDialog;

    TQDateTime mlockDateTime;
};

#endif

