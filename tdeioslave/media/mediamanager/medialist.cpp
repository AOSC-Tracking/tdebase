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

#include "medialist.h"

#include <kdebug.h>

MediaList::MediaList()
{
	kdDebug(1219) << "MediaList::MediaList()" << endl;

	m_media.setAutoDelete(true);
}

const TQPtrList<Medium> MediaList::list() const
{
	kdDebug(1219) << "MediaList::list()" << endl;

	return m_media;
}

const Medium *MediaList::findById(const TQString &id) const
{
	kdDebug(1219) << "MediaList::findById(" << id << ")" << endl;

	if ( !m_idMap.contains(id) ) return 0L;

	return m_idMap[id];
}

const Medium *MediaList::findByName(const TQString &name) const
{
	kdDebug(1219) << "MediaList::findByName(" << name << ")" << endl;

	if ( !m_nameMap.contains(name) ) return 0L;

	return m_nameMap[name];
}

const Medium *MediaList::findByNode(const TQString &node) const
{
	kdDebug(1219) << "MediaList::findByNode(" << node << ")" << endl;

	if ( !m_nodeMap.contains(node) ) return 0L;

	return m_nodeMap[node];
}

const Medium *MediaList::findByClearUdi(const TQString &name)
{
	kdDebug(1219) << "MediaList::findByClearUdi(" << name << ")" << endl;

	Medium *medium;
	for (medium = m_media.first(); medium; medium = m_media.next()) {
		if (medium->clearDeviceUdi() == name) return medium;
	}

	return 0L;
}

TQString MediaList::addMedium(Medium *medium, bool allowNotification)
{
	kdDebug(1219) << "MediaList::addMedium(@" << medium->id() << ")" << endl;

	TQString name = medium->name();
	if (!m_nameMap.contains(name))
	{
		m_nameMap[name] = medium;
	}

	TQString node = medium->deviceNode();
	if (!m_nodeMap.contains(node))
	{
		m_nodeMap[node] = medium;
	}

	TQString id = medium->id();
	if (m_idMap.contains(id))
	{
		return TQString::null;
	}

	m_media.append(medium);
	m_idMap[id] = medium;

	kdDebug(1219) << "MediaList emits mediumAdded(" << id << ", " << name << ")" << endl;
	emit mediumAdded(id, name, allowNotification);
	return name;
}

bool MediaList::removeMedium(const TQString &id, bool allowNotification)
{
	kdDebug(1219) << "MediaList::removeMedium(" << id << ")" << endl;

	if (!m_idMap.contains(id))
	{
		return false;
	}

	Medium *medium = m_idMap[id];
	m_idMap.remove(id);
	m_nameMap.remove(medium->name());
	m_nodeMap.remove(medium->deviceNode());

	TQString name = medium->name();
	m_media.remove(medium);

	emit mediumRemoved(id, name, allowNotification);
	return true;
}

bool MediaList::changeMediumState(const Medium &medium, bool allowNotification)
{
	kdDebug(1219) << "MediaList::changeMediumState(const Medium &) for id " << medium.id() << endl;

	if ( !m_idMap.contains(medium.id()) ) return false;

	Medium *m = m_idMap[medium.id()];

	if ( medium.isMountable() )
	{
		TQString device_node = medium.deviceNode();
		TQString clear_device_udi = medium.clearDeviceUdi();
		TQString mount_point = medium.mountPoint();
		TQString fs_type = medium.fsType();
		bool mounted = medium.isMounted();

		m->mountableState( device_node, clear_device_udi, mount_point, fs_type, mounted );
	}
	else
	{
		m->unmountableState( medium.baseURL() );
	}


	if (!medium.mimeType().isEmpty())
	{
		m->setMimeType( medium.mimeType() );
	}

	if (!medium.iconName().isEmpty())
	{
		m->setIconName( medium.iconName() );
	}

	if (!medium.label().isEmpty())
	{
		m->setLabel( medium.label() );
	}

	m->setHidden(medium.hidden());

	emit mediumStateChanged(m->id(), m->name(), !m->needMounting(), allowNotification);
	return true;
}

bool MediaList::changeMediumState(const TQString &id,
                                  const TQString &baseURL,
                                  bool allowNotification,
                                  const TQString &mimeType,
                                  const TQString &iconName,
                                  const TQString &label)
{
	kdDebug(1219) << "MediaList::changeMediumState(" << id << ", "
	          << baseURL << ", " << mimeType << ", " << iconName << ")"
	          << endl;

	if ( !m_idMap.contains(id) ) return false;

	Medium *medium = m_idMap[id];

	medium->unmountableState( baseURL );

	if (!mimeType.isEmpty())
	{
		medium->setMimeType( mimeType );
	}

	if (!iconName.isEmpty())
	{
		medium->setIconName( iconName );
	}

	if (!label.isEmpty())
	{
		medium->setLabel( label );
	}

	emit mediumStateChanged(id, medium->name(),
	                        !medium->needMounting(),
	                        allowNotification);
	return true;
}

bool MediaList::changeMediumState(const TQString &id,
                                  const TQString &deviceNode,
                                  const TQString &mountPoint,
                                  const TQString &fsType, bool mounted,
                                  bool allowNotification,
                                  const TQString &mimeType,
                                  const TQString &iconName,
                                  const TQString &label)
{
	kdDebug(1219) << "MediaList::changeMediumState(" << id << ", "
	          << deviceNode << ", " << mountPoint << ", " << fsType << ", "
	          << mounted << ", " << mimeType << ", " << iconName << ")"
	          << endl;

	if ( !m_idMap.contains(id) ) return false;

	Medium *medium = m_idMap[id];

	medium->mountableState( deviceNode, mountPoint, fsType, mounted );

	if (!mimeType.isEmpty())
	{
		medium->setMimeType( mimeType );
	}

	if (!iconName.isEmpty())
	{
		medium->setIconName( iconName );
	}

	if (!label.isEmpty())
	{
		medium->setLabel( label );
	}

	emit mediumStateChanged(id, medium->name(),
	                        !medium->needMounting(),
	                        allowNotification);
	return true;
}

bool MediaList::changeMediumState(const TQString &id, bool mounted,
                                  bool allowNotification,
                                  const TQString &mimeType,
                                  const TQString &iconName,
                                  const TQString &label)
{
	kdDebug(1219) << "MediaList::changeMediumState(" << id << ", "
	          << mounted << ", " << mimeType << ", " << iconName << ")"
	          << endl;

	if ( !m_idMap.contains(id) ) return false;

	Medium *medium = m_idMap[id];

	if ( !medium->mountableState( mounted ) ) return false;

	if (!mimeType.isEmpty())
	{
		medium->setMimeType( mimeType );
	}

	if (!iconName.isEmpty())
	{
		medium->setIconName( iconName );
	}

	if (!label.isEmpty())
	{
		medium->setLabel( label );
	}

	emit mediumStateChanged(id, medium->name(),
	                        !medium->needMounting(),
	                        allowNotification);
	return true;
}

bool MediaList::setUserLabel(const TQString &name, const TQString &label)
{
	kdDebug(1219) << "MediaList::setUserLabel(" << name << ", "
	          << label << ")" << endl;

	if ( !m_nameMap.contains(name) ) return false;

	Medium *medium = m_nameMap[name];
	medium->setUserLabel(label);

	emit mediumStateChanged(medium->id(), name,
	                        !medium->needMounting(),
	                        false);
	return true;
}

#include "medialist.moc"
