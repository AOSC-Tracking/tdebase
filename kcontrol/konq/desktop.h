/**
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __desktop_h__
#define __desktop_h__

#include <tdecmodule.h>

class TQSpinBox;
class TQLabel;
class TQCheckBox;
class KLineEdit;
class KIntNumInput;

// if you change this, update also the number of keyboard shortcuts in twin/twinbindings.cpp
static const int maxDesktops = 20;

class KDesktopConfig : public TDECModule
{
  TQ_OBJECT

 public:
  KDesktopConfig(TQWidget *parent = 0L, const char *name = 0L);

  void load();
  void load( bool useDefaults );
  void save();
  void defaults();

 protected slots:
  void slotValueChanged(int);

 private:
  KIntNumInput *_numInput;
  TQLabel       *_nameLabel[maxDesktops];
  KLineEdit    *_nameInput[maxDesktops];
  TQCheckBox    *_wheelOption;
  bool         _wheelOptionImmutable;
  bool         _labelImmutable[maxDesktops];
};

#endif
