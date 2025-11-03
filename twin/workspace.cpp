/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#include "workspace.h"

#include <tdeapplication.h>
#include <tdestartupinfo.h>
#include <fixx11h.h>
#include <tdeconfig.h>
#include <tdeglobal.h>
#include <tqpopupmenu.h>
#include <tdelocale.h>
#include <tqregexp.h>
#include <tqpainter.h>
#include <tqbitmap.h>
#include <tqclipboard.h>
#include <tqfile.h>
#include <tdemenubar.h>
#include <kprocess.h>
#include <kglobalaccel.h>
#include <tdestandarddirs.h>
#include <dcopclient.h>
#include <kipc.h>

#include "plugins.h"
#include "client.h"
#include "popupinfo.h"
#include "tabbox.h"
#include "atoms.h"
#include "placement.h"
#include "notifications.h"
#include "group.h"
#include "rules.h"

#include <X11/XKBlib.h>
#include <X11/extensions/shape.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/cursorfont.h>

#include <pwd.h>

#include "config.h"

namespace KWinInternal
{

extern int screen_number;

Workspace *Workspace::_self = 0;

TDEProcess* kompmgr = 0;
TDESelectionOwner* kompmgr_selection;

bool allowKompmgrRestart = true;
extern bool disable_twin_composition_manager;

bool supportsCompMgr()
{
    if (disable_twin_composition_manager) {
        return false;
    }

    int i;

    bool damageExt = XQueryExtension(tqt_xdisplay(), "DAMAGE", &i, &i, &i);
    bool compositeExt = XQueryExtension(tqt_xdisplay(), "Composite", &i, &i, &i);
    bool xfixesExt = XQueryExtension(tqt_xdisplay(), "XFIXES", &i, &i, &i);

    return damageExt && compositeExt && xfixesExt;
}

TQString compositorPIDFile () {
    return locateLocal("tmp", TQString("compton-tde.").append(getenv("DISPLAY")).append(".pid"));
}

pid_t getCompositorPID() {
    // Attempt to load the compton-tde pid file
    pid_t rv = 0;
    TQFile pidFile(compositorPIDFile());

    if (pidFile.open(IO_ReadOnly)) {
        bool ok;
        TQString pidStr;

        pidFile.readLine(pidStr, 21);
        rv = pidStr.toInt(&ok);

        if (!ok) {
            return 0;
        }
    }

    return rv;
}

// Rikkus: This class is too complex. It needs splitting further.
// It's a nightmare to understand, especially with so few comments :(

// Matthias: Feel free to ask me questions about it. Feel free to add
// comments. I disagree that further splittings makes it easier. 2500
// lines are not too much. It's the task that is complex, not the
// code.
Workspace::Workspace( bool restore )
  : DCOPObject        ("KWinInterface"),
    TQObject           (0, "workspace"),
    current_desktop   (0),
    number_of_desktops(0),
    active_screen     (0),
    active_popup( NULL ),
    active_popup_client( NULL ),
    desktop_widget    (0),
    temporaryRulesMessages( "_KDE_NET_WM_TEMPORARY_RULES", NULL, false ),
    rules_updates_disabled( false ),
    active_client     (0),
    last_active_client     (0),
    next_active_client     (0),
    most_recently_raised (0),
    movingClient(0),
    pending_take_activity ( NULL ),
    delayfocus_client (0),
    showing_desktop( false ),
    block_showing_desktop( 0 ),
    was_user_interaction (false),
    session_saving    (false),
    control_grab      (false),
    tab_grab          (false),
    mouse_emulation   (false),
    block_focus       (0),
    tab_box           (0),
    popupinfo         (0),
    popup             (0),
    advanced_popup    (0),
    desk_popup        (0),
    desk_popup_index  (0),
    keys              (0),
    client_keys       ( NULL ),
    client_keys_dialog ( NULL ),
    client_keys_client ( NULL ),
    disable_shortcuts_keys ( NULL ),
    global_shortcuts_disabled( false ),
    global_shortcuts_disabled_for_client( false ),
    root              (0),
    workspaceInit     (true),
    startup(0),
    layoutOrientation(TQt::Vertical),
    layoutX(-1),
    layoutY(2),
    workarea(NULL),
    screenarea(NULL),
    managing_topmenus( false ),
    topmenu_selection( NULL ),
    topmenu_watcher( NULL ),
    topmenu_height( 0 ),
    topmenu_space( NULL ),
    set_active_client_recursion( 0 ),
    block_stacking_updates( 0 ),
    forced_global_mouse_grab( false )
    {
    _self = this;
    mgr = new PluginMgr;
    root = tqt_xrootwin();
    default_colormap = DefaultColormap(tqt_xdisplay(), tqt_xscreen() );
    installed_colormap = default_colormap;
    session.setAutoDelete( true );

    for (int i = 0; i < ACTIVE_BORDER_COUNT; ++i)
    {
        active_reserved[i] = 0;
        active_windows[i] = None;
    }

    connect( &temporaryRulesMessages, TQ_SIGNAL( gotMessage( const TQString& )),
        this, TQ_SLOT( gotTemporaryRulesMessage( const TQString& )));
    connect( &rulesUpdatedTimer, TQ_SIGNAL( timeout()), this, TQ_SLOT( writeWindowRules()));

    updateXTime(); // needed for proper initialization of user_time in Client ctor

    delayFocusTimer = 0;

    active_time_first = get_tqt_x_time();
    active_time_last = get_tqt_x_time();

    if ( restore )
      loadSessionInfo();

    loadWindowRules();

    (void) TQApplication::desktop(); // trigger creation of desktop widget

    desktop_widget =
      new TQWidget(
        0,
        "desktop_widget",
        (WFlags)(TQt::WType_Desktop | TQt::WPaintUnclipped)
    );

    kapp->setGlobalMouseTracking( true ); // so that this doesn't mess eventmask on root window later
    // call this before XSelectInput() on the root window
    startup = new TDEStartupInfo(
        TDEStartupInfo::DisableKWinModule | TDEStartupInfo::AnnounceSilenceChanges, this );

    // select windowmanager privileges
    XSelectInput(tqt_xdisplay(), root,
                 KeyPressMask |
                 PropertyChangeMask |
                 ColormapChangeMask |
                 SubstructureRedirectMask |
                 SubstructureNotifyMask |
                 FocusChangeMask // for NotifyDetailNone
                 );

    Shape::init();

    // compatibility
    long data = 1;

    XChangeProperty(
      tqt_xdisplay(),
      tqt_xrootwin(),
      atoms->twin_running,
      atoms->twin_running,
      32,
      PropModeAppend,
      (unsigned char*) &data,
      1
    );

    client_keys = new TDEGlobalAccel( this );
    initShortcuts();
    tab_box = new TabBox( this );
    popupinfo = new PopupInfo( this );

    init();

#if (TQT_VERSION-0 >= 0x030200) // XRANDR support
    connect( kapp->desktop(), TQ_SIGNAL( resized( int )), TQ_SLOT( desktopResized()));
#endif

    if (!supportsCompMgr()) {
        options->useTranslucency = false;
    }

    // start kompmgr - i wanted to put this into main.cpp, but that would prevent dcop support, as long as Application was no dcop_object

    // If compton-tde is already running, send it SIGTERM
    pid_t kompmgrpid = getCompositorPID();

    if (options->useTranslucency)
        {
        createKompmgrProcess();

        if (kompmgrpid)
            {
            if (kill(kompmgrpid, 0) < 0)
                {
                // Stale PID file detected; (re)start compositor!
                startKompmgr();
                }
            }
        else
            {
            startKompmgr();
            }
        }
    else if (!disable_twin_composition_manager)
        {

        if (kompmgrpid)
            {
            kill(kompmgrpid, SIGTERM);
            }
        else
            {
            stopKompmgr();
            }
        }
    }


void Workspace::init()
{
    if (options->activeBorders() == Options::ActiveSwitchAlways)
    {
        reserveActiveBorderSwitching(true);
    }
    updateActiveBorders();

// not used yet
//     topDock = 0L;
//     maximizedWindowCounter = 0;

    supportWindow = new TQWidget;
    XLowerWindow( tqt_xdisplay(), supportWindow->winId()); // see usage in layers.cpp

    XSetWindowAttributes attr;
    attr.override_redirect = 1;
    null_focus_window = XCreateWindow( tqt_xdisplay(), tqt_xrootwin(), -1,-1, 1, 1, 0, CopyFromParent,
        InputOnly, CopyFromParent, CWOverrideRedirect, &attr );
    XMapWindow(tqt_xdisplay(), null_focus_window);

    unsigned long protocols[ 5 ] =
        {
        NET::Supported |
        NET::SupportingWMCheck |
        NET::ClientList |
        NET::ClientListStacking |
        NET::DesktopGeometry |
        NET::NumberOfDesktops |
        NET::CurrentDesktop |
        NET::ActiveWindow |
        NET::WorkArea |
        NET::CloseWindow |
        NET::DesktopNames |
        NET::KDESystemTrayWindows |
        NET::WMName |
        NET::WMVisibleName |
        NET::WMDesktop |
        NET::WMWindowType |
        NET::WMState |
        NET::WMStrut |
        NET::WMIconGeometry |
        NET::WMIcon |
        NET::WMPid |
        NET::WMMoveResize |
        NET::WMKDESystemTrayWinFor |
        NET::WMFrameExtents |
        NET::WMPing
        ,
        NET::NormalMask |
        NET::DesktopMask |
        NET::DockMask |
        NET::ToolbarMask |
        NET::MenuMask |
        NET::DialogMask |
        NET::OverrideMask |
        NET::TopMenuMask |
        NET::UtilityMask |
        NET::SplashMask |
        0
        ,
        NET::Modal |
//        NET::Sticky |  // large desktops not supported (and probably never will be)
        NET::MaxVert |
        NET::MaxHoriz |
        NET::Shaded |
        NET::SkipTaskbar |
        NET::KeepAbove |
//        NET::StaysOnTop |  the same like KeepAbove
        NET::SkipPager |
        NET::Hidden |
        NET::FullScreen |
        NET::KeepBelow |
        NET::DemandsAttention |
        0
        ,
        NET::WM2UserTime |
        NET::WM2StartupId |
        NET::WM2AllowedActions |
        NET::WM2RestackWindow |
        NET::WM2MoveResizeWindow |
        NET::WM2ExtendedStrut |
        NET::WM2KDETemporaryRules |
        NET::WM2ShowingDesktop |
        NET::WM2FullPlacement |
        NET::WM2DesktopLayout |
        0
        ,
        NET::ActionMove |
        NET::ActionResize |
        NET::ActionMinimize |
        NET::ActionShade |
//        NET::ActionStick | // Sticky state is not supported
        NET::ActionMaxVert |
        NET::ActionMaxHoriz |
        NET::ActionFullScreen |
        NET::ActionChangeDesktop |
        NET::ActionClose |
        0
        ,
        };

    rootInfo = new RootInfo( this, tqt_xdisplay(), supportWindow->winId(), "KWin",
        protocols, 5, tqt_xscreen() );

    loadDesktopSettings();
    updateDesktopLayout();
    // extra NETRootInfo instance in Client mode is needed to get the values of the properties
    NETRootInfo client_info( tqt_xdisplay(), NET::ActiveWindow | NET::CurrentDesktop );
    int initial_desktop;
    if( !kapp->isSessionRestored())
        initial_desktop = client_info.currentDesktop();
    else
        {
        TDEConfigGroupSaver saver( kapp->sessionConfig(), "Session" );
        initial_desktop = kapp->sessionConfig()->readNumEntry( "desktop", 1 );
        }
    if( !setCurrentDesktop( initial_desktop ))
        setCurrentDesktop( 1 );

    // now we know how many desktops we'll, thus, we initialise the positioning object
    initPositioning = new Placement(this);

    connect(&reconfigureTimer, TQ_SIGNAL(timeout()), this,
            TQ_SLOT(slotReconfigure()));
    connect( &updateToolWindowsTimer, TQ_SIGNAL( timeout()), this, TQ_SLOT( slotUpdateToolWindows()));

    connect(kapp, TQ_SIGNAL(appearanceChanged()), this,
            TQ_SLOT(slotReconfigure()));
    connect(kapp, TQ_SIGNAL(settingsChanged(int)), this,
            TQ_SLOT(slotSettingsChanged(int)));
    connect(kapp, TQ_SIGNAL( kipcMessage( int, int )), this, TQ_SLOT( kipcMessage( int, int )));

    active_client = NULL;
    rootInfo->setActiveWindow( None );
    focusToNull();
    if( !kapp->isSessionRestored())
        ++block_focus; // because it will be set below

    char nm[ 100 ];
    sprintf( nm, "_KDE_TOPMENU_OWNER_S%d", DefaultScreen( tqt_xdisplay()));
    Atom topmenu_atom = XInternAtom( tqt_xdisplay(), nm, False );
    topmenu_selection = new TDESelectionOwner( topmenu_atom );
    topmenu_watcher = new TDESelectionWatcher( topmenu_atom );
// TODO grabXServer(); - where exactly put this? topmenu selection claiming down belong must be before

        { // begin updates blocker block
        StackingUpdatesBlocker blocker( this );

        if( options->topMenuEnabled() && topmenu_selection->claim( false ))
            setupTopMenuHandling(); // this can call updateStackingOrder()
        else
            lostTopMenuSelection();

        unsigned int i, nwins;
        Window root_return, parent_return, *wins;
        XQueryTree(tqt_xdisplay(), root, &root_return, &parent_return, &wins, &nwins);
        for (i = 0; i < nwins; i++)
            {
            XWindowAttributes attr;
            XGetWindowAttributes(tqt_xdisplay(), wins[i], &attr);
            if (attr.override_redirect )
                continue;
            if( topmenu_space && topmenu_space->winId() == wins[ i ] )
                continue;
            if (attr.map_state != IsUnmapped)
                {
                if ( addSystemTrayWin( wins[i] ) )
                    continue;
                Client* c = createClient( wins[i], true );
                if ( c != NULL && root != tqt_xrootwin() )
                    { // TODO what is this?
                // TODO may use TQWidget:.create
                    XReparentWindow( tqt_xdisplay(), c->frameId(), root, 0, 0 );
                    c->move(0,0);
                    }
                }
            }
        if ( wins )
            XFree((void *) wins);
    // propagate clients, will really happen at the end of the updates blocker block
        updateStackingOrder( true );

        updateClientArea();

    // NETWM spec says we have to set it to (0,0) if we don't support it
        NETPoint* viewports = new NETPoint[ number_of_desktops ];
        rootInfo->setDesktopViewport( number_of_desktops, *viewports );
        delete[] viewports;
        TQRect geom = TQApplication::desktop()->geometry();
        NETSize desktop_geometry;
        desktop_geometry.width = geom.width();
        desktop_geometry.height = geom.height();
        rootInfo->setDesktopGeometry( -1, desktop_geometry );
        setShowingDesktop( false );

        } // end updates blocker block

    Client* new_active_client = NULL;
    if( !kapp->isSessionRestored())
        {
        --block_focus;
        new_active_client = findClient( WindowMatchPredicate( client_info.activeWindow()));
        }
    if( new_active_client == NULL
        && activeClient() == NULL && should_get_focus.count() == 0 ) // no client activated in manage()
        {
        if( new_active_client == NULL )
            new_active_client = topClientOnDesktop( currentDesktop());
        if( new_active_client == NULL && !desktops.isEmpty() )
            new_active_client = findDesktop( true, currentDesktop());
        }
    if( new_active_client != NULL )
        activateClient( new_active_client );

    // outline windows for active border maximize window mode
    outline_left = XCreateWindow(tqt_xdisplay(), rootWin(), 0, 0, 1, 1, 0,
                                 CopyFromParent, CopyFromParent, CopyFromParent,
                                 CWOverrideRedirect, &attr);
    outline_right = XCreateWindow(tqt_xdisplay(), rootWin(), 0, 0, 1, 1, 0,
                                  CopyFromParent, CopyFromParent, CopyFromParent,
                                  CWOverrideRedirect, &attr);
    outline_top = XCreateWindow(tqt_xdisplay(), rootWin(), 0, 0, 1, 1, 0,
                                CopyFromParent, CopyFromParent, CopyFromParent,
                                CWOverrideRedirect, &attr);
    outline_bottom = XCreateWindow(tqt_xdisplay(), rootWin(), 0, 0, 1, 1, 0,
                                   CopyFromParent, CopyFromParent, CopyFromParent,
                                   CWOverrideRedirect, &attr);

    // SELI TODO this won't work with unreasonable focus policies,
    // and maybe in rare cases also if the selected client doesn't
    // want focus
    workspaceInit = false;
// TODO ungrabXServer()
}

Workspace::~Workspace()
    {
    if (kompmgr)
        delete kompmgr;
    blockStackingUpdates( true );
// TODO    grabXServer();
    // use stacking_order, so that twin --replace keeps stacking order
    for( ClientList::ConstIterator it = stacking_order.begin();
         it != stacking_order.end();
         ++it )
        {
        // only release the window
        (*it)->releaseWindow( true );
        // No removeClient() is called, it does more than just removing.
        // However, remove from some lists to e.g. prevent performTransiencyCheck()
        // from crashing.
        clients.remove( *it );
        desktops.remove( *it );
        }
    delete desktop_widget;
    delete tab_box;
    delete popupinfo;
    delete popup;
    if ( root == tqt_xrootwin() )
        XDeleteProperty(tqt_xdisplay(), tqt_xrootwin(), atoms->twin_running);

    writeWindowRules();
    TDEGlobal::config()->sync();

    // destroy outline windows for active border maximize window mode
    XDestroyWindow(tqt_xdisplay(), outline_left);
    XDestroyWindow(tqt_xdisplay(), outline_right);
    XDestroyWindow(tqt_xdisplay(), outline_top);
    XDestroyWindow(tqt_xdisplay(), outline_bottom);

    delete rootInfo;
    delete supportWindow;
    delete mgr;
    delete[] workarea;
    delete[] screenarea;
    delete startup;
    delete initPositioning;
    delete topmenu_watcher;
    delete topmenu_selection;
    delete topmenu_space;
    delete client_keys_dialog;
    while( !rules.isEmpty())
        {
        delete rules.front();
        rules.pop_front();
        }
    XDestroyWindow( tqt_xdisplay(), null_focus_window );
// TODO    ungrabXServer();
    _self = 0;
    }

Client* Workspace::createClient( Window w, bool is_mapped )
    {
    StackingUpdatesBlocker blocker( this );
    Client* c = new Client( this );
    if( !c->manage( w, is_mapped ))
        {
        Client::deleteClient( c, Allowed );
        return NULL;
        }
    addClient( c, Allowed );
    return c;
    }

void Workspace::addClient( Client* c, allowed_t )
    {
    // waited with trans settings until window figured out if active or not ;)
//     tqWarning("%s", (const char*)(c->resourceClass()));
    c->setBMP(c->resourceName() == "beep-media-player" || c->decorationId() == None);
    // first check if the window has it's own opinion of it's translucency ;)
    c->getWindowOpacity();
    if (c->isDock())
        {
//         if (c->x() == 0 && c->y() == 0 && c->width() > c->height()) topDock = c;
        if (!c->hasCustomOpacity()) // this xould be done slightly more efficient, but we want to support the topDock in future
            {
            c->setShadowSize(options->dockShadowSize);
            c->setOpacity(options->translucentDocks, options->dockOpacity);
            }
        }

    if (c->isMenu() || c->isTopMenu())
        {
            c->setShadowSize(options->menuShadowSize);
        }
//------------------------------------------------
    Group* grp = findGroup( c->window());
    if( grp != NULL )
        grp->gotLeader( c );

    if ( c->isDesktop() )
        {
        desktops.append( c );
        if( active_client == NULL && should_get_focus.isEmpty() && c->isOnCurrentDesktop())
            requestFocus( c ); // CHECKME? make sure desktop is active after startup if there's no other window active
        }
    else
        {
        updateFocusChains( c, FocusChainUpdate ); // add to focus chain if not already there
        clients.append( c );
        }
    if( !unconstrained_stacking_order.contains( c ))
        unconstrained_stacking_order.append( c );
    if( !stacking_order.contains( c )) // it'll be updated later, and updateToolWindows() requires
        stacking_order.append( c );    // c to be in stacking_order
    if( c->isTopMenu())
        addTopMenu( c );
    updateClientArea(); // this cannot be in manage(), because the client got added only now
    updateClientLayer( c );
    if( c->isDesktop())
        {
        raiseClient( c );
        // if there's no active client, make this desktop the active one
        if( activeClient() == NULL && should_get_focus.count() == 0 )
            activateClient( findDesktop( true, currentDesktop()));
        }
    c->checkActiveModal();
    checkTransients( c->window()); // SELI does this really belong here?
    updateStackingOrder( true ); // propagate new client
    if( c->isUtility() || c->isMenu() || c->isToolbar())
        updateToolWindows( true );
    checkNonExistentClients();
    }

/*
  Destroys the client \a c
 */
void Workspace::removeClient( Client* c, allowed_t )
    {
    if (c == active_popup_client)
        closeActivePopup();

    if( client_keys_client == c )
        setupWindowShortcutDone( false );
    if( !c->shortcut().isNull())
        c->setShortcut( TQString::null ); // remove from client_keys

    if( c->isDialog())
        Notify::raise( Notify::TransDelete );
    if( c->isNormalWindow())
        Notify::raise( Notify::Delete );

    Q_ASSERT( clients.contains( c ) || desktops.contains( c ));
    clients.remove( c );
    desktops.remove( c );
    unconstrained_stacking_order.remove( c );
    stacking_order.remove( c );
    for( int i = 1;
         i <= numberOfDesktops();
         ++i )
        focus_chain[ i ].remove( c );
    global_focus_chain.remove( c );
    attention_chain.remove( c );
    showing_desktop_clients.remove( c );
    if( c->isTopMenu())
        removeTopMenu( c );
    Group* group = findGroup( c->window());
    if( group != NULL )
        group->lostLeader();

    if ( c == most_recently_raised )
        most_recently_raised = 0;
    should_get_focus.remove( c );
    Q_ASSERT( c != active_client );
    if ( c == last_active_client )
        last_active_client = 0;
    if( c == pending_take_activity )
        pending_take_activity = NULL;
    if( c == delayfocus_client )
        cancelDelayFocus();

    updateStackingOrder( true );

    if (tab_grab)
       tab_box->repaint();

    updateClientArea();
    }

void Workspace::updateFocusChains( Client* c, FocusChainChange change )
    {
    if( !c->wantsTabFocus()) // doesn't want tab focus, remove
        {
        for( int i=1;
             i<= numberOfDesktops();
             ++i )
            focus_chain[i].remove(c);
        global_focus_chain.remove( c );
        return;
        }
    if(c->desktop() == NET::OnAllDesktops)
        { //now on all desktops, add it to focus_chains it is not already in
        for( int i=1; i<= numberOfDesktops(); i++)
            { // making first/last works only on current desktop, don't affect all desktops
            if( i == currentDesktop()
                && ( change == FocusChainMakeFirst || change == FocusChainMakeLast ))
                {
                focus_chain[ i ].remove( c );
                if( change == FocusChainMakeFirst )
                    focus_chain[ i ].append( c );
                else
                    focus_chain[ i ].prepend( c );
                }
            else if( !focus_chain[ i ].contains( c ))
                { // add it after the active one
                if( active_client != NULL && active_client != c
                    && !focus_chain[ i ].isEmpty() && focus_chain[ i ].last() == active_client )
                    focus_chain[ i ].insert( focus_chain[ i ].fromLast(), c );
                else
                    focus_chain[ i ].append( c ); // otherwise add as the first one
                }
            }
        }
    else    //now only on desktop, remove it anywhere else
        {
        for( int i=1; i<= numberOfDesktops(); i++)
            {
            if( i == c->desktop())
                {
                if( change == FocusChainMakeFirst )
                    {
                    focus_chain[ i ].remove( c );
                    focus_chain[ i ].append( c );
                    }
                else if( change == FocusChainMakeLast )
                    {
                    focus_chain[ i ].remove( c );
                    focus_chain[ i ].prepend( c );
                    }
                else if( !focus_chain[ i ].contains( c ))
                    {
                    if( active_client != NULL && active_client != c
                        && !focus_chain[ i ].isEmpty() && focus_chain[ i ].last() == active_client )
                        focus_chain[ i ].insert( focus_chain[ i ].fromLast(), c );
                    else
                        focus_chain[ i ].append( c ); // otherwise add as the first one
                    }
                }
            else
                focus_chain[ i ].remove( c );
            }
        }
    if( change == FocusChainMakeFirst )
        {
        global_focus_chain.remove( c );
        global_focus_chain.append( c );
        }
    else if( change == FocusChainMakeLast )
        {
        global_focus_chain.remove( c );
        global_focus_chain.prepend( c );
        }
    else if( !global_focus_chain.contains( c ))
        {
        if( active_client != NULL && active_client != c
            && !global_focus_chain.isEmpty() && global_focus_chain.last() == active_client )
            global_focus_chain.insert( global_focus_chain.fromLast(), c );
        else
            global_focus_chain.append( c ); // otherwise add as the first one
        }
    }

void Workspace::updateOverlappingShadows(unsigned long window)
    {
    Client *client;

    if ((client = findClient(WindowMatchPredicate((WId)window))))
        // Redraw overlapping shadows without waiting for the specified window
        // to redraw its own shadow
        client->drawOverlappingShadows(false);
    }

void Workspace::setShadowed(unsigned long window, bool shadowed)
    {
    Client *client;

    if ((client = findClient(WindowMatchPredicate((WId)window))))
        client->setShadowed(shadowed);
    }

void Workspace::updateCurrentTopMenu()
    {
    if( !managingTopMenus())
        return;
    // toplevel menubar handling
    Client* menubar = 0;
    bool block_desktop_menubar = false;
    if( active_client )
        {
        // show the new menu bar first...
        Client* menu_client = active_client;
        for(;;)
            {
            if( menu_client->isFullScreen())
                block_desktop_menubar = true;
            for( ClientList::ConstIterator it = menu_client->transients().begin();
                 it != menu_client->transients().end();
                 ++it )
                if( (*it)->isTopMenu())
                    {
                    menubar = *it;
                    break;
                    }
            if( menubar != NULL || !menu_client->isTransient())
                break;
            if( menu_client->isModal() || menu_client->transientFor() == NULL )
                break; // don't use mainwindow's menu if this is modal or group transient
            menu_client = menu_client->transientFor();
            }
        if( !menubar )
            { // try to find any topmenu from the application (#72113)
            for( ClientList::ConstIterator it = active_client->group()->members().begin();
                 it != active_client->group()->members().end();
                 ++it )
                if( (*it)->isTopMenu())
                    {
                    menubar = *it;
                    break;
                    }
            }
        }
    if( !menubar && !block_desktop_menubar && options->desktopTopMenu())
        {
        // Find the menubar of the desktop
        Client* desktop = findDesktop( true, currentDesktop());
        if( desktop != NULL )
            {
            for( ClientList::ConstIterator it = desktop->transients().begin();
                 it != desktop->transients().end();
                 ++it )
                if( (*it)->isTopMenu())
                    {
                    menubar = *it;
                    break;
                    }
            }
        // TODO to be cleaned app with window grouping
        // Without qt-copy patch #0009, the topmenu and desktop are not in the same group,
        // thus the topmenu is not transient for it :-/.
        if( menubar == NULL )
            {
            for( ClientList::ConstIterator it = topmenus.begin();
                 it != topmenus.end();
                 ++it )
                if( (*it)->wasOriginallyGroupTransient()) // kdesktop's topmenu has WM_TRANSIENT_FOR
                    {                                     // set pointing to the root window
                    menubar = *it;                        // to recognize it here
                    break;                                // Also, with the xroot hack in kdesktop,
                    }                                     // there's no NET::Desktop window to be transient for
            }
        }

//    kdDebug() << "CURRENT TOPMENU:" << menubar << ":" << active_client << endl;
    if ( menubar )
        {
        if( active_client && !menubar->isOnDesktop( active_client->desktop()))
            menubar->setDesktop( active_client->desktop());
        menubar->hideClient( false );
        topmenu_space->hide();
        // make it appear like it's been raised manually - it's in the Dock layer anyway,
        // and not raising it could mess up stacking order of topmenus within one application,
        // and thus break raising of mainclients in raiseClient()
        unconstrained_stacking_order.remove( menubar );
        unconstrained_stacking_order.append( menubar );
        }
    else if( !block_desktop_menubar )
        { // no topmenu active - show the space window, so that there's not empty space
        topmenu_space->show();
        }

    // ... then hide the other ones. Avoids flickers.
    for ( ClientList::ConstIterator it = clients.begin(); it != clients.end(); ++it)
        {
        if( (*it)->isTopMenu() && (*it) != menubar )
            (*it)->hideClient( true );
        }
    }


void Workspace::updateToolWindows( bool also_hide )
    {
    // TODO what if Client's transiency/group changes? should this be called too? (I'm paranoid, am I not?)
    if( !options->hideUtilityWindowsForInactive )
        {
        for( ClientList::ConstIterator it = clients.begin();
             it != clients.end();
             ++it )
            (*it)->hideClient( false );
        return;
        }
    const Group* group = NULL;
    const Client* client = active_client;
// Go up in transiency hiearchy, if the top is found, only tool transients for the top mainwindow
// will be shown; if a group transient is group, all tools in the group will be shown
    while( client != NULL )
        {
        if( !client->isTransient())
            break;
        if( client->groupTransient())
            {
            group = client->group();
            break;
            }
        client = client->transientFor();
        }
    // use stacking order only to reduce flicker, it doesn't matter if block_stacking_updates == 0,
    // i.e. if it's not up to date

    // SELI but maybe it should - what if a new client has been added that's not in stacking order yet?
    ClientList to_show, to_hide;
    for( ClientList::ConstIterator it = stacking_order.begin();
         it != stacking_order.end();
         ++it )
        {
        if( (*it)->isUtility() || (*it)->isMenu() || (*it)->isToolbar())
            {
            bool show = true;
            if( !(*it)->isTransient())
                {
                if( (*it)->group()->members().count() == 1 ) // has its own group, keep always visible
                    show = true;
                else if( client != NULL && (*it)->group() == client->group())
                    show = true;
                else
                    show = false;
                }
            else
                {
                if( group != NULL && (*it)->group() == group )
                    show = true;
                else if( client != NULL && client->hasTransient( (*it), true ))
                    show = true;
                else
                    show = false;
                }
            if( !show && also_hide )
                {
                const ClientList mainclients = (*it)->mainClients();
                // don't hide utility windows which are standalone(?) or
                // have e.g. kicker as mainwindow
                if( mainclients.isEmpty())
                    show = true;
                for( ClientList::ConstIterator it2 = mainclients.begin();
                     it2 != mainclients.end();
                     ++it2 )
                    {
                    if( (*it2)->isSpecialWindow())
                        show = true;
                    }
                if( !show )
                    to_hide.append( *it );
                }
            if( show )
                to_show.append( *it );
            }
        } // first show new ones, then hide
    for( ClientList::ConstIterator it = to_show.fromLast();
         it != to_show.end();
         --it ) // from topmost
        // TODO since this is in stacking order, the order of taskbar entries changes :(
        (*it)->hideClient( false );
    if( also_hide )
        {
        for( ClientList::ConstIterator it = to_hide.begin();
             it != to_hide.end();
             ++it ) // from bottommost
            (*it)->hideClient( true );
        updateToolWindowsTimer.stop();
        }
    else // setActiveClient() is after called with NULL client, quickly followed
        {    // by setting a new client, which would result in flickering
        updateToolWindowsTimer.start( 50, true );
        }
    }

void Workspace::slotUpdateToolWindows()
    {
    updateToolWindows( true );
    }

/*!
  Updates the current colormap according to the currently active client
 */
void Workspace::updateColormap()
    {
    Colormap cmap = default_colormap;
    if ( activeClient() && activeClient()->colormap() != None )
        cmap = activeClient()->colormap();
    if ( cmap != installed_colormap )
        {
        XInstallColormap(tqt_xdisplay(), cmap );
        installed_colormap = cmap;
        }
    }

void Workspace::reconfigure()
    {
    reconfigureTimer.start(200, true);
    }


void Workspace::slotSettingsChanged(int category)
    {
    kdDebug(1212) << "Workspace::slotSettingsChanged()" << endl;
    if( category == (int) TDEApplication::SETTINGS_SHORTCUTS )
        readShortcuts();
    }

/*!
  Reread settings
 */
KWIN_PROCEDURE( CheckBorderSizesProcedure, cl->checkBorderSizes() );

void Workspace::slotReconfigure()
    {
    kdDebug(1212) << "Workspace::slotReconfigure()" << endl;
    reconfigureTimer.stop();

    if (options->activeBorders() == Options::ActiveSwitchAlways)
    {
        reserveActiveBorderSwitching(false);
    }

    TDEGlobal::config()->reparseConfiguration();
    unsigned long changed = options->updateSettings();
    tab_box->reconfigure();
    popupinfo->reconfigure();
    initPositioning->reinitCascading( 0 );
    readShortcuts();
    forEachClient( CheckIgnoreFocusStealingProcedure());
    updateToolWindows( true );

    if( mgr->reset( changed ))
        { // decorations need to be recreated
#if 0 // This actually seems to make things worse now
        TQWidget curtain;
        curtain.setBackgroundMode( NoBackground );
        curtain.setGeometry( TQApplication::desktop()->geometry() );
        curtain.show();
#endif
        for( ClientList::ConstIterator it = clients.begin();
                it != clients.end();
                ++it )
            {
            (*it)->updateDecoration( true, true );
            }
        mgr->destroyPreviousPlugin();
        }
    else
        {
        forEachClient( CheckBorderSizesProcedure());
        }

    if (options->activeBorders() == Options::ActiveSwitchAlways)
    {
        reserveActiveBorderSwitching(true);
    }

    if( options->topMenuEnabled() && !managingTopMenus())
        {
        if( topmenu_selection->claim( false ))
            setupTopMenuHandling();
        else
            lostTopMenuSelection();
        }
    else if( !options->topMenuEnabled() && managingTopMenus())
        {
        topmenu_selection->release();
        lostTopMenuSelection();
        }
    topmenu_height = 0; // invalidate used menu height
    if( managingTopMenus())
        {
        updateTopMenuGeometry();
        updateCurrentTopMenu();
        }

    loadWindowRules();
    for( ClientList::Iterator it = clients.begin();
         it != clients.end();
         ++it )
        {
        (*it)->setupWindowRules( true );
        (*it)->applyWindowRules();
        discardUsedWindowRules( *it, false );
        }

    if (options->resetKompmgr) // need restart
        {
        bool tmp = options->useTranslucency;

        // If compton-tde is already running, sending SIGUSR2 will force a reload of its settings
        pid_t kompmgrpid = getCompositorPID();

        if (tmp)
            {
            if (kompmgrpid)
                {
                kill(kompmgrpid, SIGUSR2);
                }
            else
                {
                stopKompmgr();
                if (!kompmgr)
                    {
                    createKompmgrProcess();
                    }
                TQTimer::singleShot( 200, this, TQ_SLOT(startKompmgr()) ); // wait some time to ensure system's ready for restart
                }
            }
        else
            {
            if (kompmgrpid)
                {
                kill(kompmgrpid, SIGTERM);
                }
            else
                {
                stopKompmgr();
                }
            }
        }
    }

void Workspace::loadDesktopSettings()
    {
    TDEConfig* c = TDEGlobal::config();
    TQCString groupname;
    if (screen_number == 0)
        groupname = "Desktops";
    else
        groupname.sprintf("Desktops-screen-%d", screen_number);
    TDEConfigGroupSaver saver(c,groupname);

    int n = c->readNumEntry("Number", 4);
    number_of_desktops = n;
    delete workarea;
    workarea = new TQRect[ n + 1 ];
    delete screenarea;
    screenarea = NULL;
    rootInfo->setNumberOfDesktops( number_of_desktops );
    desktop_focus_chain.resize( n );
    // make it +1, so that it can be accessed as [1..numberofdesktops]
    focus_chain.resize( n + 1 );
    for(int i = 1; i <= n; i++)
        {
        TQString s = c->readEntry(TQString("Name_%1").arg(i),
                                i18n("Desktop %1").arg(i));
        rootInfo->setDesktopName( i, s.utf8().data() );
        desktop_focus_chain[i-1] = i;
        }
    }

void Workspace::saveDesktopSettings()
    {
    TDEConfig* c = TDEGlobal::config();
    TQCString groupname;
    if (screen_number == 0)
        groupname = "Desktops";
    else
        groupname.sprintf("Desktops-screen-%d", screen_number);
    TDEConfigGroupSaver saver(c,groupname);

    c->writeEntry("Number", number_of_desktops );
    for(int i = 1; i <= number_of_desktops; i++)
        {
        TQString s = desktopName( i );
        TQString defaultvalue = i18n("Desktop %1").arg(i);
        if ( s.isEmpty() )
            {
            s = defaultvalue;
            rootInfo->setDesktopName( i, s.utf8().data() );
            }

        if (s != defaultvalue)
            {
            c->writeEntry( TQString("Name_%1").arg(i), s );
            }
        else
            {
            TQString currentvalue = c->readEntry(TQString("Name_%1").arg(i));
            if (currentvalue != defaultvalue)
                c->writeEntry( TQString("Name_%1").arg(i), "" );
            }
        }
    }

TQStringList Workspace::configModules(bool controlCenter)
    {
    TQStringList args;
    args <<  "tde-twindecoration.desktop";
    if (controlCenter)
        args << "tde-twinoptions.desktop";
    else if (kapp->authorizeControlModule("tde-twinoptions.desktop"))
        args  << "twinactions" << "twinfocus" <<  "twinmoving" << "twinadvanced" << "twinrules" << "twintranslucency";
    return args;
    }

void Workspace::configureWM()
    {
    TDEApplication::tdeinitExec( "tdecmshell", configModules(false) );
    }

/*!
  avoids managing a window with title \a title
 */
void Workspace::doNotManage( TQString title )
    {
    doNotManageList.append( title );
    }

/*!
  Hack for java applets
 */
bool Workspace::isNotManaged( const TQString& title )
    {
    for ( TQStringList::Iterator it = doNotManageList.begin(); it != doNotManageList.end(); ++it )
        {
        TQRegExp r( (*it) );
        if (r.search(title) != -1)
            {
            doNotManageList.remove( it );
            return true;
            }
        }
    return false;
    }

/*!
  Refreshes all the client windows
 */
void Workspace::refresh()
    {
    TQWidget w;
    w.setGeometry( TQApplication::desktop()->geometry() );
    w.show();
    w.hide();
    TQApplication::flushX();
    }

/*!
  During virt. desktop switching, desktop areas covered by windows that are
  going to be hidden are first obscured by new windows with no background
  ( i.e. transparent ) placed right below the windows. These invisible windows
  are removed after the switch is complete.
  Reduces desktop ( wallpaper ) repaints during desktop switching
*/
class ObscuringWindows
    {
    public:
        ~ObscuringWindows();
        void create( Client* c );
    private:
        TQValueList<Window> obscuring_windows;
        static TQValueList<Window>* cached;
        static unsigned int max_cache_size;
    };

TQValueList<Window>* ObscuringWindows::cached = 0;
unsigned int ObscuringWindows::max_cache_size = 0;

void ObscuringWindows::create( Client* c )
    {
    if( cached == 0 )
        cached = new TQValueList<Window>;
    Window obs_win;
    XWindowChanges chngs;
    int mask = CWSibling | CWStackMode;
    if( cached->count() > 0 )
        {
        cached->remove( obs_win = cached->first());
        chngs.x = c->x();
        chngs.y = c->y();
        chngs.width = c->width();
        chngs.height = c->height();
        mask |= CWX | CWY | CWWidth | CWHeight;
        }
    else
        {
        XSetWindowAttributes a;
        a.background_pixmap = None;
        a.override_redirect = True;
        obs_win = XCreateWindow( tqt_xdisplay(), tqt_xrootwin(), c->x(), c->y(),
            c->width(), c->height(), 0, CopyFromParent, InputOutput,
            CopyFromParent, CWBackPixmap | CWOverrideRedirect, &a );
        }
    chngs.sibling = c->frameId();
    chngs.stack_mode = Below;
    XConfigureWindow( tqt_xdisplay(), obs_win, mask, &chngs );
    XMapWindow( tqt_xdisplay(), obs_win );
    obscuring_windows.append( obs_win );
    }

ObscuringWindows::~ObscuringWindows()
    {
    max_cache_size = TQMAX( max_cache_size, obscuring_windows.count() + 4 ) - 1;
    for( TQValueList<Window>::ConstIterator it = obscuring_windows.begin();
         it != obscuring_windows.end();
         ++it )
        {
        XUnmapWindow( tqt_xdisplay(), *it );
        if( cached->count() < max_cache_size )
            cached->prepend( *it );
        else
            XDestroyWindow( tqt_xdisplay(), *it );
        }
    }


/*!
  Sets the current desktop to \a new_desktop

  Shows/Hides windows according to the stacking order and finally
  propages the new desktop to the world
 */
bool Workspace::setCurrentDesktop( int new_desktop )
    {
    if (new_desktop < 1 || new_desktop > number_of_desktops )
        return false;

    closeActivePopup();
    ++block_focus;
// TODO    Q_ASSERT( block_stacking_updates == 0 ); // make sure stacking_order is up to date
    StackingUpdatesBlocker blocker( this );

    int old_desktop = current_desktop;
    if (new_desktop != current_desktop)
        {
        ++block_showing_desktop;
        /*
          optimized Desktop switching: unmapping done from back to front
          mapping done from front to back => less exposure events
        */
        Notify::raise((Notify::Event) (Notify::DesktopChange+new_desktop));

        ObscuringWindows obs_wins;

        current_desktop = new_desktop; // change the desktop (so that Client::updateVisibility() works)

        bool desktopHasCompositing = kapp->isCompositionManagerAvailable();	// Technically I should call isX11CompositionAvailable(), but it isn't initialized via my kapp constructir, and in this case it doesn't really matter anyway....
        if (!desktopHasCompositing) {
            // If composition is not in use then we can hide the old windows before showing the new ones
            for ( ClientList::ConstIterator it = stacking_order.begin(); it != stacking_order.end(); ++it) {
                if ( !(*it)->isOnDesktop( new_desktop ) && (*it) != movingClient )
                {
                    if( (*it)->isShown( true ) && (*it)->isOnDesktop( old_desktop )) {
                        obs_wins.create( *it );
                    }
                    (*it)->updateVisibility();
                }
            }
        }

        rootInfo->setCurrentDesktop( current_desktop ); // now propagate the change, after hiding, before showing

        if( movingClient && !movingClient->isOnDesktop( new_desktop ))
            movingClient->setDesktop( new_desktop );

        for ( ClientList::ConstIterator it = stacking_order.fromLast(); it != stacking_order.end(); --it) {
            if ( (*it)->isOnDesktop( new_desktop ) ) {
                (*it)->updateVisibility();
            }
        }

        if (desktopHasCompositing) {
            // If composition is in use then we cannot hide the old windows before showing the new ones,
            // unless you happen to like the "flicker annoyingly to desktop" effect... :-P
            XSync( tqt_xdisplay(), false);	// Make absolutely certain all new windows are shown before hiding the old ones
            for ( ClientList::ConstIterator it = stacking_order.begin(); it != stacking_order.end(); ++it) {
                if ( !(*it)->isOnDesktop( new_desktop ) && (*it) != movingClient )
                {
                    if( (*it)->isShown( true ) && (*it)->isOnDesktop( old_desktop )) {
                        obs_wins.create( *it );
                    }
                    (*it)->updateVisibility();
                }
            }
        }

        --block_showing_desktop;
        if( showingDesktop()) // do this only after desktop change to avoid flicker
            resetShowingDesktop( false );
        }

    // restore the focus on this desktop
    --block_focus;
    Client* c = 0;

    if ( options->focusPolicyIsReasonable())
        {
        // Search in focus chain
        if ( movingClient != NULL && active_client == movingClient
            && focus_chain[currentDesktop()].contains( active_client )
            && active_client->isShown( true ) && active_client->isOnCurrentDesktop())
            {
            c = active_client; // the requestFocus below will fail, as the client is already active
            }
        if ( !c )
            {
            for( ClientList::ConstIterator it = focus_chain[currentDesktop()].fromLast();
                 it != focus_chain[currentDesktop()].end();
                 --it )
                {
                if ( (*it)->isShown( false ) && (*it)->isOnCurrentDesktop())
                    {
                    c = *it;
                    break;
                    }
                }
            }
        }

    //if "unreasonable focus policy"
    // and active_client is on_all_desktops and under mouse (hence == old_active_client),
    // conserve focus (thanks to Volker Schatz <V.Schatz at thphys.uni-heidelberg.de>)
    else if( active_client && active_client->isShown( true ) && active_client->isOnCurrentDesktop())
      c = active_client;

    if( c == NULL && !desktops.isEmpty())
        c = findDesktop( true, currentDesktop());

    if( c != active_client )
        setActiveClient( NULL, Allowed );

    if ( c )
        requestFocus( c );
    else
        focusToNull();

    updateCurrentTopMenu();

    // Update focus chain:
    //  If input: chain = { 1, 2, 3, 4 } and current_desktop = 3,
    //   Output: chain = { 3, 1, 2, 4 }.
//    kdDebug(1212) << TQString("Switching to desktop #%1, at focus_chain[currentDesktop()] index %2\n")
//      .arg(currentDesktop()).arg(desktop_focus_chain.find( currentDesktop() ));
    for( int i = desktop_focus_chain.find( currentDesktop() ); i > 0; i-- )
        desktop_focus_chain[i] = desktop_focus_chain[i-1];
    desktop_focus_chain[0] = currentDesktop();

//    TQString s = "desktop_focus_chain[] = { ";
//    for( uint i = 0; i < desktop_focus_chain.size(); i++ )
//        s += TQString::number(desktop_focus_chain[i]) + ", ";
//    kdDebug(1212) << s << "}\n";

    if( old_desktop != 0 )  // not for the very first time
        popupinfo->showInfo( desktopName(currentDesktop()) );
    return true;
    }

// called only from DCOP
void Workspace::nextDesktop()
    {
    int desktop = currentDesktop() + 1;
    setCurrentDesktop(desktop > numberOfDesktops() ? 1 : desktop);
    }

// called only from DCOP
void Workspace::previousDesktop()
    {
    int desktop = currentDesktop() - 1;
    setCurrentDesktop(desktop > 0 ? desktop : numberOfDesktops());
    }

int Workspace::desktopToRight( int desktop ) const
    {
    int x,y;
    calcDesktopLayout(x,y);
    int dt = desktop-1;
    if (layoutOrientation == TQt::Vertical)
        {
        dt += y;
        if ( dt >= numberOfDesktops() )
            {
            if ( options->rollOverDesktops )
              dt -= numberOfDesktops();
            else
              return desktop;
            }
        }
    else
        {
        int d = (dt % x) + 1;
        if ( d >= x )
            {
            if ( options->rollOverDesktops )
              d -= x;
            else
              return desktop;
            }
        dt = dt - (dt % x) + d;
        }
    return dt+1;
    }

int Workspace::desktopToLeft( int desktop ) const
    {
    int x,y;
    calcDesktopLayout(x,y);
    int dt = desktop-1;
    if (layoutOrientation == TQt::Vertical)
        {
        dt -= y;
        if ( dt < 0 )
            {
            if ( options->rollOverDesktops )
              dt += numberOfDesktops();
            else
              return desktop;
            }
        }
    else
        {
        int d = (dt % x) - 1;
        if ( d < 0 )
            {
            if ( options->rollOverDesktops )
              d += x;
            else
              return desktop;
            }
        dt = dt - (dt % x) + d;
        }
    return dt+1;
    }

int Workspace::desktopUp( int desktop ) const
    {
    int x,y;
    calcDesktopLayout(x,y);
    int dt = desktop-1;
    if (layoutOrientation == TQt::Horizontal)
        {
        dt -= x;
        if ( dt < 0 )
            {
            if ( options->rollOverDesktops )
              dt += numberOfDesktops();
            else
              return desktop;
            }
        }
    else
        {
        int d = (dt % y) - 1;
        if ( d < 0 )
            {
            if ( options->rollOverDesktops )
              d += y;
            else
              return desktop;
            }
        dt = dt - (dt % y) + d;
        }
    return dt+1;
    }

int Workspace::desktopDown( int desktop ) const
    {
    int x,y;
    calcDesktopLayout(x,y);
    int dt = desktop-1;
    if (layoutOrientation == TQt::Horizontal)
        {
        dt += x;
        if ( dt >= numberOfDesktops() )
            {
            if ( options->rollOverDesktops )
              dt -= numberOfDesktops();
            else
              return desktop;
            }
        }
    else
        {
        int d = (dt % y) + 1;
        if ( d >= y )
            {
            if ( options->rollOverDesktops )
              d -= y;
            else
              return desktop;
            }
        dt = dt - (dt % y) + d;
        }
    return dt+1;
    }


/*!
  Sets the number of virtual desktops to \a n
 */
void Workspace::setNumberOfDesktops( int n )
    {
    if ( n == number_of_desktops )
        return;
    int old_number_of_desktops = number_of_desktops;
    number_of_desktops = n;

    if( currentDesktop() > numberOfDesktops())
        setCurrentDesktop( numberOfDesktops());

    // if increasing the number, do the resizing now,
    // otherwise after the moving of windows to still existing desktops
    if( old_number_of_desktops < number_of_desktops )
        {
        rootInfo->setNumberOfDesktops( number_of_desktops );
        NETPoint* viewports = new NETPoint[ number_of_desktops ];
        rootInfo->setDesktopViewport( number_of_desktops, *viewports );
        delete[] viewports;
        updateClientArea( true );
        focus_chain.resize( number_of_desktops + 1 );
        }

    // if the number of desktops decreased, move all
    // windows that would be hidden to the last visible desktop
    if( old_number_of_desktops > number_of_desktops )
        {
        for( ClientList::ConstIterator it = clients.begin();
              it != clients.end();
              ++it)
            {
            if( !(*it)->isOnAllDesktops() && (*it)->desktop() > numberOfDesktops())
                sendClientToDesktop( *it, numberOfDesktops(), true );
            }
        }
    if( old_number_of_desktops > number_of_desktops )
        {
        rootInfo->setNumberOfDesktops( number_of_desktops );
        NETPoint* viewports = new NETPoint[ number_of_desktops ];
        rootInfo->setDesktopViewport( number_of_desktops, *viewports );
        delete[] viewports;
        updateClientArea( true );
        focus_chain.resize( number_of_desktops + 1 );
        }

    saveDesktopSettings();

    // Resize and reset the desktop focus chain.
    desktop_focus_chain.resize( n );
    for( int i = 0; i < (int)desktop_focus_chain.size(); i++ )
        desktop_focus_chain[i] = i+1;
    }

/*!
  Sends client \a c to desktop \a desk.

  Takes care of transients as well.
 */
void Workspace::sendClientToDesktop( Client* c, int desk, bool dont_activate )
    {
    bool was_on_desktop = c->isOnDesktop( desk ) || c->isOnAllDesktops();
    c->setDesktop( desk );
    if ( c->desktop() != desk ) // no change or desktop forced
        return;
    desk = c->desktop(); // Client did range checking

    if ( c->isOnDesktop( currentDesktop() ) )
        {
        if ( c->wantsTabFocus() && options->focusPolicyIsReasonable()
            && !was_on_desktop // for stickyness changes
            && !dont_activate )
            requestFocus( c );
        else
            restackClientUnderActive( c );
        }
    else
        {
        raiseClient( c );
        }

    ClientList transients_stacking_order = ensureStackingOrder( c->transients());
    for( ClientList::ConstIterator it = transients_stacking_order.begin();
         it != transients_stacking_order.end();
         ++it )
        sendClientToDesktop( *it, desk, dont_activate );
    updateClientArea();
    }

int Workspace::numScreens() const
    {
    if( !options->xineramaEnabled )
        return 0;
    return tqApp->desktop()->numScreens();
    }

int Workspace::activeScreen() const
    {
    if( !options->xineramaEnabled )
        return 0;
    if( !options->activeMouseScreen )
        {
        if( activeClient() != NULL && !activeClient()->isOnScreen( active_screen ))
            return tqApp->desktop()->screenNumber( activeClient()->geometry().center());
        return active_screen;
        }
    return tqApp->desktop()->screenNumber( TQCursor::pos());
    }

// check whether a client moved completely out of what's considered the active screen,
// if yes, set a new active screen
void Workspace::checkActiveScreen( const Client* c )
    {
    if( !options->xineramaEnabled )
        return;
    if( !c->isActive())
        return;
    if( !c->isOnScreen( active_screen ))
        active_screen = c->screen();
    }

// called e.g. when a user clicks on a window, set active screen to be the screen
// where the click occured
void Workspace::setActiveScreenMouse( TQPoint mousepos )
    {
    if( !options->xineramaEnabled )
        return;
    active_screen = tqApp->desktop()->screenNumber( mousepos );
    }

TQRect Workspace::screenGeometry( int screen ) const
    {
    if (( !options->xineramaEnabled ) || (kapp->desktop()->numScreens() < 2))
        return tqApp->desktop()->geometry();
    return tqApp->desktop()->screenGeometry( screen );
    }

int Workspace::screenNumber( TQPoint pos ) const
    {
    if( !options->xineramaEnabled )
        return 0;
    return tqApp->desktop()->screenNumber( pos );
    }

void Workspace::sendClientToScreen( Client* c, int screen )
    {
    if( c->screen() == screen ) // don't use isOnScreen(), that's true even when only partially
        return;
    GeometryUpdatesPostponer blocker( c );
    TQRect old_sarea = clientArea( MaximizeArea, c );
    TQRect sarea = clientArea( MaximizeArea, screen, c->desktop());
    c->setGeometry( sarea.x() - old_sarea.x() + c->x(), sarea.y() - old_sarea.y() + c->y(),
        c->size().width(), c->size().height());
    c->checkWorkspacePosition();
    ClientList transients_stacking_order = ensureStackingOrder( c->transients());
    for( ClientList::ConstIterator it = transients_stacking_order.begin();
         it != transients_stacking_order.end();
         ++it )
        sendClientToScreen( *it, screen );
    if( c->isActive())
        active_screen = screen;
    }


void Workspace::setDesktopLayout( int, int, int )
    { // DCOP-only, unused
    }

void Workspace::updateDesktopLayout()
    {
    // rootInfo->desktopLayoutCorner(); // I don't find this worth bothering, feel free to
    layoutOrientation = ( rootInfo->desktopLayoutOrientation() == NET::OrientationHorizontal
        ? TQt::Horizontal : TQt::Vertical );
    layoutX = rootInfo->desktopLayoutColumnsRows().width();
    layoutY = rootInfo->desktopLayoutColumnsRows().height();
    if( layoutX == 0 && layoutY == 0 ) // not given, set default layout
        layoutY = 2;
    }

void Workspace::calcDesktopLayout(int &x, int &y) const
    {
    x = layoutX; // <= 0 means compute it from the other and total number of desktops
    y = layoutY;
    if((x <= 0) && (y > 0))
       x = (numberOfDesktops()+y-1) / y;
    else if((y <=0) && (x > 0))
       y = (numberOfDesktops()+x-1) / x;

    if(x <=0)
       x = 1;
    if (y <= 0)
       y = 1;
    }

/*!
  Check whether \a w is a system tray window. If so, add it to the respective
  datastructures and propagate it to the world.
 */
bool Workspace::addSystemTrayWin( WId w )
    {
    if ( systemTrayWins.contains( w ) )
        return true;

    NETWinInfo ni( tqt_xdisplay(), w, root, NET::WMKDESystemTrayWinFor );
    WId trayWinFor = ni.kdeSystemTrayWinFor();
    if ( !trayWinFor )
        return false;
    systemTrayWins.append( SystemTrayWindow( w, trayWinFor ) );
    XSelectInput( tqt_xdisplay(), w,
                  StructureNotifyMask
                  );
    XAddToSaveSet( tqt_xdisplay(), w );
    propagateSystemTrayWins();
    return true;
    }

/*!
  Check whether \a w is a system tray window. If so, remove it from
  the respective datastructures and propagate this to the world.
 */
bool Workspace::removeSystemTrayWin( WId w, bool check )
    {
    if ( !systemTrayWins.contains( w ) )
        return false;
    if( check )
        {
    // When getting UnmapNotify, it's not clear if it's the systray
    // reparenting the window into itself, or if it's the window
    // going away. This is obviously a flaw in the design, and we were
    // just lucky it worked for so long. Kicker's systray temporarily
    // sets _TDE_SYSTEM_TRAY_EMBEDDING property on the window while
    // embedding it, allowing KWin to figure out. Kicker just mustn't
    // crash before removing it again ... *shrug* .
        int num_props;
        Atom* props = XListProperties( tqt_xdisplay(), w, &num_props );
        if( props != NULL )
            {
            for( int i = 0;
                 i < num_props;
                 ++i )
                if( props[ i ] == atoms->kde_system_tray_embedding )
                    {
                    XFree( props );
                    return false;
                    }
            XFree( props );
            }
        }
    systemTrayWins.remove( w );
    XRemoveFromSaveSet (tqt_xdisplay (), w);
    propagateSystemTrayWins();
    return true;
    }


/*!
  Propagates the systemTrayWins to the world
 */
void Workspace::propagateSystemTrayWins()
    {
    Window *cl = new Window[ systemTrayWins.count()];

    int i = 0;
    for ( SystemTrayWindowList::ConstIterator it = systemTrayWins.begin(); it != systemTrayWins.end(); ++it )
        {
        cl[i++] =  (*it).win;
        }

    rootInfo->setKDESystemTrayWindows( cl, i );
    delete [] cl;
    }


void Workspace::killWindowId( Window window_to_kill )
    {
    if( window_to_kill == None )
        return;
    Window window = window_to_kill;
    Client* client = NULL;
    for(;;)
        {
        client = findClient( FrameIdMatchPredicate( window ));
        if( client != NULL ) // found the client
            break;
        Window parent = 0L;
        Window root = 0L;
        Window* children = 0L;
        unsigned int children_count;
        XQueryTree( tqt_xdisplay(), window, &root, &parent, &children, &children_count );
        if( children != NULL )
            XFree( children );
        if( window == root ) // we didn't find the client, probably an override-redirect window
            break;
        window = parent; // go up
        if( window == 0L )
            break;
        }
    if( client != NULL )
        client->killWindow();
    else
        XKillClient( tqt_xdisplay(), window_to_kill );
    }

void Workspace::suspendWindowId( Window window_to_suspend )
    {
    if( window_to_suspend == None )
        return;
    Window window = window_to_suspend;
    Client* client = NULL;
    for(;;)
        {
        client = findClient( FrameIdMatchPredicate( window ));
        if( client != NULL ) // found the client
            break;
        Window parent = 0L;
        Window root = 0L;
        Window* children = 0L;
        unsigned int children_count;
        XQueryTree( tqt_xdisplay(), window, &root, &parent, &children, &children_count );
        if( children != NULL )
            XFree( children );
        if( window == root ) // we didn't find the client, probably an override-redirect window
            break;
        window = parent; // go up
        if( window == 0L )
            break;
        }
    if( client != NULL )
        client->suspendWindow();
    else
        return;
    }

void Workspace::resumeWindowId( Window window_to_resume )
    {
    if( window_to_resume == None )
        return;
    Window window = window_to_resume;
    Client* client = NULL;
    for(;;)
        {
        client = findClient( FrameIdMatchPredicate( window ));
        if( client != NULL ) // found the client
            break;
        Window parent = 0L;
        Window root = 0L;
        Window* children = 0L;
        unsigned int children_count;
        XQueryTree( tqt_xdisplay(), window, &root, &parent, &children, &children_count );
        if( children != NULL )
            XFree( children );
        if( window == root ) // we didn't find the client, probably an override-redirect window
            break;
        window = parent; // go up
        if( window == 0L )
            break;
        }
    if( client != NULL )
        client->resumeWindow();
    else
        return;
    }


bool Workspace::isResumeableWindowID( Window window_to_check )
    {
    if( window_to_check == None )
        return false;
    Window window = window_to_check;
    Client* client = NULL;
    for(;;)
        {
        client = findClient( FrameIdMatchPredicate( window ));
        if( client != NULL ) // found the client
            break;
        Window parent = 0L;
        Window root = 0L;
        Window* children = 0L;
        unsigned int children_count;
        XQueryTree( tqt_xdisplay(), window, &root, &parent, &children, &children_count );
        if( children != NULL )
            XFree( children );
        if( window == root ) // we didn't find the client, probably an override-redirect window
            break;
        window = parent; // go up
        if( window == 0L )
            break;
        }
    if( client != NULL )
        return client->isResumeable();
    else
        return false;
    }


void Workspace::sendPingToWindow( Window window, Time timestamp )
    {
    rootInfo->sendPing( window, timestamp );
    }

void Workspace::sendTakeActivity( Client* c, Time timestamp, long flags )
    {
    rootInfo->takeActivity( c->window(), timestamp, flags );
    pending_take_activity = c;
    }


/*!
  Takes a screenshot of the current window and puts it in the clipboard.
*/
void Workspace::slotGrabWindow()
    {
    if ( active_client )
        {
        TQPixmap snapshot = TQPixmap::grabWindow( active_client->frameId() );

        //No XShape - no work.
        if( Shape::available())
            {
            //As the first step, get the mask from XShape.
            int count, order;
            XRectangle* rects = XShapeGetRectangles( tqt_xdisplay(), active_client->frameId(),
                                                     ShapeBounding, &count, &order);
            //The ShapeBounding region is the outermost shape of the window;
            //ShapeBounding - ShapeClipping is defined to be the border.
            //Since the border area is part of the window, we use bounding
            // to limit our work region
            if (rects)
                {
                //Create a TQRegion from the rectangles describing the bounding mask.
                TQRegion contents;
                for (int pos = 0; pos < count; pos++)
                    contents += TQRegion(rects[pos].x, rects[pos].y,
                                        rects[pos].width, rects[pos].height);
                XFree(rects);

                //Create the bounding box.
                TQRegion bbox(0, 0, snapshot.width(), snapshot.height());

                //Get the masked away area.
                TQRegion maskedAway = bbox - contents;
                TQMemArray<TQRect> maskedAwayRects = maskedAway.rects();

                //Construct a bitmap mask from the rectangles
                TQBitmap mask( snapshot.width(), snapshot.height());
                TQPainter p(&mask);
                p.fillRect(0, 0, mask.width(), mask.height(), TQt::color1);
                for (uint pos = 0; pos < maskedAwayRects.count(); pos++)
                    p.fillRect(maskedAwayRects[pos], TQt::color0);
                p.end();
                snapshot.setMask(mask);
                }
            }

        TQClipboard *cb = TQApplication::clipboard();
        cb->setPixmap( snapshot );
        }
    else
        slotGrabDesktop();
    }

/*!
  Takes a screenshot of the whole desktop and puts it in the clipboard.
*/
void Workspace::slotGrabDesktop()
    {
    TQPixmap p = TQPixmap::grabWindow( tqt_xrootwin() );
    TQClipboard *cb = TQApplication::clipboard();
    cb->setPixmap( p );
    }


/*!
  Invokes keyboard mouse emulation
 */
void Workspace::slotMouseEmulation()
    {

    if ( mouse_emulation )
        {
        XUngrabKeyboard(tqt_xdisplay(), get_tqt_x_time());
        mouse_emulation = false;
        return;
        }

    if ( XGrabKeyboard(tqt_xdisplay(),
                       root, false,
                       GrabModeAsync, GrabModeAsync,
                       get_tqt_x_time()) == GrabSuccess )
        {
        mouse_emulation = true;
        mouse_emulation_state = 0;
        mouse_emulation_window = 0;
        }
    }

/*!
  Returns the child window under the mouse and activates the
  respective client if necessary.

  Auxiliary function for the mouse emulation system.
 */
WId Workspace::getMouseEmulationWindow()
    {
    Window root;
    Window child = tqt_xrootwin();
    int root_x, root_y, lx, ly;
    uint state;
    Window w;
    Client * c = 0;
    do
        {
        w = child;
        if (!c)
            c = findClient( FrameIdMatchPredicate( w ));
        XQueryPointer( tqt_xdisplay(), w, &root, &child,
                       &root_x, &root_y, &lx, &ly, &state );
        } while  ( child != None && child != w );

    if ( c && !c->isActive() )
        activateClient( c );
    return (WId) w;
    }

/*!
  Sends a faked mouse event to the specified window. Returns the new button state.
 */
unsigned int Workspace::sendFakedMouseEvent( TQPoint pos, WId w, MouseEmulation type, int button, unsigned int state )
    {
    if ( !w )
        return state;
    TQWidget* widget = TQWidget::find( w );
    if ( (!widget ||  widget->inherits("TQToolButton") ) && !findClient( WindowMatchPredicate( w )) )
        {
        int x, y;
        Window xw;
        XTranslateCoordinates( tqt_xdisplay(), tqt_xrootwin(), w, pos.x(), pos.y(), &x, &y, &xw );
        if ( type == EmuMove )
            { // motion notify events
            XEvent e;
            e.type = MotionNotify;
            e.xmotion.window = w;
            e.xmotion.root = tqt_xrootwin();
            e.xmotion.subwindow = w;
            e.xmotion.time = get_tqt_x_time();
            e.xmotion.x = x;
            e.xmotion.y = y;
            e.xmotion.x_root = pos.x();
            e.xmotion.y_root = pos.y();
            e.xmotion.state = state;
            e.xmotion.is_hint = NotifyNormal;
            XSendEvent( tqt_xdisplay(), w, True, ButtonMotionMask, &e );
            }
        else
            {
            XEvent e;
            e.type = type == EmuRelease ? ButtonRelease : ButtonPress;
            e.xbutton.window = w;
            e.xbutton.root = tqt_xrootwin();
            e.xbutton.subwindow = w;
            e.xbutton.time = get_tqt_x_time();
            e.xbutton.x = x;
            e.xbutton.y = y;
            e.xbutton.x_root = pos.x();
            e.xbutton.y_root = pos.y();
            e.xbutton.state = state;
            e.xbutton.button = button;
            XSendEvent( tqt_xdisplay(), w, True, ButtonPressMask, &e );

            if ( type == EmuPress )
                {
                switch ( button )
                    {
                    case 2:
                        state |= Button2Mask;
                        break;
                    case 3:
                        state |= Button3Mask;
                        break;
                    default: // 1
                        state |= Button1Mask;
                        break;
                    }
                }
            else
                {
                switch ( button )
                    {
                    case 2:
                        state &= ~Button2Mask;
                        break;
                    case 3:
                        state &= ~Button3Mask;
                        break;
                    default: // 1
                        state &= ~Button1Mask;
                        break;
                    }
                }
            }
        }
    return state;
    }

/*!
  Handles keypress event during mouse emulation
 */
bool Workspace::keyPressMouseEmulation( XKeyEvent& ev )
    {
    if ( root != tqt_xrootwin() )
        return false;
    int kc = XkbKeycodeToKeysym(tqt_xdisplay(), ev.keycode, 0, 0);
    int km = ev.state & (ControlMask | Mod1Mask | ShiftMask);

    bool is_control = km & ControlMask;
    bool is_alt = km & Mod1Mask;
    bool is_shift = km & ShiftMask;
    int delta = is_control?1:is_alt?32:8;
    TQPoint pos = TQCursor::pos();

    switch ( kc )
        {
        case XK_Left:
        case XK_KP_Left:
            pos.rx() -= delta;
            break;
        case XK_Right:
        case XK_KP_Right:
            pos.rx() += delta;
            break;
        case XK_Up:
        case XK_KP_Up:
            pos.ry() -= delta;
            break;
        case XK_Down:
        case XK_KP_Down:
            pos.ry() += delta;
            break;
        case XK_F1:
            if ( !mouse_emulation_state )
                mouse_emulation_window = getMouseEmulationWindow();
            if ( (mouse_emulation_state & Button1Mask) == 0 )
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuPress, Button1, mouse_emulation_state );
            if ( !is_shift )
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuRelease, Button1, mouse_emulation_state );
            break;
        case XK_F2:
            if ( !mouse_emulation_state )
                mouse_emulation_window = getMouseEmulationWindow();
            if ( (mouse_emulation_state & Button2Mask) == 0 )
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuPress, Button2, mouse_emulation_state );
            if ( !is_shift )
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuRelease, Button2, mouse_emulation_state );
            break;
        case XK_F3:
            if ( !mouse_emulation_state )
                mouse_emulation_window = getMouseEmulationWindow();
            if ( (mouse_emulation_state & Button3Mask) == 0 )
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuPress, Button3, mouse_emulation_state );
            if ( !is_shift )
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuRelease, Button3, mouse_emulation_state );
            break;
        case XK_Return:
        case XK_space:
        case XK_KP_Enter:
        case XK_KP_Space:
            {
            if ( !mouse_emulation_state )
                {
            // nothing was pressed, fake a LMB click
                mouse_emulation_window = getMouseEmulationWindow();
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuPress, Button1, mouse_emulation_state );
                mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuRelease, Button1, mouse_emulation_state );
                }
            else
                { // release all
                if ( mouse_emulation_state & Button1Mask )
                    mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuRelease, Button1, mouse_emulation_state );
                if ( mouse_emulation_state & Button2Mask )
                    mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuRelease, Button2, mouse_emulation_state );
                if ( mouse_emulation_state & Button3Mask )
                    mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuRelease, Button3, mouse_emulation_state );
                }
            }
    // fall through
        case XK_Escape:
            XUngrabKeyboard(tqt_xdisplay(), get_tqt_x_time());
            mouse_emulation = false;
            return true;
        default:
            return false;
        }

    TQCursor::setPos( pos );
    if ( mouse_emulation_state )
        mouse_emulation_state = sendFakedMouseEvent( pos, mouse_emulation_window, EmuMove, 0,  mouse_emulation_state );
    return true;

    }

