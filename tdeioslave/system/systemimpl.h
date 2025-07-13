/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

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

#ifndef SYSTEMIMPL_H
#define SYSTEMIMPL_H

#include <tdeio/global.h>
#include <tdeio/job.h>
#include <tdedesktopfile.h>
#include <kurl.h>
#include <dcopobject.h>

#include <tqobject.h>
#include <tqstring.h>

class SystemImpl : public TQObject
{
TQ_OBJECT
public:
	SystemImpl();

	void createTopLevelEntry(TDEIO::UDSEntry& entry) const;
	bool statByName(const TQString &filename, TDEIO::UDSEntry& entry);

	bool listRoot(TQValueList<TDEIO::UDSEntry> &list);

	bool parseURL(const KURL &url, TQString &name, TQString &path) const;
	bool realURL(const TQString &name, const TQString &path, KURL &url) const;

	int lastErrorCode() const { return m_lastErrorCode; }
	TQString lastErrorMessage() const { return m_lastErrorMessage; }

private slots:
	KURL findBaseURL(const TQString &filename) const;
	void slotEntries(TDEIO::Job *job, const TDEIO::UDSEntryList &list);
	void slotResult(TDEIO::Job *job);

private:
	void createEntry(TDEIO::UDSEntry& entry, const TQString &directory,
	                 const TQString &file);

	bool m_lastListingEmpty;

	TQString readPathINL(TQString filename);

	/// Last error code stored in class to simplify API.
	/// Note that this means almost no method can be const.
	int m_lastErrorCode;
	TQString m_lastErrorMessage;
};

#endif
