/**
 * Copyright (c) 2000 Antonio Larrosa <larrosa@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ICONTHEMES_H
#define ICONTHEMES_H

#include <tqimage.h>
#include <tdecmodule.h>
#include <tqmap.h>
#include <tdelistview.h>

class TQPushButton;
class DeviceManager;
class TQCheckBox;
class TQStringList;


class IconThemesConfig : public TDECModule
{
  TQ_OBJECT

public:
  IconThemesConfig(TQWidget *parent = 0L, const char *name = 0L);
  virtual ~IconThemesConfig();

  void loadThemes();
  bool installThemes(const TQStringList &themes, const TQString &archiveName);
  TQStringList findThemeDirs(const TQString &archiveName);

  void updateRemoveButton();

  void load();
  void save();
  void defaults();

  int buttons();

protected slots:
  void themeSelected(TQListViewItem *item);
  void installNewTheme();
  void removeSelectedTheme();

private:
  TQListViewItem *iconThemeItem(const TQString &name);

  TDEListView *m_iconThemes;
  TQPushButton *m_removeButton;

  TQLabel *m_previewExec;
  TQLabel *m_previewFolder;
  TQLabel *m_previewDocument;
  TQListViewItem *m_defaultTheme;
  TQMap <TQString, TQString>m_themeNames;
  bool m_bChanged;
};

#endif // ICONTHEMES_H

