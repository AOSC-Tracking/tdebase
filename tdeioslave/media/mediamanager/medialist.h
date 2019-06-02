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

#ifndef _MEDIALIST_H_
#define _MEDIALIST_H_

#include <tqobject.h>

#include "medium.h"

class MediaList : public QObject
{
Q_OBJECT

public:
	MediaList();

	// FIXME: should be <const Medium> or something similar...
	const TQPtrList<Medium> list() const;
	const Medium *findById(const TQString &id) const;
	const Medium *findByName(const TQString &name) const;
	const Medium *findByNode(const TQString &node) const;
	const Medium *findByClearUdi(const TQString &name);

public:
	TQString addMedium(Medium *medium, bool allowNotification = true);
	bool removeMedium(const TQString &id, bool allowNotification = true);

	bool changeMediumState(const Medium &medium, bool allowNotification);
	bool changeMediumState(const TQString &id,
	                       const TQString &baseURL,
	                       bool allowNotification = true,
	                       const TQString &mimeType = TQString::null,
	                       const TQString &iconName = TQString::null,
	                       const TQString &label = TQString::null);
	bool changeMediumState(const TQString &id,
	                       const TQString &deviceNode,
	                       const TQString &mountPoint,
	                       const TQString &fsType, bool mounted,
	                       bool allowNotification = true,
	                       const TQString &mimeType = TQString::null,
	                       const TQString &iconName = TQString::null,
	                       const TQString &label = TQString::null);
	bool changeMediumState(const TQString &id, bool mounted,
	                       bool allowNotification = true,
	                       const TQString &mimeType = TQString::null,
	                       const TQString &iconName = TQString::null,
	                       const TQString &label = TQString::null);

	bool setUserLabel(const TQString &name, const TQString &label);

signals:
	void mediumAdded(const TQString &id, const TQString &name,
	                 bool allowNotification);
	void mediumRemoved(const TQString &id, const TQString &name,
	                   bool allowNotification);
	void mediumStateChanged(const TQString &id, const TQString &name,
	                        bool mounted, bool allowNotification);

private:
	TQPtrList<Medium> m_media;
	TQMap<TQString, Medium*> m_idMap;
	TQMap<TQString, Medium*> m_nameMap;
	TQMap<TQString, Medium*> m_nodeMap;
};

#endif
