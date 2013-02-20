//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
// Copyright (c) 2003 Oswald Buddenhagen <ossi@kde.org>
// Copyright (c) 2010-2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

//kdesktop keeps running and checks user inactivity
//when it should show screensaver (and maybe lock the session),
//it starts kdesktop_lock, who does all the locking and who
//actually starts the screensaver

//It's done this way to prevent screen unlocking when kdesktop
//crashes (e.g. because it's set to multiple wallpapers and
//some image will be corrupted).

#define protected public
#include <tqwidget.h>
#undef protected

#include <config.h>

#include <execinfo.h>

#include "lockprocess.h"
#include "lockdlg.h"
#include "infodlg.h"
#include "querydlg.h"
#include "sakdlg.h"
#include "securedlg.h"
#include "autologout.h"
#include "kdesktopsettings.h"

#include <dmctl.h>
#include <dcopref.h>

#include <kstandarddirs.h>
#include <kapplication.h>
#include <kservicegroup.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <klibloader.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kpixmapeffect.h>
#include <kpixmap.h>
#include <kwin.h>
#include <kwinmodule.h>
#include <kdialog.h>

#include <tqframe.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqcursor.h>
#include <tqtimer.h>
#include <tqfile.h>
#include <tqsocketnotifier.h>
#include <tqvaluevector.h>
#include <tqtooltip.h>
#include <tqimage.h>
#include <tqregexp.h>
#include <tqpainter.h>
#include <tqeventloop.h>

#include <tqdatetime.h>

#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#ifdef HAVE_SETPRIORITY
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>

#include <kcrash.h>

#include <linux/stat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#ifdef HAVE_DPMS
extern "C" {
#include <X11/Xmd.h>
#ifndef Bool
#define Bool BOOL
#endif
#include <X11/extensions/dpms.h>

#ifndef HAVE_DPMSINFO_PROTO
Status DPMSInfo ( Display *, CARD16 *, BOOL * );
#endif
}
#endif

#ifdef HAVE_XF86MISC
#include <X11/extensions/xf86misc.h>
#endif

#ifdef HAVE_GLXCHOOSEVISUAL
#include <GL/glx.h>
#endif

#define LOCK_GRACE_DEFAULT          5000
#define AUTOLOGOUT_DEFAULT          600

#define DESKTOP_WALLPAPER_OBTAIN_TIMEOUT_MS	3000

// Setting this define is INSECURE
// Use it for debugging purposes ONLY
// #define KEEP_MOUSE_UNGRABBED 1

// These lines are taken on 10/2009 from X.org (X11/XF86keysym.h), defining some special multimedia keys
#define XF86XK_AudioMute 0x1008FF12
#define XF86XK_AudioRaiseVolume 0x1008FF13
#define XF86XK_AudioLowerVolume 0x1008FF11
#define XF86XK_Display 0x1008FF59

static Window gVRoot = 0;
static Window gVRootData = 0;
static Atom   gXA_VROOT;
static Atom   gXA_SCREENSAVER_VERSION;

void print_trace()
{
#ifdef WITH_KDESKTOP_LOCK_BACKTRACE
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	printf ("[kdesktop_lock] Obtained %zd stack frames.\n\r", size);

	for (i = 0; i < size; i++) {
		printf ("[kdesktop_lock] %s\n\r", strings[i]);
	}

	free (strings);
#endif
}

static void segv_handler(int)
{
	printf("[kdesktop_lock] WARNING: A fatal exception was encountered.  Trapping and ignoring it so as not to compromise desktop security...\n\r");
	print_trace();
	sleep(1);
}

extern Atom qt_wm_state;
extern bool trinity_desktop_lock_use_system_modal_dialogs;
extern bool trinity_desktop_lock_delay_screensaver_start;
extern bool trinity_desktop_lock_use_sak;
extern bool trinity_desktop_lock_hide_active_windows;
extern bool trinity_desktop_lock_forced;

extern TQXLibWindowList trinity_desktop_lock_hidden_window_list;

bool trinity_desktop_lock_autohide_lockdlg = TRUE;
bool trinity_desktop_lock_closing_windows = FALSE;
bool trinity_desktop_lock_in_sec_dlg = FALSE;
bool trinity_desktop_hack_active = FALSE;

#define ENABLE_CONTINUOUS_LOCKDLG_DISPLAY \
if (!mForceContinualLockDisplayTimer->isActive()) mForceContinualLockDisplayTimer->start(100, FALSE); \
trinity_desktop_lock_autohide_lockdlg = FALSE; \
mHackDelayStartupTimer->stop();

#define DISABLE_CONTINUOUS_LOCKDLG_DISPLAY \
mForceContinualLockDisplayTimer->stop(); \
trinity_desktop_lock_autohide_lockdlg = TRUE; \
mHackDelayStartupTimer->stop();

//===========================================================================
//
// Screen saver handling process.  Handles screensaver window,
// starting screensaver hacks, and password entry.
//
LockProcess::LockProcess()
    : TQWidget(0L, "saver window", ((WFlags)(WStyle_StaysOnTop|WStyle_Customize|WStyle_NoBorder))),
      mOpenGLVisual(0),
      mParent(0),
      mShowLockDateTime(false),
      mSuspended(false),
      mVisibility(false),
      mRestoreXF86Lock(false),
      mForbidden(false),
      mAutoLogout(false),
      resizeTimer(NULL),
      hackResumeTimer(NULL),
      mVkbdProcess(NULL),
      mKWinModule(NULL),
      mPipeOpen(false),
      mPipeOpen_out(false),
      mInfoMessageDisplayed(false),
      mDialogControlLock(false),
      mForceReject(false),
      currentDialog(NULL),
      mEnsureScreenHiddenTimer(NULL),
      mForceContinualLockDisplayTimer(NULL),
      mEnsureVRootWindowSecurityTimer(NULL),
      mHackDelayStartupTimer(NULL),
      mHackDelayStartupTimeout(0),
      mHackStartupEnabled(true),
      mResizingDesktopLock(false),
      m_rootPixmap(NULL),
      mBackingStartupDelayTimer(0),
      m_startupStatusDialog(NULL),
      m_mouseDown(0),
      m_mousePrevX(0),
      m_mousePrevY(0),
      m_dialogPrevX(0),
      m_dialogPrevY(0),
      m_maskWidget(NULL)
{
#ifdef KEEP_MOUSE_UNGRABBED
    setNFlags(WX11DisableMove|WX11DisableClose|WX11DisableShade|WX11DisableMinimize|WX11DisableMaximize);
#endif

    setupSignals();
    setupPipe();

    kapp->installX11EventFilter(this);

    mForceContinualLockDisplayTimer = new TQTimer( this );
    mHackDelayStartupTimer = new TQTimer( this );
    mEnsureVRootWindowSecurityTimer = new TQTimer( this );

    // Try to get the root pixmap
    if (!m_rootPixmap) m_rootPixmap = new KRootPixmap(this);
    connect(m_rootPixmap, TQT_SIGNAL(backgroundUpdated(const TQPixmap &)), this, TQT_SLOT(slotPaintBackground(const TQPixmap &)));
    m_rootPixmap->setCustomPainting(true);
    m_rootPixmap->start();

    // Get root window attributes
    XWindowAttributes rootAttr;
    XGetWindowAttributes(qt_xdisplay(), RootWindow(qt_xdisplay(), qt_xscreen()), &rootAttr);
    { // trigger creation of QToolTipManager, it does XSelectInput() on the root window
        TQWidget w;
        TQToolTip::add( &w, "foo" );
    }
    XSelectInput( qt_xdisplay(), qt_xrootwin(), SubstructureNotifyMask | rootAttr.your_event_mask );

    // Add non-KDE path
    KGlobal::dirs()->addResourceType("scrsav",
                                    KGlobal::dirs()->kde_default("apps") +
                                    "System/ScreenSavers/");

    // Add KDE specific screensaver path
    TQString relPath="System/ScreenSavers/";
    KServiceGroup::Ptr servGroup = KServiceGroup::baseGroup( "screensavers");
    if (servGroup)
    {
      relPath=servGroup->relPath();
      kdDebug(1204) << "relPath=" << relPath << endl;
    }
    KGlobal::dirs()->addResourceType("scrsav",
                                     KGlobal::dirs()->kde_default("apps") +
                                     relPath);

    // virtual root property
    gXA_VROOT = XInternAtom (qt_xdisplay(), "__SWM_VROOT", False);
    gXA_SCREENSAVER_VERSION = XInternAtom (qt_xdisplay(), "_SCREENSAVER_VERSION", False);

    TQStringList dmopt =
        TQStringList::split(TQChar(','),
                            TQString::fromLatin1( ::getenv( "XDM_MANAGED" )));
    for (TQStringList::ConstIterator it = dmopt.begin(); it != dmopt.end(); ++it)
        if ((*it).startsWith("method="))
            mMethod = (*it).mid(7);

#ifdef KEEP_MOUSE_UNGRABBED
    setEnabled(false);
#endif

    greetPlugin.library = 0;

    KCrash::setCrashHandler(segv_handler);
}

//---------------------------------------------------------------------------
//
// Destructor - usual cleanups.
//
LockProcess::~LockProcess()
{
    if (resizeTimer != NULL) {
        resizeTimer->stop();
        delete resizeTimer;
    }
    if (hackResumeTimer != NULL) {
        hackResumeTimer->stop();
        delete hackResumeTimer;
    }
    if (mEnsureScreenHiddenTimer != NULL) {
        mEnsureScreenHiddenTimer->stop();
        delete mEnsureScreenHiddenTimer;
    }
    if (mForceContinualLockDisplayTimer != NULL) {
        mForceContinualLockDisplayTimer->stop();
        delete mForceContinualLockDisplayTimer;
    }
    if (mHackDelayStartupTimer != NULL) {
        mHackDelayStartupTimer->stop();
        delete mHackDelayStartupTimer;
    }
    if (mEnsureVRootWindowSecurityTimer != NULL) {
        mEnsureVRootWindowSecurityTimer->stop();
        delete mEnsureVRootWindowSecurityTimer;
    }

    if (greetPlugin.library) {
        if (greetPlugin.info->done)
            greetPlugin.info->done();
        greetPlugin.library->unload();
    }

    if (m_rootPixmap) {
        m_rootPixmap->stop();
        delete m_rootPixmap;
    }

    mPipeOpen = false;
    mPipeOpen_out = false;
}

