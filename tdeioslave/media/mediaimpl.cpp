/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

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

#include "mediaimpl.h"

#include <tdelocale.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <tdeio/netaccess.h>

#include <kmimetype.h>

#include <tdeapplication.h>
#include <tqeventloop.h>

#include <sys/stat.h>

#include "medium.h"

#include <config.h>

MediaImpl::MediaImpl() : TQObject(), DCOPObject("mediaimpl"), mp_mounting(0L)
{

}

bool MediaImpl::parseURL(const KURL &url, TQString &name, TQString &path) const
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

bool MediaImpl::realURL(const TQString &name, const TQString &path, KURL &url)
{
	bool ok;
	Medium m = findMediumByName(name, ok);
	if ( !ok ) return false;

	ok = ensureMediumMounted(m);
	if ( !ok ) return false;

	url = m.prettyBaseURL();
	url.addPath(path);
	return true;
}


bool MediaImpl::statMedium(const TQString &name, TDEIO::UDSEntry &entry)
{
	kdDebug(1219) << "MediaImpl::statMedium: " << name << endl;

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "properties", name );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = TDEIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The TDE mediamanager is not running.");
		return false;
	}

	Medium m = Medium::create(reply);

	if (m.id().isEmpty())
	{
		entry.clear();
		return false;
	}

	createMediumEntry(entry, m);

	return true;
}

bool MediaImpl::statMediumByLabel(const TQString &label, TDEIO::UDSEntry &entry)
{
	kdDebug(1219) << "MediaImpl::statMediumByLabel: " << label << endl;

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "nameForLabel", label );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = TDEIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The TDE mediamanager is not running.");
		return false;
	}

	TQString name = reply;

	if (name.isEmpty())
	{
		entry.clear();
		return false;
	}

	return statMedium(name, entry);
}

bool MediaImpl::listMedia(TQValueList<TDEIO::UDSEntry> &list)
{
	kdDebug(1219) << "MediaImpl::listMedia" << endl;

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "fullList" );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = TDEIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The TDE mediamanager is not running.");
		return false;
	}

	const Medium::MList media = Medium::createList(reply);

	TDEIO::UDSEntry entry;

	Medium::MList::const_iterator it = media.begin();
	Medium::MList::const_iterator end = media.end();

	for(; it != end; ++it)
	{
		if (!(*it).hidden()) {
			entry.clear();

			createMediumEntry(entry, *it);

			list.append(entry);
		}
	}

	return true;
}

bool MediaImpl::setUserLabel(const TQString &name, const TQString &label)
{
	kdDebug(1219) << "MediaImpl::setUserLabel: " << name << ", " << label << endl;


	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "nameForLabel", label );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = TDEIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The TDE mediamanager is not running.");
		return false;
	}
	else
	{
		TQString returned_name = reply;
		if (!returned_name.isEmpty()
		 && returned_name!=name)
		{
			m_lastErrorCode = TDEIO::ERR_DIR_ALREADY_EXIST;
			m_lastErrorMessage = i18n("This media name already exists.");
			return false;
		}
	}

	reply = mediamanager.call( "setUserLabel", name, label );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = TDEIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The TDE mediamanager is not running.");
		return false;
	}
	else
	{
		return true;
	}
}

const Medium MediaImpl::findMediumByName(const TQString &name, bool &ok)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "properties", name );

	if ( reply.isValid() )
	{
		ok = true;
	}
	else
	{
		m_lastErrorCode = TDEIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The TDE mediamanager is not running.");
		ok = false;
	}

	return Medium::create(reply);
}

bool MediaImpl::ensureMediumMounted(Medium &medium)
{
	if (medium.id().isEmpty())
	{
		m_lastErrorCode = TDEIO::ERR_COULD_NOT_MOUNT;
		m_lastErrorMessage = i18n("No such medium.");
		return false;
	}

	if ( medium.needMounting() )
	{
		m_lastErrorCode = 0;

		mp_mounting = &medium;
		
		
		/*
		TDEIO::Job* job = TDEIO::mount(false, 0,
		                           medium.deviceNode(),
		                           medium.mountPoint());
		job->setAutoWarningHandlingEnabled(false);
		connect( job, TQ_SIGNAL( result( TDEIO::Job * ) ),
		         this, TQ_SLOT( slotMountResult( TDEIO::Job * ) ) );
		connect( job, TQ_SIGNAL( warning( TDEIO::Job *, const TQString & ) ),
		         this, TQ_SLOT( slotWarning( TDEIO::Job *, const TQString & ) ) );
		*/
		tdeApp->dcopClient()
		->connectDCOPSignal("kded", "mediamanager",
		                    "mediumChanged(TQString, bool)",
		                    "mediaimpl",
		                    "slotMediumChanged(TQString)",
		                    false);

		DCOPRef mediamanager("kded", "mediamanager");
		DCOPReply reply = mediamanager.call( "mount", medium.id());
		TQStringVariantMap mountResult;
		if (reply.isValid()) {
			reply.get(mountResult);
		}
		if (!mountResult.contains("result") || !mountResult["result"].toBool()) {
			m_lastErrorMessage = mountResult.contains("errStr") ? mountResult["errStr"].toString() : i18n("Unknown mount error.");
		}
		if (!m_lastErrorMessage.isEmpty()) {
			m_lastErrorCode = TDEIO::ERR_SLAVE_DEFINED;
		}
		else {
		  tqApp->eventLoop()->enterLoop();
		}

		mp_mounting = 0L;
		
		tdeApp->dcopClient()
		->disconnectDCOPSignal("kded", "mediamanager",
		                       "mediumChanged(TQString, bool)",
		                       "mediaimpl",
		                       "slotMediumChanged(TQString)");
		
		return m_lastErrorCode==0;
	}

	if (medium.id().isEmpty())
	{
		m_lastErrorCode = TDEIO::ERR_COULD_NOT_MOUNT;
		m_lastErrorMessage = i18n("No such medium.");
		return false;
	}

	return true;
}

