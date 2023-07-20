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

#ifndef _MEDIAIMPL_H_
#define _MEDIAIMPL_H_

#include <tdeio/global.h>
#include <tdeio/job.h>
#include <kurl.h>
#include <dcopobject.h>

#include <tqobject.h>
#include <tqstring.h>

#include "medium.h"

class MediaImpl : public TQObject, public DCOPObject
{
TQ_OBJECT
K_DCOP
public:
	MediaImpl();
	bool parseURL(const KURL &url, TQString &name, TQString &path) const;
	bool realURL(const TQString &name, const TQString &path, KURL &url);

	bool statMedium(const TQString &name, TDEIO::UDSEntry &entry);
	bool statMediumByLabel(const TQString &label, TDEIO::UDSEntry &entry);
	bool listMedia(TQValueList<TDEIO::UDSEntry> &list);
	bool setUserLabel(const TQString &name, const TQString &label);

	void createTopLevelEntry(TDEIO::UDSEntry& entry) const;

	int lastErrorCode() const { return m_lastErrorCode; }
	TQString lastErrorMessage() const { return m_lastErrorMessage; }

k_dcop:
	void slotMediumChanged(const TQString &name);

signals:
	void warning(const TQString &msg);

private slots:
	void slotWarning(TDEIO::Job *job, const TQString &msg);
	void slotMountResult(TDEIO::Job *job);
	void slotStatResult(TDEIO::Job *job);

private:
	const Medium findMediumByName(const TQString &name, bool &ok);
	bool ensureMediumMounted(Medium &medium);

	TDEIO::UDSEntry extractUrlInfos(const KURL &url);
	TDEIO::UDSEntry m_entryBuffer;

	void createMediumEntry(TDEIO::UDSEntry& entry,
	                       const Medium &medium);

	Medium *mp_mounting;
	
	/// Last error code stored in class to simplify API.
	/// Note that this means almost no method can be const.
	int m_lastErrorCode;
	TQString m_lastErrorMessage;
};

#endif
