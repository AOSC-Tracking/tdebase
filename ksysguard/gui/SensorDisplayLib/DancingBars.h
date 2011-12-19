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

#ifndef KSG_DANCINGBARS_H
#define KSG_DANCINGBARS_H

#include <SensorDisplay.h>
#include <tqbitarray.h>

class KIntNumInput;

class TQGroupBox;
class TQLineEdit;
class TQListViewItem;

class BarGraph;
class DancingBarsSettings;

class DancingBars : public KSGRD::SensorDisplay
{
  Q_OBJECT

  public:
    DancingBars( TQWidget *parent = 0, const char *name = 0,
                 const TQString &title = TQString::null, int min = 0,
                 int max = 100, bool noFrame = false, bool isApplet = false );
    virtual ~DancingBars();

    void configureSettings();

    bool addSensor( const TQString &hostName, const TQString &name,
                    const TQString &type, const TQString &title );
    bool removeSensor( uint pos );

    void updateSamples( const TQMemArray<double> &samples );

    virtual TQSize sizeHint();

    virtual void answerReceived( int id, const TQString &answer );

    bool restoreSettings( TQDomElement& );
    bool saveSettings( TQDomDocument&, TQDomElement&, bool save = true );

    virtual bool hasSettingsDialog() const;

  public slots:
    void applySettings();
    virtual void applyStyle();

  protected:
    virtual void resizeEvent( TQResizeEvent* );

  private:
    uint mBars;

    BarGraph* mPlotter;

    DancingBarsSettings* mSettingsDialog;

    /**
      The sample buffer and the flags are needed to store the incoming
      samples for each beam until all samples of the period have been
      received. The flags variable is used to ensure that all samples have
      been received.
     */
    TQMemArray<double> mSampleBuffer;
    TQBitArray mFlags;
};

#endif
