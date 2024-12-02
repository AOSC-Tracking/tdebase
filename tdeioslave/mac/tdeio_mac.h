/***************************************************************************
                          mac.cpp
                             -------------------
    copyright            : (C) 2002 Jonathan Riddell
    email                : jr@jriddell.org
    version              : 1.0
    release date         : 10 Feburary 2002
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tdeio/slavebase.h>
#include <tdeio/global.h>
#include <kurl.h>
#include <tdeprocess.h>

#include <tqstring.h>
#include <tqcstring.h>
#include <tqfile.h>
#include <tqtextstream.h>

class MacProtocol : public TQObject, public TDEIO::SlaveBase
{
    TQ_OBJECT
public:
    MacProtocol(const TQCString &pool, const TQCString &app);
    ~MacProtocol();
    virtual void get(const KURL& url );
    virtual void listDir(const KURL& url);
    virtual void stat(const KURL& url);
protected slots:
    void slotGetStdOutput(TDEProcess*, char*, int);
    void slotSetDataStdOutput(TDEProcess*, char *s, int len);
protected:
    TQString prepareHP(const KURL& _url);
    TQValueList<TDEIO::UDSAtom> makeUDS(const TQString& _line);
    int makeTime(TQString mday, TQString mon, TQString third);
    TQString getMimetype(TQString type, TQString app);
    TQValueList<TDEIO::UDSAtom> doStat(const KURL& url);

    TDEIO::filesize_t processedBytes;
    TQString standardOutputStream;
    TDEProcess* myTDEProcess;

    //for debugging
    //TQFile* logFile;
    //TQTextStream* logStream;
};
