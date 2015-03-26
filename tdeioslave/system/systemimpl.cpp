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

#include "systemimpl.h"

#include <kdebug.h>
#include <tdeglobalsettings.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>

#include <tqapplication.h>
#include <tqeventloop.h>
#include <tqdir.h>
#include <tqfile.h>

#include <sys/stat.h>

SystemImpl::SystemImpl() : TQObject()
{
	TDEGlobal::dirs()->addResourceType("system_entries",
		TDEStandardDirs::kde_default("data") + "systemview");
}

bool SystemImpl::listRoot(TQValueList<TDEIO::UDSEntry> &list)
{
	kdDebug() << "SystemImpl::listRoot" << endl;

	TQStringList names_found;
	TQStringList dirList = TDEGlobal::dirs()->resourceDirs("system_entries");

	TQStringList::ConstIterator dirpath = dirList.begin();
	TQStringList::ConstIterator end = dirList.end();
	for(; dirpath!=end; ++dirpath)
	{
		TQDir dir = *dirpath;
		if (!dir.exists()) continue;

		TQStringList filenames
			= dir.entryList( TQDir::Files | TQDir::Readable );


		TDEIO::UDSEntry entry;

		TQStringList::ConstIterator filename = filenames.begin();
		TQStringList::ConstIterator endf = filenames.end();

		for(; filename!=endf; ++filename)
		{
			if (!names_found.contains(*filename))
			{
				entry.clear();
				createEntry(entry, *dirpath, *filename);
				if ( !entry.isEmpty() )
				{
					list.append(entry);
					names_found.append(*filename);
				}
			}
		}
	}

	return true;
}

bool SystemImpl::parseURL(const KURL &url, TQString &name, TQString &path) const
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

bool SystemImpl::realURL(const TQString &name, const TQString &path,
                         KURL &url) const
{
	url = findBaseURL(name);
	if (!url.isValid())
	{
		return false;
	}

	url.addPath(path);
	return true;
}

bool SystemImpl::statByName(const TQString &filename, TDEIO::UDSEntry& entry)
{
	kdDebug() << "SystemImpl::statByName" << endl;

	TQStringList dirList = TDEGlobal::dirs()->resourceDirs("system_entries");

	TQStringList::ConstIterator dirpath = dirList.begin();
	TQStringList::ConstIterator end = dirList.end();
	for(; dirpath!=end; ++dirpath)
	{
		TQDir dir = *dirpath;
		if (!dir.exists()) continue;

		TQStringList filenames
			= dir.entryList( TQDir::Files | TQDir::Readable );


		TQStringList::ConstIterator name = filenames.begin();
		TQStringList::ConstIterator endf = filenames.end();

		for(; name!=endf; ++name)
		{
			if (*name==filename+".desktop")
			{
				createEntry(entry, *dirpath, *name);
				return true;
			}
		}
	}

	return false;
}

KURL SystemImpl::findBaseURL(const TQString &filename) const
{
	kdDebug() << "SystemImpl::findBaseURL" << endl;

	TQStringList dirList = TDEGlobal::dirs()->resourceDirs("system_entries");

	TQStringList::ConstIterator dirpath = dirList.begin();
	TQStringList::ConstIterator end = dirList.end();
	for(; dirpath!=end; ++dirpath)
	{
		TQDir dir = *dirpath;
		if (!dir.exists()) continue;

		TQStringList filenames
			= dir.entryList( TQDir::Files | TQDir::Readable );


		TDEIO::UDSEntry entry;

		TQStringList::ConstIterator name = filenames.begin();
		TQStringList::ConstIterator endf = filenames.end();

		for(; name!=endf; ++name)
		{
			if (*name==filename+".desktop")
			{
				KDesktopFile desktop(*dirpath+filename+".desktop", true);
				if ( desktop.readURL().isEmpty() )
				{
					KURL url;
					url.setPath( desktop.readPath() );
					return url;
				}
				
				return desktop.readURL();
			}
		}
	}

	return KURL();
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


void SystemImpl::createTopLevelEntry(TDEIO::UDSEntry &entry) const
{
	entry.clear();
	addAtom(entry, TDEIO::UDS_NAME, 0, ".");
	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, TDEIO::UDS_ACCESS, 0555);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "inode/system_directory");
	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, "computer");
}

TQString SystemImpl::readPathINL(TQString filename)
{
	bool isPathExpanded = false;
	TQString unexpandedPath;
	TQFile f( filename );
	if (!f.open(IO_ReadOnly))
		return TQString();
	// set the codec for the current locale
	TQTextStream s(&f);
	TQString line = s.readLine();
	while (!line.isNull())
	{
		if (line.startsWith("Path=$(")) {
			isPathExpanded = true;
			unexpandedPath = line.remove("Path=");
		}
		line = s.readLine();
	}
 	if (isPathExpanded == false) {
		KDesktopFile desktop(filename, true);
		return desktop.readPath();
	}
	else {
		return unexpandedPath;
	}
}

void SystemImpl::createEntry(TDEIO::UDSEntry &entry,
                             const TQString &directory,
                             const TQString &file)
{
	kdDebug() << "SystemImpl::createEntry" << endl;

	KDesktopFile desktop(directory+file, true);

	kdDebug() << "path = " << directory << file << endl;

	entry.clear();

	// Ensure that we really want this entry to be displayed
	if ( desktop.readURL().isEmpty() && readPathINL(directory+file).isEmpty() )
	{
		return;
	}
	
	addAtom(entry, TDEIO::UDS_NAME, 0, desktop.readName());
	
	TQString new_filename = file;
	new_filename.truncate(file.length()-8);

	if ( desktop.readURL().isEmpty() )
	{
		addAtom(entry, TDEIO::UDS_URL, 0, readPathINL(directory+file));
	}
	else
	{
		addAtom(entry, TDEIO::UDS_URL, 0, "system:/"+new_filename);
	}
	
	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "inode/directory");

	TQString icon = desktop.readIcon();
	TQString empty_icon = desktop.readEntry("EmptyIcon");

	if (!empty_icon.isEmpty())
	{
		KURL url = desktop.readURL();

		m_lastListingEmpty = true;

		TDEIO::ListJob *job = TDEIO::listDir(url, false, false);
		connect( job, TQT_SIGNAL( entries(TDEIO::Job *,
		                      const TDEIO::UDSEntryList &) ),
		         this, TQT_SLOT( slotEntries(TDEIO::Job *,
			             const TDEIO::UDSEntryList &) ) );
		connect( job, TQT_SIGNAL( result(TDEIO::Job *) ),
		         this, TQT_SLOT( slotResult(TDEIO::Job *) ) );
		tqApp->eventLoop()->enterLoop();

		if (m_lastListingEmpty) icon = empty_icon;
	}

	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, icon);
}

void SystemImpl::slotEntries(TDEIO::Job *job, const TDEIO::UDSEntryList &list)
{
	if (list.size()>0)
	{
		job->kill(true);
		m_lastListingEmpty = false;
		tqApp->eventLoop()->exitLoop();
	}
}

void SystemImpl::slotResult(TDEIO::Job *)
{
	tqApp->eventLoop()->exitLoop();
}


#include "systemimpl.moc"
