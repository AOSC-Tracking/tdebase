/* kasbar.cpp
**
** Copyright (C) 2001-2004 Richard Moore <rich@kde.org>
** Contributor: Mosfet
**     All rights reserved.
**
** KasBar is dual-licensed: you can choose the GPL or the BSD license.
** Short forms of both licenses are included below.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/

/*
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

/*
** Bug reports and questions can be sent to kde-devel@kde.org
*/
#include <math.h>

#include <tqbitmap.h>
#include <tqcursor.h>
#include <tqpainter.h>
#include <tqmemarray.h>
#include <tqtimer.h>

#include <kapplication.h>
#include <kdebug.h>
#include <krootpixmap.h>
#include <kpixmapio.h>
#include <kiconloader.h>

#include "kasitem.h"

#include "kasbar.h"
#include "kasbar.moc"

static const int SMALL_EXTENT = 36;
static const int MEDIUM_EXTENT = 52;
static const int LARGE_EXTENT = 68;
static const int HUGE_EXTENT = 84;
static const int ENORMOUS_EXTENT = 148;

KasBar::KasBar( Orientation o, TQWidget *parent, const char *name, WFlags f )
   : TQWidget( parent, name, f ),
     master_(0),
     orient( o ),
     direction_( o == Qt::Horizontal ? TQBoxLayout::LeftToRight : TQBoxLayout::TopToBottom ),
     itemUnderMouse_( 0 ),
     boxesPerLine_(10), // Temp value
     inDrag( false ),
     detached( false ),
     maxBoxes_( 100 ), // Temp value
     itemSize_( Medium ),
     itemExtent_( MEDIUM_EXTENT ),
     paintInactiveFrame_( true ),
     transparent_( false ),
     rootPix( 0 ),
     enableTint_( false ),
     tintAmount_( 0.1 ), 
     tintColour_( colorGroup().mid() ),
     useMask_( true ),
     res(0)
{
    setBackgroundMode( NoBackground );
    items.setAutoDelete( true );
    setMouseTracking( true );
    setMaxBoxes( 0 );

    connect( this, TQT_SIGNAL( configChanged() ), TQT_SLOT( tqrepaint() ) );
}

KasBar::KasBar( Orientation o, KasBar *master, TQWidget *parent, const char *name, WFlags f )
   : TQWidget( parent, name, f ),
     master_(master),
     orient( o ),
     direction_( o == Qt::Horizontal ? TQBoxLayout::LeftToRight : TQBoxLayout::TopToBottom ),
     itemUnderMouse_( 0 ),
     boxesPerLine_(10), // Temp value
     inDrag( false ),
     detached( false ),
     maxBoxes_( 100 ), // Temp value
     itemSize_( Medium ),
     itemExtent_( MEDIUM_EXTENT ),
     paintInactiveFrame_( true ),
     transparent_( false ),
     rootPix( 0 ),
     enableTint_( false ),
     tintAmount_( 0.1 ), 
     tintColour_( colorGroup().mid() ),
     useMask_( true ),
     res(0)
{
    setBackgroundMode( NoBackground );
    items.setAutoDelete( true );
    setMouseTracking( true );
    setMaxBoxes( 0 );
    connect( master_, TQT_SIGNAL( configChanged() ), TQT_SLOT( tqrepaint() ) );
}

KasBar::~KasBar()
{
   delete res;
}

KasResources *KasBar::resources()
{
    if ( res )
	return res;

    if ( isTopLevel() ) {
	res = new KasResources( this );
	connect( res, TQT_SIGNAL( changed() ), TQT_SIGNAL( configChanged() ) );
	connect( this, TQT_SIGNAL( itemSizeChanged(int) ), res, TQT_SLOT( itemSizeChanged() ) );
	return res;
    }

    return master_->resources();
}

KasBar *KasBar::createChildBar( Orientation o, TQWidget *parent, const char *name )
{
    KasBar *child = new KasBar( o, this, parent, name );
    child->rereadMaster();
    return child;
}

void KasBar::setItemSize( int size )
{
   switch( size ) {
   case Small:
     setItemExtent( SMALL_EXTENT );
     break;
   case Medium:
     setItemExtent( MEDIUM_EXTENT );
     break;
   case Large:
     setItemExtent( LARGE_EXTENT );
     break;
   case Huge:
     setItemExtent( HUGE_EXTENT );
     break;
   case Enormous:
     setItemExtent( ENORMOUS_EXTENT );
     break;
   default:
       break;
   }
}

