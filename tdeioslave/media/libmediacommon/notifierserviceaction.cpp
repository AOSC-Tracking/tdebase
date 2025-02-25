/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

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

#include "notifierserviceaction.h"

#include <tqdir.h>
#include <tqfile.h>
#include <tqfileinfo.h>
#include <tdestandarddirs.h>
#include <kdesktopfile.h>
#include <tdelocale.h>

NotifierServiceAction::NotifierServiceAction()
	: NotifierAction()
{
	NotifierAction::setIconName("button_cancel");
	NotifierAction::setLabel(i18n("Unknown"));

	m_service.m_strName = "New Service";
	m_service.m_strIcon = "button_cancel";
	m_service.m_strExec = "konqueror %u";
}

TQString NotifierServiceAction::id() const
{
	if (m_filePath.isEmpty() || m_service.m_strName.isEmpty())
	{
		return TQString();
	}
	else
	{
		return "#Service:"+m_filePath;
	}
}

void NotifierServiceAction::setIconName( const TQString &icon )
{
	m_service.m_strIcon = icon;
	NotifierAction::setIconName( icon );
}

void NotifierServiceAction::setLabel( const TQString &label )
{
	m_service.m_strName = label;
	NotifierAction::setLabel( label );
	
	updateFilePath();
}

void NotifierServiceAction::execute(KFileItem &medium)
{
	KURL::List urls = KURL::List( medium.url() );
	KDEDesktopMimeType::executeService( urls, m_service );
}

void NotifierServiceAction::setService(KDEDesktopMimeType::Service service)
{
	NotifierAction::setIconName( service.m_strIcon );
	NotifierAction::setLabel( service.m_strName );

	m_service = service;
	
	updateFilePath();
}

KDEDesktopMimeType::Service NotifierServiceAction::service() const
{
	return m_service;
}

void NotifierServiceAction::setFilePath(const TQString &filePath)
{
	m_filePath = filePath;
}

TQString NotifierServiceAction::filePath() const
{
	return m_filePath;
}

void NotifierServiceAction::updateFilePath()
{
	if ( !m_filePath.isEmpty() ) return;
	
	TQString action_name = m_service.m_strName;
	action_name.replace(   " ", "_" );
	
	TQDir actions_dir( locateLocal( "data", "konqueror/servicemenus/", true ) );

	TQString filename = actions_dir.absFilePath( action_name + ".desktop" );
	
	int counter = 1;
	while ( TQFile::exists( filename ) )
	{
		filename = actions_dir.absFilePath( action_name
		                                  + TQString::number( counter )
		                                  + ".desktop" );
		counter++;
	}
	
	m_filePath = filename;
}

void NotifierServiceAction::setMimetypes(const TQStringList &mimetypes)
{
	m_mimetypes = mimetypes;
}

TQStringList NotifierServiceAction::mimetypes()
{
	return m_mimetypes;
}

bool NotifierServiceAction::isWritable() const
{
	TQFileInfo info( m_filePath );

	if ( info.exists() )
	{
		return info.isWritable();
	}
	else
	{
		info = TQFileInfo( info.dirPath() );
		return info.isWritable();
	}
}

bool NotifierServiceAction::supportsMimetype(const TQString &mimetype) const
{
	return m_mimetypes.contains(mimetype);
}

void NotifierServiceAction::save() const
{
	TQFile::remove( m_filePath );
	KDesktopFile desktopFile(m_filePath);

	desktopFile.setGroup(TQString("Desktop Action ") + m_service.m_strName);
	desktopFile.writeEntry(TQString("Icon"), m_service.m_strIcon);
	desktopFile.writeEntry(TQString("Name"), m_service.m_strName);
	desktopFile.writeEntry(TQString("Exec"), m_service.m_strExec);

	desktopFile.setDesktopGroup();

	desktopFile.writeEntry(TQString("X-TDE-ServiceTypes"), m_mimetypes, ",");
	desktopFile.writeEntry(TQString("Actions"),
	                       TQStringList(m_service.m_strName),";");
}

