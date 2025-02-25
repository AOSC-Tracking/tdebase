/*
    KAppfinder, the KDE application finder

    Copyright (c) 2002-2003 Tobias Koenig <tokoe@kde.org>

    Based on code written by Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <kdebug.h>
#include <kdesktopfile.h>
#include <tdeglobal.h>
#include <tdestandarddirs.h>

#include <tqdir.h>
#include <tqfile.h>

#include <stdlib.h>

#include "common.h"

#define DBG_CODE 1213

void copyFile( const TQString &inFileName, const TQString &outFileName )
{
  TQFile inFile( inFileName );
  if ( inFile.open( IO_ReadOnly ) ) {
    TQFile outFile( outFileName );
    if ( outFile.open( IO_WriteOnly ) ) {
      outFile.writeBlock( inFile.readAll() );
      outFile.close();
    }

    inFile.close();
  }
}

bool scanDesktopFile( TQPtrList<AppLnkCache> &appCache, const TQString &templ,
                      TQString destDir )
{
  KDesktopFile desktop( templ, true );

  // find out where to put the .desktop files
  TQString destName;
  if ( destDir.isNull() )
    destDir = TDEGlobal::dirs()->saveLocation( "apps" );
  else
    destDir += "/";

  // find out the name of the file to store
  destName = templ;
  int pos = templ.find( "kappfinder/apps/" );
  if ( pos > 0 )
    destName = destName.mid( pos + strlen( "kappfinder/apps/" ) );

  // calculate real dir and filename
  destName = destDir + destName;
  pos = destName.findRev( '/' );
  if ( pos > 0 ) {
    destDir = destName.left( pos );
    destName = destName.mid( pos + 1 );
  }

  // determine for which executable to look
  TQString exec = desktop.readPathEntry( "TryExec" );
  if ( exec.isEmpty() )
    exec = desktop.readPathEntry( "Exec" );
  pos = exec.find( ' ' );
  if ( pos > 0 )
    exec = exec.left( pos );

  // try to locate the binary
  TQString pexec = TDEGlobal::dirs()->findExe( exec, 
                 TQString( ::getenv( "PATH" ) ) + ":/usr/X11R6/bin:/usr/games" );
  if ( pexec.isEmpty() ) {
    kdDebug(DBG_CODE) << "looking for " << exec.local8Bit()
                      << "\t\tnot found" << endl;
    return false;
  }

  AppLnkCache *cache = new AppLnkCache;
  cache->destDir = destDir;
  cache->destName = destName;
  cache->templ = templ;
  cache->item = 0;

  appCache.append( cache );

  kdDebug(DBG_CODE) << "looking for " << exec.local8Bit()
                    << "\t\tfound" << endl;
  return true;
}

void createDesktopFiles( TQPtrList<AppLnkCache> &appCache, int &added )
{
  AppLnkCache* cache;
  for ( cache = appCache.first(); cache; cache = appCache.next() ) {
    if ( cache->item == 0 || ( cache->item && cache->item->isOn() ) ) {
      added++;

      TQString destDir = cache->destDir;
      TQString destName = cache->destName;
      TQString templ = cache->templ;

      destDir += "/";
      TQDir d;
      int pos = -1;
      while ( ( pos = destDir.find( '/', pos + 1 ) ) >= 0 ) {
        TQString path = destDir.left( pos + 1 );
        d = path;
        if ( !d.exists() )
          d.mkdir( path );
      }

      // write out the desktop file
      copyFile( templ, destDir + "/" + destName );
    }
  }
}

void decorateDirs( TQString destDir )
{
  // find out where to put the .directory files
  if ( destDir.isNull() )
    destDir = TDEGlobal::dirs()->saveLocation( "apps" );
  else
    destDir += "/";

  TQStringList dirs = TDEGlobal::dirs()->findAllResources( "data", "kappfinder/apps/*.directory", true );

  TQStringList::Iterator it;
  for ( it = dirs.begin(); it != dirs.end(); ++it ) {
    // find out the name of the file to store
    TQString destName = *it;
    int pos = destName.find( "kappfinder/apps/" );
    if ( pos > 0 )
      destName = destName.mid( pos + strlen( "kappfinder/apps/" ) );

    destName = destDir + "/" + destName;

    if ( !TQFile::exists( destName ) ) {	
      kdDebug(DBG_CODE) << "Copy " << *it << " to " << destName << endl;
      copyFile( *it, destName );
    }
  }
}