void KasBar::setItemExtent( int size )
{
    if ( size == itemExtent_ )
	return;

    itemExtent_ = size;

    if ( size < MEDIUM_EXTENT )
	itemSize_ = Small;
    else if ( size < LARGE_EXTENT )
	itemSize_ = Medium;
    else if ( size < HUGE_EXTENT )
	itemSize_ = Large;
    else if ( size < ENORMOUS_EXTENT )
	itemSize_ = Huge;
    else
	itemSize_ = Enormous;
    
   emit itemSizeChanged( itemSize_ );
   emit configChanged();

   updateLayout();
}

void KasBar::setTransparent( bool enable )
{
   if ( transparent_ == enable )
      return;

   transparent_ = enable;

   if ( transparent_ ) {
       kdDebug(1345) << "KasBar: Enabling transparency" << endl;

       rootPix = new KRootPixmap( this );
       connect( rootPix, TQT_SIGNAL( backgroundUpdated(const TQPixmap &) ),
		this, TQT_SLOT( setBackground(const TQPixmap &) ) );

       rootPix->setCustomPainting( true );

       if ( enableTint_ )
	   rootPix->setFadeEffect( tintAmount_, tintColour_ );

       rootPix->start();
   }
   else {
       kdDebug(1345) << "KasBar: Disabling transparency" << endl;

       rootPix->stop();
       delete rootPix;
       rootPix = 0;
   }

   emit configChanged();
}

void KasBar::setTint( bool enable )
{
   if ( enableTint_ == enable )
      return;

   enableTint_ = enable;

   if ( transparent_ && rootPix ) {
      if ( enableTint_ ) {
	 rootPix->setFadeEffect( tintAmount_, tintColour_ );
      }
      else {
	 rootPix->setFadeEffect( 0.0, tintColour_ );
      }

      emit configChanged();
      tqrepaint( true );
   }
}

void KasBar::setTint( double amount, TQColor color )
{
   tintAmount_ = amount;
   tintColour_ = color;

   if ( transparent_ && enableTint_ ) {
      rootPix->setFadeEffect( tintAmount_, tintColour_ );
      emit configChanged();

      if ( rootPix->isAvailable() )
	rootPix->tqrepaint( true );
   }
}

void KasBar::setTintColor( const TQColor &c )
{
   setTint( tintAmount_, c );
}

void KasBar::setTintAmount( int percent )
{
   double amt = (double) percent / 100.0;
   setTint( amt, tintColour_ );
}

void KasBar::setMaxBoxes( int count )
{
   if ( count == maxBoxes_ )
       return;

   if ( count == 0 )
       count = 15; // XXX Hacked

   maxBoxes_ = count;
   emit configChanged();
   setBoxesPerLine( count );
}

void KasBar::setBoxesPerLine( int count )
{
   boxesPerLine_ = QMIN( count, maxBoxes_ );
   updateLayout();
}

void KasBar::setDetachedPosition( const TQPoint &pos )
{
    if ( detachedPos == pos )
	return;

    detachedPos = pos;
    emit detachedPositionChanged( pos );
}

void KasBar::setDirection( Direction dir )
{
    if ( direction_ == dir )
	return;

    if ( ( dir == TQBoxLayout::LeftToRight ) || ( dir == TQBoxLayout::RightToLeft ) )
	orient = Qt::Horizontal;
    else
	orient = Qt::Vertical;

    direction_ = dir;
    emit directionChanged();
    updateLayout();
}

void KasBar::setOrientation( Orientation o )
{
    if ( orient == o )
	return;

    if ( o == Qt::Horizontal )
	setDirection( TQBoxLayout::LeftToRight );
    else
	setDirection( TQBoxLayout::TopToBottom );
}

void KasBar::toggleOrientation()
{
    switch( direction_ ) {
	case TQBoxLayout::LeftToRight:
	    setDirection( TQBoxLayout::RightToLeft );
	    break;
	case TQBoxLayout::RightToLeft:
	    setDirection( TQBoxLayout::TopToBottom );
	    break;
	case TQBoxLayout::TopToBottom:
	    setDirection( TQBoxLayout::BottomToTop );
	    break;
	case TQBoxLayout::BottomToTop:
	    setDirection( TQBoxLayout::LeftToRight );
	    break;
	default:
	    kdWarning() << "toggleOrientation got an odd direction: " << (uint) direction_ << endl;
	    setDirection( TQBoxLayout::LeftToRight );
	    break;
    }
}

void KasBar::toggleDetached()
{
    setDetached( !detached );
}

void KasBar::setDetached( bool detach )
{
    if ( detached == detach )
	return;

    detached = detach;
    updateLayout();
    emit detachedChanged( detached );
}

