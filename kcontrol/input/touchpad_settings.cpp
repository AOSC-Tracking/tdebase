/*******************************************************************************
 tdecm_touchpad
 A touchpad module for the TDE Control Centre

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

// TDE
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>

// DCOP
#include <dcopref.h>

// X11
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

// tdecm_touchpad
#include "xiproperty.h"
#include "touchpad_settings.h"


/****************************** TouchpadSettings ******************************/
TouchpadSettings::TouchpadSettings()
: m_foundTouchpad(false)
{
    findTouchpad();
}

bool TouchpadSettings::findTouchpad()
{
    Display *display = tqt_xdisplay();
    ATOM(isTouchpad, XI_TOUCHPAD)
    ATOM(isLibinput, "libinput Send Events Mode Enabled")
    ATOM(isSynaptics, "Synaptics Off")

    int devicesCount;
    XDeviceInfo *deviceList = XListInputDevices(display, &devicesCount);

    for (int d = 0; d < devicesCount; ++d)
    {
        if (deviceList[d].type != isTouchpad) continue;

        m_foundTouchpad = true;
        m_touchpad.init(deviceList[d].id, deviceList[d].name);

        int propertiesCount;
        Atom *propertiesList = XIListProperties(display, deviceList[d].id,
                                                &propertiesCount);
        for (int p = 0; p < propertiesCount; ++p)
        {
            if (propertiesList[p] == isLibinput)
            {
                m_touchpad.driver = Touchpad::Driver::LibInput;
                break;
            }

            else if (propertiesList[p] == isSynaptics)
            {
                m_touchpad.driver = Touchpad::Driver::Synaptics;
            }
        }

        XFree(propertiesList);

        if (m_foundTouchpad) break;
    }

    XFreeDeviceList(deviceList);

    return m_foundTouchpad;
}

void TouchpadSettings::load(bool defaults)
{
    TDEConfig cfg("kcminputrc");
    cfg.setGroup("Touchpad");
    cfg.setReadDefaults(defaults);

    enabled = cfg.readBoolEntry("Enabled", true);

    // Behaviour
    offWhileTyping = cfg.readBoolEntry("OffWhileTyping", false);

    IF_DRIVER(LibInput)
    {
        midButtonEmulation = cfg.readBoolEntry("MidButtonEmulation", false);
    }

    // Speed
    IF_DRIVER(LibInput)
    {
        accelSpeed = cfg.readNumEntry("AccelSpeed", 0);
        accelProfile = cfg.readNumEntry("AccelProfile", 0);
    }

    // Tapping
    tapClick = cfg.readBoolEntry("TapToClick", true);
    tapDrag = cfg.readBoolEntry("TapAndDrag", true);

    IF_DRIVER(LibInput)
    {
        tapDragLock = cfg.readBoolEntry("TapAndDragLock", false);
    }

    tapMapping = cfg.readNumEntry("TapMapping", 0);

    // Scrolling options
    int both = TQt::Horizontal | TQt::Vertical;
    scrollDirections = cfg.readNumEntry("ScrollDirections", both);
    naturalScroll = cfg.readBoolEntry("NaturalScroll", false);
    naturalScrollDirections = cfg.readNumEntry("NaturalScrollDirections", both);

    // Scrolling method
    scrollMethod = cfg.readNumEntry("ScrollMethod", 0);
}

void TouchpadSettings::save()
{
    TDEConfig cfg("kcminputrc");
    cfg.setGroup("Touchpad");

    cfg.writeEntry("Enabled", enabled);

    // Behaviour
    cfg.writeEntry("OffWhileTyping", offWhileTyping);

    IF_DRIVER(LibInput)
    {
        cfg.writeEntry("MidButtonEmulation", midButtonEmulation);
    }

    // Speed
    cfg.writeEntry("AccelSpeed", accelSpeed);
    cfg.writeEntry("AccelProfile", accelProfile);

    // Tapping
    cfg.writeEntry("TapToClick", tapClick);
    cfg.writeEntry("TapAndDrag", tapDrag);

    IF_DRIVER(LibInput)
    {
        cfg.writeEntry("TapAndDragLock", tapDragLock);
    }

    cfg.writeEntry("TapMapping", tapMapping);

    // Scrolling options
    cfg.writeEntry("ScrollDirections", scrollDirections);
    cfg.writeEntry("NaturalScroll", naturalScroll);
    cfg.writeEntry("NaturalScrollDirections", naturalScrollDirections);

    // Scrolling method
    cfg.writeEntry("ScrollMethod", scrollMethod);

    cfg.sync();
}

bool TouchpadSettings::setTouchpadEnabled(bool on)
{
    enabled = on;

    XIProperty *prop = nullptr;
    int fail = 0;

    IF_DRIVER(LibInput)
    {
        SET_PROP("Device Enabled", b)
        {
            prop->b[0] = enabled;
            prop->set();
        }
    }

    else
    IF_DRIVER(Synaptics)
    {
        SET_PROP("Synaptics Off", b)
        {
            prop->b[0] = !enabled;
            prop->set();
        }
    }

    return !fail;
}

