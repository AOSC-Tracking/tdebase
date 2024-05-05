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

#include <stdlib.h>

#include <kdebug.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <dcopclient.h>
#include <tdecmdlineargs.h>
#include <tdeglobal.h>


#include "tdeio_home.h"

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
		TDECmdLineArgs::init(argc, argv, "tdeio_home", 0, 0, 0, 0);
		TDECmdLineArgs::addCmdLineOptions( options );
		TDEApplication app( false, false, false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();

		TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
		HomeProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


HomeProtocol::HomeProtocol(const TQCString &protocol,
                               const TQCString &pool, const TQCString &app)
	: ForwardingSlaveBase(protocol, pool, app)
{
}

HomeProtocol::~HomeProtocol()
{
}

bool HomeProtocol::rewriteURL(const KURL &url, KURL &newUrl)
{
	TQString name, path;

	if ( !m_impl.parseURL(url, name, path) )
	{
		error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
		return false;
	}


	if ( !m_impl.realURL(name, path, newUrl) )
	{
		error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
		return false;
	}

	return true;
}


void HomeProtocol::listDir(const KURL &url)
{
	kdDebug() << "HomeProtocol::listDir: " << url << endl;

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

void HomeProtocol::listRoot()
{
	TDEIO::UDSEntry entry;

	TDEIO::UDSEntryList home_entries;
	bool ok = m_impl.listHomes(home_entries);

	if (!ok) // can't happen
	{
		error(TDEIO::ERR_UNKNOWN, "");
		return;
	}

	totalSize(home_entries.count()+1);

	m_impl.createTopLevelEntry(entry);
	listEntry(entry, false);

	TDEIO::UDSEntryListIterator it = home_entries.begin();
	TDEIO::UDSEntryListIterator end = home_entries.end();

	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}

void HomeProtocol::stat(const KURL &url)
{
	kdDebug() << "HomeProtocol::stat: " << url << endl;
 
	TQString path = url.path();
	if ( path.isEmpty() || path == "/" )
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

		if ( m_impl.statHome(name, entry) )
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
