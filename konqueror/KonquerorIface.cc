/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "KonquerorIface.h"
#include "konq_misc.h"
#include "KonqMainWindowIface.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include "konq_view.h"
#include <konq_settings.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <tqfile.h>
#include "konq_settingsxt.h"

// these DCOP calls come from outside, so any windows created by these
// calls would have old user timestamps (for KWin's no-focus-stealing),
// it's better to reset the timestamp and rely on other means
// of detecting the time when the user action that triggered all this
// happened
// TODO a valid timestamp should be passed in the DCOP calls that
// are not for user scripting
#include <X11/Xlib.h>

KonquerorIface::KonquerorIface()
 : DCOPObject( "KonquerorIface" )
{
}

KonquerorIface::~KonquerorIface()
{
}

DCOPRef KonquerorIface::openBrowserWindow( const TQString &url )
{
    SET_QT_X_USER_TIME(0);
    KonqMainWindow *res = KonqMisc::createSimpleWindow( KURL(url) );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::openBrowserWindowASN( const TQString &url, const TQCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return openBrowserWindow( url );
}

DCOPRef KonquerorIface::createNewWindow( const TQString &url )
{
    return createNewWindow( url, TQString::null, false );
}

DCOPRef KonquerorIface::createNewWindowASN( const TQString &url, const TQCString& startup_id, bool tempFile )
{
    kapp->setStartupId( startup_id );
    return createNewWindow( url, TQString::null, tempFile );
}

DCOPRef KonquerorIface::createNewWindowWithSelection( const TQString &url, TQStringList filesToSelect )
{
    SET_QT_X_USER_TIME(0);
    KonqMainWindow *res = KonqMisc::createNewWindow( KURL(url), KParts::URLArgs(), false, filesToSelect );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createNewWindowWithSelectionASN( const TQString &url, TQStringList filesToSelect, const TQCString &startup_id )
{
    kapp->setStartupId( startup_id );
    return createNewWindowWithSelection( url, filesToSelect );
}

DCOPRef KonquerorIface::createNewWindow( const TQString &url, const TQString &mimetype, bool tempFile )
{
    SET_QT_X_USER_TIME(0);
    KParts::URLArgs args;
    args.serviceType = mimetype;
    // Filter the URL, so that "kfmclient openURL gg:foo" works also when konq is already running
    KURL finalURL = KonqMisc::konqFilteredURL( 0, url );
    KonqMainWindow *res = KonqMisc::createNewWindow( finalURL, args, false, TQStringList(), tempFile );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createNewWindowASN( const TQString &url, const TQString &mimetype,
    const TQCString& startup_id, bool tempFile )
{
    kapp->setStartupId( startup_id );
    return createNewWindow( url, mimetype, tempFile );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfile( const TQString &path )
{
    SET_QT_X_USER_TIME(0);
    kdDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( const TQString &path ) " << endl;
    kdDebug(1202) << path << endl;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, TQString::null );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileASN( const TQString &path, const TQCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfile( path );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfile( const TQString & path, const TQString &filename )
{
    SET_QT_X_USER_TIME(0);
    kdDebug(1202) << "void KonquerorIface::createBrowserWindowFromProfile( path, filename ) " << endl;
    kdDebug(1202) << path << "," << filename << endl;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileASN( const TQString &path, const TQString &filename,
    const TQCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfile( path, filename );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURL( const TQString & path, const TQString &filename, const TQString &url )
{
    SET_QT_X_USER_TIME(0);
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KURL(url) );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURLASN( const TQString & path, const TQString &filename, const TQString &url,
    const TQCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfileAndURL( path, filename, url );
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURL( const TQString &path, const TQString &filename, const TQString &url, const TQString &mimetype )
{
    SET_QT_X_USER_TIME(0);
    KParts::URLArgs args;
    args.serviceType = mimetype;
    KonqMainWindow *res = KonqMisc::createBrowserWindowFromProfile( path, filename, KURL(url), args );
    if ( !res )
        return DCOPRef();
    return res->dcopObject();
}

DCOPRef KonquerorIface::createBrowserWindowFromProfileAndURLASN( const TQString & path, const TQString &filename, const TQString &url, const TQString &mimetype,
    const TQCString& startup_id )
{
    kapp->setStartupId( startup_id );
    return createBrowserWindowFromProfileAndURL( path, filename, url, mimetype );
}


void KonquerorIface::reparseConfiguration()
{
  KGlobal::config()->reparseConfiguration();
  KonqFMSettings::reparseConfiguration();

  TQPtrList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    TQPtrListIterator<KonqMainWindow> it( *mainWindows );
    for (; it.current(); ++it )
        it.current()->reparseConfiguration();
  }
}

void KonquerorIface::updateProfileList()
{
  TQPtrList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
  if ( !mainWindows )
    return;

  TQPtrListIterator<KonqMainWindow> it( *mainWindows );
  for (; it.current(); ++it )
    it.current()->viewManager()->profileListDirty( false );
}

TQString KonquerorIface::crashLogFile()
{
  return KonqMainWindow::s_crashlog_file->name();
}

TQValueList<DCOPRef> KonquerorIface::getWindows()
{
    TQValueList<DCOPRef> lst;
    TQPtrList<KonqMainWindow> *mainWindows = KonqMainWindow::mainWindowList();
    if ( mainWindows )
    {
      TQPtrListIterator<KonqMainWindow> it( *mainWindows );
      for (; it.current(); ++it )
        lst.append( DCOPRef( kapp->dcopClient()->appId(), it.current()->dcopObject()->objId() ) );
    }
    return lst;
}

void KonquerorIface::addToCombo( TQString url, TQCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboAdd, url, objId );
}

void KonquerorIface::removeFromCombo( TQString url, TQCString objId )
{
  KonqMainWindow::comboAction( KonqMainWindow::ComboRemove, url, objId );
}

void KonquerorIface::comboCleared( TQCString objId )
{
    KonqMainWindow::comboAction( KonqMainWindow::ComboClear,
				 TQString::null, objId );
}

bool KonquerorIface::processCanBeReused( int screen )
{
    if( qt_xscreen() != screen )
        return false; // this instance run on different screen, and Qt apps can't migrate
    if( KonqMainWindow::isPreloaded())
        return false; // will be handled by preloading related code instead
    TQPtrList<KonqMainWindow>* windows = KonqMainWindow::mainWindowList();
    if( windows == NULL )
        return true;
    TQStringList allowed_parts = KonqSettings::safeParts();
    bool all_parts_allowed = false;
    
    if( allowed_parts.count() == 1 && allowed_parts.first() == TQString::fromLatin1( "SAFE" ))
    {
        allowed_parts.clear();
        // is duplicated in client/kfmclient.cc
        allowed_parts << TQString::fromLatin1( "konq_iconview.desktop" )
                      << TQString::fromLatin1( "konq_multicolumnview.desktop" )
                      << TQString::fromLatin1( "konq_sidebartng.desktop" )
                      << TQString::fromLatin1( "konq_infolistview.desktop" )
                      << TQString::fromLatin1( "konq_treeview.desktop" )
                      << TQString::fromLatin1( "konq_detailedlistview.desktop" );
    }
    else if( allowed_parts.count() == 1 && allowed_parts.first() == TQString::fromLatin1( "ALL" ))
    {
        allowed_parts.clear();
        all_parts_allowed = true;
    }
    if( all_parts_allowed )
        return true;
    for( TQPtrListIterator<KonqMainWindow> it1( *windows );
         it1 != NULL;
         ++it1 )
    {
        kdDebug(1202) << "processCanBeReused: count=" << (*it1)->viewCount() << endl;
        const KonqMainWindow::MapViews& views = (*it1)->viewMap();
        for( KonqMainWindow::MapViews::ConstIterator it2 = views.begin();
             it2 != views.end();
             ++it2 )
        {
            kdDebug(1202) << "processCanBeReused: part=" << (*it2)->service()->desktopEntryPath() << ", URL=" << (*it2)->url().prettyURL() << endl;
            if( !allowed_parts.contains( (*it2)->service()->desktopEntryPath()))
                return false;
        }
    }
    return true;
}

void KonquerorIface::terminatePreloaded()
{
    if( KonqMainWindow::isPreloaded())
        kapp->exit();
}
