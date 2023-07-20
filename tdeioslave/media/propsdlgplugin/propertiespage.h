/*
    Copyright (c) 2004 Jan Schaefer <j_schaef@informatik.uni-kl.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef PROPERTIESPAGE_H
#define PROPERTIESPAGE_H

#include "propertiespagegui.h"
#include <tqmap.h>

class TQCheckBox;
class Medium;

class PropertiesPage : public PropertiesPageGUI
{
  TQ_OBJECT

public:
  PropertiesPage(TQWidget* parent, const TQString &_id);
  virtual ~PropertiesPage();

  bool save();

protected:

  TQMap<TQString,TQString> options;
  TQString id;

};

#endif
