/*******************************************************************************
 tdesyndaemon - daemon for the Synaptics touchpad driver which disables touchpad
                on keyboard input

 Copyright © 2024 Mavridis Philippe <mavridisf@gmail.com>

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

#ifndef __TDESYNDAEMON_IFACE_H__
#define __TDESYNDAEMON_IFACE_H__

// DCOP
#include <dcopobject.h>

class TDESynDaemonIface : virtual public DCOPObject
{
    K_DCOP
    k_dcop:
        virtual void stop() = 0;
};

#endif
