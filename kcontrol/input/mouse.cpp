/*
 * mouse.cpp
 *
 * Copyright (c) 1997 Patrick Dowler dowler@morgul.fsh.uvic.ca
 *
 * Layout management, enhancements:
 * Copyright (c) 1999 Dirk A. Mueller <dmuell@gmx.net>
 *
 * SC/DC/AutoSelect/ChangeCursor:
 * Copyright (c) 2000 David Faure <faure@kde.org>
 *
 * Double click interval, drag time & dist
 * Copyright (c) 2000 Bernd Gehrmann
 *
 * Large cursor support
 * Visual activation TODO: speed
 * Copyright (c) 2000 Rik Hemsley <rik@kde.org>
 *
 * White cursor support
 * TODO: give user the option to choose a certain cursor font
 * -> Theming
 *
 * General/Advanced tabs
 * Copyright (c) 2000 Brad Hughes <bhughes@trolltech.com>
 *
 * redesign for KDE 2.2
 * Copyright (c) 2001 Ralf Nolden <nolden@kde.org>
 *
 * Logitech mouse support
 * Copyright (C) 2004 Brad Hards <bradh@frogmouth.net>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tqlayout.h>
#include <tqcheckbox.h>
#undef Below
#undef Above
#include <tqslider.h>
#include <tqwhatsthis.h>
#include <tqtabwidget.h>
#include <tqradiobutton.h>
#include <tqstyle.h>
#include <tqevent.h>

#include <tdelocale.h>
#include <kdialog.h>
#include <tdeconfig.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <tdeaboutdata.h>

#include <config.h>

#include "mouse.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <kipc.h>

#undef Below

MouseConfig::MouseConfig (TQWidget * parent, const char *name)
  : TDECModule(parent, name)
{


   setQuickHelp( i18n("<h1>Mouse</h1> This module allows you to choose various"
     " options for the way in which your pointing device works. Your"
     " pointing device may be a mouse, trackball, or some other hardware"
     " that performs a similar function."));

    TQString wtstr;

    TQBoxLayout *top = new TQVBoxLayout(this, 0, KDialog::spacingHint());

    tabwidget = new TQTabWidget(this);
    top->addWidget(tabwidget);

    tab1 = new KMouseDlg(this);
    TQButtonGroup *group = new TQButtonGroup( tab1 );
    group->setExclusive( true );
    group->hide();
    group->insert( tab1->singleClick );
    group->insert( tab1->doubleClick );

    tabwidget->addTab(tab1, i18n("&General"));

    connect(tab1->handedBox, TQ_SIGNAL(clicked(int)), this, TQ_SLOT(changed()));
    connect(tab1->handedBox, TQ_SIGNAL(clicked(int)), this, TQ_SLOT(slotHandedChanged(int)));

    wtstr = i18n("If you are left-handed, you may prefer to swap the"
         " functions of the left and right buttons on your pointing device"
         " by choosing the 'left-handed' option. If your pointing device"
         " has more than two buttons, only those that function as the"
         " left and right buttons are affected. For example, if you have"
         " a three-button mouse, the middle button is unaffected.");
    TQWhatsThis::add( tab1->handedBox, wtstr );

    connect(tab1->doubleClick, TQ_SIGNAL(clicked()), TQ_SLOT(changed()));

    wtstr = i18n("The default behavior in TDE is to select and activate"
         " icons with a single click of the left button on your pointing"
         " device. This behavior is consistent with what you would expect"
         " when you click links in most web browsers. If you would prefer"
         " to select with a single click, and activate with a double click,"
         " check this option.");
    TQWhatsThis::add( tab1->doubleClick, wtstr );

    wtstr = i18n("Activates and opens a file or folder with a single click.");
    TQWhatsThis::add( tab1->singleClick, wtstr );


    connect(tab1->cbAutoSelect, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));

    wtstr = i18n("If you check this option, pausing the mouse pointer"
         " over an icon on the screen will automatically select that icon."
         " This may be useful when single clicks activate icons, and you"
         " want only to select the icon without activating it.");
    TQWhatsThis::add( tab1->cbAutoSelect, wtstr );

//    slAutoSelect = new TQSlider(0, 2000, 10, 0, TQSlider::Horizontal, tab1);
    tab1->slAutoSelect->setSteps( 125, 125 );
    tab1->slAutoSelect->setTickmarks( TQSlider::Below );
    tab1->slAutoSelect->setTickInterval( 250 );
    tab1->slAutoSelect->setTracking( true );

    wtstr = i18n("If you have checked the option to automatically select"
         " icons, this slider allows you to select how long the mouse pointer"
         " must be paused over the icon before it is selected.");
    TQWhatsThis::add( tab1->slAutoSelect, wtstr );

    wtstr = i18n("Show feedback when clicking an icon");
    TQWhatsThis::add( tab1->cbVisualActivate, wtstr );

    connect(tab1->slAutoSelect, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
    connect(tab1->cbVisualActivate, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));

    connect(tab1->cb_pointershape, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));

    connect(tab1->singleClick, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));
    connect(tab1->singleClick, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotClick()));

    connect( tab1->doubleClick, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotClick() ) );
    connect( tab1->cbAutoSelect, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotClick() ) );

    // Only allow setting reversing scroll polarity if we have scroll buttons
    unsigned char map[20];
    if ( XGetPointerMapping(kapp->getDisplay(), map, 20) >= 5 )
    {
      tab1->cbScrollPolarity->setEnabled( true );
      tab1->cbScrollPolarity->show();
    }
    else
    {
      tab1->cbScrollPolarity->setEnabled( false );
      tab1->cbScrollPolarity->hide();
    }
    connect(tab1->cbScrollPolarity, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));
    connect(tab1->cbScrollPolarity, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotScrollPolarityChanged()));

    // Cursor theme tab
    themetab = new ThemePage(this);
    connect(themetab, TQ_SIGNAL(changed(bool)), TQ_SLOT(changed()));
    tabwidget->addTab(themetab, i18n("&Cursor Theme"));

    // Advanced tab
    tab2 = new TQWidget(0, "Advanced Tab");
    tabwidget->addTab(tab2, i18n("Advanced"));

    TQBoxLayout *lay = new TQVBoxLayout(tab2, KDialog::marginHint(),
              KDialog::spacingHint());

    accel = new KDoubleNumInput(1, 20, 2, 0.1, 1, tab2);
    accel->setLabel(i18n("Pointer acceleration:"));
    accel->setSuffix("x");
    lay->addWidget(accel);
    connect(accel, TQ_SIGNAL(valueChanged(double)), this, TQ_SLOT(changed()));

    wtstr = i18n("This option allows you to change the relationship"
         " between the distance that the mouse pointer moves on the"
         " screen and the relative movement of the physical device"
         " itself (which may be a mouse, trackball, or some other"
         " pointing device.)<p>"
         " A high value for the acceleration will lead to large"
         " movements of the mouse pointer on the screen even when"
         " you only make a small movement with the physical device."
         " Selecting very high values may result in the mouse pointer"
         " flying across the screen, making it hard to control.");
    TQWhatsThis::add( accel, wtstr );

    thresh = new KIntNumInput(accel, 20, tab2);
    thresh->setLabel(i18n("Pointer threshold:"));
    thresh->setRange(0,20,1);
    thresh->setSteps(1,1);
    lay->addWidget(thresh);
    connect(thresh, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
    connect(thresh, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(slotThreshChanged(int)));
    slotThreshChanged(thresh->value());

    wtstr = i18n("The threshold is the smallest distance that the"
         " mouse pointer must move on the screen before acceleration"
         " has any effect. If the movement is smaller than the threshold,"
         " the mouse pointer moves as if the acceleration was set to 1X;<p>"
         " thus, when you make small movements with the physical device,"
         " there is no acceleration at all, giving you a greater degree"
         " of control over the mouse pointer. With larger movements of"
         " the physical device, you can move the mouse pointer"
         " rapidly to different areas on the screen.");
    TQWhatsThis::add( thresh, wtstr );

    doubleClickInterval = new KIntNumInput(thresh, 2000, tab2);
    doubleClickInterval->setLabel(i18n("Double click interval:"));
    doubleClickInterval->setRange(0, 2000, 100);
    doubleClickInterval->setSuffix(i18n(" msec"));
    doubleClickInterval->setSteps(100, 100);
    lay->addWidget(doubleClickInterval);
    connect(doubleClickInterval, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));

    wtstr = i18n("The double click interval is the maximal time"
         " (in milliseconds) between two mouse clicks which"
         " turns them into a double click. If the second"
         " click happens later than this time interval after"
         " the first click, they are recognized as two"
         " separate clicks.");
    TQWhatsThis::add( doubleClickInterval, wtstr );

    lay->addSpacing(10);

    doubleClickLabel = new TQLabel(i18n("Double-click on the image below to test your double-click interval:"), tab2);
    lay->addWidget(doubleClickLabel);
    wtstr = i18n("The image will change when your double-click"
         " test time is less than or equal to the interval you"
         " configured. When changing the interval, be sure to select"
         " the Apply button before testing. For example, the image"
         " will not change when you configure a double-click interval"
         " of 700 milliseconds and the time between two successive"
         " clicks on the image is 800 milliseconds, but the image will"
         " change when the time between clicks is 600 milliseconds."
         " The goal is to select a comfortable interval that you find"
         " is not too fast or slow.");
    TQWhatsThis::add( doubleClickLabel, wtstr );
    doubleClickStatus = false;  // First image will be displayed
    doubleClickButton = new TQPushButton( tab2 );
    doubleClickButton->setAcceptDrops( false );
    // The images are 32x32.
    uint sz = style().pixelMetric( TQStyle::PM_ButtonMargin ) * 2 + 48;
    doubleClickButton->setFixedSize( sz, sz );
    // Show the first image.
    doubleClickButton->setPixmap(locate("data", "kcminput/pics/doubleclick_1.png"));
    lay->addWidget(doubleClickButton);
    // Use the same What's This help for the pushbutton.
    TQWhatsThis::add( doubleClickButton, wtstr );
    connect(doubleClickButton, TQ_SIGNAL(pressed()), this, TQ_SLOT(slotDoubleClickButtonPressed()));
    doubleClickTimer=new TQTimer();
    connect(doubleClickTimer, TQ_SIGNAL(timeout()), this, TQ_SLOT(slotDoubleClickTimerDone()) );

    lay->addSpacing(10);

    dragStartTime = new KIntNumInput(doubleClickInterval, 2000, tab2);
    dragStartTime->setLabel(i18n("Drag start time:"));
    dragStartTime->setRange(0, 2000, 100);
    dragStartTime->setSuffix(i18n(" msec"));
    dragStartTime->setSteps(100, 100);
    lay->addWidget(dragStartTime);
    connect(dragStartTime, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));

    wtstr = i18n("If you click with the mouse (e.g. in a multi-line"
         " editor) and begin to move the mouse within the"
         " drag start time, a drag operation will be initiated.");
    TQWhatsThis::add( dragStartTime, wtstr );

    dragStartDist = new KIntNumInput(dragStartTime, 20, tab2);
    dragStartDist->setLabel(i18n("Drag start distance:"));
    dragStartDist->setRange(1, 20, 1);
    dragStartDist->setSteps(1,1);
    lay->addWidget(dragStartDist);
    connect(dragStartDist, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
    connect(dragStartDist, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(slotDragStartDistChanged(int)));
    slotDragStartDistChanged(dragStartDist->value());

    wtstr = i18n("If you click with the mouse and begin to move the"
         " mouse at least the drag start distance, a drag"
         " operation will be initiated.");
    TQWhatsThis::add( dragStartDist, wtstr);

    wheelScrollLines = new KIntNumInput(dragStartDist, 3, tab2);
    wheelScrollLines->setLabel(i18n("Mouse wheel scrolls by:"));
    wheelScrollLines->setRange(1, 12, 1);
    wheelScrollLines->setSteps(1,1);
    lay->addWidget(wheelScrollLines);
    connect(wheelScrollLines, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
    connect(wheelScrollLines, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotWheelScrollLinesChanged(int)));
    slotWheelScrollLinesChanged(wheelScrollLines->value());

    wtstr = i18n("If you use the wheel of a mouse, this value determines the number of lines to scroll for each wheel movement. Note that if this number exceeds the number of visible lines, it will be ignored and the wheel movement will be handled as a page up/down movement.");
    TQWhatsThis::add( wheelScrollLines, wtstr);
    lay->addStretch();

{
  TQWidget *mouse = new TQWidget(this, "Mouse Navigation");
  tabwidget->addTab(mouse, i18n("Mouse Navigation"));


  TQBoxLayout *vbox = new TQVBoxLayout(mouse, KDialog::marginHint(),
    KDialog::spacingHint());

  TQVBoxLayout *vvbox = new TQVBoxLayout(mouse->layout(), KDialog::spacingHint());

  mouseKeys = new TQCheckBox(i18n("&Move pointer with keyboard (using the num pad)"), mouse);
  vvbox->addWidget(mouseKeys);

  TQBoxLayout *hbox = new TQHBoxLayout(vvbox, KDialog::spacingHint());
  hbox->addSpacing(24);
  mk_delay = new KIntNumInput(mouse);
  mk_delay->setLabel(i18n("&Acceleration delay:"), AlignVCenter);
  mk_delay->setSuffix(i18n(" msec"));
  mk_delay->setRange(1, 1000, 50);
  hbox->addWidget(mk_delay);

  hbox = new TQHBoxLayout(vvbox, KDialog::spacingHint());
  hbox->addSpacing(24);
  mk_interval = new KIntNumInput(mk_delay, 0, mouse);
  mk_interval->setLabel(i18n("R&epeat interval:"), AlignVCenter);
  mk_interval->setSuffix(i18n(" msec"));
  mk_interval->setRange(1, 1000, 10);
  hbox->addWidget(mk_interval);

  hbox = new TQHBoxLayout(vvbox, KDialog::spacingHint());
  hbox->addSpacing(24);
  mk_time_to_max = new KIntNumInput(mk_interval, 0, mouse);
  mk_time_to_max->setLabel(i18n("Acceleration &time:"), AlignVCenter);
  mk_time_to_max->setRange(100, 10000, 200);
  mk_time_to_max->setSuffix(i18n(" msec"));
  hbox->addWidget(mk_time_to_max);

  hbox = new TQHBoxLayout(vvbox, KDialog::spacingHint());
  hbox->addSpacing(24);
  mk_max_speed = new KIntNumInput(mk_time_to_max, 0, mouse);
  mk_max_speed->setLabel(i18n("Ma&ximum speed:"), AlignVCenter);
  mk_max_speed->setRange(1, 2000, 20);
  mk_max_speed->setSuffix(i18n(" pixel/sec"));
  hbox->addWidget(mk_max_speed);

  hbox = new TQHBoxLayout(vvbox, KDialog::spacingHint());
  hbox->addSpacing(24);
  mk_curve = new KIntNumInput(mk_max_speed, 0, mouse);
  mk_curve->setLabel(i18n("Acceleration &profile:"), AlignVCenter);
  mk_curve->setRange(-1000, 1000, 100);
  hbox->addWidget(mk_curve);

  connect(mouseKeys, TQ_SIGNAL(clicked()), this, TQ_SLOT(checkAccess()));
  connect(mouseKeys, TQ_SIGNAL(clicked()), this, TQ_SLOT(changed()));
  connect(mk_delay, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
  connect(mk_interval, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
  connect(mk_time_to_max, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
  connect(mk_max_speed, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));
  connect(mk_curve, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(changed()));

  vbox->addStretch();
}

  settings = new MouseSettings;

  // This part is for handling features on Logitech USB mice.
  // It only works if libusb is available.
#ifdef HAVE_LIBUSB

  struct device_table {
      int idVendor;
      int idProduct;
      TQString Model;
      TQString Name;
      int flags;
  } device_table[] = {
      { VENDOR_LOGITECH, 0xC00E, "M-BJ58", "Wheel Mouse Optical", HAS_RES },
      { VENDOR_LOGITECH, 0xC00F, "M-BJ79", "MouseMan Traveler", HAS_RES },
      { VENDOR_LOGITECH, 0xC012, "M-BL63B", "MouseMan Dual Optical", HAS_RES },
      { VENDOR_LOGITECH, 0xC01B, "M-BP86", "MX310 Optical Mouse", HAS_RES },
      { VENDOR_LOGITECH, 0xC01D, "M-BS81A", "MX510 Optical Mouse", HAS_RES | HAS_SS | HAS_SSR },
      { VENDOR_LOGITECH, 0xC024, "M-BP82", "MX300 Optical Mouse", HAS_RES },
      { VENDOR_LOGITECH, 0xC025, "M-BP81A", "MX500 Optical Mouse", HAS_RES | HAS_SS | HAS_SSR },
      { VENDOR_LOGITECH, 0xC031, "M-UT58A", "iFeel Mouse (silver)", HAS_RES },
      { VENDOR_LOGITECH, 0xC501, "C-BA4-MSE", "Mouse Receiver", HAS_CSR },
      { VENDOR_LOGITECH, 0xC502, "C-UA3-DUAL", "Dual Receiver", HAS_CSR | USE_CH2},
      { VENDOR_LOGITECH, 0xC504, "C-BD9-DUAL", "Cordless Freedom Optical", HAS_CSR | USE_CH2 },
      { VENDOR_LOGITECH, 0xC505, "C-BG17-DUAL", "Cordless Elite Duo", HAS_SS | HAS_SSR | HAS_CSR | USE_CH2},
      { VENDOR_LOGITECH, 0xC506, "C-BF16-MSE", "MX700 Optical Mouse", HAS_SS | HAS_CSR },
      { VENDOR_LOGITECH, 0xC508, "C-BA4-MSE", "Cordless Optical TrackMan", HAS_SS | HAS_CSR },
      { VENDOR_LOGITECH, 0xC50B, "967300-0403", "Cordless MX Duo Receiver", HAS_SS|HAS_CSR },
      { VENDOR_LOGITECH, 0xC50E, "M-RAG97", "MX1000 Laser Mouse", HAS_SS | HAS_CSR },
      { VENDOR_LOGITECH, 0xC512, "M-RAK89B", "Cordless Desktop Pro LX700", HAS_CSR | USE_CH2 },
      { VENDOR_LOGITECH, 0xC702, "C-UF15", "Receiver for Cordless Presenter", HAS_CSR },
      { 0, 0, TQString(), TQString(), 0 }
  };

  usb_init();
  usb_find_busses();
  usb_find_devices();

  struct usb_bus *bus;
  struct usb_device *dev;

  for (bus = usb_busses; bus; bus = bus->next) {
      for (dev = bus->devices; dev; dev = dev->next) {
	  for (int n = 0; device_table[n].idVendor; n++)
	      if ( (device_table[n].idVendor == dev->descriptor.idVendor) &&
		   (device_table[n].idProduct == dev->descriptor.idProduct) ) {
		  // OK, we have a device that appears to be one of the ones we support
		  LogitechMouse *mouse = new LogitechMouse( dev, device_table[n].flags, this, device_table[n].Name.latin1() );
		  settings->logitechMouseList.append(mouse);
		  tabwidget->addTab( (TQWidget*)mouse, device_table[n].Name );
	      }
      }
  }

#endif

  load();

  TDEAboutData* about = new TDEAboutData("kcmmouse", I18N_NOOP("Mouse"), 0, 0,
        TDEAboutData::License_GPL, I18N_NOOP("(c) 1997 - 2005 Mouse developers")); 
  about->addAuthor("Patrick Dowler", 0, 0);
  about->addAuthor("Dirk A. Mueller", 0, 0);
  about->addAuthor("David Faure", 0, 0);
  about->addAuthor("Bernd Gehrmann", 0, 0);
  about->addAuthor("Rik Hemsley", 0, 0);
  about->addAuthor("Brad Hughes", 0, 0);
  about->addAuthor("Ralf Nolden", 0, 0);
  about->addAuthor("Brad Hards", 0, 0);
  setAboutData( about );
}

void MouseConfig::checkAccess()
{
  mk_delay->setEnabled(mouseKeys->isChecked());
  mk_interval->setEnabled(mouseKeys->isChecked());
  mk_time_to_max->setEnabled(mouseKeys->isChecked());
  mk_max_speed->setEnabled(mouseKeys->isChecked());
  mk_curve->setEnabled(mouseKeys->isChecked());
}


MouseConfig::~MouseConfig()
{
    delete settings;
}

double MouseConfig::getAccel()
{
  return accel->value();
}

void MouseConfig::setAccel(double val)
{
  accel->setValue(val);
}

int MouseConfig::getThreshold()
{
  return thresh->value();
}

void MouseConfig::setThreshold(int val)
{
  thresh->setValue(val);
}


int MouseConfig::getHandedness()
{
  if (tab1->rightHanded->isChecked())
    return RIGHT_HANDED;
  else
    return LEFT_HANDED;
}

void MouseConfig::setHandedness(int val)
{
  tab1->rightHanded->setChecked(false);
  tab1->leftHanded->setChecked(false);
  if (val == RIGHT_HANDED){
    tab1->rightHanded->setChecked(true);
    tab1->mousePix->setPixmap(locate("data", "kcminput/pics/mouse_rh.png"));
  }
  else{
    tab1->leftHanded->setChecked(true);
    tab1->mousePix->setPixmap(locate("data", "kcminput/pics/mouse_lh.png"));
  }
}

void MouseConfig::load()
{
   load( false );
}

void MouseConfig::load( bool useDefaults )
{

  TDEConfig config( "kcminputrc", true );

  config.setReadDefaults( useDefaults );

  settings->load(&config);

  tab1->rightHanded->setEnabled(settings->handedEnabled);
  tab1->leftHanded->setEnabled(settings->handedEnabled);
  if ( tab1->cbScrollPolarity->isEnabled() )
    tab1->cbScrollPolarity->setEnabled(settings->handedEnabled);
  tab1->cbScrollPolarity->setChecked( settings->reverseScrollPolarity );

  setAccel(settings->accelRate);
  setThreshold(settings->thresholdMove);
  setHandedness(settings->handed);

  doubleClickInterval->setValue(settings->doubleClickInterval);
  dragStartTime->setValue(settings->dragStartTime);
  dragStartDist->setValue(settings->dragStartDist);
  wheelScrollLines->setValue(settings->wheelScrollLines);

  tab1->singleClick->setChecked( settings->singleClick );
  tab1->doubleClick->setChecked(!settings->singleClick);
  tab1->cb_pointershape->setChecked(settings->changeCursor);
  tab1->cbAutoSelect->setChecked( settings->autoSelectDelay >= 0 );
  if ( settings->autoSelectDelay < 0 )
     tab1->slAutoSelect->setValue( 0 );
  else
     tab1->slAutoSelect->setValue( settings->autoSelectDelay );
  tab1->cbVisualActivate->setChecked( settings->visualActivate );
  slotClick();


  TDEConfig ac("kaccessrc", true);

  ac.setGroup("Mouse");
  mouseKeys->setChecked(ac.readBoolEntry("MouseKeys", false));
  mk_delay->setValue(ac.readNumEntry("MKDelay", 160));

  int interval = ac.readNumEntry("MKInterval", 5);
  mk_interval->setValue(interval);

  // Default time to reach maximum speed: 5000 msec
  int time_to_max = ac.readNumEntry("MKTimeToMax",
												(5000+interval/2)/interval);
  time_to_max = ac.readNumEntry("MK-TimeToMax",
										  time_to_max*interval);
  mk_time_to_max->setValue(time_to_max);

  // Default maximum speed: 1000 pixels/sec
  //     (The old default maximum speed from KDE <= 3.4
  //     (100000 pixels/sec) was way too fast)
  long max_speed = ac.readNumEntry("MKMaxSpeed", interval);
  max_speed = max_speed * 1000 / interval;
  if (max_speed > 2000)
     max_speed = 2000;
  max_speed = ac.readNumEntry("MK-MaxSpeed", max_speed);
  mk_max_speed->setValue(max_speed);

  mk_curve->setValue(ac.readNumEntry("MKCurve", 0));

  themetab->load();

  checkAccess();
  changed( useDefaults );
}

void MouseConfig::save()
{
  settings->accelRate = getAccel();
  settings->thresholdMove = getThreshold();
  settings->handed = getHandedness();

  settings->doubleClickInterval = doubleClickInterval->value();
  settings->dragStartTime = dragStartTime->value();
  settings->dragStartDist = dragStartDist->value();
  settings->wheelScrollLines = wheelScrollLines->value();
  settings->singleClick = !tab1->doubleClick->isChecked();
  settings->autoSelectDelay = tab1->cbAutoSelect->isChecked()? tab1->slAutoSelect->value():-1;
  settings->visualActivate = tab1->cbVisualActivate->isChecked();
//  settings->changeCursor = tab1->singleClick->isChecked();
  settings->changeCursor = tab1->cb_pointershape->isChecked();
  settings->reverseScrollPolarity = tab1->cbScrollPolarity->isChecked();

  settings->apply();
  TDEConfig config( "kcminputrc" );
  settings->save(&config);

  TDEConfig ac("kaccessrc", false);

  ac.setGroup("Mouse");

  int interval = mk_interval->value();
  ac.writeEntry("MouseKeys", mouseKeys->isChecked());
  ac.writeEntry("MKDelay", mk_delay->value());
  ac.writeEntry("MKInterval", interval);
  ac.writeEntry("MK-TimeToMax", mk_time_to_max->value());
  ac.writeEntry("MKTimeToMax",
                (mk_time_to_max->value() + interval/2)/interval);
  ac.writeEntry("MK-MaxSpeed", mk_max_speed->value());
  ac.writeEntry("MKMaxSpeed",
					 (mk_max_speed->value()*interval + 500)/1000);
  ac.writeEntry("MKCurve", mk_curve->value());
  ac.sync();

  themetab->save();

  // restart kaccess
  kapp->startServiceByDesktopName("kaccess");

  TDECModule::changed(false);

}

void MouseConfig::defaults()
{
   load( true );
}

void MouseConfig::slotClick()
{
  // Autoselect has a meaning only in single-click mode
  tab1->cbAutoSelect->setEnabled(!tab1->doubleClick->isChecked() || tab1->singleClick->isChecked());
  // Delay has a meaning only for autoselect
  bool bDelay = tab1->cbAutoSelect->isChecked() && ! tab1->doubleClick->isChecked();
   tab1->slAutoSelect->setEnabled( bDelay );
   tab1->lDelay->setEnabled( bDelay );
   tab1->lb_short->setEnabled( bDelay );
   tab1->lb_long->setEnabled( bDelay );

}

void MouseConfig::slotDoubleClickButtonPressed()
{
  if (!doubleClickTimer->isActive())
  {
    // First click or click after the timer has expired -> start the timer
    doubleClickTimer->start(doubleClickInterval->value(), true);
  }
  else
  {
    // Double click event: stop the timer and change the picture
    doubleClickTimer->stop();
    if (!doubleClickStatus)
      doubleClickButton->setPixmap(locate("data", "kcminput/pics/doubleclick_2.png"));
    else
      doubleClickButton->setPixmap(locate("data", "kcminput/pics/doubleclick_1.png"));
    doubleClickStatus= !doubleClickStatus;
  }
}

void MouseConfig::slotDoubleClickTimerDone()
{
  // Stop the timer, even though not strictly needed since it is a single shot timer
  doubleClickTimer->stop();
}

/** No descriptions */
void MouseConfig::slotHandedChanged(int val){
  if(val==RIGHT_HANDED)
    tab1->mousePix->setPixmap(locate("data", "kcminput/pics/mouse_rh.png"));
  else
    tab1->mousePix->setPixmap(locate("data", "kcminput/pics/mouse_lh.png"));
  settings->m_handedNeedsApply = true;
}

