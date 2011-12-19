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

#ifndef __userrectsel_h__
#define __userrectsel_h__

#include <tqwidget.h>
#include <tqvaluevector.h>
#include <tqcolor.h>

#include <kpanelextension.h>

class ShutUpCompiler;

class UserRectSel : public TQWidget
{
  Q_OBJECT

    public:
        class PanelStrut
        {
            public:
                PanelStrut()
                    : m_screen(-1),
                      m_pos(KPanelExtension::Bottom),
                      m_alignment(KPanelExtension::LeftTop)
                {
                }

                PanelStrut(const TQRect& rect, int XineramaScreen,
                           KPanelExtension::Position pos,
                           KPanelExtension::Alignment alignment)
                    : m_rect(rect),
                      m_screen(XineramaScreen),
                      m_pos(pos),
                      m_alignment(alignment)
                {
                }

                bool operator==(const PanelStrut& rhs)
                {
                    return m_screen == rhs.m_screen &&
                           m_pos == rhs.m_pos &&
                           m_alignment == rhs.m_alignment;
                }

                bool operator!=(const PanelStrut& rhs)
                {
                    return !(*this == rhs);
                }

                TQRect m_rect;
                int m_screen;
                KPanelExtension::Position m_pos;
                KPanelExtension::Alignment m_alignment;
        };

        typedef TQValueVector<PanelStrut> RectList;
        static PanelStrut select(const RectList& rects, const TQPoint& _offset, const TQColor& color);

    protected:
        void mouseReleaseEvent(TQMouseEvent *);
        void mouseMoveEvent(TQMouseEvent *);

    private:
        UserRectSel(const RectList& rects, const TQPoint& _offset, const TQColor& color);
        ~UserRectSel();
        void paintCurrent();

        const RectList rectangles;
        PanelStrut current;
        TQPoint offset;
        TQWidget *_frame[8];
        TQColor _color;

        friend class ShutUpCompiler;
};

#endif
