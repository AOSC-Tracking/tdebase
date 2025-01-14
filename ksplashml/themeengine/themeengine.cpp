/***************************************************************************
 *   Copyright Brian Ledbetter 2001-2003 <brian@shadowcom.net>             *
 *   Copyright Ravikiran Rajagopal 2003 <ravi@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation. (The original KSplash/ML   *
 *   codebase (upto version 0.95.3) is BSD-licensed.)                      *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <twin.h>

#include <tqevent.h>
#include <tqfile.h>
#include <tqwidget.h>

#include <X11/Xlib.h>

#ifdef HAVE_XCURSOR
# include <X11/Xlib.h>
# include <X11/Xcursor/Xcursor.h>
#endif

#include "objkstheme.h"
#include "themeengine.h"
#include "themeengine.moc"

struct ThemeEngine::ThemeEnginePrivate
{
    TQValueList< Window > mSplashWindows;
};

ThemeEngine::ThemeEngine( TQWidget *, const char *, const TQStringList& args )
  : TQVBox( 0, "wndSplash", (WFlags)(WStyle_Customize|WX11BypassWM) ), d(0), mUseWM(false)
{
  d = new ThemeEnginePrivate;
  tdeApp->installX11EventFilter( this );
  tdeApp->installEventFilter( this );
  (void)tdeApp->desktop();
  XWindowAttributes rootAttr;
  XGetWindowAttributes(tqt_xdisplay(), RootWindow(tqt_xdisplay(),
                        tqt_xscreen()), &rootAttr);
  XSelectInput( tqt_xdisplay(), tqt_xrootwin(),
        SubstructureNotifyMask | rootAttr.your_event_mask );
  if (args.isEmpty())
    mTheme = new ObjKsTheme( "Default" );
  else
    mTheme = new ObjKsTheme( args.first() );
  if (args.first() == "Unified") {
    mUseWM = true;
  }
  mTheme->loadCmdLineArgs( TDECmdLineArgs::parsedArgs() );
}

ThemeEngine::~ThemeEngine()
{
    delete d;
}

/*
 This is perhaps a bit crude, but I'm not aware of any better way
 of fixing #85030 and keeping backwards compatibility if there
 are any 3rd party splashscreens. Check all toplevel windows,
 force them to be WX11BypassWM (so that ksplash can handle their stacking),
 and keep them on top, even above all windows handled by KWin.
*/
bool ThemeEngine::eventFilter( TQObject* o, TQEvent* e )
{
    if( e->type() == TQEvent::Show && o->isWidgetType())
        addSplashWindow( static_cast<TQWidget*>( o ));
    return false;
}

namespace
{
class HackWidget : public TQWidget { friend class ::ThemeEngine; };
}

void ThemeEngine::addSplashWindow( TQWidget* w )
{
    if( !w->isTopLevel())
        return;
    if( d->mSplashWindows.contains( w->winId()))
        return;
    if( !w->testWFlags( WX11BypassWM ) && (mUseWM == false))
    { // All toplevel widgets should be probably required to be WX11BypassWM
      // for KDE4 instead of this ugly hack.
        static_cast< HackWidget* >( w )->setWFlags( WX11BypassWM );
        XSetWindowAttributes attrs;
        attrs.override_redirect = True;
        XChangeWindowAttributes( tqt_xdisplay(), w->winId(), CWOverrideRedirect, &attrs );
    }
    d->mSplashWindows.prepend( w->winId());
    connect( w, TQ_SIGNAL( destroyed( TQObject* )), TQ_SLOT( splashWindowDestroyed( TQObject* )));
    w->raise();
}

void ThemeEngine::splashWindowDestroyed( TQObject* obj )
{
    d->mSplashWindows.remove( static_cast<TQWidget*>( obj )->winId());
}

bool ThemeEngine::x11Event( XEvent* e )
{
    if( e->type != ConfigureNotify && e->type != MapNotify )
        return false;
    if( e->type == ConfigureNotify && e->xconfigure.event != tqt_xrootwin())
        return false;
    if( e->type == MapNotify && e->xmap.event != tqt_xrootwin())
        return false;
    if( d->mSplashWindows.count() == 0 )
        return false;
    // this restacking is written in a way so that
    // if the stacking positions actually don't change,
    // all restacking operations will be no-op,
    // and no ConfigureNotify will be generated,
    // thus avoiding possible infinite loops
    XRaiseWindow( tqt_xdisplay(), d->mSplashWindows.first()); // raise topmost
    // and stack others below it
    Window* stack = new Window[ d->mSplashWindows.count() ];
    int count = 0;
    for( TQValueList< Window >::ConstIterator it = d->mSplashWindows.begin();
         it != d->mSplashWindows.end();
         ++it )
        stack[ count++ ] = *it;
    XRestackWindows( x11Display(), stack, count );
    delete[] stack;
    return false;
}
