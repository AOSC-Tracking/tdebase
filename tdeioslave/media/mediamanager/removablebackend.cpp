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

#include "removablebackend.h"

#include <tdelocale.h>
#include <kdirwatch.h>
#include <kurl.h>
#include <kmountpoint.h>
#include <tdestandarddirs.h>

#ifdef _OS_SOLARIS_
#define MTAB "/etc/mnttab"
#else
#define MTAB "/etc/mtab"
#endif



RemovableBackend::RemovableBackend(MediaList &list)
	: TQObject(), BackendBase(list)
{
	KDirWatch::self()->addFile(MTAB);

	connect( KDirWatch::self(), TQ_SIGNAL( dirty(const TQString&) ),
	         this, TQ_SLOT( slotDirty(const TQString&) ) );
	KDirWatch::self()->startScan();
}

RemovableBackend::~RemovableBackend()
{
	TQStringList::iterator it = m_removableIds.begin();
	TQStringList::iterator end = m_removableIds.end();

	for (; it!=end; ++it)
	{
		m_mediaList.removeMedium(*it, false);
	}

        KDirWatch::self()->removeFile(MTAB);
}

bool RemovableBackend::plug(const TQString &devNode, const TQString &label)
{
	TQString name = generateName(devNode);
	TQString id = generateId(devNode);

	if (!m_removableIds.contains(id))
	{
		Medium *medium = new Medium(id, id, name);
		medium->setMountable(true);
		medium->setDeviceNode(devNode);
		medium->setMountPoint(TQString::null);
		medium->setFsType(TQString::null);
		medium->setMounted(false);

		TQStringList words = TQStringList::split(" ", label);
		
		TQStringList::iterator it = words.begin();
		TQStringList::iterator end = words.end();

		TQString tmp = (*it).lower();
		tmp[0] = tmp[0].upper();
		TQString new_label = tmp;
		
		++it;
		for (; it!=end; ++it)
		{
			tmp = (*it).lower();
			tmp[0] = tmp[0].upper();
			new_label+= " "+tmp;
		}
		
		medium->setLabel(new_label);
		medium->setMimeType("media/removable_unmounted");

		m_removableIds.append(id);
		return !m_mediaList.addMedium(medium).isNull();
	}
	return false;
}

bool RemovableBackend::unplug(const TQString &devNode)
{
	TQString id = generateId(devNode);
	if (m_removableIds.contains(id))
	{
		m_removableIds.remove(id);
		return m_mediaList.removeMedium(id);
	}
	return false;
}

bool RemovableBackend::camera(const TQString &devNode)
{
	TQString id = generateId(devNode);
	if (m_removableIds.contains(id))
	{
		return m_mediaList.changeMediumState(id,
			TQString("camera:/"), false, "media/gphoto2camera");
	}
	return false;
}

void RemovableBackend::slotDirty(const TQString &path)
{
	if (path==MTAB)
	{
		handleMtabChange();
	}
}


void RemovableBackend::handleMtabChange()
{
	TQStringList new_mtabIds;
	KMountPoint::List mtab = KMountPoint::currentMountPoints();

	KMountPoint::List::iterator it = mtab.begin();
	KMountPoint::List::iterator end = mtab.end();

	for (; it!=end; ++it)
	{
		TQString dev = (*it)->mountedFrom();
		TQString mp = (*it)->mountPoint();
		TQString fs = (*it)->mountType();

		TQString id = generateId(dev);
		new_mtabIds+=id;

		if ( !m_mtabIds.contains(id)
		  && m_removableIds.contains(id) )
		{
			m_mediaList.changeMediumState(id, dev, mp, fs, true,
				false, "media/removable_mounted");
		}
	}

	TQStringList::iterator it2 = m_mtabIds.begin();
	TQStringList::iterator end2 = m_mtabIds.end();

	for (; it2!=end2; ++it2)
	{
		if ( !new_mtabIds.contains(*it2)
		  && m_removableIds.contains(*it2) )
		{
			m_mediaList.changeMediumState(*it2, false,
				false, "media/removable_unmounted");
		}
	}

	m_mtabIds = new_mtabIds;
}

TQString RemovableBackend::generateId(const TQString &devNode)
{
	TQString dev = TDEStandardDirs::realFilePath(devNode);

	return "/org/kde/mediamanager/removable/"
	      +dev.replace("/", "");
}

TQString RemovableBackend::generateName(const TQString &devNode)
{
	return KURL(devNode).fileName();
}

#include "removablebackend.moc"
