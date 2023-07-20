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

#ifndef __usersizesel_h__
#define __usersizesel_h__

#include <tqwidget.h>
#include <tqvaluevector.h>
#include <tqcolor.h>

#include <kpanelextension.h>

class ShutUpCompiler;

class UserSizeSel : public TQWidget
{
  TQ_OBJECT

    public:
        static TQRect select(const TQRect& rect, const KPanelExtension::Position pos, const TQColor& color);

    protected:
        void mouseReleaseEvent(TQMouseEvent *);
        void mouseMoveEvent(TQMouseEvent *);

    private:
        UserSizeSel(const TQRect& rect, const KPanelExtension::Position pos, const TQColor& color);
        ~UserSizeSel();
        void paintCurrent();

        TQPoint _orig_mouse_pos;
        int _orig_size;
        TQRect _rect;
        TQRect _orig_rect;
        KPanelExtension::Position _pos;
        TQWidget *_frame[8];
        TQColor _color;
        bool _frame1_shown;
        bool _frame2_shown;

        friend class ShutUpCompiler;
};

#endif
