#include "test.h"

#include <tqtooltip.h>
#include <kglobal.h>
#include <kdebug.h>

namespace KWinTest
{

Decoration::Decoration( KDecorationBridge* bridge, KDecorationFactory* factory )
    :   KDecoration( bridge, factory ),
	button( NULL )
    {
    }
    
void Decoration::init()
    {
    createMainWidget();
    widget()->setEraseColor( red );
    widget()->installEventFilter( this );
    if( isCloseable())
	{
        button = new TQPushButton( widget());
        button->show();
        button->setCursor( tqarrowCursor );
	button->move( 0, 0 );
        connect( button, TQT_SIGNAL( clicked()), TQT_SLOT( closeWindow()));
	TQToolTip::add( button, "Zelva Mana" );
	}
    }
    
Decoration::MousePosition Decoration::mousePosition( const TQPoint& p ) const
    {
    const int range = 16;
    const int border = 4;

    MousePosition m = Nowhere;

    int width = widget()->width();
    int height = widget()->height();
    if ( ( p.x() > border && p.x() < width - border )
         && ( p.y() > border && p.y() < height - border ) )
        return Center;

    if ( p.y() <= range && p.x() <= range)
        m = TopLeft2;
    else if ( p.y() >= height-range && p.x() >= width-range)
        m = BottomRight2;
    else if ( p.y() >= height-range && p.x() <= range)
        m = BottomLeft2;
    else if ( p.y() <= range && p.x() >= width-range)
        m = TopRight2;
    else if ( p.y() <= border )
        m = Top;
    else if ( p.y() >= height-border )
        m = Bottom;
    else if ( p.x() <= border )
        m = Left;
    else if ( p.x() >= width-border )
        m = Right;
    else
        m = Center;
    return m;
    }
    
void Decoration::borders( int& left, int& right, int& top, int& bottom ) const
    {
    if( options()->preferredBorderSize( factory()) == BorderTiny )
        {
        left = right = bottom = 1;
        top = 5;
        }
    else
        {
        left = right = options()->preferredBorderSize( factory()) * 5;
        top = options()->preferredBorderSize( factory()) * 10;
        bottom = options()->preferredBorderSize( factory()) * 2;
        }
    if( isShade())
        bottom = 0;
    if( ( maximizeMode() & MaximizeHorizontal ) && !options()->moveResizeMaximizedWindows())
        left = right = 0;
    if( ( maximizeMode() & MaximizeVertical ) && !options()->moveResizeMaximizedWindows())
        bottom = 0;
    }

void Decoration::reset( unsigned long )
    {
    }
    
void Decoration::resize( const TQSize& s )
    {
    widget()->resize( s );
    }
    
TQSize Decoration::minimumSize() const
    {
    return TQSize( 100, 50 );
    }
    
bool Decoration::eventFilter( TQObject* o, TQEvent* e )
    {
    if( TQT_BASE_OBJECT(o) == TQT_BASE_OBJECT(widget()))
        {
        switch( e->type())
            {
            case TQEvent::MouseButtonPress:
	        { // FRAME
                processMousePressEvent( TQT_TQMOUSEEVENT( e ));
        	return true;
	        }
            case TQEvent::Show:
                break;
            case TQEvent::Hide:
                break;
            default:
                break;
            }
        }
    return false;
    }

}
#include <tqapplication.h>
#include <tqpainter.h>
#include <X11/Xlib.h>
#include <math.h>
#include <unistd.h>
namespace KWinTest
{

// taken from riscos
bool Decoration::animateMinimize(bool iconify)
{
  int style = 1;
  switch (style) {

    case 1:
      {
        // Double twisting double back, with pike ;)

        if (!iconify) // No animation for restore.
          return true;

        // Go away quick.
        helperShowHide( false );
        tqApp->syncX();

        TQRect r = iconGeometry();

        if (!r.isValid())
          return true;

        // Algorithm taken from Window Maker (http://www.windowmaker.org)

        int sx = geometry().x();
        int sy = geometry().y();
        int sw = width();
        int sh = height();
        int dx = r.x();
        int dy = r.y();
        int dw = r.width();
        int dh = r.height();

        double steps = 12;

        double xstep = double((dx-sx)/steps);
        double ystep = double((dy-sy)/steps);
        double wstep = double((dw-sw)/steps);
        double hstep = double((dh-sh)/steps);

        double cx = sx;
        double cy = sy;
        double cw = sw;
        double ch = sh;

        double finalAngle = 3.14159265358979323846;

        double delta  = finalAngle / steps;

        TQPainter p( workspaceWidget());
        p.setRasterOp(TQt::NotROP);

        for (double angle = 0; ; angle += delta) {

          if (angle > finalAngle)
            angle = finalAngle;

          double dx = (cw / 10) - ((cw / 5) * sin(angle));
          double dch = (ch / 2) * cos(angle);
          double midy = cy + (ch / 2);

          TQPoint p1(int(cx + dx), int(midy - dch));
          TQPoint p2(int(cx + cw - dx), p1.y());
          TQPoint p3(int(cx + dw + dx), int(midy + dch));
          TQPoint p4(int(cx - dx), p3.y());

          grabXServer();

          p.drawLine(p1, p2);
          p.drawLine(p2, p3);
          p.drawLine(p3, p4);
          p.drawLine(p4, p1);

          p.flush();

          usleep(500);

          p.drawLine(p1, p2);
          p.drawLine(p2, p3);
          p.drawLine(p3, p4);
          p.drawLine(p4, p1);

          ungrabXServer();

// FRAME          tqApp->processEvents(); // FRAME ???

          cx += xstep;
          cy += ystep;
          cw += wstep;
          ch += hstep;

          if (angle >= finalAngle)
            break;
        }
      }
      break;

    case 2:
      {
        // KVirc style ? Maybe. For qwertz.

        if (!iconify) // No animation for restore.
          return true;

        // Go away quick.
        helperShowHide( false );
        
        tqApp->syncX();

        int stepCount = 12;

        TQRect r(geometry());

        int dx = r.width() / (stepCount * 2);
        int dy = r.height() / (stepCount * 2);

        TQPainter p( workspaceWidget());
        p.setRasterOp(TQt::NotROP);

        for (int step = 0; step < stepCount; step++) {

          r.moveBy(dx, dy);
          r.setWidth(r.width() - 2 * dx);
          r.setHeight(r.height() - 2 * dy);

          grabXServer();

          p.drawRect(r);
          p.flush();
          usleep(200);
          p.drawRect(r);

          ungrabXServer();

// FRAME          tqApp->processEvents();
        }
      }
      break;


    default:
      {
        TQRect icongeom = iconGeometry();

        if (!icongeom.isValid())
          return true;

        TQRect wingeom = geometry();

        TQPainter p( workspaceWidget());

        p.setRasterOp(TQt::NotROP);

#if 0
        if (iconify)
          p.setClipRegion(
              TQRegion( workspaceWidget()->rect()) - wingeom
          );
#endif

        grabXServer();

        p.drawLine(wingeom.bottomRight(), icongeom.bottomRight());
        p.drawLine(wingeom.bottomLeft(), icongeom.bottomLeft());
        p.drawLine(wingeom.topLeft(), icongeom.topLeft());
        p.drawLine(wingeom.topRight(), icongeom.topRight());

        p.flush();

        tqApp->syncX();

        usleep(30000);

        p.drawLine(wingeom.bottomRight(), icongeom.bottomRight());
        p.drawLine(wingeom.bottomLeft(), icongeom.bottomLeft());
        p.drawLine(wingeom.topLeft(), icongeom.topLeft());
        p.drawLine(wingeom.topRight(), icongeom.topRight());

        ungrabXServer();
      }
      break;
  }
  return true;
}

KDecoration* Factory::createDecoration( KDecorationBridge* bridge )
    {
    NET::WindowType type = windowType( SUPPORTED_WINDOW_TYPES_MASK, bridge );
    if( type == NET::Dialog )
        ;
    return new Decoration( bridge, this );
    }

bool Factory::reset( unsigned long changed )
    {
    resetDecorations( changed );
    return false;
    }
        
} // namespace

extern "C"
{

KDE_EXPORT KDecorationFactory *create_factory()
    {
    return new KWinTest::Factory();
    }

}

#include "test.moc"