void MouseSettings::load(TDEConfig *config)
{
  int accel_num, accel_den, threshold;
  double accel;
  XGetPointerControl( kapp->getDisplay(),
              &accel_num, &accel_den, &threshold );
  accel = float(accel_num) / float(accel_den);

  // get settings from X server
  int h = RIGHT_HANDED;
  unsigned char map[20];
  num_buttons = XGetPointerMapping(kapp->getDisplay(), map, 20);

  handedEnabled = true;

  // ## keep this in sync with TDEGlobalSettings::mouseSettings
  if( num_buttons == 1 )
  {
      /* disable button remapping */
      handedEnabled = false;
  }
  else if( num_buttons == 2 )
  {
      if ( (int)map[0] == 1 && (int)map[1] == 2 )
        h = RIGHT_HANDED;
      else if ( (int)map[0] == 2 && (int)map[1] == 1 )
        h = LEFT_HANDED;
      else
        /* custom button setup: disable button remapping */
        handedEnabled = false;
  }
  else
  {
      middle_button = (int)map[1];
      if ( (int)map[0] == 1 && (int)map[2] == 3 )
    h = RIGHT_HANDED;
      else if ( (int)map[0] == 3 && (int)map[2] == 1 )
    h = LEFT_HANDED;
      else
    {
      /* custom button setup: disable button remapping */
      handedEnabled = false;
    }
  }

  config->setGroup("Mouse");
  double a = config->readDoubleNumEntry("Acceleration",-1);
  if (a == -1)
    accelRate = accel;
  else
    accelRate = a;

  int t = config->readNumEntry("Threshold",-1);
  if (t == -1)
    thresholdMove = threshold;
  else
    thresholdMove = t;

  TQString key = config->readEntry("MouseButtonMapping");
  if (key == "RightHanded")
    handed = RIGHT_HANDED;
  else if (key == "LeftHanded")
    handed = LEFT_HANDED;
  else if (key == NULL)
    handed = h;
  reverseScrollPolarity = config->readBoolEntry( "ReverseScrollPolarity", false );
  m_handedNeedsApply = false;

  // SC/DC/AutoSelect/ChangeCursor
  config->setGroup("KDE");
  doubleClickInterval = config->readNumEntry("DoubleClickInterval", 400);
  dragStartTime = config->readNumEntry("StartDragTime", 500);
  dragStartDist = config->readNumEntry("StartDragDist", 4);
  wheelScrollLines = config->readNumEntry("WheelScrollLines", 3);

  singleClick = config->readBoolEntry("SingleClick", KDE_DEFAULT_SINGLECLICK);
  autoSelectDelay = config->readNumEntry("AutoSelectDelay", KDE_DEFAULT_AUTOSELECTDELAY);
  visualActivate = config->readBoolEntry("VisualActivate", KDE_DEFAULT_VISUAL_ACTIVATE);
  changeCursor = config->readBoolEntry("ChangeCursor", KDE_DEFAULT_CHANGECURSOR);
}

