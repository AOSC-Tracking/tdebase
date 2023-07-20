/*****************************************************************

Copyright (c) 2004-2005 Aaron J. Seigo <aseigo@kde.org>
Copyright (c) 2004 Zack Rusin <zrusin@kde.org>
                   Sami Kyostil <skyostil@kempele.fi>

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

#ifndef ADDAPPLETVISUALFEEDBACK_H
#define ADDAPPLETVISUALFEEDBACK_H

#include <tqbitmap.h>
#include <tqpixmap.h>
#include <tqtimer.h>
#include <tqwidget.h>

#include <kpanelapplet.h>

class AppletItem;
class TQPaintEvent;
class TQSimpleRichText;
class TQTimer;

class AddAppletVisualFeedback : TQWidget
{
    TQ_OBJECT

    public:
        AddAppletVisualFeedback(AppletWidget* parent,
                                const TQWidget* destination,
                                KPanelApplet::Direction direction);
        ~AddAppletVisualFeedback();

    protected slots:
        void internalUpdate();
        void swoopCloser();

    protected:
        void paintEvent(TQPaintEvent * e);
        void mousePressEvent(TQMouseEvent * e);

        void makeMask();
        void displayInternal();

    private:
        const TQWidget* m_target;
        KPanelApplet::Direction m_direction;
        TQBitmap m_mask;
        TQPixmap m_pixmap;
        TQPixmap m_icon;
        TQSimpleRichText* m_richText;

        int m_dissolveSize;
        int m_dissolveDelta;
        int m_frames;

        TQTimer m_moveTimer;
        bool m_dirty;

        TQPoint m_destination;
};

#endif
