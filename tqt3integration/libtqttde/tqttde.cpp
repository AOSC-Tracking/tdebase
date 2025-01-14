 /*
 *  This file is part of the Trinity Desktop Environment
 *
 *  Original file taken from the OpenSUSE tdebase builds
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

#include "tqttde.h"

#include <assert.h>
#include <dcopclient.h>
#include <dcoptypes.h>
#include <tqapplication.h>
#include <tqregexp.h>
#include <tqstringlist.h>
#include <tqwidget.h>
#include <unistd.h>

#include <X11/Xlib.h>

extern Time tqt_x_time;

static TQString convertFileFilter( const TQString& filter )
    {
    if( filter.isEmpty())
        return filter;
    TQString f2 = filter;
    f2.replace( '\n', ";;" ); // TQt says separator is ";;", but it also silently accepts newline
    f2.replace( '/', "\\/" ); // escape /'s for KFileDialog
    TQStringList items = TQStringList::split( ";;", f2 );
    TQRegExp reg( "\\((.*)\\)" );
    for( TQStringList::Iterator it = items.begin();
         it != items.end();
         ++it )
        {
        if( reg.search( *it ))
            *it = reg.cap( 1 ) + '|' + *it;
        }
    return items.join( "\n" );
    }

static TQString convertBackFileFilter( const TQString& filter )
    {
    if( filter.isEmpty())
        return filter;
    TQStringList items = TQStringList::split( "\n", filter );
    for( TQStringList::Iterator it = items.begin();
         it != items.end();
         ++it )
        {
        int pos = (*it).find( '|' );
        if( pos >= 0 )
            (*it) = (*it).mid( pos + 1 );
        }
    return items.join( ";;" );
    }

static DCOPClient* dcopClient()
    {
    DCOPClient* dcop = DCOPClient::mainClient();
    if( dcop == NULL )
        {
        static DCOPClient* dcop_private;
        if( dcop_private == NULL )
            {
            dcop_private = new DCOPClient;
            dcop_private->attach();
            }
        dcop = dcop_private;
        }
    static bool prepared = false;
    if( !prepared )
        {
        assert( tqApp != NULL ); // TODO
        prepared = true;
        dcop->bindToApp();
        if( !tqApp->inherits( "TDEApplication" )) // TDEApp takes care of input blocking
            {
            static tqttde_EventLoop* loop = new tqttde_EventLoop;
            TQObject::connect( dcop, TQ_SIGNAL( blockUserInput( bool )), loop, TQ_SLOT( block( bool )));
            }
        }
    return dcop;
    }

// defined in qapplication_x11.cpp
typedef int (*QX11EventFilter) (XEvent*);
extern QX11EventFilter tqt_set_x11_event_filter (QX11EventFilter filter);

static QX11EventFilter old_filter;

static int input_filter( XEvent* e )
    {
    switch( e->type )
        {
        case ButtonPress:
        case ButtonRelease:
        case KeyPress:
        case KeyRelease:
        case MotionNotify:
        case EnterNotify:
        case LeaveNotify:
            return true;
        default:
            break;
        }
    if( old_filter != NULL )
        return old_filter( e );
    return false;
    }

void tqttde_EventLoop::block( bool b )
    {
    if( b )
        old_filter = tqt_set_x11_event_filter( input_filter );
    else
        tqt_set_x11_event_filter( old_filter );
    }

// duped in kded module
static TQString getHostname()
    {
    char hostname[ 256 ];
    if( gethostname( hostname, 255 ) == 0 )
        {
        hostname[ 255 ] = '\0';
        return hostname;
        }
    return "";
    }

#include "tqttde_functions.cpp"

#include "tqttde.moc"
