/*
 * klocalesample.cpp
 *
 * Copyright (c) 1998 Matthias Hoelzer (hoelzer@physik.uni-wuerzburg.de)
 * Copyright (c) 1999 Preston Brown <pbrown@kde.org>
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

#include <tqdatetime.h>
#include <tqlabel.h>
#include <tqwhatsthis.h>
#include <tqlayout.h>
#include <tqtimer.h>

#include <stdio.h>

#include <klocale.h>

#include "klocalesample.h"
#include "klocalesample.moc"

KLocaleSample::KLocaleSample(KLocale *locale,
                             TQWidget *parent, const char*name)
  : TQWidget(parent, name),
    m_locale(locale)
{
  TQGridLayout *lay = new TQGridLayout(this, 5, 2);
  lay->setAutoAdd(TRUE);

  // Whatever the color scheme is, we want black text
  TQColorGroup a = palette().active();
  a.setColor(TQColorGroup::Foreground, Qt::black);
  TQPalette pal(a, a, a);

  m_labNumber = new TQLabel(this, I18N_NOOP("Numbers:"));
  m_labNumber->setPalette(pal);
  m_numberSample = new TQLabel(this);
  m_numberSample->setPalette(pal);

  m_labMoney = new TQLabel(this, I18N_NOOP("Money:"));
  m_labMoney->setPalette(pal);
  m_moneySample = new TQLabel(this);
  m_moneySample->setPalette(pal);

  m_labDate = new TQLabel(this, I18N_NOOP("Date:"));
  m_labDate->setPalette(pal);
  m_dateSample = new TQLabel(this);
  m_dateSample->setPalette(pal);

  m_labDateShort = new TQLabel(this, I18N_NOOP("Short date:"));
  m_labDateShort->setPalette(pal);
  m_dateShortSample = new TQLabel(this);
  m_dateShortSample->setPalette(pal);

  m_labTime = new TQLabel(this, I18N_NOOP("Time:"));
  m_labTime->setPalette(pal);
  m_timeSample = new TQLabel(this);
  m_timeSample->setPalette(pal);

  lay->setColStretch(0, 1);
  lay->setColStretch(1, 3);

  TQTimer *timer = new TQTimer(this, "clock_timer");
  connect(timer, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotUpdateTime()));
  timer->start(1000);
}

KLocaleSample::~KLocaleSample()
{
}

void KLocaleSample::slotUpdateTime()
{
  TQDateTime dt = TQDateTime::currentDateTime();

  m_dateSample->setText(m_locale->formatDate(TQT_TQDATE_OBJECT(dt.date()), false));
  m_dateShortSample->setText(m_locale->formatDate(TQT_TQDATE_OBJECT(dt.date()), true));
  m_timeSample->setText(m_locale->formatTime(TQT_TQTIME_OBJECT(dt.time()), true));
}

void KLocaleSample::slotLocaleChanged()
{
  m_numberSample->setText(m_locale->formatNumber(1234567.89) +
                          TQString::fromLatin1(" / ") +
                          m_locale->formatNumber(-1234567.89));

  m_moneySample->setText(m_locale->formatMoney(123456789.00) +
                         TQString::fromLatin1(" / ") +
                         m_locale->formatMoney(-123456789.00));

  slotUpdateTime();

  TQString str;

  str = m_locale->translate("This is how numbers will be displayed.");
  TQWhatsThis::add( m_labNumber,  str );
  TQWhatsThis::add( m_numberSample, str );

  str = m_locale->translate("This is how monetary values will be displayed.");
  TQWhatsThis::add( m_labMoney,    str );
  TQWhatsThis::add( m_moneySample, str );

  str = m_locale->translate("This is how date values will be displayed.");
  TQWhatsThis::add( m_labDate,    str );
  TQWhatsThis::add( m_dateSample, str );

  str = m_locale->translate("This is how date values will be displayed using "
                            "a short notation.");
  TQWhatsThis::add( m_labDateShort, str );
  TQWhatsThis::add( m_dateShortSample, str );

  str = m_locale->translate("This is how the time will be displayed.");
  TQWhatsThis::add( m_labTime,    str );
  TQWhatsThis::add( m_timeSample, str );
}
