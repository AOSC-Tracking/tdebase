/**
 * kcmaccess.h
 *
 * Copyright (c) 2000 Matthias H�zer-Klpfel <hoelzer@kde.org>
 *
 */

#ifndef __kcmaccess_h__
#define __kcmaccess_h__


#include <tdecmodule.h>
#include <knuminput.h>


class TQCheckBox;
class TQComboBox;
class TQLabel;
class TQLineEdit;
class TQPushButton;
class TQRadioButton;
class KColorButton;
class TQSlider;
class KNumInput;
class TDEAboutData;

class ExtendedIntNumInput : public KIntNumInput
{
	TQ_OBJECT

	public:
    /**
	  * Constructs an input control for integer values
	  * with base 10 and initial value 0.
	  */
		ExtendedIntNumInput(TQWidget *parent=0, const char *name=0);
		
    /**
	  * Destructor
	  */
		virtual ~ExtendedIntNumInput();

    /**
	  * @param min  minimum value
	  * @param max  maximum value
	  * @param step step size for the QSlider
	  * @param slider whether the slider is created or not
	  */
		void setRange(int min, int max, int step=1, bool slider=true);

	private slots:
		void slotSpinValueChanged(int);
		void slotSliderValueChanged(int);
	
	private:
		int min, max;
		int sliderMax;
};

class KAccessConfig : public TDECModule
{
  TQ_OBJECT

public:

  KAccessConfig(TQWidget *parent = 0L, const char *name = 0L);
  virtual ~KAccessConfig();
  
  void load();
  void load(bool useDefaults);
  void save();
  void defaults();
  virtual TQString handbookSection() const;

protected slots:

  void configChanged();
  void checkAccess();
  void invertClicked();
  void flashClicked();
  void selectSound();
  void changeFlashScreenColor();
  void configureKNotify();

private:

	TQTabWidget* tabWidget;
  TQCheckBox *systemBell, *customBell, *visibleBell;
  TQRadioButton *invertScreen, *flashScreen;
  TQLabel    *soundLabel, *colorLabel;
  TQLineEdit *soundEdit;
  TQPushButton *soundButton;
  KColorButton *colorButton;
  ExtendedIntNumInput *durationSlider;

  TQCheckBox *stickyKeys, *stickyKeysLock, *stickyKeysAutoOff;
  TQCheckBox *stickyKeysBeep, *toggleKeysBeep, *kNotifyModifiers;
  TQPushButton *kNotifyModifiersButton;

  TQCheckBox *slowKeys, *bounceKeys;
  ExtendedIntNumInput *slowKeysDelay, *bounceKeysDelay;
  TQCheckBox *slowKeysPressBeep, *slowKeysAcceptBeep;
  TQCheckBox *slowKeysRejectBeep, *bounceKeysRejectBeep;

  TQCheckBox *gestures, *gestureConfirmation;
  TQCheckBox *timeout;
  KIntNumInput *timeoutDelay;
  TQCheckBox *accessxBeep, *kNotifyAccessX;
  TQPushButton *kNotifyAccessXButton;
};


#endif
