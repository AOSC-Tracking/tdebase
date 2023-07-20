/*
    KSysGuard, the KDE System Guard
   
    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>
    
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

#ifndef KSG_DANCINGBARSSETTINGS_H
#define KSG_DANCINGBARSSETTINGS_H

#include <kdialogbase.h>

class KColorButton;
class KDoubleSpinBox;
class KIntNumInput;
class KLineEdit;
class TDEListView;

class TQCheckBox;
class TQListViewItem;
class TQPushButton;

class DancingBarsSettings : public KDialogBase
{
  TQ_OBJECT

  public:
    DancingBarsSettings( TQWidget* parent = 0, const char* name = 0 );
    ~DancingBarsSettings();

    void setTitle( const TQString& title );
    TQString title() const;

    void setMinValue( double min );
    double minValue() const;

    void setMaxValue( double max );
    double maxValue() const;

    void setUseLowerLimit( bool value );
    bool useLowerLimit() const;

    void setLowerLimit( double limit );
    double lowerLimit() const;

    void setUseUpperLimit( bool value );
    bool useUpperLimit() const;

    void setUpperLimit( double limit );
    double upperLimit() const;

    void setForegroundColor( const TQColor &color );
    TQColor foregroundColor() const;

    void setAlarmColor( const TQColor &color );
    TQColor alarmColor() const;

    void setBackgroundColor( const TQColor &color );
    TQColor backgroundColor() const;

    void setFontSize( int size );
    int fontSize() const;

    void setSensors( const TQValueList< TQStringList > &list );
    TQValueList< TQStringList > sensors() const;

  private slots:
    void editSensor();
    void removeSensor();
    void selectionChanged( TQListViewItem* );

  private:
    KColorButton *mForegroundColor;
    KColorButton *mAlarmColor;
    KColorButton *mBackgroundColor;
    KDoubleSpinBox *mMinValue;
    KDoubleSpinBox *mMaxValue;
    KDoubleSpinBox *mLowerLimit;
    KDoubleSpinBox *mUpperLimit;
    KLineEdit *mTitle;
    TDEListView *mSensorView;
    KIntNumInput *mFontSize;

    TQCheckBox *mUseLowerLimit;
    TQCheckBox *mUseUpperLimit;
    TQPushButton *mEditButton;
    TQPushButton *mRemoveButton;
};

#endif