TQSize KasBar::sizeHint( Orientation o,  TQSize sz )
{
    if ( o == Qt::Horizontal )
	setBoxesPerLine( sz.width() / itemExtent() );
    else
	setBoxesPerLine( sz.height() / itemExtent() );

   unsigned int r=0, c=0;
   if( items.count() > (unsigned int) boxesPerLine_ ) {
      r = items.count()/boxesPerLine_;
      c = boxesPerLine_;
   }
   else {
      r = 1;
      c = items.count();
   }

   if( r*c < items.count() ) // remainders
      ++r;

   TQSize s;
   if( o == Qt::Horizontal ) {
      s.setWidth( c*itemExtent() );
      s.setHeight( r*itemExtent() );
   }
   else {
      s.setWidth( r*itemExtent() );
      s.setHeight( c*itemExtent() );
   }

   return s;
}

void KasBar::updateLayout()
{
//    kdDebug(1345) << "KasBar: updateLayout(), count is " << items.count() << endl;
    if ( !isUpdatesEnabled() )
	return;
    bool updates = isUpdatesEnabled();
    setUpdatesEnabled( false );

// This is for testing a rectangular layout
//    boxesPerLine_ = (uint) ceil(sqrt( items.count() ));

   // Work out the number of rows and columns
   unsigned int r=0, c=0;
   if( items.count() > (unsigned int) boxesPerLine_ ) {
      r = items.count()/boxesPerLine_;
      c = boxesPerLine_;
   }
   else{
      r = 1;
      c = items.count();
   }

   if( r*c < items.count() ) // remainders
      ++r;

   TQSize sz;
   if ( orient == Qt::Horizontal )
       sz = TQSize( c * itemExtent(), r * itemExtent() );
   else
       sz = TQSize( r * itemExtent(), c * itemExtent() );

   if ( sz != size() ) {
       resize( sz );
   }

   setUpdatesEnabled( updates );

   TQWidget *top = topLevelWidget();
   TQRegion mask;

   KasItem *i;
   if ( orient == Qt::Horizontal ) {
       for ( i = items.first(); i; i = items.next() ) {
	   int x = (items.tqat() % c) * itemExtent();

	   if ( direction_ == TQBoxLayout::RightToLeft )
	       x = width() - x - itemExtent();

	   i->setPos( x, (items.tqat() / c) * itemExtent() );
	   i->update();
	   mask = mask.unite( TQRegion( TQRect( i->pos(), TQSize(itemExtent(),itemExtent()) ) ) );
       }
   }
   else {
       for ( i = items.first(); i; i = items.next() ) {
	   int y = (items.tqat() / r) * itemExtent();

	   if ( direction_ == TQBoxLayout::BottomToTop )
	       y = height() - y - itemExtent();

	   i->setPos( (items.tqat() % r) * itemExtent(), y );
	   i->update();
	   mask = mask.unite( TQRegion( TQRect( i->pos(), TQSize(itemExtent(),itemExtent()) ) ) );
       }
   }

   if ( useMask_ )
       top->setMask( mask );
   else
       top->clearMask();
   update();
}

void KasBar::rereadMaster()
{
    if ( !master_ )
	return;

    setItemSize( master_->itemSize() );
    setTint( master_->hasTint() );
    setTintColor( master_->tintColor() );
    setTintAmount( master_->tintAmount() );
}

void KasBar::append( KasItem *i )
{
   if ( !i )
      return;

   items.append( i );
   updateLayout();
}

void KasBar::insert( int index, KasItem *i )
{
   if ( (!i) || (index < 0) )
      return;

   items.insert( index, i );
   updateLayout();
}

void KasBar::remove( KasItem *i )
{
   items.remove( i );

   if ( i == itemUnderMouse_ )
      itemUnderMouse_ = 0;
   updateLayout();
}

void KasBar::clear()
{
   items.clear();
   itemUnderMouse_ = 0;
   updateLayout();
}

void KasBar::mousePressEvent(TQMouseEvent *ev)
{
   KasItem *i = itemAt( ev->pos() );
   if ( i )
      i->mousePressEvent( ev );

   pressPos = ev->globalPos();
}

void KasBar::mouseReleaseEvent(TQMouseEvent *ev)
{
   if ( !inDrag ) {
       KasItem *i = itemAt( ev->pos() );
       if ( i )
	   i->mouseReleaseEvent( ev );
   }
   else if ( detached ) {
       setDetachedPosition( pos() );
       emit configChanged();
   }

   pressPos = TQPoint();
   inDrag = false;
}

void KasBar::updateMouseOver()
{
    updateMouseOver( mapFromGlobal( TQCursor::pos() ) );
}

