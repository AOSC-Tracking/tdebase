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

#include "medium.h"

#include <tdeconfig.h>
#include <tdelocale.h>

const TQString Medium::SEPARATOR = "---";

void Medium::initMedium()
{
	m_properties.clear();
	m_properties += TQString::null; // ID
	m_properties += TQString::null; // UUID
	m_properties += TQString::null; // NAME
	m_properties += TQString::null; // LABEL
	m_properties += TQString::null; // USER_LABEL
	m_properties += "false";        // MOUNTABLE
	m_properties += TQString::null; // DEVICE_NODE
	m_properties += TQString::null; // MOUNT_POINT
	m_properties += TQString::null; // FS_TYPE
	m_properties += "false";        // MOUNTED
	m_properties += TQString::null; // BASE_URL
	m_properties += TQString::null; // MIME_TYPE
	m_properties += TQString::null; // ICON_NAME
	m_properties += "false";        // ENCRYPTED
	m_properties += TQString::null; // CLEAR_DEVICE_UDI
	m_properties += "false";        // HIDDEN
	m_properties += "false";        // SOFT_HIDDEN
	m_properties += "false";        // LOCKED
}

Medium::Medium(const TQString id, TQString uuid, const TQString name)
{
	initMedium();
	if (!id.isEmpty() && !uuid.isEmpty())
	{
		m_properties[ID]    = id;
		m_properties[UUID]  = uuid;
		m_properties[NAME]  = name;
		m_properties[LABEL] = name;
		loadUserLabel();
	}
}

Medium::Medium()
{
	initMedium();
}

const Medium Medium::create(const TQStringList &properties)
{
	Medium m;

	if (properties.size() >= PROPERTIES_COUNT)
	{
		m.m_properties[ID] = properties[ID];
		m.m_properties[UUID] = properties[UUID];
		m.m_properties[NAME] = properties[NAME];
		m.m_properties[LABEL] = properties[LABEL];
		m.m_properties[USER_LABEL] = properties[USER_LABEL];
		m.m_properties[MOUNTABLE] = properties[MOUNTABLE];
		m.m_properties[DEVICE_NODE] = properties[DEVICE_NODE];
		m.m_properties[MOUNT_POINT] = properties[MOUNT_POINT];
		m.m_properties[FS_TYPE] = properties[FS_TYPE];
		m.m_properties[MOUNTED] = properties[MOUNTED];
		m.m_properties[BASE_URL] = properties[BASE_URL];
		m.m_properties[MIME_TYPE] = properties[MIME_TYPE];
		m.m_properties[ICON_NAME] = properties[ICON_NAME];
		m.m_properties[ENCRYPTED] = properties[ENCRYPTED];
		m.m_properties[CLEAR_DEVICE_UDI] = properties[CLEAR_DEVICE_UDI];
		m.m_properties[HIDDEN] = properties[HIDDEN];
		m.m_properties[SOFT_HIDDEN] = properties[SOFT_HIDDEN];
		m.m_properties[LOCKED] = properties[LOCKED];
	}

	return m;
}

Medium::MList Medium::createList(const TQStringList &properties)
{
	MList l;

	if (properties.size() % (PROPERTIES_COUNT+1) == 0)
	{
		int media_count = properties.size() / (PROPERTIES_COUNT + 1);
		TQStringList props = properties;

		for (int i=0; i < media_count; i++)
		{
			const Medium m = create(props);
			l.append(m);

			TQStringList::iterator first = props.begin();
			TQStringList::iterator last = props.find(SEPARATOR);
			++last;
			props.erase(first, last);
		}
	}

	return l;
}

void Medium::setName(const TQString &name)
{
	m_properties[NAME] = name;
}

void Medium::setLabel(const TQString &label)
{
	m_properties[LABEL] = label;
}

void Medium::setUserLabel(const TQString &label)
{
	TDEConfig cfg("mediamanagerrc");
	cfg.setGroup("UserLabels");

	TQString entry_name = m_properties[UUID];
	if (!entry_name.isEmpty())
	{
		if (label.isEmpty())
		{
			cfg.deleteEntry(entry_name);
		}
		else
		{
			cfg.writeEntry(entry_name, label);
		}
	}

	m_properties[USER_LABEL] = label;
}

void Medium::loadUserLabel()
{
	TDEConfig cfg("mediamanagerrc");
	cfg.setGroup("UserLabels");

	TQString entry_name = m_properties[UUID];
	if (!entry_name.isEmpty())
	{
		m_properties[USER_LABEL] = cfg.readEntry(entry_name, TQString::null);
	}
	else
	{
		m_properties[USER_LABEL] = TQString::null;
	}
}

void Medium::setMountable(bool mountable)
{
	m_properties[MOUNTABLE] = mountable ? "true" : "false";
	if (!mountable)
	{
		setMountPoint(TQString::null);
		setMounted(false);
	}
}

void Medium::setDeviceNode(const TQString &deviceNode)
{
	m_properties[DEVICE_NODE] = deviceNode;
}

void Medium::setMountPoint(const TQString &mountPoint)
{
	if (isMountable())
	{
		m_properties[MOUNT_POINT] = mountPoint;
	}
	else
	{
		m_properties[MOUNT_POINT] = TQString::null;
	}
}

void Medium::setFsType(const TQString &fsType)
{
	m_properties[FS_TYPE] = fsType;
}

void Medium::setMounted(bool mounted)
{
	if (isMountable())
	{
		m_properties[MOUNTED] = mounted ? "true" : "false";
	}
	else
	{
		m_properties[MOUNTED] = "false";
	}
}

void Medium::setBaseURL(const TQString &baseURL)
{
	m_properties[BASE_URL] = baseURL;
}

void Medium::setMimeType(const TQString &mimeType)
{
	m_properties[MIME_TYPE] = mimeType;
}

void Medium::setIconName(const TQString &iconName)
{
	m_properties[ICON_NAME] = iconName;
}

void Medium::setEncrypted(bool encrypted)
{
	m_properties[ENCRYPTED] = encrypted ? "true" : "false";
	if (!encrypted)
	{
		setLocked(false);
	}
}

void Medium::setClearDeviceUdi(const TQString &clearDeviceUdi)
{
	m_properties[CLEAR_DEVICE_UDI] = clearDeviceUdi;
}

void Medium::setHidden(bool state)
{
	m_properties[HIDDEN] = state ? "true" : "false";
}

void Medium::setSoftHidden(bool state)
{
	m_properties[SOFT_HIDDEN] = state ? "true" : "false";
}

void Medium::setLocked(bool locked)
{
	m_properties[LOCKED] = locked ? "true" : "false";
}

KURL Medium::prettyBaseURL() const
{
	if (!baseURL().isEmpty())
	{
		return baseURL();
	}

	return KURL(mountPoint());
}

TQString Medium::prettyLabel() const
{
	if (!userLabel().isEmpty())
	{
		return userLabel();
	}

	return label();
}
