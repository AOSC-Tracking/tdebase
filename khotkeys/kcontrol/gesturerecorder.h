/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Mike Pilone <mpilone@slac.com>
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef GESTURE_RECORDER_H
#define GESTURE_RECORDER_H

#include <tqframe.h>
#include <tqstring.h>

#include <gestures.h>

class TQMouseEvent;

namespace KHotKeys
{

class GestureRecorder : public TQFrame
    {
    TQ_OBJECT

    public:
        GestureRecorder(TQWidget *parent, const char *name);
        ~GestureRecorder();

    protected:
        void mousePressEvent(TQMouseEvent *);
        void mouseReleaseEvent(TQMouseEvent *);
        void mouseMoveEvent(TQMouseEvent *);

    signals:
        void recorded(const TQString &data);

    private:
        bool _mouseButtonDown;
            Stroke stroke;
    };

} // namespace KHotKeys

#endif