/*!
  Returns the workspace's desktop widget. The desktop widget is
  sometimes required by clients to draw on it, for example outlines on
  moving or resizing.
 */
TQWidget* Workspace::desktopWidget()
    {
    return desktop_widget;
    }

//Delayed focus functions
void Workspace::delayFocus()
    {
    requestFocus( delayfocus_client );
    cancelDelayFocus();
    }

void Workspace::requestDelayFocus( Client* c )
    {
    delayfocus_client = c;
    delete delayFocusTimer;
    delayFocusTimer = new TQTimer( this );
    connect( delayFocusTimer, TQ_SIGNAL( timeout() ), this, TQ_SLOT( delayFocus() ) );
    delayFocusTimer->start( options->delayFocusInterval, true  );
    }

void Workspace::cancelDelayFocus()
    {
    delete delayFocusTimer;
    delayFocusTimer = 0;
    }

/* Active (Electric) Borders
 * ========================================================================
 * Active Border Window management. Active borders allow a user to switch
 * to another virtual desktop or activate other features by moving
 * the mouse pointer to the borders or corners of the workspace.
 * Technically this is done with input only windows.
 */
void Workspace::updateActiveBorders()
    {
    active_time_first = get_tqt_x_time();
    active_time_last = get_tqt_x_time();
    active_time_last_trigger = get_tqt_x_time();
    active_current_border = ActiveNone;
    TQRect r = TQApplication::desktop()->geometry();
    activeTop = r.top();
    activeBottom = r.bottom();
    activeLeft = r.left();
    activeRight = r.right();

    for (int pos = 0; pos < ACTIVE_BORDER_COUNT; ++pos)
    {
        if (active_reserved[pos] == 0)
        {
            if (active_windows[pos] != None)
            {
                XDestroyWindow( tqt_xdisplay(), active_windows[pos] );
            }
            active_windows[pos] = None;
            continue;
        }

        if (active_windows[pos] != None)
        {
            continue;
        }

        XSetWindowAttributes attributes;
        attributes.override_redirect = True;
        attributes.event_mask = EnterWindowMask;
        unsigned long valuemask = CWOverrideRedirect | CWEventMask;
        int xywh[ ACTIVE_BORDER_COUNT ][ 4 ] =
            {
                { r.left() + 1, r.top(), r.width() - 2, 1 }, // top
                { r.right(), r.top(), 1, 1 }, // topright
                { r.right(), r.top() + 1, 1, r.height() - 2 }, // etc.
                { r.right(), r.bottom(), 1, 1 },
                { r.left() + 1, r.bottom(), r.width() - 2, 1 },
                { r.left(), r.bottom(), 1, 1 },
                { r.left(), r.top() + 1, 1, r.height() - 2 },
                { r.left(), r.top(), 1, 1 }
            };
        active_windows[pos] = XCreateWindow(tqt_xdisplay(), tqt_xrootwin(),
                                              xywh[pos][0], xywh[pos][1],
                                              xywh[pos][2], xywh[pos][3],
                                              0, CopyFromParent, InputOnly,
                                              CopyFromParent, valuemask,
                                              &attributes);
        XMapWindow(tqt_xdisplay(), active_windows[pos]);

        // Set XdndAware on the windows, so that DND enter events are received (#86998)
        Atom version = 4; // XDND version
        XChangeProperty(tqt_xdisplay(), active_windows[pos],
                        atoms->xdnd_aware, XA_ATOM, 32, PropModeReplace,
                        (unsigned char*)&version, 1);
    }
}

