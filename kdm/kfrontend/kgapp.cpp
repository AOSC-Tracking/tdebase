/*

Greeter module for xdm

Copyright (C) 1997, 1998 Steffen Hansen <hansen@kde.org>
Copyright (C) 2000-2003 Oswald Buddenhagen <ossi@kde.org>


This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <config.h>

#include "kdm_greet.h"
#include "kdmshutdown.h"
#include "kdmconfig.h"
#include "kgapp.h"
#include "kgreeter.h"
#ifdef XDMCP
# include "kchooser.h"
#endif
#include "sakdlg.h"

#include <kprocess.h>
#include <kcmdlineargs.h>
#include <kcrash.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>
#include <klocale.h>
#include <kdebug.h>
#ifdef WITH_XRANDR
#include <libkrandr/libkrandr.h>
#endif

#include <tqtimer.h>
#include <tqstring.h>
#include <tqcursor.h>
#include <tqpalette.h>

#include <stdlib.h> // free(), exit()
#include <unistd.h> // alarm()

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#ifdef HAVE_XCOMPOSITE
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#endif

#include <pwd.h>

#define TSAK_FIFO_FILE "/tmp/ksocket-global/tsak"

bool argb_visual_available = false;
bool has_kwin = false;
bool is_themed = false;
bool trinity_desktop_lock_use_sak = TRUE;

static int
ignoreXError( Display *dpy ATTR_UNUSED, XErrorEvent *event ATTR_UNUSED )
{
        return 0;
}

extern "C" {

static void
sigAlarm( int )
{
	exit( EX_RESERVER_DPY );
}

}

GreeterApp::GreeterApp()
{
	pingInterval = _isLocal ? 0 : _pingInterval;
	if (pingInterval) {
		struct sigaction sa;
		sigemptyset( &sa.sa_mask );
		sa.sa_flags = 0;
		sa.sa_handler = sigAlarm;
		sigaction( SIGALRM, &sa, 0 );
		alarm( pingInterval * 70 ); // sic! give the "proper" pinger enough time
		startTimer( pingInterval * 60000 );
	}
}

GreeterApp::GreeterApp(Display *dpy, Qt::HANDLE visual, Qt::HANDLE colormap) : KApplication(dpy, visual, colormap)
{
	pingInterval = _isLocal ? 0 : _pingInterval;
	if (pingInterval) {
		struct sigaction sa;
		sigemptyset( &sa.sa_mask );
		sa.sa_flags = 0;
		sa.sa_handler = sigAlarm;
		sigaction( SIGALRM, &sa, 0 );
		alarm( pingInterval * 70 ); // sic! give the "proper" pinger enough time
		startTimer( pingInterval * 60000 );
	}
}

void
GreeterApp::timerEvent( TQTimerEvent * )
{
	alarm( 0 );
	if (!PingServer( qt_xdisplay() ))
		::exit( EX_RESERVER_DPY );
	alarm( pingInterval * 70 ); // sic! give the "proper" pinger enough time
}

bool
GreeterApp::x11EventFilter( XEvent * ev )
{
	KeySym sym;

	switch (ev->type) {
	case FocusIn:
	case FocusOut:
		// Hack to tell dialogs to take focus when the keyboard is grabbed
		ev->xfocus.mode = NotifyNormal;
		break;
	case KeyPress:
		sym = XLookupKeysym( &ev->xkey, 0 );
		if (sym != XK_Return && !IsModifierKey( sym ))
			emit activity();
		break;
	case ButtonPress:
		emit activity();
		/* fall through */
	case ButtonRelease:
		// Hack to let the RMB work as LMB
		if (ev->xbutton.button == 3)
			ev->xbutton.button = 1;
		/* fall through */
	case MotionNotify:
		if (ev->xbutton.state & Button3Mask)
			ev->xbutton.state = (ev->xbutton.state & ~Button3Mask) | Button1Mask;
		break;
	}
	return false;
}

extern bool kde_have_kipc;

