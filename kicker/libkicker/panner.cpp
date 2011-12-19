/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <tqlayout.h>
#include <tqtooltip.h>
#include <tqtimer.h>
#include <tqpainter.h>
#include <tqstyle.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

#include "simplebutton.h"
#include "panner.h"
#include "panner.moc"

Panner::Panner( TQWidget* parent, const char* name )
    : TQWidget( parent, name ),
      _luSB(0),
      _rdSB(0),
      _cwidth(0), _cheight(0),
      _cx(0), _cy(0)
{
    KGlobal::locale()->insertCatalogue("libkicker");
    setBackgroundOrigin( AncestorOrigin );

    _updateScrollButtonsTimer = new TQTimer(this);
    connect(_updateScrollButtonsTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(reallyUpdateScrollButtons()));

    _clipper = new TQWidget(this);
    _clipper->setBackgroundOrigin(AncestorOrigin);
    _clipper->installEventFilter( this );
    _viewport = new TQWidget(_clipper);
    _viewport->setBackgroundOrigin(AncestorOrigin);
    
    // layout
    _layout = new TQBoxLayout(this, TQBoxLayout::LeftToRight);
    _layout->addWidget(_clipper, 1);
    setOrientation(Qt::Horizontal);
}

Panner::~Panner() 
{
}

void Panner::createScrollButtons()
{
    if (_luSB)
    {
        return;
    }

    // left/up scroll button
    _luSB = new SimpleArrowButton(this);
    _luSB->installEventFilter(this);
    //_luSB->setAutoRepeat(true);
    _luSB->setMinimumSize(12, 12);
    _luSB->hide();
    _layout->addWidget(_luSB);
    connect(_luSB, TQT_SIGNAL(pressed()), TQT_SLOT(startScrollLeftUp()));
    connect(_luSB, TQT_SIGNAL(released()), TQT_SLOT(stopScroll()));

    // right/down scroll button
    _rdSB = new SimpleArrowButton(this);
    _rdSB->installEventFilter(this);
    //_rdSB->setAutoRepeat(true);
    _rdSB->setMinimumSize(12, 12);
    _rdSB->hide();
    _layout->addWidget(_rdSB);
    connect(_rdSB, TQT_SIGNAL(pressed()), TQT_SLOT(startScrollRightDown()));
    connect(_rdSB, TQT_SIGNAL(released()), TQT_SLOT(stopScroll()));

    // set up the buttons
    setupButtons();
}

void Panner::setupButtons()
{
    if (orientation() == Qt::Horizontal)
    {
        if (_luSB)
        {
            _luSB->setArrowType(Qt::LeftArrow);
            _rdSB->setArrowType(Qt::RightArrow);
            _luSB->setSizePolicy(TQSizePolicy(TQSizePolicy::Minimum, TQSizePolicy::Expanding));
            _rdSB->setSizePolicy(TQSizePolicy(TQSizePolicy::Minimum, TQSizePolicy::Expanding));
            TQToolTip::add(_luSB, i18n("Scroll left"));
            TQToolTip::add(_rdSB, i18n("Scroll right"));
            setMinimumSize(24, 0);
        }
        _layout->setDirection(TQBoxLayout::LeftToRight);
    }
    else
    {
        if (_luSB)
        {
            _luSB->setArrowType(Qt::UpArrow);
            _rdSB->setArrowType(Qt::DownArrow);
            _luSB->setSizePolicy(TQSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Minimum));
            _rdSB->setSizePolicy(TQSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Minimum));
            TQToolTip::add(_luSB, i18n("Scroll up"));
            TQToolTip::add(_rdSB, i18n("Scroll down"));
            setMinimumSize(0, 24);
        }
        _layout->setDirection(TQBoxLayout::TopToBottom);
    }

    if (isVisible())
    {
        // we need to manually redo the layout if we are visible
        // otherwise let the toolkit decide when to do this
        _layout->activate();
    }
}

void Panner::setOrientation(Orientation o)
{
    _orient = o;
    setupButtons();
    reallyUpdateScrollButtons();
}

void Panner::resizeEvent( TQResizeEvent* )
{
    //TQScrollView::resizeEvent( e );
    //updateScrollButtons();
}

void Panner::scrollRightDown()
{
    if(orientation() == Qt::Horizontal) // scroll right
        scrollBy( _step, 0 );
    else // scroll down
        scrollBy( 0, _step );
    if (_step < 64)
    _step++;
}

void Panner::scrollLeftUp()
{
    if(orientation() == Qt::Horizontal) // scroll left
        scrollBy( -_step, 0 );
    else // scroll up
        scrollBy( 0, -_step );
    if (_step < 64)
        _step++;
}

void Panner::startScrollRightDown()
{
    _scrollTimer = new TQTimer(this);
    connect(_scrollTimer, TQT_SIGNAL(timeout()), TQT_SLOT(scrollRightDown()));
    _scrollTimer->start(50);
    _step = 8;
    scrollRightDown();
}

void Panner::startScrollLeftUp()
{
    _scrollTimer = new TQTimer(this);
    connect(_scrollTimer, TQT_SIGNAL(timeout()), TQT_SLOT(scrollLeftUp()));
    _scrollTimer->start(50);
    _step = 8;
    scrollLeftUp();
}

void Panner::stopScroll()
{
    delete _scrollTimer;
    _scrollTimer = 0;
}