void MediaImpl::slotWarning( TDEIO::Job * /*job*/, const TQString &msg )
{
	emit warning( msg );
}

void MediaImpl::slotMountResult(TDEIO::Job *job)
{
	kdDebug(1219) << "MediaImpl::slotMountResult" << endl;
	
	if ( job->error() != 0)
	{
		m_lastErrorCode = job->error();
		m_lastErrorMessage = job->errorText();
		tqApp->eventLoop()->exitLoop();
	}
}

void MediaImpl::slotMediumChanged(const TQString &name)
{
	kdDebug(1219) << "MediaImpl::slotMediumChanged:" << name << endl;

	if (mp_mounting->name()==name)
	{
		kdDebug(1219) << "MediaImpl::slotMediumChanged: updating mp_mounting" << endl;
		bool ok;
		*mp_mounting = findMediumByName(name, ok);
		tqApp->eventLoop()->exitLoop();
	}
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


void MediaImpl::createTopLevelEntry(TDEIO::UDSEntry& entry) const
{
	entry.clear();
	addAtom(entry, TDEIO::UDS_URL, 0, "media:/");
	addAtom(entry, TDEIO::UDS_NAME, 0, ".");
	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, TDEIO::UDS_ACCESS, 0555);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, "blockdevice");
}

void MediaImpl::slotStatResult(TDEIO::Job *job)
{
	if ( job->error() == 0)
	{
		TDEIO::StatJob *stat_job = static_cast<TDEIO::StatJob *>(job);
		m_entryBuffer = stat_job->statResult();
	}

	tqApp->eventLoop()->exitLoop();
}

TDEIO::UDSEntry MediaImpl::extractUrlInfos(const KURL &url)
{
	if (url.isEmpty())
	{
		return TDEIO::UDSEntry();
	}

	m_entryBuffer.clear();

	TDEIO::StatJob *job = TDEIO::stat(url, false);
	job->setAutoWarningHandlingEnabled( false );
	connect( job, TQ_SIGNAL( result(TDEIO::Job *) ),
	         this, TQ_SLOT( slotStatResult(TDEIO::Job *) ) );
	connect( job, TQ_SIGNAL( warning( TDEIO::Job *, const TQString & ) ),
	         this, TQ_SLOT( slotWarning( TDEIO::Job *, const TQString & ) ) );
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

	if (url.isLocalFile())
	{
		addAtom(infos, TDEIO::UDS_LOCAL_PATH, 0, url.path());
	}
	
	return infos;
}


void MediaImpl::createMediumEntry(TDEIO::UDSEntry& entry,
                                  const Medium &medium)
{
	kdDebug(1219) << "MediaProtocol::createMedium" << endl;

	TQString url = "media:/"+medium.name();

	kdDebug(1219) << "url = " << url << ", mime = " << medium.mimeType() << endl;

	entry.clear();

	addAtom(entry, TDEIO::UDS_URL, 0, url);

	TQString label = TDEIO::encodeFileName( medium.prettyLabel() );
	addAtom(entry, TDEIO::UDS_NAME, 0, label);

	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);

	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, medium.mimeType());

	if (medium.softHidden()) {
		addAtom(entry, TDEIO::UDS_HIDDEN , true);
	}

	if (!medium.iconName().isEmpty())
	{
		addAtom(entry, TDEIO::UDS_ICON_NAME, 0, medium.iconName());
	}
	else
	{
		TQString mime = medium.mimeType();
		TQString icon = KMimeType::mimeType(mime)->icon(mime, false);
		addAtom(entry, TDEIO::UDS_ICON_NAME, 0, icon);
	}

	if (medium.needMounting())
	{
		addAtom(entry, TDEIO::UDS_ACCESS, 0400);
	}
	else
	{
		KURL url = medium.prettyBaseURL();
		entry += extractUrlInfos(url);
	}
}

#include "mediaimpl.moc"
