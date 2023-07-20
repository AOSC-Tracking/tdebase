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

#ifndef KSG_SIGNALPLOTTER_H
#define KSG_SIGNALPLOTTER_H

#include <tqptrlist.h>
#include <tqstring.h>
#include <tqvaluelist.h>
#include <tqwidget.h>

#define GRAPH_POLYGON     0
#define	GRAPH_ORIGINAL    1

class TQColor;

class SignalPlotter : public TQWidget
{
  TQ_OBJECT

  public:
    SignalPlotter( TQWidget *parent = 0, const char *name = 0 );
    ~SignalPlotter();

    bool addBeam( const TQColor &color );
    void addSample( const TQValueList<double> &samples );

    void removeBeam( uint pos );

    void changeRange( int beam, double min, double max );

    TQValueList<TQColor> &beamColors();

    void setTitle( const TQString &title );
    TQString title() const;

    void setUseAutoRange( bool value );
    bool useAutoRange() const;

    void setMinValue( double min );
    double minValue() const;

    void setMaxValue( double max );
    double maxValue() const;

    void setGraphStyle( uint style );
    uint graphStyle() const;

    void setHorizontalScale( uint scale );
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
    void reorderBeams( const TQValueList<int>& newOrder );

    void setThinFrame(bool set) { mShowThinFrame = set; }

  protected:
    void updateDataBuffers();

    virtual void resizeEvent( TQResizeEvent* );
    virtual void paintEvent( TQPaintEvent* );

  private:
    double mMinValue;
    double mMaxValue;
    bool mUseAutoRange;
    bool mShowThinFrame;

    uint mGraphStyle;

    bool mShowVerticalLines;
    TQColor mVerticalLinesColor;
    uint mVerticalLinesDistance;
    bool mVerticalLinesScroll;
    uint mVerticalLinesOffset;
    uint mHorizontalScale;

    bool mShowHorizontalLines;
    TQColor mHorizontalLinesColor;
    uint mHorizontalLinesCount;

    bool mShowLabels;
    bool mShowTopBar;
    uint mFontSize;

    TQColor mBackgroundColor;

    TQPtrList<double> mBeamData;
    TQValueList<TQColor> mBeamColor;

    unsigned int mSamples;

    TQString mTitle;
};

#endif
