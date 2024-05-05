/*****************************************************************

Copyright (c) 2004 Zack Rusin <zrusin@kde.org>
                   Sami Kyostil <skyostil@kempele.fi>
                   Aaron J. Seigo <aseigo@kde.org>

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

#ifndef KICKER_TIP_H
#define KICKER_TIP_H

#include <tqbitmap.h>
#include <tqpixmap.h>
#include <tqtimer.h>
#include <tqwidget.h>

#include <kpanelapplet.h>

class TQMimeSourceFactory;
class TQPaintEvent;
class TQSimpleRichText;
class TQTimer;

class TDE_EXPORT KickerTip : public TQWidget
{
    TQ_OBJECT

public:
    enum MaskEffect { Plain, Dissolve };

    struct Data
    {
            TQString message;
            TQString subtext;
            TQPixmap icon;
            KickerTip::MaskEffect maskEffect;
            int duration;
            KPanelApplet::Direction direction;

            // do NOT delete this in the client!
            TQMimeSourceFactory* mimeFactory;
    };

    class TDE_EXPORT Client
    {
        public:
            virtual void updateKickerTip(KickerTip::Data&) = 0;
            void updateKickerTip() const;
    };

    static KickerTip* the();
    static void enableTipping(bool tip);
    static bool tippingEnabled();

    void untipFor(const TQWidget* w);
    bool eventFilter(TQObject *o, TQEvent *e);

protected:
    KickerTip(TQWidget * parent);
    ~KickerTip();

    void paintEvent(TQPaintEvent * e);
    void mousePressEvent(TQMouseEvent * e);

    void plainMask();
    void dissolveMask();

    void displayInternal();
    void hide();

    void tipFor(const TQWidget* w);
    bool isTippingFor(const TQWidget* w) const;

protected slots:
    void tipperDestroyed(TQObject* o);
    void internalUpdate();
    void display();
    void slotSettingsChanged();

private:
    TQBitmap m_mask;
    TQPixmap m_pixmap;
    TQPixmap m_icon;
    MaskEffect m_maskEffect;
    TQSimpleRichText* m_richText;
    TQMimeSourceFactory* m_mimeFactory;

    int m_dissolveSize;
    int m_dissolveDelta;
    KPanelApplet::Direction m_direction;

    TQTimer m_timer;
    TQTimer m_frameTimer;
    bool m_dirty;

    const TQWidget* m_tippingFor;

    static KickerTip* m_self;
    static int m_tippingEnabled;

    friend class KickerTip::Client;
};

#endif
