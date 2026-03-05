#ifndef __K_ACCESS_H__
#define __K_ACCESS_H__


#include <tqwidget.h>
#include <tqcolor.h>


#include <tdeuniqueapplication.h>
#include <twinmodule.h>


#include <X11/Xlib.h>
#define explicit int_explicit        // avoid compiler name clash in XKBlib.h
#include <X11/XKBlib.h>
#undef explicit

class KDialogBase;
class TQLabel;
class KComboBox;

class KAccessApp : public TDEUniqueApplication
{
  TQ_OBJECT

public:

  KAccessApp(bool allowStyles=true, bool GUIenabled=true);

  bool x11EventFilter(XEvent *event);

  int newInstance();

  void setXkbOpcode(int opcode);

protected:

  void readSettings();

  void xkbStateNotify();
  void xkbBellNotify(XkbBellNotifyEvent *event);
  void xkbControlsNotify(XkbControlsNotifyEvent *event);


private slots:

  void activeWindowChanged(WId wid);
  void slotArtsBellTimeout();
  void notifyChanges();
  void applyChanges();
  void yesClicked();
  void noClicked();
  void dialogClosed();

private:
   void  createDialogContents();
   void  initMasks();

  int xkb_opcode;
  unsigned int features;
  unsigned int requestedFeatures;

  bool    _systemBell, _artsBell, _visibleBell, _visibleBellInvert;
  bool    _artsBellBlocked;
  TQString _artsBellFile;
  TQColor  _visibleBellColor;
  int     _visibleBellPause;

  bool    _gestures, _gestureConfirmation;
  bool    _kNotifyModifiers, _kNotifyAccessX;

  TQWidget *overlay;

  TQTimer *artsBellTimer;

  TWinModule wm;

  WId _activeWindow;

  KDialogBase *dialog;
  TQLabel *featuresLabel;
  KComboBox *showModeCombobox;

  int keys[8];
  int state;
};


class VisualBell : public TQWidget
{
  TQ_OBJECT

public:

  VisualBell(int pause) 
    : TQWidget(0, 0, WX11BypassWM), _pause(pause)
    {};

  
protected:
  
  void paintEvent(TQPaintEvent *);


private:

  int _pause;

};




#endif
