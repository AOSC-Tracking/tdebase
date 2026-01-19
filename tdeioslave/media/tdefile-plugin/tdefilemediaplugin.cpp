/* This file is part of the KDE project
   Copyright (C) 2004 Kevin Ottens <ervin ipsquad net>

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

#include "config.h"
#include "tdefilemediaplugin.h"

#include <kgenericfactory.h>

#include <dcopref.h>

#include <tqpixmap.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <tqapplication.h>
#include <tqfile.h>

#ifdef HAVE_STATVFS
# include <sys/statvfs.h>
#else
# ifdef __OpenBSD__
#  include <sys/param.h>
# endif
# include <sys/mount.h>
# define statvfs statfs
# define f_frsize f_bsize
#endif

typedef KGenericFactory<KFileMediaPlugin> KFileMediaPluginFactory;
K_EXPORT_COMPONENT_FACTORY(tdefile_media, KFileMediaPluginFactory("tdeio_media"))

KFileMediaPlugin::KFileMediaPlugin(TQObject *parent, const char *name,
		                     const TQStringList& args)
	: KFilePlugin(parent, name, args)
{
	addMimeType( "media/audiocd" );
	addMimeType( "media/blankbluray" );
	addMimeType( "media/blankcd" );
	addMimeType( "media/blankdvd" );
	addMimeType( "media/bluray_mounted" );
	addMimeType( "media/bluray_unmounted" );
	addMimeType( "media/bluray_encrypted_unlocked" );
	addMimeType( "media/bluray_encrypted_locked" );
	addMimeType( "media/cdrom_mounted" );
	addMimeType( "media/cdrom_unmounted" );
	addMimeType( "media/cdrom_encrypted_unlocked" );
	addMimeType( "media/cdrom_encrypted_locked" );
	addMimeType( "media/cd-r_mounted" );
	addMimeType( "media/cd-r_unmounted" );
	addMimeType( "media/cd-r_encrypted_unlocked" );
	addMimeType( "media/cd-r_encrypted_locked" );
	addMimeType( "media/cd-rw_mounted" );
	addMimeType( "media/cd-rw_unmounted" );
	addMimeType( "media/cd-rw_encrypted_unlocked" );
	addMimeType( "media/cd-rw_encrypted_locked" );
	addMimeType( "media/dvd_mounted" );
	addMimeType( "media/dvd_unmounted" );
	addMimeType( "media/dvd_encrypted_unlocked" );
	addMimeType( "media/dvd_encrypted_locked" );
	addMimeType( "media/hdd_mounted" );
	addMimeType( "media/hdd_unmounted" );
	addMimeType( "media/hdd_encrypted_unlocked" );
	addMimeType( "media/hdd_encrypted_locked" );
	addMimeType( "media/removable_mounted" );
	addMimeType( "media/removable_unmounted" );
	addMimeType( "media/removable_encrypted_unlocked" );
	addMimeType( "media/removable_encrypted_locked" );
	addMimeType( "media/nfs_mounted" );
	addMimeType( "media/nfs_unmounted" );
	addMimeType( "media/smb_mounted" );
	addMimeType( "media/smb_unmounted" );
	addMimeType( "media/floppy_mounted" );
	addMimeType( "media/floppy_unmounted" );
	addMimeType( "media/floppy5_mounted" );
	addMimeType( "media/floppy5_unmounted" );
	addMimeType( "media/zip_mounted" );
	addMimeType( "media/zip_unmounted" );
	addMimeType( "media/blurayvideo" );
	addMimeType( "media/dvdvideo" );
	addMimeType( "media/svcd" );
	addMimeType( "media/vcd" );
	addMimeType( "media/camera_mounted" );
	addMimeType( "media/camera_unmounted" );
	addMimeType( "media/gphoto2camera" );
}

bool KFileMediaPlugin::readInfo(KFileMetaInfo &info, uint /*what*/)
{
	const Medium medium = askMedium(info);

	kdDebug() << "KFileMediaPlugin::readInfo " << medium.id() << endl;
 
	if (medium.id().isNull()) return false;
	
	TQString mount_point = medium.mountPoint();
	KURL base_url = medium.prettyBaseURL();
	TQString device_node = medium.deviceNode();
	TQString fs_type = medium.fsType();

	KFileMetaInfoGroup group = appendGroup(info, "mediumInfo");

	if (base_url.isValid())
	{
		appendItem(group, "baseURL", base_url.prettyURL());
	}

	if (!device_node.isEmpty())
	{
		appendItem(group, "deviceNode", device_node);
	}

	if (!fs_type.isEmpty())
	{
		appendItem(group, "fsType", fs_type);
	}

	if (!mount_point.isEmpty() && medium.isMounted())
	{
		m_total = 0;
		m_used = 0;
		m_free = 0;

		struct statvfs vfs;
		memset(&vfs, 0, sizeof(vfs));

		if ( ::statvfs(TQFile::encodeName(mount_point), &vfs) != -1 )
		{
			m_total = static_cast<TDEIO::filesize_t>(vfs.f_blocks) * static_cast<TDEIO::filesize_t>(vfs.f_frsize);
			m_free = static_cast<TDEIO::filesize_t>(vfs.f_bavail) * static_cast<TDEIO::filesize_t>(vfs.f_frsize);
			m_used = m_total - m_free;

			int percent = 0;
			int length = 0;

			if (m_total != 0)
			{
				percent = 100 * m_used / m_total;
				length = 150 * m_used / m_total;
			}

			appendItem(group, "space_free", m_free);
			appendItem(group, "space_used", m_used);
			appendItem(group, "space_total", m_total);

			group = appendGroup(info, "mediumSummary");

			appendItem(group, "percent", TQString("%1%").arg(percent));

			TQPixmap bar(150, 20);
			TQPainter p(&bar);

			p.fillRect(0, 0, length, 20, TQt::red);
			p.fillRect(length, 0, 150-length, 20, TQt::green);

			TQColorGroup cg = TQApplication::palette().active();

			TQApplication::style().drawPrimitive(TQStyle::PE_Panel, &p,
							    TQRect(0, 0, 150, 20), cg,
							    TQStyle::Style_Sunken);

			appendItem( group, "thumbnail", bar );
		}
	}

	return true;
}

