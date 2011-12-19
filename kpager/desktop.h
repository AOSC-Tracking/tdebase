/**************************************************************************

    desktop.h  - KPager's desktop
    Copyright (C) 2000  Antonio Larrosa Jimenez
			Matthias Ettrich
			Matthias Elter

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    Send comments and bug fixes to larrosa@kde.org

***************************************************************************/
#ifndef __DESKTOP_H
#define __DESKTOP_H

#include <tqwidget.h>
#include <tqintdict.h>
#include <kwin.h>

class KSharedPixmap;
class KPopupMenu;

class TQPainter;
class TQPoint;

class Desktop : public TQWidget
{
    Q_OBJECT

public:
  Desktop( int desk, TQString desktopName, TQWidget *parent=0,
		const char *name=0);
  ~Desktop();

  int id() const { return m_desk; };
  bool isCurrent() const;

//  int widthForHeight(int height) const;
//  int heightForWidth(int width) const;

  static const bool c_defShowName;
  static const bool c_defShowNumber;
  static const bool c_defShowBackground;
  static const bool c_defShowWindows;
  static const bool c_defWindowDragging;
  enum WindowDrawMode { Plain=0, Icon=1, Pixmap=2 };
  enum WindowTransparentMode { NoWindows=0, MaximizedWindows=1, AllWindows=2};
  static const WindowDrawMode c_defWindowDrawMode;
  static const WindowTransparentMode c_defWindowTransparentMode;

  virtual int deskX() const { return 0; };
  virtual int deskY() const { return 0; };
  virtual int deskWidth() const { return width(); };
  virtual int deskHeight() const { return height(); };

  void startDrag(const TQPoint &point);
  void dragEnterEvent(TQDragEnterEvent *ev);
  void dragMoveEvent(TQDragMoveEvent *);
  void dropEvent(TQDropEvent *ev);
  void convertRectS2P(TQRect &r);
  void convertCoordP2S(int &x, int &y);

	static void removeCachedPixmap(int nWin) { m_windowPixmaps.remove(nWin); };

  TQSize sizeHint() const;

  /**
   * active is a bool that specifies if the frame is the active
   * one or not (so that it's painted highlighted or not)
   */
  void paintFrame(bool active);
 
  bool m_grabWindows;
public slots:
  void backgroundLoaded(bool b);

  void loadBgPixmap();

protected:
  void mousePressEvent( TQMouseEvent *ev );
  void mouseMoveEvent( TQMouseEvent *ev );
  void mouseReleaseEvent( TQMouseEvent *ev );
  
  void paintEvent( TQPaintEvent *ev );

  KWin::WindowInfo *windowAtPosition (const TQPoint &p, TQPoint *internalpos);

  bool shouldPaintWindow( KWin::WindowInfo *info );

  int m_desk;
  TQString m_name;
  KSharedPixmap *m_bgPixmap;
  bool m_bgDirty;
  TQPixmap *m_bgSmallPixmap;
  static TQPixmap *m_bgCommonSmallPixmap;
  static bool m_isCommon;
  static TQIntDict<TQPixmap> m_windowPixmaps;
  static TQMap<int,bool> m_windowPixmapsDirty;
  WindowTransparentMode m_transparentMode;

  TQPixmap *paintNewWindow(const KWin::WindowInfo *info);

  void paintWindow(TQPainter &p, const KWin::WindowInfo *info,
			bool onDesktop=true);
  void paintWindowPlain(TQPainter &p, const KWin::WindowInfo *info,
			bool onDesktop=true);
  void paintWindowIcon(TQPainter &p, const KWin::WindowInfo *info,
			bool onDesktop=true);
  void paintWindowPixmap(TQPainter &p, const KWin::WindowInfo *info,
			bool onDesktop=true);

private:
  class KPager* pager() const;
    TQPoint pressPos;

};

#endif
