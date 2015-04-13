/* This file is part of the KDE project
   Copyright (C) 1999 David Faure
   Copyright (c) 2003 Oswald Buddenhagen <ossi@kde.org>
   Copyright (c) 2010-2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <config.h>

#include "lockprocess.h"
#include "main.h"
#include "kdesktopsettings.h"

#include <tqfileinfo.h>

#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <kdebug.h>
#include <tdeglobalsettings.h>
#include <dcopref.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

#include <tdmtsak.h>

#include <stdlib.h>

#if defined(Q_WS_X11) && defined(HAVE_XRENDER) && TQT_VERSION >= 0x030300
#define COMPOSITE
#endif

#ifdef COMPOSITE
# include <X11/Xlib.h>
# include <X11/extensions/Xrender.h>
# include <fixx11h.h>
# include <dlfcn.h>
#else
# include <X11/Xlib.h>
# include <fixx11h.h>
#endif

#define OPEN_TDMCONFIG_AND_SET_GROUP									\
if( stat( KDE_CONFDIR "/tdm/tdmdistrc" , &st ) == 0) {							\
	tdmconfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdm/tdmdistrc" ));		\
}													\
else {													\
	tdmconfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdm/tdmrc" ));		\
}													\
tdmconfig->setGroup("X-:*-Greeter");

TQXLibWindowList trinity_desktop_lock_hidden_window_list;

// [FIXME] Add GUI configuration checkboxes for these three settings (see kdesktoprc [ScreenSaver] UseUnmanagedLockWindows, DelaySaverStart, and UseTDESAK)
bool trinity_desktop_lock_use_system_modal_dialogs = FALSE;
bool trinity_desktop_lock_delay_screensaver_start = FALSE;
bool trinity_desktop_lock_use_sak = FALSE;
bool trinity_desktop_lock_hide_active_windows = FALSE;
bool trinity_desktop_lock_hide_cancel_button = FALSE;
bool trinity_desktop_lock_forced = FALSE;

LockProcess* trinity_desktop_lock_process = NULL;

bool signalled_forcelock;
bool signalled_dontlock;
bool signalled_securedialog;
bool signalled_blank;
bool signalled_run;
bool in_internal_mode = FALSE;

bool argb_visual = FALSE;
pid_t kdesktop_pid = -1;
bool trinity_desktop_lock_settings_initialized = FALSE;

static void sigusr1_handler(int)
{
    signalled_forcelock = TRUE;
}

static void sigusr2_handler(int)
{
    signalled_dontlock = TRUE;
}

static void sigusr3_handler(int)
{
    signalled_securedialog = TRUE;
}

static void sigusr4_handler(int)
{
    signalled_blank = TRUE;
}

static void sigusr5_handler(int)
{
    signalled_run = TRUE;
}

static int trapXErrors(Display *, XErrorEvent *)
{
    return 0;
}

bool MyApp::x11EventFilter( XEvent *ev )
{
    if (ev->type == ButtonPress || ev->type == ButtonRelease || ev->type == MotionNotify) {
        emit mouseInteraction(ev);
    }
    if (ev->type == XKeyPress || ev->type == ButtonPress) {
        emit activity();
    }
    else if (ev->type == MotionNotify) {
        time_t tick = time( 0 );
        if (tick != lastTick) {
            lastTick = tick;
            emit activity();
        }
    }
    else if (ev->type == MapNotify) {
        // HACK
        // Hide all tooltips and notification windows
        XMapEvent map_event = ev->xmap;
        XWindowAttributes childAttr;
        Window childTransient;
        if (XGetWindowAttributes(map_event.display, map_event.window, &childAttr) && XGetTransientForHint(map_event.display, map_event.window, &childTransient)) {
            if((childAttr.map_state == IsViewable) && (childAttr.override_redirect) && (childTransient)) {
                if (!trinity_desktop_lock_hidden_window_list.contains(map_event.window)) {
                    trinity_desktop_lock_hidden_window_list.append(map_event.window);
                }
                XLowerWindow(map_event.display, map_event.window);
                XFlush(map_event.display);
            }
        }
    }
    else if (ev->type == VisibilityNotify) {
        // HACK
        // Hide all tooltips and notification windows
        XVisibilityEvent visibility_event = ev->xvisibility;
        XWindowAttributes childAttr;
        Window childTransient;
        if ((visibility_event.state == VisibilityUnobscured) || (visibility_event.state == VisibilityPartiallyObscured)) {
            if (XGetWindowAttributes(visibility_event.display, visibility_event.window, &childAttr) && XGetTransientForHint(visibility_event.display, visibility_event.window, &childTransient)) {
                if((childAttr.map_state == IsViewable) && (childAttr.override_redirect) && (childTransient)) {
                    if (!trinity_desktop_lock_hidden_window_list.contains(visibility_event.window)) {
                        trinity_desktop_lock_hidden_window_list.append(visibility_event.window);
                    }
                    XLowerWindow(visibility_event.display, visibility_event.window);
                    XFlush(visibility_event.display);
                }
            }
        }
    }
    else if (ev->type == CreateNotify) {
        // HACK
        // Close all tooltips and notification windows
        XCreateWindowEvent create_event = ev->xcreatewindow;
        XWindowAttributes childAttr;
        Window childTransient;

        // XGetWindowAttributes may generate BadWindow errors, so make sure they are silently ignored
        int (*oldHandler)(Display *, XErrorEvent *);
        oldHandler = XSetErrorHandler(trapXErrors);
        if (XGetWindowAttributes(create_event.display, create_event.window, &childAttr) && XGetTransientForHint(create_event.display, create_event.window, &childTransient)) {
            if ((childAttr.override_redirect) && (childTransient)) {
                if (!trinity_desktop_lock_hidden_window_list.contains(create_event.window)) {
                    trinity_desktop_lock_hidden_window_list.append(create_event.window);
                }
                XLowerWindow(create_event.display, create_event.window);
                XFlush(create_event.display);
            }
        }
        XSetErrorHandler(oldHandler);
    }
    else if (ev->type == DestroyNotify) {
        XDestroyWindowEvent destroy_event = ev->xdestroywindow;
        if (trinity_desktop_lock_hidden_window_list.contains(destroy_event.window)) {
            trinity_desktop_lock_hidden_window_list.remove(destroy_event.window);
        }
    }
#if 0
    else if (ev->type == CreateNotify) {
        // HACK
        // Close all tooltips and notification windows
        XCreateWindowEvent create_event = ev->xcreatewindow;
        XWindowAttributes childAttr;
        Window childTransient;
        if (XGetWindowAttributes(create_event.display, create_event.window, &childAttr) && XGetTransientForHint(create_event.display, create_event.window, &childTransient)) {
            if ((childAttr.override_redirect) && (childTransient)) {
                XDestroyWindow(create_event.display, create_event.window);
            }
        }
    }
#endif
    return TDEApplication::x11EventFilter( ev );
}


static TDECmdLineOptions options[] =
{
   { "forcelock", I18N_NOOP("Force session locking"), 0 },
   { "dontlock", I18N_NOOP("Only start screensaver"), 0 },
   { "securedialog", I18N_NOOP("Launch the secure dialog"), 0 },
   { "blank", I18N_NOOP("Only use the blank screensaver"), 0 },
   { "internal <pid>", I18N_NOOP("TDE internal command for background process loading"), 0 },
   TDECmdLineLastOption
};

void restore_hidden_override_redirect_windows() {
    TQXLibWindowList::iterator it;
    for (it = trinity_desktop_lock_hidden_window_list.begin(); it != trinity_desktop_lock_hidden_window_list.end(); ++it) {
        Window win = *it;
        XRaiseWindow(tqt_xdisplay(), win);
    }
}

// -----------------------------------------------------------------------------

int main( int argc, char **argv )
{
    TDELocale::setMainCatalogue("kdesktop");

    TDECmdLineArgs::init( argc, argv, "kdesktop_lock", I18N_NOOP("KDesktop Locker"), I18N_NOOP("Session Locker for KDesktop"), "2.1" );
    TDECmdLineArgs::addCmdLineOptions( options );
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

    putenv(strdup("SESSION_MANAGER="));

    TDEApplication::disableAutoDcopRegistration(); // not needed

    XSetErrorHandler(trapXErrors);

    MyApp* app = NULL;

    while (1 == 1) {
        signalled_forcelock = FALSE;
        signalled_dontlock = FALSE;
        signalled_securedialog = FALSE;
        signalled_blank = FALSE;
        signalled_run = FALSE;

        int kdesktop_screen_number = 0;
        int starting_screen = 0;

        bool child = false;
        int parent_connection = 0; // socket to the parent saver
        TQValueList<int> child_sockets;

        if (TDEGlobalSettings::isMultiHead())
        {
            Display *dpy = XOpenDisplay(NULL);
            if (! dpy) {
                fprintf(stderr,
                        "%s: FATAL ERROR: couldn't open display '%s'\n",
                        argv[0], XDisplayName(NULL));
                exit(1);
            }

            int number_of_screens = ScreenCount(dpy);
            starting_screen = kdesktop_screen_number = DefaultScreen(dpy);
            int pos;
            TQCString display_name = XDisplayString(dpy);
            XCloseDisplay(dpy);
            kdDebug() << "screen " << number_of_screens << " " << kdesktop_screen_number << " " << display_name << " " << starting_screen << endl;
            dpy = 0;

            if ((pos = display_name.findRev('.')) != -1)
                display_name.remove(pos, 10);

            TQCString env;
            if (number_of_screens != 1) {
                for (int i = 0; i < number_of_screens; i++) {
                    if (i != starting_screen) {
                        int fd[2];
                        if (pipe(fd)) {
                            perror("pipe");
                            break;
                        }
                        if (fork() == 0) {
                            child = true;
                            kdesktop_screen_number = i;
                            parent_connection = fd[0];
                            // break here because we are the child process, we don't
                            // want to fork() anymore
                            break;
                        } else {
                            child_sockets.append(fd[1]);
                        }
                    }
                }

                env.sprintf("DISPLAY=%s.%d", display_name.data(),
                            kdesktop_screen_number);
                kdDebug() << "env " << env << endl;

                if (putenv(strdup(env.data()))) {
                    fprintf(stderr,
                            "%s: WARNING: unable to set DISPLAY environment variable\n",
                            argv[0]);
                    perror("putenv()");
                }
            }
        }

        if (!app) {
#ifdef COMPOSITE
            app = new MyApp(TDEApplication::openX11RGBADisplay());
            argb_visual = app->isX11CompositionAvailable();
#else
            app = new MyApp;
#endif
        }

        TDELockFile lock(locateLocal("tmp", "kdesktop_lock_lockfile"));
        lock.setStaleTime(0);
        TDELockFile::LockResult lockRet = lock.lock();
        if (lockRet != TDELockFile::LockOK) {
            // Terminate existing (stale) process if needed
            int pid;
            TQString hostName;
            TQString appName;
            if (lock.getLockInfo(pid, hostName, appName)) {
                // Verify that the pid in question is an instance of kdesktop_lock
                int len;
                char procpath[PATH_MAX];
                char fullpath[PATH_MAX];
                snprintf(procpath, sizeof(procpath), "/proc/%d/exe", pid);
                len = readlink( procpath, fullpath, sizeof(fullpath) );
                if (len >= 0) {
                    fullpath[len] = 0;
                    TQFileInfo fileInfo(fullpath);
                    if (fileInfo.baseName() == "kdesktop_lock") {
                        // Verify that pid in question is owned by current user before killing it
                        uid_t current_uid = geteuid();

                        struct stat info;
                        if (lstat(procpath, &info) == 0) {
                            if (info.st_uid == current_uid) {
                                kill(pid, SIGKILL);
                            }
                        }
                    }
                }
            }
        }

        // Force a relock as a stale lockfile or process may have been dealt with above
        if (!lock.isLocked()) {
            lockRet = lock.lock(TDELockFile::LockNoBlock | TDELockFile::LockForce);
        }

        kdDebug() << "app " << kdesktop_screen_number << " " << starting_screen << " " << child << " " << child_sockets.count() << " " << parent_connection << endl;
        app->disableSessionManagement();
        TDEGlobal::locale()->insertCatalogue("libdmctl");

        struct stat st;
        KSimpleConfig* tdmconfig;
        OPEN_TDMCONFIG_AND_SET_GROUP

        trinity_desktop_lock_process = new LockProcess;

        // Start loading core functions, such as the desktop wallpaper interface
        app->processEvents();

        if (args->isSet( "internal" )) {
            kdesktop_pid = atoi(args->getOption( "internal" ));
            sigset_t new_mask;
            sigset_t orig_mask;
            struct sigaction act;

            in_internal_mode = TRUE;

            // handle SIGUSR1
            act.sa_handler= sigusr1_handler;
            sigemptyset(&(act.sa_mask));
            sigaddset(&(act.sa_mask), SIGUSR1);
            act.sa_flags = 0;
            sigaction(SIGUSR1, &act, 0L);
            // handle SIGUSR2
            act.sa_handler= sigusr2_handler;
            sigemptyset(&(act.sa_mask));
            sigaddset(&(act.sa_mask), SIGUSR2);
            act.sa_flags = 0;
            sigaction(SIGUSR2, &act, 0L);
            // handle SIGWINCH (an ersatz SIGUSR3)
            act.sa_handler= sigusr3_handler;
            sigemptyset(&(act.sa_mask));
            sigaddset(&(act.sa_mask), SIGWINCH);
            act.sa_flags = 0;
            sigaction(SIGWINCH, &act, 0L);
            // handle SIGTTIN (an ersatz SIGUSR4)
            act.sa_handler= sigusr4_handler;
            sigemptyset(&(act.sa_mask));
            sigaddset(&(act.sa_mask), SIGTTIN);
            act.sa_flags = 0;
            sigaction(SIGTTIN, &act, 0L);
            // handle SIGTTOU (an ersatz SIGUSR5)
            act.sa_handler= sigusr5_handler;
            sigemptyset(&(act.sa_mask));
            sigaddset(&(act.sa_mask), SIGTTOU);
            act.sa_flags = 0;
            sigaction(SIGTTOU, &act, 0L);

            // initialize the signal masks
            sigemptyset(&new_mask);
            sigaddset(&new_mask,SIGUSR1);
            sigaddset(&new_mask,SIGUSR2);
            sigaddset(&new_mask,SIGWINCH);
            sigaddset(&new_mask,SIGTTIN);
            sigaddset(&new_mask,SIGTTOU);

            while (signalled_run == FALSE) {
                // let kdesktop know the saver process is ready
                if (kill(kdesktop_pid, SIGTTIN) < 0) {
                    // The controlling kdesktop process probably died.  Commit suicide...
                    return 12;
                }

		// Get root window attributes
		XWindowAttributes rootAttr;
		XGetWindowAttributes(tqt_xdisplay(), RootWindow(tqt_xdisplay(), tqt_xscreen()), &rootAttr);

		// Disable reception of all X11 events on the root window
		XSelectInput( tqt_xdisplay(), tqt_xrootwin(), 0 );
		app->processEvents();

                // wait for SIGUSR1, SIGUSR2, SIGWINCH, SIGTTIN, or SIGTTOU
		sigprocmask(SIG_BLOCK, &new_mask, &orig_mask);
		if (signalled_run != TRUE) {
			sigsuspend(&orig_mask);
		}
		sigprocmask(SIG_UNBLOCK, &new_mask, NULL);

		// Reenable reception of X11 events on the root window
		XSelectInput( tqt_xdisplay(), tqt_xrootwin(), rootAttr.your_event_mask );
            }

	    // Block reception of all signals in this thread
	    sigprocmask(SIG_BLOCK, &new_mask, NULL);
        }

        // (re)load settings here so that they actually reflect reality
        // we need to read from the right rc file - possibly taking screen number in account
        if (!trinity_desktop_lock_settings_initialized) {
            KDesktopSettings::instance("kdesktoprc");
            trinity_desktop_lock_settings_initialized = true;
        }
        else {
            KDesktopSettings::self()->readConfig();
        }
        trinity_desktop_lock_use_system_modal_dialogs = !KDesktopSettings::useUnmanagedLockWindows();
        trinity_desktop_lock_delay_screensaver_start = KDesktopSettings::delaySaverStart();
        if (trinity_desktop_lock_use_system_modal_dialogs) {
#ifdef BUILD_TSAK
            trinity_desktop_lock_use_sak = tdmconfig->readBoolEntry("UseSAK", false) && KDesktopSettings::useTDESAK();
#else
            trinity_desktop_lock_use_sak = false;
#endif
        }
        else {
            trinity_desktop_lock_use_sak = false;			// If SAK is enabled with unmanaged windows, the SAK dialog will never close and will "burn in" the screen
            trinity_desktop_lock_delay_screensaver_start = false;	// If trinity_desktop_lock_delay_screensaver_start is true with unmanaged windows, the lock dialog may never appear
        }
        trinity_desktop_lock_hide_active_windows = KDesktopSettings::hideActiveWindowsFromSaver();
        trinity_desktop_lock_hide_cancel_button = KDesktopSettings::hideCancelButton();

        delete tdmconfig;

        if (args->isSet( "forcelock" ) || (signalled_forcelock == TRUE)) {
            trinity_desktop_lock_forced = TRUE;
        }

        trinity_desktop_lock_process->init(child, (args->isSet( "blank" ) || (signalled_blank == TRUE)));
        if (!child) {
            trinity_desktop_lock_process->setChildren(child_sockets);
        }
        else {
            trinity_desktop_lock_process->setParent(parent_connection);
        }

        bool rt;
        if( (((!child) && (args->isSet( "forcelock" ))) || (signalled_forcelock == TRUE))) {
            rt = trinity_desktop_lock_process->lock();
        }
        else if( child || (args->isSet( "dontlock" ) || (signalled_dontlock == TRUE))) {
            rt = trinity_desktop_lock_process->dontLock();
        }
        else if( child || (args->isSet( "securedialog" ) || (signalled_securedialog == TRUE))) {
            int retcode = tde_sak_verify_calling_process();
            if (retcode == 0) {
                rt = trinity_desktop_lock_process->runSecureDialog();
            }
            else {
                return 1;
            }
        }
        else {
            rt = trinity_desktop_lock_process->defaultSave();
        }
        if (!rt) {
            return 0;
        }

        if (in_internal_mode == FALSE) {
            trinity_desktop_lock_hidden_window_list.clear();
            int ret = app->exec();
            restore_hidden_override_redirect_windows();
            return ret;
        }
        else {
            if (kill(kdesktop_pid, 0) < 0) {
                // The controlling kdesktop process probably died.  Commit suicide...
                return 12;
            }
            trinity_desktop_lock_hidden_window_list.clear();
            app->exec();
            restore_hidden_override_redirect_windows();
            if (kill(kdesktop_pid, SIGUSR1) < 0) {
                // The controlling kdesktop process probably died.  Commit suicide...
                return 12;
            }

            delete trinity_desktop_lock_process;
            trinity_desktop_lock_process = NULL;

            // FIXME
            // We should not have to return (restart) at all,
            // but it seems that some X11 connections are left active,
            // preventing the lock process from restarting properly in the while() loop above.
            return 0;
        }
    }
}

#include "main.moc"
