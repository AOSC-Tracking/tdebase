/*
 * localenum.h
 *
 * Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>
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


#ifndef __KLOCALECONFIGNUM_H__
#define __KLOCALECONFIGNUM_H__

#include <tqwidget.h>

class TQCheckBox;
class TQComboBox;
class TQLineEdit;

class TDELocale;
class KLanguageCombo;

class TDELocaleConfigNumber : public TQWidget
{
  TQ_OBJECT

public:
  TDELocaleConfigNumber( TDELocale *_locale,
		       TQWidget *parent=0, const char *name=0);
  virtual ~TDELocaleConfigNumber( );

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
  // Numbers
  void slotMonPosSignChanged(const TQString &t);
  void slotMonNegSignChanged(const TQString &t);
  void slotDecSymChanged(const TQString &t);
  void slotThoSepChanged(const TQString &t);

private:
  TDELocale *m_locale;

  // Numbers
  TQLabel *m_labDecSym;
  TQLineEdit *m_edDecSym;
  TQLabel *m_labThoSep;
  TQLineEdit *m_edThoSep;
  TQLabel *m_labMonPosSign;
  TQLineEdit *m_edMonPosSign;
  TQLabel *m_labMonNegSign;
  TQLineEdit *m_edMonNegSign;
};

#endif
