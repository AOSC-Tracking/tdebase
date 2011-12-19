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

#ifndef __moduletreeview_h__
#define __moduletreeview_h__

#include <tqpalette.h>
#include <tqptrlist.h>
#include <tqlistview.h>
#include <klistview.h>
#include <tqdict.h>


class ConfigModule;
class ConfigModuleList;
class TQPainter;

class ModuleTreeItem : public TQListViewItem
{

public:
  ModuleTreeItem(TQListViewItem *parent, ConfigModule *module = 0);
  ModuleTreeItem(TQListViewItem *parent, const TQString& text);
  ModuleTreeItem(TQListView *parent, ConfigModule *module = 0);
  ModuleTreeItem(TQListView *parent, const TQString& text);

  void setTag(const TQString& tag) { _tag = tag; }
  void setCaption(const TQString& caption) { _caption = caption; }
  void setModule(ConfigModule *m) { _module = m; }
  TQString tag() const { return _tag; };
  TQString caption() const { return _caption; };
  TQString icon() const { return _icon; };
  ConfigModule *module() { return _module; };
  void regChildIconWidth(int width);
  int  maxChildIconWidth() { return _maxChildIconWidth; }

  void setPixmap(int column, const TQPixmap& pm);
  void setGroup(const TQString &path);

protected:
  void paintCell( TQPainter * p, const TQColorGroup & cg, int column, int width, int align );

private:
  ConfigModule *_module;
  TQString       _tag;
  TQString       _caption;
  int _maxChildIconWidth;
  TQString       _icon;
};

class ModuleTreeView : public KListView
{
  Q_OBJECT

public:
  ModuleTreeView(ConfigModuleList *list, TQWidget * parent = 0, const char * name = 0);

  void makeSelected(ConfigModule* module);
  void makeVisible(ConfigModule *module);
  void fill();
  TQSize sizeHint() const;

signals:
  void moduleSelected(ConfigModule*);
  void categorySelected(TQListViewItem*);

protected slots:
  void slotItemSelected(TQListViewItem*);

protected:
  void updateItem(ModuleTreeItem *item, ConfigModule* module); 
  void keyPressEvent(TQKeyEvent *);
  void fill(ModuleTreeItem *parent, const TQString &parentPath);

private:
  ConfigModuleList *_modules;
};

#endif
