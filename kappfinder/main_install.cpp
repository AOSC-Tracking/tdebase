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

#include <tdecmdlineargs.h>
#include <tdeglobal.h>
#include <kinstance.h>
#include <tdelocale.h>
#include <tdestandarddirs.h>

#include <tqstringlist.h>

#include <stdio.h>

#include "common.h"


int main( int argc, char *argv[] )
{
  TDEInstance instance( "kappfinder_install" );
  int added = 0;

  if ( argc != 2 ) {
    fprintf( stderr, "Usage: kappfinder_install $directory\n" );
    return -1;
  }

  TQStringList templates = TDEGlobal::dirs()->findAllResources( "data", "kappfinder/apps/*.desktop", true );

  TQString dir = TQString( argv[ 1 ] ) + "/";

  TQPtrList<AppLnkCache> appCache;
  appCache.setAutoDelete( true );

  TQStringList::Iterator it;
  for ( it = templates.begin(); it != templates.end(); ++it )
    scanDesktopFile( appCache, *it, dir );

  createDesktopFiles( appCache, added );
  decorateDirs( dir );

  appCache.clear();

  printf( "%i application(s) added\n", added );

  return 0;
}