void MouseConfig::slotThreshChanged(int value)
{
  thresh->setSuffix(i18n(" pixel", " pixels", value));
}

void MouseConfig::slotDragStartDistChanged(int value)
{
  dragStartDist->setSuffix(i18n(" pixel", " pixels", value));
}

void MouseConfig::slotWheelScrollLinesChanged(int value)
{
  wheelScrollLines->setSuffix(i18n(" line", " lines", value));
}

void MouseSettings::apply(bool force)
{
  XChangePointerControl( kapp->getDisplay(),
                         true, true, int(tqRound(accelRate*10)), 10, thresholdMove);

  // 256 might seems extreme, but X has already been known to return 32, 
  // and we don't want to truncate things. Xlib limits the table to 256 bytes,
  // so it's a good uper bound..
  unsigned char map[256];
  num_buttons = XGetPointerMapping(kapp->getDisplay(), map, 256);

  int remap=(num_buttons>=1);
  if (handedEnabled && (m_handedNeedsApply || force)) {
      if( num_buttons == 1 )
      {
          map[0] = (unsigned char) 1;
      }
      else if( num_buttons == 2 )
      {
          if (handed == RIGHT_HANDED)
          {
              map[0] = (unsigned char) 1;
              map[1] = (unsigned char) 3;
          }
          else
          {
              map[0] = (unsigned char) 3;
              map[1] = (unsigned char) 1;
          }
      }
      else // 3 buttons and more
      {
          if (handed == RIGHT_HANDED)
          {
              map[0] = (unsigned char) 1;
              map[1] = (unsigned char) middle_button;
              map[2] = (unsigned char) 3;
          }
          else
          {
              map[0] = (unsigned char) 3;
              map[1] = (unsigned char) middle_button;
              map[2] = (unsigned char) 1;
          }
          if( num_buttons >= 5 )
          {
          // Apps seem to expect logical buttons 4,5 are the vertical wheel.
          // With mice with more than 3 buttons (not including wheel) the physical
          // buttons mapped to logical 4,5 may not be physical 4,5 , so keep
          // this mapping, only possibly reversing them.
              int pos;
              for( pos = 0; pos < num_buttons; ++pos )
                  if( map[pos] == 4 || map[pos] == 5 )
                      break;
              if( pos < num_buttons - 1 )
              {
                  map[pos] = reverseScrollPolarity ? (unsigned char) 5 : (unsigned char) 4;
                  map[pos+1] = reverseScrollPolarity ? (unsigned char) 4 : (unsigned char) 5;
              }
          }
      }
      int retval;
      if (remap)
          while ((retval=XSetPointerMapping(kapp->getDisplay(), map,
                                            num_buttons)) == MappingBusy)
              /* keep trying until the pointer is free */
          { };
      m_handedNeedsApply = false;
  }

  // This iterates through the various Logitech mice, if we have support.
  #ifdef HAVE_LIBUSB
  LogitechMouse *logitechMouse;
  for (logitechMouse = logitechMouseList.first(); logitechMouse; logitechMouse = logitechMouseList.next() ) {
      logitechMouse->applyChanges();
  }
  #endif
}

