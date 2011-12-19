/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <kservicegroup.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kuser.h>

#include <tqobjectlist.h>
#include <tqaccel.h>

#include "global.h"

bool KCGlobal::_root = false;
bool KCGlobal::_infocenter = false;
TQStringList KCGlobal::_types;
TQString KCGlobal::_uname = "";
TQString KCGlobal::_hname = "";
TQString KCGlobal::_kdeversion = "";
TQString KCGlobal::_isystem = "";
TQString KCGlobal::_irelease = "";
TQString KCGlobal::_iversion = "";
TQString KCGlobal::_imachine = "";
IndexViewMode KCGlobal::_viewmode = Icon;
KIcon::StdSizes KCGlobal::_iconsize = KIcon::SizeMedium;
TQString KCGlobal::_baseGroup = "";

void KCGlobal::init()
{
  char buf[256];
  buf[0] = '\0';
  if (!gethostname(buf, sizeof(buf)))
    buf[sizeof(buf)-1] ='\0';
  TQString hostname(buf);
  
  setHostName(hostname);
  setUserName(KUser().loginName());
  setRoot(getuid() == 0);

  setKDEVersion(KDE::versionString());

  struct utsname info;
  uname(&info);

  setSystemName(info.sysname);
  setSystemRelease(info.release);
  setSystemVersion(info.version);
  setSystemMachine(info.machine);
}

void KCGlobal::setType(const TQCString& s)
{
  TQString string = s.lower();
  _types = TQStringList::split(',', string);
}

TQString KCGlobal::baseGroup()
{
  if ( _baseGroup.isEmpty() )
  {
    KServiceGroup::Ptr group = KServiceGroup::baseGroup( _infocenter ? "info" : "settings" );
    if (group)
    {
      _baseGroup = group->relPath();
      kdDebug(1208) << "Found basegroup = " << _baseGroup << endl;
      return _baseGroup;
    }
    // Compatibility with old behaviour, in case of missing .directory files.
    if (_baseGroup.isEmpty())
    {
      if (_infocenter)
      {
        kdWarning() << "No K menu group with X-KDE-BaseGroup=info found ! Defaulting to Settings/Information/" << endl;
        _baseGroup = TQString::fromLatin1("Settings/Information/");
      }
      else
      {
        kdWarning() << "No K menu group with X-KDE-BaseGroup=settings found ! Defaulting to Settings/" << endl;
        _baseGroup = TQString::fromLatin1("Settings/");
      }
    }
  }
  return _baseGroup;
}

void KCGlobal::repairAccels( TQWidget * tw )
{
    TQObjectList * l = tw->queryList( TQACCEL_OBJECT_NAME_STRING );
    TQObjectListIt it( *l );             // iterate over the buttons
    TQObject * obj;
    while ( (obj=it.current()) != 0 ) { // for each found object...
        ++it;
        ((TQAccel*)obj)->repairEventFilter();
    }
    delete l;                           // delete the list, not the objects
}