//---------------------------------------------------------------------------
//
// Initialization for startup
// This is where instance settings should be set--all objects should have already been created in the constructor above
//
void LockProcess::init(bool child, bool useBlankOnly)
{
    // Get root window size
    XWindowAttributes rootAttr;
    XGetWindowAttributes(qt_xdisplay(), RootWindow(qt_xdisplay(), qt_xscreen()), &rootAttr);
    mRootWidth = rootAttr.width;
    mRootHeight = rootAttr.height;

    // Connect all signals
    connect( mForceContinualLockDisplayTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(displayLockDialogIfNeeded()) );
    connect( mHackDelayStartupTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(closeDialogAndStartHack()) );
    connect( mEnsureVRootWindowSecurityTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(repaintRootWindowIfNeeded()) );
    connect(tqApp, TQT_SIGNAL(mouseInteraction(XEvent *)), TQT_SLOT(slotMouseActivity(XEvent *)));
    connect(&mHackProc, TQT_SIGNAL(processExited(KProcess *)), TQT_SLOT(hackExited(KProcess *)));
    connect(&mSuspendTimer, TQT_SIGNAL(timeout()), TQT_SLOT(suspend()));

#ifdef HAVE_DPMS
    if (mDPMSDepend) {
        BOOL on;
        CARD16 state;
        DPMSInfo(qt_xdisplay(), &state, &on);
        if (on)
        {
            connect(&mCheckDPMS, TQT_SIGNAL(timeout()), TQT_SLOT(checkDPMSActive()));
            // we can save CPU if we stop it as quickly as possible
            // but we waste CPU if we check too often -> so take 10s
            mCheckDPMS.start(10000);
        }
    }
#endif

#if (QT_VERSION-0 >= 0x030200) // XRANDR support
    connect( kapp->desktop(), TQT_SIGNAL( resized( int )), TQT_SLOT( desktopResized()));
#endif

    if (!trinity_desktop_lock_use_system_modal_dialogs) {
        setWFlags((WFlags)WX11BypassWM);
    }

    child_saver = child;
    mUseBlankOnly = useBlankOnly;

    mShowLockDateTime = KDesktopSettings::showLockDateTime();
    mlockDateTime = TQDateTime::currentDateTime();

    mHackDelayStartupTimeout = trinity_desktop_lock_delay_screensaver_start?KDesktopSettings::timeout()*1000:10*1000;
    mHackStartupEnabled = trinity_desktop_lock_use_system_modal_dialogs?KDesktopSettings::screenSaverEnabled():true;

    configure();
}

static int signal_pipe[2];

static void sigterm_handler(int)
{
    if (!trinity_desktop_lock_in_sec_dlg) {
        // Exit uncleanly
        exit(1);
    }
}

static void sighup_handler(int)
{
    char tmp = 'H';
    if (::write( signal_pipe[1], &tmp, 1) == -1) {
        // Error handler to shut up gcc warnings
    }
}

bool LockProcess::closeCurrentWindow()
{
    trinity_desktop_lock_closing_windows = TRUE;
    if (currentDialog != NULL) {
        mForceReject = true;
        if (dynamic_cast<SAKDlg*>(currentDialog)) {
            dynamic_cast<SAKDlg*>(currentDialog)->closeDialogForced();
        }
        else if (dynamic_cast<SecureDlg*>(currentDialog)) {
            dynamic_cast<SecureDlg*>(currentDialog)->closeDialogForced();
        }
        else {
            currentDialog->close();
        }
    }

    if( mDialogs.isEmpty() ) {
        trinity_desktop_lock_closing_windows = FALSE;
        mForceReject = false;
        return false;
    }
    else {
        trinity_desktop_lock_closing_windows = TRUE;
        return true;
    }
}

void LockProcess::timerEvent(TQTimerEvent *ev)
{
	if (mAutoLogout && ev->timerId() == mAutoLogoutTimerId)
	{
		killTimer(mAutoLogoutTimerId);
		AutoLogout autologout(this);
		execDialog(&autologout);
	}
}

void LockProcess::resizeEvent(TQResizeEvent *)
{
	//
}

void LockProcess::setupPipe()
{
    /* Create the FIFOs if they do not exist */
    umask(0);
    mkdir(FIFO_DIR,0644);
    mknod(FIFO_FILE, S_IFIFO|0644, 0);
    chmod(FIFO_FILE, 0644);

    mPipe_fd = open(FIFO_FILE, O_RDONLY | O_NONBLOCK);
    if (mPipe_fd > -1) {
        mPipeOpen = true;
        TQTimer::singleShot( PIPE_CHECK_INTERVAL, this, TQT_SLOT(checkPipe()) );
    }

    mknod(FIFO_FILE_OUT, S_IFIFO|0600, 0);
    chmod(FIFO_FILE_OUT, 0600);

    mPipe_fd_out = open(FIFO_FILE_OUT, O_RDWR | O_NONBLOCK);
    if (mPipe_fd_out > -1) {
        mPipeOpen_out = true;
    }
}

void LockProcess::checkPipe()
{
    char readbuf[128];
    int numread;
    TQString to_display;
    const char * pin_entry;

    if (mPipeOpen == true) {
        readbuf[0]=' ';
        numread = read(mPipe_fd, readbuf, 128);
        readbuf[numread] = 0;
        if (numread > 0) {
            if (readbuf[0] == 'C') {
                mInfoMessageDisplayed=false;
                while (mDialogControlLock == true) usleep(100000);
                mDialogControlLock = true;
                if (currentDialog != NULL) {
                    mForceReject = true;
                    closeCurrentWindow();
                }
                mDialogControlLock = false;
            }
            if (readbuf[0] == 'T') {
                to_display = readbuf;
                to_display = to_display.remove(0,1);
                // Lock out password dialogs and close any active dialog
                mInfoMessageDisplayed=true;
                while (mDialogControlLock == true) usleep(100000);
                mDialogControlLock = true;
                if (currentDialog != NULL) {
                    mForceReject = true;
                    closeCurrentWindow();
                }
                mDialogControlLock = false;
                // Display info message dialog
                TQTimer::singleShot( PIPE_CHECK_INTERVAL, this, TQT_SLOT(checkPipe()) );
                InfoDlg inDlg( this );
                inDlg.updateLabel(to_display);
                inDlg.setUnlockIcon();
                execDialog( &inDlg );
                mForceReject = false;
                return;
            }
            if ((readbuf[0] == 'E') || (readbuf[0] == 'W') || (readbuf[0] == 'I') || (readbuf[0] == 'K')) {
                to_display = readbuf;
                to_display = to_display.remove(0,1);
                // Lock out password dialogs and close any active dialog
                mInfoMessageDisplayed=true;
                while (mDialogControlLock == true) usleep(100000);
                mDialogControlLock = true;
                if (currentDialog != NULL) {
                    mForceReject = true;
                    closeCurrentWindow();
                }
                mDialogControlLock = false;
                // Display info message dialog
                TQTimer::singleShot( PIPE_CHECK_INTERVAL, this, TQT_SLOT(checkPipe()) );
                InfoDlg inDlg( this );
                inDlg.updateLabel(to_display);
                if (readbuf[0] == 'K') inDlg.setKDEIcon();
                if (readbuf[0] == 'I') inDlg.setInfoIcon();
                if (readbuf[0] == 'W') inDlg.setWarningIcon();
                if (readbuf[0] == 'E') inDlg.setErrorIcon();
                execDialog( &inDlg );
                mForceReject = false;
                return;
            }
            if (readbuf[0] == 'Q') {
                to_display = readbuf;
                to_display = to_display.remove(0,1);
                // Lock out password dialogs and close any active dialog
                mInfoMessageDisplayed=true;
                while (mDialogControlLock == true) usleep(100000);
                mDialogControlLock = true;
                if (currentDialog != NULL) {
                    mForceReject = true;
                    closeCurrentWindow();
                }
                mDialogControlLock = false;
                // Display query dialog
                TQTimer::singleShot( PIPE_CHECK_INTERVAL, this, TQT_SLOT(checkPipe()) );
                QueryDlg qryDlg( this );
                qryDlg.updateLabel(to_display);
                qryDlg.setUnlockIcon();
                mForceReject = false;
                execDialog( &qryDlg );
                if (mForceReject == false) {
                    pin_entry = qryDlg.getEntry();
                    mInfoMessageDisplayed=false;
                    if (mPipeOpen_out == true) {
                        if (write(mPipe_fd_out, pin_entry, strlen(pin_entry)+1) == -1) {
                            // Error handler to shut up gcc warnings
                        }
                        if (write(mPipe_fd_out, "\n\r", 3) == -1) {
                            // Error handler to shut up gcc warnings
                        }
                    }
                }
                mForceReject = false;
                return;
            }
        }
        TQTimer::singleShot( PIPE_CHECK_INTERVAL, this, TQT_SLOT(checkPipe()) );
    }
}

void LockProcess::setupSignals()
{
    struct sigaction act;
    // ignore SIGINT
    act.sa_handler=SIG_IGN;
    sigemptyset(&(act.sa_mask));
    sigaddset(&(act.sa_mask), SIGINT);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0L);
    // ignore SIGQUIT
    act.sa_handler=SIG_IGN;
    sigemptyset(&(act.sa_mask));
    sigaddset(&(act.sa_mask), SIGQUIT);
    act.sa_flags = 0;
    sigaction(SIGQUIT, &act, 0L);
    // exit uncleanly on SIGTERM
    act.sa_handler= sigterm_handler;
    sigemptyset(&(act.sa_mask));
    sigaddset(&(act.sa_mask), SIGTERM);
    act.sa_flags = 0;
    sigaction(SIGTERM, &act, 0L);
    // SIGHUP forces lock
    act.sa_handler= sighup_handler;
    sigemptyset(&(act.sa_mask));
    sigaddset(&(act.sa_mask), SIGHUP);
    act.sa_flags = 0;
    sigaction(SIGHUP, &act, 0L);

    if (pipe(signal_pipe) == -1) {
        // Error handler to shut up gcc warnings
    }
    TQSocketNotifier* notif = new TQSocketNotifier(signal_pipe[0],
	TQSocketNotifier::Read, TQT_TQOBJECT(this) );
    connect( notif, TQT_SIGNAL(activated(int)), TQT_SLOT(signalPipeSignal()));
}


void LockProcess::signalPipeSignal()
{
    char tmp;
    if (::read( signal_pipe[0], &tmp, 1) == -1) {
        // Error handler to shut up gcc warnings
    }
    if( tmp == 'T' )
        quitSaver();
    else if( tmp == 'H' ) {
        if( !mLocked )
            startLock();
    }
}

//---------------------------------------------------------------------------
bool LockProcess::lock()
{
#ifdef USE_SECURING_DESKTOP_NOTIFICATION
	m_startupStatusDialog = new KSMModalDialog(this);
	m_startupStatusDialog->setStatusMessage(i18n("Securing desktop session").append("..."));
	m_startupStatusDialog->show();
	m_startupStatusDialog->setActiveWindow();
	tqApp->processEvents();
#endif

	if (startSaver()) {
		// In case of a forced lock we don't react to events during
		// the dead-time to give the screensaver some time to activate.
		// That way we don't accidentally show the password dialog before
		// the screensaver kicks in because the user moved the mouse after
		// selecting "lock screen", that looks really untidy.
		mBusy = true;
		if (startLock())
		{
			TQTimer::singleShot(1000, this, TQT_SLOT(slotDeadTimePassed()));
			return true;
		}
		stopSaver();
		mBusy = false;
	}
	return false;
}
//---------------------------------------------------------------------------
void LockProcess::slotDeadTimePassed()
{
    mBusy = false;
}

