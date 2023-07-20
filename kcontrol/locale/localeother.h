/*
 * localeother.h
 *
 * Copyright (c) 2001-2003 Hans Petter Bieker <bieker@kde.org>
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

#ifndef __KLOCALECONFIGOTHER_H__
#define __KLOCALECONFIGOTHER_H__

#include <tqwidget.h>

class TQLabel;
class TQComboBox;

class TDELocale;

class TDELocaleConfigOther : public TQWidget
{
  TQ_OBJECT

public:
  TDELocaleConfigOther(TDELocale *locale, TQWidget *parent = 0, const char *name = 0);
  virtual ~TDELocaleConfigOther();

  void save();

public slots:
  /**
   * Loads all settings from the current locale into the current widget.
   */
  void slotLocaleChanged();
  /**
   * Retranslate all objects owned by this object using the current locale.
   */
  void slotTranslate();

signals:
  void localeChanged();

private slots:
  void slotPageSizeChanged(int i);
  void slotMeasureSystemChanged(int i);

private:
  TDELocale *m_locale;

  TQLabel *m_labMeasureSystem;
  TQComboBox *m_combMeasureSystem;
  TQLabel *m_labPageSize;
  TQComboBox *m_combPageSize;
};

#endif
