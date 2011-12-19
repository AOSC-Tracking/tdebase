/**************************************************************************

    desktop.cpp  - KPager's desktop
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

#include "kpager.h"

#include <dcopobject.h>
#include <dcopclient.h>
#include <kdatastream.h>
#include <kapplication.h>
#include <kglobalsettings.h>
#include <kwinmodule.h>
#include <kwin.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <ksharedpixmap.h>
#include <kpixmapio.h>
#include <kpopupmenu.h>
#include <netwm.h>
#include <tqcstring.h>
#include <tqpixmap.h>
#include <tqpainter.h>
#include <tqdrawutil.h>
#include <tqpoint.h>

#include "desktop.h"
#include "config.h"
#include "windowdrag.h"

Desktop::Desktop( int desk, TQString desktopName, TQWidget *parent, const char *name): TQWidget(parent,name)
{
  m_desk = desk;
  m_name = desktopName;
  m_bgSmallPixmap=0L;
  m_bgCommonSmallPixmap=0L;
  m_bgPixmap = 0L;
  m_bgDirty=true;
  m_grabWindows=false;
  setAcceptDrops(TRUE);
  setBackgroundMode(NoBackground);

  if (m_desk==1) Desktop::m_windowPixmaps.setAutoDelete(true);
  KConfig *cfg= KGlobal::config();
  m_transparentMode=static_cast<WindowTransparentMode>
      (cfg->readNumEntry("windowTransparentMode", c_defWindowTransparentMode));
  resize(67, 50);
}

Desktop::~Desktop()
{
  delete m_bgPixmap;
  delete m_bgSmallPixmap;
}

void Desktop::mouseMoveEvent( TQMouseEvent *ev )
{
    if ( !KPagerConfigDialog::m_windowDragging )
	return;
    if ( (ev->state() & Qt::LeftButton) == 0 )
	return;
    TQPoint p( ev->pos() - pressPos );
    if ( p.manhattanLength() >= tqApp->startDragDistance() )
	startDrag( pressPos );
}

void Desktop::mousePressEvent( TQMouseEvent * ev)
{
    bool showWindows= KPagerConfigDialog::m_showWindows;
    if (ev->button()==Qt::LeftButton){
	pressPos = ev->pos();
    }
    else if ((ev->button()==Qt::MidButton)&&(showWindows))
	startDrag(ev->pos());
    else if (ev->button()==Qt::RightButton) {
	TQPoint pos;
	KWin::WindowInfo *info = windowAtPosition(ev->pos(), &pos);
	if ( info && showWindows )
	    pager()->showPopupMenu(info->win(), mapToGlobal(ev->pos()));
	else
	    pager()->showPopupMenu(0, mapToGlobal(ev->pos()));
    }
}

void Desktop::mouseReleaseEvent( TQMouseEvent *ev )
{
/** Note that mouseReleaseEvent is not called when releasing the mouse
 to drop a window in this desktop */
  if (ev->button()==Qt::LeftButton)
  {
    bool showWindows= KPagerConfigDialog::m_showWindows;
    TQPoint pos;
    KWin::setCurrentDesktop(m_desk);
    if (showWindows)
    {
      KWin::WindowInfo *info = windowAtPosition(ev->pos(), &pos);
      if (info)
      {
	KWin::forceActiveWindow(info->win());

	//	    if ( static_cast<WindowDrawMode>( KPagerConfigDialog::m_windowDrawMode ) == Pixmap )
	//		m_windowPixmapsDirty.replace(info->win,true);
      }
    }
  }
}

KWin::WindowInfo *Desktop::windowAtPosition(const TQPoint &p, TQPoint *internalpos)
{
	TQRect r;
	const TQValueList<WId> &list(pager()->kwin()->stackingOrder());
	if (list.count() <= 0)
		return 0L;

	for (TQValueList<WId>::ConstIterator it = list.fromLast(); ; --it)
	{
		KWin::WindowInfo* info = pager()->info( *it );
		if (shouldPaintWindow(info))
		{
			r=info->geometry();
			convertRectS2P(r);
			if (r.contains(p))
			{
				if (internalpos)
				{
					internalpos->setX(p.x()-r.x());
					internalpos->setY(p.y()-r.y());
				}
				return info;
			}
		}

		if (it == list.begin())
			break;
	}
	return 0L;
}

void Desktop::convertRectS2P(TQRect &r)
{
    TQRect tmp(r);
    r.setRect(deskX()+tmp.x()*deskWidth()/kapp->desktop()->width(),
	      deskY()+tmp.y()*deskHeight()/kapp->desktop()->height(),
	      tmp.width()*deskWidth()/kapp->desktop()->width(),
	      tmp.height()*deskHeight()/kapp->desktop()->height());
}