//---------------------------------------------------------------------------
bool LockProcess::defaultSave()
{
    mLocked = false;
    if (startSaver()) {
        if (mLockGrace >= 0)
            TQTimer::singleShot(mLockGrace, this, TQT_SLOT(startLock()));
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------
bool LockProcess::dontLock()
{
    mLocked = false;
    return startSaver();
}

//---------------------------------------------------------------------------
void LockProcess::quitSaver()
{
    DISABLE_CONTINUOUS_LOCKDLG_DISPLAY
    if (closeCurrentWindow()) {
        TQTimer::singleShot( 0, this, SLOT(quitSaver()) );
        return;
    }
    stopSaver();
    kapp->quit();
}

//---------------------------------------------------------------------------
void LockProcess::startSecureDialog()
{
	if ((backingPixmap.isNull()) && (mBackingStartupDelayTimer < 100)) {
		TQTimer::singleShot(10, this, TQT_SLOT(startSecureDialog()));
		mBackingStartupDelayTimer++;
		return;
	}

	setGeometry(0, 0, mRootWidth, mRootHeight);

	int ret;
	SecureDlg inDlg( this );
	inDlg.setRetInt(&ret);
	mBusy = true;
	execDialog( &inDlg );
	mBusy = false;
	bool forcecontdisp = mForceContinualLockDisplayTimer->isActive();
	if (forcecontdisp) {
		DISABLE_CONTINUOUS_LOCKDLG_DISPLAY
	}
	trinity_desktop_lock_in_sec_dlg = false;
	if (ret == 0) {
		trinity_desktop_lock_closing_windows = 1;
		kapp->quit();
	}
	if (ret == 1) {
		// In case of a forced lock we don't react to events during
		// the dead-time to give the screensaver some time to activate.
		// That way we don't accidentally show the password dialog before
		// the screensaver kicks in because the user moved the mouse after
		// selecting "lock screen", that looks really untidy.
		mBusy = true;
		trinity_desktop_lock_forced = true;
		// Make sure the cursor is not showing busy status
		setCursor( tqarrowCursor );
		if (startLock())
		{
			if (trinity_desktop_lock_delay_screensaver_start) {
				mBusy = false;
			}
			else {
				TQTimer::singleShot(1000, this, TQT_SLOT(slotDeadTimePassed()));
			}
			if (trinity_desktop_lock_delay_screensaver_start && trinity_desktop_lock_forced && trinity_desktop_lock_use_system_modal_dialogs) {
				ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
				if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
			}
			else {
				if (mHackStartupEnabled == true) {
					startHack();
				}
				else {
					if (trinity_desktop_lock_use_system_modal_dialogs == true) {
						ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
						if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
					}
					else {
						startHack();
					}
				}
			}
			return;
		}
		stopSaver();
		mBusy = false;
		return;
	}
	if (ret == 2) {
		trinity_desktop_lock_closing_windows = 1;
		if (system("ksysguard &") == -1) {
                    // Error handler to shut up gcc warnings
                }
		kapp->quit();
	}
	if (ret == 3) {
		trinity_desktop_lock_closing_windows = 1;
		DCOPRef("ksmserver","ksmserver").send("logout", (int)KApplication::ShutdownConfirmYes, (int)KApplication::ShutdownTypeNone, (int)KApplication::ShutdownModeInteractive);
		kapp->quit();
	}
	// FIXME
	// Handle remaining case (switch user)
	if (forcecontdisp) {
		ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
		if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
	}
	stopSaver();
}

bool LockProcess::runSecureDialog()
{
#ifdef USE_SECURING_DESKTOP_NOTIFICATION
	m_startupStatusDialog = new KSMModalDialog(this);
	m_startupStatusDialog->setStatusMessage(i18n("Securing desktop session").append("..."));
	m_startupStatusDialog->show();
	m_startupStatusDialog->setActiveWindow();
	tqApp->processEvents();
#endif

	trinity_desktop_lock_in_sec_dlg = true;
	if (startSaver()) {
		mBackingStartupDelayTimer = 0;
		TQTimer::singleShot(0, this, TQT_SLOT(startSecureDialog()));
		return true;
	}
	else {
		return false;
	}
}

//---------------------------------------------------------------------------
//
// Read and apply configuration.
//
void LockProcess::configure()
{
    // the configuration is stored in kdesktop's config file
    if( KDesktopSettings::lock() )
    {
        mLockGrace = KDesktopSettings::lockGrace();
        if (mLockGrace < 0)
            mLockGrace = 0;
        else if (mLockGrace > 300000)
            mLockGrace = 300000; // 5 minutes, keep the value sane
    }
    else
        mLockGrace = -1;

    if ( KDesktopSettings::autoLogout() )
    {
        mAutoLogout = true;
        mAutoLogoutTimeout = KDesktopSettings::autoLogoutTimeout();
        mAutoLogoutTimerId = startTimer(mAutoLogoutTimeout * 1000); // in milliseconds
    }

#ifdef HAVE_DPMS
    //if the user  decided that the screensaver should run independent from
    //dpms, we shouldn't check for it, aleXXX
    mDPMSDepend = KDesktopSettings::dpmsDependent();
#endif

    mPriority = KDesktopSettings::priority();
    if (mPriority < 0) mPriority = 0;
    if (mPriority > 19) mPriority = 19;

    mSaver = KDesktopSettings::saver();
    if (mSaver.isEmpty() || mUseBlankOnly) {
        mSaver = "KBlankscreen.desktop";
    }
    if (KDesktopSettings::screenSaverEnabled() == false) {
        mSaver = "";
        mSaverExec = "";
    }

    readSaver();

    mPlugins = KDesktopSettings::pluginsUnlock();
    if (mPlugins.isEmpty())
        mPlugins = TQStringList("classic");
    mPluginOptions = KDesktopSettings::pluginOptions();
}

//---------------------------------------------------------------------------
//
// Read the command line needed to run the screensaver given a .desktop file.
//
void LockProcess::readSaver()
{
    if (!mSaver.isEmpty())
    {
        TQString file = locate("scrsav", mSaver);

	bool opengl = kapp->authorize("opengl_screensavers");
	bool manipulatescreen = kapp->authorize("manipulatescreen_screensavers");
        KDesktopFile config(file, true);
	if (config.readEntry("X-KDE-Type").utf8() != 0)
	{
		TQString saverType = config.readEntry("X-KDE-Type").utf8();
		TQStringList saverTypes = TQStringList::split(";", saverType);
		for (uint i = 0; i < saverTypes.count(); i++)
		{
			if ((saverTypes[i] == "ManipulateScreen") && !manipulatescreen)
			{
				kdDebug(1204) << "Screensaver is type ManipulateScreen and ManipulateScreen is forbidden" << endl;
				mForbidden = true;
			}
			if ((saverTypes[i] == "OpenGL") && !opengl)
			{
				kdDebug(1204) << "Screensaver is type OpenGL and OpenGL is forbidden" << endl;
				mForbidden = true;
			}
			if (saverTypes[i] == "OpenGL")
			{
				mOpenGLVisual = true;
			}
		}
	}

	kdDebug(1204) << "mForbidden: " << (mForbidden ? "true" : "false") << endl;

        if (trinity_desktop_lock_use_system_modal_dialogs) {
            if (config.hasActionGroup("InWindow"))
            {
                config.setActionGroup("InWindow");
                mSaverExec = config.readPathEntry("Exec");
            }
        }
        else {
            if (config.hasActionGroup("Root"))
            {
                config.setActionGroup("Root");
                mSaverExec = config.readPathEntry("Exec");
            }
        }
    }
}

//---------------------------------------------------------------------------
//
// Create a window to draw our screen saver on.
//
void LockProcess::createSaverWindow()
{
    Visual* visual = CopyFromParent;
    XSetWindowAttributes attrs;
    int flags = trinity_desktop_lock_use_system_modal_dialogs?0:CWOverrideRedirect;
#ifdef HAVE_GLXCHOOSEVISUAL
    if( mOpenGLVisual )
    {
        static int attribs[][ 15 ] =
        {
        #define R GLX_RED_SIZE
        #define G GLX_GREEN_SIZE
        #define B GLX_BLUE_SIZE
            { GLX_RGBA, R, 8, G, 8, B, 8, GLX_DEPTH_SIZE, 8, GLX_DOUBLEBUFFER, GLX_STENCIL_SIZE, 1, None },
            { GLX_RGBA, R, 4, G, 4, B, 4, GLX_DEPTH_SIZE, 4, GLX_DOUBLEBUFFER, GLX_STENCIL_SIZE, 1, None },
            { GLX_RGBA, R, 8, G, 8, B, 8, GLX_DEPTH_SIZE, 8, GLX_DOUBLEBUFFER, None },
            { GLX_RGBA, R, 4, G, 4, B, 4, GLX_DEPTH_SIZE, 4, GLX_DOUBLEBUFFER, None },
            { GLX_RGBA, R, 8, G, 8, B, 8, GLX_DEPTH_SIZE, 8, GLX_STENCIL_SIZE, 1, None },
            { GLX_RGBA, R, 4, G, 4, B, 4, GLX_DEPTH_SIZE, 4, GLX_STENCIL_SIZE, 1, None },
            { GLX_RGBA, R, 8, G, 8, B, 8, GLX_DEPTH_SIZE, 8, None },
            { GLX_RGBA, R, 4, G, 4, B, 4, GLX_DEPTH_SIZE, 4, None },
            { GLX_RGBA, GLX_DEPTH_SIZE, 8, GLX_DOUBLEBUFFER, GLX_STENCIL_SIZE, 1, None },
            { GLX_RGBA, GLX_DEPTH_SIZE, 8, GLX_DOUBLEBUFFER, None },
            { GLX_RGBA, GLX_DEPTH_SIZE, 8, GLX_STENCIL_SIZE, 1, None },
            { GLX_RGBA, GLX_DEPTH_SIZE, 8, None }
        #undef R
        #undef G
        #undef B
        };
        for( unsigned int i = 0;
             i < sizeof( attribs ) / sizeof( attribs[ 0 ] );
             ++i )
        {
            if( XVisualInfo* info = glXChooseVisual( x11Display(), x11Screen(), attribs[ i ] ))
            {
                visual = info->visual;
                static Colormap colormap = 0;
                if( colormap != 0 )
                    XFreeColormap( x11Display(), colormap );
                colormap = XCreateColormap( x11Display(), RootWindow( x11Display(), x11Screen()), visual, AllocNone );
                attrs.colormap = colormap;
                flags |= CWColormap;
                XFree( info );
                break;
            }
        }
    }
#endif

    attrs.override_redirect = 1;
    hide();
    Window w = XCreateWindow( x11Display(), RootWindow( x11Display(), x11Screen()),
        x(), y(), width(), height(), 0, x11Depth(), InputOutput, visual, flags, &attrs );
    create( w );

    // Some xscreensaver hacks check for this property
    const char *version = "KDE 2.0";
    XChangeProperty (qt_xdisplay(), winId(),
                     gXA_SCREENSAVER_VERSION, XA_STRING, 8, PropModeReplace,
                     (unsigned char *) version, strlen(version));

    XSetWindowAttributes attr;
    attr.event_mask = KeyPressMask | ButtonPressMask | PointerMotionMask |
                        VisibilityChangeMask | ExposureMask;
    XChangeWindowAttributes(qt_xdisplay(), winId(),
                            CWEventMask, &attr);

    // erase();

    // set NoBackground so that the saver can capture the current
    // screen state if necessary
    // this is a security risk and has been deactivated--welcome to the 21st century folks!
    // setBackgroundMode(TQWidget::NoBackground);

    setGeometry(0, 0, mRootWidth, mRootHeight);

    // HACK
    // Hide all tooltips and notification windows
    {
        Window rootWindow = RootWindow(x11Display(), x11Screen());
        Window parent;
        Window* children = NULL;
        unsigned int noOfChildren = 0;
        XWindowAttributes childAttr;
        Window childTransient;

        if (XQueryTree(x11Display(), rootWindow, &rootWindow, &parent, &children, &noOfChildren) && noOfChildren>0 ) {
            for (unsigned int i=0; i<noOfChildren; i++) {
                if (XGetWindowAttributes(x11Display(), children[i], &childAttr) && XGetTransientForHint(x11Display(), children[i], &childTransient)) {
                    if ((childAttr.map_state == IsViewable) && (childAttr.override_redirect) && (childTransient)) {
                        if (!trinity_desktop_lock_hidden_window_list.contains(children[i])) {
                            trinity_desktop_lock_hidden_window_list.append(children[i]);
                        }
                        XLowerWindow(x11Display(), children[i]);
                        XFlush(x11Display());
                    }
                }
            }
        }
    }

    kdDebug(1204) << "Saver window Id: " << winId() << endl;
}

void LockProcess::desktopResized()
{
    // Get root window size
    XWindowAttributes rootAttr;
    XGetWindowAttributes(qt_xdisplay(), RootWindow(qt_xdisplay(), qt_xscreen()), &rootAttr);
    if ((rootAttr.width == mRootWidth) && (rootAttr.height == mRootHeight)) {
        return;
    }
    mRootWidth = rootAttr.width;
    mRootHeight = rootAttr.height;

    mBusy = true;
    mHackDelayStartupTimer->stop();
    stopHack();
    DISABLE_CONTINUOUS_LOCKDLG_DISPLAY
    mResizingDesktopLock = true;

    backingPixmap = TQPixmap();

    if (trinity_desktop_lock_use_system_modal_dialogs) {
        // Temporarily hide the entire screen with a new override redirect window
        if (m_maskWidget) {
        	m_maskWidget->setGeometry(0, 0, mRootWidth, mRootHeight);
        }
        else {
        	int flags = CWOverrideRedirect;
        	Visual* visual = CopyFromParent;
        	XSetWindowAttributes attrs;
        	attrs.override_redirect = 1;
        	Window maskWindow = XCreateWindow(x11Display(), RootWindow( x11Display(), x11Screen()), 0, 0, mRootWidth, mRootHeight, 0, x11Depth(), InputOutput, visual, flags, &attrs);
        	m_maskWidget = new TQWidget();
        	m_maskWidget->create(maskWindow);
        	m_maskWidget->setBackgroundColor(TQt::black);
        	m_maskWidget->erase();
        	m_maskWidget->show();
        }
	XSync(qt_xdisplay(), False);

	if (mEnsureScreenHiddenTimer) {
		mEnsureScreenHiddenTimer->stop();
	}
	else {
		mEnsureScreenHiddenTimer = new TQTimer( this );
		connect( mEnsureScreenHiddenTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotForcePaintBackground()) );
	}
	mEnsureScreenHiddenTimer->start(DESKTOP_WALLPAPER_OBTAIN_TIMEOUT_MS, true);
    }

    // Resize the background widget
    setGeometry(0, 0, mRootWidth, mRootHeight);
    XSync(qt_xdisplay(), False);

    // Black out the background widget to hide ugly resize tiling artifacts
    setBackgroundColor(black);
    erase();

    // This slot needs to be able to execute very rapidly so as to prevent the user's desktop from ever
    // being displayed, so we finish the hack restarting/display prettying operations in a separate timed slot
    if (resizeTimer == NULL) {
        resizeTimer = new TQTimer( this );
        connect( resizeTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(doDesktopResizeFinish()) );
    }
    resizeTimer->start( 100, TRUE ); // 100 millisecond single shot timer; should allow display switching operations to finish before hack is started
}

void LockProcess::doDesktopResizeFinish()
{
	while (mDialogControlLock == true) {
		usleep(100000);
	}
	mDialogControlLock = true;
	if (closeCurrentWindow()) {
		TQTimer::singleShot( 0, this, SLOT(doDesktopResizeFinish()) );
		mDialogControlLock = false;
		return;
	}
	mDialogControlLock = false;

	// Restart the hack as the window size is now different
	if (trinity_desktop_lock_delay_screensaver_start && trinity_desktop_lock_use_system_modal_dialogs) {
		ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
		if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
	}
	else {
		if (mHackStartupEnabled == true) {
			startHack();
		}
		else {
			if (trinity_desktop_lock_use_system_modal_dialogs == true) {
				ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
				if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
			}
			else {
				startHack();
			}
		}
	}

        mResizingDesktopLock = false;
	mBusy = false;
}

//---------------------------------------------------------------------------
//
// Hide the screensaver window
//
void LockProcess::hideSaverWindow()
{
  hide();
  lower();
  removeVRoot(winId());
  XDeleteProperty(qt_xdisplay(), winId(), gXA_SCREENSAVER_VERSION);
  if ( gVRoot ) {
      unsigned long vroot_data[1] = { gVRootData };
      XChangeProperty(qt_xdisplay(), gVRoot, gXA_VROOT, XA_WINDOW, 32,
                      PropModeReplace, (unsigned char *)vroot_data, 1);
      gVRoot = 0;
  }
  XSync(qt_xdisplay(), False);
}

//---------------------------------------------------------------------------
static int ignoreXError(Display *, XErrorEvent *)
{
    return 0;
}

//---------------------------------------------------------------------------
//
// Save the current virtual root window
//
void LockProcess::saveVRoot()
{
  Window rootReturn, parentReturn, *children;
  unsigned int numChildren;
  Window root = RootWindowOfScreen(ScreenOfDisplay(qt_xdisplay(), qt_xscreen()));

  gVRoot = 0;
  gVRootData = 0;

  int (*oldHandler)(Display *, XErrorEvent *);
  oldHandler = XSetErrorHandler(ignoreXError);

  if (XQueryTree(qt_xdisplay(), root, &rootReturn, &parentReturn,
      &children, &numChildren))
  {
    for (unsigned int i = 0; i < numChildren; i++)
    {
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytesafter;
      unsigned char *newRoot = 0;

      if ((XGetWindowProperty(qt_xdisplay(), children[i], gXA_VROOT, 0, 1,
          False, XA_WINDOW, &actual_type, &actual_format, &nitems, &bytesafter,
          &newRoot) == Success) && newRoot)
      {
        gVRoot = children[i];
        Window *dummy = (Window*)newRoot;
        gVRootData = *dummy;
        XFree ((char*) newRoot);
        break;
      }
    }
    if (children)
    {
      XFree((char *)children);
    }
  }

  XSetErrorHandler(oldHandler);
}

//---------------------------------------------------------------------------
//
// Set the virtual root property
//
void LockProcess::setVRoot(Window win, Window vr)
{
    if (gVRoot)
        removeVRoot(gVRoot);

    unsigned long rw = RootWindowOfScreen(ScreenOfDisplay(qt_xdisplay(), qt_xscreen()));
    unsigned long vroot_data[1] = { vr };

    Window rootReturn, parentReturn, *children;
    unsigned int numChildren;
    Window top = win;
    while (1) {
        XQueryTree(qt_xdisplay(), top , &rootReturn, &parentReturn,
                                 &children, &numChildren);
        if (children)
            XFree((char *)children);
        if (parentReturn == rw) {
            break;
        } else
            top = parentReturn;
    }

    XChangeProperty(qt_xdisplay(), top, gXA_VROOT, XA_WINDOW, 32,
                     PropModeReplace, (unsigned char *)vroot_data, 1);
}

//---------------------------------------------------------------------------
//
// Remove the virtual root property
//
void LockProcess::removeVRoot(Window win)
{
    XDeleteProperty (qt_xdisplay(), win, gXA_VROOT);
}

//---------------------------------------------------------------------------
//
// Grab the keyboard. Returns true on success
//
bool LockProcess::grabKeyboard()
{
    int rv = XGrabKeyboard( qt_xdisplay(), TQApplication::desktop()->winId(),
        True, GrabModeAsync, GrabModeAsync, CurrentTime );

    return (rv == GrabSuccess);
}

#define GRABEVENTS ButtonPressMask | ButtonReleaseMask | PointerMotionMask | \
		   EnterWindowMask | LeaveWindowMask

//---------------------------------------------------------------------------
//
// Grab the mouse.  Returns true on success
//
bool LockProcess::grabMouse()
{
    HANDLE cursorHandle;
    if (trinity_desktop_hack_active) {
       cursorHandle = TQCursor(tqblankCursor).handle();
    }
    else {
       cursorHandle = TQCursor(tqbusyCursor).handle();
    }
    int rv = XGrabPointer( qt_xdisplay(), TQApplication::desktop()->winId(),
            True, GRABEVENTS, GrabModeAsync, GrabModeAsync, None,
            cursorHandle, CurrentTime );

    return (rv == GrabSuccess);
}

//---------------------------------------------------------------------------
//
// Grab keyboard and mouse.  Returns true on success.
//
bool LockProcess::grabInput()
{
    XSync(qt_xdisplay(), False);

    if (!grabKeyboard())
    {
        usleep(100000);
        if (!grabKeyboard())
        {
            return false;
        }
    }

#ifndef KEEP_MOUSE_UNGRABBED
    if (!grabMouse())
    {
        usleep(100000);
        if (!grabMouse())
        {
            XUngrabKeyboard(qt_xdisplay(), CurrentTime);
            return false;
        }
    }
#endif

    lockXF86();

    return true;
}

//---------------------------------------------------------------------------
//
// Release mouse an keyboard grab.
//
void LockProcess::ungrabInput()
{
    XUngrabKeyboard(qt_xdisplay(), CurrentTime);
    XUngrabPointer(qt_xdisplay(), CurrentTime);
    unlockXF86();
}

//---------------------------------------------------------------------------
//
// Start the screen saver.
//
bool LockProcess::startSaver()
{
	if (!child_saver && !grabInput())
	{
		kdWarning(1204) << "LockProcess::startSaver() grabInput() failed!!!!" << endl;
		return false;
	}
	mBusy = false;

	// eliminate nasty flicker on first show
	TQImage m_grayImage = TQImage( TQApplication::desktop()->width(), TQApplication::desktop()->height(), 32 );
	m_grayImage = m_grayImage.convertDepth(32);
	m_grayImage.setAlphaBuffer(false);
	m_grayImage.fill(0);	// Set the alpha buffer to 0 (fully transparent)
	m_grayImage.setAlphaBuffer(true);
	TQPixmap m_root;
	m_root.resize(mRootWidth, mRootHeight);
	TQPainter p;
	p.begin( &m_root );
	m_grayImage.setAlphaBuffer(false);
	p.drawImage( 0, 0, m_grayImage );
	p.end();
	setBackgroundPixmap( m_root );

	saveVRoot();

	if (mParent) {
		TQSocketNotifier *notifier = new TQSocketNotifier(mParent, TQSocketNotifier::Read, TQT_TQOBJECT(this), "notifier");
		connect(notifier, TQT_SIGNAL( activated (int)), TQT_SLOT( quitSaver()));
	}
	createSaverWindow();
	move(0, 0);
	show();

	raise();
	XSync(qt_xdisplay(), False);
	setVRoot( winId(), winId() );

	if (!trinity_desktop_lock_hide_active_windows) {
		if (m_rootPixmap) m_rootPixmap->stop();
		TQPixmap rootWinSnapShot = TQPixmap::grabWindow(TQApplication::desktop()->winId());
		slotPaintBackground(rootWinSnapShot);
	}
	else {
		// Sometimes KRootPixmap fails...make sure the desktop is hidden regardless
		if (backingPixmap.isNull()) {
			if (!mEnsureScreenHiddenTimer) {
				mEnsureScreenHiddenTimer = new TQTimer( this );
				connect( mEnsureScreenHiddenTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotForcePaintBackground()) );
				mEnsureScreenHiddenTimer->start(DESKTOP_WALLPAPER_OBTAIN_TIMEOUT_MS, true);
			}

			int exitTimer = 0;
			while ((backingPixmap.isNull()) && (mEnsureScreenHiddenTimer->isActive())) {
				kapp->eventLoop()->processEvents(TQEventLoop::ExcludeUserInput);

				// HACK
				// Work around an issue with the underlying system whereby the TQTimer sometimes fails to time out!
				// This issue was reported in Bug #1288
				usleep(100);
				exitTimer++;
				if (exitTimer > (DESKTOP_WALLPAPER_OBTAIN_TIMEOUT_MS*10)) {
					printf("[WARNING] The Qt3 timer event loop appears to have hung.  Aborting wait for desktop background!\n\r"); fflush(stdout);
					break;
				}
			}
		}
	}

	if (((!(trinity_desktop_lock_delay_screensaver_start && trinity_desktop_lock_forced)) && (!trinity_desktop_lock_in_sec_dlg)) && mHackStartupEnabled) {
		if (backingPixmap.isNull()) {
			setBackgroundColor(black);
		}
		else {
			setBackgroundPixmap(backingPixmap);
		}
		setGeometry(0, 0, mRootWidth, mRootHeight);
		erase();
	}

	if (trinity_desktop_lock_in_sec_dlg == FALSE) {
		if (trinity_desktop_lock_delay_screensaver_start && trinity_desktop_lock_forced && trinity_desktop_lock_use_system_modal_dialogs) {
			ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
			if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
		}
		else {
			if (mHackStartupEnabled == true) {
				startHack();
			}
			else {
				if (trinity_desktop_lock_use_system_modal_dialogs == true) {
					ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
					if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
				}
				else {
					startHack();
				}
			}
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
// Stop the screen saver.
//
void LockProcess::stopSaver()
{
    kdDebug(1204) << "LockProcess: stopping saver" << endl;
    mHackProc.kill(SIGCONT);
    stopHack();
    mSuspended = false;
    hideSaverWindow();
    mVisibility = false;
    if (!child_saver) {
        if (mLocked)
            DM().setLock( false );
        ungrabInput();
        const char *out = "GOAWAY!";
        for (TQValueList<int>::ConstIterator it = child_sockets.begin(); it != child_sockets.end(); ++it)
            if (write(*it, out, sizeof(out)) == -1) {
                // Error handler to shut up gcc warnings
            }
    }
}

// private static
TQVariant LockProcess::getConf(void *ctx, const char *key, const TQVariant &dflt)
{
    LockProcess *that = (LockProcess *)ctx;
    TQString fkey = TQString::fromLatin1( key ) + '=';
    for (TQStringList::ConstIterator it = that->mPluginOptions.begin();
         it != that->mPluginOptions.end(); ++it)
        if ((*it).startsWith( fkey ))
            return (*it).mid( fkey.length() );
    return dflt;
}

void LockProcess::cantLock( const TQString &txt)
{
    msgBox( TQMessageBox::Critical, i18n("Will not lock the session, as unlocking would be impossible:\n") + txt );
}

#if 0 // placeholders for later
i18n("Cannot start <i>kcheckpass</i>.");
i18n("<i>kcheckpass</i> is unable to operate. Possibly it is not SetUID root.");
#endif

//---------------------------------------------------------------------------
//
// Make the screen saver password protected.
//
bool LockProcess::startLock()
{
    for (TQStringList::ConstIterator it = mPlugins.begin(); it != mPlugins.end(); ++it) {
        GreeterPluginHandle plugin;
        TQString path = KLibLoader::self()->findLibrary(
                    ((*it)[0] == '/' ? *it : "kgreet_" + *it ).latin1() );
        if (path.isEmpty()) {
            kdWarning(1204) << "GreeterPlugin " << *it << " does not exist" << endl;
            continue;
        }
        if (!(plugin.library = KLibLoader::self()->library( path.latin1() ))) {
            kdWarning(1204) << "Cannot load GreeterPlugin " << *it << " (" << path << ")" << endl;
            continue;
        }
        if (!plugin.library->hasSymbol( "kgreeterplugin_info" )) {
            kdWarning(1204) << "GreeterPlugin " << *it << " (" << path << ") is no valid greet widget plugin" << endl;
            plugin.library->unload();
            continue;
        }
        plugin.info = (kgreeterplugin_info*)plugin.library->symbol( "kgreeterplugin_info" );
        if (plugin.info->method && !mMethod.isEmpty() && mMethod != plugin.info->method) {
            kdDebug(1204) << "GreeterPlugin " << *it << " (" << path << ") serves " << plugin.info->method << ", not " << mMethod << endl;
            plugin.library->unload();
            continue;
        }
        if (!plugin.info->init( mMethod, getConf, this )) {
            kdDebug(1204) << "GreeterPlugin " << *it << " (" << path << ") refuses to serve " << mMethod << endl;
            plugin.library->unload();
            continue;
        }
        kdDebug(1204) << "GreeterPlugin " << *it << " (" << plugin.info->method << ", " << plugin.info->name << ") loaded" << endl;
        greetPlugin = plugin;
	mLocked = true;
	DM().setLock( true );
	return true;
    }
    cantLock( i18n("No appropriate greeter plugin configured.") );
    return false;
}

//---------------------------------------------------------------------------
//

void LockProcess::closeDialogAndStartHack()
{
    // Close any active dialogs
    DISABLE_CONTINUOUS_LOCKDLG_DISPLAY
    mSuspended = true;
    if (closeCurrentWindow()) {
        TQTimer::singleShot( 0, this, SLOT(closeDialogAndStartHack()) );
    }
    else {
        resume(true);
    }
}

void LockProcess::repaintRootWindowIfNeeded()
{
	if (trinity_desktop_lock_use_system_modal_dialogs) {
		if (!mHackProc.isRunning()) {
			if (backingPixmap.isNull()) {
				setBackgroundColor(black);
				setGeometry(0, 0, mRootWidth, mRootHeight);
				erase();
			}
			else {
				bitBlt(this, 0, 0, &backingPixmap);
			}
		}
		if (currentDialog == NULL) {
			raise();
		}
	}
}

bool LockProcess::startHack()
{
    trinity_desktop_hack_active = TRUE;

    setCursor( tqblankCursor );
    XChangeActivePointerGrab( qt_xdisplay(), GRABEVENTS, TQCursor(tqblankCursor).handle(), CurrentTime);

    if ((mEnsureVRootWindowSecurityTimer) && (!mEnsureVRootWindowSecurityTimer->isActive())) mEnsureVRootWindowSecurityTimer->start(250, FALSE);

    if (currentDialog || (!mDialogs.isEmpty()))
    {
        // no resuming with dialog visible or when not visible
	if (backingPixmap.isNull()) {
		setBackgroundColor(black);
	}
	else {
		setBackgroundPixmap(backingPixmap);
	}
	setGeometry(0, 0, mRootWidth, mRootHeight);
	erase();
        return false;
    }

    if (mSaverExec.isEmpty())
    {
        return false;
    }

    if (mHackProc.isRunning())
    {
        stopHack();
    }

    mHackProc.clearArguments();

    TQTextStream ts(&mSaverExec, IO_ReadOnly);
    TQString word;
    ts >> word;
    TQString path = KStandardDirs::findExe(word);

    if (!path.isEmpty())
    {
        mHackProc << path;

        kdDebug(1204) << "Starting hack: " << path << endl;

        while (!ts.atEnd())
        {
            ts >> word;
            if (word == "%w")
            {
                word = word.setNum(winId());
            }
            mHackProc << word;
        }

	if (!mForbidden)
	{
		if (trinity_desktop_lock_use_system_modal_dialogs) {
			// Make sure we have a nice clean display to start with!
			if (backingPixmap.isNull()) {
				setBackgroundColor(black);
			}
			else {
				setBackgroundPixmap(backingPixmap);
			}
			setGeometry(0, 0, mRootWidth, mRootHeight);
			erase();
			mSuspended = false;
		}

		if (mHackProc.start() == true)
		{
#ifdef HAVE_SETPRIORITY
			setpriority(PRIO_PROCESS, mHackProc.pid(), mPriority);
#endif
 		        //bitBlt(this, 0, 0, &mOriginal);
 		        DISABLE_CONTINUOUS_LOCKDLG_DISPLAY
			if (trinity_desktop_lock_delay_screensaver_start && trinity_desktop_lock_forced) {
				// Close any active dialogs
				if (closeCurrentWindow()) {
					TQTimer::singleShot( 0, this, SLOT(closeCurrentWindow()) );
				}
			}
			if (m_startupStatusDialog) { m_startupStatusDialog->closeSMDialog(); m_startupStatusDialog=NULL; }
	                return true;
		}
	}
	else
	// we aren't allowed to start the specified screensaver either because it didn't run for some reason
	// according to the kiosk restrictions forbid it
	{
		usleep(100);
		TQApplication::syncX();
		if (!trinity_desktop_lock_use_system_modal_dialogs) {
			if (backingPixmap.isNull()) {
				setBackgroundColor(black);
			}
			else {
				setBackgroundPixmap(backingPixmap);
			}
		}
		if (backingPixmap.isNull()) {
			setGeometry(0, 0, mRootWidth, mRootHeight);
			erase();
		}
		else bitBlt(this, 0, 0, &backingPixmap);
		if (trinity_desktop_lock_use_system_modal_dialogs) {
			ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
			if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
		}
	}
    }
    if (m_startupStatusDialog) { m_startupStatusDialog->closeSMDialog(); m_startupStatusDialog=NULL; }
    return false;
}

//---------------------------------------------------------------------------
//
void LockProcess::stopHack()
{
    if (mHackProc.isRunning())
    {
        mHackProc.kill();
        if (!mHackProc.wait(10))
        {
            mHackProc.kill(SIGKILL);
        }
    }
    setCursor( tqarrowCursor );

    trinity_desktop_hack_active = FALSE;
}

//---------------------------------------------------------------------------
//
void LockProcess::hackExited(KProcess *)
{
	// Hack exited while we're supposed to be saving the screen.
	// Make sure the saver window is black.
	trinity_desktop_hack_active = FALSE;
	usleep(100);
	TQApplication::syncX();
	if (!trinity_desktop_lock_use_system_modal_dialogs) {
		if (backingPixmap.isNull()) {
			setBackgroundColor(black);
		}
		else {
			setBackgroundPixmap(backingPixmap);
		}
	}
	if (backingPixmap.isNull()) {
		setGeometry(0, 0, mRootWidth, mRootHeight);
		erase();
	}
	else bitBlt(this, 0, 0, &backingPixmap);
	if (!mSuspended) {
		if (trinity_desktop_lock_use_system_modal_dialogs) {
			ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
			if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
		}
	}
}

void LockProcess::displayLockDialogIfNeeded()
{
	if (m_startupStatusDialog) {
		m_startupStatusDialog->closeSMDialog(); m_startupStatusDialog=NULL;
	}
	if (!trinity_desktop_lock_in_sec_dlg) {
		if (trinity_desktop_lock_use_system_modal_dialogs) {
			if (!mBusy) {
				mBusy = true;
				if (mLocked) {
					if (checkPass()) {
						trinity_desktop_lock_closing_windows = true;
						stopSaver();
						kapp->quit();
					}
				}
				mBusy = false;
			}
		}
	}
}

void LockProcess::suspend()
{
    if(!mSuspended)
    {
        if (trinity_desktop_lock_use_system_modal_dialogs) {
            mSuspended = true;
            stopHack();
            ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
            if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
        }
        else {
            TQString hackStatus;
            mHackProc.kill(SIGSTOP);
            mSuspended = true;
#if 0
            // wait for the stop signal to take effect
            while (hackStatus != "T") {
                char hackstat[8192];
                FILE *fp = fopen(TQString("/proc/%1/stat").arg(mHackProc.pid()).ascii(),"r");
                if (fp != NULL) {
                    fgets (hackstat, 8192, fp);
                    fclose (fp);
                }
                hackstat[8191] = 0;
                hackStatus = hackstat;
                hackStatus = hackStatus.remove(TQRegExp("(*) ", TRUE, TRUE));
                TQStringList hackStatusList = TQStringList::split(" ", hackStatus);
                hackStatus = (*(hackStatusList.at(1)));
            }
#endif
            TQApplication::syncX();
            usleep(100000);		// Allow certain bad graphics drivers (*cough* fglrx *cough*) time to actually sync up the display
        }
        TQApplication::syncX();
        mSavedScreen = TQPixmap::grabWindow( winId());
    }
}

void LockProcess::resume( bool force )
{
    if (trinity_desktop_lock_use_sak && (mHackDelayStartupTimer->isActive() || !mHackStartupEnabled)) {
        return;
    }
    if( !force && (!mDialogs.isEmpty() || !mVisibility )) {
        // no resuming with dialog visible or when not visible
	if (trinity_desktop_lock_use_system_modal_dialogs) {
		if (backingPixmap.isNull()) {
			setBackgroundColor(black);
		}
		else {
			setBackgroundPixmap(backingPixmap);
		}
		setGeometry(0, 0, mRootWidth, mRootHeight);
		erase();
	}
	else {
		setGeometry(0, 0, mRootWidth, mRootHeight);
	}
        return;
    }
    if ((mSuspended) && (mHackProc.isRunning()))
    {
        XForceScreenSaver(qt_xdisplay(), ScreenSaverReset );
        bitBlt( this, 0, 0, &mSavedScreen );
        TQApplication::syncX();
        mHackProc.kill(SIGCONT);
        mSuspended = false;
    }
    else if (mSuspended && trinity_desktop_lock_use_system_modal_dialogs) {
        startHack();
    }
}

//---------------------------------------------------------------------------
//
// Show the password dialog
// This is called only in the master process
//
bool LockProcess::checkPass()
{
    if (!mDialogs.isEmpty()) {
        // Another dialog is already shown
        // Abort!
        return 0;
    }
    if (mInfoMessageDisplayed == false) {
        if (mAutoLogout) {
            killTimer(mAutoLogoutTimerId);
        }

        // Make sure we never launch the SAK or login dialog if windows are being closed down
        // Otherwise we can get stuck in an irrecoverable state where any attempt to show the login screen is instantly aborted
        if (trinity_desktop_lock_closing_windows)
            return 0;

        if (trinity_desktop_lock_use_sak) {
            // Verify SAK operational status
            KProcess* checkSAKProcess = new KProcess;
            *checkSAKProcess << "kdmtsak" << "check";
            checkSAKProcess->start(KProcess::Block, KProcess::NoCommunication);
            int retcode = checkSAKProcess->exitStatus();
            delete checkSAKProcess;
            if (retcode != 0) {
                trinity_desktop_lock_use_sak = false;
            }
        }

        if (trinity_desktop_lock_use_sak) {
            // Wait for SAK press before continuing...
            SAKDlg inDlg( this );
            execDialog( &inDlg );
            if (trinity_desktop_lock_closing_windows) {
                return 0;
            }
        }

        showVkbd();
        PasswordDlg passDlg( this, &greetPlugin, (mShowLockDateTime)?mlockDateTime:TQDateTime());
        int ret = execDialog( &passDlg );
        hideVkbd();

        if (mForceReject == true) {
            ret = TQDialog::Rejected;
        }
        mForceReject = false;

        XWindowAttributes rootAttr;
        XGetWindowAttributes(qt_xdisplay(), RootWindow(qt_xdisplay(),
                        qt_xscreen()), &rootAttr);
        if(( rootAttr.your_event_mask & SubstructureNotifyMask ) == 0 )
        {
            kdWarning() << "ERROR: Something removed SubstructureNotifyMask from the root window!!!" << endl;
            XSelectInput( qt_xdisplay(), qt_xrootwin(),
                SubstructureNotifyMask | rootAttr.your_event_mask );
        }

        return ret == TQDialog::Accepted;
    }
    else {
        return 0;
    }
}

static void fakeFocusIn( WId window )
{
    // We have keyboard grab, so this application will
    // get keyboard events even without having focus.
    // Fake FocusIn to make Qt realize it has the active
    // window, so that it will correctly show cursor in the dialog.
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xfocus.display = qt_xdisplay();
    ev.xfocus.type = FocusIn;
    ev.xfocus.window = window;
    ev.xfocus.mode = NotifyNormal;
    ev.xfocus.detail = NotifyAncestor;
    XSendEvent( qt_xdisplay(), window, False, NoEventMask, &ev );
}

void LockProcess::resumeUnforced()
{
    resume( false );
}

int LockProcess::execDialog( TQDialog *dlg )
{
    currentDialog=dlg;
    dlg->adjustSize();

    TQRect rect = dlg->geometry();
    rect.moveCenter(KGlobalSettings::desktopGeometry(TQCursor::pos()).center());
    dlg->move( rect.topLeft() );

    if (mDialogs.isEmpty())
    {
        suspend();
        XChangeActivePointerGrab( qt_xdisplay(), GRABEVENTS,
                TQCursor(tqarrowCursor).handle(), CurrentTime);
    }
    mDialogs.prepend( dlg );
    fakeFocusIn( dlg->winId());
    if (trinity_desktop_lock_use_system_modal_dialogs) {
        if (backingPixmap.isNull()) {
            setGeometry(0, 0, mRootWidth, mRootHeight);
            erase();
        }
        else {
            bitBlt(this, 0, 0, &backingPixmap);
        }
    }
    // dlg->exec may generate BadMatch errors, so make sure they are silently ignored
    int (*oldHandler)(Display *, XErrorEvent *);
    oldHandler = XSetErrorHandler(ignoreXError);
    int rt = dlg->exec();
    XSetErrorHandler(oldHandler);
    while (mDialogControlLock == true) usleep(100000);
    currentDialog = NULL;
    mDialogs.remove( dlg );
    if( mDialogs.isEmpty() ) {
        HANDLE cursorHandle;
        if (trinity_desktop_hack_active) {
           cursorHandle = TQCursor(tqblankCursor).handle();
        }
        else {
           cursorHandle = TQCursor(tqbusyCursor).handle();
        }
        XChangeActivePointerGrab( qt_xdisplay(), GRABEVENTS,
                cursorHandle, CurrentTime);
        if (trinity_desktop_lock_use_system_modal_dialogs) {
            // Slight delay before screensaver resume to allow the dialog window to fully disappear
            if (hackResumeTimer == NULL) {
                hackResumeTimer = new TQTimer( this );
                connect( hackResumeTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(resumeUnforced()) );
            }
            if (mResizingDesktopLock == false) {
                hackResumeTimer->start( 10, TRUE );
            }
        }
        else {
            resume( false );
        }
    } else {
        fakeFocusIn( mDialogs.first()->winId());
        currentDialog = dynamic_cast<TQDialog*>(mDialogs.first());
    }
    return rt;
}

void LockProcess::slotForcePaintBackground()
{
	TQPixmap blankPixmap(mRootWidth, mRootHeight);
	blankPixmap.fill(Qt::black);
	slotPaintBackground(blankPixmap);
	printf("[WARNING] Unable to obtain desktop wallpaper in a timely manner.  High system load or possibly a TDE bug!\n\r"); fflush(stdout);
}

void LockProcess::slotPaintBackground(const TQPixmap &rpm)
{
	if (mEnsureScreenHiddenTimer) {
		mEnsureScreenHiddenTimer->stop();
	}
	else {
		mEnsureScreenHiddenTimer = new TQTimer( this );
		connect( mEnsureScreenHiddenTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotForcePaintBackground()) );
	}

	// Only remove the mask widget once the resize is 100% complete!
	if (m_maskWidget) {
		delete m_maskWidget;
		m_maskWidget = NULL;
		XSync(qt_xdisplay(), False);
	}

	TQPixmap pm = rpm;

	if (TQPaintDevice::x11AppDepth() == 32) {
		// Remove the alpha components from the image
		TQImage correctedImage = pm.convertToImage();
		correctedImage = correctedImage.convertDepth(32);
		correctedImage.setAlphaBuffer(true);
		int w = correctedImage.width();
		int h = correctedImage.height();
		for (int y = 0; y < h; ++y) {
			TQRgb *ls = (TQRgb *)correctedImage.scanLine( y );
			for (int x = 0; x < w; ++x) {
				TQRgb l = ls[x];
				int r = int( tqRed( l ) );
				int g = int( tqGreen( l ) );
				int b = int( tqBlue( l ) );
				int a = int( 255 );
				ls[x] = tqRgba( r, g, b, a );
			}
		}
		pm.convertFromImage(correctedImage);
	}

	backingPixmap = pm;
	if ((trinity_desktop_lock_delay_screensaver_start && trinity_desktop_lock_forced) || (!mHackStartupEnabled)) {
		setBackgroundPixmap(backingPixmap);
		setGeometry(0, 0, mRootWidth, mRootHeight);
		erase();
	}
}

void LockProcess::preparePopup()
{
    TQWidget *dlg = (TQWidget *)sender();
    mDialogs.prepend( dlg );
    fakeFocusIn( dlg->winId() );
}

void LockProcess::cleanupPopup()
{
    TQWidget *dlg = (TQWidget *)sender();
    mDialogs.remove( dlg );
    if ( mDialogs.isEmpty() ) {
        fakeFocusIn( mDialogs.first()->winId() );
    }
}

void LockProcess::doFunctionKeyBroadcast() {
    // Provide a clean, pretty display switch by hiding the password dialog here
    // This does NOT work with the SAK or system modal dialogs!
    if ((!trinity_desktop_lock_use_system_modal_dialogs) && (!trinity_desktop_lock_use_sak)) {
        mBusy=true;
        TQTimer::singleShot(1000, this, TQT_SLOT(slotDeadTimePassed()));
        if (mkeyCode == XKeysymToKeycode(qt_xdisplay(), XF86XK_Display)) {
            while (mDialogControlLock == true) usleep(100000);
            mDialogControlLock = true;
            currentDialog->close();		// DO NOT use closeCurrentWindow() here!
            mDialogControlLock = false;
        }
    }

    DCOPRef ref( "*", "MainApplication-Interface");
    ref.send("sendFakeKey", DCOPArg(mkeyCode , "unsigned int"));
}

//---------------------------------------------------------------------------
//
// X11 Event.
//
bool LockProcess::x11Event(XEvent *event)
{
    // Allow certain very specific keypresses through
    // Key:			Reason:
    // XF86Display		You need to be able to see the screen when unlocking your computer
    // XF86AudioMute		Would be nice to be able to shut your computer up in an emergency while it is locked
    // XF86AudioRaiseVolume	Ditto
    // XF86AudioLowerVolume	Ditto

    //if ((event->type == KeyPress) || (event->type == KeyRelease)) {
    if (event->type == KeyPress) {
        if ((event->xkey.keycode == XKeysymToKeycode(event->xkey.display, XF86XK_Display)) || \
        (event->xkey.keycode == XKeysymToKeycode(event->xkey.display, XF86XK_AudioMute)) || \
        (event->xkey.keycode == XKeysymToKeycode(event->xkey.display, XF86XK_AudioRaiseVolume)) || \
        (event->xkey.keycode == XKeysymToKeycode(event->xkey.display, XF86XK_AudioLowerVolume))) {
            mkeyCode = event->xkey.keycode;
            TQTimer::singleShot( 100, this, TQT_SLOT(doFunctionKeyBroadcast()) );
            return true;
        }
    }

    switch (event->type)
    {
        case ButtonPress:
        case MotionNotify:
        case ButtonRelease:
            if( forwardVkbdEvent( event ))
                return true; // filter out
            // fall through
        case KeyPress:
            if ((mHackDelayStartupTimer) && (mHackDelayStartupTimer->isActive())) {
                if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
            }
            if (mBusy || !mDialogs.isEmpty())
                break;
            mBusy = true;
            if (trinity_desktop_lock_delay_screensaver_start) {
                if (mLocked) {
                    ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
                    if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
                }
                if ((!mLocked) && (!trinity_desktop_lock_in_sec_dlg))
                {
                    stopSaver();
                    kapp->quit();
                }
                if (mAutoLogout) // we need to restart the auto logout countdown
                {
                    killTimer(mAutoLogoutTimerId);
                    mAutoLogoutTimerId = startTimer(mAutoLogoutTimeout);
                }
            }
            else {
                if (!mLocked || checkPass())
                {
                    trinity_desktop_lock_closing_windows = true;
                    stopSaver();
                    kapp->quit();
                }
                else if (mAutoLogout) // we need to restart the auto logout countdown
                {
                    killTimer(mAutoLogoutTimerId);
                    mAutoLogoutTimerId = startTimer(mAutoLogoutTimeout);
                }
            }
            mBusy = false;
            return true;

        case VisibilityNotify:
            if( event->xvisibility.window == winId())
            {  // mVisibility == false means the screensaver is not visible at all
               // e.g. when switched to text console
                mVisibility = !(event->xvisibility.state == VisibilityFullyObscured);
                if(!mVisibility) {
                    mSuspendTimer.start(2000, true);
                }
                else
                {
                    mSuspendTimer.stop();
                    if (mResizingDesktopLock == false) {
			if (trinity_desktop_lock_delay_screensaver_start && trinity_desktop_lock_forced && trinity_desktop_lock_use_system_modal_dialogs) {
				// Do nothing
			}
			else {
				if (mHackStartupEnabled == true) {
					resume( false );
				}
				else {
					if (trinity_desktop_lock_use_system_modal_dialogs == true) {
						ENABLE_CONTINUOUS_LOCKDLG_DISPLAY
						if (mHackStartupEnabled) mHackDelayStartupTimer->start(mHackDelayStartupTimeout, TRUE);
					}
					else {
						resume( false );
					}
				}
			}
                    }
                }
                if (event->xvisibility.state != VisibilityUnobscured) {
                    stayOnTop();
                }
            }
            break;

        case ConfigureNotify: // from SubstructureNotifyMask on the root window
            if(event->xconfigure.event == qt_xrootwin()) {
                stayOnTop();
            }
            for( TQValueList< VkbdWindow >::Iterator it = mVkbdWindows.begin();
                 it != mVkbdWindows.end();
                 ++it ) {
                if( (*it).id == event->xconfigure.window ) {
                    (*it).rect = TQRect( event->xconfigure.x, event->xconfigure.y,
                        event->xconfigure.width, event->xconfigure.height );
                    break;
                }
            }
            break;
        case MapNotify: // from SubstructureNotifyMask on the root window
            windowAdded( event->xmap.window, false );
            if( event->xmap.event == qt_xrootwin())
                stayOnTop();
            break;
        case DestroyNotify:
            for( TQValueList< VkbdWindow >::Iterator it = mVkbdWindows.begin();
                 it != mVkbdWindows.end();
                 ++it )
                if( (*it).id == event->xdestroywindow.window ) {
                    mVkbdWindows.remove( it );
                    break;
            }
            break;
    }

    // We have grab with the grab window being the root window.
    // This results in key events being sent to the root window,
    // but they should be sent to the dialog if it's visible.
    // It could be solved by setFocus() call, but that would mess
    // the focus after this process exits.
    // Qt seems to be quite hard to persuade to redirect the event,
    // so let's simply dupe it with correct destination window,
    // and ignore the original one.
    if(!mDialogs.isEmpty() && ( event->type == KeyPress || event->type == KeyRelease)
        && event->xkey.window != mDialogs.first()->winId())
    {
        XEvent ev2 = *event;
        ev2.xkey.window = ev2.xkey.subwindow = mDialogs.first()->winId();
        tqApp->x11ProcessEvent( &ev2 );
        return true;
    }

    return false;
}

void LockProcess::stayOnTop()
{
    if(!mDialogs.isEmpty() || !mVkbdWindows.isEmpty())
    {
        // this restacking is written in a way so that
        // if the stacking positions actually don't change,
        // all restacking operations will be no-op,
        // and no ConfigureNotify will be generated,
        // thus avoiding possible infinite loops
        if( !mVkbdWindows.isEmpty())
            XRaiseWindow( qt_xdisplay(), mVkbdWindows.first().id );
        else
            XRaiseWindow( qt_xdisplay(), mDialogs.first()->winId()); // raise topmost
        // and stack others below it
        Window* stack = new Window[ mDialogs.count() + mVkbdWindows.count() + 1 ];
        int count = 0;
        for( TQValueList< VkbdWindow >::ConstIterator it = mVkbdWindows.begin();
             it != mVkbdWindows.end();
             ++it )
            stack[ count++ ] = (*it).id;
        for( TQValueList< TQWidget* >::ConstIterator it = mDialogs.begin();
             it != mDialogs.end();
             ++it )
            stack[ count++ ] = (*it)->winId();
        stack[ count++ ] = winId();
        XRestackWindows( x11Display(), stack, count );
        delete[] stack;
    }
    else
        XRaiseWindow(qt_xdisplay(), winId());
}

void LockProcess::checkDPMSActive()
{
#ifdef HAVE_DPMS
    BOOL on;
    CARD16 state;
    DPMSInfo(qt_xdisplay(), &state, &on);
    //kdDebug() << "checkDPMSActive " << on << " " << state << endl;
    if (state == DPMSModeStandby || state == DPMSModeSuspend || state == DPMSModeOff)
    {
       suspend();
    } else if ( mSuspended )
    {
        if (mResizingDesktopLock == false) {
            resume( true );
        }
    }
#endif
}

#if defined(HAVE_XF86MISC) && defined(HAVE_XF86MISCSETGRABKEYSSTATE)
// see http://cvsweb.xfree86.org/cvsweb/xc/programs/Xserver/hw/xfree86/common/xf86Events.c#rev3.113
// This allows enabling the "Allow{Deactivate/Closedown}Grabs" options in XF86Config,
// and kdesktop_lock will still lock the session.
static enum { Unknown, Yes, No } can_do_xf86_lock = Unknown;
void LockProcess::lockXF86()
{
    if( can_do_xf86_lock == Unknown )
    {
        int major, minor;
        if( XF86MiscQueryVersion( qt_xdisplay(), &major, &minor )
            && major >= 0 && minor >= 5 )
            can_do_xf86_lock = Yes;
        else
            can_do_xf86_lock = No;
    }
    if( can_do_xf86_lock != Yes )
        return;
    if( mRestoreXF86Lock )
        return;
    if( XF86MiscSetGrabKeysState( qt_xdisplay(), False ) != MiscExtGrabStateSuccess )
        return;
    // success
    mRestoreXF86Lock = true;
}

void LockProcess::unlockXF86()
{
    if( can_do_xf86_lock != Yes )
        return;
    if( !mRestoreXF86Lock )
        return;
    XF86MiscSetGrabKeysState( qt_xdisplay(), True );
    mRestoreXF86Lock = false;
}
#else
void LockProcess::lockXF86()
{
}

void LockProcess::unlockXF86()
{
}
#endif

void LockProcess::msgBox( TQMessageBox::Icon type, const TQString &txt )
{
    TQDialog box( 0, "messagebox", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM)) );
    if (trinity_desktop_lock_use_system_modal_dialogs) {
        // Signal that we do not want any window controls to be shown at all
        Atom kde_wm_system_modal_notification;
        kde_wm_system_modal_notification = XInternAtom(qt_xdisplay(), "_KDE_WM_MODAL_SYS_NOTIFICATION", False);
        XChangeProperty(qt_xdisplay(), box.winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
    }
    box.setCaption(i18n("Authentication Subsystem Notice"));
    TQFrame *winFrame = new TQFrame( &box );
    if (trinity_desktop_lock_use_system_modal_dialogs)
        winFrame->setFrameStyle( TQFrame::NoFrame );
    else
        winFrame->setFrameStyle( TQFrame::WinPanel | TQFrame::Raised );
    winFrame->setLineWidth( 2 );
    TQLabel *label1 = new TQLabel( winFrame );
    label1->setPixmap( TQMessageBox::standardIcon( type ) );
    TQLabel *label2 = new TQLabel( txt, winFrame );
    KPushButton *button = new KPushButton( KStdGuiItem::ok(), winFrame );
    button->setDefault( true );
    button->setSizePolicy( TQSizePolicy( TQSizePolicy::Preferred, TQSizePolicy::Preferred ) );
    connect( button, TQT_SIGNAL( clicked() ), &box, TQT_SLOT( accept() ) );

    TQVBoxLayout *vbox = new TQVBoxLayout( &box );
    vbox->addWidget( winFrame );
    TQGridLayout *grid = new TQGridLayout( winFrame, 2, 2, 10 );
    grid->addWidget( label1, 0, 0, Qt::AlignCenter );
    grid->addWidget( label2, 0, 1, Qt::AlignCenter );
    grid->addMultiCellWidget( button, 1,1, 0,1, Qt::AlignCenter );

    execDialog( &box );
}

static int run_vkbd = -1;
void LockProcess::showVkbd()
{
    if( run_vkbd == - 1 ) {
        int status = system( "hal-find-by-property --key system.formfactor.subtype --string tabletpc" );
//        status = 0; // enable for testing
        run_vkbd = ( WIFEXITED( status ) && WEXITSTATUS( status ) == 0
            && !KStandardDirs::findExe( "xvkbd" ).isEmpty()) ? 1 : 0;
    }
    if( run_vkbd ) {
        mVkbdWindows.clear();
        mVkbdLastEventWindow = None;
        mKWinModule = new KWinModule( NULL, KWinModule::INFO_WINDOWS );
        connect( mKWinModule, TQT_SIGNAL( windowAdded( WId )), TQT_SLOT( windowAdded( WId )));
        mVkbdProcess = new KProcess;
        *mVkbdProcess << "xvkbd" << "-compact" << "-geometry" << "-0-0" << "-xdm";
        mVkbdProcess->start();
    }
}

void LockProcess::hideVkbd()
{
    if( mVkbdProcess != NULL ) {
        mVkbdProcess->kill();
        delete mVkbdProcess;
        mVkbdProcess = NULL;
        delete mKWinModule;
        mKWinModule = NULL;
        mVkbdWindows.clear();
    }
}

void LockProcess::windowAdded( WId w )
{
    windowAdded( w, true );
}

void LockProcess::windowAdded( WId w, bool managed )
{
    // KWin::windowInfo may generate BadWindow errors, so make sure they are silently ignored
    int (*oldHandler)(Display *, XErrorEvent *);
    oldHandler = XSetErrorHandler(ignoreXError);
    KWin::WindowInfo info = KWin::windowInfo( w, 0, NET::WM2WindowClass );
    XSetErrorHandler(oldHandler);

    if( info.windowClassClass().lower() != "xvkbd" )
        return;
    // Unmanaged windows (i.e. popups) don't currently work anyway, since they
    // don't have WM_CLASS set anyway. I could perhaps try tricks with X id
    // ranges if really needed.
    if( managed ) {
        // withdraw the window, wait for it to be withdrawn, reparent it directly
        // to root at the right position
        XWithdrawWindow( qt_xdisplay(), w, qt_xscreen());
        for(;;) {
            Atom type;
            int format;
            unsigned long length, after;
            unsigned char *data;
            int r = XGetWindowProperty( qt_xdisplay(), w, qt_wm_state, 0, 2,
                                        false, AnyPropertyType, &type, &format,
                                        &length, &after, &data );
            bool withdrawn = true;
            if ( r == Success && data && format == 32 ) {
                TQ_UINT32 *wstate = (TQ_UINT32*)data;
                withdrawn  = (*wstate == WithdrawnState );
                XFree( (char *)data );
            }
            if( withdrawn )
                break;
        }
    }
    XSelectInput( qt_xdisplay(), w, StructureNotifyMask );
    XWindowAttributes attr_geom;
    if( !XGetWindowAttributes( qt_xdisplay(), w, &attr_geom ))
        return;
    int x = XDisplayWidth( qt_xdisplay(), qt_xscreen()) - attr_geom.width;
    int y = XDisplayHeight( qt_xdisplay(), qt_xscreen()) - attr_geom.height;
    if( managed ) {
        XSetWindowAttributes attr;
        if (!trinity_desktop_lock_use_system_modal_dialogs) {
            attr.override_redirect = True;
            XChangeWindowAttributes( qt_xdisplay(), w, CWOverrideRedirect, &attr );
        }
        XReparentWindow( qt_xdisplay(), w, qt_xrootwin(), x, y );
        XMapWindow( qt_xdisplay(), w );
    }
    VkbdWindow data;
    data.id = w;
    data.rect = TQRect( x, y, attr_geom.width, attr_geom.height );
    mVkbdWindows.prepend( data );
}

bool LockProcess::forwardVkbdEvent( XEvent* event )
{
    if( mVkbdProcess == NULL )
        return false;
    TQPoint pos;
    Time time;
    switch( event->type )
    {
        case ButtonPress:
        case ButtonRelease:
            pos = TQPoint( event->xbutton.x, event->xbutton.y );
            time = event->xbutton.time;
            break;
        case MotionNotify:
            pos = TQPoint( event->xmotion.x, event->xmotion.y );
            time = event->xmotion.time;
            break;
        default:
            return false;
    }
    // vkbd windows are kept topmost, so just find the first one in the position
    for( TQValueList< VkbdWindow >::ConstIterator it = mVkbdWindows.begin();
         it != mVkbdWindows.end();
         ++it ) {
        if( TQT_TQRECT_OBJECT((*it).rect).contains( pos )) {
            // Find the subwindow where the event should actually go.
            // Not exactly cheap in the number of X roundtrips but oh well.
            Window window = (*it).id;
            Window root, child;
            int root_x, root_y, x, y;
            unsigned int mask;
            for(;;) {
                if( !XQueryPointer( qt_xdisplay(), window, &root, &child, &root_x, &root_y, &x, &y, &mask ))
                    return false;
                if( child == None )
                    break;
                window = child;
            }
            switch( event->type )
            {
                case ButtonPress:
                case ButtonRelease:
                    event->xbutton.x = x;
                    event->xbutton.y = y;
                    event->xbutton.subwindow = None;
                    break;
                case MotionNotify:
                    event->xmotion.x = x;
                    event->xmotion.y = y;
                    event->xmotion.subwindow = None;
                    break;
            }
            event->xany.window = window;
            sendVkbdFocusInOut( window, time );
            XSendEvent( qt_xdisplay(), window, False, 0, event );
            return true;
        }
    }
    sendVkbdFocusInOut( None, time );
    return false;
}

// Fake EnterNotify/LeaveNotify events as the mouse moves. They're not sent by X
// because of the grab and having them makes xvkbd highlight the buttons (but
// not needed otherwise it seems).
void LockProcess::sendVkbdFocusInOut( WId window, Time t )
{
    if( mVkbdLastEventWindow == window )
        return;
    if( mVkbdLastEventWindow != None ) {
        XEvent e;
        e.xcrossing.type = LeaveNotify;
        e.xcrossing.display = qt_xdisplay();
        e.xcrossing.window = mVkbdLastEventWindow;
        e.xcrossing.root = qt_xrootwin();
        e.xcrossing.subwindow = None;
        e.xcrossing.time = t;
        e.xcrossing.x = 0;
        e.xcrossing.y = 0;
        e.xcrossing.x_root = -1;
        e.xcrossing.y_root = -1;
        e.xcrossing.mode = NotifyNormal;
        e.xcrossing.detail = NotifyAncestor;
        e.xcrossing.same_screen = True;
        e.xcrossing.focus = False;
        e.xcrossing.state = 0;
        XSendEvent( qt_xdisplay(), mVkbdLastEventWindow, False, 0, &e );
    }
    mVkbdLastEventWindow = window;
    if( mVkbdLastEventWindow != None ) {
        XEvent e;
        e.xcrossing.type = EnterNotify;
        e.xcrossing.display = qt_xdisplay();
        e.xcrossing.window = mVkbdLastEventWindow;
        e.xcrossing.root = qt_xrootwin();
        e.xcrossing.subwindow = None;
        e.xcrossing.time = t;
        e.xcrossing.x = 0;
        e.xcrossing.y = 0;
        e.xcrossing.x_root = 0;
        e.xcrossing.y_root = 0;
        e.xcrossing.mode = NotifyNormal;
        e.xcrossing.detail = NotifyAncestor;
        e.xcrossing.same_screen = True;
        e.xcrossing.focus = False;
        e.xcrossing.state = 0;
        XSendEvent( qt_xdisplay(), mVkbdLastEventWindow, False, 0, &e );
    }
}

void LockProcess::slotMouseActivity(XEvent *event)
{
	bool inFrame = 0;
	bool inDialog = 0;
	XButtonEvent *be = (XButtonEvent *) event;
	XMotionEvent *me = (XMotionEvent *) event;
	if ((event->type == ButtonPress) && (!mDialogs.isEmpty())) {
		// Get geometry including window frame/titlebar
		TQRect fgeom = mDialogs.first()->frameGeometry();
		TQRect wgeom = mDialogs.first()->geometry();

		if (((be->x_root > fgeom.x()) && (be->y_root > fgeom.y())) && ((be->x_root < (fgeom.x()+fgeom.width())) && (be->y_root < (fgeom.y()+fgeom.height())))) {
			inFrame = 1;
		}
		if (((be->x_root > wgeom.x()) && (be->y_root > wgeom.y())) && ((be->x_root < (wgeom.x()+wgeom.width())) && (be->y_root < (wgeom.y()+wgeom.height())))) {
			inDialog = 1;
		}

		// Clicked inside dialog; set focus
		if (inFrame == TRUE) {
			WId window = mDialogs.first()->winId();
			XSetInputFocus(qt_xdisplay(), window, RevertToParent, CurrentTime);
			fakeFocusIn(window);
			// Why this needs to be repeated I have no idea...
			XSetInputFocus(qt_xdisplay(), window, RevertToParent, CurrentTime);
			fakeFocusIn(window);
		}

		// Clicked inside window handle (or border); drag window
		if ((inFrame == TRUE) && (inDialog == FALSE)) {
			TQPoint oldPoint = mDialogs.first()->pos();
			m_mouseDown = 1;
			m_dialogPrevX = oldPoint.x();
			m_dialogPrevY = oldPoint.y();
			m_mousePrevX = be->x_root;
			m_mousePrevY = be->y_root;
			XChangeActivePointerGrab( qt_xdisplay(), GRABEVENTS, TQCursor(tqsizeAllCursor).handle(), CurrentTime);
		}
	}

	// Drag the window...
	if (event->type == MotionNotify) {
		if (m_mouseDown == TRUE) {
			int deltaX = me->x_root - m_mousePrevX;
			int deltaY = me->y_root - m_mousePrevY;
			m_dialogPrevX = m_dialogPrevX + deltaX;
			m_dialogPrevY = m_dialogPrevY + deltaY;
			if (!mDialogs.isEmpty()) mDialogs.first()->move(m_dialogPrevX, m_dialogPrevY);

			m_mousePrevX = me->x_root;
			m_mousePrevY = me->y_root;
		}
	}

	if (event->type == ButtonRelease) {
		m_mouseDown = 0;
		XChangeActivePointerGrab( qt_xdisplay(), GRABEVENTS, TQCursor(tqarrowCursor).handle(), CurrentTime);
	}
}

#include "lockprocess.moc"
