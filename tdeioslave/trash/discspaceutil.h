/*
   This file is part of the TDE project

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

#ifndef DISCSPACEUTIL_H
#define DISCSPACEUTIL_H

#include <tqobject.h>
#include <tqstring.h>

/**
 * A small utility class to access and calculate
 * size and usage of mount points.
 */
class DiscSpaceUtil : public TQObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new disc space util.
     *
     * @param directory A directory the util shall work on.
     * @param parent The parent object.
     */
    explicit DiscSpaceUtil( const TQString &directory, TQObject *parent = 0 );

    /**
     * Returns the usage of the directory pass in the constructor on this
     * mount point in percent.
     *
     * @param additional An additional amount of bytes that is added to the
     *                   directory size before the usage is calculated.
     */
    double usage( unsigned long additional = 0 ) const;

    /**
     * Returns the size of the partition in kilo bytes.
     */
    unsigned long size() const;

    /**
     * Returns the mount point of the directory.
     */
    TQString mountPoint() const;

    /**
     * Returns the size of the given path in bytes.
     */
    static unsigned long sizeOfPath( const TQString &path );

  private slots:
    void foundMountPoint( const TQString&, unsigned long, unsigned long, unsigned long );
    void done();

  private:
    void calculateFullSize();

    TQString mDirectory;
    unsigned long mFullSize;
    TQString mMountPoint;
};

#endif
