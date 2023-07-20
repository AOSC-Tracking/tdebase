/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Mike Pilone <mpilone@slac.com>
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef GESTURE_RECORD_PAGE_H
#define GESTURE_RECORD_PAGE_H

#include <tqvbox.h>

#include "gesturedrawer.h"

class TQWidget;
class TQPushButton;
class TQLabel;

namespace KHotKeys
{

class Gesture;
class GestureRecorder;

class GestureRecordPage : public TQVBox
    {
    TQ_OBJECT

    public:
        GestureRecordPage(const TQString &gesture,
                          TQWidget *parent, const char *name);
        ~GestureRecordPage();

        const TQString &getGesture() const { return _gest; }

    protected slots:
        void slotRecorded(const TQString &data);
         void slotResetClicked();

    signals:
        void gestureRecorded(bool);

    private:
        GestureRecorder *_recorder;
        TQPushButton *_resetButton;
        GestureDrawer *_tryOne;
        GestureDrawer *_tryTwo;
        GestureDrawer *_tryThree;

        TQString _gest;

        TQ_UINT32 _tryCount;
    };

} // namespace KHotKeys

#endif
