/*
   This file is part of the KDE project

   Copyright (C) 2008 Tobias Koenig <tokoe@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <tqapplication.h>
#include <tqdir.h>
#include <tqeventloop.h>
#include <tqfileinfo.h>

#include <kdiskfreesp.h>
#include <kdebug.h>

#include "discspaceutil.h"

DiscSpaceUtil::DiscSpaceUtil( const TQString &directory, TQObject *parent )
  : TQObject( parent ),
    mDirectory( directory ),
    mFullSize( 0 )
{
  calculateFullSize();
}

unsigned long DiscSpaceUtil::sizeOfPath( const TQString &path )
{
  TQFileInfo info( path );
  if ( !info.exists() ) {
    return 0;
  }

  if ( info.isFile() ) {
    return info.size();
  } else if ( info.isDir() ) {
    TQDir dir( path );
    const TQFileInfoList *infos = dir.entryInfoList( TQDir::Files | TQDir::Dirs | TQDir::NoSymLinks );
    TQFileInfoListIterator it( *infos );

    unsigned long sum = 0;
    TQFileInfo *info = 0;
    while ( (info = it.current()) != 0 ) {
      if ( info->fileName() != "." && info->fileName() != ".." )
      sum += sizeOfPath( info->absFilePath() );
      ++it;
    }

    return sum;
  } else {
    return 0;
  }
}

double DiscSpaceUtil::usage( unsigned long additional ) const
{
  if ( mFullSize == 0 )
    return 0;

  unsigned long sum = sizeOfPath( mDirectory );
  sum += additional;

  sum = sum/1024; // convert to kB

  return (((double)sum*100)/(double)mFullSize);
}

unsigned long DiscSpaceUtil::size() const
{
  return mFullSize;
}

TQString DiscSpaceUtil::mountPoint() const
{
  return mMountPoint;
}

void DiscSpaceUtil::foundMountPoint( const TQString &mountPoint, unsigned long kbSize, unsigned long, unsigned long )
{
  mFullSize = kbSize;
  mMountPoint = mountPoint;
}

void DiscSpaceUtil::done()
{
  tqApp->eventLoop()->exitLoop();
}

void DiscSpaceUtil::calculateFullSize()
{
  KDiskFreeSp *sp = KDiskFreeSp::findUsageInfo( mDirectory );
  connect( sp, SIGNAL( foundMountPoint( const TQString&, unsigned long, unsigned long, unsigned long ) ),
           this, SLOT( foundMountPoint( const TQString&, unsigned long, unsigned long, unsigned long ) ) );
  connect( sp, SIGNAL( done() ), this, SLOT( done() ) );

  tqApp->eventLoop()->enterLoop();
}

#include "discspaceutil.moc"