void TouchpadSettings::apply(bool force)
{
    kdDebug() << "applying touchpad settings" << endl;
    if (!foundTouchpad())
    {
        kdWarning() << "no supported touchpads! settings not applied" << endl;
        return;
    }

    load();

    Display *display = tqt_xdisplay();
    XIProperty *prop = nullptr;
    int fail = 0;

    if (!setTouchpadEnabled(enabled))
        ++fail;

    IF_DRIVER(LibInput)
    {
        kdDebug() << "driver: libinput" << endl;

        SET_PROP("libinput Disable While Typing Enabled", b)
        {
            prop->b[0] = offWhileTyping;
            prop->set();
        }

        SET_PROP("libinput Middle Emulation Enabled", b)
        {
            prop->b[0] = midButtonEmulation;
            prop->set();
        }

        SET_PROP("libinput Accel Speed", f)
        {
            float val = accelSpeed;
            val /= 100;
            prop->f[0] = val;
            prop->set();
        }

        SET_PROP("libinput Accel Profile Enabled", b)
        {
            prop->b[0] = (accelProfile == 0);
            prop->b[1] = (accelProfile == 1);
            prop->set();
        }

        SET_PROP("libinput Tapping Enabled", b)
        {
            prop->b[0] = tapClick;
            prop->set();
        }

        SET_PROP("libinput Tapping Drag Enabled", b)
        {
            prop->b[0] = tapClick && tapDrag;
            prop->set();
        }

        SET_PROP("libinput Tapping Drag Lock Enabled", b)
        {
            prop->b[0] = tapClick && tapDrag && tapDragLock;
            prop->set();
        }

        SET_PROP("libinput Tapping Button Mapping Enabled", b)
        {
            prop->b[0] = (tapMapping == 0);
            prop->b[1] = (tapMapping == 1);
            prop->set();
        }

        SET_PROP("libinput Horizontal Scroll Enabled", b)
        {
            prop->b[0] = scrollDirections & TQt::Horizontal;
            prop->set();
        }

        SET_PROP("libinput Natural Scrolling Enabled", b)
        {
            prop->b[0] = naturalScroll;
            prop->set();
        }

        SET_PROP("libinput Scroll Method Enabled", b)
        {
            prop->b[0] = scrollDirections ? (scrollMethod == 0) : 0; // two-finger
            prop->b[1] = scrollDirections ? (scrollMethod == 1) : 0; // edge
            prop->b[2] = scrollDirections ? (scrollMethod == 2) : 0; // button
            prop->set();
        }
    }

    else IF_DRIVER(Synaptics)
    {
        kdDebug() << "driver: synaptics" << endl;

        SET_PROP("Synaptics Tap Action", b)
        {
            prop->b[0] = 0;
            prop->b[1] = 0;
            prop->b[2] = 0;
            prop->b[3] = 0;
            prop->b[4] = tapClick ? 1 : 0; // 1 finger
            prop->b[5] = tapClick ? (tapMapping == 0 ? 3 : 2) : 0; // 2 fingers
            prop->b[6] = tapClick ? (tapMapping == 0 ? 2 : 3) : 0; // 3 fingers
            prop->set();
        }

        SET_PROP("Synaptics Gestures", b)
        {
            prop->b[0] = tapDrag;
            prop->set();
        }

        SET_PROP("Synaptics Edge Scrolling", b)
        {
            prop->b[0] = scrollMethod == 1 ? (scrollDirections & TQt::Vertical ? 1 : 0) : 0;
            prop->b[1] = scrollMethod == 1 ? (scrollDirections & TQt::Horizontal ? 1 : 0) : 0;
            prop->b[2] = 0; // corner
            prop->set();
        }

        SET_PROP("Synaptics Two-Finger Scrolling", b)
        {
            prop->b[0] = scrollMethod == 0 ? (scrollDirections & TQt::Vertical ? 1 : 0) : 0;
            prop->b[1] = scrollMethod == 0 ? (scrollDirections & TQt::Horizontal ? 1 : 0) : 0;
            prop->set();
        }

        SET_PROP("Synaptics Scrolling Distance", i)
        {
            prop->i[0] = naturalScroll && naturalScrollDirections & TQt::Vertical ? -80 : 80;
            prop->i[1] = naturalScroll && naturalScrollDirections & TQt::Horizontal ? -80 : 80;
            prop->set();
        }

        // start/stop tdesyndaemon
        DCOPRef tdesyndaemon("tdesyndaemon", "tdesyndaemon");
        tdesyndaemon.call("stop()");

        if (offWhileTyping)
        {
            kapp->tdeinitExec("tdesyndaemon");
        }
    }

    if (fail > 0)
        kdWarning() << "some options could not be applied!" << endl;
}

TQValueList<bool> TouchpadSettings::getScrollMethodsAvailability()
{
    TQValueList<bool> avail;

    IF_DRIVER(LibInput)
    {
        PROP(propScrollMethodsAvail, "libinput Scroll Methods Available")
        for (int i = 0; i < propScrollMethodsAvail.count(); ++i)
        {
            avail.append(propScrollMethodsAvail[i].toBool());
        }
    }

    IF_DRIVER(Synaptics)
    {
        avail.append(1); // two-finger
        avail.append(1); // edge
    }

    return avail;
}

TQValueList<bool> TouchpadSettings::getAccelProfilesAvailability()
{
    TQValueList<bool> avail;

    IF_DRIVER(LibInput)
    {
        PROP(propAccelProfilesAvail, "libinput Accel Profiles Available")
        for (int i = 0; i < propAccelProfilesAvail.count(); ++i)
        {
            avail.append(propAccelProfilesAvail[i].toBool());
        }
    }

    IF_DRIVER(Synaptics) { /* TODO no support yet */ }

    return avail;
}

Touchpad TouchpadSettings::touchpad()
{
    return m_touchpad;
}

bool TouchpadSettings::foundTouchpad()
{
    return m_foundTouchpad;
}

bool TouchpadSettings::supportedTouchpad()
{
    return m_foundTouchpad && m_touchpad.driver != Touchpad::Driver::None;
}
