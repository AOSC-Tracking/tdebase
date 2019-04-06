/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _FSTABBACKEND_H_
#define _FSTABBACKEND_H_

#include "backendbase.h"

#include <tqobject.h>
#include <tqstringlist.h>
#include <tqmap.h>

#if defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
#include <tqtimer.h>
#endif

class FstabBackend : public TQObject, public BackendBase
{
Q_OBJECT

public:
	FstabBackend(MediaList &list, bool networkSharesOnly = false);
	virtual ~FstabBackend();

	static void guess(const TQString &devNode, const TQString &mountPoint,
                          const TQString &fsType, bool mounted,
                          TQString &mimeType, TQString &iconName,
	                  TQString &label);

	TQString mount(const TQString &id);
	TQString unmount(const TQString &id);

private slots:
	void slotDirty(const TQString &path);
	void handleFstabChange(bool allowNotification = true);
	void handleMtabChange(bool allowNotification = true);

private:
	static TQString generateId(const TQString &devNode,
	                          const TQString &mountPoint);
	static TQString generateName(const TQString &devNode,
	                            const TQString &fsType);

	bool m_networkSharesOnly;
	TQStringList m_mtabIds;
        TQMap<TQString, TQString> m_mtabEntries;
	TQStringList m_fstabIds;
#if defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
	TQTimer m_mtabTimer;
#endif
};

#endif