void Workspace::destroyActiveBorders()
{
    for (int pos = 0; pos < ACTIVE_BORDER_COUNT; ++pos)
    {
        if (active_windows[ pos ] != None)
        {
            XDestroyWindow( tqt_xdisplay(), active_windows[ pos ] );
        }
        active_windows[ pos ] = None;
    }
}

void Workspace::reserveActiveBorderSwitching( bool reserve )
{
    for (int pos = 0; pos < ACTIVE_BORDER_COUNT; ++pos)
    {
        if (reserve)
        {
            reserveActiveBorder(static_cast<ActiveBorder>(pos));
        }
        else
        {
            unreserveActiveBorder(static_cast<ActiveBorder>(pos));
        }
    }
}

void Workspace::reserveActiveBorder( ActiveBorder border )
{
    if (border == ActiveNone)
        return;

    if (active_reserved[border]++ == 0)
        TQTimer::singleShot(0, this, TQ_SLOT(updateActiveBorders()));
}

void Workspace::unreserveActiveBorder( ActiveBorder border )
{
    if (border == ActiveNone)
        return;

    assert(active_reserved[ border ] > 0);
    if (--active_reserved[ border ] == 0)
        TQTimer::singleShot(0, this, TQ_SLOT(updateActiveBorders()));
}

