#ifndef KSYSTRAYCMD_H
#define KSYSTRAYCMD_H

#include <tqlabel.h>
#include <twin.h>

class KShellProcess;
class TWinModule;

/**
 * Provides a system tray icon for a normal window.
 *
 * @author Richard Moore, rich@kde.org
 */
class KSysTrayCmd : public TQLabel
{
  TQ_OBJECT
public:
  KSysTrayCmd();
  ~KSysTrayCmd();

  void setWindow( WId w ) { win = w; }
  void setCommand( const TQString &cmd ) { command = cmd; }
  void setPattern( const TQString &regexp ) { window = regexp; }
    void setStartOnShow( bool enable ) { lazyStart = enable; isVisible = !enable; }
  void setNoQuit( bool enable ) { noquit = enable; }
  void setQuitOnHide( bool enable ) { quitOnHide = enable; }
  void setOnTop( bool enable ) { onTop = enable; }
  void setOwnIcon( bool enable ) { ownIcon = enable; }
  void setDefaultTip( const TQString &tip ) { tooltip = tip; }
  bool hasTargetWindow() const { return (win != 0); }
  bool hasRunningClient() const { return (client != 0); }
  const TQString &errorMsg() const { return errStr; }

  bool start();

  static WId findRealWindow( WId w, int depth = 0 );

public slots:
  void refresh();

  void showWindow();
  void hideWindow();
  void toggleWindow() { if ( isVisible ) hideWindow(); else showWindow(); }

  void setTargetWindow( WId w );
  void execContextMenu( const TQPoint &pos );

  void quit();
  void quitClient();

protected slots:
  void clientExited();

  void windowAdded(WId w);
  void windowChanged(WId w);

protected:
  bool startClient();
  void checkExistingWindows();
  void setTargetWindow( const KWin::WindowInfo &info );

  void mousePressEvent( TQMouseEvent *e );

private:
  TQString command;
  TQString window;
  TQString tooltip;
  bool isVisible;
  bool lazyStart;
  bool noquit;
  bool quitOnHide;
  bool onTop; ///< tells if window must stay on top or not
  bool ownIcon; ///< tells if the ksystraycmd icon must be used in systray

  WId win;
  KShellProcess *client;
  TWinModule *twinmodule;
  TQString errStr;

  /** Memorized 'top' position of the window*/
  int top;
  /** Memorized 'left' position of the window*/
  int left;
};

#endif // KSYSTRAYCMD_H
