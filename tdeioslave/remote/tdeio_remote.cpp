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
#include <tdeglobal.h>


#include "tdeio_remote.h"

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
		TDECmdLineArgs::init(argc, argv, "tdeio_remote", 0, 0, 0, 0);
		TDECmdLineArgs::addCmdLineOptions( options );
		TDEApplication app( false, false, false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();

		TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
		RemoteProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


RemoteProtocol::RemoteProtocol(const TQCString &protocol,
                               const TQCString &pool, const TQCString &app)
	: SlaveBase(protocol, pool, app)
{
}

RemoteProtocol::~RemoteProtocol()
{
}

void RemoteProtocol::listDir(const KURL &url)
{
	kdDebug(1220) << "RemoteProtocol::listDir: " << url << endl;

	if ( url.path().length() <= 1 )
	{
		listRoot();
		return;
	}

	int second_slash_idx = url.path().find( '/', 1 );
	TQString root_dirname = url.path().mid( 1, second_slash_idx-1 );
	
	KURL target = m_impl.findBaseURL( root_dirname );
	kdDebug(1220) << "possible redirection target : " << target << endl;
	if( target.isValid() )
	{
		target.addPath( url.path().remove(0, second_slash_idx) );
		redirection(target);
		finished();
		return;
	}

	error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
}

void RemoteProtocol::listRoot()
{
	TDEIO::UDSEntry entry;

	TDEIO::UDSEntryList remote_entries;
        m_impl.listRoot(remote_entries);

	totalSize(remote_entries.count()+2);

	m_impl.createTopLevelEntry(entry);
	listEntry(entry, false);
	
	m_impl.createWizardEntry(entry);
	listEntry(entry, false);

	TDEIO::UDSEntryListIterator it = remote_entries.begin();
	TDEIO::UDSEntryListIterator end = remote_entries.end();

	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}

void RemoteProtocol::stat(const KURL &url)
{
	kdDebug(1220) << "RemoteProtocol::stat: " << url << endl;
 
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

	if (m_impl.isWizardURL(url))
	{
		TDEIO::UDSEntry entry;
		if (m_impl.createWizardEntry(entry))
		{
			statEntry(entry);
			finished();
		}
		else
		{
			error(TDEIO::ERR_DOES_NOT_EXIST, url.prettyURL());
		}
		return;
	}

	int second_slash_idx = url.path().find( '/', 1 );
	TQString root_dirname = url.path().mid( 1, second_slash_idx-1 );
	
	if ( second_slash_idx==-1 || ( (int)url.path().length() )==second_slash_idx+1 )
	{
		TDEIO::UDSEntry entry;
		if (m_impl.statNetworkFolder(entry, root_dirname))
		{
			statEntry(entry);
			finished();
			return;
		}
	}
	else
	{
		KURL target = m_impl.findBaseURL(  root_dirname );
		kdDebug( 1220 ) << "possible redirection target : " << target << endl;
		if (  target.isValid() )
		{
			target.addPath( url.path().remove( 0, second_slash_idx ) );
			redirection( target );
			finished();
			return;
		}
	}
	
	error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
}

void RemoteProtocol::del(const KURL &url, bool /*isFile*/)
{
	kdDebug(1220) << "RemoteProtocol::del: " << url << endl;

	if (!m_impl.isWizardURL(url)
	 && m_impl.deleteNetworkFolder(url.fileName()))
	{
		finished();
		return;
	}

	error(TDEIO::ERR_CANNOT_DELETE, url.prettyURL());
}

void RemoteProtocol::get(const KURL &url)
{
	kdDebug(1220) << "RemoteProtocol::get: " << url << endl;

	TQString file = m_impl.findDesktopFile( url.fileName() );
	kdDebug(1220) << "desktop file : " << file << endl;
	
	if (!file.isEmpty())
	{
		KURL desktop;
		desktop.setPath(file);
		
		redirection(desktop);
		finished();
		return;
	}

	error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
}

void RemoteProtocol::rename(const KURL &src, const KURL &dest,
                            bool overwrite)
{
	if (src.protocol()!="remote" || dest.protocol()!="remote"
         || m_impl.isWizardURL(src) || m_impl.isWizardURL(dest))
	{
		error(TDEIO::ERR_UNSUPPORTED_ACTION, src.prettyURL());
		return;
	}

	if (m_impl.renameFolders(src.fileName(), dest.fileName(), overwrite))
	{
		finished();
		return;
	}

	error(TDEIO::ERR_CANNOT_RENAME, src.prettyURL());
}
