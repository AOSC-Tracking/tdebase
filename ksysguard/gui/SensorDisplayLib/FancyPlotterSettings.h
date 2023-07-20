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

#ifndef FANCYPLOTTERSETTINGS_H
#define FANCYPLOTTERSETTINGS_H

#include <kdialogbase.h>

class KColorButton;
class KIntNumInput;
class KLineEdit;
class TDEListView;

class TQCheckBox;
class TQListViewItem;
class TQPushButton;
class TQRadioButton;

class FancyPlotterSettings : public KDialogBase
{
  TQ_OBJECT

  public:
    FancyPlotterSettings( TQWidget* parent = 0, const char* name = 0 );
    ~FancyPlotterSettings();

    void setTitle( const TQString &title );
    TQString title() const;

    void setUseAutoRange( bool value );
    bool useAutoRange() const;

    void setMinValue( double min );
    double minValue() const;

    void setMaxValue( double max );
    double maxValue() const;

    void setUsePolygonStyle( bool value );
    bool usePolygonStyle() const;

    void setHorizontalScale( int scale );
    int horizontalScale() const;

    void setShowVerticalLines( bool value );
    bool showVerticalLines() const;

    void setVerticalLinesColor( const TQColor &color );
    TQColor verticalLinesColor() const;

    void setVerticalLinesDistance( int distance );
    int verticalLinesDistance() const;

    void setVerticalLinesScroll( bool value );
    bool verticalLinesScroll() const;

    void setShowHorizontalLines( bool value );
    bool showHorizontalLines() const;

    void setHorizontalLinesColor( const TQColor &color );
    TQColor horizontalLinesColor() const;

    void setHorizontalLinesCount( int count );
    int horizontalLinesCount() const;

    void setShowLabels( bool value );
    bool showLabels() const;

    void setShowTopBar( bool value );
    bool showTopBar() const;

    void setFontSize( int size );
    int fontSize() const;

    void setBackgroundColor( const TQColor &color );
    TQColor backgroundColor() const;

    void setSensors( const TQValueList< TQStringList > &list );
    TQValueList< TQStringList > sensors() const;
    TQValueList<int> order() const;
    TQValueList<int> deleted() const;
    void clearDeleted();
    void resetOrder();

  private slots:
    void editSensor();
    void removeSensor();
    void moveUpSensor();
    void moveDownSensor();
    void selectionChanged( TQListViewItem* );

  private:

    KColorButton *mVerticalLinesColor;
    KColorButton *mHorizontalLinesColor;
    KColorButton *mBackgroundColor;
    KLineEdit *mMinValue;
    KLineEdit *mMaxValue;
    KLineEdit *mTitle;
    KIntNumInput *mHorizontalScale;
    KIntNumInput *mVerticalLinesDistance;
    KIntNumInput *mHorizontalLinesCount;
    KIntNumInput *mFontSize;
    TDEListView *mSensorView;

    TQCheckBox *mShowVerticalLines;
    TQCheckBox *mShowHorizontalLines;
    TQCheckBox *mVerticalLinesScroll;
    TQCheckBox *mUseAutoRange;
    TQCheckBox *mShowLabels;
    TQCheckBox *mShowTopBar;
    TQPushButton *mEditButton;
    TQPushButton *mRemoveButton;
    TQPushButton *mMoveUpButton;
    TQPushButton *mMoveDownButton;
    TQRadioButton *mUsePolygonStyle;
    TQRadioButton *mUseOriginalStyle;

    /** The numbers of the sensors to be delete.*/
    TQValueList<int> mDeleted;
};

#endif