void Workspace::checkActiveBorder(const TQPoint &pos, Time now)
{
    Time treshold_set = options->activeBorderDelay(); // set timeout
    Time treshold_trigger = 250; // Minimum time between triggers
    Time treshold_reset = 250; // reset timeout
    int activation_distance = options->borderActivationDistance();

    bool have_borders = false;
    for (int i = 0; i < ACTIVE_BORDER_COUNT; ++i)
    {
        if (active_windows[ i ] != None)
        {
            have_borders = true;
        }
    }
    if (!have_borders) {
        return;
    }

    // Mouse should not move more than this many pixels
    int distance_reset = activation_distance + 10;

    // Leave active maximizing mode when window moved away
    if (movingClient &&
       (options->activeBorders() == Options::ActiveTileMaximize ||
        options->activeBorders() == Options::ActiveTileOnly))
    {
        TQRect r = TQApplication::desktop()->screenGeometry(pos);
        activeTop = r.top();
        activeBottom = r.bottom();
        activeLeft = r.left();
        activeRight = r.right();

        if (active_current_border != ActiveNone &&
           (pos.x() > activeLeft   + distance_reset) &&
           (pos.x() < activeRight  - distance_reset) &&
           (pos.y() > activeTop    + distance_reset) &&
           (pos.y() < activeBottom - distance_reset))
        {
            movingClient->cancelActiveBorderMaximizing();
            return;
        }
    }

    // These checks take activation distance into account, creating a
    // virtual "activation band" for easier border/corner activation.
    bool active_left   = pos.x() < activeLeft   + activation_distance;
    bool active_right  = pos.x() > activeRight  - activation_distance;
    bool active_top    = pos.y() < activeTop    + activation_distance;
    bool active_bottom = pos.y() > activeBottom - activation_distance;

    if (!active_left && !active_right && !active_top && !active_bottom)
       return;

    // These checks are used to make corner activation easier: we assume
    // a 25% zone on the edge of each border where instead of half size
    // tiling we perform quarter size tiling. The rest 50% is left for
    // normal half size tiling.
    // These options make sense only for the tiling mode.
    int active_width_quart  = (activeRight - activeLeft) / 4;
    int active_height_quart = (activeBottom - activeTop) / 4;

    bool active_qleft   = false;
    bool active_qright  = false;
    bool active_qtop    = false;
    bool active_qbottom = false;
    if (options->activeBorders() == Options::ActiveTileMaximize ||
        options->activeBorders() == Options::ActiveTileOnly)
    {
      active_qleft   = pos.x() < activeLeft   + active_width_quart;
      active_qright  = pos.x() > activeRight  - active_width_quart;
      active_qtop    = pos.y() < activeTop    + active_height_quart;
      active_qbottom = pos.y() > activeBottom - active_height_quart;
    }

    ActiveBorder border = ActiveNone;
    if ((active_left && active_qtop) || (active_top && active_qleft))
    {
      border = ActiveTopLeft;
    }
    else if ((active_right && active_qtop) || (active_top && active_qright))
    {
      border = ActiveTopRight;
    }
    else if ((active_left && active_qbottom) || (active_bottom && active_qleft))
    {
      border = ActiveBottomLeft;
    }
    else if ((active_right && active_qbottom) || (active_bottom && active_qright))
    {
      border = ActiveBottomRight;
    }
    else if (active_left)
    {
      border = ActiveLeft;
    }
    else if (active_right)
    {
      border = ActiveRight;
    }
    else if (active_top)
    {
      border = ActiveTop;
    }
    else if (active_bottom)
    {
      border = ActiveBottom;
    }
    else
    {
      // Should never happen
      abort();
    }

    if( active_windows[border] == None )
    {
      return;
    }

    if ((active_current_border == border) &&
        (timestampDiff(active_time_last, now) < treshold_reset) &&
        (timestampDiff(active_time_last_trigger, now) > treshold_trigger) &&
        ((pos-active_push_point).manhattanLength() < distance_reset))
    {
        active_time_last = now;
        if (timestampDiff(active_time_first, now) > treshold_set)
        {
            active_time_last_trigger = now;
            active_current_border = ActiveNone;
            bool isSide = (border == ActiveTop || border == ActiveRight ||
                           border == ActiveBottom || border == ActiveLeft);

            if (movingClient)
            {
                // Desktop switching
                if (options->activeBorders() == Options::ActiveSwitchAlways ||
                    options->activeBorders() == Options::ActiveSwitchOnMove)
                {
                    activeBorderSwitchDesktop(border, pos);
                    return; // Don't reset cursor position
                }

                // Tiling maximize
                else if (options->activeBorders() == Options::ActiveTileMaximize &&
                         border == ActiveTop && movingClient->isMaximizable())
                {
                    if (!movingClient->isResizable()) return;
                    movingClient->setActiveBorderMode(ActiveMaximizeMode);
                    movingClient->setActiveBorderPos(pos);
                    movingClient->setActiveBorder(ActiveNone);
                    movingClient->setActiveBorderMaximizing(true);
                }

                // Tiling
                else if ((options->activeBorders() == Options::ActiveTileMaximize ||
                          options->activeBorders() == Options::ActiveTileOnly))
                {
                    if (!movingClient->isResizable()) return;
                    movingClient->setActiveBorderMode(ActiveTilingMode);
                    movingClient->setActiveBorderPos(pos);
                    movingClient->setActiveBorder(border);
                    movingClient->setActiveBorderMaximizing(true);
                }

                else
                {
                    return; // Don't reset cursor position
                }
            }
            else
            {
                // Desktop switching
                if (options->activeBorders() == Options::ActiveSwitchAlways && isSide)
                {
                    activeBorderSwitchDesktop(border, pos);
                    return; // Don't reset cursor position
                }
            }
        }
    }
    else
    {
        active_current_border = border;
        active_time_first = now;
        active_time_last = now;
        active_push_point = pos;
    }

    if ((options->activeBorders() == Options::ActiveSwitchAlways && !movingClient) ||
        activation_distance < 2)
    {
        // Reset the pointer to find out whether the user is really pushing
        // (ordered according to enum ActiveBorder minus ActiveNone)
        const int xdiff[ ACTIVE_BORDER_COUNT ] = { 0, -1, -1, -1, 0, 1, 1, 1 };
        const int ydiff[ ACTIVE_BORDER_COUNT ] = { 1, 1, 0, -1, -1, -1, 0, 1 };
        TQCursor::setPos(pos.x() + xdiff[border], pos.y() + ydiff[border]);
    }
}

