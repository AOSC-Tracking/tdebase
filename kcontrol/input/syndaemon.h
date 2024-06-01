/*******************************************************************************
 syndaemon - daemon for the Synaptics touchpad driver which disables touchpad
             on keyboard input

 Copyright © 2004 Nadeem Hasan <nhasan@kde.org>
                  Stefan Kombrink <katakombi@web.de>
             2024 Mavridis Philippe <mavridisf@gmail.com>

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with
 this program. If not, see <https://www.gnu.org/licenses/>.

*******************************************************************************/

#ifndef __SYNDAEMON_H__
#define __SYNDAEMON_H__

// TQt
#include <tqobject.h>
#include <tqthread.h>

// DCOP
#include <dcopobject.h>

// X11
#include <X11/Xlib.h>
#undef Bool /* fix problems in --enable-final mode */
#undef None /* fix problems in --enable-final mode */

// Syndaemon
#include "syndaemon_iface.h"


class TQTimer;

class SynDaemon : public TQObject, public virtual SynDaemonIface
{
    TQ_OBJECT

    public:
        SynDaemon();
        ~SynDaemon();

        bool touchpadEnabled();

    public slots:
        void poll();
        void setTouchpadOn(bool on);
        virtual void stop();

    protected:
        void clearBit(unsigned char* ptr, int bit);
        bool hasKeyboardActivity();

    private:
        TouchpadSettings *d_settings;

        TQTimer *m_poll;
        TQTime *m_time;
        Display *m_display;
        bool m_typing;

        static const unsigned int POLL_INTERVAL;
        static const unsigned int TIME_OUT;
        static const unsigned int KEYMAP_SIZE;
        static unsigned char *m_keyboard_mask;
};

#endif

