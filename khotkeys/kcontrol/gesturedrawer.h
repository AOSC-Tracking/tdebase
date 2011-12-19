/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Mike Pilone <mpilone@slac.com>
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef GESTURE_DRAWER_H
#define GESTURE_DRAWER_H

#include <tqframe.h>
#include <tqstring.h>
#include <tqevent.h>
#include <tqpoint.h>
#include <tqwidget.h>
#include <tqsize.h>

namespace KHotKeys
{

class GestureDrawer : public TQFrame
    {
    Q_OBJECT
    public:
        GestureDrawer(TQWidget *parent, const char *name);
        ~GestureDrawer();

        void setData(const TQString &data);

        virtual TQSize sizeHint() const { return TQSize(30, 30); }

    protected:
        void paintEvent(TQPaintEvent *ev);

    private:
        TQPoint lookupCellCoords(TQ_UINT32 cell);
        void drawArrowHead(TQPoint &start, TQPoint &end,
                           TQPainter &p);


        TQString _data;
    };

} // namespace KHotKeys

#endif
