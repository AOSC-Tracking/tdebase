/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2005 Lubos Lunak <l.lunak@kde.org>

relatively small extensions by Oswald Buddenhagen <ob6@inf.tu-dresden.de>

some code taken from the dcopserver (part of the KDE libraries), which is
Copyright (c) 1999 Matthias Ettrich <ettrich@kde.org>
Copyright (c) 1999 Preston Brown <pbrown@kde.org>

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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "server.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <unistd.h>

#include <tqtimer.h>

#include <tdeconfig.h>
#include <kdebug.h>
#include <twinmodule.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>


/*
 * Legacy session management
 */
const int WM_SAVE_YOURSELF_TIMEOUT = 4000;

static WindowMap* windowMapPtr = 0;

static Atom wm_save_yourself = None;
static Atom wm_protocols = None;
static Atom wm_client_leader = None;

static int winsErrorHandler(Display *, XErrorEvent *ev)
{
    if (windowMapPtr) {
        WindowMap::Iterator it = windowMapPtr->find(ev->resourceid);
        if (it != windowMapPtr->end())
            (*it).type = SM_ERROR;
    }
    return 0;
}

void KSMServer::performLegacySessionSave()
{
    kdDebug( 1218 ) << "Saving legacy session apps" << endl;
    // Setup error handler
    legacyWindows.clear();
    windowMapPtr = &legacyWindows;
    XErrorHandler oldHandler = XSetErrorHandler(winsErrorHandler);
    // Compute set of leader windows that need legacy session management
    // and determine which style (WM_COMMAND or WM_SAVE_YOURSELF)
    TWinModule module;
    if( wm_save_yourself == (Atom)None ) {
	Atom atoms[ 3 ];
	const char* const names[]
	    = { "WM_SAVE_YOURSELF", "WM_PROTOCOLS", "WM_CLIENT_LEADER" };
	XInternAtoms( tqt_xdisplay(), const_cast< char** >( names ), 3,
	    False, atoms );
	wm_save_yourself = atoms[ 0 ];
	wm_protocols = atoms[ 1 ];
	wm_client_leader = atoms[ 2 ];
    }
    for ( TQValueList<WId>::ConstIterator it = module.windows().begin();
	  it != module.windows().end(); ++it) {
        WId leader = windowWmClientLeader( *it );
        if (!legacyWindows.contains(leader) && windowSessionId( *it, leader ).isEmpty()) {
            SMType wtype = SM_WMCOMMAND;
            int nprotocols = 0;
            Atom *protocols = 0;
            if( XGetWMProtocols(tqt_xdisplay(), leader, &protocols, &nprotocols)) {
                for (int i=0; i<nprotocols; i++)
                    if (protocols[i] == wm_save_yourself) {
                        wtype = SM_WMSAVEYOURSELF;
                        break;
                    }
                XFree((void*) protocols);
            }
	    SMData data;
	    data.type = wtype;
            XClassHint classHint;
            if( XGetClassHint( tqt_xdisplay(), leader, &classHint ) ) {
                data.wmclass1 = classHint.res_name;
                data.wmclass2 = classHint.res_class;
                XFree( classHint.res_name );
                XFree( classHint.res_class );
            }
            legacyWindows.insert(leader, data);
        }
    }
    // Open fresh display for sending WM_SAVE_YOURSELF
    XSync(tqt_xdisplay(), False);
    Display *newdisplay = XOpenDisplay(DisplayString(tqt_xdisplay()));
    if (!newdisplay) {
	windowMapPtr = NULL;
	XSetErrorHandler(oldHandler);
	return;
    }
    WId root = DefaultRootWindow(newdisplay);
    XGrabKeyboard(newdisplay, root, False,
                  GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(newdisplay, root, False, Button1Mask|Button2Mask|Button3Mask,
                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    // Send WM_SAVE_YOURSELF messages
    XEvent ev;
    int awaiting_replies = 0;
    for (WindowMap::Iterator it = legacyWindows.begin(); it != legacyWindows.end(); ++it) {
        if ( (*it).type == SM_WMSAVEYOURSELF ) {
            WId w = it.key();
            awaiting_replies += 1;
            memset(&ev, 0, sizeof(ev));
            ev.xclient.type = ClientMessage;
            ev.xclient.window = w;
            ev.xclient.message_type = wm_protocols;
            ev.xclient.format = 32;
            ev.xclient.data.l[0] = wm_save_yourself;
            ev.xclient.data.l[1] = get_tqt_x_time();
            XSelectInput(newdisplay, w, PropertyChangeMask|StructureNotifyMask);
            XSendEvent(newdisplay, w, False, 0, &ev);
        }
    }
    // Wait for change in WM_COMMAND with timeout
    XFlush(newdisplay);
    TQTime start = TQTime::currentTime();
    while (awaiting_replies > 0) {
        if (XPending(newdisplay)) {
            /* Process pending event */
            XNextEvent(newdisplay, &ev);
            if ( ( ev.xany.type == UnmapNotify ) ||
                 ( ev.xany.type == PropertyNotify && ev.xproperty.atom == XA_WM_COMMAND ) ) {
                WindowMap::Iterator it = legacyWindows.find( ev.xany.window );
                if ( it != legacyWindows.end() && (*it).type != SM_WMCOMMAND ) {
                    awaiting_replies -= 1;
                    if ( (*it).type != SM_ERROR )
                        (*it).type = SM_WMCOMMAND;
                }
            }
        } else {
            /* Check timeout */
            int msecs = start.elapsed();
            if (msecs >= WM_SAVE_YOURSELF_TIMEOUT)
                break;
            /* Wait for more events */
            fd_set fds;
            FD_ZERO(&fds);
            int fd = ConnectionNumber(newdisplay);
            FD_SET(fd, &fds);
            struct timeval tmwait;
            tmwait.tv_sec = (WM_SAVE_YOURSELF_TIMEOUT - msecs) / 1000;
            tmwait.tv_usec = ((WM_SAVE_YOURSELF_TIMEOUT - msecs) % 1000) * 1000;
            ::select(fd+1, &fds, NULL, &fds, &tmwait);
        }
    }
    // Terminate work in new display
    XAllowEvents(newdisplay, ReplayPointer, CurrentTime);
    XAllowEvents(newdisplay, ReplayKeyboard, CurrentTime);
    XSync(newdisplay, False);
    XCloseDisplay(newdisplay);
    // Restore old error handler
    XSync(tqt_xdisplay(), False);
    XSetErrorHandler(oldHandler);
    for (WindowMap::Iterator it = legacyWindows.begin(); it != legacyWindows.end(); ++it) {
        if ( (*it).type != SM_ERROR) {
            WId w = it.key();
            (*it).wmCommand = windowWmCommand(w);
            (*it).wmClientMachine = windowWmClientMachine(w);
	}
    }
    kdDebug( 1218 ) << "Done saving " << legacyWindows.count() << " legacy session apps" << endl;
}

/*!
  Stores legacy session management data
*/
void KSMServer::storeLegacySession( TDEConfig* config )
{
    // Write LegacySession data
    config->deleteGroup( "Legacy" + sessionGroup );
    TDEConfigGroupSaver saver( config, "Legacy" + sessionGroup );
    int count = 0;
    for (WindowMap::ConstIterator it = legacyWindows.begin(); it != legacyWindows.end(); ++it) {
        if ( (*it).type != SM_ERROR) {
            if( excludeApps.contains( (*it).wmclass1.lower())
                || excludeApps.contains( (*it).wmclass2.lower()) || (*it).wmCommand[0] == "compiz" || (*it).wmCommand[0] == "beryl" || (*it).wmCommand[0] == "aquamarine" || (*it).wmCommand[0] == "beryl-manager" || (*it).wmCommand[0] == "beryl-settings" || (*it).wmCommand[0] == "kde-window-decorator" || (*it).wmCommand[0] == "emerald")
                continue;
            if ( !(*it).wmCommand.isEmpty() && !(*it).wmClientMachine.isEmpty() ) {
                count++;
                TQString n = TQString::number(count);
                config->writeEntry( TQString("command")+n, (*it).wmCommand );
                config->writeEntry( TQString("clientMachine")+n, (*it).wmClientMachine );
            }
        }
    }
    config->writeEntry( "count", count );
}

/*!
  Restores legacy session management data (i.e. restart applications)
*/
void KSMServer::restoreLegacySession( TDEConfig* config )
{
    if( config->hasGroup( "Legacy" + sessionGroup )) {
        TDEConfigGroupSaver saver( config, "Legacy" + sessionGroup );
        restoreLegacySessionInternal( config );
    } else if( wm == "twin" ) { // backwards comp. - get it from twinrc
	TDEConfigGroupSaver saver( config, sessionGroup );
	int count =  config->readNumEntry( "count", 0 );
	for ( int i = 1; i <= count; i++ ) {
    	    TQString n = TQString::number(i);
    	    if ( config->readEntry( TQString("program")+n ) != wm )
                continue;
    	    TQStringList restartCommand =
                config->readListEntry( TQString("restartCommand")+n );
	    for( TQStringList::ConstIterator it = restartCommand.begin();
		 it != restartCommand.end();
		 ++it ) {
		if( (*it) == "-session" ) {
		    ++it;
		    if( it != restartCommand.end()) {
			TDEConfig cfg( "session/" + wm + "_" + (*it), true );
			cfg.setGroup( "LegacySession" );
			restoreLegacySessionInternal( &cfg, ' ' );
		    }
		}
	    }
	}
    }
}

void KSMServer::restoreLegacySessionInternal( TDEConfig* config, char sep )
{
    int count = config->readNumEntry( "count" );
    for ( int i = 1; i <= count; i++ ) {
        TQString n = TQString::number(i);
        TQStringList wmCommand = config->readListEntry( TQString("command")+n, sep );
        if( wmCommand.isEmpty())
            continue;
        if( isWM( wmCommand.first()))
            continue;
        startApplication( wmCommand,
                          config->readEntry( TQString("clientMachine")+n ),
                          config->readEntry( TQString("userId")+n ));
    }
}
    
static TQCString getQCStringProperty(WId w, Atom prop)
{
    Atom type;
    int format, status;
    unsigned long nitems = 0;
    unsigned long extra = 0;
    unsigned char *data = 0;
    TQCString result = "";
    status = XGetWindowProperty( tqt_xdisplay(), w, prop, 0, 10000,
                                 False, XA_STRING, &type, &format,
                                 &nitems, &extra, &data );
    if ( status == Success) {
	if( data )
	    result = (char*)data;
        XFree(data);
    }
    return result;
}

static TQStringList getQStringListProperty(WId w, Atom prop)
{
    Atom type;
    int format, status;
    unsigned long nitems = 0;
    unsigned long extra = 0;
    unsigned char *data = 0;
    TQStringList result;

    status = XGetWindowProperty( tqt_xdisplay(), w, prop, 0, 10000,
                                 False, XA_STRING, &type, &format,
                                 &nitems, &extra, &data );
    if ( status == Success) {
	if (!data)
	    return result;
        for (int i=0; i<(int)nitems; i++) {
            result << TQString::fromLatin1( (const char*)data + i );
            while(data[i]) i++;
        }
        XFree(data);
    }
    return result;
}

TQStringList KSMServer::windowWmCommand(WId w)
{
    TQStringList ret = getQStringListProperty(w, XA_WM_COMMAND);
    // hacks here
    if( ret.count() == 1 ) {
        TQString command = ret.first();
        // Mozilla is launched using wrapper scripts, so it's launched using "mozilla",
        // but the actual binary is "mozilla-bin" or "<path>/mozilla-bin", and that's what
        // will be also in WM_COMMAND - using this "mozilla-bin" doesn't work at all though
        if( command.endsWith( "mozilla-bin" ))
            return TQStringList() << "mozilla";
        if( command.endsWith( "firefox-bin" ))
            return TQStringList() << "firefox";
        if( command.endsWith( "thunderbird-bin" ))
            return TQStringList() << "thunderbird";
        if( command.endsWith( "sunbird-bin" ))
            return TQStringList() << "sunbird";
    }
    return ret;
}

TQString KSMServer::windowWmClientMachine(WId w)
{
    TQCString result = getQCStringProperty(w, XA_WM_CLIENT_MACHINE);
    if (result.isEmpty()) {
        result = "localhost";
    } else {
        // special name for the local machine (localhost)
        char hostnamebuf[80];
        if (gethostname (hostnamebuf, sizeof hostnamebuf) >= 0) {
            hostnamebuf[sizeof(hostnamebuf)-1] = 0;
            if (result == hostnamebuf)
                result = "localhost";
            if(char *dot = strchr(hostnamebuf, '.')) {
                *dot = '\0';
                if(result == hostnamebuf)
                    result = "localhost";
            }
        }
    }
    return TQString::fromLatin1(result);
}

WId KSMServer::windowWmClientLeader(WId w)
{
    Atom type;
    int format, status;
    unsigned long nitems = 0;
    unsigned long extra = 0;
    unsigned char *data = 0;
    Window result = w;
    status = XGetWindowProperty( tqt_xdisplay(), w, wm_client_leader, 0, 10000,
                                 False, XA_WINDOW, &type, &format,
                                 &nitems, &extra, &data );
    if (status  == Success ) {
        if (data && nitems > 0)
            result = *((Window*) data);
        XFree(data);
    }
    return result;
}


/*
  Returns sessionId for this client,
  taken either from its window or from the leader window.
 */
extern Atom tqt_sm_client_id;
TQCString KSMServer::windowSessionId(WId w, WId leader)
{
    TQCString result = getQCStringProperty(w, tqt_sm_client_id);
    if (result.isEmpty() && leader != (WId)None && leader != w)
	result = getQCStringProperty(leader, tqt_sm_client_id);
    return result;
}
