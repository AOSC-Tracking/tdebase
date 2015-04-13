/*
 * windows.h
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
 */

#ifndef __KWINDOWCONFIG_H__
#define __KWINDOWCONFIG_H__

#include <tqwidget.h>
#include <tdecmodule.h>
#include <config.h>

class TQRadioButton;
class TQCheckBox;
class TQPushButton;
class TQComboBox;
class TQGroupBox;
class TQLabel;
class TQSlider;
class TQButtonGroup;
class TQSpinBox;
class TQVButtonGroup;

class KColorButton;
class KIntNumInput;

#define TRANSPARENT 0
#define OPAQUE      1

#define CLICK_TO_FOCUS     0
#define FOCUS_FOLLOW_MOUSE 1

#define TITLEBAR_PLAIN  0
#define TITLEBAR_SHADED 1

#define RESIZE_TRANSPARENT  0
#define RESIZE_OPAQUE       1

#define SMART_PLACEMENT        0
#define MAXIMIZING_PLACEMENT   1
#define CASCADE_PLACEMENT      2
#define RANDOM_PLACEMENT       3
#define CENTERED_PLACEMENT     4
#define ZEROCORNERED_PLACEMENT 5
#define INTERACTIVE_PLACEMENT  6
#define MANUAL_PLACEMENT       7

#define  CLICK_TO_FOCUS               0
#define  FOCUS_FOLLOWS_MOUSE          1
#define  FOCUS_UNDER_MOUSE            2
#define  FOCUS_STRICTLY_UNDER_MOUSE   3

class TQSpinBox;

class KFocusConfig : public TDECModule
{
  Q_OBJECT
public:
  KFocusConfig( bool _standAlone, TDEConfig *_config, TQWidget *parent=0, const char* name=0 );
  ~KFocusConfig();

  void load();
  void save();
  void defaults();

private slots:
  void setDelayFocusEnabled();
  void setAutoRaiseEnabled();
  void autoRaiseOnTog(bool);//CT 23Oct1998
  void delayFocusOnTog(bool);
  void clickRaiseOnTog(bool);
  void updateAltTabMode();
  void updateActiveMouseScreen();
	void changed() { emit TDECModule::changed(true); }


private:

  int getFocus( void );
  int getAutoRaiseInterval( void );
  int getDelayFocusInterval( void );

  void setFocus(int);
  void setAutoRaiseInterval(int);
  void setAutoRaise(bool);
  void setDelayFocusInterval(int);
  void setDelayFocus(bool);
  void setClickRaise(bool);
  void setSeparateScreenFocus(bool);
  void setActiveMouseScreen(bool);
  void setAltTabMode(bool);
  void setTraverseAll(bool);
  void setRollOverDesktops(bool);
  void setShowPopupinfo(bool);
  void setFocusStealing(int);

  TQButtonGroup *fcsBox;
  TQComboBox *focusCombo;
  TQCheckBox *autoRaiseOn;
  TQCheckBox *delayFocusOn;
  TQCheckBox *clickRaiseOn;
  KIntNumInput *autoRaise;
  KIntNumInput *delayFocus;
  TQCheckBox *separateScreenFocus;
  TQCheckBox *activeMouseScreen;
  TQComboBox* focusStealing;

  TQButtonGroup *kbdBox;
  TQCheckBox    *altTabPopup;
  TQCheckBox    *traverseAll;
  TQCheckBox    *rollOverDesktops;
  TQCheckBox    *showPopupinfo;

  TDEConfig *config;
  bool     standAlone;
};

class KMovingConfig : public TDECModule
{
  Q_OBJECT
public:
  KMovingConfig( bool _standAlone, TDEConfig *config, TQWidget *parent=0, const char* name=0 );
  ~KMovingConfig();

  void load();
  void save();
  void defaults();

private slots:
  void setMinimizeAnim( bool );
  void setMinimizeAnimSpeed( int );
	void changed() { emit TDECModule::changed(true); }
  void slotBrdrSnapChanged( int );
  void slotWndwSnapChanged( int );

private:
  int getMove( void );
  bool getMinimizeAnim( void );
  int getMinimizeAnimSpeed( void );
  int getResizeOpaque ( void );
  bool getGeometryTip( void ); //KS
  int getPlacement( void ); //CT

  void setMove(int);
  void setResizeOpaque(int);
  void setGeometryTip(bool); //KS
  void setPlacement(int); //CT
  void setMoveResizeMaximized(bool);

