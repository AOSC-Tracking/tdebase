/*****************************************************************

Copyright (c) 2002 Siegfried Nijssen <snijssen@liacs.nl>
Copyright (c) 2003 Lubos Lunak <l.lunak@suse.cz>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#define _MENUAPPLET_CPP_

#include "menuapplet.h"

#include <tqlayout.h>
#include <tqtooltip.h>
#include <tqvariant.h> // avoid X11 #define's

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <twin.h>
#include <twinmodule.h>

#include <netwm.h>

#include <X11/Xlib.h>

/*

 KMenuBar from KDE3.1 and older won't work very well with this applet.
 This is because TQMenuBar tries really hard to keep its preffered size,
 se even if the X window for the menubar has the size enforced by this
 applet, Qt thinks it has the size Qt wants. This results in parts
 of the menubar not being repainted. Also, old KMenuBar always forced
 with to be the width of the screen, so even if the menubar has only
 few entries, this applet will still indicate the menubar doesn't
 fit completely in it. There's no way to fix this, besides upgrading
 to KDE3.2.

*/


extern "C"
{
    TDE_EXPORT KPanelApplet* init( TQWidget* parent_P, const TQString& configFile_P )
    {
      TDEGlobal::locale()->insertCatalogue("kmenuapplet");
      return new KickerMenuApplet::Applet( configFile_P, parent_P );
    }
}

