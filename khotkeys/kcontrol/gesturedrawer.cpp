/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Mike Pilone <mpilone@slac.com>
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#include <tqcolor.h>
#include <tqpainter.h>

#include "gesturedrawer.h"

namespace KHotKeys
{

GestureDrawer::GestureDrawer(TQWidget *parent, const char *name)
  : TQFrame(parent, name), _data(TQString::null)
    {
    setBackgroundColor( colorGroup().base());
    setFrameStyle(TQFrame::Panel | TQFrame::Sunken);
    setMinimumSize(30, 30);
    }

GestureDrawer::~GestureDrawer()
    {
    }

void GestureDrawer::setData(const TQString &data)
    {
    _data = data;

    repaint();
    }

void GestureDrawer::paintEvent(TQPaintEvent *ev)
    {
  // Iterate through the data points and draw a line to each of them
    TQ_UINT32 startCell = 0;
    TQ_UINT32 endCell = 0;
    TQPoint startPoint;
    TQPoint endPoint;

    TQPainter p(this);

    if (_data.length() > 0)
        {
        startCell = TQString(_data[0]).toUInt();
        }

    for (TQ_UINT32 index = 1; index < _data.length(); ++index)
        {
        endCell = TQString(_data[index]).toUInt();

        startPoint = lookupCellCoords(startCell);
        endPoint = lookupCellCoords(endCell);

        if (index == 1)
            {
      // Draw something to show the starting point
            p.drawRect(startPoint.x()-2, startPoint.y()-2, 4, 4);
            p.fillRect(startPoint.x()-2, startPoint.y()-2, 4, 4,
                       TQBrush(black));
            }

        p.drawLine(startPoint, endPoint);
        drawArrowHead(startPoint, endPoint, p);

        startCell = endCell;
        }

    p.end();

    TQFrame::paintEvent(ev);
    }

TQPoint GestureDrawer::lookupCellCoords(TQ_UINT32 cell)
    {
  // First divide the widget into thirds, horizontally and vertically
    TQ_UINT32 w = width();
    TQ_UINT32 h = height();

    TQ_UINT32 wThird = w / 3;
    TQ_UINT32 hThird = h / 3;

    switch(cell)
        {
        case 1:
            return TQPoint(wThird/2, 2*hThird+hThird/2);

        case 2:
            return TQPoint(wThird+wThird/2, 2*hThird+hThird/2);

        case 3:
            return TQPoint(2*wThird+wThird/2, 2*hThird+hThird/2);

        case 4:
            return TQPoint(wThird/2, hThird+hThird/2);

        case 5:
            return TQPoint(wThird+wThird/2, hThird+hThird/2);

        case 6:
            return TQPoint(2*wThird+wThird/2, hThird+hThird/2);

        case 7:
            return TQPoint(wThird/2, hThird/2);

        case 8:
            return TQPoint(wThird+wThird/2, hThird/2);

        case 9:
            return TQPoint(2*wThird+wThird/2, hThird/2);
        }

    return TQPoint(0, 0);
    }

void GestureDrawer::drawArrowHead(TQPoint &start, TQPoint &end,
                                  TQPainter &p)
    {
    int deltaX = end.x() - start.x();
    int deltaY = end.y() - start.y();

    if (deltaY == 0)
        {
    // horizontal line
        int offset = 0;
        if (deltaX > 0)
          offset = -3;
        else
          offset = 3;

        p.drawLine(TQPoint(end.x()+offset, end.y()+2), end);
        p.drawLine(TQPoint(end.x()+offset, end.y()-2), end);
        }
    else if (deltaX == 0)
        {
    // vertical line
        int offset = 0;
        if (deltaY > 0)
          offset = -3;
        else
          offset = +3;

        p.drawLine(TQPoint(end.x()+2, end.y()+offset), end);
        p.drawLine(TQPoint(end.x()-2, end.y()+offset), end);
        }
    else
        {
    // diagnal - The math would be pretty complex, so don't do anything
        }

    }

} // namespace KHotKeys

#include "gesturedrawer.moc"
