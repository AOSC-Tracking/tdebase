/*
 * applettab.h
 *
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
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
 */


#ifndef __applettab_impl_h__
#define __applettab_impl_h__

#include <tqwidget.h>
#include "applettab.h"

class TQGroupBox;
class TQButtonGroup;
class TQRadioButton;
class TQPushButton;
class TDEListView;
class TQListViewItem;

class AppletTab : public AppletTabBase
{
  TQ_OBJECT

 public:
  AppletTab( TQWidget *parent=0, const char* name=0 );

  void load();
  void load(bool useDefaults);
  void save();
  void defaults();

  TQString quickHelp() const;

 signals:
  void changed();

 protected slots:
  void level_changed(int level);
  void trusted_selection_changed(TQListViewItem *);
  void available_selection_changed(TQListViewItem *);
  void add_clicked();
  void remove_clicked();

 protected:
  void updateTrusted();
  void updateAvailable();
  void updateAddRemoveButton();
  
 private:
  TQStringList   available, l_available, l_trusted;
};

#endif

