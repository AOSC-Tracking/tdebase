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

#ifndef __TOUCHPAD_H__
#define __TOUCHPAD_H__

// TDE
#include <tdecmodule.h>
#include <tdelocale.h>

// Macros
#define OPTION_NOT_SUPPORTED I18N_NOOP("This option is not compatible with the currently used driver")
#define DISABLE_UNSUPPORTED_OPTION(optionWidget) \
    optionWidget->setEnabled(false); \
    TQToolTip::add(optionWidget, i18n(OPTION_NOT_SUPPORTED));

// Forward definitions
class TQTabWidget;
class TQButtonGroup;
class TQGroupBox;
class TQCheckBox;
class TQComboBox;
class TQSlider;
class TQLabel;
class TQFrame;
class TDEConfig;
class TouchpadSettings;
struct Touchpad;


/******************************* TouchpadConfig *******************************/
class TouchpadConfig : public TDECModule
{
    TQ_OBJECT

    public:
        TouchpadConfig(TQWidget *parent, const char *name);
        ~TouchpadConfig();

        void load();
        void load(bool useDefaults);
        void save();
        void defaults();

        Touchpad touchpad();

    protected:
        void initWidgets();

    protected slots:
        void updateWidgetStates();

    private:
        TouchpadSettings *d_settings;

        TQTabWidget *m_container;
        TQLabel *m_error;
        TQCheckBox *m_enabled;

        TQGroupBox *m_behaviour;
        TQCheckBox *m_offWhileTyping, *m_leftHanded, *m_mbEmulation;

        TQGroupBox *m_speed;
        TQSlider *m_accel;
        TQCheckBox *m_accelAdaptive;

        TQGroupBox *m_tapping;
        TQCheckBox *m_tapClick, *m_tapDrag, *m_tapDragLock;
        TQComboBox *m_tapMapping;

        TQGroupBox *m_scrolling;
        TQCheckBox *m_horScroll, *m_verScroll, *m_naturalScroll,
                   *m_horNaturalScroll, *m_verNaturalScroll;

        TQFrame *m_naturalScrollDirections;

        TQButtonGroup *m_scrollMethods;
};

#endif // __TOUCHPAD_H__