void Workspace::activeBorderSwitchDesktop(ActiveBorder border, const TQPoint& _pos)
{
    TQPoint pos = _pos;
    TQRect r = TQApplication::desktop()->geometry();
    const int offset = 5;

    int desk_before = currentDesktop();
    if (border == ActiveLeft || border == ActiveTopLeft || border == ActiveBottomLeft)
    {
            slotSwitchDesktopLeft();
            pos.setX(r.width() - offset);
    }
    if (border == ActiveRight || border == ActiveTopRight || border == ActiveBottomRight)
    {
            slotSwitchDesktopRight();
            pos.setX(offset);
    }

    if (border == ActiveTop || border == ActiveTopLeft || border == ActiveTopRight)
    {
            slotSwitchDesktopUp();
            pos.setY(r.height() - offset);
    }
    if (border == ActiveBottom || border == ActiveBottomLeft || border == ActiveBottomRight)
    {
            slotSwitchDesktopDown();
            pos.setY(offset);
    }

    if (currentDesktop() != desk_before)
    {
        TQCursor::setPos(pos);
    }
}

// this function is called when the user entered an active border
// with the mouse. It may switch to another virtual desktop
bool Workspace::activeBorderEvent(XEvent *e)
{
    if (e->type == EnterNotify)
    {
        for (int i = 0; i < ACTIVE_BORDER_COUNT; ++i)
        {
            if (active_windows[i] != None && e->xcrossing.window == active_windows[i])
            { // the user entered an active border
                checkActiveBorder(TQPoint(e->xcrossing.x_root, e->xcrossing.y_root), e->xcrossing.time);
                return true;
            }
        }
    }
    if (e->type == ClientMessage)
    {
        if (e->xclient.message_type == atoms->xdnd_position)
        {
            for (int i = 0; i < ACTIVE_BORDER_COUNT; ++i)
            {
                if (active_windows[i] != None && e->xclient.window == active_windows[i])
                {
                    updateXTime();
                    checkActiveBorder(TQPoint(e->xclient.data.l[2]>>16, e->xclient.data.l[2]&0xffff), get_tqt_x_time());
                    return true;
                }
            }
        }
    }
    return false;
}