void KasBar::updateMouseOver( TQPoint pos )
{
   KasItem *i = itemAt(pos);

   if ( i == itemUnderMouse_ )
       return;

   if ( itemUnderMouse_ )
       itemUnderMouse_->mouseLeave();
   if ( i )
       i->mouseEnter();
   if ( i && itemUnderMouse_ )
       itemUnderMouse_->hidePopup();

   itemUnderMouse_ = i;
}

void KasBar::mouseMoveEvent(TQMouseEvent *ev)
{
    if ( detached && (!pressPos.isNull()) ) {
	TQPoint moved = ev->globalPos() - pressPos;

	if ( !inDrag ) {
	    if ( moved.manhattanLength() > 6 ) {
		inDrag = true;
		emit dragStarted();
	    }
	}

	if ( inDrag ) {
	    if ( itemUnderMouse_ )
		itemUnderMouse_->hidePopup();

	    move( pos() + moved );
	    pressPos = ev->globalPos();
	}
    }
    else {
	updateMouseOver( ev->pos() );
    }
}

void KasBar::dragMoveEvent ( TQDragMoveEvent *ev )
{
   KasItem *i = itemAt( ev->pos() );
   if ( itemUnderMouse_ != i ) {
      if ( itemUnderMouse_ )
	 itemUnderMouse_->dragLeave();
      if ( i )
	 i->dragEnter();
      itemUnderMouse_ = i;
   }
}

void KasBar::paintEvent(TQPaintEvent *ev)
{
   TQPainter q( this );
   q.drawPixmap( ev->rect().topLeft(), offscreen, ev->rect() );
}

void KasBar::resizeEvent(TQResizeEvent *ev)
{
    offscreen.resize( ev->size() );
    TQPainter p( &offscreen );
    paintBackground( &p, TQRect(TQPoint(0,0),size()) );
    TQWidget::resizeEvent(ev);
    emit layoutChanged();
}


TQPoint KasBar::itemPos( KasItem *i )
{
    return i->pos();
}

void KasBar::updateItem( KasItem *i )
{
    if ( !i )
	return;
    if ( !isShown() )
	return;

    TQPainter p( &offscreen );
    TQPoint pos = i->pos();

    paintBackground( &p, TQRect( pos, TQSize( itemExtent(), itemExtent() ) ) );
    i->paint( &p, pos.x(), pos.y() );
    update( TQRect( pos, TQSize( itemExtent(), itemExtent() ) ) );
}

void KasBar::repaintItem(KasItem *i, bool erase )
{
    if ( !i )
	return;
    if ( !isShown() )
	return;

    TQPainter p( &offscreen );
    TQPoint pos = i->pos();

    paintBackground( &p, TQRect( pos, TQSize( itemExtent(), itemExtent() ) ) );
    i->paint( &p, pos.x(), pos.y() );
    tqrepaint( TQRect( pos, TQSize( itemExtent(), itemExtent()  ) ), transparent_ || erase );
}

KasItem* KasBar::itemAt(const TQPoint &p)
{
   KasItem *i;
   TQRect cr;

   for (i = items.first(); i; i = items.next()) {
       cr.setTopLeft( i->pos() );
       cr.setSize( TQSize( itemExtent(), itemExtent() ) );

       if(cr.contains(p))
	   return i;
   }

   return 0;
}

void KasBar::setBackground( const TQPixmap &newBg )
{
    bg = newBg;

    TQPainter p( &offscreen );
    paintBackground( &p, TQRect(TQPoint(0,0),size()) );

    updateLayout();
}

void KasBar::setMasked( bool mask )
{
    if ( useMask_ == mask )
	return;

    useMask_ = mask;
}

void KasBar::setPaintInactiveFrames( bool enable )
{
    paintInactiveFrame_ = enable;
    update();
}

void KasBar::paintBackground( TQPainter *p, const TQRect &r )
{
    // If we're transparent
    if ( transparent_ ) {
	if ( !bg.isNull() ) {
	    p->drawPixmap( r.topLeft(), bg, r );
	    return;
	}
    }
}

void KasBar::addTestItems()
{
   KasItem *i = new KasItem( this );
   insert( 0, i );
   i->setText( "Animated" );
   i->setIcon( KGlobal::iconLoader()->loadIcon( "icons", KIcon::NoGroup, KIcon::SizeMedium ) );
   i->setAnimation( resources()->startupAnimation() );
   TQTimer *aniTimer = new TQTimer( i, "aniTimer" );
   connect( aniTimer, TQT_SIGNAL( timeout() ), i, TQT_SLOT( advanceAnimation() ) );
   aniTimer->start( 100 );
   i->setShowAnimation( true );

   updateLayout();
}
