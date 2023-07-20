/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org>
 
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

#ifndef __searchwidget_h__
#define __searchwidget_h__

#include <tqwidget.h>
#include <tqptrlist.h>
#include <tqstring.h>
#include <tqstringlist.h>

#include "modules.h"

class TDEListBox;
class KLineEdit;
class TQListBoxItem;

class KeywordListEntry
{
 public:
  KeywordListEntry(const TQString& name, ConfigModule* module);
  
  void addModule(ConfigModule* module);

  TQString moduleName() { return _name; }
  TQPtrList<ConfigModule> modules() { return _modules; }
  
 private:
  TQString _name;
  TQPtrList<ConfigModule> _modules;
  
};

class SearchWidget : public TQWidget
{  
  TQ_OBJECT    
  
public:   
  SearchWidget(TQWidget *parent, const char *name=0);

  void populateKeywordList(ConfigModuleList *list);
  void searchTextChanged(const TQString &);

signals:
  void moduleSelected(ConfigModule *);

protected:
  void populateKeyListBox(const TQString& regexp);
  void populateResultListBox(const TQString& keyword);

protected slots:
  void slotKeywordSelected(const TQString &);
  void slotModuleSelected(TQListBoxItem *item);
  void slotModuleClicked(TQListBoxItem *item);

private:
  TDEListBox  *_keyList, *_resultList;
  TQPtrList<KeywordListEntry> _keywords;
};

#endif