void Workspace::addTopMenu( Client* c )
    {
    assert( c->isTopMenu());
    assert( !topmenus.contains( c ));
    topmenus.append( c );
    if( managingTopMenus())
        {
        int minsize = c->minSize().height();
        if( minsize > topMenuHeight())
            {
            topmenu_height = minsize;
            updateTopMenuGeometry();
            }
        updateTopMenuGeometry( c );
        updateCurrentTopMenu();
        }
//        kdDebug() << "NEW TOPMENU:" << c << endl;
    }

void Workspace::removeTopMenu( Client* c )
    {
//    if( c->isTopMenu())
//        kdDebug() << "REMOVE TOPMENU:" << c << endl;
    assert( c->isTopMenu());
    assert( topmenus.contains( c ));
    topmenus.remove( c );
    updateCurrentTopMenu();
    // TODO reduce topMenuHeight() if possible?
    }

void Workspace::lostTopMenuSelection()
    {
//    kdDebug() << "lost TopMenu selection" << endl;
    // make sure this signal is always set when not owning the selection
    disconnect( topmenu_watcher, TQ_SIGNAL( lostOwner()), this, TQ_SLOT( lostTopMenuOwner()));
    connect( topmenu_watcher, TQ_SIGNAL( lostOwner()), this, TQ_SLOT( lostTopMenuOwner()));
    if( !managing_topmenus )
        return;
    connect( topmenu_watcher, TQ_SIGNAL( lostOwner()), this, TQ_SLOT( lostTopMenuOwner()));
    disconnect( topmenu_selection, TQ_SIGNAL( lostOwnership()), this, TQ_SLOT( lostTopMenuSelection()));
    managing_topmenus = false;
    delete topmenu_space;
    topmenu_space = NULL;
    updateClientArea();
    for( ClientList::ConstIterator it = topmenus.begin();
         it != topmenus.end();
         ++it )
        (*it)->checkWorkspacePosition();
    }

