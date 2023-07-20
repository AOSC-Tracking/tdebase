/*  This file is part of the KDE libraries
    Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>


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
#ifndef __tdeio_man_h__
#define __tdeio_man_h__


#include <tqstring.h>
#include <tqcstring.h>
#include <tqstringlist.h>
#include <tqdict.h>
#include <tqbuffer.h>


#include <tdeio/global.h>
#include <tdeio/slavebase.h>


class MANProtocol : public TQObject, public TDEIO::SlaveBase
{
    TQ_OBJECT

public:

    MANProtocol(const TQCString &pool_socket, const TQCString &app_socket);
    virtual ~MANProtocol();

    virtual void get(const KURL& url);
    virtual void stat(const KURL& url);

    virtual void mimetype(const KURL &url);
    virtual void listDir(const KURL &url);

    void outputError(const TQString& errmsg);
    void outputMatchingPages(const TQStringList &matchingPages);

    void showMainIndex();
    void showIndex(const TQString& section);

    // the following two functions are the interface to man2html
    void output(const char *insert);
    char *readManPage(const char *filename);

    static MANProtocol *self();

private slots:
  void slotGetStdOutput(TDEProcess*, char*, int);
  void slotGetStdOutputUtf8(TDEProcess*, char*, int);
    
private:
    void checkManPaths();
    TQStringList manDirectories();
    static bool hasManRecode(bool force=0);
    TQMap<TQString, TQString> buildIndexMap(const TQString& section);
    bool addWhatIs(TQMap<TQString, TQString>& i, const TQString& f, const TQString& mark);
    void parseWhatIs( TQMap<TQString, TQString> &i, TQTextStream &t, const TQString &mark );
    TQStringList findPages(const TQString& section, 
                          const TQString &title, 
                          bool full_path = true);

    void addToBuffer(const char *buffer, int buflen);
    TQString pageName(const TQString& page) const;
    TQStringList buildSectionList(const TQStringList& dirs) const;
    void constructPath(TQStringList& constr_path, TQStringList constr_catmanpath);
private:
    static MANProtocol *_self;
    TQCString lastdir;

    void findManPagesInSection(const TQString &dir, const TQString &title, bool full_path, TQStringList &list);
    TQStringList m_manpath; ///< Path of man directories
    TQStringList m_mandbpath; ///< Path of catman directories
    TQStringList section_names;

    TQString myStdStream;  
    TQString mySgml2RoffPath;
    void getProgramPath();

    TQCString m_htmlPath; ///< Path to TDE resources, encoded for HTML
    TQCString m_cssPath; ///< Path to TDE resources, encoded for CSS
    TQBuffer m_outputBuffer; ///< Buffer for the output
    TQString m_manCSSFile; ///< Path to tdeio_man.css
};


#endif
