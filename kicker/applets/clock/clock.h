/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef __CLOCK_H
#define __CLOCK_H

#include <tqlcdnumber.h>
#include <tqlabel.h>
#include <tqtoolbutton.h>
#include <tqguardedptr.h>
#include <tqdatetime.h>
#include <tqvbox.h>
#include <tqstringlist.h>
#include <tqtooltip.h>
#include <tqevent.h>

#include <dcopobject.h>
#include <kpanelapplet.h>
#include <kdirwatch.h>
#include <tdeconfigdialog.h>

#include <kickertip.h>
#include "settings.h"
#include "kshadowengine.h"

class TQTimer;
class TQBoxLayout;
class DatePicker;
class TQPixmap;
class Zone;
class TDEPopupMenu;
class Prefs;
class ClockApplet;

namespace TDEIO
{
  class Job;
}

class DigitalWidget;
class AnalogWidget;
class FuzzyWidget;
class ClockApplet;
class TDEConfigDialogManager;
class SettingsWidgetImp;

class SettingsWidgetImp : public SettingsWidget
{
    TQ_OBJECT
    
    public:
        SettingsWidgetImp(Prefs *p=0,
                Zone *z=0,
                TQWidget* parent=0,
                const char* name=0,
                WFlags fl=0);
    public slots:
        void OkApply();
    
    private:
        Prefs *prefs;
        Zone *zone;
};

class TDEConfigDialogSingle : public TDEConfigDialog
{
    TQ_OBJECT
    
    public:
        TDEConfigDialogSingle(Zone *zone,
                TQWidget *parent=0,
                const char *name=0,
                Prefs *prefs=0,
                KDialogBase::DialogType dialogType = KDialogBase::IconList,
                bool modal=false);
    
        SettingsWidgetImp* settings;
    
        void updateSettings();
        void updateWidgets();
        void updateWidgetsDefault();

    protected slots:
        void selectPage(int p);
        void dateToggled();
    
    private:
        DigitalWidget *digitalPage;
        AnalogWidget *analogPage;
        FuzzyWidget *fuzzyPage;
        Prefs *_prefs;
};

/**
 * Base class for all clock types
 */
class ClockWidget
{
    public:
        ClockWidget(ClockApplet *applet, Prefs *prefs);
        virtual ~ClockWidget();
        
        virtual TQWidget* widget()=0;
        virtual int preferedWidthForHeight(int h) const =0;
        virtual int preferedHeightForWidth(int w) const =0;
        virtual void updateClock()=0;
        virtual void forceUpdate() { _force = true; updateClock(); }
        virtual void loadSettings()=0;
        virtual bool showDate()=0;
        virtual bool showDayOfWeek()=0;
    
    protected:
        ClockApplet *_applet;
        Prefs *_prefs;
        TQTime _time;
        bool _force;
};


class PlainClock : public TQLabel, public ClockWidget
{
    TQ_OBJECT

    public:
        PlainClock(ClockApplet *applet, Prefs *prefs, TQWidget *parent=0, const char *name=0);
        
        TQWidget* widget()    { return this; }
        int preferedWidthForHeight(int h) const;
        int preferedHeightForWidth(int w) const;
        void updateClock();
        void loadSettings();
        bool showDate();
        bool showDayOfWeek();
    
    protected:
        void paintEvent(TQPaintEvent *e);
        void drawContents(TQPainter *p);
    
        TQString _timeStr;
};


class DigitalClock : public TQLCDNumber, public ClockWidget
{
    TQ_OBJECT

    public:
        DigitalClock(ClockApplet *applet, Prefs *prefs, TQWidget *parent=0, const char *name=0);
        ~DigitalClock();
    
        TQWidget* widget()    { return this; }
        int preferedWidthForHeight(int h) const;
        int preferedHeightForWidth(int w) const;
        void updateClock();
        void loadSettings();
        bool showDate();
        bool showDayOfWeek();
    
    protected:
        void paintEvent( TQPaintEvent*);
        void drawContents( TQPainter * p);
        void resizeEvent ( TQResizeEvent *ev);
        
        TQPixmap *_buffer;
        TQString _timeStr;
        TQPixmap lcdPattern;
};


class AnalogClock : public TQFrame, public ClockWidget
{
    TQ_OBJECT

