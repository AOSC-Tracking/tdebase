/*
 *  dtime.h
 *
 *  Copyright (C) 1998 Luca Montecchiani <m.luca@usa.net>
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
 *
 */

#ifndef dtime_included
#define dtime_included

#include <tqapplication.h>
#include <tqdatetime.h>
#include <tqlineedit.h>
#include <tqspinbox.h>
#include <tqstring.h>
#include <tqtimer.h>
#include <tqvalidator.h>
#include <tqwidget.h>
#include <tqcheckbox.h>

#include <kdatepicker.h>
#include <knuminput.h>

class Kclock;

class HMSTimeWidget : public KIntSpinBox
{
  TQ_OBJECT
 public:
  HMSTimeWidget(TQWidget *parent=0, const char *name=0);
 protected:
  TQString mapValueToText(int);
};

class Dtime : public TQWidget
{
  TQ_OBJECT
 public:
  Dtime( TQWidget *parent=0, const char* name=0 );

  void	save();
  void	load();

  TQString quickHelp() const;

signals:
  void	timeChanged(bool);

 private slots:
  void	configChanged();
  void	serverTimeCheck(); 
  void	timeout();
  void	set_time();
  void	changeDate(TQDate);

private:
  void	findNTPutility();
  TQString	ntpUtility;

  TQWidget*	privateLayoutWidget;
  TQCheckBox	*setDateTimeAuto;
  TQComboBox	*timeServerList;

  KDatePicker	*cal;
  TQComboBox	*month;
  TQSpinBox	*year;

  HMSTimeWidget	*hour;
  HMSTimeWidget	*minute;
  HMSTimeWidget	*second;

  Kclock	*kclock;
  
  TQTime	time;
  TQDate	date;
  TQTimer	internalTimer;
  
  TQString	BufS;
  int		BufI;
  bool		refresh;
  bool		ontimeout;
};

class Kclock : public TQWidget
{
  TQ_OBJECT

public:
  Kclock( TQWidget *parent=0, const char *name=0 ) 
    : TQWidget(parent, name) {};
  
  void setTime(const TQTime&);
  
protected:
  virtual void	paintEvent( TQPaintEvent *event );
  
  
private:
  TQTime	time;
};

class KStrictIntValidator : public TQIntValidator 
{
public:
  KStrictIntValidator(int bottom, int top, TQWidget * parent,
		      const char * name = 0 )
    : TQIntValidator(bottom, top, parent, name) {};
  
  TQValidator::State validate( TQString & input, int & d ) const; 
};

#endif // dtime_included