void Desktop::convertCoordP2S(int &x, int &y)
{
    x=(x-deskX())*(kapp->desktop()->width())/deskWidth();
    y=(y-deskY())*(kapp->desktop()->height())/deskHeight();
}

TQPixmap scalePixmap(const TQPixmap &pixmap, int width, int height)
{
  if (pixmap.width()>100)
  {
    KPixmapIO io;
    TQImage img(io.convertToImage(pixmap));
    return io.convertToPixmap(img.smoothScale(width,height));
  }

  TQImage img(TQImage(pixmap.convertToImage()).smoothScale(width,height));
  TQPixmap pix;
  pix.convertFromImage(img);

  return pix;
}

TQPixmap fastScalePixmap(const TQPixmap &pixmap, int width, int height)
{
  TQWMatrix m;
  m.scale(width/(double)pixmap.width(),
      height/(double)pixmap.height());
  return pixmap.xForm(m);
}

void Desktop::loadBgPixmap(void)
{
  bool retval;

//  if (!m_bgDirty) return;
  DCOPClient *client = kapp->dcopClient();
  if (!client->isAttached())
      client->attach();
  TQByteArray data, data2, replyData;
  TQCString replyType;
  if (client->call("kdesktop", "KBackgroundIface", "isCommon()",
                  data, replyType, replyData))
  {
    TQDataStream reply(replyData, IO_ReadOnly);
    if (replyType == "bool") {
      reply >> m_isCommon;
    }
  }
  if  ( m_isCommon && m_desk!=1 ) return;

/*
  TQDataStream args2( data2, IO_WriteOnly );
  args2 << m_desk-1 << 0 << 0 << -1 << -1 << 200 << 150 ;
  if (client->call("kdesktop", "KBackgroundIface",
	"wallpaper(int,int,int,int,int,int,int)", data2, replyType, replyData))
  {
    TQDataStream reply(replyData, IO_ReadOnly);
    if (replyType == "TQPixmap") {
      TQPixmap pixmap;
      reply >> pixmap;
      if (!pixmap.isNull())
      {
        kdDebug() << "getting small bg through dcop\n";
	if (m_isCommon)
	{
	  if (m_bgSmallPixmap) { delete m_bgSmallPixmap; m_bgSmallPixmap=0L; }

	  if (!m_bgCommonSmallPixmap) m_bgCommonSmallPixmap=new TQPixmap(pixmap);
	  else *m_bgCommonSmallPixmap=pixmap;
	}
	else
	{
	  if (m_bgCommonSmallPixmap)
	  {
	    delete m_bgCommonSmallPixmap;
	    m_bgCommonSmallPixmap=0L;
	  }

	  if (!m_bgSmallPixmap) m_bgSmallPixmap=new TQPixmap(pixmap);
	  else *m_bgSmallPixmap=pixmap;
	}
        return;
      }
    }
  }
  kdDebug() << "getting whole bg through shpixmap\n";
 */

  if (!m_bgPixmap)
  {
     m_bgPixmap = new KSharedPixmap;
     connect(m_bgPixmap, TQT_SIGNAL(done(bool)), TQT_SLOT(backgroundLoaded(bool)));
  }

  retval = m_bgPixmap->loadFromShared(TQString("DESKTOP%1").arg(m_isCommon?1:m_desk));
  if (retval == false) {
    TQDataStream args( data, IO_WriteOnly );
    args << 1;	// Argument is 1 (true)
    client->send("kdesktop", "KBackgroundIface", "setExport(int)", data);
    retval = m_bgPixmap->loadFromShared(TQString("DESKTOP%1").arg(m_isCommon?1:m_desk));
  }

}

void Desktop::paintWindow(TQPainter &p, const KWin::WindowInfo *info, bool onDesktop)
{
    switch (static_cast<WindowDrawMode>(KPagerConfigDialog::m_windowDrawMode ) )
	{
	case (Plain)  : paintWindowPlain (p, info, onDesktop);break;
	case (Icon)   : paintWindowIcon  (p, info, onDesktop);break;
	case (Pixmap) : paintWindowPixmap(p, info, onDesktop);break;
	}
}

