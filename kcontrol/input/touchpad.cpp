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

// TQt
#include <tqwhatsthis.h>
#include <tqtooltip.h>
#include <tqtabwidget.h>
#include <tqradiobutton.h>
#include <tqbuttongroup.h>
#include <tqcombobox.h>
#include <tqcheckbox.h>
#include <tqslider.h>
#include <tqlayout.h>
#include <tqlabel.h>

// TDE
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdeaboutdata.h>
#include <kdialog.h>
#include <kdebug.h>

// TouchpadConfig
#include "touchpad_settings.h"
#include "touchpad.h"
#include "touchpad.moc"


/******************************* TouchpadConfig *******************************/
TouchpadConfig::TouchpadConfig(TQWidget *parent, const char *name)
: TDECModule(parent, name),
  m_error(nullptr)
{
    TDEGlobal::iconLoader()->addAppDir("kcminput");

    d_settings = new TouchpadSettings;
    d_settings->apply();

    if (!d_settings->supportedTouchpad())
    {
        TQString error_str;

        if (!d_settings->foundTouchpad())
        {
            error_str = i18n(
                "<qt><h1>Touchpad not found</h1>"
                "Please check your system installation.</qt>"
            );
        }

        else IF_DRIVER(None)
        {
            error_str = i18n(
                "<qt><h1>Unsupported driver</h1>"
                "<p>This module only supports the following drivers:"
                "<p>Libinput, Synaptics</qt>"
            );
        }

        else error_str = i18n("<qt><h1>Unknown error</h1></qt>");

        m_error = new TQLabel(error_str, this);
        m_error->setAlignment(TQt::AlignCenter);
        new TQVBoxLayout(this);
        layout()->add(m_error);
        return;
    }

    initWidgets();
    load();

    kdDebug() << "managed touchpad: " << d_settings->touchpad().name
              << " (xid = " <<  d_settings->touchpad().id << ")" << endl;

    TDEAboutData* about = new TDEAboutData(
        "tdecm_touchpad",
        I18N_NOOP("Touchpad"),
        0, 0,
        TDEAboutData::License_GPL,
        I18N_NOOP("(c) 2024 Mavridis Philippe")
    );
    about->addAuthor("Mavridis Philippe", 0, 0);
    setAboutData(about);
}

TouchpadConfig::~TouchpadConfig()
{
    DEL(m_error)
    DEL(d_settings);
}

