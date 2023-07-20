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

#ifndef KSG_STYLESETTINGS_H
#define KSG_STYLESETTINGS_H

#include <kdialogbase.h>

#include <tqcolor.h>

class KColorButton;

class TQListBoxItem;
class TQPushButton;

class StyleSettings : public KDialogBase
{
  TQ_OBJECT

  public:
    StyleSettings( TQWidget *parent = 0, const char *name = 0 );
    ~StyleSettings();

    void setFirstForegroundColor( const TQColor &color );
    TQColor firstForegroundColor() const;

    void setSecondForegroundColor( const TQColor &color );
    TQColor secondForegroundColor() const;

    void setAlarmColor( const TQColor &color );
    TQColor alarmColor() const;

    void setBackgroundColor( const TQColor &color );
    TQColor backgroundColor() const;

    void setFontSize( uint size );
    uint fontSize() const;

    void setSensorColors( const TQValueList<TQColor> &list );
    TQValueList<TQColor> sensorColors();

  private slots:
    void editSensorColor();
    void selectionChanged( TQListBoxItem* );

  private:
    KColorButton *mFirstForegroundColor;
    KColorButton *mSecondForegroundColor;
    KColorButton *mAlarmColor;
    KColorButton *mBackgroundColor;

    TQSpinBox *mFontSize;

    TQListBox *mColorListBox;
    TQPushButton *mEditColorButton;
};

#endif