void Panner::reallyUpdateScrollButtons()
{
    int delta = 0;
    
    _updateScrollButtonsTimer->stop();

    if (orientation() == Qt::Horizontal)
    {
        delta = contentsWidth() - width();
    }
    else
    {
        delta = contentsHeight() - height();
    }

    if (delta >= 1)
    {
        createScrollButtons();

        // since the buttons may be visible but of the wrong size
        // we need to do this every single time
        _luSB->show();
        _rdSB->show();
    }
    else if (_luSB && _luSB->isVisibleTo(this))
    {
        _luSB->hide();
        _rdSB->hide();
    }
}

void Panner::updateScrollButtons()
{
    _updateScrollButtonsTimer->start(200, true);
}

void Panner::setContentsPos(int x, int y)
{
    if (x < 0)
        x = 0;
    else if (x > (contentsWidth() - visibleWidth()))
        x = contentsWidth() - visibleWidth();
    
    if (y < 0)
        y = 0;
    else if (y > (contentsHeight() - visibleHeight()))
        y = contentsHeight() - visibleHeight();
        
    if (x == contentsX() && y == contentsY())
        return;
        
    _viewport->move(-x, -y);
    emit contentsMoving(x, y);
}

void Panner::scrollBy(int dx, int dy)
{
    setContentsPos(contentsX() + dx, contentsY() + dy);
}

void Panner::resizeContents( int w, int h )
{
    _viewport->resize(w, h);
    setContentsPos(contentsX(), contentsY());
    updateScrollButtons();
}

TQPoint Panner::contentsToViewport( const TQPoint& p ) const
{
    return TQPoint(p.x() - contentsX() - _clipper->x(), p.y() - contentsY() - _clipper->y());
}

TQPoint Panner::viewportToContents( const TQPoint& vp ) const
{
    return TQPoint(vp.x() + contentsX() + _clipper->x(), vp.y() + contentsY() + _clipper->y());
}

void Panner::contentsToViewport( int x, int y, int& vx, int& vy ) const
{
    const TQPoint v = contentsToViewport(TQPoint(x,y));
    vx = v.x();
    vy = v.y();
}

void Panner::viewportToContents( int vx, int vy, int& x, int& y ) const
{
    const TQPoint c = viewportToContents(TQPoint(vx,vy));
    x = c.x();
    y = c.y();
}

void Panner::ensureVisible( int x, int y )
{
    ensureVisible(x, y, 50, 50);
}

void Panner::ensureVisible( int x, int y, int xmargin, int ymargin )
{
    int pw=visibleWidth();
    int ph=visibleHeight();

    int cx=-contentsX();
    int cy=-contentsY();
    int cw=contentsWidth();
    int ch=contentsHeight();

    if ( pw < xmargin*2 )
        xmargin=pw/2;
    if ( ph < ymargin*2 )
        ymargin=ph/2;

    if ( cw <= pw ) {
        xmargin=0;
        cx=0;
    }
    if ( ch <= ph ) {
        ymargin=0;
        cy=0;
    }

    if ( x < -cx+xmargin )
        cx = -x+xmargin;
    else if ( x >= -cx+pw-xmargin )
        cx = -x+pw-xmargin;

    if ( y < -cy+ymargin )
        cy = -y+ymargin;
    else if ( y >= -cy+ph-ymargin )
        cy = -y+ph-ymargin;

    if ( cx > 0 )
        cx=0;
    else if ( cx < pw-cw && cw>pw )
        cx=pw-cw;

    if ( cy > 0 )
        cy=0;
    else if ( cy < ph-ch && ch>ph )
        cy=ph-ch;

    setContentsPos( -cx, -cy );
}

bool Panner::eventFilter( TQObject *obj, TQEvent *e )
{
    if ( TQT_BASE_OBJECT(obj) == TQT_BASE_OBJECT(_viewport) || TQT_BASE_OBJECT(obj) == TQT_BASE_OBJECT(_clipper) ) 
    {
        switch ( e->type() ) 
        {
            case TQEvent::Resize:
                viewportResizeEvent((TQResizeEvent *)e);
                break;
            case TQEvent::MouseButtonPress:
                viewportMousePressEvent( (TQMouseEvent*)e );
                if ( ((TQMouseEvent*)e)->isAccepted() )
                    return true;
                break;
            case TQEvent::MouseButtonRelease:
                viewportMouseReleaseEvent( (TQMouseEvent*)e );
                if ( ((TQMouseEvent*)e)->isAccepted() )
                    return true;
                break;
            case TQEvent::MouseButtonDblClick:
                viewportMouseDoubleClickEvent( (TQMouseEvent*)e );
                if ( ((TQMouseEvent*)e)->isAccepted() )
                    return true;
                break;
            case TQEvent::MouseMove:
                viewportMouseMoveEvent( (TQMouseEvent*)e );
                if ( ((TQMouseEvent*)e)->isAccepted() )
                    return true;
                break;
            default:
                break;
        }
    }
    
    return TQWidget::eventFilter( obj, e );  // always continue with standard event processing
}

void Panner::viewportResizeEvent( TQResizeEvent* )
{
}

void Panner::viewportMousePressEvent( TQMouseEvent* e)
{
    e->ignore();
}

void Panner::viewportMouseReleaseEvent( TQMouseEvent* e )
{
    e->ignore();
}

void Panner::viewportMouseDoubleClickEvent( TQMouseEvent* e )
{
    e->ignore();
}

void Panner::viewportMouseMoveEvent( TQMouseEvent* e )
{
    e->ignore();
}
