/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Mike Pilone <mpilone@slac.com>
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#include <tqcolor.h>
#include <tqevent.h>

#include "gesturerecorder.h"

namespace KHotKeys
{

GestureRecorder::GestureRecorder(TQWidget *parent, const char *name)
  : TQFrame(parent, name), _mouseButtonDown(false)
    {
    setBackgroundColor( colorGroup().base());
    setFrameStyle(TQFrame::Sunken | TQFrame::Panel);
    setLineWidth(2);
    setMidLineWidth(0);
    }

GestureRecorder::~GestureRecorder()
    {
    }

void GestureRecorder::mousePressEvent(TQMouseEvent *ev)
    {
    if (ev->button() == Qt::LeftButton)
        {
        _mouseButtonDown = true;
        stroke.reset();
        TQPoint pos = ev->pos();
        stroke.record(pos.x(), pos.y());
        }
    }

void GestureRecorder::mouseReleaseEvent(TQMouseEvent *ev)
    {
    if ((ev->button() == Qt::LeftButton) && (_mouseButtonDown))
        {
        TQPoint pos = ev->pos();
        stroke.record(pos.x(), pos.y());
        TQString data( stroke.translate());
        if( !data.isEmpty())
            emit recorded(data);
        }
    }

void GestureRecorder::mouseMoveEvent(TQMouseEvent *ev)
    {
    if (_mouseButtonDown)
        {
        TQPoint pos = ev->pos();
        stroke.record(pos.x(), pos.y());
        }
    }

} // namespace KHotKeys

#include "gesturerecorder.moc"
