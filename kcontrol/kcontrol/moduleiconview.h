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

#ifndef __moduleiconview_h__
#define __moduleiconview_h__

#include <tdelistview.h>

class ConfigModule;
class ConfigModuleList;

class ModuleIconItem : public TDEListViewItem
{

public:
  ModuleIconItem(TQListView *parent, const TQString& text, const TQPixmap& pm, ConfigModule *m = 0)
	: TDEListViewItem(parent, text)
	, _tag(TQString::null)
	, _module(m)
	{
	  setPixmap(0, pm);
	}

  void setConfigModule(ConfigModule* m) { _module = m; }
  void setTag(const TQString& t) { _tag = t; }
  void setOrderNo(int order)
  {
    TQString s;
    setText(1, s.sprintf( "%02d", order ) );
  }

  ConfigModule* module() { return _module; }
  TQString tag() { return _tag; }


private:
  TQString       _tag;
  ConfigModule *_module;
};

class ModuleIconView : public TDEListView
{
  TQ_OBJECT

public:
  ModuleIconView(ConfigModuleList *list, TQWidget * parent = 0, const char * name = 0);
  
  void makeSelected(ConfigModule* module);
  void makeVisible(ConfigModule *module);
  void fill();

signals:
  void moduleSelected(ConfigModule*);

protected slots:
  void slotItemSelected(TQListViewItem*);

protected:
  void keyPressEvent(TQKeyEvent *);
  TQPixmap loadIcon( const TQString &name );
  
private:
  TQString           _path; 
  ConfigModuleList *_modules;

};



#endif
