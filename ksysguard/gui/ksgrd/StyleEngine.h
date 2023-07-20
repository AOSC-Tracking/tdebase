/*
    KSysGuard, the KDE System Guard
   
    Copyright (c) 1999 - 2001 Chris Schlaeger <cs@kde.org>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
    Please do not commit any changes without consulting me first. Thanks!

*/

#ifndef KSG_STYLEENGINE_H
#define KSG_STYLEENGINE_H

#include <tqcolor.h>
#include <tqobject.h>
#include <tqptrlist.h>

#include <kdemacros.h>

class TDEConfig;

class TQListBoxItem;

class StyleSettings;

namespace KSGRD {

class KDE_EXPORT StyleEngine : public QObject
{
  TQ_OBJECT

  public:
    StyleEngine();
    ~StyleEngine();

    void readProperties( TDEConfig* );
    void saveProperties( TDEConfig* );

    const TQColor& firstForegroundColor() const;
    const TQColor& secondForegroundColor() const;
    const TQColor& alarmColor() const;
    const TQColor& backgroundColor() const;

    uint fontSize() const;

    const TQColor& sensorColor( uint pos );
    uint numSensorColors() const;

  public slots:
    void configure();
    void applyToWorksheet();

  signals:
	  void applyStyleToWorksheet();

  private:
    void apply();

    TQColor mFirstForegroundColor;
    TQColor mSecondForegroundColor;
    TQColor mAlarmColor;
    TQColor mBackgroundColor;
    uint mFontSize;
    TQValueList<TQColor> mSensorColors;

    StyleSettings *mSettingsDialog;
};

KDE_EXPORT extern StyleEngine* Style;

}

#endif