void Workspace::lostTopMenuOwner()
    {
    if( !options->topMenuEnabled())
        return;
//    kdDebug() << "TopMenu selection lost owner" << endl;
    if( !topmenu_selection->claim( false ))
        {
//        kdDebug() << "Failed to claim TopMenu selection" << endl;
        return;
        }
//    kdDebug() << "claimed TopMenu selection" << endl;
    setupTopMenuHandling();
    }

void Workspace::setupTopMenuHandling()
    {
    if( managing_topmenus )
        return;
    connect( topmenu_selection, TQ_SIGNAL( lostOwnership()), this, TQ_SLOT( lostTopMenuSelection()));
    disconnect( topmenu_watcher, TQ_SIGNAL( lostOwner()), this, TQ_SLOT( lostTopMenuOwner()));
    managing_topmenus = true;
    topmenu_space = new TQWidget;
    Window stack[ 2 ];
    stack[ 0 ] = supportWindow->winId();
    stack[ 1 ] = topmenu_space->winId();
    XRestackWindows(tqt_xdisplay(), stack, 2);
    updateTopMenuGeometry();
    topmenu_space->show();
    updateClientArea();
    updateCurrentTopMenu();
    }

int Workspace::topMenuHeight() const
    {
    if( topmenu_height == 0 )
        { // simply create a dummy menubar and use its preffered height as the menu height
        KMenuBar tmpmenu;
        tmpmenu.insertItem( "dummy" );
        topmenu_height = tmpmenu.sizeHint().height();
        }
    return topmenu_height;
    }

KDecoration* Workspace::createDecoration( KDecorationBridge* bridge )
    {
    return mgr->createDecoration( bridge );
    }

TQString Workspace::desktopName( int desk ) const
    {
    return TQString::fromUtf8( rootInfo->desktopName( desk ) );
    }

bool Workspace::checkStartupNotification( Window w, TDEStartupInfoId& id, TDEStartupInfoData& data )
    {
    return startup->checkStartup( w, id, data ) == TDEStartupInfo::Match;
    }

/*!
  Puts the focus on a dummy window
  Just using XSetInputFocus() with None would block keyboard input
 */
void Workspace::focusToNull()
    {
    XSetInputFocus(tqt_xdisplay(), null_focus_window, RevertToPointerRoot, get_tqt_x_time() );
    }

void Workspace::helperDialog( const TQString& message, const Client* c )
    {
    TQStringList args;
    TQString type;
    if( message == "noborderaltf3" )
        {
        TQString shortcut = TQString( "%1 (%2)" ).arg( keys->label( "Window Operations Menu" ))
            .arg( keys->shortcut( "Window Operations Menu" ).seq( 0 ).toString());
        args << "--msgbox" <<
              i18n( "You have selected to show a window without its border.\n"
                    "Without the border, you will not be able to enable the border "
                    "again using the mouse: use the window operations menu instead, "
                    "activated using the %1 keyboard shortcut." )
                .arg( shortcut );
        type = "altf3warning";
        }
    else if( message == "fullscreenaltf3" )
        {
        TQString shortcut = TQString( "%1 (%2)" ).arg( keys->label( "Window Operations Menu" ))
            .arg( keys->shortcut( "Window Operations Menu" ).seq( 0 ).toString());
        args << "--msgbox" <<
              i18n( "You have selected to show a window in fullscreen mode.\n"
                    "If the application itself does not have an option to turn the fullscreen "
                    "mode off you will not be able to disable it "
                    "again using the mouse: use the window operations menu instead, "
                    "activated using the %1 keyboard shortcut." )
                .arg( shortcut );
        type = "altf3warning";
        }
    else
        assert( false );
    TDEProcess proc;
    proc << "kdialog" << args;
    if( !type.isEmpty())
        {
        TDEConfig cfg( "twin_dialogsrc" );
        cfg.setGroup( "Notification Messages" ); // this depends on KMessageBox
        if( !cfg.readBoolEntry( type, true )) // has don't show again checked
            return;                           // save launching kdialog
        proc << "--dontagain" << "twin_dialogsrc:" + type;
        }
    if( c != NULL )
        proc << "--embed" << TQString::number( c->window());
    proc.start( TDEProcess::DontCare );
    }


// kompmgr stuff

void Workspace::createKompmgrProcess()
{
    kompmgr = new TDEProcess;
    connect(kompmgr, TQ_SIGNAL(receivedStderr(TDEProcess*, char*, int)), TQ_SLOT(handleKompmgrOutput(TDEProcess*, char*, int)));
    *kompmgr << TDE_COMPOSITOR_BINARY;
    *kompmgr << "--write-pid-path" << compositorPIDFile();
}

