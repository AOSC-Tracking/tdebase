/**
 * kdatetimedlg.cpp
 *
 * Copyright (c) 2001 George Staikos <staikos@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kdatetimedlg.h"
#include <tqlayout.h>
#include <tqlabel.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <kdatepicker.h>
#include <kpushbutton.h>
#include <knuminput.h>
#include <kstdguiitem.h>

KDateTimeDlg::KDateTimeDlg(TQWidget *parent, const char *name)
                             : KDialog(parent, name, true) {
TQGridLayout *grid = new TQGridLayout(this, 9, 6, marginHint(), spacingHint());

   setCaption(i18n("Date & Time Selector"));

   _date = new KDatePicker(this);
   grid->addMultiCellWidget(_date, 0, 5, 0, 5);

   grid->addWidget(new TQLabel(i18n("Hour:"), this), 7, 0);
   _hours = new KIntNumInput(this);
   _hours->setRange(0, 23, 1, false);
   grid->addWidget(_hours, 7, 1);

   grid->addWidget(new TQLabel(i18n("Minute:"), this), 7, 2);
   _mins = new KIntNumInput(this);
   _mins->setRange(0, 59, 1, false);
   grid->addWidget(_mins, 7, 3);

   grid->addWidget(new TQLabel(i18n("Second:"), this), 7, 4);
   _secs = new KIntNumInput(this);
   _secs->setRange(0, 59, 1, false);
   grid->addWidget(_secs, 7, 5);

   _ok = new KPushButton(KStdGuiItem::ok(), this);
   grid->addWidget(_ok, 8, 4);
   connect(_ok, TQT_SIGNAL(clicked()), TQT_SLOT(accept()));

   _cancel = new KPushButton(KStdGuiItem::cancel(), this);
   grid->addWidget(_cancel, 8, 5);
   connect(_cancel, TQT_SIGNAL(clicked()), TQT_SLOT(reject()));

}


KDateTimeDlg::~KDateTimeDlg() {

}


TQDate KDateTimeDlg::getDate() {
   return _date->date();
}


TQTime KDateTimeDlg::getTime() {
TQTime rc(_hours->value(), _mins->value(), _secs->value());
return rc;
}


TQDateTime KDateTimeDlg::getDateTime() {
TQDateTime qdt;
TQTime qtime(_hours->value(), _mins->value(), _secs->value());

   qdt.setDate(_date->date());
   qdt.setTime(qtime);

return qdt;
}


void KDateTimeDlg::setDate(const TQDate& qdate) {
   _date->setDate(qdate);
}


void KDateTimeDlg::setTime(const TQTime& qtime) {
   _hours->setValue(qtime.hour());
   _mins->setValue(qtime.minute());
   _secs->setValue(qtime.second());
}


void KDateTimeDlg::setDateTime(const TQDateTime& qdatetime) {
   _date->setDate(qdatetime.date());
   _hours->setValue(qdatetime.time().hour());
   _mins->setValue(qdatetime.time().minute());
   _secs->setValue(qdatetime.time().second());
}


#include "kdatetimedlg.moc"

