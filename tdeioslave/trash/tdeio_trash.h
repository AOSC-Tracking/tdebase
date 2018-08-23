/* This file is part of the KDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

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

#ifndef TDEIO_TRASH_H
#define TDEIO_TRASH_H

#include <tdeio/slavebase.h>
#include "trashimpl.h"
namespace TDEIO { class Job; }

typedef TrashImpl::TrashedFileInfo TrashedFileInfo;
typedef TrashImpl::TrashedFileInfoList TrashedFileInfoList;

class TrashProtocol : public TQObject, public TDEIO::SlaveBase
{
    Q_OBJECT
public:
    TrashProtocol( const TQCString& protocol, const TQCString &pool, const TQCString &app);
    virtual ~TrashProtocol();
    virtual void stat(const KURL& url);
    virtual void listDir(const KURL& url);
    virtual void get( const KURL& url );
    virtual void put( const KURL& url, int , bool overwrite, bool );
    virtual void rename( const KURL &, const KURL &, bool );
    virtual void copy( const KURL &src, const KURL &dest, int permissions, bool overwrite );
    // TODO (maybe) chmod( const KURL& url, int permissions );
    virtual void del( const KURL &url, bool isfile );
    /**
     * Special actions: (first int in the byte array)
     * 1 : empty trash
     * 2 : migrate old (pre-kde-3.4) trash contents
     * 3 : restore a file to its original location. Args: KURL trashURL.
     */
    virtual void special( const TQByteArray & data );

private slots:
    void slotData( TDEIO::Job*, const TQByteArray& );
    void slotMimetype( TDEIO::Job*, const TQString& );
    void jobFinished( TDEIO::Job* job );

private:
    enum CopyOrMove { Copy, Move };
    void copyOrMove( const KURL& src, const KURL& dest, bool overwrite, CopyOrMove action );
    void createTopLevelDirEntry(TDEIO::UDSEntry& entry);
    bool createUDSEntry( const TQString& physicalPath, const TQString& fileName, const TQString& url,
                         TDEIO::UDSEntry& entry, const TrashedFileInfo& info );
    void listRoot();
    void restore( const KURL& trashURL );

    TrashImpl impl;
    TQString m_userName;
    TQString m_groupName;
};

#endif