const Medium KFileMediaPlugin::askMedium(KFileMetaInfo &info)
{
	DCOPRef mediamanager("kded", "mediamanager");
	kdDebug() << "properties " << info.url() << endl;
	DCOPReply reply = mediamanager.call( "properties", info.url().url() );

	if ( !reply.isValid() )
	{
		return Medium(TQString::null, TQString::null, TQString::null);
	}

	return Medium::create(reply);
}

void KFileMediaPlugin::addMimeType(const char *mimeType)
{
	KFileMimeTypeInfo *info = addMimeTypeInfo( mimeType );

	KFileMimeTypeInfo::GroupInfo *group
		= addGroupInfo(info, "mediumInfo", i18n("Medium Information"));

	KFileMimeTypeInfo::ItemInfo *item;
	item = addItemInfo(group, "baseURL", i18n("Base URL"), TQVariant::String);
	item = addItemInfo(group, "deviceNode", i18n("Device Node"), TQVariant::String);
	item = addItemInfo(group, "fsType", i18n("File System"), TQVariant::String);
	item = addItemInfo(group, "mountPoint", i18n("Mount Point"), TQVariant::String);

	item = addItemInfo(group, "space_free", i18n("Free"), TQVariant::ULongLong);
	setUnit(item, KFileMimeTypeInfo::Bytes);

	item = addItemInfo(group, "space_used", i18n("Used"), TQVariant::ULongLong);
	setUnit(item, KFileMimeTypeInfo::Bytes);

	item = addItemInfo(group, "space_total", i18n("Total"), TQVariant::ULongLong);
	setUnit(item, KFileMimeTypeInfo::Bytes);

	group = addGroupInfo(info, "mediumSummary", i18n("Medium Summary"));
	item = addItemInfo(group, "percent", i18n("Usage"), TQVariant::String);
	item = addItemInfo( group, "thumbnail", i18n("Bar Graph"), TQVariant::Image );
	setHint( item, KFileMimeTypeInfo::Thumbnail );
}

#include "tdefilemediaplugin.moc"