namespace KickerMenuApplet
{

static const int MOVE_DIFF = 100; // size increment for left/right menu moving
static const int GROW_WIDTH = 10; // width of grow buttons

const long SUPPORTED_WINDOW_TYPES = NET::NormalMask | NET::DesktopMask | NET::DockMask
                | NET::ToolbarMask | NET::MenuMask | NET::DialogMask | NET::OverrideMask
                | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask;

Applet::Applet( const TQString& configFile_P, TQWidget* parent_P )
    :   KPanelApplet( configFile_P, Normal, 0, parent_P, "menuapplet" ),
        DCOPObject( "menuapplet" ),
        module( NULL ),
	active_menu( NULL ),
        selection( NULL ),
        selection_watcher( NULL ),
        desktop_menu( false ),
        topEdgeOffset( 0 )
    {
    setBackgroundOrigin(AncestorOrigin);
    dcopclient.registerAs( "menuapplet", false );
    // toolbarAppearanceChanged(int) is sent when changing macstyle
    connect( tdeApp, TQ_SIGNAL( toolbarAppearanceChanged( int )),
        this, TQ_SLOT( readSettings()));
    claimSelection();
    readSettings();
    updateTopEdgeOffset();
    }

Applet::~Applet()
    {
    lostSelection(); // release all menu's before really loosing the selection
    delete selection;
    delete selection_watcher;
    delete module;
    TDEGlobal::locale()->removeCatalogue("kmenuapplet");
    }

void Applet::windowAdded( WId w_P )
    {
    NETWinInfo info( tqt_xdisplay(), w_P, tqt_xrootwin(), NET::WMWindowType );
    if( info.windowType( SUPPORTED_WINDOW_TYPES ) != NET::TopMenu )
	return;
//    kdDebug() << "embedding:" << w_P << endl;
    Window transient_for = KWin::transientFor( w_P );
    if( transient_for == None )
	return;
    MenuEmbed* embed;
    if( transient_for == tqt_xrootwin())
    {
        embed = new MenuEmbed( transient_for, true, this );
    }
    else
        {
        KWin::WindowInfo info2 = KWin::windowInfo( transient_for, NET::WMWindowType );
        embed = new MenuEmbed( transient_for,
            info2.windowType( SUPPORTED_WINDOW_TYPES ) == NET::Desktop, this );
        }
        embed->hide();
    embed->move( 0, -topEdgeOffset );
    embed->resize( embed->width(), height() + topEdgeOffset );
    embed->embed( w_P );
    if( embed->embeddedWinId() == None )
	{
	delete embed;
	return;
	}
    menus.append( embed );
    // in case the app mapped its menu after its mainwindow, check which menu should be shown
    activeWindowChanged( module->activeWindow());
    }

// - if the active window has its topmenu -> show the menu
// - if desktop menu is enabled (i.e. explicitly in kdesktop) :
//   - show it
//   - otherwise show nothing
void Applet::activeWindowChanged( WId w_P )
    {
//    kdDebug() << "active:" << w_P << endl;
    for( WId window = w_P;
	 window != None;
	 window = tryTransientFor( window ))
	{
	for( TQValueList< MenuEmbed* >::ConstIterator it = menus.begin();
	     it != menus.end();
	     ++it )
	    {
	    if( window == (*it)->mainWindow())
                {
                activateMenu( *it );
	        return;
		}
	    }
	}
//    kdDebug() << "no active" << endl;
    // No menu for active window found - if desktop menu
    // (in kdesktoprc) is enabled, use kdesktop's menu instead of none.
    bool try_desktop = desktop_menu;
    if( !try_desktop && w_P != None )
        { // also use the desktop menu if the active window is desktop
        KWin::WindowInfo info = KWin::windowInfo( w_P, NET::WMWindowType );
        if( info.windowType( SUPPORTED_WINDOW_TYPES ) == NET::Desktop )
            try_desktop = true;
        }
    if( try_desktop )
        {
	for( TQValueList< MenuEmbed* >::ConstIterator it = menus.begin();
	     it != menus.end();
	     ++it )
	    {
            if( (*it)->isDesktopMenu())
                {
                activateMenu( *it );
                return;
                }
            }
        }
    activateMenu( NULL );
    }

void Applet::activateMenu( MenuEmbed* embed_P )
    {
    if( embed_P != active_menu )
	{
//        kdDebug() << "activate:" << embed_P << endl;
	if( active_menu != NULL )
	    active_menu->hide();
	active_menu = embed_P;
    if( active_menu != NULL )
    {
	    active_menu->show();
        //if (embed->isDesktopMenu())
        {
            active_menu->setMinimumSize( width(), height() + topEdgeOffset );
        }
    }
    emit updateLayout();
    }
    
    setBackground();
    }

void Applet::updateMenuGeometry( MenuEmbed* embed )
    {
    if( embed == active_menu )
        emit updateLayout();
    }

// If there's no menu for the window, try finding menu for its mainwindow
// (where the window's WM_TRANSIENT_FOR property points).
// If the window is modal (_NET_WM_STATE_MODAL), stop.
WId Applet::tryTransientFor( WId w_P )
    {
    KWin::WindowInfo info = KWin::windowInfo( w_P, NET::WMState );
    if( info.state() & NET::Modal )
	return None;
    WId ret = KWin::transientFor( w_P );
    if( ret == tqt_xrootwin())
        ret = None;
    return ret;
    }

void Applet::menuLost( MenuEmbed* embed )
    {
    for( TQValueList< MenuEmbed* >::Iterator it = menus.begin();
	 it != menus.end();
	 ++it )
	{
	if( *it == embed )
	    {
	    menus.remove( it );
	    embed->deleteLater();
//	    kdDebug() << "deleting:" << (*it)->mainWindow() << endl;
	    if( embed == active_menu )
		{
		active_menu = NULL;
		// trigger selecting new active menu
		activeWindowChanged( module->activeWindow());
		}
	    return;
	    }
	}
    }

void Applet::positionChange( Position )
    {
    updateTopEdgeOffset();
    }

// Detect mouse movement at the top screen edge also if the menubar
// has a popup open - in such case, Qt has a grab, and this avoids
// Kicker's FittsLawFrame. Therefore move the menubar a bit higher,
// so that it actually is positioned exactly at the screen edge
// (i.e. at a negative y coordinate within this applet, due to
// Kicker's frame).
void Applet::updateTopEdgeOffset()
    {
    TQPoint p = topLevelWidget()->mapToGlobal( TQPoint( 0, 0 ));
    if( p.y() <= 2 ) // 2 = work also when running in appletproxy
        topEdgeOffset = mapToGlobal( TQPoint( 0, 0 )).y() - p.y();
    else
        topEdgeOffset = 0;
    if( active_menu != NULL )
        active_menu->move( active_menu->x(), -topEdgeOffset );
    }

void Applet::paletteChange(const TQPalette & /* oldPalette */)
{
    setBackground();
}

void Applet::moveEvent( TQMoveEvent* )
{
    setBackground();
}

void Applet::setBackground()
{
    if (active_menu)
        active_menu->setBackground();
}

void Applet::claimSelection()
    {
    assert( selection == NULL );
    selection = new TDESelectionOwner( makeSelectionAtom(), DefaultScreen( tqt_xdisplay()));
// force taking the selection, but don't kill previous owner
    if( selection->claim( true, false ))
	{
        delete selection_watcher;
        selection_watcher = NULL;
        connect( selection, TQ_SIGNAL( lostOwnership()), TQ_SLOT( lostSelection()));
        module = new TWinModule;
	connect( module, TQ_SIGNAL( windowAdded( WId )), this, TQ_SLOT( windowAdded( WId )));
	connect( module, TQ_SIGNAL( activeWindowChanged( WId )),
	    this, TQ_SLOT( activeWindowChanged( WId )));
	TQValueList< WId > windows = module->windows();
	for( TQValueList< WId >::ConstIterator it = windows.begin();
	     it != windows.end();
	     ++it )
	    windowAdded( *it );
	activeWindowChanged( module->activeWindow());
	}
    else
        lostSelection();
    }

void Applet::lostSelection()
    {
    if( selection == NULL )
        return;
//    kdDebug() << "lost selection" << endl;
    for( TQValueList< MenuEmbed* >::ConstIterator it = menus.begin();
	 it != menus.end();
	 ++it )
	delete (*it); // delete all MenuEmbed's = release all menus
    menus.clear();
    active_menu = NULL;
    if( selection_watcher == NULL )
        {
        selection_watcher = new TDESelectionWatcher( makeSelectionAtom(), DefaultScreen( tqt_xdisplay()));
        connect( selection_watcher, TQ_SIGNAL( lostOwner()), this, TQ_SLOT( claimSelection()));
        }
    delete module;
    module = NULL;
    selection->deleteLater();
    selection = NULL;
    // selection_watcher stays
    }

void Applet::readSettings()
    {
    TDEConfig cfg( "kdesktoprc", true );
    cfg.setGroup( "Menubar" );
    desktop_menu = cfg.readBoolEntry( "ShowMenubar", false );
    cfg.setGroup( "KDE" );
    if( cfg.readBoolEntry( "macStyle", false ) || desktop_menu )
        TQToolTip::remove( this );
    else
        TQToolTip::add( this, i18n(
            "You do not appear to have enabled the standalone menubar; "
            "enable it in the Behavior control module for desktop." ));
    if( !isDisabled() && active_menu == NULL )
        activeWindowChanged( module->activeWindow()); //enforce desktop_menu
    }

void Applet::configure()
    {
    readSettings();
    }

int Applet::widthForHeight( int ) const
    {
    if (active_menu)
        return active_menu->width();
    return 0; // we're stretch applet
    }

int Applet::heightForWidth( int ) const
    {
    // *shrug* running this applet in vertical mode is a bad idea anyway
    return 50;
    }

static Atom selection_atom = None;
static Atom msg_type_atom = None;

static
void initAtoms()
    {
    char nm[ 100 ];
    sprintf( nm, "_KDE_TOPMENU_OWNER_S%d", DefaultScreen( tqt_xdisplay()));
    char nm2[] = "_KDE_TOPMENU_MINSIZE";
    char* names[ 2 ] = { nm, nm2 };
    Atom atoms[ 2 ];
    XInternAtoms( tqt_xdisplay(), names, 2, False, atoms );
    selection_atom = atoms[ 0 ];
    msg_type_atom = atoms[ 1 ];
    }

Atom Applet::makeSelectionAtom()
    {
    if( selection_atom == None )
	initAtoms();
    return selection_atom;
    }

MenuEmbed::MenuEmbed( WId mainwindow_P, bool desktop_P,
    TQWidget* parent_P, const char* name_P )
    :   QXEmbed( parent_P, name_P ),
	main_window( mainwindow_P ),
        desktop( desktop_P )
    {
    setAutoDelete( false );
    }

void MenuEmbed::windowChanged( WId w_P )
    {
    if( w_P == None )
	static_cast< Applet* >( parent())->menuLost( this );
    }

bool MenuEmbed::x11Event( XEvent* ev_P )
    {
    if( ev_P->type == ConfigureRequest
	&& ev_P->xconfigurerequest.window == embeddedWinId()
        && ev_P->xconfigurerequest.value_mask & ( CWWidth | CWHeight ))
        {
	XConfigureRequestEvent& ev = ev_P->xconfigurerequest;
        TQSize new_size = size();
        if( ev.value_mask & CWWidth )
            new_size.setWidth( ev.width );
        if( ev.value_mask & CWHeight )
            new_size.setHeight( ev.height );
	// resize when the embedded window resizes (still obey min size)
//	kdDebug() << "RES:" << embeddedWinId() << ":" << ev.width << ":" << ev.height << endl;
	if( ev.width != width() || ev.height != height())
            {
            resize( ev.width, ev.height );
            static_cast< Applet* >( parent())->updateMenuGeometry( this );
            }
	sendSyntheticConfigureNotifyEvent();
//        int x, y;
//        unsigned int w, h, d, b;
//        Window root;
//        XGetGeometry( tqt_xdisplay(), embeddedWinId(), &root, &x, &y, &w, &h, &b, &d );
//        kdDebug() << "RES3:" << width() << ":" << height() << ":" << w << ":" << h << endl;
	return true;
	}
    return QXEmbed::x11Event( ev_P );
    }

void MenuEmbed::sendSyntheticConfigureNotifyEvent()
{
    TQPoint globalPos = mapToGlobal(TQPoint(0,0));
    if (embeddedWinId()) {
        XConfigureEvent c;
        memset(&c, 0, sizeof(c));
        c.type = ConfigureNotify;
        c.display = tqt_xdisplay();
        c.send_event = True;
        c.event = embeddedWinId();
        c.window = winId();
        c.x = globalPos.x();
        c.y = globalPos.y();
        c.width = width();
        c.height = height();
        c.border_width = 0;
        c.above = None;
        c.override_redirect = 0;
        XSendEvent(tqt_xdisplay(), c.event, true, StructureNotifyMask, (XEvent*)&c);
    }
}

void MenuEmbed::setMinimumSize( int w, int h )
{
    QXEmbed::setMinimumSize( w, h );
    // tell the menubar also the allowed minimum size
    // the applet won't allow resizing to smaller size
    if( embeddedWinId() != None )
        {
//        kdDebug() << "RES2:" << width() << ":" << height() << ":" << minimumWidth() << ":" << minimumHeight() << endl;
        XEvent ev;
        ev.xclient.display = tqt_xdisplay();
        ev.xclient.type = ClientMessage;
        ev.xclient.window = embeddedWinId();
        assert( msg_type_atom != None );
        ev.xclient.message_type = msg_type_atom;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = get_tqt_x_time();
        ev.xclient.data.l[1] = minimumWidth();
        ev.xclient.data.l[2] = minimumHeight();
        ev.xclient.data.l[3] = 0;
        ev.xclient.data.l[4] = 0;
        XSendEvent( tqt_xdisplay(), embeddedWinId(), False, NoEventMask, &ev );
        }
}

void MenuEmbed::setBackground()
{
    const TQPixmap *pbg = parentWidget()->backgroundPixmap();
    
    if (pbg)
    {
        TQPixmap bg(width(), height());
        bg.fill(parentWidget(), pos());
        setPaletteBackgroundPixmap(bg);
        setBackgroundOrigin(WidgetOrigin);
    }
    else
        unsetPalette();
    
    hide();
    show();
    //XClearArea(x11Display(), embeddedWinId(), 0, 0, 0, 0, True);
}

} // namespace

#include "menuapplet.moc"
