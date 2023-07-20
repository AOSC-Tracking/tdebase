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

#ifndef _REMOVABLEBACKEND_H_
#define _REMOVABLEBACKEND_H_

#include "backendbase.h"

#include <tqobject.h>
#include <tqstringlist.h>

class RemovableBackend : public TQObject, public BackendBase
{
TQ_OBJECT

public:
	RemovableBackend(MediaList &list);
	virtual ~RemovableBackend();

	bool plug(const TQString &devNode, const TQString &label);
	bool unplug(const TQString &devNode);
	bool camera(const TQString &devNode);

private slots:
	void slotDirty(const TQString &path);

private:
	void handleMtabChange();

	static TQString generateId(const TQString &devNode);
	static TQString generateName(const TQString &devNode);

	TQStringList m_removableIds;
	TQStringList m_mtabIds;
};

#endif
