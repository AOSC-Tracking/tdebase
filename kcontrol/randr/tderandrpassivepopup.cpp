/* 
 * Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "tderandrpassivepopup.h"

#include <tdeapplication.h>

// this class is just like KPassivePopup, but it keeps track of the widget
// it's supposed to be positioned next to, and adjust its position if that
// widgets moves (needed because after a resolution switch Kicker will
// reposition itself, causing normal KPassivePopup to stay at weird places)

KRandrPassivePopup::KRandrPassivePopup( TQWidget *parent, const char *name, WFlags f )
    : KPassivePopup( parent, name, f )
    {
    connect( &update_timer, TQ_SIGNAL( timeout()), TQ_SLOT( slotPositionSelf()));
    }
    
KRandrPassivePopup* KRandrPassivePopup::message( const TQString &caption, const TQString &text,
    const TQPixmap &icon, TQWidget *parent, const char *name, int timeout )
    {
    KRandrPassivePopup *pop = new KRandrPassivePopup( parent, name );
    pop->setAutoDelete( true );
    pop->setView( caption, text, icon );
    pop->setTimeout( timeout );
    pop->show();
    pop->startWatchingWidget( parent );
    return pop;
    }

void KRandrPassivePopup::startWatchingWidget( TQWidget* widget_P )
    {
    static Atom wm_state = XInternAtom( tqt_xdisplay() , "WM_STATE", False );
    Window win = widget_P->winId();
    bool x11_events = false;
    for(;;)
	{
	Window root, parent;
	Window* children;
	unsigned int nchildren;
	XQueryTree( tqt_xdisplay(), win, &root, &parent, &children, &nchildren );
	if( children != NULL )
	    XFree( children );
	if( win == root ) // huh?
	    break;
	win = parent;
	
	TQWidget* widget = TQWidget::find( win );
	if( widget != NULL )
	    {
	    widget->installEventFilter( this );
	    watched_widgets.append( widget );
	    }
	else
	    {
	    XWindowAttributes attrs;
	    XGetWindowAttributes( tqt_xdisplay(), win, &attrs );
	    XSelectInput( tqt_xdisplay(), win, attrs.your_event_mask | StructureNotifyMask );
	    watched_windows.append( win );
	    x11_events = true;
	    }
	Atom type;
	int format;
	unsigned long nitems, after;
	unsigned char* data;
	if( XGetWindowProperty( tqt_xdisplay(), win, wm_state, 0, 0, False, AnyPropertyType,
	    &type, &format, &nitems, &after, &data ) == Success )
	    {
	    if( data != NULL )
		XFree( data );
	    if( type != None ) // toplevel window
		break;
	    }
	}
    if( x11_events )
	tdeApp->installX11EventFilter( this );
    }
    	
bool KRandrPassivePopup::eventFilter( TQObject* o, TQEvent* e )
    {
    if( e->type() == TQEvent::Move && o->isWidgetType()
	&& watched_widgets.contains( static_cast<TQWidget*>( o )))
        TQTimer::singleShot( 0, this, TQ_SLOT( slotPositionSelf()));
    return false;
    }

bool KRandrPassivePopup::x11Event( XEvent* e )
    {
    if( e->type == ConfigureNotify && watched_windows.contains( e->xconfigure.window ))
	{
	if( !update_timer.isActive())
	    update_timer.start( 10, true );
	return false;
	}
    return KPassivePopup::x11Event( e );
    }
        
void KRandrPassivePopup::slotPositionSelf()
    {
    positionSelf();
    }
    
#include "tderandrpassivepopup.moc"