TQPixmap *Desktop::paintNewWindow(const KWin::WindowInfo *info)
{
    TQRect r = info->frameGeometry();
    int dw = TQApplication::desktop()->width();
    int dh = TQApplication::desktop()->height();
    r = TQRect( r.x() * width() / dw, 2 + r.y() * height() / dh,
	       r.width() * width() / dw, r.height() * height() / dh );
    r.moveTopLeft(TQPoint(0,0));


    TQPixmap *pixmap=new TQPixmap(r.width(),r.height());
    TQPainter p;

    p.begin(pixmap);
    p.setFont(font());
    p.fillRect( r, colorGroup().brush(TQColorGroup::Dark));
    paintWindow(p, info, false);
    p.end();

    return pixmap;
}

void Desktop::startDrag(const TQPoint &p)
{
  TQPoint dragpos;
  KWin::WindowInfo *info=windowAtPosition(p,&dragpos);
  if ( (!info)/* || (info->state & NET::Max)*/ ) return;

  TQPixmap *pixmap=paintNewWindow(info);

  int deltax=dragpos.x();
  int deltay=dragpos.y();
  PagerWindowDrag *wdrag= new PagerWindowDrag( info->win(), deltax, deltay,
				m_desk, this);
  wdrag->setPixmap( *pixmap, TQPoint( deltax, deltay) );
  delete pixmap;
  wdrag->dragCopy();

}

void Desktop::dragEnterEvent(TQDragEnterEvent *ev)
{
    if (PagerWindowDrag::canDecode( ev )) ev->accept();
}

void Desktop::dragMoveEvent(TQDragMoveEvent *)
{
    // TODO Moving the window while dragging would be cool, wouldn't it ?
    // Matthias: No, is way to slow on low end machines.
    // Antonio:Ok, I'll make it configurable after 2.0 (it would add a string)
}

void Desktop::dropEvent(TQDropEvent *ev)
{
  WId win=0;
  int deltax,deltay;
  int origdesk;
  if (!PagerWindowDrag::decode(ev,win,deltax,deltay,origdesk)) return;

  int x=ev->pos().x()-deltax;
  int y=ev->pos().y()-deltay;
  /*
   * x and y now contain the position (in local coordinates) which
   * has the origin of the window
   */
  convertCoordP2S(x,y);

//  kdDebug() << "moving window " << win << "d from " << origdesk << " to " << m_desk << endl;
//  NETWinInfo NETinfo( qt_xdisplay(),  win, qt_xrootwin(), NET::Client | NET::WMDesktop);

  if (m_desk==0)
  {
    /*
     * The next line moves the window to the active desktop. This is done
     * because in other case, kwm raises the window when it's in a semi
     * changed state and doesn't work well with kpager. Let's see how well
     * KWin behaves.
     * if (activedesktop!=KWM::desktop(w))
     *  KWM::moveToDesktop(w,activedesktop);
     */
//    KWin::setState(win, NET::Sticky);
    KWin::setOnAllDesktops(win, true);
  }
  else
  {
    if (origdesk==0) KWin::setOnAllDesktops(win, false);

    KWin::WindowInfo *info = pager()->info(win);
    if (!info->onAllDesktops())
      KWin::setOnDesktop(win, m_desk);
  }

  XMoveWindow(x11Display(), win, x, y );
}

bool Desktop::shouldPaintWindow( KWin::WindowInfo *info )
{
  if (!info)
    return false;

//  if (info->mappingState != NET::Visible)
//    return false;

  NET::WindowType type = info->windowType( NET::NormalMask | NET::DesktopMask
      | NET::DockMask | NET::ToolbarMask | NET::MenuMask | NET::DialogMask
      | NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask );
  if (type == NET::Desktop || type == NET::Dock
      || type == NET::TopMenu)
    return false;

  if (!info->isOnDesktop(m_desk))
    return false;

  if (info->state() & NET::SkipPager
      || info->state() & NET::Shaded )
    return false;

  if (info->win() == pager()->winId())
    return false;

  if ( info->isMinimized() )
    return false;

  return true;
}

void Desktop::paintFrame(bool active)
{
  TQPainter p(this);

  if ( active )
     p.setPen(yellow);
  else
     p.setPen(TQColorGroup::Base);
  p.drawRect(rect());
  p.end();
}

