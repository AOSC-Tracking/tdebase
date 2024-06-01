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

#ifndef __TOUCHPAD_SETTINGS_H__
#define __TOUCHPAD_SETTINGS_H__

// TQt
#include <tqvaluelist.h>

// Macros
#define DEL(var) \
  if (var) { delete var; var = nullptr; }

#define ATOM(var, atom) \
  Atom var = XInternAtom(display, atom, true);

#define PROP(var, property) \
  XIProperty var = XIProperty(m_touchpad.id, property);

#define SET_PROP(property, type) \
  DEL(prop) \
  prop = new XIProperty(m_touchpad.id, property); \
  if (prop->type == nullptr) \
  { \
     kdWarning() << "Failed to set property " << property << endl; \
     ++fail; \
  } \
  else

#define IF_DRIVER(drv) \
  if (touchpad().driver == Touchpad::Driver::drv)


/****************************** struct Touchpad *******************************/
#undef None

struct Touchpad
{
    enum Driver { None, LibInput, Synaptics };

    bool valid = false;
    unsigned int id;
    TQCString name;
    Driver driver = Touchpad::Driver::None;

    void init(unsigned int _id, TQCString _name)
    {
        valid = true;
        id = _id;
        name = _name;
    }
};


/***************************** TouchpadSettings *******************************/
class TouchpadSettings
{
    public:
        TouchpadSettings();

        void load(bool defaults = false);
        void save();
        void apply(bool force = false);

        TQValueList<bool> getScrollMethodsAvailability();
        TQValueList<bool> getAccelProfilesAvailability();

        bool enabled, tapClick, tapDrag, tapDragLock, tapMapping, offWhileTyping,
             leftHandedMode, midButtonEmulation, naturalScroll, scrollMethod;
        int scrollDirections, naturalScrollDirections;

        int accelSpeed, accelProfile;

        bool foundTouchpad();
        Touchpad touchpad();

        // Enable/disable touchpad without applying all settings
        bool setTouchpadEnabled(bool on);

        bool supportedTouchpad();

    protected:
        bool findTouchpad();

    private:
        Touchpad m_touchpad;
        bool m_foundTouchpad;
};

#endif // __TOUCHPAD_SETTINGS_H__