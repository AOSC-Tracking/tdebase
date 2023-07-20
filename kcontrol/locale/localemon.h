/*
 * localemon.h
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


#ifndef __KLOCALECONFIGMON_H__
#define __KLOCALECONFIGMON_H__

#include <tqwidget.h>

class TQCheckBox;
class TQComboBox;
class TQLineEdit;

class KIntNumInput;
class TDELocale;
class KLanguageCombo;

class TDELocaleConfigMoney : public TQWidget
{
  TQ_OBJECT

public:
  TDELocaleConfigMoney(TDELocale *locale, TQWidget *parent = 0, const char *name = 0);
  virtual ~TDELocaleConfigMoney();

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
  // Money
  void slotMonCurSymChanged(const TQString &t);
  void slotMonDecSymChanged(const TQString &t);
  void slotMonThoSepChanged(const TQString &t);
  void slotMonFraDigChanged(int value);
  void slotMonPosPreCurSymChanged();
  void slotMonNegPreCurSymChanged();
  void slotMonPosMonSignPosChanged(int i);
  void slotMonNegMonSignPosChanged(int i);

private:
  TDELocale *m_locale;

  // Money
  TQLabel *m_labMonCurSym;
  TQLineEdit *m_edMonCurSym;
  TQLabel *m_labMonDecSym;
  TQLineEdit *m_edMonDecSym;
  TQLabel *m_labMonThoSep;
  TQLineEdit *m_edMonThoSep;
  TQLabel *m_labMonFraDig;
  KIntNumInput * m_inMonFraDig;

  TQCheckBox *m_chMonPosPreCurSym;
  TQCheckBox *m_chMonNegPreCurSym;
  TQLabel *m_labMonPosMonSignPos;
  TQComboBox *m_cmbMonPosMonSignPos;
  TQLabel *m_labMonNegMonSignPos;
  TQComboBox *m_cmbMonNegMonSignPos;
};

#endif
