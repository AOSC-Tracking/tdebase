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

#ifndef __global_h__
#define __global_h__

#include <kicontheme.h>

#include <tqstring.h>
#include <tqstringlist.h>
#include <tqcstring.h>

enum IndexViewMode {Icon, Tree};

class KCGlobal
{
public:

  static void init();

  static bool isInfoCenter() { return _infocenter; }
  static bool root() { return _root; }
  static TQStringList types() { return _types; }
  static TQString userName() { return _uname; }
  static TQString hostName() { return _hname; }
  static TQString kdeVersion() { return _tdeversion; }
  static TQString systemName() { return _isystem; }
  static TQString systemRelease() { return _irelease; }
  static TQString systemVersion() { return _iversion; }
  static TQString systemMachine() { return _imachine; }
  static IndexViewMode viewMode() { return _viewmode; }
  static TDEIcon::StdSizes iconSize() { return _iconsize; }
  static bool showHiddenModules() { return _showhidden; }
  static TQString baseGroup();

  static void setIsInfoCenter(bool b) { _infocenter = b; }
  static void setRoot(bool r) { _root = r; }
  static void setType(const TQCString& s);
  static void setUserName(const TQString& n){ _uname = n; }
  static void setHostName(const TQString& n){ _hname = n; }
  static void setKDEVersion(const TQString& n){ _tdeversion = n; }
  static void setSystemName(const TQString& n){ _isystem = n; }
  static void setSystemRelease(const TQString& n){ _irelease = n; }
  static void setSystemVersion(const TQString& n){ _iversion = n; }
  static void setSystemMachine(const TQString& n){ _imachine = n; }
  static void setViewMode(IndexViewMode m) { _viewmode = m; }
  static void setIconSize(TDEIcon::StdSizes s) { _iconsize = s; }
  static void setShowHiddenModules(bool o) { _showhidden = o; }

  static void repairAccels( TQWidget * tw );

private:
  static bool _root;
  static bool _infocenter;
  static TQStringList _types;
  static TQString _uname, _hname, _isystem, _irelease, _iversion, _imachine, _tdeversion;
  static IndexViewMode _viewmode;
  static TDEIcon::StdSizes _iconsize;
  static TQString _baseGroup;
  static bool _showhidden;
};

#endif