extern "C" {

static int
xIOErr( Display * )
{
	exit( EX_RESERVER_DPY );
}

//KSimpleConfig *iccconfig;

void
checkSAK(GreeterApp* app)
{
	app->restoreOverrideCursor();
	SAKDlg sak(0);
	sak.exec();
	app->setOverrideCursor( Qt::WaitCursor );
}

void
kg_main( const char *argv0 )
{
	static char *argv[] = { (char *)"kdmgreet", 0 };
	KCmdLineArgs::init( 1, argv, *argv, 0, 0, 0, true );

	kdDebug() << timestamp() << "start" << endl;
	kde_have_kipc = false;
	KApplication::disableAutoDcopRegistration();
	KCrash::setSafer( true );

	KProcess *tsak = 0;
	KProcess *proc = 0;
	KProcess *comp = 0;
	KProcess *dcop = 0;
	KProcess *kwin = 0;

#ifdef BUILD_TSAK
	trinity_desktop_lock_use_sak = _useSAK;
#else
	trinity_desktop_lock_use_sak = false;
#endif
 	if (trinity_desktop_lock_use_sak) {
		if (system(KDE_BINDIR "/tsak checkdeps") != 0) {
			trinity_desktop_lock_use_sak = false;
		}
	}
	if (trinity_desktop_lock_use_sak) {
		tsak = new KProcess;
		*tsak << TQCString( argv0, strrchr( argv0, '/' ) - argv0 + 2 ) + "tsak";
		tsak->start(KProcess::Block, KProcess::AllOutput);
	}
	else {
		remove(TSAK_FIFO_FILE);
	}
	if (tsak) {
		tsak->closeStdin();
		tsak->closeStdout();
		tsak->detach();
		delete tsak;
	}

#ifdef HAVE_XCOMPOSITE
	// Begin ARGB initialization
	XSetErrorHandler( ignoreXError );
	argb_visual_available = false;
	char *display = 0;

	Display *dpyi = XOpenDisplay( display );
	if ( !dpyi ) {
		kdError() << "cannot connect to X server " << display << endl;
		exit( 1 );
	}

	int screen = DefaultScreen( dpyi );
	Colormap colormap = 0;
	Visual *visual = 0;
	int event_base, error_base;

	if ( XRenderQueryExtension( dpyi, &event_base, &error_base ) ) {
		int nvi;
		XVisualInfo templ;
		templ.screen  = screen;
		templ.depth   = 32;
		templ.c_class = TrueColor;
		XVisualInfo *xvi = XGetVisualInfo( dpyi, VisualScreenMask | VisualDepthMask
				| VisualClassMask, &templ, &nvi );

		for ( int i = 0; i < nvi; i++ ) {
			XRenderPictFormat *format = XRenderFindVisualFormat( dpyi, xvi[i].visual );
			if ( format->type == PictTypeDirect && format->direct.alphaMask ) {
				visual = xvi[i].visual;
				colormap = XCreateColormap( dpyi, RootWindow( dpyi, screen ), visual, AllocNone );
				kdDebug() << "found visual with alpha support" << endl;
				argb_visual_available = true;
				break;
			}
		}
	}
	XSync( qt_xdisplay(), False );
	XSetErrorHandler( (XErrorHandler)0 );

	GreeterApp *app;
	if ((!_compositor.isEmpty()) && ( argb_visual_available == true )) {
		app = new GreeterApp(dpyi, Qt::HANDLE( visual ), Qt::HANDLE( colormap ));
	}
	else {
		argb_visual_available = false;
		app = new GreeterApp();
	}
	// End ARGB initialization
#else
	GreeterApp *app = new GreeterApp();
#endif

	XSetIOErrorHandler( xIOErr );
	TQString login_user;
	TQString login_session_wm;

	Display *dpy = qt_xdisplay();

	if (!_GUIStyle.isEmpty())
		app->setStyle( _GUIStyle );

	// Load up systemwide display settings
#ifdef WITH_XRANDR
	KRandrSimpleAPI *randrsimple = new KRandrSimpleAPI();
	TQPoint primaryScreenPosition = randrsimple->applySystemwideDisplayConfiguration("", KDE_CONFDIR);
	delete randrsimple;
#endif

	// Load up the systemwide ICC profile
	TQString iccConfigFile = TQString(KDE_CONFDIR);
	iccConfigFile += "/kicc/kiccconfigrc";
	KSimpleConfig iccconfig(iccConfigFile, true);
	if (iccconfig.readBoolEntry("EnableICC", false) == true) {
		TQString iccCommand = TQString("/usr/bin/xcalib ");
		iccCommand += iccconfig.readEntry("ICCFile");
		iccCommand += TQString(" &");
		system(iccCommand.ascii());
	}

	_colorScheme = locate( "data", "kdisplay/color-schemes/" + _colorScheme + ".kcsrc" );
	if (!_colorScheme.isEmpty()) {
		KSimpleConfig config( _colorScheme, true );
		config.setGroup( "Color Scheme" );
		app->setPalette( app->createApplicationPalette( &config, 7 ) );
	}

	app->setFont( _normalFont );

	setup_modifiers( dpy, _numLockStatus );
	SecureDisplay( dpy );
	if (!_grabServer) {
		if (_useBackground) {
			proc = new KProcess;
			*proc << TQCString( argv0, strrchr( argv0, '/' ) - argv0 + 2 ) + "krootimage";
			*proc << _backgroundCfg;
			proc->start();
		}
		GSendInt( G_SetupDpy );
		GRecvInt();
	}

	if (!_compositor.isEmpty()) {
		comp = new KProcess;
		*comp << TQCString( argv0, strrchr( argv0, '/' ) - argv0 + 2 ) + _compositor.ascii();
		comp->start(KProcess::NotifyOnExit, KProcess::Stdin);
	}

	if (!_windowManager.isEmpty()) {
		if (_windowManager == "kwin") {
			// Special case
			// Start DCOP...
			dcop = new KProcess;
			*dcop << TQCString( argv0, strrchr( argv0, '/' ) - argv0 + 2 ) + "dcopserver" << TQCString("--suicide");
			dcop->start();
		}
		kwin = new KProcess;
		*kwin << TQCString( argv0, strrchr( argv0, '/' ) - argv0 + 2 ) + _windowManager.ascii();
		if (_windowManager == "kwin") {
			// Special case
			// Do not allow kwin to start kompmgr...
			*kwin << "--disablecompositionmanager";
		}
		kwin->start();
		has_kwin = true;
	}

	GSendInt( G_Ready );

	kdDebug() << timestamp() << " main1" << endl;
	setCursor( dpy, app->desktop()->winId(), XC_left_ptr );

	for (;;) {
		int rslt, cmd = GRecvInt();

		if (cmd == G_ConfShutdown) {
			int how = GRecvInt(), uid = GRecvInt();
			char *os = GRecvStr();
			KDMSlimShutdown::externShutdown( how, os, uid );
			if (os)
				free( os );
			GSendInt( G_Ready );
			_autoLoginDelay = 0;
			continue;
		}

		if (cmd == G_ErrorGreet) {
			if (KGVerify::handleFailVerify( TQT_TQWIDGET(tqApp->desktop()->screen( _greeterScreen )) ))
				break;
			_autoLoginDelay = 0;
			cmd = G_Greet;
		}

		KProcess *proc2 = 0;
		app->setOverrideCursor( Qt::WaitCursor );
		FDialog *dialog = NULL;
#ifdef XDMCP
		if (cmd == G_Choose) {
			dialog = new ChooserDlg;
			GSendInt( G_Ready ); /* tell chooser to go into async mode */
			GRecvInt(); /* ack */
		} else
#endif
		{
			if ((cmd != G_GreetTimed && !_autoLoginAgain) ||
			    _autoLoginUser.isEmpty())
				_autoLoginDelay = 0;
			if (_useTheme && !_theme.isEmpty() && !trinity_desktop_lock_use_sak) {
				// Qt4 has a nasty habit of generating BadWindow errors in normal operation, so we simply ignore them
				// This also prevents the user from being dropped to a console login if Xorg glitches or is buggy
				XSetErrorHandler( ignoreXError );
				KThemedGreeter *tgrt;
				bool has_kwin_bkp = has_kwin;
				is_themed = true;
				has_kwin = false;	// [FIXME] The themed greeter is built on the assumption that there is no window manager available (i.e. it keeps stealing focus) and needs to be repaired.
				dialog = tgrt = new KThemedGreeter;
				kdDebug() << timestamp() << " themed" << endl;
				if (!tgrt->isOK()) {
					is_themed = false;
					has_kwin = has_kwin_bkp;
					delete tgrt;
					if (trinity_desktop_lock_use_sak) {
						checkSAK(app);
					}
					dialog = new KStdGreeter;
#ifdef WITH_XRANDR
					dialog->move(dialog->x() + primaryScreenPosition.x(), dialog->y() + primaryScreenPosition.y());
#endif
				}
				else {
#ifdef WITH_XRANDR
					dialog->move(primaryScreenPosition.x(), primaryScreenPosition.y());
#endif
				}
				XSync( qt_xdisplay(), False );
				XSetErrorHandler( (XErrorHandler)0 );
			} else {
				if (trinity_desktop_lock_use_sak) {
					checkSAK(app);
				}
				dialog = new KStdGreeter;
#ifdef WITH_XRANDR
				dialog->move(dialog->x() + primaryScreenPosition.x(), dialog->y() + primaryScreenPosition.y());
#endif
			}
			TQPoint oldCursorPos = TQCursor::pos();
#ifdef WITH_XRANDR
			TQCursor::setPos(oldCursorPos.x() + primaryScreenPosition.x(), oldCursorPos.y() + primaryScreenPosition.y());
#endif
			if (*_preloader) {
				proc2 = new KProcess;
				*proc2 << _preloader;
				proc2->start();
			}
		}
		app->restoreOverrideCursor();
		Debug( "entering event loop\n" );
		// Qt4 has a nasty habit of generating BadWindow errors in normal operation, so we simply ignore them
		// This also prevents the user from being dropped to a console login if Xorg glitches or is buggy
		XSetErrorHandler( ignoreXError );
		rslt = dialog->exec();
		XSync( qt_xdisplay(), False );
		XSetErrorHandler( (XErrorHandler)0 );
		Debug( "left event loop\n" );

		login_user = static_cast<KGreeter*>(dialog)->curUser;
		login_session_wm = static_cast<KGreeter*>(dialog)->curWMSession;

		if (rslt != ex_greet) {
			delete dialog;
		}
		delete proc2;
#ifdef XDMCP
		switch (rslt) {
		case ex_greet:
			GSendInt( G_DGreet );
			continue;
		case ex_choose:
			GSendInt( G_DChoose );
			continue;
		default:
			break;
		}
#endif
		break;
	}

	KGVerify::done();

	if (comp) {
		if (comp->isRunning()) {
			if (_compositor == "kompmgr") {
				// Change process UID
				// Get user UID
				passwd* userinfo = getpwnam(login_user.ascii());
				if (userinfo) {
					TQString newuid = TQString("%1").arg(userinfo->pw_uid);
					// kompmgr allows us to change its uid in this manner:
					// 1.) Send SIGUSR1
					// 2.) Send the new UID to it on the command line
					comp->kill(SIGUSR1);
					comp->writeStdin(newuid.ascii(), newuid.length());
					usleep(50000);	// Give the above function some time to execute.  Note that on REALLY slow systems this could fail, leaving kompmgr running as root.  TODO: Look into ways to make this more robust.
				}
			}
			comp->closeStdin();
			comp->detach();
		}
		delete comp;
	}
	if (kwin) {
		if (kwin->isRunning()) {
			if (login_session_wm.endsWith("/startkde") || (login_session_wm == "failsafe")) {
				kwin->closeStdin();
				kwin->detach();
				dcop->detach();
			}
			else {
				kwin->kill();
				dcop->kill();
			}
		}
		delete kwin;
		delete dcop;
	}
	delete proc;
	UnsecureDisplay( dpy );
	restore_modifiers();

	// Qt4 has a nasty habit of generating BadWindow errors in normal operation, so we simply ignore them
	// This also prevents the user from being dropped to a console login if Xorg glitches or is buggy
	XSetErrorHandler( ignoreXError );
	XSetInputFocus( qt_xdisplay(), PointerRoot, PointerRoot, CurrentTime );
	XSync( qt_xdisplay(), False );
	XSetErrorHandler( (XErrorHandler)0 );

	delete app;
}

} // extern "C"

#include "kgapp.moc"
