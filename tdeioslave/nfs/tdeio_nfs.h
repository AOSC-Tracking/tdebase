/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

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

#ifndef TDEIO_NFS_H
#define TDEIO_NFS_H

#include <tdeio/slavebase.h>
#include <tdeio/global.h>

#include <tqmap.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqintdict.h>
#include <tqtimer.h>

#define PORTMAP  //this seems to be required to compile on Solaris
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

class NFSFileHandle
{
   public:
      NFSFileHandle();
      NFSFileHandle(const NFSFileHandle & handle);
      ~NFSFileHandle();
      NFSFileHandle& operator= (const NFSFileHandle& src);
      NFSFileHandle& operator= (const char* src);
      operator const char* () const {return m_handle;}
      bool isInvalid() const {return m_isInvalid;}
      void setInvalid() {m_isInvalid=true;}
//      time_t age() const;
   protected:
      char m_handle[NFS_FHSIZE+1];
      bool m_isInvalid;
//      time_t m_detectTime;
};

//ostream& operator<<(ostream&, const NFSFileHandle&);

typedef TQMap<TQString,NFSFileHandle> NFSFileHandleMap;


class NFSProtocol : public TDEIO::SlaveBase
{
   public:
      NFSProtocol (const TQCString &pool, const TQCString &app );
      virtual ~NFSProtocol();

      virtual void openConnection();
      virtual void closeConnection();

      virtual void setHost( const TQString& host, int port, const TQString& user, const TQString& pass );

      virtual void put( const KURL& url, int _mode,bool _overwrite, bool _resume );
      virtual void get( const KURL& url );
      virtual void listDir( const KURL& url);
      virtual void symlink( const TQString &target, const KURL &dest, bool );
      virtual void stat( const KURL & url);
      virtual void mkdir( const KURL& url, int permissions );
      virtual void del( const KURL& url, bool isfile);
      virtual void chmod(const KURL& url, int permissions );
      virtual void rename(const KURL &src, const KURL &dest, bool overwrite);
      virtual void copy( const KURL& src, const KURL &dest, int mode, bool overwrite );
   protected:
//      void createVirtualDirEntry(TDEIO::UDSEntry & entry);
      bool checkForError(int clientStat, int nfsStat, const TQString& text);
      bool isExportedDir(const TQString& path);
      void completeUDSEntry(TDEIO::UDSEntry& entry, fattr& attributes);
      void completeBadLinkUDSEntry(TDEIO::UDSEntry& entry, fattr& attributes);
      void completeAbsoluteLinkUDSEntry(TDEIO::UDSEntry& entry, const TQCString& path);
      bool isValidLink(const TQString& parentDir, const TQString& linkDest);
//      bool isAbsoluteLink(const TQString& path);
      
      NFSFileHandle getFileHandle(TQString path);

      NFSFileHandleMap m_handleCache;
      TQIntDict<TQString> m_usercache;      // maps long ==> TQString *
      TQIntDict<TQString> m_groupcache;

      TQStringList m_exportedDirs;
      TQString m_currentHost;
      CLIENT *m_client;
      CLIENT *m_nfsClient;
      timeval total_timeout;
      timeval pertry_timeout;
      int m_sock;
      time_t m_lastCheck;
      void checkForOldFHs();
};

#endif
