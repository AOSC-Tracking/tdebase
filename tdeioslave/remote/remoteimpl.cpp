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

#include "remoteimpl.h"

#include <kdebug.h>
#include <tdeglobalsettings.h>
#include <tdestandarddirs.h>
#include <tdedesktopfile.h>
#include <kservice.h>
#include <tdelocale.h>

#include <tqdir.h>
#include <tqfile.h>

#include <sys/stat.h>

#define WIZARD_URL "remote:/x-wizard_service.desktop"
#define WIZARD_SERVICE "knetattach"

RemoteImpl::RemoteImpl()
{
	TDEGlobal::dirs()->addResourceType("remote_entries",
		TDEStandardDirs::kde_default("data") + "remoteview");

	TQString path = TDEGlobal::dirs()->saveLocation("remote_entries");

	TQDir dir = path;
	if (!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("remoteview");
	}
}

void RemoteImpl::listRoot(TQValueList<TDEIO::UDSEntry> &list) const
{
	kdDebug(1220) << "RemoteImpl::listRoot" << endl;

	TQStringList names_found;
	TQStringList dirList = TDEGlobal::dirs()->resourceDirs("remote_entries");

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
			if (!names_found.contains(*name))
			{
				entry.clear();
				createEntry(entry, *dirpath, *name);
				list.append(entry);
				names_found.append(*name);
			}
		}
	}
}

bool RemoteImpl::findDirectory(const TQString &filename, TQString &directory) const
{
	kdDebug(1220) << "RemoteImpl::findDirectory" << endl;

	TQStringList dirList = TDEGlobal::dirs()->resourceDirs("remote_entries");

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
			if (*name==filename)
			{
				directory = *dirpath;
				return true;
			}
		}
	}

	return false;
}

TQString RemoteImpl::findDesktopFile(const TQString &filename) const
{
	kdDebug(1220) << "RemoteImpl::findDesktopFile" << endl;
	
	TQString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		return directory+filename+".desktop";
	}
	
	return TQString::null;
}

KURL RemoteImpl::findBaseURL(const TQString &filename) const
{
	kdDebug(1220) << "RemoteImpl::findBaseURL" << endl;

	TQString file = findDesktopFile(filename);
	if (!file.isEmpty())
	{
		TDEDesktopFile desktop(file, true);
		return desktop.readURL();
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


void RemoteImpl::createTopLevelEntry(TDEIO::UDSEntry &entry) const
{
	entry.clear();
	addAtom(entry, TDEIO::UDS_NAME, 0, ".");
	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, TDEIO::UDS_ACCESS, 0555);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, "network");
}

static KURL findWizardRealURL()
{
	KURL url;
	KService::Ptr service = KService::serviceByDesktopName(WIZARD_SERVICE);

	if (service && service->isValid())
	{
		url.setPath( locate("apps",
				    service->desktopEntryPath())
				);
	}

	return url;
}

bool RemoteImpl::createWizardEntry(TDEIO::UDSEntry &entry) const
{
	entry.clear();

	KURL url = findWizardRealURL();

	if (!url.isValid())
	{
		return false;
	}

	addAtom(entry, TDEIO::UDS_NAME, 0, i18n("Add a Network Folder"));
	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFREG);
	addAtom(entry, TDEIO::UDS_URL, 0, WIZARD_URL);
	addAtom(entry, TDEIO::UDS_LOCAL_PATH, 0, url.path());
	addAtom(entry, TDEIO::UDS_ACCESS, 0500);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "application/x-desktop");
	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, "wizard");

	return true;
}

bool RemoteImpl::isWizardURL(const KURL &url) const
{
	return url==KURL(WIZARD_URL);
}


void RemoteImpl::createEntry(TDEIO::UDSEntry &entry,
                             const TQString &directory,
                             const TQString &file) const
{
	kdDebug(1220) << "RemoteImpl::createEntry" << endl;

	TQString desktopPath(directory + file);
	TDEDesktopFile desktop(desktopPath, true);

	kdDebug(1220) << "path = " << directory << file << endl;

	entry.clear();

	TQString new_filename = file;
	new_filename.truncate( file.length()-8);
	
	addAtom(entry, TDEIO::UDS_NAME, 0, desktop.readName());
	addAtom(entry, TDEIO::UDS_URL, 0, "remote:/"+new_filename);

	addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, "inode/directory");

	TQString icon = desktop.readIcon();

	addAtom(entry, TDEIO::UDS_ICON_NAME, 0, icon);
	addAtom(entry, TDEIO::UDS_LINK_DEST, 0, desktop.readURL());

	addAtom(entry, TDEIO::UDS_LOCAL_PATH, 0, desktopPath);
}

bool RemoteImpl::statNetworkFolder(TDEIO::UDSEntry &entry, const TQString &filename) const
{
	kdDebug(1220) << "RemoteImpl::statNetworkFolder: " << filename << endl;

	TQString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		createEntry(entry, directory, filename+".desktop");
		return true;
	}
	
	return false;
}

bool RemoteImpl::deleteNetworkFolder(const TQString &filename) const
{
	kdDebug(1220) << "RemoteImpl::deleteNetworkFolder: " << filename << endl;

	TQString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		kdDebug(1220) << "Removing " << directory << filename << ".desktop" << endl;
		return TQFile::remove(directory+filename+".desktop");
	}
	
	return false;
}

bool RemoteImpl::renameFolders(const TQString &src, const TQString &dest,
                               bool overwrite) const
{
	kdDebug(1220) << "RemoteImpl::renameFolders: "
	          << src << ", " << dest << endl;

	TQString directory;
	if (findDirectory(src+".desktop", directory))
	{
		if (!overwrite && TQFile::exists(directory+dest+".desktop"))
		{
			return false;
		}
		
		kdDebug(1220) << "Renaming " << directory << src << ".desktop"<< endl;
		TQDir dir(directory);
		bool res = dir.rename(src+".desktop", dest+".desktop");
		if (res)
		{
			TDEDesktopFile desktop(directory+dest+".desktop");
			desktop.writeEntry("Name", dest);
		}
		return res;
	}

	return false;
}


