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

#include <stdlib.h>

#include <kdebug.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <dcopclient.h>
#include <tdecmdlineargs.h>

#include <tqeventloop.h>

#include "mediaimpl.h"
#include "tdeio_media.h"


static const TDECmdLineOptions options[] =
{
	{ "+protocol", I18N_NOOP( "Protocol name" ), 0 },
	{ "+pool", I18N_NOOP( "Socket name" ), 0 },
	{ "+app", I18N_NOOP( "Socket name" ), 0 },
	TDECmdLineLastOption
};

extern "C" {
	int TDE_EXPORT kdemain( int argc, char **argv )
	{
		// TDEApplication is necessary to use other ioslaves
		putenv(strdup("SESSION_MANAGER="));
		TDECmdLineArgs::init(argc, argv, "tdeio_media", 0, 0, 0, 0);
		TDECmdLineArgs::addCmdLineOptions( options );
		TDEApplication app( false, false, false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();

		TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
		MediaProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


MediaProtocol::MediaProtocol(const TQCString &protocol,
                             const TQCString &pool, const TQCString &app)
	: ForwardingSlaveBase(protocol, pool, app)
{
	connect( &m_impl, TQ_SIGNAL( warning( const TQString & ) ),
	         this, TQ_SLOT( slotWarning( const TQString & ) ) );
}

MediaProtocol::~MediaProtocol()
{
}

bool MediaProtocol::rewriteURL(const KURL &url, KURL &newUrl)
{
	TQString name, path;

	if ( !m_impl.parseURL(url, name, path) )
	{
		error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
		return false;
	}

	if ( !m_impl.realURL(name, path, newUrl) )
	{
		error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
		return false;
	}

	return true;
}

void MediaProtocol::put(const KURL &url, int permissions,
                        bool overwrite, bool resume)
{
	kdDebug(1219) << "MediaProtocol::put: " << url << endl;

	TQString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( ok && path.isEmpty() )
	{
		error(TDEIO::ERR_CANNOT_OPEN_FOR_WRITING, url.prettyURL());
	}
	else
	{
		ForwardingSlaveBase::put(url, permissions, overwrite, resume);
	}
}

void MediaProtocol::rename(const KURL &src, const KURL &dest, bool overwrite)
{
	kdDebug(1219) << "MediaProtocol::rename: " << src << ", " << dest << ", "
	          << overwrite << endl;

	TQString src_name, src_path;
	bool ok = m_impl.parseURL(src, src_name, src_path);
	TQString dest_name, dest_path;
	ok &= m_impl.parseURL(dest, dest_name, dest_path);

	if ( ok && src_path.isEmpty() && dest_path.isEmpty()
	  && src.protocol() == "media" && dest.protocol() == "media" )
	{
		if (!m_impl.setUserLabel(src_name, dest_name))
		{
			error(m_impl.lastErrorCode(), m_impl.lastErrorMessage());
		}
		else
		{
			finished();
		}
	}
	else
	{
		ForwardingSlaveBase::rename(src, dest, overwrite);
	}
}

void MediaProtocol::mkdir(const KURL &url, int permissions)
{
	kdDebug(1219) << "MediaProtocol::mkdir: " << url << endl;

	TQString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( ok && path.isEmpty() )
	{
		error(TDEIO::ERR_COULD_NOT_MKDIR, url.prettyURL());
	}
	else
	{
		ForwardingSlaveBase::mkdir(url, permissions);
	}
}

void MediaProtocol::del(const KURL &url, bool isFile)
{
	kdDebug(1219) << "MediaProtocol::del: " << url << endl;

	TQString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( ok && path.isEmpty() )
	{
		error(TDEIO::ERR_CANNOT_DELETE, url.prettyURL());
	}
	else
	{
		ForwardingSlaveBase::del(url, isFile);
	}
}

void MediaProtocol::stat(const KURL &url)
{
	kdDebug(1219) << "MediaProtocol::stat: " << url << endl;
	TQString path = url.path();
	if( path.isEmpty() || path == "/" )
	{
		// The root is "virtual" - it's not a single physical directory
		TDEIO::UDSEntry entry;
		m_impl.createTopLevelEntry( entry );
		statEntry( entry );
		finished();
		return;
	}

	TQString name;
	bool ok = m_impl.parseURL(url, name, path);

	if ( !ok )
	{
		error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
		return;
	}

	if( path.isEmpty() )
	{
		TDEIO::UDSEntry entry;

		if ( m_impl.statMedium(name, entry)
		  || m_impl.statMediumByLabel(name, entry) )
		{
			statEntry(entry);
			finished();
		}
		else
		{
			error(TDEIO::ERR_DOES_NOT_EXIST, url.prettyURL());
		}
	}
	else
	{
		ForwardingSlaveBase::stat(url);
	}
}

void MediaProtocol::listDir(const KURL &url)
{
	kdDebug(1219) << "MediaProtocol::listDir: " << url << endl;

	if ( url.path().length() <= 1 )
	{
		listRoot();
		return;
	}

	TQString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( !ok )
	{
		error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
		return;
	}

	ForwardingSlaveBase::listDir(url);
}

void MediaProtocol::listRoot()
{
	TDEIO::UDSEntry entry;

	TDEIO::UDSEntryList media_entries;
	bool ok = m_impl.listMedia(media_entries);

	if (!ok)
	{
		error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
		return;
	}

	totalSize(media_entries.count()+1);

	m_impl.createTopLevelEntry(entry);
	listEntry(entry, false);

	TDEIO::UDSEntryListIterator it = media_entries.begin();
	TDEIO::UDSEntryListIterator end = media_entries.end();

	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}

void MediaProtocol::slotWarning( const TQString &msg )
{
	warning( msg );
}

#include "tdeio_media.moc"
