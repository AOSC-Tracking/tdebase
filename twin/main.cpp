/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#include <tdeconfig.h>

#include "main.h"

#include <tdelocale.h>
#include <tdeglobal.h>
#include <kdebug.h>
#include <stdlib.h>
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "atoms.h"
#include "options.h"
#include "sm.h"

#define INT8 _X11INT8
#define INT32 _X11INT32
#include <X11/Xproto.h>
#undef INT8
#undef INT32

namespace KWinInternal
{

Options* options;

Atoms* atoms;

int screen_number = -1;
bool disable_twin_composition_manager = false;

static bool initting = false;

static
int x11ErrorHandler(Display *d, XErrorEvent *e)
    {
    char msg[80], req[80], number[80];
    bool ignore_badwindow = true; //maybe temporary

    if (initting &&
        (
         e->request_code == X_ChangeWindowAttributes
         || e->request_code == X_GrabKey
         )
        && (e->error_code == BadAccess)) 
        {
        fputs(i18n("[twin] it looks like there's already a window manager running. twin not started.\n").local8Bit(), stderr);
        exit(1);
        }

    if (ignore_badwindow && (e->error_code == BadWindow || e->error_code == BadColor))
        return 0;

    XGetErrorText(d, e->error_code, msg, sizeof(msg));
    sprintf(number, "%d", e->request_code);
    XGetErrorDatabaseText(d, "XRequest", number, "<unknown>", req, sizeof(req));

    fprintf(stderr, "[twin] %s(0x%lx): %s\n", req, e->resourceid, msg);

    if (initting) 
        {
        fputs(i18n("[twin] failure during initialization; aborting").local8Bit(), stderr);
        exit(1);
        }
    return 0;
    }

Application::Application( )
: TDEApplication( ), owner( screen_number )
    {
    TDECmdLineArgs* args = TDECmdLineArgs::parsedArgs();
    if (!config()->isImmutable() && args->isSet("lock"))
        {
        config()->setReadOnly(true);
        config()->reparseConfiguration();
        }

    if (screen_number == -1) {
        screen_number = DefaultScreen(tqt_xdisplay());
    }

    if (args->isSet( "disablecompositionmanager" )) {
        disable_twin_composition_manager = true;
    }

    if( !owner.claim( args->isSet( "replace" ), true ))
        {
        Display* dpy = tqt_xdisplay();
        Window w;
        Atom a;
        static char net_wm_sm[] = "WM_Sxx";

        snprintf (net_wm_sm, sizeof (net_wm_sm), "WM_S%d", screen_number);
        a = XInternAtom (dpy, net_wm_sm, False);

        w = XGetSelectionOwner (dpy, a);

        if (w != None)
            {
            Atom actual;
            int format;
            unsigned long n, left;
            unsigned char *data;
            Atom twinRunningAtom = XInternAtom (dpy, "_KDE_WM_IS_KWIN", True);

            int result = XGetWindowProperty (dpy, w, twinRunningAtom, 0L, 1L, False,
                XA_ATOM, &actual, &format,
                &n, &left, &data);

            if (result == Success && data != None && format == 32 )
                {
                Atom a;
                a = *(long*)data;
                XFree ( (void *) data);
                if( !owner.claim( true, true ))
                    {
                    fputs(i18n("[twin] unable to claim manager selection, another wm running? (try using --replace)\n").local8Bit(), stderr);
                    ::exit(1);
                    }
                }
            else
                {
                fputs(i18n("[twin] unable to claim manager selection, another wm running? (try using --replace)\n").local8Bit(), stderr);
                ::exit(1);
                }
            }
        else
            {
            fputs(i18n("[twin] unable to claim manager selection, another wm running? (try using --replace)\n").local8Bit(), stderr);
            ::exit(1);
            }
        }
    connect( &owner, TQ_SIGNAL( lostOwnership()), TQ_SLOT( lostSelection()));
    
    // if there was already twin running, it saved its configuration after loosing the selection -> reread
    config()->reparseConfiguration();

    initting = true; // startup....

    // install X11 error handler
    XSetErrorHandler( x11ErrorHandler );

    // check  whether another windowmanager is running
    XSelectInput(tqt_xdisplay(), tqt_xrootwin(), SubstructureRedirectMask  );
    syncX(); // trigger error now

    options = new Options;
    atoms = new Atoms;

    // Signal that we are The KWin!
    Atom kde_wm_system_modal_notification;
    kde_wm_system_modal_notification = XInternAtom(tqt_xdisplay(), "_KDE_WM_IS_KWIN", False);
    XChangeProperty(tqt_xdisplay(), owner.ownerWindow(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);

    // create workspace.
    (void) new Workspace( isSessionRestored() );

    syncX(); // trigger possible errors, there's still a chance to abort

    DCOPRef ref( "kded", "kded" );
    ref.send( "unloadModule", TQCString( "kdetrayproxy" ));
    
    initting = false; // startup done, we are up and running now.
       
    dcopClient()->send( "ksplash", "", "upAndRunning(TQString)", TQString("wm started"));
    XEvent e;
    e.xclient.type = ClientMessage;
    e.xclient.message_type = XInternAtom( tqt_xdisplay(), "_KDE_SPLASH_PROGRESS", False );
    e.xclient.display = tqt_xdisplay();
    e.xclient.window = tqt_xrootwin();
    e.xclient.format = 8;
    strcpy( e.xclient.data.b, "wm started" );
    XSendEvent( tqt_xdisplay(), tqt_xrootwin(), False, SubstructureNotifyMask, &e );
    }

Application::~Application()
    {
    delete Workspace::self();
    if( owner.ownerWindow() != None ) // if there was no --replace (no new WM)
        {
        XSetInputFocus( tqt_xdisplay(), PointerRoot, RevertToPointerRoot, get_tqt_x_time() );
        DCOPRef ref( "kded", "kded" );
        if( !ref.send( "loadModule", TQCString( "kdetrayproxy" )))
            kdWarning( 176 ) << "Loading of kdetrayproxy failed." << endl;
        }
    delete options;
    }

void Application::lostSelection()
    {
    delete Workspace::self();
    // remove windowmanager privileges
    XSelectInput(tqt_xdisplay(), tqt_xrootwin(), PropertyChangeMask );
    DCOPRef ref( "kded", "kded" );
    if( !ref.send( "loadModule", TQCString( "kdetrayproxy" )))
        kdWarning( 176 ) << "Loading of kdetrayproxy failed." << endl;
    quit();
    }

bool Application::x11EventFilter( XEvent *e )
    {
    if ( Workspace::self()->workspaceEvent( e ) )
             return true;
    return TDEApplication::x11EventFilter( e );
    }
    
static void sighandler(int) 
    {
    TQApplication::exit();
    }


} // namespace

static const char version[] = "3.0";
static const char description[] = I18N_NOOP( "TDE window manager" );

static TDECmdLineOptions args[] =
    {
        { "lock", I18N_NOOP("Disable configuration options"), 0 },
        { "replace", I18N_NOOP("Replace already-running ICCCM2.0-compliant window manager"), 0 },
        { "disablecompositionmanager", I18N_NOOP("Do not start composition manager"), 0 },
        TDECmdLineLastOption
    };

extern "C"
TDE_EXPORT int kdemain( int argc, char * argv[] )
    {
    bool restored = false;
    for (int arg = 1; arg < argc; arg++) 
        {
        if (! qstrcmp(argv[arg], "-session")) 
            {
            restored = true;
            break;
            }
        }

    if (! restored) 
        {
        // we only do the multihead fork if we are not restored by the session
        // manager, since the session manager will register multiple twins,
        // one for each screen...
        TQCString multiHead = getenv("TDE_MULTIHEAD");
        if (multiHead.lower() == "true") 
            {

            Display* dpy = XOpenDisplay( NULL );
            if ( !dpy ) 
                {
                fprintf(stderr, "%s: FATAL ERROR while trying to open display %s\n",
                        argv[0], XDisplayName(NULL ) );
                exit (1);
                }

            int number_of_screens = ScreenCount( dpy );
            KWinInternal::screen_number = DefaultScreen( dpy );
            int pos; // temporarily needed to reconstruct DISPLAY var if multi-head
            TQCString display_name = XDisplayString( dpy );
            XCloseDisplay( dpy );
            dpy = 0;

            if ((pos = display_name.findRev('.')) != -1 )
                display_name.remove(pos,10); // 10 is enough to be sure we removed ".s"

            TQCString envir;
            if (number_of_screens != 1) 
                {
                for (int i = 0; i < number_of_screens; i++ ) 
                    {
                    // if execution doesn't pass by here, then twin
                    // acts exactly as previously
                    if ( i != KWinInternal::screen_number && fork() == 0 ) 
                        {
                        KWinInternal::screen_number = i;
                        // break here because we are the child process, we don't
                        // want to fork() anymore
                        break;
                        }
                    }
                // in the next statement, display_name shouldn't contain a screen
                //   number. If it had it, it was removed at the "pos" check
                envir.sprintf("DISPLAY=%s.%d", display_name.data(), KWinInternal::screen_number);

                if (putenv( strdup(envir.data())) ) 
                    {
                    fprintf(stderr,
                            "[twin] %s: WARNING: unable to set DISPLAY environment variable\n",
                            argv[0]);
                    perror("[twin] putenv()");
                    }
                }
            }
        }

    TDEGlobal::locale()->setMainCatalogue("twin");

    TDEAboutData aboutData( "twin", I18N_NOOP("TWin"),
                          version, description, TDEAboutData::License_GPL,
                          I18N_NOOP("(c) 1999-2005, The KDE Developers"));
    aboutData.addAuthor("Matthias Ettrich",0, "ettrich@kde.org");
    aboutData.addAuthor("Cristian Tibirna",0, "tibirna@kde.org");
    aboutData.addAuthor("Daniel M. Duley",0, "mosfet@kde.org");
    aboutData.addAuthor("Luboš Luňák", I18N_NOOP( "Maintainer" ), "l.lunak@kde.org");

    TDECmdLineArgs::init(argc, argv, &aboutData);
    TDECmdLineArgs::addCmdLineOptions( args );

    if (signal(SIGTERM, KWinInternal::sighandler) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
    if (signal(SIGINT, KWinInternal::sighandler) == SIG_IGN)
        signal(SIGINT, SIG_IGN);
    if (signal(SIGHUP, KWinInternal::sighandler) == SIG_IGN)
        signal(SIGHUP, SIG_IGN);

    TDEApplication::disableAutoDcopRegistration();
    KWinInternal::Application a;
    KWinInternal::SessionManaged weAreIndeed;
    KWinInternal::SessionSaveDoneHelper helper;

    fcntl(ConnectionNumber(tqt_xdisplay()), F_SETFD, 1);

    TQCString appname;
    if (KWinInternal::screen_number == 0)
        appname = "twin";
    else
        appname.sprintf("twin-screen-%d", KWinInternal::screen_number);

    TDEGlobal::locale()->insertCatalogue("twin_lib");

    DCOPClient* client = a.dcopClient();
    client->registerAs( appname.data(), false);
    client->setDefaultObject( "KWinInterface" );

    return a.exec();
    }

#include "main.moc"
