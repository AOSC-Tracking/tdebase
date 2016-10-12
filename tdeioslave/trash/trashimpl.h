/* This file is part of the TDE project
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

#ifndef TRASHIMPL_H
#define TRASHIMPL_H

#include <tdeio/jobclasses.h>
#include <ksimpleconfig.h>

#include <tqstring.h>
#include <tqdatetime.h>
#include <tqmap.h>
#include <tqvaluelist.h>
#include <tqstrlist.h>
#include <assert.h>

/**
 * Implementation of all low-level operations done by tdeio_trash
 * The structure of the trash directory follows the freedesktop.org standard <TODO URL>
 */
class TrashImpl : public TQObject
{
    Q_OBJECT
public:
    TrashImpl();

    /// Check the "home" trash directory
    /// This MUST be called before doing anything else
    bool init();

    /// Create info for a file to be trashed
    /// Returns trashId and fileId
    /// The caller is then responsible for actually trashing the file
    bool createInfo( const TQString& origPath, int& trashId, TQString& fileId );

    /// Delete info file for a file to be trashed
    /// Usually used for undoing what createInfo did if trashing failed
    bool deleteInfo( int trashId, const TQString& fileId );

    /// Moving a file or directory into the trash. The ids come from createInfo.
    bool moveToTrash( const TQString& origPath, int trashId, const TQString& fileId );

    /// Moving a file or directory out of the trash. The ids come from createInfo.
    bool moveFromTrash( const TQString& origPath, int trashId, const TQString& fileId, const TQString& relativePath );

    /// Copying a file or directory into the trash. The ids come from createInfo.
    bool copyToTrash( const TQString& origPath, int trashId, const TQString& fileId );

    /// Copying a file or directory out of the trash. The ids come from createInfo.
    bool copyFromTrash( const TQString& origPath, int trashId, const TQString& fileId, const TQString& relativePath );

    /// Create a top-level trashed directory
    //bool mkdir( int trashId, const TQString& fileId, int permissions );

    /// Get rid of a trashed file
    bool del( int trashId, const TQString& fileId );

    /// Empty trash, i.e. delete all trashed files
    bool emptyTrash();

    /// Return true if the trash is empty
    bool isEmpty() const;

    struct TrashedFileInfo {
        int trashId; // for the url
        TQString fileId; // for the url
        TQString physicalPath; // for stat'ing etc.
        TQString origPath; // from info file
        TQDateTime deletionDate; // from info file
    };
    /// List trashed files
    typedef TQValueList<TrashedFileInfo> TrashedFileInfoList;
    TrashedFileInfoList list();

    /// Return the info for a given trashed file
    bool infoForFile( int trashId, const TQString& fileId, TrashedFileInfo& info );

    /// Return the physicalPath for a given trashed file - helper method which
    /// encapsulates the call to infoForFile. Don't use if you need more info from TrashedFileInfo.
    TQString physicalPath( int trashId, const TQString& fileId, const TQString& relativePath );

    /// Move data from the old trash system to the new one
    void migrateOldTrash();

    /// TDEIO error code
    int lastErrorCode() const { return m_lastErrorCode; }
    TQString lastErrorMessage() const { return m_lastErrorMessage; }

    TQStrList listDir( const TQString& physicalPath );

    static KURL makeURL( int trashId, const TQString& fileId, const TQString& relativePath );
    static bool parseURL( const KURL& url, int& trashId, TQString& fileId, TQString& relativePath );

    typedef TQMap<int, TQString> TrashDirMap;
    /// @internal This method is for TestTrash only. Home trash is included (id 0).
    TrashDirMap trashDirectories() const;
    /// @internal This method is for TestTrash only. No entry with id 0.
    TrashDirMap topDirectories() const;

private:
    /// Helper method. Moves a file or directory using the appropriate method.
    bool move( const TQString& src, const TQString& dest );
    bool copy( const TQString& src, const TQString& dest );
    /// Helper method. Tries to call ::rename(src,dest) and does error handling.
    bool directRename( const TQString& src, const TQString& dest );

    void fileAdded();
    void fileRemoved();

    bool adaptTrashSize( const TQString& origPath, int trashId );

    // Warning, returns error code, not a bool
    int testDir( const TQString& name ) const;
    void error( int e, const TQString& s );

    bool readInfoFile( const TQString& infoPath, TrashedFileInfo& info, int trashId );

    TQString infoPath( int trashId, const TQString& fileId ) const;
    TQString filesPath( int trashId, const TQString& fileId ) const;

    /// Find the trash dir to use for a given file to delete, based on original path
    int findTrashDirectory( const TQString& origPath );

    TQString trashDirectoryPath( int trashId ) const;
    TQString topDirectoryPath( int trashId ) const;

    bool synchronousDel( const TQString& path, bool setLastErrorCode, bool isDir );

    void scanTrashDirectories() const;

    int idForTrashDirectory( const TQString& trashDir ) const;
    bool initTrashDirectory( const TQCString& trashDir_c ) const;
    bool checkTrashSubdirs( const TQCString& trashDir_c ) const;
    TQString trashForMountPoint( const TQString& topdir, bool createIfNeeded ) const;
    static TQString makeRelativePath( const TQString& topdir, const TQString& path );

private slots:
    void jobFinished(TDEIO::Job *job);

private:
    /// Last error code stored in class to simplify API.
    /// Note that this means almost no method can be const.
    int m_lastErrorCode;
    TQString m_lastErrorMessage;

    enum { InitToBeDone, InitOK, InitError } m_initStatus;

    // A "trash directory" is a physical directory on disk,
    // e.g. $HOME/.local/share/Trash/$uid or /mnt/foo/.Trash/$uid
    // It has an id (number) and a path.
    // The home trash has id 0.
    mutable TrashDirMap m_trashDirectories; // id -> path of trash directory
    mutable TrashDirMap m_topDirectories; // id -> $topdir of partition
    mutable int m_lastId;
    dev_t m_homeDevice;
    mutable bool m_trashDirectoriesScanned;
    int m_mibEnum;

    KSimpleConfig m_config;

    // We don't cache any data related to the trashed files.
    // Another tdeioslave could change that behind our feet.
    // If we want to start caching data - and avoiding some race conditions -,
    // we should turn this class into a kded module and use DCOP to talk to it
    // from the tdeioslave.
};

#endif