void Workspace::startKompmgr()
{
    // See if the desktop is loaded yet
    Atom type;
    int format;
    unsigned long length, after;
    unsigned char* data_root;
    Atom prop_root;
    prop_root = XInternAtom(tqt_xdisplay(), "_XROOTPMAP_ID", False);
    if( XGetWindowProperty( tqt_xdisplay(), tqt_xrootwin(), prop_root, 0L, 1L, False, AnyPropertyType, &type, &format, &length, &after, &data_root) == Success && data_root != NULL ) {
        // Root pixmap is available; OK to load...
    }
    else {
        // Try again a bit later!
        TQTimer::singleShot( 200, this, TQ_SLOT(startKompmgr()) );
        return;
    }
    pid_t kompmgrpid = getCompositorPID();
    if (kompmgrpid && kill(kompmgrpid, 0) >= 0)
        {
        // Active PID file detected; do not attempt to restart
        return;
        }
    if (!kompmgr || kompmgr->isRunning()) {
        kompmgrReloadSettings();
        return;
    }
    if (!kompmgr->start(TDEProcess::OwnGroup, TDEProcess::Stderr))
    {
        options->useTranslucency = false;
        TDEProcess proc;
        proc << "kdialog" << "--error"
            << i18n("The Composite Manager could not be started.\\nMake sure you have \"" TDE_COMPOSITOR_BINARY "\" in a $PATH directory.")
            << "--title" << "Composite Manager Failure";
        proc.start(TDEProcess::DontCare);
    }
    else
    {
        delete kompmgr_selection;
        char selection_name[ 100 ];
        sprintf( selection_name, "_NET_WM_CM_S%d", DefaultScreen( tqt_xdisplay()));
        kompmgr_selection = new TDESelectionOwner( selection_name );
        connect( kompmgr_selection, TQ_SIGNAL( lostOwnership()), TQ_SLOT( stopKompmgr()));
        kompmgr_selection->claim( true );
        connect(kompmgr, TQ_SIGNAL(processExited(TDEProcess*)), TQ_SLOT(restartKompmgr(TDEProcess*)));
        options->useTranslucency = true;
        //allowKompmgrRestart = false;
        //TQTimer::singleShot( 60000, this, TQ_SLOT(unblockKompmgrRestart()) );
        TQByteArray ba;
        TQDataStream arg(ba, IO_WriteOnly);
        arg << "";
        kapp->dcopClient()->emitDCOPSignal("default", "kompmgrStarted()", ba);
    }
    if (popup){ delete popup; popup = 0L; } // to add/remove opacity slider
}

void Workspace::stopKompmgr()
{
    if (!kompmgr || !kompmgr->isRunning()) {
        return;
    }
    delete kompmgr_selection;
    kompmgr_selection = NULL;
    kompmgr->disconnect(this, TQ_SLOT(restartKompmgr(TDEProcess*)));
    options->useTranslucency = false;
    if (popup){ delete popup; popup = 0L; } // to add/remove opacity slider
    kompmgr->kill(SIGKILL);
    TQByteArray ba;
    TQDataStream arg(ba, IO_WriteOnly);
    arg << "";
    kapp->dcopClient()->emitDCOPSignal("default", "kompmgrStopped()", ba);
}

void Workspace::kompmgrReloadSettings()
{
    if (!kompmgr || !kompmgr->isRunning()) {
        return;
    }
    kompmgr->kill(SIGUSR2);
}

bool Workspace::kompmgrIsRunning()
{
   return kompmgr && kompmgr->isRunning();
}

void Workspace::unblockKompmgrRestart()
{
    allowKompmgrRestart = true;
}

void Workspace::restartKompmgr( TDEProcess *proc )
// this is for inernal purpose (crashhandling) only, usually you want to use workspace->stopKompmgr(); TQTimer::singleShot(200, workspace, TQ_SLOT(startKompmgr()));
{
    bool crashed;
    if (proc->signalled()) {	// looks like kompmgr may have crashed
      int exit_signal_number = proc->exitSignal();
      if ( (exit_signal_number == SIGILL) || (exit_signal_number == SIGTRAP) || (exit_signal_number == SIGABRT) || (exit_signal_number == SIGSYS) || (exit_signal_number == SIGFPE) || (exit_signal_number == SIGBUS) || (exit_signal_number == SIGSEGV) ) {
        crashed = true;
      }
      else {
        crashed = false;
      }
      if (!allowKompmgrRestart)   // uh oh, it exited recently already
      {
          delete kompmgr_selection;
          kompmgr_selection = NULL;
          options->useTranslucency = false;
          if (crashed) {
            TDEProcess proc;
            proc << "kdialog" << "--error"
                << i18n( "The Composite Manager crashed twice within a minute and is therefore disabled for this session.")
                << "--title" << i18n("Composite Manager Failure");
            proc.start(TDEProcess::DontCare);
          }
          return;
      }
      if (!kompmgr)
            return;
// this should be useless, i keep it for maybe future need
//         if (!kcompmgr)
//             {
//             kompmgr = new TDEProcess;
//             kompmgr->clearArguments();
//             *kompmgr << TDE_COMPOSITOR_BINARY;
//             }
// -------------------
        if (!kompmgr->start(TDEProcess::NotifyOnExit, TDEProcess::Stderr))
        {
            delete kompmgr_selection;
            kompmgr_selection = NULL;
            options->useTranslucency = false;
            TDEProcess proc;
            proc << "kdialog" << "--error"
                << i18n("The Composite Manager could not be started.\\nMake sure you have \"" TDE_COMPOSITOR_BINARY "\" in a $PATH directory.")
                << "--title" << i18n("Composite Manager Failure");
            proc.start(TDEProcess::DontCare);
        }
        else
        {
            allowKompmgrRestart = false;
            TQTimer::singleShot( 60000, this, TQ_SLOT(unblockKompmgrRestart()) );
        }
    }
}

void Workspace::handleKompmgrOutput( TDEProcess* , char *buffer, int buflen)
{
    TQString message;
    TQString output = TQString::fromLocal8Bit( buffer, buflen );
    if (output.contains("Started",false))
        ; // don't do anything, just pass to the connection release
    else if (output.contains("Can't open display",false))
        message = i18n("<qt><b>The TDE composition manager failed to open the display</b><br>There is probably an invalid display entry in your ~/.compton-tde.conf file.</qt>");
    else if (output.contains("No render extension",false))
        message = i18n("<qt><b>The TDE composition manager cannot find the Xrender extension</b><br>You are using either an outdated or a crippled version of XOrg.<br>Get XOrg &ge; 6.8 from www.freedesktop.org.<br></qt>");
    else if (output.contains("No composite extension",false))
        message = i18n("<qt><b>Composite extension not found</b><br>You <i>must</i> use XOrg &ge; 6.8 for translucency and shadows to work.<br>Additionally, you need to add a new section to your X config file:<br>"
        "<i>Section \"Extensions\"<br>"
        "Option \"Composite\" \"Enable\"<br>"
        "EndSection</i></qt>");
    else if (output.contains("No damage extension",false))
        message = i18n("<qt><b>Damage extension not found</b><br>You <i>must</i> use XOrg &ge; 6.8 for translucency and shadows to work.</qt>");
    else if (output.contains("No XFixes extension",false))
        message = i18n("<qt><b>XFixes extension not found</b><br>You <i>must</i> use XOrg &ge; 6.8 for translucency and shadows to work.</qt>");
    else return; //skip others
    // kompmgr startup failed or succeeded, release connection
    kompmgr->closeStderr();
    disconnect(kompmgr, TQ_SIGNAL(receivedStderr(TDEProcess*, char*, int)), this, TQ_SLOT(handleKompmgrOutput(TDEProcess*, char*, int)));
    if( !message.isEmpty())
        {
        TDEProcess proc;
        proc << "kdialog" << "--error"
            << message
            << "--title" << i18n("Composite Manager Failure");
        proc.start(TDEProcess::DontCare);
        }
}

void Workspace::setOpacity(unsigned long winId, unsigned int opacityPercent)
{
    if (opacityPercent > 100) opacityPercent = 100;
    for( ClientList::ConstIterator it = stackingOrder().begin(); it != stackingOrder().end(); it++ )
        if (winId == (*it)->window())
            {
            (*it)->setOpacity(opacityPercent < 100, (unsigned int)((opacityPercent/100.0)*0xFFFFFFFF));
            return;
            }
}

void Workspace::setShadowSize(unsigned long winId, unsigned int shadowSizePercent)
{
    //this is open to the user by dcop - to avoid stupid trials, we limit the max shadow size to 400%
    if (shadowSizePercent > 400) shadowSizePercent = 400;
    for( ClientList::ConstIterator it = stackingOrder().begin(); it != stackingOrder().end(); it++ )
        if (winId == (*it)->window())
            {
            (*it)->setShadowSize(shadowSizePercent);
            return;
            }
}

void Workspace::setUnshadowed(unsigned long winId)
{
    for( ClientList::ConstIterator it = stackingOrder().begin(); it != stackingOrder().end(); it++ )
        if (winId == (*it)->window())
            {
            (*it)->setShadowSize(0);
            return;
            }
}

void Workspace::setShowingDesktop( bool showing )
    {
    rootInfo->setShowingDesktop( showing );
    showing_desktop = showing;
    ++block_showing_desktop;
    if( showing_desktop )
        {
        showing_desktop_clients.clear();
        ++block_focus;
        ClientList cls = stackingOrder();
        // find them first, then minimize, otherwise transients may get minimized with the window
        // they're transient for
        for( ClientList::ConstIterator it = cls.begin();
             it != cls.end();
             ++it )
            {
            if( (*it)->isOnCurrentDesktop() && (*it)->isShown( true ) && !(*it)->isSpecialWindow())
                showing_desktop_clients.prepend( *it ); // topmost first to reduce flicker
            }
        for( ClientList::ConstIterator it = showing_desktop_clients.begin();
             it != showing_desktop_clients.end();
             ++it )
            (*it)->minimize(true);
        --block_focus;
        if( Client* desk = findDesktop( true, currentDesktop()))
            requestFocus( desk );
        }
    else
        {
        for( ClientList::ConstIterator it = showing_desktop_clients.begin();
             it != showing_desktop_clients.end();
             ++it )
            (*it)->unminimize(true);
        if( showing_desktop_clients.count() > 0 )
            requestFocus( showing_desktop_clients.first());
        showing_desktop_clients.clear();
        }
    --block_showing_desktop;
    }

// Following Kicker's behavior:
// Changing a virtual desktop resets the state and shows the windows again.
// Unminimizing a window resets the state but keeps the windows hidden (except
// the one that was unminimized).
// A new window resets the state and shows the windows again, with the new window
// being active. Due to popular demand (#67406) by people who apparently
// don't see a difference between "show desktop" and "minimize all", this is not
// true if "showDesktopIsMinimizeAll" is set in twinrc. In such case showing
// a new window resets the state but doesn't show windows.
void Workspace::resetShowingDesktop( bool keep_hidden )
    {
    if( block_showing_desktop > 0 )
        return;
    rootInfo->setShowingDesktop( false );
    showing_desktop = false;
    ++block_showing_desktop;
    if( !keep_hidden )
        {
        for( ClientList::ConstIterator it = showing_desktop_clients.begin();
             it != showing_desktop_clients.end();
             ++it )
            (*it)->unminimize(true);
        }
    showing_desktop_clients.clear();
    --block_showing_desktop;
    }

// Activating/deactivating this feature works like this:
// When nothing is active, and the shortcut is pressed, global shortcuts are disabled
//   (using global_shortcuts_disabled)
// When a window that has disabling forced is activated, global shortcuts are disabled.
//   (using global_shortcuts_disabled_for_client)
// When a shortcut is pressed and global shortcuts are disabled (either by a shortcut
// or for a client), they are enabled again.
void Workspace::slotDisableGlobalShortcuts()
    {
    if( global_shortcuts_disabled || global_shortcuts_disabled_for_client )
        disableGlobalShortcuts( false );
    else
        disableGlobalShortcuts( true );
    }

static bool pending_dfc = false;

void Workspace::disableGlobalShortcutsForClient( bool disable )
    {
    if( global_shortcuts_disabled_for_client == disable )
        return;
    if( !global_shortcuts_disabled )
        {
        if( disable )
            pending_dfc = true;
        KIPC::sendMessageAll( KIPC::BlockShortcuts, disable );
        // twin will get the kipc message too
        }
    }

void Workspace::disableGlobalShortcuts( bool disable )
    {
    KIPC::sendMessageAll( KIPC::BlockShortcuts, disable );
    // twin will get the kipc message too
    }

void Workspace::kipcMessage( int id, int data )
    {
    if( id != KIPC::BlockShortcuts )
        return;
    if( pending_dfc && data )
        {
        global_shortcuts_disabled_for_client = true;
        pending_dfc = false;
        }
    else
        {
        global_shortcuts_disabled = data;
        global_shortcuts_disabled_for_client = false;
        }
    // update also Alt+LMB actions etc.
    for( ClientList::ConstIterator it = clients.begin();
         it != clients.end();
         ++it )
        (*it)->updateMouseGrab();
    }

} // namespace

#include "workspace.moc"
