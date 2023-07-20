/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <tdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#ifndef TDEIO_PRINT_H
#define TDEIO_PRINT_H

#include <tdeio/slavebase.h>
#include <tdeio/global.h>
#include <tqstring.h>
#include <tqbuffer.h>

class KMPrinter;
namespace TDEIO {
	class Job;
}

class TDEIO_Print : public TQObject, public TDEIO::SlaveBase
{
	TQ_OBJECT
public:
	TDEIO_Print(const TQCString& pool, const TQCString& app);

	void listDir(const KURL& url);
	void get(const KURL& url);
	void stat(const KURL& url);

protected slots:
	void slotResult( TDEIO::Job* );
	void slotData( TDEIO::Job*, const TQByteArray& );
	void slotTotalSize( TDEIO::Job*, TDEIO::filesize_t );
	void slotProcessedSize( TDEIO::Job*, TDEIO::filesize_t );

private:
	void listRoot();
	void listDirDB( const KURL& );
	void statDB( const KURL& );
	bool getDBFile( const KURL& );
	void getDB( const KURL& );
	void showClassInfo(KMPrinter*);
	void showPrinterInfo(KMPrinter*);
	void showSpecialInfo(KMPrinter*);
	void showData(const TQString&);
	TQString locateData(const TQString&);
	void showJobs(KMPrinter *p = 0, bool completed = false);
	void showDriver(KMPrinter*);

	bool loadTemplate(const TQString& filename, TQString& buffer);

	TQBuffer m_httpBuffer;
	int     m_httpError;
	TQString m_httpErrorTxt;
};

#endif