  TQButtonGroup *windowsBox;
  TQCheckBox *opaque;
  TQCheckBox *resizeOpaqueOn;
  TQCheckBox *geometryTipOn;
  TQCheckBox* minimizeAnimOn;
  TQSlider *minimizeAnimSlider;
  TQLabel *minimizeAnimSlowLabel, *minimizeAnimFastLabel;
  TQCheckBox *moveResizeMaximized;

  TQComboBox *placementCombo;

  TDEConfig *config;
  bool     standAlone;

  int getBorderSnapZone();
  void setBorderSnapZone( int );
  int getWindowSnapZone();
  void setWindowSnapZone( int );

  TQVButtonGroup *MagicBox;
  KIntNumInput *BrdrSnap, *WndwSnap;
  TQCheckBox *OverlapSnap;

};

class KAdvancedConfig : public TDECModule
{
  Q_OBJECT
public:
  KAdvancedConfig( bool _standAlone, TDEConfig *config, TQWidget *parent=0, const char* name=0 );
  ~KAdvancedConfig();

  void load();
  void save();
  void defaults();

private slots:
  void shadeHoverChanged(bool);

  //copied from kcontrol/konq/twindesktop, aleXXX
  void setEBorders();

  void changed() { emit TDECModule::changed(true); }

private:

  int getShadeHoverInterval (void );
  void setAnimateShade(bool);
  void setShadeHover(bool);
  void setShadeHoverInterval(int);

  TQCheckBox *animateShade;
  TQButtonGroup *shBox;
  TQCheckBox *shadeHoverOn;
  KIntNumInput *shadeHover;

  TDEConfig *config;
  bool     standAlone;

  int getElectricBorders( void );
  int getElectricBorderDelay();
  void setElectricBorders( int );
  void setElectricBorderDelay( int );

  TQVButtonGroup *electricBox;
  TQRadioButton *active_disable;
  TQRadioButton *active_move;
  TQRadioButton *active_always;
  KIntNumInput *delays;
  
  void setHideUtilityWindowsForInactive( bool );

  TQCheckBox* hideUtilityWindowsForInactive;
};

class TDEProcess;
class KTranslucencyConfig : public TDECModule
{
  Q_OBJECT
public:
  KTranslucencyConfig( bool _standAlone, TDEConfig *config, TQWidget *parent=0, const char* name=0 );
  ~KTranslucencyConfig();
  
  void load();
  void save();
  void defaults();
  
private:
  TQCheckBox *useTranslucency;
  TQCheckBox *activeWindowTransparency;
  TQCheckBox *inactiveWindowTransparency;
  TQCheckBox *movingWindowTransparency;
  TQCheckBox *dockWindowTransparency;
  TQCheckBox *keepAboveAsActive;
  TQCheckBox *disableARGB;
  TQCheckBox *useOpenGL;
  TQCheckBox *blurBackground;
  TQCheckBox *greyscaleBackground;
  TQCheckBox *fadeInWindows;
  TQCheckBox *fadeInMenuWindows;
  TQCheckBox *fadeInToolTipWindows;
  TQCheckBox *fadeOnOpacityChange;
  TQCheckBox *useShadows;
  TQCheckBox *useShadowsOnMenuWindows;
  TQCheckBox *useShadowsOnToolTipWindows;
  TQCheckBox *useShadowsOnDockWindows;
  TQCheckBox *removeShadowsOnResize;
  TQCheckBox *removeShadowsOnMove;
  TQGroupBox *sGroup;
  TQCheckBox *onlyDecoTranslucent;
//   TQPushButton *xcompmgrButton;
  KIntNumInput *activeWindowOpacity;
  KIntNumInput *inactiveWindowOpacity;
  KIntNumInput *movingWindowOpacity;
  KIntNumInput *dockWindowOpacity;
  KIntNumInput *dockWindowShadowSize;
  KIntNumInput *menuWindowShadowSize;
  KIntNumInput *activeWindowShadowSize;
  KIntNumInput *inactiveWindowShadowSize;
  KIntNumInput *baseShadowSize;
  KIntNumInput *shadowTopOffset;
  KIntNumInput *shadowLeftOffset;
  KIntNumInput *fadeInSpeed;
  KIntNumInput *fadeOutSpeed;
  KColorButton *shadowColor;
  TDEConfig *config;
  bool standAlone;
  bool alphaActivated;
  bool resetKompmgr_;
  bool kompmgrAvailable();
  void startKompmgr();
  void stopKompmgr();
  bool kompmgrAvailable_;
  TDEProcess *kompmgr;

private slots:
  void resetKompmgr();
  void showWarning(bool alphaActivated);
  void processShadowLockouts();

};
#endif