void Desktop::paintEvent( TQPaintEvent * )
{
  TQPixmap pixmap(width(),height());
  TQPainter p;

  p.begin(&pixmap);
//  p.setFont(font());
//  p.fillRect(rect(), colorGroup().brush(TQColorGroup::Dark));
//  p.setPen(Qt::black);
//  p.drawRect(rect());

  if (KPagerConfigDialog::m_showBackground )
  {
    if ( ( !m_isCommon && !m_bgSmallPixmap )
      || (m_isCommon && !m_bgCommonSmallPixmap) )
	loadBgPixmap();

    if ( ( !m_isCommon && m_bgSmallPixmap && !m_bgSmallPixmap->isNull() )
	|| ( m_isCommon &&
		m_bgCommonSmallPixmap && !m_bgCommonSmallPixmap->isNull() ) )
    {
      TQPixmap tmp;
      if ( m_isCommon )
	tmp=fastScalePixmap(*m_bgCommonSmallPixmap, width(),height());
      else
	tmp=fastScalePixmap(*m_bgSmallPixmap, width(),height());

      p.drawPixmap(0,0,tmp);
    }
     else pixmap.fill(Qt::gray);
  }
  else
    p.fillRect(rect(), colorGroup().brush(TQColorGroup::Mid));

    // set in/active pen
  if (isCurrent())
    p.setPen(yellow);
  else
    p.setPen(TQColorGroup::Base);

    // paint number & name
    bool sname=KPagerConfigDialog::m_showName;
    bool snumber=KPagerConfigDialog::m_showNumber;
    if ( sname || snumber ) {
	TQString txt;

	// set font
	if (sname) {
	    TQFont f(KGlobalSettings::generalFont().family(), 10, TQFont::Bold);
	    p.setFont(f);
	}
	else {
	    TQFont f(KGlobalSettings::generalFont().family(), 12, TQFont::Bold);
	    p.setFont(f);
	}

	// draw text
	if ( sname && snumber )
	    txt=TQString("%1. %2").arg(m_desk).arg(pager()->kwin()->desktopName( m_desk ));
	else if ( sname )
	    txt=pager()->kwin()->desktopName( m_desk );
	else if ( snumber )
	    txt=TQString::number( m_desk );
	p.drawText(2, 0, width()-4, height(), AlignCenter, txt );
    }

    // paint windows
    if ( KPagerConfigDialog::m_showWindows ) {
	TQValueList<WId>::ConstIterator it;
	for ( it = pager()->kwin()->stackingOrder().begin();
	      it != pager()->kwin()->stackingOrder().end(); ++it ) {

	    KWin::WindowInfo* info = pager()->info( *it );

	    if (shouldPaintWindow(info))
		paintWindow(p,info);
	}
    }

    // paint border rectangle
    p.drawRect(rect());
    p.end();

    // blit pixmap to widget
    p.begin(this);
    p.drawPixmap(0,0,pixmap);
    p.end();

    m_grabWindows=false;
}

void Desktop::paintWindowPlain(TQPainter &p, const KWin::WindowInfo *info, bool onDesktop)
{
    TQRect r =  info->frameGeometry();
    int dw = TQApplication::desktop()->width();
    int dh = TQApplication::desktop()->height();
    r = TQRect( r.x() * width() / dw, 2 + r.y() * height() / dh,
	       r.width() * width() / dw, r.height() * height() / dh );
    if ( !onDesktop )
	r.moveTopLeft(TQPoint(0,0));

  bool isActive=(pager()->kwin()->activeWindow() == info->win());

  TQBrush brush;

  if ( isActive ) brush=colorGroup().brush( TQColorGroup::Highlight );
  else brush=colorGroup().brush(  TQColorGroup::Button );

  if ( m_transparentMode==AllWindows
      || (m_transparentMode==MaximizedWindows && ( info->state() & NET::Max )) )
    brush.setStyle(Qt::Dense4Pattern);

  if ( isActive )
  {
    qDrawShadeRect( &p, r, colorGroup(), false, 1, 0, &brush );
  }
  else
  {
    p.fillRect( r, brush );
    qDrawShadeRect( &p, r, colorGroup(), true, 1, 0 );
  }

}


void Desktop::paintWindowIcon(TQPainter &p, const KWin::WindowInfo *info, bool onDesktop)
{
  TQRect r =  info->frameGeometry();
  int dw = TQApplication::desktop()->width();
  int dh = TQApplication::desktop()->height();
  r = TQRect( r.x() * width() / dw, 2 + r.y() * height() / dh,
      r.width() * width() / dw, r.height() * height() / dh );
  TQPixmap icon=KWin::icon( info->win(), int(r.width()*0.8),
			   int(r.height()*0.8), true);

  NET::WindowType type = info->windowType( NET::NormalMask | NET::DesktopMask
      | NET::DockMask | NET::ToolbarMask | NET::MenuMask | NET::DialogMask
      | NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask );
  if ( icon.isNull() || type!=NET::Override )
    paintWindowPlain(p,info,onDesktop);

  if ( !onDesktop )
    r.moveTopLeft(TQPoint(0,0));

  p.drawPixmap( r.topLeft()+ TQPoint(int(r.width()*0.1),int(r.height()*0.1)),
		icon );

}

