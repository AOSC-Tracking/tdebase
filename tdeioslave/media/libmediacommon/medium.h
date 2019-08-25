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

#ifndef _MEDIUM_H_
#define _MEDIUM_H_

#include <tqstring.h>
#include <tqstringlist.h>
#include <kurl.h>
#include <tqmap.h>

class Medium
{
public:
	typedef TQValueList<Medium> MList;

	static const uint ID               =  0;
	static const uint UUID             =  1;
	static const uint NAME             =  2;
	static const uint LABEL            =  3;
	static const uint USER_LABEL       =  4;
	static const uint MOUNTABLE        =  5;
	static const uint DEVICE_NODE      =  6;
	static const uint MOUNT_POINT      =  7;
	static const uint FS_TYPE          =  8;
	static const uint MOUNTED          =  9;
	static const uint BASE_URL         = 10;
	static const uint MIME_TYPE        = 11;
	static const uint ICON_NAME        = 12;
	static const uint ENCRYPTED        = 13;
	static const uint CLEAR_DEVICE_UDI = 14;
	static const uint HIDDEN           = 15;
	static const uint SOFT_HIDDEN      = 16;
	static const uint LOCKED           = 17;
	static const uint PROPERTIES_COUNT = 18;
	static const TQString SEPARATOR;

	Medium(const TQString id, TQString uuid, const TQString name);
	static const Medium create(const TQStringList &properties);
	static MList createList(const TQStringList &properties);

	const TQStringList &properties() const { return m_properties; }

	TQString id() const { return m_properties[ID]; }
	TQString uuid() const { return m_properties[UUID]; }
	TQString name() const { return m_properties[NAME]; }
	TQString label() const { return m_properties[LABEL]; }
	TQString userLabel() const { return m_properties[USER_LABEL]; }
	bool     isMountable() const { return m_properties[MOUNTABLE] == "true"; }
	TQString deviceNode() const { return m_properties[DEVICE_NODE]; }
	TQString mountPoint() const { return m_properties[MOUNT_POINT]; }
	TQString fsType() const { return m_properties[FS_TYPE]; }
	bool     isMounted() const { return m_properties[MOUNTED] == "true"; }
	TQString baseURL() const { return m_properties[BASE_URL]; }
	TQString mimeType() const { return m_properties[MIME_TYPE]; }
	TQString iconName() const { return m_properties[ICON_NAME]; }
	bool     isEncrypted() const { return m_properties[ENCRYPTED] == "true"; };
	TQString clearDeviceUdi() const { return m_properties[CLEAR_DEVICE_UDI]; };
	bool     hidden() const { return m_properties[HIDDEN] == "true"; };
	bool     softHidden() const { return m_properties[SOFT_HIDDEN] == "true"; };
	bool     isLocked() const { return m_properties[LOCKED] == "true"; };

	bool needMounting() const { return isMountable() && !isMounted(); };
	bool needUnlocking() const { return isEncrypted() && isLocked(); }
	KURL prettyBaseURL() const;
	TQString prettyLabel() const;

	void setName(const TQString &name);
	void setLabel(const TQString &label);
	void setUserLabel(const TQString &label);
	void setMountable(bool mountable);
	void setDeviceNode(const TQString &deviceNode);
	void setMountPoint(const TQString &mountPoint);
	void setFsType(const TQString &fsType);
	void setMounted(bool mounted);
	void setBaseURL(const TQString &baseURL);
	void setMimeType(const TQString &mimeType);
	void setIconName(const TQString &iconName);
	void setEncrypted(bool encrypted);
	void setClearDeviceUdi(const TQString &clearDeviceUdi);
	void setHidden(bool state);
	void setSoftHidden(bool state);
	void setLocked(bool locked);

	Medium();

private:
	void initMedium();
	void loadUserLabel();

	TQStringList m_properties;
	
	friend class TQValueListNode<const Medium>;
};

namespace MediaManagerUtils {
	static inline TQMap<TQString,TQString> splitOptions(const TQStringList &options)
	{
		TQMap<TQString,TQString> valids;

		for (TQStringList::ConstIterator it = options.begin(); it != options.end(); ++it)
		{
			int pos = (*it).find('=');
			TQString key = (*it).left(pos);
			TQString value = (*it).mid(pos + 1);
			valids[key] = value;
		}
		return valids;
	}
}

#endif