void TouchpadConfig::initWidgets()
{
    // Create containers
    m_container = new TQTabWidget(this);

    TQFrame *touchpadWidget = new TQFrame(this);
    touchpadWidget->setMargin(0);
    new TQVBoxLayout(touchpadWidget);

    // Enable option
    TQFrame *enableCheckBox = new TQFrame(touchpadWidget);
    enableCheckBox->setSizePolicy(TQSizePolicy::Maximum, TQSizePolicy::Fixed);

    m_enabled = new TQCheckBox(i18n("Enable touchpad"), enableCheckBox);
    TQWhatsThis::add(m_enabled, i18n(
        "This option determines whether the touchpad is enabled or disabled"
    ));

    // Compute margin for idented checkboxes based on checkbox height
    int lmargin = m_enabled->height() / 2;

    // Align the Enable checkbox with the other options below
    new TQHBoxLayout(enableCheckBox);
    enableCheckBox->layout()->addItem(new TQSpacerItem(lmargin, lmargin, TQSizePolicy::Fixed));
    enableCheckBox->layout()->add(m_enabled);

    // Settings frame
    TQFrame *settingsFrame = new TQFrame(touchpadWidget);
    TQGridLayout *grid = new TQGridLayout(settingsFrame, 3, 2, KDialog::spacingHint());

    connect(m_enabled, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
    connect(m_enabled, TQ_SIGNAL(toggled(bool)), settingsFrame, TQ_SLOT(setEnabled(bool)));

    // Behaviour
    m_behaviour = new TQGroupBox(2, TQt::Vertical, i18n("Behaviour"), settingsFrame);

    m_offWhileTyping = new TQCheckBox(i18n("Disable touchpad while typing"), m_behaviour);
    TQWhatsThis::add(m_offWhileTyping, i18n(
        "If this option is checked, the touchpad is disabled while you are typing, so as "
        "to prevent accidental cursor movement and clicks."
    ));
    connect(m_offWhileTyping, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));

    m_mbEmulation = new TQCheckBox(i18n("Middle button emulation"), m_behaviour);
    TQWhatsThis::add(m_mbEmulation, i18n(
        "If this option is enabled, a simultaneous left and right button click is "
        "automatically transformed into a middle button click."
    ));
    IF_DRIVER(LibInput)
    {
        connect(m_mbEmulation, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
    }
    else
    {
        DISABLE_UNSUPPORTED_OPTION(m_mbEmulation);
    }

    // Speed
    m_speed = new TQGroupBox(4, TQt::Vertical, i18n("Speed"), settingsFrame);

    TQLabel *accelLabel = new TQLabel(i18n("Acceleration:"), m_speed);

    m_accel = new TQSlider(-100, 100, 5, 0, TQt::Horizontal, m_speed);

    TQWidget *accelSliderMarkBox = new TQWidget(m_speed);
    new TQHBoxLayout(accelSliderMarkBox);
    accelSliderMarkBox->layout()->setAutoAdd(true);

    TQLabel *l;
    l = new TQLabel(i18n("Slower"), accelSliderMarkBox);
    l->setAlignment(TQt::AlignLeft);
    l = new TQLabel(i18n("Normal"), accelSliderMarkBox);
    l->setAlignment(TQt::AlignHCenter);
    l = new TQLabel(i18n("Faster"), accelSliderMarkBox);
    l->setAlignment(TQt::AlignRight);
    l = nullptr;

    m_accelAdaptive = new TQCheckBox(i18n("Use adaptive profile"), m_speed);

    IF_DRIVER(LibInput)
    {
        connect(m_accel, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
        connect(m_accelAdaptive, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));

        // check available profiles
        TQValueList<bool> accelProfilesAvail = d_settings->getAccelProfilesAvailability();
        if (!accelProfilesAvail.count() || accelProfilesAvail[0] == 0 || accelProfilesAvail[1] == 0)
        {
            m_accelAdaptive->setEnabled(false);
        }
    }
    else
    {
        DISABLE_UNSUPPORTED_OPTION(m_speed)
    }

    // Tapping
    m_tapping = new TQGroupBox(5, TQt::Vertical, i18n("Tapping"), settingsFrame);

    m_tapClick = new TQCheckBox(i18n("Tap to click"), m_tapping);
    TQWhatsThis::add(m_tapClick, i18n(
        "If this option is checked, a tap on the touchpad is interpreted as a button click."
    ));
    connect(m_tapClick, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
    connect(m_tapClick, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(updateWidgetStates()));

    m_tapDrag = new TQCheckBox(i18n("Tap-and-drag"), m_tapping);
    TQWhatsThis::add(m_tapDrag, i18n(
        "Tap-and-drag is a tap which is immediately followed by a finger down and that finger "
        "being held down emulates a button press. Moving the finger around can thus drag the "
        "selected item on the screen."
    ));
    connect(m_tapDrag, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
    connect(m_tapDrag, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(updateWidgetStates()));

    m_tapDragLock = new TQCheckBox(i18n("Tap-and-drag lock"), m_tapping);
    TQWhatsThis::add(m_tapDragLock, i18n(
        "When enabled, lifting a finger while dragging will not immediately stop dragging."
    ));

    IF_DRIVER(LibInput)
    {
        connect(m_tapDragLock, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
    }
    else
    {
        DISABLE_UNSUPPORTED_OPTION(m_tapDragLock);
    }

    TQLabel *tapMappingLabel = new TQLabel(i18n("Two-finger tap:"), m_tapping);
    m_tapMapping = new TQComboBox(m_tapping); // "lrm" and "lmr"
    m_tapMapping->setSizePolicy(TQSizePolicy::Maximum, TQSizePolicy::Fixed);
    m_tapMapping->insertItem(
        TDEGlobal::iconLoader()->loadIcon("mouse3", TDEIcon::Small),
        i18n("Right click (three-finger tap for middle click)"),
        0);
    m_tapMapping->insertItem(
        TDEGlobal::iconLoader()->loadIcon("mouse2", TDEIcon::Small),
        i18n("Middle click (three-finger tap for right click)"),
        1);
    connect(m_tapMapping, TQ_SIGNAL(activated(const TQString&)), this, TQ_SLOT(changed()));

    // Scrolling options
    m_scrolling = new TQGroupBox(4, TQt::Vertical, i18n("Scrolling options"), settingsFrame);

    m_verScroll = new TQCheckBox(i18n("Vertical scrolling"), m_scrolling);
    TQWhatsThis::add(m_verScroll, i18n(
        "This option enables/disables the vertical scrolling gesture on the touchpad. "
        "(The actual gesture depends on the selected scroll method.) "
        "Unless the used driver is Synaptics, disabling vertical scrolling also disables "
        "horizontal scrolling."
    ));
    connect(m_verScroll, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
    connect(m_verScroll, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(updateWidgetStates()));

    m_horScroll = new TQCheckBox(i18n("Horizontal scrolling"), m_scrolling);
    TQWhatsThis::add(m_horScroll, i18n(
        "This option enables/disables the horizontal scrolling gesture on the touchpad. "
        "(The actual gesture depends on the selected scroll method.)"
    ));
    connect(m_horScroll, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));
    connect(m_horScroll, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(updateWidgetStates()));

    m_naturalScroll = new TQCheckBox(i18n("Reverse scroll direction"), m_scrolling);
    TQWhatsThis::add(m_naturalScroll, i18n(
        "If this option is checked, the scrolling direction is reversed to resemble natural "
        "movement of content. This feature is also called natural scrolling."
    ));
    connect(m_naturalScroll, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(changed()));

    m_naturalScrollDirections = new TQFrame(m_scrolling);
    TQWhatsThis::add(m_naturalScrollDirections, i18n(
        "This option allows you to select the scrolling directions to which reversed scrolling will be applied. "
        "It is only available if the Synaptics driver is used."
    ));
    TQGridLayout *nsdl = new TQGridLayout(m_naturalScrollDirections, 2, 2, KDialog::spacingHint());
    m_horNaturalScroll = new TQCheckBox(i18n("Apply to horizontal scrolling"), m_naturalScrollDirections);
    m_verNaturalScroll = new TQCheckBox(i18n("Apply to vertical scrolling"), m_naturalScrollDirections);
    nsdl->addItem(new TQSpacerItem(lmargin, lmargin, TQSizePolicy::Fixed), 0, 0);
    nsdl->addItem(new TQSpacerItem(lmargin, lmargin, TQSizePolicy::Fixed), 1, 0);
    nsdl->addWidget(m_horNaturalScroll, 0, 1);
    nsdl->addWidget(m_verNaturalScroll, 1, 1);

    IF_DRIVER(Synaptics)
    {
        connect(m_horNaturalScroll, TQ_SIGNAL(toggled(bool)), TQ_SLOT(changed()));
        connect(m_verNaturalScroll, TQ_SIGNAL(toggled(bool)), TQ_SLOT(changed()));
        connect(m_naturalScroll, TQ_SIGNAL(toggled(bool)), TQ_SLOT(updateWidgetStates()));
    }
    else
    {
        // Not only disable, but also force checkboxes to be checked on
        // so that the user knows that the natural scrolling option applies
        // always to both directions
        DISABLE_UNSUPPORTED_OPTION(m_naturalScrollDirections);
        m_horNaturalScroll->setChecked(true);
        m_verNaturalScroll->setChecked(true);
    }

    // Scrolling methods
    m_scrollMethods = new TQButtonGroup(3, TQt::Vertical, i18n("Scrolling method"), settingsFrame);
    TQWhatsThis::add(m_scrollMethods, i18n(
        "Here you can select your preferred scrolling method. The two most common options are "
        "two-finger scrolling and edge scrolling. Two-finger scrolling entails a movement with "
        "two fingers vertically or horizontally upon the surface of the touchpad. Edge scrolling "
        "on the other hand tracks movements with one finger along the right or bottom edge of "
        "the touchpad."
    ));
    connect(m_scrollMethods, TQ_SIGNAL(clicked(int)), this, TQ_SLOT(changed()));

    TQStringList scrollMethodLabels;
    scrollMethodLabels << i18n("Two-finger")
                       << i18n("Edge");

    IF_DRIVER(LibInput)
    {
        scrollMethodLabels << i18n("Button");
    }

    TQValueList<bool> scrollMethodAvail = d_settings->getScrollMethodsAvailability();
    Q_ASSERT(scrollMethodLabels.count() == scrollMethodAvail.count());

    for (int i = 0; i < scrollMethodLabels.count(); ++i)
    {
        TQRadioButton *rad = new TQRadioButton(scrollMethodLabels[i], m_scrollMethods);
        rad->setEnabled(scrollMethodAvail[i]);
    }

    // Finalize layout
    grid->addWidget(m_behaviour, 0, 0);
    grid->addWidget(m_speed, 1, 0);
    grid->addMultiCellWidget(m_scrolling, 0, 1, 1, 1);
    grid->addWidget(m_scrollMethods, 2, 1);
    grid->addWidget(m_tapping, 2, 0);
    grid->addItem(new TQSpacerItem(10, 10));

    // Synaptics deprecation warning
    IF_DRIVER(Synaptics)
    {
        TQLabel *l = new TQLabel(i18n(
            "<qt><b>Warning:</b> The Synaptics driver has been deprecated.</qt>"
        ), settingsFrame);
        TQWhatsThis::add(l, i18n(
            "<qt><p><b>The Synaptics driver is no longer in active development.</b>"
            "<p>While Libinput is the preferred choice for handling input devices, "
            "you might still have valid reasons to use the older Synaptics driver "
            "in its place. Please bear in mind that you will probably not receive "
            "updates and bug fixes from its upstream.</qt>"
        ));
        grid->addMultiCellWidget(l, 3, 3, 0, 1);
    }

    touchpadWidget->layout()->add(enableCheckBox);
    touchpadWidget->layout()->add(settingsFrame);
    m_container->addTab(touchpadWidget, SmallIconSet("input-touchpad"), d_settings->touchpad().name);

    new TQVBoxLayout(this, KDialog::marginHint());
    layout()->add(m_container);
}

// We handle more complex UI cases here
void TouchpadConfig::updateWidgetStates()
{
    if (!d_settings->foundTouchpad()) return;

    // Scrolling related options
    bool on;

    IF_DRIVER(LibInput)
    {
        // To disable vertical scrolling under LibInput one has to disable scrolling entirely
        // so we mirror this in the UI
        on = m_verScroll->isChecked();
        m_horScroll->setEnabled(on);
    }

    else
    {
        // In case we can control both horizontal and vertical scrolling separately, any UI
        // changes should be triggered when both are disabled
        on = m_verScroll->isChecked() || m_horScroll->isChecked();

        // Only enable natural scroll directions options when not under LibInput
        m_naturalScrollDirections->setEnabled(on && m_naturalScroll->isChecked());
    }

    m_naturalScroll->setEnabled(on);
    m_scrollMethods->setEnabled(on);

    // Tapping related options
    m_tapDrag->setEnabled(m_tapClick->isChecked());

    IF_DRIVER(LibInput)
    {
        m_tapDragLock->setEnabled(m_tapClick->isChecked() && m_tapDrag->isChecked());
    }
}

void TouchpadConfig::defaults()
{
    load(true);
}

void TouchpadConfig::load()
{
    load(false);
}

void TouchpadConfig::load(bool useDefaults)
{
    if (!d_settings->foundTouchpad()) return;

    d_settings->load();

    m_enabled->setChecked(d_settings->enabled);

    // Behaviour
    m_offWhileTyping->setChecked(d_settings->offWhileTyping);

    IF_DRIVER(LibInput)
    {
        m_mbEmulation->setChecked(d_settings->midButtonEmulation);
    }

    // Speed
    IF_DRIVER(LibInput)
    {
        m_accel->setValue(d_settings->accelSpeed);
        m_accelAdaptive->setChecked(d_settings->accelProfile == 0);
    }

    // Tapping
    m_tapClick->setChecked(d_settings->tapClick);
    m_tapDrag->setChecked(d_settings->tapDrag);

    IF_DRIVER(LibInput)
    {
        m_tapDragLock->setChecked(d_settings->tapDragLock);
    }

    m_tapMapping->setCurrentItem(d_settings->tapMapping);

    // Scrolling options
    m_horScroll->setChecked(d_settings->scrollDirections & TQt::Horizontal);
    m_verScroll->setChecked(d_settings->scrollDirections & TQt::Vertical);
    m_naturalScroll->setChecked(d_settings->naturalScroll);
    IF_DRIVER(Synaptics)
    {
        m_naturalScrollDirections->setEnabled(d_settings->naturalScroll);
        m_horNaturalScroll->setChecked(d_settings->naturalScrollDirections & TQt::Horizontal);
        m_verNaturalScroll->setChecked(d_settings->naturalScrollDirections & TQt::Vertical);
    }

    IF_DRIVER(LibInput)
    {
        m_horScroll->setEnabled(m_verScroll->isOn());
        m_naturalScroll->setEnabled(m_verScroll->isOn());
        m_scrollMethods->setEnabled(m_verScroll->isOn());
    }

    // Scrolling method
    m_scrollMethods->setButton(d_settings->scrollMethod);
}

void TouchpadConfig::save()
{
    if (!d_settings->foundTouchpad()) return;

    d_settings->enabled = m_enabled->isChecked();

    // Behaviour
    d_settings->offWhileTyping = m_offWhileTyping->isChecked();

    IF_DRIVER(LibInput)
    {
        d_settings->midButtonEmulation = m_mbEmulation->isChecked();
    }

    // Speed
    IF_DRIVER(LibInput)
    {
        d_settings->accelSpeed = m_accel->value();
        d_settings->accelProfile = (m_accelAdaptive->isChecked() ? 0 : 1);
    }

    // Tapping
    d_settings->tapClick = m_tapClick->isChecked();
    d_settings->tapDrag = m_tapDrag->isChecked();

    IF_DRIVER(LibInput)
    {
        d_settings->tapDragLock = m_tapDragLock->isChecked();
    }

    d_settings->tapMapping = m_tapMapping->currentItem();

    // Scrolling options
    int scrollDirections = 0;
    if (m_horScroll->isChecked()) scrollDirections |= TQt::Horizontal;
    if (m_verScroll->isChecked()) scrollDirections |= TQt::Vertical;
    d_settings->scrollDirections = scrollDirections;

    d_settings->naturalScroll = m_naturalScroll->isChecked();

    int naturalScrollDirections = 0;
    if (m_horNaturalScroll->isChecked()) naturalScrollDirections |= TQt::Horizontal;
    if (m_verNaturalScroll->isChecked()) naturalScrollDirections |= TQt::Vertical;
    d_settings->naturalScrollDirections = naturalScrollDirections;

    // Scrolling method
    d_settings->scrollMethod = m_scrollMethods->selectedId();

    d_settings->save();
    d_settings->apply();
}

Touchpad TouchpadConfig::touchpad()
{
    return d_settings->touchpad();
}
