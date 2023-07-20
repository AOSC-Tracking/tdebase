/*
 * localetime.h
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

#ifndef __KLOCALECONFIGTIME_H__
#define __KLOCALECONFIGTIME_H__

#include <tqwidget.h>

#include <tqmap.h>

class TQCheckBox;
class TQComboBox;

class TDELocale;
class KLanguageCombo;

class StringPair;

class TDELocaleConfigTime : public TQWidget
{
  TQ_OBJECT

public:
  TDELocaleConfigTime( TDELocale *_locale, TQWidget *parent=0, const char *name=0);
  virtual ~TDELocaleConfigTime( );

  void save();

protected:
  void showEvent( TQShowEvent *e );

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
  // Time & dates
  void slotTimeFmtChanged(const TQString &t);
  void slotDateFmtChanged(const TQString &t);
  void slotDateFmtShortChanged(const TQString &t);
  void slotWeekStartDayChanged(int firstDay);
  void slotDateMonthNamePossChanged();
  void slotCalendarSystemChanged(int calendarSystem);

private:
  void updateWeekDayNames();

  TQValueList<StringPair> timeMap() const;
  TQValueList<StringPair> dateMap() const;

  TQString storeToUser(const TQValueList<StringPair> & map,
		      const TQString & storeFormat) const;
  TQString userToStore(const TQValueList<StringPair> & map,
		      const TQString & userFormat) const;
  StringPair buildStringPair(const TQChar &storeName, const TQString &userName) const;

  TDELocale *m_locale;

  // Time & dates
  TQLabel *m_labTimeFmt;
  TQComboBox *m_comboTimeFmt;
  TQLabel *m_labDateFmt;
  TQComboBox * m_comboDateFmt;
  TQLabel *m_labDateFmtShort;
  TQComboBox * m_comboDateFmtShort;
  TQLabel * m_labWeekStartDay;
  TQComboBox * m_comboWeekStartDay;
  TQCheckBox *m_chDateMonthNamePossessive;
  TQLabel * m_labCalendarSystem;
  TQComboBox * m_comboCalendarSystem;
};

#endif