    public:
        AnalogClock(ClockApplet *applet, Prefs *prefs, TQWidget *parent=0, const char *name=0);
        ~AnalogClock();
    
        TQWidget* widget()                        { return this; }
        int preferedWidthForHeight(int h) const  { return h; }
        int preferedHeightForWidth(int w) const  { return w; }
        void updateClock();
        void loadSettings();
        bool showDate();
        bool showDayOfWeek();
    
    protected:
        virtual void paintEvent(TQPaintEvent *);
        void styleChange(TQStyle&);
        void initBackgroundPixmap();
        
        TQPixmap *_spPx;
        TQPixmap lcdPattern;
        int _bgScale;
};


class FuzzyClock : public TQFrame, public ClockWidget
{
    TQ_OBJECT

    public:
        FuzzyClock(ClockApplet *applet, Prefs* prefs, TQWidget *parent=0, const char *name=0);
        
        TQWidget* widget()    { return this; }
        int preferedWidthForHeight(int h) const;
        int preferedHeightForWidth(int w) const;
        void updateClock();
        void loadSettings();
        bool showDate();
        bool showDayOfWeek();

    public slots:
        void deleteMyself();

    protected:
        virtual void drawContents(TQPainter *p);
        
        TQStringList hourNames;
        TQStringList normalFuzzy;
        TQStringList normalFuzzyOne;
        TQStringList dayTime;
        
        TQString _timeStr;

    private:
        bool alreadyDrawing;
};

class ClockAppletToolTip : public TQToolTip
{
    public:
        ClockAppletToolTip( ClockApplet* clock );

    protected:
        virtual void maybeTip( const TQPoint & );

    private:
        ClockApplet *m_clock;
};

class ClockApplet : public KPanelApplet, public KickerTip::Client, public DCOPObject
{
  TQ_OBJECT
  K_DCOP

  friend class ClockAppletToolTip;

    public:
        ClockApplet(const TQString& configFile, Type t = Normal, int actions = 0,
                    TQWidget *parent = 0, const char *name = 0);
        ~ClockApplet();
        
        int widthForHeight(int h) const;
        int heightForWidth(int w) const;
        void preferences();
        void preferences(bool timezone);
        int type();
        Orientation getOrientation()    { return orientation(); }
        void resizeRequest()            { emit(updateLayout()); }
        const Zone* timezones()            { return zone; }
        
        TQTime clockGetTime();
        TQDate clockGetDate();

        virtual void updateKickerTip(KickerTip::Data&);
    
        KTextShadowEngine *shadowEngine();

    k_dcop:
        void reconfigure();

    protected slots:
        void slotReconfigure() { reconfigure(); emit clockReconfigured(); }
        void slotUpdate();
        void slotCalendarDeleted();
        void slotEnableCalendar();
        void slotCopyMenuActivated( int id );
        void contextMenuActivated(int result);
        void aboutToShowContextMenu();
        void fixupLayout();
        void globalPaletteChange();
        void setTimerTo60();

    signals:
        void clockReconfigured();

    protected:
        void toggleCalendar();
        void openContextMenu();
        void updateDateLabel(bool reLayout = true);
        void showZone(int z);
        void nextZone();
        void prevZone();
        void updateFollowBackground();
        
        void paletteChange(const TQPalette &) { setBackground(); }
        void setBackground();
        void mousePressEvent(TQMouseEvent *ev);
        void wheelEvent(TQWheelEvent* e);
        bool eventFilter(TQObject *, TQEvent *);

        virtual void positionChange(Position p);

        TQCString configFileName;
        DatePicker *_calendar;
        bool _disableCalendar;
        ClockWidget *_clock;
        TQLabel *_date;
        TQLabel *_dayOfWeek;
        TQDate _lastDate;
        TQTimer *_timer;
        TQTimer *m_layoutTimer;
        int m_layoutDelay;
        bool m_followBackgroundSetting;
        bool m_dateFollowBackgroundSetting;
        int TZoffset;

        // settings
        Prefs *_prefs;
        Zone *zone;
        bool showDate;
        bool showDayOfWeek;
        bool m_updateOnTheMinute;
        TQStringList _remotezonelist;
        TDEPopupMenu* menu;
        ClockAppletToolTip m_tooltip;
        KTextShadowEngine *m_shadowEngine;
};


#endif
