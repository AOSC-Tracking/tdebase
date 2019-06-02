/* This file is part of the KDE Project
   Copyright (c) 2004 K�vin Ottens <ervin ipsquad net>

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

#ifndef _MEDIAMANAGER_H_
#define _MEDIAMANAGER_H_

#include <kdedmodule.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqvariant.h>

#include "medialist.h"
#include "backendbase.h"
#include "removablebackend.h"
#include "mediadirnotify.h"

class HALBackend;
class TDEBackend;
class FstabBackend;

class MediaManager : public KDEDModule
{
Q_OBJECT
K_DCOP
public:
	MediaManager(const TQCString &obj);
	~MediaManager();

k_dcop:
	TQStringList fullList();
	TQStringList properties(const TQString &name);
	TQStringList mountoptions(const TQString &name);
	bool setMountoptions(const TQString &name, const TQStringList &options);

	TQStringVariantMap mount(const TQString &uid);
	TQStringVariantMap unmount(const TQString &uid);
	TQStringVariantMap unlock(const TQString &uid, const TQString &password);
	TQStringVariantMap lock(const TQString &uid);

	TQStringVariantMap mountByNode(const TQString &deviceNode);
	TQStringVariantMap unmountByNode(const TQString &deviceNode);
	TQStringVariantMap unlockByNode(const TQString &deviceNode, const TQString &password);
	TQStringVariantMap lockByNode(const TQString &deviceNode);

	TQString nameForLabel(const TQString &label);
	ASYNC setUserLabel(const TQString &name, const TQString &label);

	ASYNC reloadBackends();

	// Removable media handling (for people not having HAL)
	bool removablePlug(const TQString &devNode, const TQString &label);
	bool removableUnplug(const TQString &devNode);
	bool removableCamera(const TQString &devNode);

k_dcop_signals:
	void mediumAdded(const TQString &name, bool allowNotification);
	void mediumRemoved(const TQString &name, bool allowNotification);
	void mediumChanged(const TQString &name, bool allowNotification);

	// For compatibility purpose, not needed for KDE4
	void mediumAdded(const TQString &name);
	void mediumRemoved(const TQString &name);
	void mediumChanged(const TQString &name);

private slots:
	void loadBackends();
	
	void slotMediumAdded(const TQString &id, const TQString &name,
	                     bool allowNotification);
	void slotMediumRemoved(const TQString &id, const TQString &name,
	                       bool allowNotification);
	void slotMediumChanged(const TQString &id, const TQString &name,
	                       bool mounted, bool allowNotification);

private:
	MediaList m_mediaList;
	TQValueList<BackendBase*> m_backends;
	RemovableBackend *mp_removableBackend;
	HALBackend *m_halbackend;
	TDEBackend *m_tdebackend;
	MediaDirNotify m_dirNotify;
	FstabBackend *m_fstabbackend;
};

#endif
