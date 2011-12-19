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

*/

#ifndef KSG_FANCYPLOTTER_H
#define KSG_FANCYPLOTTER_H

#include <kdialogbase.h>

#include <SensorDisplay.h>

#include "SignalPlotter.h"

class TQListViewItem;
class FancyPlotterSettings;

class FPSensorProperties : public KSGRD::SensorProperties
{
  public:
    FPSensorProperties();
    FPSensorProperties( const TQString &hostName, const TQString &name,
                        const TQString &type, const TQString &description,
                        const TQColor &color );
    ~FPSensorProperties();

    void setColor( const TQColor &color );
    TQColor color() const;

  private:
    TQColor mColor;
};

class FancyPlotter : public KSGRD::SensorDisplay
{
  Q_OBJECT

  public:
    FancyPlotter( TQWidget* parent = 0, const char* name = 0,
                  const TQString& title = TQString::null, double min = 0,
                  double max = 100, bool noFrame = false, bool isApplet = false );
    virtual ~FancyPlotter();

    void configureSettings();

    bool addSensor( const TQString &hostName, const TQString &name,
                    const TQString &type, const TQString &title );
    bool addSensor( const TQString &hostName, const TQString &name,
                    const TQString &type, const TQString &title,
                    const TQColor &color );

    bool removeSensor( uint pos );

    virtual TQSize sizeHint(void);

    virtual void answerReceived( int id, const TQString &answer );

    virtual bool restoreSettings( TQDomElement &element );
    virtual bool saveSettings( TQDomDocument &doc, TQDomElement &element,
                               bool save = true );

    virtual bool hasSettingsDialog() const;

  public slots:
    void applySettings();
    virtual void applyStyle();
    void killDialog();

  protected:
    virtual void resizeEvent( TQResizeEvent* );

  private:
    uint mBeams;

    SignalPlotter* mPlotter;

    FancyPlotterSettings* mSettingsDialog;

    /**
      The sample buffer and the flags are needed to store the incoming
      samples for each beam until all samples of the period have been
      received. The flags variable is used to ensure that all samples have
      been received.
     */
    TQValueList<double> mSampleBuf;
};

#endif
