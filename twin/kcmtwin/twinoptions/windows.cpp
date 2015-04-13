/*
 * windows.cpp
 *
 * Copyright (c) 1997 Patrick Dowler dowler@morgul.fsh.uvic.ca
 * Copyright (c) 2001 Waldo Bastian bastian@kde.org
 * Copyright (c) 2011-2014 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include <config.h>

#include <tqdir.h>
#include <tqlayout.h>
#include <tqslider.h>
#include <tqwhatsthis.h>
#include <tqvbuttongroup.h>
#include <tqcheckbox.h>
#include <tqradiobutton.h>
#include <tqlabel.h>
#include <tqcombobox.h>
#include <tdemessagebox.h>

#include <kactivelabel.h>
#include <tdelocale.h>
#include <kcolorbutton.h>
#include <tdeconfig.h>
#include <knuminput.h>
#include <tdeapplication.h>
#include <kdialog.h>
#include <dcopclient.h>
#include <tdeglobal.h>
#include <kprocess.h>
#include <tqtabwidget.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "windows.h"


// twin config keywords
#define KWIN_FOCUS                 "FocusPolicy"
#define KWIN_PLACEMENT             "Placement"
#define KWIN_MOVE                  "MoveMode"
#define KWIN_MINIMIZE_ANIM         "AnimateMinimize"
#define KWIN_MINIMIZE_ANIM_SPEED   "AnimateMinimizeSpeed"
#define KWIN_RESIZE_OPAQUE         "ResizeMode"
#define KWIN_GEOMETRY              "GeometryTip"
#define KWIN_AUTORAISE_INTERVAL    "AutoRaiseInterval"
#define KWIN_AUTORAISE             "AutoRaise"
#define KWIN_DELAYFOCUS_INTERVAL   "DelayFocusInterval"
#define KWIN_DELAYFOCUS            "DelayFocus"
#define KWIN_CLICKRAISE            "ClickRaise"
#define KWIN_ANIMSHADE             "AnimateShade"
#define KWIN_MOVE_RESIZE_MAXIMIZED "MoveResizeMaximizedWindows"
#define KWIN_ALTTABMODE            "AltTabStyle"
#define KWIN_TRAVERSE_ALL          "TraverseAll"
#define KWIN_SHOW_POPUP            "ShowPopup"
#define KWIN_ROLL_OVER_DESKTOPS    "RollOverDesktops"
#define KWIN_SHADEHOVER            "ShadeHover"
#define KWIN_SHADEHOVER_INTERVAL   "ShadeHoverInterval"
#define KWIN_FOCUS_STEALING        "FocusStealingPreventionLevel"
#define KWIN_HIDE_UTILITY          "HideUtilityWindowsForInactive"
#define KWIN_SEPARATE_SCREEN_FOCUS "SeparateScreenFocus"
#define KWIN_ACTIVE_MOUSE_SCREEN   "ActiveMouseScreen"

// kwm config keywords
#define KWM_ELECTRIC_BORDER                  "ElectricBorders"
#define KWM_ELECTRIC_BORDER_DELAY            "ElectricBorderDelay"

//CT 15mar 98 - magics
#define KWM_BRDR_SNAP_ZONE                   "BorderSnapZone"
#define KWM_BRDR_SNAP_ZONE_DEFAULT           10
#define KWM_WNDW_SNAP_ZONE                   "WindowSnapZone"
#define KWM_WNDW_SNAP_ZONE_DEFAULT           10

#define MAX_BRDR_SNAP                          100
#define MAX_WNDW_SNAP                          100
#define MAX_EDGE_RES                          1000

TQString TDECompositor = TDE_COMPOSITOR_BINARY;

KFocusConfig::~KFocusConfig ()
{
    if (standAlone)
        delete config;
}

// removed the LCD display over the slider - this is not good GUI design :) RNolden 051701
KFocusConfig::KFocusConfig (bool _standAlone, TDEConfig *_config, TQWidget * parent, const char *)
    : TDECModule(parent, "kcmkwm"), config(_config), standAlone(_standAlone)
{
    TQString wtstr;
    TQBoxLayout *lay = new TQVBoxLayout (this, 0, KDialog::spacingHint());

    //iTLabel = new TQLabel(i18n("  Allowed overlap:\n"
    //                         "(% of desktop space)"),
    //             plcBox);
    //iTLabel->setAlignment(AlignTop|AlignHCenter);
    //pLay->addWidget(iTLabel,1,1);

    //interactiveTrigger = new TQSpinBox(0, 500, 1, plcBox);
    //pLay->addWidget(interactiveTrigger,1,2);

    //pLay->addRowSpacing(2,KDialog::spacingHint());

    //lay->addWidget(plcBox);

    // focus policy
    fcsBox = new TQButtonGroup(i18n("Focus"),this);
    fcsBox->setColumnLayout( 0, Qt::Horizontal );

    TQBoxLayout *fLay = new TQVBoxLayout(fcsBox->layout(),
        KDialog::spacingHint());

    TQBoxLayout *cLay = new TQHBoxLayout(fLay);
    TQLabel *fLabel = new TQLabel(i18n("&Policy:"), fcsBox);
    cLay->addWidget(fLabel, 0);
    focusCombo =  new TQComboBox(false, fcsBox);
    focusCombo->insertItem(i18n("Click to Focus"), CLICK_TO_FOCUS);
    focusCombo->insertItem(i18n("Focus Follows Mouse"), FOCUS_FOLLOWS_MOUSE);
    focusCombo->insertItem(i18n("Focus Under Mouse"), FOCUS_UNDER_MOUSE);
    focusCombo->insertItem(i18n("Focus Strictly Under Mouse"), FOCUS_STRICTLY_UNDER_MOUSE);
    cLay->addWidget(focusCombo,1 ,Qt::AlignLeft);
    fLabel->setBuddy(focusCombo);

    // FIXME, when more policies have been added to TWin
    wtstr = i18n("The focus policy is used to determine the active window, i.e."
                                      " the window you can work in. <ul>"
                                      " <li><em>Click to focus:</em> A window becomes active when you click into it."
                                      " This is the behavior you might know from other operating systems.</li>"
                                      " <li><em>Focus follows mouse:</em> Moving the mouse pointer actively on to a"
                                      " normal window activates it. New windows will receive the focus,"
                                      " without you having to point the mouse at them explicitly."
                                      " Very practical if you are using the mouse a lot.</li>"
                                      " <li><em>Focus under mouse:</em> The window that happens to be under the"
                                      " mouse pointer is active. If the mouse points nowhere, the last window"
                                      " that was under the mouse has focus."
                                      " New windows will not automatically receive the focus.</li>"
                                      " <li><em>Focus strictly under mouse:</em> Only the window under the mouse pointer is"
                                      " active. If the mouse points nowhere, nothing has focus."
                                      " </ul>"
                                      "Note that 'Focus under mouse' and 'Focus strictly under mouse' prevent certain"
                                      " features such as the Alt+Tab walk through windows dialog in the TDE mode"
                                      " from working properly."
                         );
    TQWhatsThis::add( focusCombo, wtstr);
    TQWhatsThis::add(fLabel, wtstr);

    connect(focusCombo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(setAutoRaiseEnabled()) );

    // autoraise delay
    autoRaiseOn = new TQCheckBox(i18n("Auto &raise"), fcsBox);
    fLay->addWidget(autoRaiseOn);
    connect(autoRaiseOn,TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(autoRaiseOnTog(bool)));

    autoRaise = new KIntNumInput(500, fcsBox);
    autoRaise->setLabel(i18n("Dela&y:"), Qt::AlignVCenter|Qt::AlignLeft);
    autoRaise->setRange(0, 3000, 100, true);
    autoRaise->setSteps(100,100);
    autoRaise->setSuffix(i18n(" msec"));
    fLay->addWidget(autoRaise);

    connect(focusCombo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(setDelayFocusEnabled()) );

    delayFocusOn = new TQCheckBox(i18n("Delay focus"), fcsBox);
    fLay->addWidget(delayFocusOn);
    connect(delayFocusOn,TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(delayFocusOnTog(bool)));

    delayFocus = new KIntNumInput(500, fcsBox);
    delayFocus->setLabel(i18n("Dela&y:"), Qt::AlignVCenter|Qt::AlignLeft);
    delayFocus->setRange(0, 3000, 100, true);
    delayFocus->setSteps(100,100);
    delayFocus->setSuffix(i18n(" msec"));
    fLay->addWidget(delayFocus);

    clickRaiseOn = new TQCheckBox(i18n("Click &raises active window"), fcsBox);
    connect(clickRaiseOn,TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(clickRaiseOnTog(bool)));
    fLay->addWidget(clickRaiseOn);

//     fLay->addColSpacing(0,TQMAX(autoRaiseOn->sizeHint().width(),
//                                clickRaiseOn->sizeHint().width()) + 15);

    TQLabel* focusStealingLabel = new TQLabel( i18n( "Focus stealing prevention &level:" ), fcsBox);
    cLay->addWidget(focusStealingLabel, 0);
    focusStealing = new TQComboBox(false, fcsBox);
    focusStealing->insertItem( i18n( "Focus Stealing Prevention Level", "None" ));
    focusStealing->insertItem( i18n( "Focus Stealing Prevention Level", "Low" ));
    focusStealing->insertItem( i18n( "Focus Stealing Prevention Level", "Normal" ));
    focusStealing->insertItem( i18n( "Focus Stealing Prevention Level", "High" ));
    focusStealing->insertItem( i18n( "Focus Stealing Prevention Level", "Extreme" ));
    focusStealingLabel->setBuddy( focusStealing );
    cLay->addWidget(focusStealing,2 ,Qt::AlignLeft);
    wtstr = i18n( "<p>This option specifies how much TWin will try to prevent unwanted focus stealing "
                  "caused by unexpected activation of new windows. (Note: This feature does not "
                  "work with the Focus Under Mouse or Focus Strictly Under Mouse focus policies.)"
                  "<ul>"
                  "<li><em>None:</em> Prevention is turned off "
                  "and new windows always become activated.</li>"
                  "<li><em>Low:</em> Prevention is enabled; when some window does not have support "
                  "for the underlying mechanism and TWin cannot reliably decide whether to "
                  "activate the window or not, it will be activated. This setting may have both "
                  "worse and better results than normal level, depending on the applications.</li>"
                  "<li><em>Normal:</em> Prevention is enabled.</li>"
                  "<li><em>High:</em> New windows get activated only if no window is currently active "
                  "or if they belong to the currently active application. This setting is probably "
                  "not really usable when not using mouse focus policy.</li>"
                  "<li><em>Extreme:</em> All windows must be explicitly activated by the user.</li>"
                  "</ul></p>"
                  "<p>Windows that are prevented from stealing focus are marked as demanding attention, "
                  "which by default means their taskbar entry will be highlighted. This can be changed "
                  "in the Notifications control module.</p>" );
    TQWhatsThis::add( focusStealing, wtstr );
    TQWhatsThis::add( focusStealingLabel, wtstr );
    
    TQWhatsThis::add( autoRaiseOn, i18n("When this option is enabled, a window in the background will automatically"
                                       " come to the front when the mouse pointer has been over it for some time.") );
    wtstr = i18n("This is the delay after which the window that the mouse pointer is over will automatically"
                 " come to the front.");
    TQWhatsThis::add( autoRaise, wtstr );

    TQWhatsThis::add( clickRaiseOn, i18n("When this option is enabled, the active window will be brought to the"
                                        " front when you click somewhere into the window contents. To change"
                                        " it for inactive windows, you need to change the settings"
                                        " in the Actions tab.") );

    TQWhatsThis::add( delayFocusOn, i18n("When this option is enabled, there will be a delay after which the"
                                        " window the mouse pointer is over will become active (receive focus).") );
    TQWhatsThis::add( delayFocus, i18n("This is the delay after which the window the mouse pointer is over"
                                       " will automatically receive focus.") );

    separateScreenFocus = new TQCheckBox( i18n( "S&eparate screen focus" ), fcsBox );
    fLay->addWidget( separateScreenFocus );
    wtstr = i18n( "When this option is enabled, focus operations are limited only to the active Xinerama screen" );
    TQWhatsThis::add( separateScreenFocus, wtstr );

    activeMouseScreen = new TQCheckBox( i18n( "Active &mouse screen" ), fcsBox );
    fLay->addWidget( activeMouseScreen );
    wtstr = i18n( "When this option is enabled, active Xinerama screen (where for example new windows appear)"
                  " is the screen with the mouse pointer. When disabled, the active Xinerama screen is the screen"
                  " with the focused window. This option is by default disabled for Click to focus and"
                  " enabled for other focus policies." );
    TQWhatsThis::add( activeMouseScreen, wtstr );
    connect(focusCombo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(updateActiveMouseScreen()));

    if (!TQApplication::desktop()->isVirtualDesktop() ||
        TQApplication::desktop()->numScreens() == 1) // No Ximerama 
    {
        separateScreenFocus->hide();
        activeMouseScreen->hide();
    }

    lay->addWidget(fcsBox);

    kbdBox = new TQButtonGroup(i18n("Navigation"), this);
    kbdBox->setColumnLayout( 0, Qt::Horizontal );
    TQVBoxLayout *kLay = new TQVBoxLayout(kbdBox->layout(), KDialog::spacingHint());

    altTabPopup = new TQCheckBox( i18n("Show window list while switching windows"), kbdBox );
    kLay->addWidget( altTabPopup );

    wtstr = i18n("Hold down the Alt key and press the Tab key repeatedly to walk"
                 " through the windows on the current desktop (the Alt+Tab"
                 " combination can be reconfigured).\n\n"
                 "If this checkbox is checked"
                 " a popup widget is shown, displaying the icons of all windows to"
                 " walk through and the title of the currently selected one.\n\n"
                 "Otherwise, the focus is passed to a new window each time Tab"
                 " is pressed, with no popup widget.  In addition, the previously"
                 " activated window will be sent to the back in this mode.");
    TQWhatsThis::add( altTabPopup, wtstr );
    connect(focusCombo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(updateAltTabMode()));

    traverseAll = new TQCheckBox( i18n( "&Traverse windows on all desktops" ), kbdBox );
    kLay->addWidget( traverseAll );

    wtstr = i18n( "Leave this option disabled if you want to limit walking through"
                  " windows to the current desktop." );
    TQWhatsThis::add( traverseAll, wtstr );

    rollOverDesktops = new TQCheckBox( i18n("Desktop navi&gation wraps around"), kbdBox );
    kLay->addWidget(rollOverDesktops);

    wtstr = i18n( "Enable this option if you want keyboard or active desktop border navigation beyond"
                  " the edge of a desktop to take you to the opposite edge of the new desktop." );
    TQWhatsThis::add( rollOverDesktops, wtstr );

    showPopupinfo = new TQCheckBox( i18n("Popup &desktop name on desktop switch"), kbdBox );
    kLay->addWidget(showPopupinfo);

    wtstr = i18n( "Enable this option if you wish to see the current desktop"
                  " name popup whenever the current desktop is changed." );
    TQWhatsThis::add( showPopupinfo, wtstr );

    lay->addWidget(kbdBox);

    lay->addStretch();

    // Any changes goes to slotChanged()
    connect(focusCombo, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));
    connect(fcsBox, TQT_SIGNAL(clicked(int)), TQT_SLOT(changed()));
    connect(autoRaise, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
    connect(delayFocus, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
    connect(separateScreenFocus, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect(activeMouseScreen, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect(altTabPopup, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect(traverseAll, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect(rollOverDesktops, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect(showPopupinfo, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect(focusStealing, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));

    load();
}


int KFocusConfig::getFocus()
{
    return focusCombo->currentItem();
}

void KFocusConfig::setFocus(int foc)
{
    focusCombo->setCurrentItem(foc);

    // this will disable/hide the auto raise delay widget if focus==click
    setAutoRaiseEnabled();
    updateAltTabMode();
}

void KFocusConfig::updateAltTabMode()
{
    // not KDE-style Alt+Tab with unreasonable focus policies
    altTabPopup->setEnabled( focusCombo->currentItem() == 0 || focusCombo->currentItem() == 1 );
}

void KFocusConfig::setAutoRaiseInterval(int tb)
{
    autoRaise->setValue(tb);
}

void KFocusConfig::setDelayFocusInterval(int tb)
{
    delayFocus->setValue(tb);
}

int KFocusConfig::getAutoRaiseInterval()
{
    return autoRaise->value();
}

int KFocusConfig::getDelayFocusInterval()
{
    return delayFocus->value();
}

void KFocusConfig::setAutoRaise(bool on)
{
    autoRaiseOn->setChecked(on);
}

void KFocusConfig::setDelayFocus(bool on)
{
    delayFocusOn->setChecked(on);
}

void KFocusConfig::setClickRaise(bool on)
{
    clickRaiseOn->setChecked(on);
}

void KFocusConfig::setAutoRaiseEnabled()
{
    // the auto raise related widgets are: autoRaise
    if ( focusCombo->currentItem() != CLICK_TO_FOCUS )
    {
        autoRaiseOn->setEnabled(true);
        autoRaiseOnTog(autoRaiseOn->isChecked());
    }
    else
    {
        autoRaiseOn->setEnabled(false);
        autoRaiseOnTog(false);
    }
}

void KFocusConfig::setDelayFocusEnabled()
{
    // the delayed focus related widgets are: delayFocus
    if ( focusCombo->currentItem() != CLICK_TO_FOCUS )
    {
        delayFocusOn->setEnabled(true);
        delayFocusOnTog(delayFocusOn->isChecked());
    }
    else
    {
        delayFocusOn->setEnabled(false);
        delayFocusOnTog(false);
    }
}

void KFocusConfig::autoRaiseOnTog(bool a) {
    autoRaise->setEnabled(a);
    clickRaiseOn->setEnabled( !a );
}

void KFocusConfig::delayFocusOnTog(bool a) {
    delayFocus->setEnabled(a);
}

void KFocusConfig::clickRaiseOnTog(bool ) {
}

void KFocusConfig::setSeparateScreenFocus(bool s) {
    separateScreenFocus->setChecked(s);
}

void KFocusConfig::setActiveMouseScreen(bool a) {
    activeMouseScreen->setChecked(a);
}

void KFocusConfig::updateActiveMouseScreen()
{
    // on by default for non click to focus policies
    TDEConfigGroup cfg( config, "Windows" );
    if( !cfg.hasKey( KWIN_ACTIVE_MOUSE_SCREEN ))
        setActiveMouseScreen( focusCombo->currentItem() != 0 );
}

void KFocusConfig::setAltTabMode(bool a) {
    altTabPopup->setChecked(a);
}

void KFocusConfig::setTraverseAll(bool a) {
    traverseAll->setChecked(a);
}

void KFocusConfig::setRollOverDesktops(bool a) {
    rollOverDesktops->setChecked(a);
}

void KFocusConfig::setShowPopupinfo(bool a) {
    showPopupinfo->setChecked(a);
}

void KFocusConfig::setFocusStealing(int l) {
    l = KMAX( 0, KMIN( 4, l ));
    focusStealing->setCurrentItem(l);
}

void KFocusConfig::load( void )
{
    TQString key;

    config->setGroup( "Windows" );

    key = config->readEntry(KWIN_FOCUS);
    if( key == "ClickToFocus")
        setFocus(CLICK_TO_FOCUS);
    else if( key == "FocusFollowsMouse")
        setFocus(FOCUS_FOLLOWS_MOUSE);
    else if(key == "FocusUnderMouse")
        setFocus(FOCUS_UNDER_MOUSE);
    else if(key == "FocusStrictlyUnderMouse")
        setFocus(FOCUS_STRICTLY_UNDER_MOUSE);

    int k = config->readNumEntry(KWIN_AUTORAISE_INTERVAL,750);
    setAutoRaiseInterval(k);

    k = config->readNumEntry(KWIN_DELAYFOCUS_INTERVAL,750);
    setDelayFocusInterval(k);

    key = config->readEntry(KWIN_AUTORAISE);
    setAutoRaise(key == "on");
    key = config->readEntry(KWIN_DELAYFOCUS);
    setDelayFocus(key == "on");
    key = config->readEntry(KWIN_CLICKRAISE);
    setClickRaise(key != "off");
    setAutoRaiseEnabled();      // this will disable/hide the auto raise delay widget if focus==click
    setDelayFocusEnabled();
    
    setSeparateScreenFocus( config->readBoolEntry(KWIN_SEPARATE_SCREEN_FOCUS, false));
    // on by default for non click to focus policies
    setActiveMouseScreen( config->readBoolEntry(KWIN_ACTIVE_MOUSE_SCREEN, focusCombo->currentItem() != 0 ));

    key = config->readEntry(KWIN_ALTTABMODE, "KDE");
    setAltTabMode(key == "KDE");

    setRollOverDesktops( config->readBoolEntry(KWIN_ROLL_OVER_DESKTOPS, true ));

    config->setGroup( "PopupInfo" );
    setShowPopupinfo( config->readBoolEntry(KWIN_SHOW_POPUP, false ));

    // setFocusStealing( config->readNumEntry(KWIN_FOCUS_STEALING, 2 ));
    // TODO default to low for now
    setFocusStealing( config->readNumEntry(KWIN_FOCUS_STEALING, 1 ));

    config->setGroup( "TabBox" );
    setTraverseAll( config->readBoolEntry(KWIN_TRAVERSE_ALL, false ));

    config->setGroup("Desktops");
    emit TDECModule::changed(false);
}

void KFocusConfig::save( void )
{
    int v;

    config->setGroup( "Windows" );

    v = getFocus();
    if (v == CLICK_TO_FOCUS)
        config->writeEntry(KWIN_FOCUS,"ClickToFocus");
    else if (v == FOCUS_UNDER_MOUSE)
        config->writeEntry(KWIN_FOCUS,"FocusUnderMouse");
    else if (v == FOCUS_STRICTLY_UNDER_MOUSE)
        config->writeEntry(KWIN_FOCUS,"FocusStrictlyUnderMouse");
    else
        config->writeEntry(KWIN_FOCUS,"FocusFollowsMouse");

    v = getAutoRaiseInterval();
    if (v <0) v = 0;
    config->writeEntry(KWIN_AUTORAISE_INTERVAL,v);

    v = getDelayFocusInterval();
    if (v <0) v = 0;
    config->writeEntry(KWIN_DELAYFOCUS_INTERVAL,v);

    if (autoRaiseOn->isChecked())
        config->writeEntry(KWIN_AUTORAISE, "on");
    else
        config->writeEntry(KWIN_AUTORAISE, "off");

    if (delayFocusOn->isChecked())
        config->writeEntry(KWIN_DELAYFOCUS, "on");
    else
        config->writeEntry(KWIN_DELAYFOCUS, "off");

    if (clickRaiseOn->isChecked())
        config->writeEntry(KWIN_CLICKRAISE, "on");
    else
        config->writeEntry(KWIN_CLICKRAISE, "off");

    config->writeEntry(KWIN_SEPARATE_SCREEN_FOCUS, separateScreenFocus->isChecked());
    config->writeEntry(KWIN_ACTIVE_MOUSE_SCREEN, activeMouseScreen->isChecked());

    if (altTabPopup->isChecked())
        config->writeEntry(KWIN_ALTTABMODE, "KDE");
    else
        config->writeEntry(KWIN_ALTTABMODE, "CDE");

    config->writeEntry( KWIN_ROLL_OVER_DESKTOPS, rollOverDesktops->isChecked());

    config->setGroup( "PopupInfo" );
    config->writeEntry( KWIN_SHOW_POPUP, showPopupinfo->isChecked());

    config->writeEntry(KWIN_FOCUS_STEALING, focusStealing->currentItem());

    config->setGroup( "TabBox" );
    config->writeEntry( KWIN_TRAVERSE_ALL , traverseAll->isChecked());

    config->setGroup("Desktops");

    if (standAlone)
    {
        config->sync();
        if ( !kapp->dcopClient()->isAttached() )
            kapp->dcopClient()->attach();
        kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
    }
    emit TDECModule::changed(false);
}

void KFocusConfig::defaults()
{
    setAutoRaiseInterval(0);
    setDelayFocusInterval(0);
    setFocus(CLICK_TO_FOCUS);
    setAutoRaise(false);
    setDelayFocus(false);
    setClickRaise(true);
    setSeparateScreenFocus( false );
    // on by default for non click to focus policies
    setActiveMouseScreen( focusCombo->currentItem() != 0 );
    setAltTabMode(true);
    setTraverseAll( false );
    setRollOverDesktops(true);
    setShowPopupinfo(false);
    // setFocusStealing(2);
    // TODO default to low for now
    setFocusStealing(1);
    emit TDECModule::changed(true);
}

KAdvancedConfig::~KAdvancedConfig ()
{
    if (standAlone)
        delete config;
}

KAdvancedConfig::KAdvancedConfig (bool _standAlone, TDEConfig *_config, TQWidget *parent, const char *)
    : TDECModule(parent, "kcmkwm"), config(_config), standAlone(_standAlone)
{
    TQString wtstr;
    TQBoxLayout *lay = new TQVBoxLayout (this, 0, KDialog::spacingHint());

    //iTLabel = new TQLabel(i18n("  Allowed overlap:\n"
    //                         "(% of desktop space)"),
    //             plcBox);
    //iTLabel->setAlignment(AlignTop|AlignHCenter);
    //pLay->addWidget(iTLabel,1,1);

    //interactiveTrigger = new TQSpinBox(0, 500, 1, plcBox);
    //pLay->addWidget(interactiveTrigger,1,2);

    //pLay->addRowSpacing(2,KDialog::spacingHint());

    //lay->addWidget(plcBox);

    shBox = new TQVButtonGroup(i18n("Shading"), this);

    animateShade = new TQCheckBox(i18n("Anima&te"), shBox);
    TQWhatsThis::add(animateShade, i18n("Animate the action of reducing the window to its titlebar (shading)"
                                       " as well as the expansion of a shaded window") );

    shadeHoverOn = new TQCheckBox(i18n("&Enable hover"), shBox);

    connect(shadeHoverOn, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(shadeHoverChanged(bool)));

    shadeHover = new KIntNumInput(500, shBox);
    shadeHover->setLabel(i18n("Dela&y:"), Qt::AlignVCenter|Qt::AlignLeft);
    shadeHover->setRange(0, 3000, 100, true);
    shadeHover->setSteps(100, 100);
    shadeHover->setSuffix(i18n(" msec"));

    TQWhatsThis::add(shadeHoverOn, i18n("If Shade Hover is enabled, a shaded window will un-shade automatically "
                                       "when the mouse pointer has been over the title bar for some time."));

    wtstr = i18n("Sets the time in milliseconds before the window unshades "
                "when the mouse pointer goes over the shaded window.");
    TQWhatsThis::add(shadeHover, wtstr);

    lay->addWidget(shBox);

    // Any changes goes to slotChanged()
    connect(animateShade, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
    connect(shadeHoverOn, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
    connect(shadeHover, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));

    electricBox = new TQVButtonGroup(i18n("Active Desktop Borders"), this);
    electricBox->setMargin(15);

    TQWhatsThis::add( electricBox, i18n("If this option is enabled, moving the mouse to a screen border"
       " will change your desktop. This is e.g. useful if you want to drag windows from one desktop"
       " to the other.") );
    active_disable = new TQRadioButton(i18n("D&isabled"), electricBox);
    active_move    = new TQRadioButton(i18n("Only &when moving windows"), electricBox);
    active_always  = new TQRadioButton(i18n("A&lways enabled"), electricBox);

    delays = new KIntNumInput(10, electricBox);
    delays->setRange(0, MAX_EDGE_RES, 50, true);
    delays->setSuffix(i18n(" msec"));
    delays->setLabel(i18n("Desktop &switch delay:"));
    TQWhatsThis::add( delays, i18n("Here you can set a delay for switching desktops using the active"
       " borders feature. Desktops will be switched after the mouse has been pushed against a screen border"
       " for the specified number of milliseconds.") );

    connect( electricBox, TQT_SIGNAL(clicked(int)), this, TQT_SLOT(setEBorders()));

    // Any changes goes to slotChanged()
    connect(electricBox, TQT_SIGNAL(clicked(int)), TQT_SLOT(changed()));
    connect(delays, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));

    lay->addWidget(electricBox);

    hideUtilityWindowsForInactive = new TQCheckBox( i18n( "Hide utility windows for inactive applications" ), this );
    TQWhatsThis::add( hideUtilityWindowsForInactive,
        i18n( "When turned on, utility windows (tool windows, torn-off menus,...) of inactive applications will be"
              " hidden and will be shown only when the application becomes active. Note that applications"
              " have to mark the windows with the proper window type for this feature to work." ));
    connect(hideUtilityWindowsForInactive, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
    lay->addWidget( hideUtilityWindowsForInactive );

    lay->addStretch();

    load();

}

void KAdvancedConfig::setShadeHover(bool on) {
    shadeHoverOn->setChecked(on);
    shadeHover->setEnabled(on);
}

void KAdvancedConfig::setShadeHoverInterval(int k) {
    shadeHover->setValue(k);
}

int KAdvancedConfig::getShadeHoverInterval() {

    return shadeHover->value();
}

void KAdvancedConfig::shadeHoverChanged(bool a) {
    shadeHover->setEnabled(a);
}

void KAdvancedConfig::setAnimateShade(bool a) {
    animateShade->setChecked(a);
}

void KAdvancedConfig::setHideUtilityWindowsForInactive(bool s) {
    hideUtilityWindowsForInactive->setChecked( s );
}

void KAdvancedConfig::load( void )
{
    config->setGroup( "Windows" );

    setAnimateShade(config->readBoolEntry(KWIN_ANIMSHADE, true));
    setShadeHover(config->readBoolEntry(KWIN_SHADEHOVER, false));
    setShadeHoverInterval(config->readNumEntry(KWIN_SHADEHOVER_INTERVAL, 250));

    setElectricBorders(config->readNumEntry(KWM_ELECTRIC_BORDER, 0));
    setElectricBorderDelay(config->readNumEntry(KWM_ELECTRIC_BORDER_DELAY, 150));

    setHideUtilityWindowsForInactive( config->readBoolEntry( KWIN_HIDE_UTILITY, true ));

    emit TDECModule::changed(false);
}

void KAdvancedConfig::save( void )
{
    int v;

    config->setGroup( "Windows" );
    config->writeEntry(KWIN_ANIMSHADE, animateShade->isChecked());
    if (shadeHoverOn->isChecked())
        config->writeEntry(KWIN_SHADEHOVER, "on");
    else
        config->writeEntry(KWIN_SHADEHOVER, "off");

    v = getShadeHoverInterval();
    if (v<0) v = 0;
    config->writeEntry(KWIN_SHADEHOVER_INTERVAL, v);

    config->writeEntry(KWM_ELECTRIC_BORDER, getElectricBorders());
    config->writeEntry(KWM_ELECTRIC_BORDER_DELAY,getElectricBorderDelay());

    config->writeEntry(KWIN_HIDE_UTILITY, hideUtilityWindowsForInactive->isChecked());

    if (standAlone)
    {
        config->sync();
        if ( !kapp->dcopClient()->isAttached() )
            kapp->dcopClient()->attach();
        kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
    }
    emit TDECModule::changed(false);
}

void KAdvancedConfig::defaults()
{
    setAnimateShade(true);
    setShadeHover(false);
    setShadeHoverInterval(250);
    setElectricBorders(0);
    setElectricBorderDelay(150);
    setHideUtilityWindowsForInactive( true );
    emit TDECModule::changed(true);
}

void KAdvancedConfig::setEBorders()
{
    delays->setEnabled(!active_disable->isChecked());
}

int KAdvancedConfig::getElectricBorders()
{
    if (active_move->isChecked())
       return 1;
    if (active_always->isChecked())
       return 2;
    return 0;
}

int KAdvancedConfig::getElectricBorderDelay()
{
    return delays->value();
}

void KAdvancedConfig::setElectricBorders(int i){
    switch(i)
    {
      case 1: active_move->setChecked(true); break;
      case 2: active_always->setChecked(true); break;
      default: active_disable->setChecked(true); break;
    }
    setEBorders();
}

void KAdvancedConfig::setElectricBorderDelay(int delay)
{
    delays->setValue(delay);
}


KMovingConfig::~KMovingConfig ()
{
    if (standAlone)
        delete config;
}

KMovingConfig::KMovingConfig (bool _standAlone, TDEConfig *_config, TQWidget *parent, const char *)
    : TDECModule(parent, "kcmkwm"), config(_config), standAlone(_standAlone)
{
    TQString wtstr;
    TQBoxLayout *lay = new TQVBoxLayout (this, 0, KDialog::spacingHint());

    windowsBox = new TQButtonGroup(i18n("Windows"), this);
    windowsBox->setColumnLayout( 0, Qt::Horizontal );

    TQBoxLayout *wLay = new TQVBoxLayout (windowsBox->layout(), KDialog::spacingHint());

    TQBoxLayout *bLay = new TQVBoxLayout;
    wLay->addLayout(bLay);

    opaque = new TQCheckBox(i18n("Di&splay content in moving windows"), windowsBox);
    bLay->addWidget(opaque);
    TQWhatsThis::add( opaque, i18n("Enable this option if you want a window's content to be fully shown"
                                  " while moving it, instead of just showing a window 'skeleton'. The result may not be satisfying"
                                  " on slow machines without graphic acceleration.") );

    resizeOpaqueOn = new TQCheckBox(i18n("Display content in &resizing windows"), windowsBox);
    bLay->addWidget(resizeOpaqueOn);
    TQWhatsThis::add( resizeOpaqueOn, i18n("Enable this option if you want a window's content to be shown"
                                          " while resizing it, instead of just showing a window 'skeleton'. The result may not be satisfying"
                                          " on slow machines.") );

    geometryTipOn = new TQCheckBox(i18n("Display window &geometry when moving or resizing"), windowsBox);
    bLay->addWidget(geometryTipOn);
    TQWhatsThis::add(geometryTipOn, i18n("Enable this option if you want a window's geometry to be displayed"
                                        " while it is being moved or resized. The window position relative"
                                        " to the top-left corner of the screen is displayed together with"
                                        " its size."));

    TQGridLayout *rLay = new TQGridLayout(2,3);
    bLay->addLayout(TQT_TQLAYOUT(rLay));
    rLay->setColStretch(0,0);
    rLay->setColStretch(1,1);

    minimizeAnimOn = new TQCheckBox(i18n("Animate minimi&ze and restore"),
                                   windowsBox);
    TQWhatsThis::add( minimizeAnimOn, i18n("Enable this option if you want an animation shown when"
                                          " windows are minimized or restored." ) );
    rLay->addWidget(minimizeAnimOn,0,0);

    minimizeAnimSlider = new TQSlider(0,10,10,0,Qt::Horizontal, windowsBox);
    minimizeAnimSlider->setSteps(1, 1);
    // TQSlider::Below clashes with a X11/X.h #define
    #undef Below
    minimizeAnimSlider->setTickmarks(TQSlider::Below);
    rLay->addMultiCellWidget(minimizeAnimSlider,0,0,1,2);

    connect(minimizeAnimOn, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(setMinimizeAnim(bool)));
    connect(minimizeAnimSlider, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(setMinimizeAnimSpeed(int)));

    minimizeAnimSlowLabel= new TQLabel(i18n("Slow"),windowsBox);
    minimizeAnimSlowLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    rLay->addWidget(minimizeAnimSlowLabel,1,1);

    minimizeAnimFastLabel= new TQLabel(i18n("Fast"),windowsBox);
    minimizeAnimFastLabel->setAlignment(Qt::AlignTop|Qt::AlignRight);
    rLay->addWidget(minimizeAnimFastLabel,1,2);

    wtstr = i18n("Here you can set the speed of the animation shown when windows are"
                 " minimized and restored. ");
    TQWhatsThis::add( minimizeAnimSlider, wtstr );
    TQWhatsThis::add( minimizeAnimSlowLabel, wtstr );
    TQWhatsThis::add( minimizeAnimFastLabel, wtstr );

    moveResizeMaximized = new TQCheckBox( i18n("Allow moving and resizing o&f maximized windows"), windowsBox);
    bLay->addWidget(moveResizeMaximized);
    TQWhatsThis::add(moveResizeMaximized, i18n("When enabled, this feature activates the border of maximized windows"
                                              " and allows you to move or resize them,"
                                              " just like for normal windows"));

    TQBoxLayout *vLay = new TQHBoxLayout(bLay);

    TQLabel *plcLabel = new TQLabel(i18n("&Placement:"),windowsBox);

    placementCombo = new TQComboBox(false, windowsBox);
    placementCombo->insertItem(i18n("Smart"), SMART_PLACEMENT);
    placementCombo->insertItem(i18n("Maximizing"), MAXIMIZING_PLACEMENT);
    placementCombo->insertItem(i18n("Cascade"), CASCADE_PLACEMENT);
    placementCombo->insertItem(i18n("Random"), RANDOM_PLACEMENT);
    placementCombo->insertItem(i18n("Centered"), CENTERED_PLACEMENT);
    placementCombo->insertItem(i18n("Zero-Cornered"), ZEROCORNERED_PLACEMENT);
    // CT: disabling is needed as long as functionality misses in twin
    //placementCombo->insertItem(i18n("Interactive"), INTERACTIVE_PLACEMENT);
    //placementCombo->insertItem(i18n("Manual"), MANUAL_PLACEMENT);
    placementCombo->setCurrentItem(SMART_PLACEMENT);

    // FIXME, when more policies have been added to TWin
    wtstr = i18n("The placement policy determines where a new window"
                 " will appear on the desktop."
                 " <ul>"
                 " <li><em>Smart</em> will try to achieve a minimum overlap of windows</li>"
                 " <li><em>Maximizing</em> will try to maximize every window to fill the whole screen."
                 " It might be useful to selectively affect placement of some windows using"
                 " the window-specific settings.</li>"
                 " <li><em>Cascade</em> will cascade the windows</li>"
                 " <li><em>Random</em> will use a random position</li>"
                 " <li><em>Centered</em> will place the window centered</li>"
                 " <li><em>Zero-Cornered</em> will place the window in the top-left corner</li>"
                 "</ul>") ;

    TQWhatsThis::add( plcLabel, wtstr);
    TQWhatsThis::add( placementCombo, wtstr);

    plcLabel->setBuddy(placementCombo);
    vLay->addWidget(plcLabel, 0);
    vLay->addWidget(placementCombo, 1, Qt::AlignLeft);

    bLay->addSpacing(10);

    lay->addWidget(windowsBox);

    //iTLabel = new TQLabel(i18n("  Allowed overlap:\n"
    //                         "(% of desktop space)"),
    //             plcBox);
    //iTLabel->setAlignment(AlignTop|AlignHCenter);
    //pLay->addWidget(iTLabel,1,1);

    //interactiveTrigger = new TQSpinBox(0, 500, 1, plcBox);
    //pLay->addWidget(interactiveTrigger,1,2);

    //pLay->addRowSpacing(2,KDialog::spacingHint());

    //lay->addWidget(plcBox);


    //CT 15mar98 - add EdgeResistance, BorderAttractor, WindowsAttractor config
    MagicBox = new TQVButtonGroup(i18n("Snap Zones"), this);
    MagicBox->setMargin(15);

    BrdrSnap = new KIntNumInput(10, MagicBox);
    BrdrSnap->setSpecialValueText( i18n("none") );
    BrdrSnap->setRange( 0, MAX_BRDR_SNAP);
    BrdrSnap->setLabel(i18n("&Border snap zone:"));
    BrdrSnap->setSteps(1,10);
    TQWhatsThis::add( BrdrSnap, i18n("Here you can set the snap zone for screen borders, i.e."
                                    " the 'strength' of the magnetic field which will make windows snap to the border when"
                                    " moved near it.") );

    WndwSnap = new KIntNumInput(10, MagicBox);
    WndwSnap->setSpecialValueText( i18n("none") );
    WndwSnap->setRange( 0, MAX_WNDW_SNAP);
    WndwSnap->setLabel(i18n("&Window snap zone:"));
    BrdrSnap->setSteps(1,10);
    TQWhatsThis::add( WndwSnap, i18n("Here you can set the snap zone for windows, i.e."
                                    " the 'strength' of the magnetic field which will make windows snap to each other when"
                                    " they're moved near another window.") );

    OverlapSnap=new TQCheckBox(i18n("Snap windows onl&y when overlapping"),MagicBox);
    TQWhatsThis::add( OverlapSnap, i18n("Here you can set that windows will be only"
                                       " snapped if you try to overlap them, i.e. they will not be snapped if the windows"
                                       " comes only near another window or border.") );

    lay->addWidget(MagicBox);
    lay->addStretch();

    load();

    // Any changes goes to slotChanged()
    connect( opaque, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect( resizeOpaqueOn, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect( geometryTipOn, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
    connect( minimizeAnimOn, TQT_SIGNAL(clicked() ), TQT_SLOT(changed()));
    connect( minimizeAnimSlider, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
    connect( moveResizeMaximized, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
    connect( placementCombo, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));
    connect( BrdrSnap, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
    connect( BrdrSnap, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(slotBrdrSnapChanged(int)));
    connect( WndwSnap, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
    connect( WndwSnap, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(slotWndwSnapChanged(int)));
    connect( OverlapSnap, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));

    // To get suffix to BrdrSnap and WndwSnap inputs with default values.
    slotBrdrSnapChanged(BrdrSnap->value());
    slotWndwSnapChanged(WndwSnap->value());
}

int KMovingConfig::getMove()
{
    return (opaque->isChecked())? OPAQUE : TRANSPARENT;
}

void KMovingConfig::setMove(int trans)
{
    opaque->setChecked(trans == OPAQUE);
}

void KMovingConfig::setGeometryTip(bool showGeometryTip)
{
    geometryTipOn->setChecked(showGeometryTip);
}

bool KMovingConfig::getGeometryTip()
{
    return geometryTipOn->isChecked();
}

// placement policy --- CT 31jan98 ---
int KMovingConfig::getPlacement()
{
    return placementCombo->currentItem();
}

void KMovingConfig::setPlacement(int plac)
{
    placementCombo->setCurrentItem(plac);
}

bool KMovingConfig::getMinimizeAnim()
{
    return minimizeAnimOn->isChecked();
}

int KMovingConfig::getMinimizeAnimSpeed()
{
    return minimizeAnimSlider->value();
}

void KMovingConfig::setMinimizeAnim(bool anim)
{
    minimizeAnimOn->setChecked( anim );
    minimizeAnimSlider->setEnabled( anim );
    minimizeAnimSlowLabel->setEnabled( anim );
    minimizeAnimFastLabel->setEnabled( anim );
}

void KMovingConfig::setMinimizeAnimSpeed(int speed)
{
    minimizeAnimSlider->setValue(speed);
}

int KMovingConfig::getResizeOpaque()
{
    return (resizeOpaqueOn->isChecked())? RESIZE_OPAQUE : RESIZE_TRANSPARENT;
}

void KMovingConfig::setResizeOpaque(int opaque)
{
    resizeOpaqueOn->setChecked(opaque == RESIZE_OPAQUE);
}

void KMovingConfig::setMoveResizeMaximized(bool a) {
    moveResizeMaximized->setChecked(a);
}

void KMovingConfig::slotBrdrSnapChanged(int value) {
    BrdrSnap->setSuffix(i18n(" pixel", " pixels", value));
}

void KMovingConfig::slotWndwSnapChanged(int value) {
    WndwSnap->setSuffix(i18n(" pixel", " pixels", value));
}

void KMovingConfig::load( void )
{
    TQString key;

    config->setGroup( "Windows" );

    key = config->readEntry(KWIN_MOVE, "Opaque");
    if( key == "Transparent")
        setMove(TRANSPARENT);
    else if( key == "Opaque")
        setMove(OPAQUE);

    //CT 17Jun1998 - variable animation speed from 0 (none!!) to 10 (max)
    bool anim = config->readBoolEntry(KWIN_MINIMIZE_ANIM, true );
    int animSpeed = config->readNumEntry(KWIN_MINIMIZE_ANIM_SPEED, 5);
    if( animSpeed < 1 ) animSpeed = 0;
    if( animSpeed > 10 ) animSpeed = 10;
    setMinimizeAnim( anim );
    setMinimizeAnimSpeed( animSpeed );

    // DF: please keep the default consistent with twin (options.cpp line 145)
    key = config->readEntry(KWIN_RESIZE_OPAQUE, "Opaque");
    if( key == "Opaque")
        setResizeOpaque(RESIZE_OPAQUE);
    else if ( key == "Transparent")
        setResizeOpaque(RESIZE_TRANSPARENT);

    //KS 10Jan2003 - Geometry Tip during window move/resize
    bool showGeomTip = config->readBoolEntry(KWIN_GEOMETRY, false);
    setGeometryTip( showGeomTip );

    // placement policy --- CT 19jan98 ---
    key = config->readEntry(KWIN_PLACEMENT);
    //CT 13mar98 interactive placement
//   if( key.left(11) == "interactive") {
//     setPlacement(INTERACTIVE_PLACEMENT);
//     int comma_pos = key.find(',');
//     if (comma_pos < 0)
//       interactiveTrigger->setValue(0);
//     else
//       interactiveTrigger->setValue (key.right(key.length()
//                           - comma_pos).toUInt(0));
//     iTLabel->setEnabled(true);
//     interactiveTrigger->show();
//   }
//   else {
//     interactiveTrigger->setValue(0);
//     iTLabel->setEnabled(false);
//     interactiveTrigger->hide();
    if( key == "Random")
        setPlacement(RANDOM_PLACEMENT);
    else if( key == "Cascade")
        setPlacement(CASCADE_PLACEMENT); //CT 31jan98
    //CT 31mar98 manual placement
    else if( key == "manual")
        setPlacement(MANUAL_PLACEMENT);
    else if( key == "Centered")
        setPlacement(CENTERED_PLACEMENT);
    else if( key == "ZeroCornered")
        setPlacement(ZEROCORNERED_PLACEMENT);
    else if( key == "Maximizing")
        setPlacement(MAXIMIZING_PLACEMENT);
    else
        setPlacement(SMART_PLACEMENT);
//  }

    setMoveResizeMaximized(config->readBoolEntry(KWIN_MOVE_RESIZE_MAXIMIZED, false));

    int v;

    v = config->readNumEntry(KWM_BRDR_SNAP_ZONE, KWM_BRDR_SNAP_ZONE_DEFAULT);
    if (v > MAX_BRDR_SNAP) setBorderSnapZone(MAX_BRDR_SNAP);
    else if (v < 0) setBorderSnapZone (0);
    else setBorderSnapZone(v);

    v = config->readNumEntry(KWM_WNDW_SNAP_ZONE, KWM_WNDW_SNAP_ZONE_DEFAULT);
    if (v > MAX_WNDW_SNAP) setWindowSnapZone(MAX_WNDW_SNAP);
    else if (v < 0) setWindowSnapZone (0);
    else setWindowSnapZone(v);

    OverlapSnap->setChecked(config->readBoolEntry("SnapOnlyWhenOverlapping",false));
    emit TDECModule::changed(false);
}

void KMovingConfig::save( void )
{
    int v;

    config->setGroup( "Windows" );

    v = getMove();
    if (v == TRANSPARENT)
        config->writeEntry(KWIN_MOVE,"Transparent");
    else
        config->writeEntry(KWIN_MOVE,"Opaque");

    config->writeEntry(KWIN_GEOMETRY, getGeometryTip());

    // placement policy --- CT 31jan98 ---
    v =getPlacement();
    if (v == RANDOM_PLACEMENT)
        config->writeEntry(KWIN_PLACEMENT, "Random");
    else if (v == CASCADE_PLACEMENT)
        config->writeEntry(KWIN_PLACEMENT, "Cascade");
    else if (v == CENTERED_PLACEMENT)
        config->writeEntry(KWIN_PLACEMENT, "Centered");
    else if (v == ZEROCORNERED_PLACEMENT)
        config->writeEntry(KWIN_PLACEMENT, "ZeroCornered");
    else if (v == MAXIMIZING_PLACEMENT)
        config->writeEntry(KWIN_PLACEMENT, "Maximizing");
//CT 13mar98 manual and interactive placement
//   else if (v == MANUAL_PLACEMENT)
//     config->writeEntry(KWIN_PLACEMENT, "Manual");
//   else if (v == INTERACTIVE_PLACEMENT) {
//       TQString tmpstr = TQString("Interactive,%1").arg(interactiveTrigger->value());
//       config->writeEntry(KWIN_PLACEMENT, tmpstr);
//   }
    else
        config->writeEntry(KWIN_PLACEMENT, "Smart");

    config->writeEntry(KWIN_MINIMIZE_ANIM, getMinimizeAnim());
    config->writeEntry(KWIN_MINIMIZE_ANIM_SPEED, getMinimizeAnimSpeed());

    v = getResizeOpaque();
    if (v == RESIZE_OPAQUE)
        config->writeEntry(KWIN_RESIZE_OPAQUE, "Opaque");
    else
        config->writeEntry(KWIN_RESIZE_OPAQUE, "Transparent");

    config->writeEntry(KWIN_MOVE_RESIZE_MAXIMIZED, moveResizeMaximized->isChecked());


    config->writeEntry(KWM_BRDR_SNAP_ZONE,getBorderSnapZone());
    config->writeEntry(KWM_WNDW_SNAP_ZONE,getWindowSnapZone());
    config->writeEntry("SnapOnlyWhenOverlapping",OverlapSnap->isChecked());

    if (standAlone)
    {
        config->sync();
        if ( !kapp->dcopClient()->isAttached() )
            kapp->dcopClient()->attach();
        kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
    }
    emit TDECModule::changed(false);
}

void KMovingConfig::defaults()
{
    setMove(OPAQUE);
    setResizeOpaque(RESIZE_TRANSPARENT);
    setGeometryTip(false);
    setPlacement(SMART_PLACEMENT);
    setMoveResizeMaximized(false);

    //copied from kcontrol/konq/twindesktop, aleXXX
    setWindowSnapZone(KWM_WNDW_SNAP_ZONE_DEFAULT);
    setBorderSnapZone(KWM_BRDR_SNAP_ZONE_DEFAULT);
    OverlapSnap->setChecked(false);

    setMinimizeAnim( true );
    setMinimizeAnimSpeed( 5 );
    emit TDECModule::changed(true);
}

int KMovingConfig::getBorderSnapZone() {
  return BrdrSnap->value();
}

void KMovingConfig::setBorderSnapZone(int pxls) {
  BrdrSnap->setValue(pxls);
}

int KMovingConfig::getWindowSnapZone() {
  return WndwSnap->value();
}

void KMovingConfig::setWindowSnapZone(int pxls) {
  WndwSnap->setValue(pxls);
}

KTranslucencyConfig::~KTranslucencyConfig ()
{
    if (standAlone)
        delete config;
    if (kompmgr)
        kompmgr->detach();
}

KTranslucencyConfig::KTranslucencyConfig (bool _standAlone, TDEConfig *_config, TQWidget *parent, const char *)
    : TDECModule(parent, "kcmkwm"), config(_config), standAlone(_standAlone)
{
  kompmgr = 0L;
  resetKompmgr_ = FALSE;
  TQVBoxLayout *lay = new TQVBoxLayout (this);
  kompmgrAvailable_ = kompmgrAvailable();
  if (!kompmgrAvailable_){
  KActiveLabel *label = new KActiveLabel(i18n("<qt><b>It seems that alpha channel support is not available.</b><br><br>"
                                 "Please make sure you have "
                                 "<a href=\"http://www.freedesktop.org/\">Xorg &ge; 6.8</a>,"
                                 " and installed the composition manager that came with twin.<br>"
                                 "Also, make sure you have the following entries in your XConfig (e.g. /etc/X11/xorg.conf):<br><br>"
                                 "<i>Section \"Extensions\"<br>"
                                 "Option \"Composite\" \"Enable\"<br>"
                                 "EndSection</i><br><br>"
                                 "And if your GPU provides hardware-accelerated Xrender support (mainly nVidia cards):<br><br>"
                                 "<i>Option     \"RenderAccel\" \"true\"</i><br>"
                                 "In <i>Section \"Device\"</i></qt>"), this);
  lay->addWidget(label);
  }
  else
  {
  TQTabWidget *tabW = new TQTabWidget(this);
  TQWidget *tGroup = new TQWidget(tabW);
  TQVBoxLayout *vLay = new TQVBoxLayout (tGroup,KDialog::marginHint(), KDialog::spacingHint());
  vLay->addSpacing(11); // to get the proper gb top offset
  
  onlyDecoTranslucent = new TQCheckBox(i18n("Apply translucency only to decoration"),tGroup);
  vLay->addWidget(onlyDecoTranslucent);
  
  vLay->addSpacing(11);
  
  TQGridLayout *gLay = new TQGridLayout(vLay,4,2,KDialog::spacingHint());
  gLay->setColStretch(1,1);

  activeWindowTransparency = new TQCheckBox(i18n("Active windows:"),tGroup);
  gLay->addWidget(activeWindowTransparency,0,0);
  activeWindowOpacity = new KIntNumInput(100, tGroup);
  activeWindowOpacity->setRange(0,100);
  activeWindowOpacity->setSuffix("%");
  gLay->addWidget(activeWindowOpacity,0,1);

  inactiveWindowTransparency = new TQCheckBox(i18n("Inactive windows:"),tGroup);
  gLay->addWidget(inactiveWindowTransparency,1,0);
  inactiveWindowOpacity = new KIntNumInput(100, tGroup);
  inactiveWindowOpacity->setRange(0,100);
  inactiveWindowOpacity->setSuffix("%");
  gLay->addWidget(inactiveWindowOpacity,1,1);

  movingWindowTransparency = new TQCheckBox(i18n("Moving windows:"),tGroup);
  gLay->addWidget(movingWindowTransparency,2,0);
  movingWindowOpacity = new KIntNumInput(100, tGroup);
  movingWindowOpacity->setRange(0,100);
  movingWindowOpacity->setSuffix("%");
  gLay->addWidget(movingWindowOpacity,2,1);

  dockWindowTransparency = new TQCheckBox(i18n("Dock windows:"),tGroup);
  gLay->addWidget(dockWindowTransparency,3,0);
  dockWindowOpacity = new KIntNumInput(100, tGroup);
  dockWindowOpacity->setRange(0,100);
  dockWindowOpacity->setSuffix("%");
  gLay->addWidget(dockWindowOpacity,3,1);
  
  vLay->addSpacing(11);

  keepAboveAsActive = new TQCheckBox(i18n("Treat 'keep above' windows as active ones"),tGroup);
  vLay->addWidget(keepAboveAsActive);

  disableARGB = new TQCheckBox(i18n("Disable ARGB windows (ignores window alpha maps, fixes gtk1 apps)"),tGroup);
  vLay->addWidget(disableARGB);
  if (TDECompositor == "compton-tde") {
      disableARGB->hide();
  }

  useOpenGL = new TQCheckBox(i18n("Use OpenGL compositor (best performance)"),tGroup);
  vLay->addWidget(useOpenGL);
  blurBackground = new TQCheckBox(i18n("Blur the background of transparent windows"),tGroup);
  vLay->addWidget(blurBackground);
  greyscaleBackground = new TQCheckBox(i18n("Desaturate the background of transparent windows"),tGroup);
  vLay->addWidget(greyscaleBackground);
  if (TDECompositor != "compton-tde") {
      useOpenGL->hide();
      blurBackground->hide();
      greyscaleBackground->hide();
  }

  vLay->addStretch();
  tabW->addTab(tGroup, i18n("Opacity"));

  TQWidget *sGroup = new TQWidget(tabW);
//   sGroup->setCheckable(TRUE);
  TQVBoxLayout *vLay2 = new TQVBoxLayout (sGroup,11,6);
  vLay2->addSpacing(11); // to get the proper gb top offset
  useShadows = new TQCheckBox(i18n("Use shadows (standard effects should be disabled in the Styles module if this is checked)"),sGroup);
  vLay2->addWidget(useShadows);
  useShadowsOnMenuWindows = new TQCheckBox(i18n("Use shadows on menus (requires menu fade effect to be disabled in the Styles module)"),sGroup);
  vLay2->addWidget(useShadowsOnMenuWindows);
  useShadowsOnToolTipWindows = new TQCheckBox(i18n("Use shadows on tooltips"),sGroup);
  vLay2->addWidget(useShadowsOnToolTipWindows);
  useShadowsOnDockWindows = new TQCheckBox(i18n("Use shadows on panels"),sGroup);
  vLay2->addWidget(useShadowsOnDockWindows);
  if (TDECompositor != "compton-tde") {
      useShadowsOnMenuWindows->hide();
      useShadowsOnToolTipWindows->hide();
      useShadowsOnDockWindows->hide();
  }

  vLay2->addSpacing(11);

  TQGridLayout *gLay2 = new TQGridLayout(vLay2,6,2);
  gLay2->setColStretch(1,1);

  TQLabel *label2 = new TQLabel(i18n("Base shadow radius:"),sGroup);
  gLay2->addWidget(label2,0,0);
  baseShadowSize = new KIntNumInput(6,sGroup);
  baseShadowSize->setRange(0,32);
//   inactiveWindowShadowSize->setSuffix("px");
  gLay2->addWidget(baseShadowSize,0,1);

  TQLabel *label2a = new TQLabel(i18n("Inactive window distance from background:"),sGroup);
  gLay2->addWidget(label2a,1,0);
  inactiveWindowShadowSize = new KIntNumInput(6,sGroup);
  inactiveWindowShadowSize->setRange(0,16);
//   inactiveWindowShadowSize->setSuffix("px");
  gLay2->addWidget(inactiveWindowShadowSize,1,1);

  TQLabel *label1 = new TQLabel(i18n("Active window distance from background:"),sGroup);
  gLay2->addWidget(label1,2,0);
  activeWindowShadowSize = new KIntNumInput(12,sGroup);
  activeWindowShadowSize->setRange(0,16);
//   activeWindowShadowSize->setSuffix("px");
  gLay2->addWidget(activeWindowShadowSize,2,1);

  TQLabel *label3 = new TQLabel(i18n("Dock distance from background:"),sGroup);
  gLay2->addWidget(label3,3,0);
  dockWindowShadowSize = new KIntNumInput(6,sGroup);
  dockWindowShadowSize->setRange(0,16);
//   dockWindowShadowSize->setSuffix("px");
  gLay2->addWidget(dockWindowShadowSize,3,1);

  TQLabel *label3a = new TQLabel(i18n("Menu distance from background:"),sGroup);
  gLay2->addWidget(label3a,4,0);
  menuWindowShadowSize = new KIntNumInput(6,sGroup);
  menuWindowShadowSize->setRange(0,16);
//   menuWindowShadowSize->setSuffix("px");
  gLay2->addWidget(menuWindowShadowSize,4,1);

  // FIXME
  // Menu control does not work!
  // Menus appear to be controlled by the base shadow radius ONLY
  label3a->hide();
  menuWindowShadowSize->hide();

  TQLabel *label4 = new TQLabel(i18n("Vertical offset:"),sGroup);
  gLay2->addWidget(label4,5,0);
  shadowTopOffset = new KIntNumInput(80,sGroup);
  shadowTopOffset->setSuffix("%");
  shadowTopOffset->setRange(-200,200);
  gLay2->addWidget(shadowTopOffset,5,1);

  TQLabel *label5 = new TQLabel(i18n("Horizontal offset:"),sGroup);
  gLay2->addWidget(label5,6,0);
  shadowLeftOffset = new KIntNumInput(0,sGroup);
  shadowLeftOffset->setSuffix("%");
  shadowLeftOffset->setRange(-200,200);
  gLay2->addWidget(shadowLeftOffset,6,1);

  TQLabel *label6 = new TQLabel(i18n("Shadow color:"),sGroup);
  gLay2->addWidget(label6,7,0);
  shadowColor = new KColorButton(Qt::black,sGroup);
  gLay2->addWidget(shadowColor,7,1);
  gLay2->setColStretch(1,1);
  vLay2->addSpacing(11);
  removeShadowsOnMove = new TQCheckBox(i18n("Remove shadows on move"),sGroup);
  vLay2->addWidget(removeShadowsOnMove);
  removeShadowsOnResize = new TQCheckBox(i18n("Remove shadows on resize"),sGroup);
  vLay2->addWidget(removeShadowsOnResize);
  vLay2->addStretch();
  tabW->addTab(sGroup, i18n("Shadows"));

  TQWidget *eGroup = new TQWidget(this);
  TQVBoxLayout *vLay3 = new TQVBoxLayout (eGroup,11,6);

  fadeInWindows = new TQCheckBox(i18n("Fade-in windows (including popups)"),eGroup);
  fadeInMenuWindows = new TQCheckBox(i18n("Fade-in menus (requires menu fade effect to be disabled in the Styles module)"),eGroup);
  fadeInToolTipWindows = new TQCheckBox(i18n("Fade-in tooltips"),eGroup);
  fadeOnOpacityChange = new TQCheckBox(i18n("Fade between opacity changes"),eGroup);
  fadeInSpeed = new KIntNumInput(100, eGroup);
  fadeInSpeed->setRange(1,100);
  fadeInSpeed->setLabel(i18n("Fade-in speed:"));
  fadeOutSpeed = new KIntNumInput(100, eGroup);
  fadeOutSpeed->setRange(1,100);
  fadeOutSpeed->setLabel(i18n("Fade-out speed:"));
  vLay3->addWidget(fadeInWindows);
  vLay3->addWidget(fadeInMenuWindows);
  vLay3->addWidget(fadeInToolTipWindows);
  vLay3->addWidget(fadeOnOpacityChange);
  vLay3->addWidget(fadeInSpeed);
  vLay3->addWidget(fadeOutSpeed);
  vLay3->addStretch();

  tabW->addTab(eGroup, i18n("Effects"));

  useTranslucency = new TQCheckBox(i18n("Enable the Trinity window composition manager"),this);
  lay->addWidget(useTranslucency);
  lay->addWidget(tabW);

  connect(useTranslucency, TQT_SIGNAL(toggled(bool)), tabW, TQT_SLOT(setEnabled(bool)));

  connect(activeWindowTransparency, TQT_SIGNAL(toggled(bool)), activeWindowOpacity, TQT_SLOT(setEnabled(bool)));
  connect(inactiveWindowTransparency, TQT_SIGNAL(toggled(bool)), inactiveWindowOpacity, TQT_SLOT(setEnabled(bool)));
  connect(movingWindowTransparency, TQT_SIGNAL(toggled(bool)), movingWindowOpacity, TQT_SLOT(setEnabled(bool)));
  connect(dockWindowTransparency, TQT_SIGNAL(toggled(bool)), dockWindowOpacity, TQT_SLOT(setEnabled(bool)));

  connect(useTranslucency, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(onlyDecoTranslucent, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(activeWindowTransparency, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(inactiveWindowTransparency, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(movingWindowTransparency, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(dockWindowTransparency, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(keepAboveAsActive, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(disableARGB, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(useOpenGL, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(useOpenGL, TQT_SIGNAL(toggled(bool)), blurBackground, TQT_SLOT(setEnabled(bool)));
  connect(blurBackground, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(useOpenGL, TQT_SIGNAL(toggled(bool)), greyscaleBackground, TQT_SLOT(setEnabled(bool)));
  connect(greyscaleBackground, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(useShadows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(useShadowsOnMenuWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(useShadowsOnToolTipWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(useShadowsOnDockWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(removeShadowsOnResize, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(removeShadowsOnMove, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));

  connect(activeWindowOpacity, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(inactiveWindowOpacity, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(movingWindowOpacity, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(dockWindowOpacity, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(dockWindowShadowSize, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(menuWindowShadowSize, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(activeWindowShadowSize, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(inactiveWindowShadowSize, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(baseShadowSize, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(shadowTopOffset, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(shadowLeftOffset, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(shadowColor, TQT_SIGNAL(changed(const TQColor&)), TQT_SLOT(changed()));
  connect(fadeInWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(fadeInMenuWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(fadeInToolTipWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(fadeOnOpacityChange, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()));
  connect(fadeInSpeed, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));
  connect(fadeOutSpeed, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(changed()));

  connect(useShadows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(processShadowLockouts()));
  connect(useShadowsOnMenuWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(processShadowLockouts()));
  connect(useShadowsOnToolTipWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(processShadowLockouts()));
  connect(useShadowsOnDockWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(processShadowLockouts()));

  load();

  tabW->setEnabled(useTranslucency->isChecked());

  connect(useTranslucency, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(showWarning(bool)));

  // handle kompmgr restarts if necessary
  connect(useTranslucency, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(disableARGB, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(useOpenGL, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(blurBackground, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(greyscaleBackground, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(useShadows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(useShadowsOnMenuWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(useShadowsOnToolTipWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(useShadowsOnDockWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(inactiveWindowShadowSize, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(resetKompmgr()));
  connect(baseShadowSize, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(resetKompmgr()));
  connect(shadowTopOffset, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(resetKompmgr()));
  connect(shadowLeftOffset, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(resetKompmgr()));
  connect(shadowColor, TQT_SIGNAL(changed(const TQColor&)), TQT_SLOT(resetKompmgr()));
  connect(fadeInWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(fadeInMenuWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(fadeInToolTipWindows, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(fadeOnOpacityChange, TQT_SIGNAL(toggled(bool)), TQT_SLOT(resetKompmgr()));
  connect(fadeInSpeed, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(resetKompmgr()));
  connect(fadeOutSpeed, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(resetKompmgr()));

  }
}

void KTranslucencyConfig::processShadowLockouts()
{
    bool enabled = (useShadows->isChecked() || useShadowsOnMenuWindows->isChecked() || useShadowsOnToolTipWindows->isChecked());

    dockWindowShadowSize->setEnabled(enabled);
    menuWindowShadowSize->setEnabled(enabled);
    activeWindowShadowSize->setEnabled(enabled);
    inactiveWindowShadowSize->setEnabled(enabled);
    baseShadowSize->setEnabled(enabled);
    shadowTopOffset->setEnabled(enabled);
    shadowLeftOffset->setEnabled(enabled);
    shadowColor->setEnabled(enabled);

    useShadowsOnDockWindows->setEnabled(useShadows->isChecked());
}

void KTranslucencyConfig::resetKompmgr()
{
    resetKompmgr_ = TRUE;
}

void KTranslucencyConfig::load( void )
{

    if (!kompmgrAvailable_)
        return;
  config->setGroup( "Notification Messages" );
  useTranslucency->setChecked(config->readBoolEntry("UseTranslucency",false));

  config->setGroup( "Translucency" );
  activeWindowTransparency->setChecked(config->readBoolEntry("TranslucentActiveWindows",false));
  inactiveWindowTransparency->setChecked(config->readBoolEntry("TranslucentInactiveWindows",false));
  movingWindowTransparency->setChecked(config->readBoolEntry("TranslucentMovingWindows",false));
  removeShadowsOnMove->setChecked(config->readBoolEntry("RemoveShadowsOnMove",false));
  removeShadowsOnResize->setChecked(config->readBoolEntry("RemoveShadowsOnResize",false));
  dockWindowTransparency->setChecked(config->readBoolEntry("TranslucentDocks",false));
  keepAboveAsActive->setChecked(config->readBoolEntry("TreatKeepAboveAsActive",true));
  onlyDecoTranslucent->setChecked(config->readBoolEntry("OnlyDecoTranslucent",false));

  activeWindowOpacity->setValue(config->readNumEntry("ActiveWindowOpacity",100));
  inactiveWindowOpacity->setValue(config->readNumEntry("InactiveWindowOpacity",75));
  movingWindowOpacity->setValue(config->readNumEntry("MovingWindowOpacity",25));
  dockWindowOpacity->setValue(config->readNumEntry("DockOpacity",80));

  int ass, iss, dss, mss;
  dss = config->readNumEntry("DockShadowSize", 0*100);
  mss = config->readNumEntry("MenuShadowSize", 1*100);
  ass = config->readNumEntry("ActiveWindowShadowSize", 2*100);
  iss = config->readNumEntry("InactiveWindowShadowSize", 1*100);

  activeWindowOpacity->setEnabled(activeWindowTransparency->isChecked());
  inactiveWindowOpacity->setEnabled(inactiveWindowTransparency->isChecked());
  movingWindowOpacity->setEnabled(movingWindowTransparency->isChecked());
  dockWindowOpacity->setEnabled(dockWindowTransparency->isChecked());

  TDEConfig conf_(TQDir::homeDirPath() + "/.xcompmgrrc");
  conf_.setGroup("xcompmgr");

  disableARGB->setChecked(conf_.readBoolEntry("DisableARGB",FALSE));
  useOpenGL->setChecked(conf_.readBoolEntry("useOpenGL",FALSE));
  blurBackground->setChecked(conf_.readBoolEntry("blurBackground",FALSE));
  blurBackground->setEnabled(useOpenGL->isChecked());
  greyscaleBackground->setChecked(conf_.readBoolEntry("greyscaleBackground",FALSE));
  greyscaleBackground->setEnabled(useOpenGL->isChecked());

  useShadows->setChecked(conf_.readEntry("Compmode","").compare("CompClientShadows") == 0);
  useShadowsOnMenuWindows->setChecked(conf_.readBoolEntry("ShadowsOnMenuWindows",TRUE));
  useShadowsOnToolTipWindows->setChecked(conf_.readBoolEntry("ShadowsOnToolTipWindows",TRUE));
  useShadowsOnDockWindows->setChecked(conf_.readBoolEntry("ShadowsOnDockWindows",FALSE));
  shadowTopOffset->setValue(-1*(conf_.readNumEntry("ShadowOffsetY",0)));
  shadowLeftOffset->setValue(-1*(conf_.readNumEntry("ShadowOffsetX",0)));

  int ss =  conf_.readNumEntry("ShadowRadius",4);
  dockWindowShadowSize->setValue((int)(dss/100.0));
  menuWindowShadowSize->setValue((int)(mss/100.0));
  activeWindowShadowSize->setValue((int)(ass/100.0));
  inactiveWindowShadowSize->setValue((int)(iss/100.0));
  baseShadowSize->setValue((int)(ss));

  TQString hex = conf_.readEntry("ShadowColor","#000000");
  uint r, g, b;
  r = g = b = 256;

  if (sscanf(hex.latin1(), "0x%02x%02x%02x", &r, &g, &b)!=3 || r > 255 || g > 255 || b > 255)
    shadowColor->setColor(Qt::black);
  else
    shadowColor->setColor(TQColor(r,g,b));

  fadeInWindows->setChecked(conf_.readBoolEntry("FadeWindows",FALSE));
  fadeInMenuWindows->setChecked(conf_.readBoolEntry("FadeMenuWindows",TRUE));
  fadeInToolTipWindows->setChecked(conf_.readBoolEntry("FadeToolTipWindows",TRUE));
  fadeOnOpacityChange->setChecked(conf_.readBoolEntry("FadeTrans",FALSE));
  fadeInSpeed->setValue((int)(conf_.readDoubleNumEntry("FadeInStep",0.070)*1000.0));
  fadeOutSpeed->setValue((int)(conf_.readDoubleNumEntry("FadeOutStep",0.070)*1000.0));

  emit TDECModule::changed(false);
}

void KTranslucencyConfig::save( void )
{
  if (!kompmgrAvailable_)
      return;

  config->setGroup( "Notification Messages" );
  config->writeEntry("UseTranslucency",useTranslucency->isChecked());

  config->setGroup( "Translucency" );
  config->writeEntry("TranslucentActiveWindows",activeWindowTransparency->isChecked());
  config->writeEntry("TranslucentInactiveWindows",inactiveWindowTransparency->isChecked());
  config->writeEntry("TranslucentMovingWindows",movingWindowTransparency->isChecked());
  config->writeEntry("TranslucentDocks",dockWindowTransparency->isChecked());
  config->writeEntry("TreatKeepAboveAsActive",keepAboveAsActive->isChecked());
  config->writeEntry("ActiveWindowOpacity",activeWindowOpacity->value());
  config->writeEntry("InactiveWindowOpacity",inactiveWindowOpacity->value());
  config->writeEntry("MovingWindowOpacity",movingWindowOpacity->value());
  config->writeEntry("DockOpacity",dockWindowOpacity->value());
  // for simplification:
  // xcompmgr supports a general shadow radius and additionally lets external apps set a multiplicator for each window
  // (speed reasons, so the shadow matrix hasn't to be recreated for every window)
  // we set inactive windows to 100%, the radius to the inactive window value and adjust the multiplicators for docks and active windows
  // this way the user can set the three values without caring about the radius/multiplicator stuff
   // additionally we find a value between big and small values to have a more smooth appereance
  config->writeEntry("DockShadowSize",(int)(100.0 * dockWindowShadowSize->value()));
  config->writeEntry("MenuShadowSize",(int)(100.0 * menuWindowShadowSize->value()));
  config->writeEntry("ActiveWindowShadowSize",(int)(100.0 * activeWindowShadowSize->value()));
  config->writeEntry("InactiveWindowShadowSize",(int)(100.0 * inactiveWindowShadowSize->value()));

  config->writeEntry("RemoveShadowsOnMove",removeShadowsOnMove->isChecked());
  config->writeEntry("RemoveShadowsOnResize",removeShadowsOnResize->isChecked());
  config->writeEntry("OnlyDecoTranslucent", onlyDecoTranslucent->isChecked());
  config->writeEntry("ResetKompmgr",resetKompmgr_);

  TDEConfig *conf_ = new TDEConfig(TQDir::homeDirPath() + "/.xcompmgrrc");
  conf_->setGroup("xcompmgr");

  conf_->writeEntry("Compmode",useShadows->isChecked()?"CompClientShadows":"");
  conf_->writeEntry("ShadowsOnMenuWindows",useShadowsOnMenuWindows->isChecked());
  conf_->writeEntry("ShadowsOnToolTipWindows",useShadowsOnToolTipWindows->isChecked());
  conf_->writeEntry("ShadowsOnDockWindows",useShadowsOnDockWindows->isChecked());
  conf_->writeEntry("DisableARGB",disableARGB->isChecked());
  conf_->writeEntry("useOpenGL",useOpenGL->isChecked());
  conf_->writeEntry("blurBackground",blurBackground->isChecked());
  conf_->writeEntry("greyscaleBackground",greyscaleBackground->isChecked());
  conf_->writeEntry("ShadowOffsetY",-1*shadowTopOffset->value());
  conf_->writeEntry("ShadowOffsetX",-1*shadowLeftOffset->value());


  int r, g, b;
  shadowColor->color().rgb( &r, &g, &b );
  TQString hex;
  hex.sprintf("0x%02X%02X%02X", r,g,b);
  conf_->writeEntry("ShadowColor",hex);
  conf_->writeEntry("ShadowRadius",baseShadowSize->value());
  conf_->writeEntry("FadeWindows",fadeInWindows->isChecked());
  conf_->writeEntry("FadeMenuWindows",fadeInMenuWindows->isChecked());
  conf_->writeEntry("FadeToolTipWindows",fadeInToolTipWindows->isChecked());
  conf_->writeEntry("FadeTrans",fadeOnOpacityChange->isChecked());
  conf_->writeEntry("FadeInStep",fadeInSpeed->value()/1000.0);
  conf_->writeEntry("FadeOutStep",fadeOutSpeed->value()/1000.0);

  delete conf_;

  // Now write out compton settings
  TQFile* compton_conf_file_ = new TQFile(TQDir::homeDirPath() + "/.compton-tde.conf");
  if ( compton_conf_file_->open( IO_WriteOnly ) ) {
      TQTextStream stream(compton_conf_file_);

      stream << "# WARNING\n";
      stream << "# This file was automatically generated by TDE\n";
      stream << "# All changes will be lost!\n";

      stream << "shadow = " << (useShadows->isChecked()?"true":"false") << ";\n";
      stream << "shadow-offset-y = " << (-1*shadowTopOffset->value()) << ";\n";
      stream << "shadow-offset-x = " << (-1*shadowLeftOffset->value()) << ";\n";

      int r, g, b;
      shadowColor->color().rgb( &r, &g, &b );
      stream << "shadow-red = " << (r/255.0) << ";\n";
      stream << "shadow-green = " << (g/255.0) << ";\n";
      stream << "shadow-blue = " << (b/255.0) << ";\n";

      stream << "shadow-radius = " << baseShadowSize->value() << ";\n";

      bool fadeOpacity = fadeOnOpacityChange->isChecked();
      bool fadeWindows = fadeInWindows->isChecked();
      bool fadeMenuWindows = fadeInMenuWindows->isChecked();
      bool fadeToolTipWindows = fadeInToolTipWindows->isChecked();
      bool shadows = useShadows->isChecked();
      bool shadowsOnMenuWindows = useShadowsOnMenuWindows->isChecked();
      bool shadowsOnToolTipWindows = useShadowsOnToolTipWindows->isChecked();
      bool shadowsOnDockWindows = useShadowsOnDockWindows->isChecked();
      stream << "fading = " << ((fadeWindows || fadeMenuWindows || fadeOpacity)?"true":"false") << ";\n";
      stream << "no-fading-opacitychange = " << (fadeOpacity?"false":"true") << ";\n";
      stream << "no-fading-openclose = " << ((fadeWindows||fadeMenuWindows)?"false":"true") << ";\n";
      stream << "wintypes:" << "\n";
      stream << "{" << "\n";
      stream << "  menu = { shadow = " << (shadowsOnMenuWindows?"true":"false") << "; fade = " << (fadeMenuWindows?"true":"false") << "; no-fading-openclose = " << (fadeMenuWindows?"false":"true") << "; };" << "\n";
      stream << "  dropdown_menu = { shadow = " << (shadowsOnMenuWindows?"true":"false") << "; fade = " << (fadeMenuWindows?"true":"false") << "; no-fading-openclose = " << (fadeMenuWindows?"false":"true") << "; };" << "\n";
      stream << "  popup_menu = { shadow = " << (shadowsOnMenuWindows?"true":"false") << "; fade = " << (fadeMenuWindows?"true":"false") << "; no-fading-openclose = " << (fadeMenuWindows?"false":"true") << "; };" << "\n";
      stream << "  tooltip = { shadow = " << (shadowsOnToolTipWindows?"true":"false") << "; fade = " << (fadeToolTipWindows?"true":"false") << "; no-fading-openclose = " << (fadeToolTipWindows?"false":"true") << "; };" << "\n";
      stream << "  normal = { shadow = " << (shadows?"true":"false") << "; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  dialog = { shadow = " << (shadows?"true":"false") << "; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  unknown = { shadow = " << (shadows?"true":"false") << "; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  toolbar = { shadow = " << (shadows?"true":"false") << "; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  utility = { shadow = " << (shadows?"true":"false") << "; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  splash = { shadow = " << (shadows?"true":"false") << "; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";

      stream << "  notify = { shadow = false; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  combo = { shadow = false; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  desktop = { shadow = false; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  dnd = { shadow = false; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "  dock = { shadow = " << (shadowsOnDockWindows?"true":"false") << "; fade = " << (fadeWindows?"true":"false") << "; no-fading-openclose = " << (fadeWindows?"false":"true") << "; };" << "\n";
      stream << "};" << "\n";

      stream << "fade-in-step = " << (fadeInSpeed->value()/1000.0) << ";\n";
      stream << "fade-out-step = " << (fadeOutSpeed->value()/1000.0) << ";\n";

      stream << "backend = \"" << (useOpenGL->isChecked()?"glx":"xrender") << "\";\n";
      stream << "vsync = \"" << (useOpenGL->isChecked()?"opengl":"none") << "\";\n";

      stream << "blur-background = " << ((blurBackground->isChecked() && useOpenGL->isChecked())?"true":"false") << ";\n";
      stream << "blur-background-fixed = true;\n";
      stream << "blur-background-exclude = [\n";
      stream << "  \"window_type = 'dock'\",\n";
      stream << "  \"window_type = 'desktop'\"\n";
      stream << "];\n";

      stream << "greyscale-background = " << ((greyscaleBackground->isChecked() && useOpenGL->isChecked())?"true":"false") << ";\n";

      // Global settings
      stream << "no-dock-shadow = " << (shadows && shadowsOnDockWindows?"false":"true") << ";\n";
      stream << "no-dnd-shadow = true;\n";
      stream << "clear-shadow = true;\n";
      stream << "shadow-ignore-shaped = false;\n";

      // Features not currently supported by compton
//       stream << "DisableARGB = " << (disableARGB->isChecked()?"true":"false") << ";\n";

      compton_conf_file_->close();
   }
   delete compton_conf_file_;

  if (standAlone)
  {
    config->sync();
        if ( !kapp->dcopClient()->isAttached() )
            kapp->dcopClient()->attach();
        kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
  }
  if (useTranslucency->isChecked())
    startKompmgr();
  else
    stopKompmgr();
  emit TDECModule::changed(false);
}

void KTranslucencyConfig::defaults()
{
    if (!kompmgrAvailable_)
        return;
  useTranslucency->setChecked(false);
  onlyDecoTranslucent->setChecked(false);
  activeWindowTransparency->setChecked(false);
  inactiveWindowTransparency->setChecked(false);
  movingWindowTransparency->setChecked(false);
  dockWindowTransparency->setChecked(false);
  keepAboveAsActive->setChecked(true);
  disableARGB->setChecked(false);

  activeWindowOpacity->setValue(100);
  inactiveWindowOpacity->setValue(75);
  movingWindowOpacity->setValue(25);
  dockWindowOpacity->setValue(80);

  dockWindowShadowSize->setValue(0);
  menuWindowShadowSize->setValue(1);
  activeWindowShadowSize->setValue(2);
  inactiveWindowShadowSize->setValue(1);
  baseShadowSize->setValue(4);
  shadowTopOffset->setValue(0);
  shadowLeftOffset->setValue(0);

  activeWindowOpacity->setEnabled(false);
  inactiveWindowOpacity->setEnabled(false);
  movingWindowOpacity->setEnabled(false);
  dockWindowOpacity->setEnabled(false);
  useShadows->setChecked(FALSE);
  useShadowsOnMenuWindows->setChecked(TRUE);
  useShadowsOnToolTipWindows->setChecked(TRUE);
  useShadowsOnDockWindows->setChecked(FALSE);
  removeShadowsOnMove->setChecked(FALSE);
  removeShadowsOnResize->setChecked(FALSE);
  shadowColor->setColor(Qt::black);
  fadeInWindows->setChecked(FALSE);
  fadeInMenuWindows->setChecked(TRUE);
  fadeInToolTipWindows->setChecked(TRUE);
  fadeOnOpacityChange->setChecked(FALSE);
  fadeInSpeed->setValue(70);
  fadeOutSpeed->setValue(70);
  emit TDECModule::changed(true);
}


bool KTranslucencyConfig::kompmgrAvailable()
{
    bool ret;
    TDEProcess proc;
    proc << TDECompositor << "-v";
    ret = proc.start(TDEProcess::DontCare, TDEProcess::AllOutput);
    proc.detach();
    return ret;
}

void KTranslucencyConfig::startKompmgr()
{
    kapp->dcopClient()->send("twin*", "", "startKompmgr()", TQString(""));
}

void KTranslucencyConfig::stopKompmgr()
{
    kapp->dcopClient()->send("twin*", "", "stopKompmgr()", TQString(""));
}

void KTranslucencyConfig::showWarning(bool alphaActivated)
{
//    if (alphaActivated)
//        KMessageBox::information(this, i18n("<qt>Translucency support is new and may cause problems<br> including crashes (sometimes the translucency engine, seldom even X).</qt>"), i18n("Warning"));
}

#include "windows.moc"
