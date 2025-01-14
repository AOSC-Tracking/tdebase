/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.
Copyright (c) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>

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

#include <tqapplication.h>
#include <tqpainter.h>
#include <tqcursor.h>

#include "usersizesel.h"
#include "usersizesel.moc"

#define PANEL_MINIMUM_HEIGHT 16

UserSizeSel::UserSizeSel(const TQRect& rect, const KPanelExtension::Position pos, const TQColor& color)
  : TQWidget(0, 0, (WFlags)(WStyle_Customize | WX11BypassWM)),
    _orig_size(0),
    _rect(rect),
    _orig_rect(rect),
    _pos(pos),
    _frame1_shown(false),
    _frame2_shown(false)
{
    if ((pos == KPanelExtension::Left) || (pos == KPanelExtension::Right))
    {
        setCursor(sizeHorCursor);
    }
    if ((pos == KPanelExtension::Top) || (pos == KPanelExtension::Bottom))
    {
        setCursor(TQt::sizeVerCursor);
    }

    setGeometry(-10, -10, 2, 2);
    _color = color;
    for (int i = 0; i < 8; i++)
    {
        _frame[i] = 0;
    }
}

UserSizeSel::~UserSizeSel()
{
    for (int i = 0; i < 8; i++)
    {
        delete _frame[i];
    }
    _frame1_shown = false;
    _frame2_shown = false;
}

void UserSizeSel::mouseReleaseEvent(TQMouseEvent * e)
{
    if (e->button() == TQt::LeftButton)
    {
        tqApp->exit_loop();
    }
}

void UserSizeSel::mouseMoveEvent(TQMouseEvent * e)
{
    int newSize = _orig_size;
    TQPoint p(e->globalPos() - _orig_mouse_pos);

    if (_pos == KPanelExtension::Left)
    {
        newSize = _orig_size + p.x();
    }
    if (_pos == KPanelExtension::Right)
    {
        newSize = _orig_size + ((-1)*p.x());
    }
    if (_pos == KPanelExtension::Top)
    {
        newSize = _orig_size + p.y();
    }
    if (_pos == KPanelExtension::Bottom)
    {
        newSize = _orig_size + ((-1)*p.y());
    }
//     int screen = xineramaScreen();
//     if (screen < 0)
//     {
//         screen = tdeApp->desktop()->screenNumber(this);
//     }
//     TQRect desktopGeom = TQApplication::desktop()->screenGeometry(screen);
    if (newSize < PANEL_MINIMUM_HEIGHT)
    {
        newSize = PANEL_MINIMUM_HEIGHT;
    }
    int maxSize = 256;
//     if ((_pos == KPanelExtension::Left) || (_pos == KPanelExtension::Right))
//     {
//         maxSize = desktopGeom.width()/2;
//     }
//     if ((_pos == KPanelExtension::Top) || (_pos == KPanelExtension::Bottom))
//     {
//         maxSize = desktopGeom.height()/2;
//     }
    if (newSize > maxSize)
    {
        newSize = maxSize;
    }

    if (_pos == KPanelExtension::Left)
    {
        _rect.setWidth(newSize);
    }
    if (_pos == KPanelExtension::Right)
    {
        _rect.setX(_orig_rect.x()-(newSize-_orig_size));
        _rect.setWidth(newSize);
    }
    if (_pos == KPanelExtension::Top)
    {
        _rect.setHeight(newSize);
    }
    if (_pos == KPanelExtension::Bottom)
    {
        _rect.setY(_orig_rect.y()-(newSize-_orig_size));
        _rect.setHeight(newSize);
    }

    // Compress paint events to increase responsiveness
    if (TQCursor::pos() == e->globalPos())
    {
        paintCurrent();
    }
}

void UserSizeSel::paintCurrent()
{
    int i;
    int x, y, w, h;

    if (!_frame[0])
    {
        for (i = 0; i < 4; i++)
        {
            _frame[i] = new TQWidget(0, 0, (WFlags)(WStyle_Customize | WStyle_NoBorder | WX11BypassWM));
            _frame[i]->setPaletteBackgroundColor(TQt::black);
        }
        for (i = 4; i < 8; i++)
        {
            _frame[i] = new TQWidget(0, 0, (WFlags)(WStyle_Customize | WStyle_NoBorder | WX11BypassWM));
            _frame[i]->setPaletteBackgroundColor(_color);
        }
    }

    x = _rect.x();
    y = _rect.y();
    w = _rect.width();
    h = _rect.height();

    if (w > 0 && h > 0)
    {
        _frame[0]->setGeometry(x, y, w, 4);
        _frame[1]->setGeometry(x, y, 4, h);
        _frame[2]->setGeometry(x + w - 4, y, 4, h);
        _frame[3]->setGeometry(x, y + h - 4, w, 4);

        if (!_frame1_shown)
        {
            for (i = 0; i < 4; i++)
            {
                _frame[i]->show();
            }
            _frame1_shown = true;
        }
    }

    x += 1;
    y += 1;
    w -= 2;
    h -= 2;

    if (w > 0 && h > 0)
    {
        _frame[4]->setGeometry(x, y, w, 2);
        _frame[5]->setGeometry(x, y, 2, h);
        _frame[6]->setGeometry(x + w - 2, y, 2, h);
        _frame[7]->setGeometry(x, y + h - 2, w, 2);

        if (!_frame2_shown)
        {
            for (i = 4; i < 8; i++)
            {
                _frame[i]->show();
            }
            _frame2_shown = true;
        }
    }

}

TQRect UserSizeSel::select(const TQRect& rect, const KPanelExtension::Position pos, const TQColor& color)
{
    UserSizeSel sel(rect, pos, color);
    sel._orig_mouse_pos = TQCursor::pos();
    if ((pos == KPanelExtension::Left) || (pos == KPanelExtension::Right))
    {
        sel._orig_size = rect.width();
    }
    if ((pos == KPanelExtension::Top) || (pos == KPanelExtension::Bottom))
    {
        sel._orig_size = rect.height();
    }
    sel.show();
    sel.grabMouse();
    sel.paintCurrent();
    tqApp->enter_loop();
    sel.paintCurrent();
    sel.releaseMouse();
    tqApp->syncX();
    return sel._rect;
}

