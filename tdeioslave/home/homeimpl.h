/* This file is part of the KDE project
   Copyright (c) 2005 Kevin Ottens <ervin ipsquad net>

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

#ifndef HOMEIMPL_H
#define HOMEIMPL_H

#include <tdeio/global.h>
#include <tdeio/job.h>
#include <kurl.h>
#include <kuser.h>

#include <tqstring.h>

class HomeImpl : public TQObject
{
TQ_OBJECT

public:
	HomeImpl();
	bool parseURL(const KURL &url, TQString &name, TQString &path) const;
	bool realURL(const TQString &name, const TQString &path, KURL &url);
		
	bool statHome(const TQString &name, TDEIO::UDSEntry &entry);
	bool listHomes(TQValueList<TDEIO::UDSEntry> &list);
	
	void createTopLevelEntry(TDEIO::UDSEntry &entry) const;

private slots:
	void slotStatResult(TDEIO::Job *job);
	
private:
	void createHomeEntry(TDEIO::UDSEntry& entry, const KUser &user);

	TDEIO::UDSEntry extractUrlInfos(const KURL &url);
	TDEIO::UDSEntry m_entryBuffer;
		
	
	long m_effectiveUid;
};

#endif