void MouseSettings::save(TDEConfig *config)
{
  config->setGroup("Mouse");
  config->writeEntry("Acceleration",accelRate);
  config->writeEntry("Threshold",thresholdMove);
  if (handed == RIGHT_HANDED)
      config->writeEntry("MouseButtonMapping",TQString("RightHanded"));
  else
      config->writeEntry("MouseButtonMapping",TQString("LeftHanded"));
  config->writeEntry( "ReverseScrollPolarity", reverseScrollPolarity );

  config->setGroup("KDE");
  config->writeEntry("DoubleClickInterval", doubleClickInterval, true, true);
  config->writeEntry("StartDragTime", dragStartTime, true, true);
  config->writeEntry("StartDragDist", dragStartDist, true, true);
  config->writeEntry("WheelScrollLines", wheelScrollLines, true, true);
  config->writeEntry("SingleClick", singleClick, true, true);
  config->writeEntry("AutoSelectDelay", autoSelectDelay, true, true );
  config->writeEntry("VisualActivate", visualActivate, true, true);
  config->writeEntry("ChangeCursor", changeCursor, true, true);
  // This iterates through the various Logitech mice, if we have support.
#ifdef HAVE_LIBUSB
  LogitechMouse *logitechMouse;
  for (logitechMouse = logitechMouseList.first(); logitechMouse; logitechMouse = logitechMouseList.next() ) {
      logitechMouse->save(config);
  }
#endif
  config->sync();
  KIPC::sendMessageAll(KIPC::SettingsChanged, TDEApplication::SETTINGS_MOUSE);
}

void MouseConfig::slotScrollPolarityChanged()
{
  settings->m_handedNeedsApply = true;
}

TQString MouseConfig::handbookSection() const
{
  int index = tabwidget->currentPageIndex();
  if (index == 0) {
    //return "mouse-general";
    return TQString::null;
  }
  else if (index == 1) {
    return "cursor-theme";
  }
  else if (index == 2) {
    return "mouse-advanced";
  }
  else if (index == 3) {
    return "mouse-navigation";
  }
  else if (index >= 4) {
    return "logitech-mouse";
  }
  else {
    return TQString::null;
  }
}

#include "mouse.moc"