void Desktop::paintWindowPixmap(TQPainter &p, const KWin::WindowInfo *info,
					bool onDesktop)
{
	const int knDefaultPixmapWd = 100;
	const int knDefaultPixmapHg = 75;
  TQRect rSmall, r =  info->frameGeometry();

  int dw = TQApplication::desktop()->width();
  int dh = TQApplication::desktop()->height();
  rSmall = TQRect( r.x() * width() / dw, 2 + r.y() * height() / dh,
      r.width() * width() / dw, r.height() * height() / dh );

  TQPixmap *pixmap=m_windowPixmaps[info->win()];
  bool isDirty=m_windowPixmapsDirty[info->win()];
  if ( !pixmap || isDirty || m_grabWindows )
  {
    if ( isCurrent() )
    {
      TQPixmap tmp=TQPixmap::grabWindow(info->win(),
			0,0,r.width(),r.height());
      if (!tmp.isNull() && tmp.width() > 0 && tmp.height() > 0)
      {
	tmp.setOptimization(TQPixmap::BestOptim);
	int nWd, nHg;
	if (rSmall.width() > knDefaultPixmapWd || rSmall.height() > knDefaultPixmapHg)
	{
		nWd = knDefaultPixmapWd;
		nHg = knDefaultPixmapHg;
	}
	else
	{
		nWd = rSmall.width();
		nHg = rSmall.height();
	}
	pixmap=new TQPixmap(fastScalePixmap(tmp, nWd, nHg));
	m_windowPixmaps.replace(info->win(),pixmap);
	m_windowPixmapsDirty.replace(info->win(),false);
      }
    }

    // It was impossible to get the pixmap, let's fallback to the icon mode.
    if ( !pixmap || pixmap->isNull() )
    {
      paintWindowIcon(p, info, onDesktop);
      return;
    }

  }

  if ( !onDesktop )
    rSmall.moveTopLeft(TQPoint(0,0));

  if (rSmall.width() != pixmap->width() || rSmall.height() != pixmap->height())
	{
		TQPixmap pixmapSmall(fastScalePixmap(*pixmap,rSmall.width(),rSmall.height()));
		p.drawPixmap( rSmall.topLeft(), pixmapSmall );
	}
	else
	{
		p.drawPixmap( rSmall.topLeft(), *pixmap);
	}

}

KPager *Desktop::pager() const
{
  return reinterpret_cast<KPager *>(parent());
}

bool Desktop::isCurrent() const
{
  return pager()->kwin()->currentDesktop()==m_desk;
}

void Desktop::backgroundLoaded(bool b)
{
  if (b)
  {
    if (m_isCommon)
    {
      if (m_bgSmallPixmap) { delete m_bgSmallPixmap; m_bgSmallPixmap=0L ; };

      if (!m_bgCommonSmallPixmap) m_bgCommonSmallPixmap=new TQPixmap;
      *m_bgCommonSmallPixmap=scalePixmap(*m_bgPixmap,200,150);
    }
    else
    {
      if (m_bgCommonSmallPixmap) { delete m_bgCommonSmallPixmap;
		m_bgCommonSmallPixmap=0L ; };

      if (!m_bgSmallPixmap) m_bgSmallPixmap=new TQPixmap;
      *m_bgSmallPixmap=fastScalePixmap(*m_bgPixmap,200,150);
    }
    delete m_bgPixmap;
    m_bgPixmap=0L;


    if (m_isCommon) pager()->redrawDesktops();
    else update();
  } else kdDebug() << "Error getting the background\n";
}

TQSize Desktop::sizeHint() const
{
  return TQSize(67,50);
}

TQPixmap *Desktop::m_bgCommonSmallPixmap=0L;
bool Desktop::m_isCommon=false;
TQIntDict<TQPixmap> Desktop::m_windowPixmaps;
TQMap<int,bool> Desktop::m_windowPixmapsDirty;

// Default Configuration -------------------------------------------------

const bool Desktop::c_defShowName=false;
const bool Desktop::c_defShowNumber=false;
const bool Desktop::c_defShowWindows=true;
const bool Desktop::c_defShowBackground=true;
const bool Desktop::c_defWindowDragging=true;
const Desktop::WindowDrawMode Desktop::c_defWindowDrawMode=Desktop::Icon;
const Desktop::WindowTransparentMode
		Desktop::c_defWindowTransparentMode=Desktop::AllWindows;
#include "desktop.moc"
