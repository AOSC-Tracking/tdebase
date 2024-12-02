/**
 * kprivacymanager.cpp
 *
 * Copyright (c) 2003 Ralf Hoelzer <ralf@well.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kprivacymanager.h"
#include <tdeapplication.h>
#include <dcopclient.h>
#include <tdeconfig.h>
#include <ksimpleconfig.h>
#include <tdeprocess.h>
#include <tdeglobal.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <tderecentdocument.h>
#include <kstandarddirs.h>

#include <tqstringlist.h>
#include <tqfile.h>
#include <tqdir.h>

KPrivacyManager::KPrivacyManager()
{
  if (!kapp->dcopClient()->isAttached())
    kapp->dcopClient()->attach();

  m_error = false;
}


KPrivacyManager::~KPrivacyManager()
{
}

bool KPrivacyManager::clearThumbnails()
{
  // http://freedesktop.org/Standards/Home
  // http://triq.net/~jens/thumbnail-spec/index.html

  TQDir thumbnailDir( TQDir::homeDirPath() + "/.thumbnails/normal");
  thumbnailDir.setFilter( TQDir::Files );
  TQStringList entries = thumbnailDir.entryList();
  for( TQStringList::Iterator it = entries.begin() ; it != entries.end() ; ++it)
    if(!thumbnailDir.remove(*it)) m_error = true;
  if(m_error) return m_error;

  thumbnailDir.setPath(TQDir::homeDirPath() + "/.thumbnails/large");
  entries = thumbnailDir.entryList();
  for( TQStringList::Iterator it = entries.begin() ; it != entries.end() ; ++it)
    if(!thumbnailDir.remove(*it)) m_error = true;
  if(m_error) return m_error;

  thumbnailDir.setPath(TQDir::homeDirPath() + "/.thumbnails/fail");
  entries = thumbnailDir.entryList();
  for( TQStringList::Iterator it = entries.begin() ; it != entries.end() ; ++it)
    if(!thumbnailDir.remove(*it)) m_error = true;
  
  return m_error;
}

bool KPrivacyManager::clearRunCommandHistory() const
{
  return kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "clearCommandHistory()", TQString("") );
}

bool KPrivacyManager::clearAllCookies() const
{
  return kapp->dcopClient()->send( "kded", "kcookiejar", "deleteAllCookies()", TQString("") );
}

bool KPrivacyManager::clearSavedClipboardContents()
{
  if(!isApplicationRegistered("klipper"))
  {
    TDEConfig *c = new TDEConfig("klipperrc", false, false);

    {
      TDEConfigGroupSaver saver(c, "General");
      c->deleteEntry("ClipboardData");
      c->sync();
    }
    delete c;
    return true;
  }

  return kapp->dcopClient()->send( "klipper", "klipper", "clearClipboardHistory()", TQString("") );
}

bool KPrivacyManager::clearFormCompletion() const
{
  TQFile completionFile(locateLocal("data", "tdehtml/formcompletions"));

  return completionFile.remove();
}

bool KPrivacyManager::clearWebCache() const
{
    TDEProcess process;
    process << "tdeio_http_cache_cleaner" << "--clear-all";
    return process.start(TDEProcess::DontCare);
}

bool KPrivacyManager::clearRecentDocuments() const
{
  TDERecentDocument::clear();
  return TDERecentDocument::recentDocuments().isEmpty();
}

bool KPrivacyManager::clearQuickStartMenu() const
{
  return kapp->dcopClient()->send( "kicker", "kicker", "clearQuickStartMenu()", TQString("") );
}

bool KPrivacyManager::clearWebHistory()
{
  TQStringList args("--preload");

  // preload Konqueror if it is not running
  if(!isApplicationRegistered("konqueror"))
  {
    kdDebug() << "couldn't find Konqueror instance, preloading." << endl;
    kapp->tdeinitExec("konqueror", args, 0,0);
  }

  return kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
                                   "notifyClear(TQCString)", TQString("") );
}

bool KPrivacyManager::clearFavIcons()
{
  TQDir favIconDir(TDEGlobal::dirs()->saveLocation( "cache", "favicons/" ));
  favIconDir.setFilter( TQDir::Files );

  TQStringList entries = favIconDir.entryList();

  // erase all files in favicon directory
  for( TQStringList::Iterator it = entries.begin() ; it != entries.end() ; ++it)
    if(!favIconDir.remove(*it)) m_error = true;
  return m_error;
}

bool KPrivacyManager::clearKPDFDocData()
{
  TQDir kPDFDir(TDEGlobal::dirs()->saveLocation( "data", "kpdf/" ));
  kPDFDir.setFilter( TQDir::Files );

  TQStringList entries = kPDFDir.entryList();

  // erase all files in kpdf directory
  for( TQStringList::Iterator it = entries.begin() ; it != entries.end() ; ++it)
    if(!kPDFDir.remove(*it)) m_error = true;
  return m_error;
}

bool KPrivacyManager::isApplicationRegistered(const TQString &appName)
{

  QCStringList regApps = kapp->dcopClient()->registeredApplications();

  for ( QCStringList::Iterator it = regApps.begin(); it != regApps.end(); ++it )
    if((*it).find(appName.latin1()) != -1) return true;

  return false;
}

#include "kprivacymanager.moc"
