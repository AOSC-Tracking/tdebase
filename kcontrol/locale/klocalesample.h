/*
 * locale.cpp
 *
 * Copyright (c) 1998 Matthias Hoelzer (hoelzer@physik.uni-wuerzburg.de)
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

#ifndef __KLOCALESAMPLE_H__
#define __KLOCALESAMPLE_H__

#include <tqwidget.h>

class TQLabel;
class TQResizeEvent;

class TDELocale;

class TDELocaleSample : public TQWidget
{
  TQ_OBJECT
public:
  TDELocaleSample(TDELocale *_locale,
                TQWidget *parent = 0, const char*name = 0);
  virtual ~TDELocaleSample();

public slots:
  void slotLocaleChanged();

protected slots:
  void slotUpdateTime();

private:
  TDELocale *m_locale;
  TQLabel *m_numberSample, *m_labNumber;
  TQLabel *m_moneySample, *m_labMoney;
  TQLabel *m_timeSample, *m_labTime;
  TQLabel *m_dateSample, *m_labDate;
  TQLabel *m_dateShortSample, *m_labDateShort;
};

#endif
