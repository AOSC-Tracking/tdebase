/*
 * klangcombo.cpp - Adds some methods for inserting languages.
 *
 * Copyright (c) 1999-2000 Hans Petter Bieker <bieker@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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

#include <tqiconset.h>

#include <kstandarddirs.h>

#include "klangcombo.h"
#include "klangcombo.moc"

KLanguageCombo::~KLanguageCombo ()
{
}

KLanguageCombo::KLanguageCombo (TQWidget * parent, const char *name)
  : KTagComboBox(parent, name)
{
}

void KLanguageCombo::insertLanguage(const TQString& path, const TQString& name, const TQString& sub, const TQString &submenu, int index)
{
  TQString output = name + TQString::fromLatin1(" (") + path + TQString::fromLatin1(")");
  TQPixmap flag(locate("locale", sub + path + TQString::fromLatin1("/flag.png")));
  insertItem(TQIconSet(flag), output, path, submenu, index);
}

void KLanguageCombo::changeLanguage(const TQString& name, int i)
{
  if (i < 0 || i >= count()) return;
  TQString output = name + TQString::fromLatin1(" (") + tag(i) + TQString::fromLatin1(")");
  changeItem(output, i);
}
