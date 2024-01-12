/* This file is part of the KDE project
   Copyright (c) 2005 Kevin Ottens <ervin ipsquad net>

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

#include "homeimpl.h"

#include <kdebug.h>
#include <tqapplication.h>
#include <tqeventloop.h>

#include <sys/stat.h>

#define MINIMUM_UID 500

HomeImpl::HomeImpl()
{
	KUser user;
	m_effectiveUid = user.uid();
}

bool HomeImpl::parseURL(const KURL &url, TQString &name, TQString &path) const
{
	TQString url_path = url.path();

	int i = url_path.find('/', 1);
	if (i > 0)
	{
		name = url_path.mid(1, i-1);
		path = url_path.mid(i+1);
	}
	else
	{
		name = url_path.mid(1);
		path = TQString::null;
	}

	return name != TQString::null;
}

bool HomeImpl::realURL(const TQString &name, const TQString &path, KURL &url)
{
	KUser user(name);
	
	if ( user.isValid() )
	{
		KURL res;
		res.setPath( user.homeDir() );
		res.addPath(path);
		url = res;
		return true;
	}
	
	return false;
}


bool HomeImpl::listHomes(TQValueList<TDEIO::UDSEntry> &list)
{
	kdDebug() << "HomeImpl::listHomes" << endl;

	KUser current_user;
	TQValueList<KUserGroup> groups = current_user.groups();
	TQValueList<int> uid_list;
	
	TQValueList<KUserGroup>::iterator groups_it = groups.begin();
	TQValueList<KUserGroup>::iterator groups_end = groups.end();

	for(; groups_it!=groups_end; ++groups_it)
	{
		TQValueList<KUser> users = (*groups_it).users();

		TQValueList<KUser>::iterator it = users.begin();
		TQValueList<KUser>::iterator users_end = users.end();
		
		for(; it!=users_end; ++it)
		{
			if ((*it).uid()>=MINIMUM_UID
			 && !uid_list.contains( (*it).uid() ) )
			{
				uid_list.append( (*it).uid() );
				TDEIO::UDSEntry entry;
				createHomeEntry(entry, *it);
				list.append(entry);
			}
		}
	}
		
	return true;
}

static void addAtom(TDEIO::UDSEntry &entry, unsigned int ID, long l,
                    const TQString &s = TQString::null)
{
	TDEIO::UDSAtom atom;
	atom.m_uds = ID;
	atom.m_long = l;
	atom.m_str = s;
	entry.append(atom);
}


void HomeImpl::createTopLevelEntry(TDEIO::UDSEntry &entry) const
{
	entry.clear();
	addAtom(entry, TDEIO::UDS_NAME, 0, ".");
	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, TDEIO::UDS_ACCESS, 0555);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, "kfm_home");
	addAtom(entry, TDEIO::UDS_USER, 0, "root");
	addAtom(entry, TDEIO::UDS_GROUP, 0, "root");
}

void HomeImpl::createHomeEntry(TDEIO::UDSEntry &entry,
                               const KUser &user)
{
	kdDebug() << "HomeImpl::createHomeEntry" << endl;
	
	entry.clear();
	
	TQString full_name = user.loginName();
	
	if (!user.fullName().isEmpty())
	{
		full_name = user.fullName()+" ("+user.loginName()+")";
	}
	
	full_name = TDEIO::encodeFileName( full_name );
	
	addAtom(entry, TDEIO::UDS_NAME, 0, full_name);
	addAtom(entry, TDEIO::UDS_URL, 0, "home:/"+user.loginName());
	
	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "inode/directory");

	TQString icon_name = "folder_home2";

	if (user.uid()==m_effectiveUid)
	{
		icon_name = "folder_home";
	}
	
	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, icon_name);

	KURL url;
	url.setPath(user.homeDir());
	entry += extractUrlInfos(url);
}

bool HomeImpl::statHome(const TQString &name, TDEIO::UDSEntry &entry)
{
	kdDebug() << "HomeImpl::statHome: " << name << endl;

	KUser user(name);

	if (user.isValid())
	{
		createHomeEntry(entry, user);
		return true;	
	}
	
	return false;
}

void HomeImpl::slotStatResult(TDEIO::Job *job)
{
	if ( job->error() == 0)
	{
		TDEIO::StatJob *stat_job = static_cast<TDEIO::StatJob *>(job);
		m_entryBuffer = stat_job->statResult();
	}

	tqApp->eventLoop()->exitLoop();
}

TDEIO::UDSEntry HomeImpl::extractUrlInfos(const KURL &url)
{
	m_entryBuffer.clear();

	TDEIO::StatJob *job = TDEIO::stat(url, false);
	connect( job, TQ_SIGNAL( result(TDEIO::Job *) ),
	         this, TQ_SLOT( slotStatResult(TDEIO::Job *) ) );
	tqApp->eventLoop()->enterLoop();

	TDEIO::UDSEntry::iterator it = m_entryBuffer.begin();
	TDEIO::UDSEntry::iterator end = m_entryBuffer.end();

	TDEIO::UDSEntry infos;

	for(; it!=end; ++it)
	{
		switch( (*it).m_uds )
		{
		case TDEIO::UDS_ACCESS:
		case TDEIO::UDS_USER:
		case TDEIO::UDS_GROUP:
		case TDEIO::UDS_CREATION_TIME:
		case TDEIO::UDS_MODIFICATION_TIME:
		case TDEIO::UDS_ACCESS_TIME:
			infos.append(*it);
			break;
		default:
			break;
		}
	}
	
	addAtom(infos, TDEIO::UDS_LOCAL_PATH, 0, url.path());

	return infos;
}

#include "homeimpl.moc"

