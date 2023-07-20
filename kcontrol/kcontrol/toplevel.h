/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
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

#ifndef __TOPLEVEL_H__
#define __TOPLEVEL_H__

#include <tdemainwindow.h>
#include <tqlistview.h>


class TQSplitter;
class TQWidgetStack;

class TDEToggleAction;
class TDEAction;

class DockContainer;
class IndexWidget;
class SearchWidget;
class HelpWidget;
class ConfigModule;
class ConfigModuleList;
class ModuleTitle;

class TopLevel : public TDEMainWindow
{
  TQ_OBJECT

public:
  TopLevel( const char* name=0 );
  ~TopLevel();

protected:
  void setupActions();

protected slots:
  void activateModule(ConfigModule *);
  void categorySelected(TQListViewItem *category);
  void newModule(const TQString &name, const TQString& docPath, const TQString &quickhelp);
  void activateIconView();
  void activateTreeView();

  void reportBug();
  void aboutModule();

  void activateSmallIcons();
  void activateMediumIcons();
  void activateLargeIcons();
  void activateHugeIcons();

  void deleteDummyAbout();

  void slotSearchChanged(const TQString &);
  void slotHandbookRequest();
  void slotHelpRequest();

  void changedModule(ConfigModule *changed);

  bool queryClose();

private:

  TQString handleAmpersand( TQString ) const;

  TQSplitter      *_splitter;
  TQWidgetStack   *_stack;
  DockContainer  *_dock;
  ModuleTitle    *_title;

  TDEToggleAction *tree_view, *icon_view;
  TDEToggleAction *icon_small, *icon_medium, *icon_large, *icon_huge;
  TDEAction *report_bug, *about_module;

  IndexWidget  *_index;
  SearchWidget *_search;
  HelpWidget   *_help;

  ConfigModule     *_active;
  ConfigModuleList *_modules;

  /**
   * if someone wants to report a bug
   * against a module with no about data
   * we construct one for him
   **/
  TDEAboutData *dummyAbout;
};

#endif
