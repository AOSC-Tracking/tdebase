/*
 *  Copyright (C) 2003 by Unai Garro <ugarro@users.sourceforge.net>
 *  Copyright (C) 2004 by Enrico Ros <rosenric@dei.unipd.it>
 *  Copyright (C) 2004 by Stephan Kulow <coolo@kde.org>
 *  Copyright (C) 2004 by Oswald Buddenhagen <ossi@kde.org>
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

/*
 * Generic Kdm Item
 */

// #define DRAW_OUTLINE 1	// for debugging only

#include "kdmitem.h"
#include "kdmlayout.h"
#include "kdmconfig.h"

#include <kglobal.h>
#include <kdebug.h>

#include <tqframe.h>
#include <tqwidget.h>
#include <tqlayout.h>
#include <tqimage.h>
#include <tqpainter.h>

extern bool argb_visual_available;

KdmItem::KdmItem( KdmItem *parent, const TQDomNode &node, const char *name )
    : TQObject( parent, name )
    , boxManager( 0 )
    , fixedManager( 0 )
    , image( 0 )
    , myWidget( 0 )
    , myLayoutItem( 0 )
    , buttonParent( 0 )
{
  init(node, name);
}


KdmItem::KdmItem( TQWidget *parent, const TQDomNode &node, const char *name )
    : TQObject( parent, name )
    , boxManager( 0 )
    , fixedManager( 0 )
    , image( 0 )
    , myWidget( 0 )
    , myLayoutItem( 0 )
    , buttonParent( 0 )
{
  init(node, name);
}

void
KdmItem::init( const TQDomNode &node, const char * )
{
	// Set default layout for every item
	currentManager = MNone;
	pos.x = pos.y = 0;
	pos.width = pos.height = 1;
	pos.xType = pos.yType = pos.wType = pos.hType = DTnone;
	pos.anchor = "nw";
	m_backgroundModifier = 255;

	isShown = InitialHidden;

	// Set defaults for derived item's properties
	properties.incrementalPaint = false;
	state = Snormal;

	// The "toplevel" node (the screen) is really just like a fixed node
	if (!parent() || !parent()->inherits( "KdmItem" )) {
		setFixedLayout();
		return;
	}
	// Read the mandatory Pos tag. Other tags such as normal, prelighted,
	// etc.. are read under specific implementations.
	TQDomNodeList childList = node.childNodes();
	for (uint nod = 0; nod < childList.count(); nod++) {
		TQDomNode child = childList.item( nod );
		TQDomElement el = child.toElement();
		TQString tagName = el.tagName(), attr;

		if (tagName == "pos") {
			parseAttribute( el.attribute( "x", TQString::null ), pos.x, pos.xType );
			parseAttribute( el.attribute( "y", TQString::null ), pos.y, pos.yType );
			parseAttribute( el.attribute( "width", TQString::null ), pos.width, pos.wType );
			parseAttribute( el.attribute( "height", TQString::null ), pos.height, pos.hType );
			pos.anchor = el.attribute( "anchor", "nw" );
		}

		if (tagName == "bgmodifier") {
			m_backgroundModifier = TQString(el.attribute( "value", "255" )).toInt();
		}
	}

	TQDomNode tnode = node;
	id = tnode.toElement().attribute( "id", TQString::number( (ulong)this, 16 ) );

	// Tell 'parent' to add 'me' to its children
	KdmItem *parentItem = static_cast<KdmItem *>( parent() );
	parentItem->addChildItem( this );
}

KdmItem::~KdmItem()
{
	delete boxManager;
	delete fixedManager;
	delete image;
}

void
KdmItem::update()
{
}

void
KdmItem::needUpdate()
{
	emit needUpdate( area.x(), area.y(), area.width(), area.height() );
}

void
KdmItem::show( bool force )
{
	if (isShown != InitialHidden && !force)
		return;

	TQValueList<KdmItem *>::Iterator it;
	for (it = m_children.begin(); it != m_children.end(); ++it)
		(*it)->show();

	isShown = Shown;

	if (myWidget)
		myWidget->show();
	// XXX showing of layouts not implemented, prolly pointless anyway

	needUpdate();
}

void
KdmItem::hide( bool force )
{
	if (isShown == ExplicitlyHidden)
		return;

	if (isShown == InitialHidden && force) {
		isShown = ExplicitlyHidden;
		return;		// no need for further action
	}

	TQValueList<KdmItem *>::Iterator it;
	for (it = m_children.begin(); it != m_children.end(); ++it)
		(*it)->hide();

	isShown = force ? ExplicitlyHidden : InitialHidden;

	if (myWidget)
		myWidget->hide();
	// XXX hiding of layouts not implemented, prolly pointless anyway

	needUpdate();
}

void
KdmItem::inheritFromButton( KdmItem *button )
{
	if (button)
		buttonParent = button;

	TQValueList<KdmItem *>::Iterator it;
	for (it = m_children.begin(); it != m_children.end(); ++it)
		(*it)->inheritFromButton( button );
}

KdmItem *
KdmItem::findNode( const TQString &_id ) const
{
	if (id == _id)
		return const_cast<KdmItem *>( this );

	TQValueList<KdmItem *>::ConstIterator it;
	for (it = m_children.begin(); it != m_children.end(); ++it) {
		KdmItem *t = (*it)->findNode( _id );
		if (t)
			return t;
	}

	return 0;
}

void
KdmItem::setWidget( TQWidget *widget )
{
//	delete myWidget;	-- themer->widget() owns the widgets

	myWidget = widget;
	if (isHidden())
		myWidget->hide();
	else
		myWidget->show();

	// Remove borders so that it blends nicely with the theme background
	TQFrame* frame = ::tqqt_cast<TQFrame *>( widget );
	if (frame)
		frame->setFrameStyle( TQFrame::NoFrame );

	setGeometry(area, true);

	connect( myWidget, TQT_SIGNAL(destroyed()), TQT_SLOT(widgetGone()) );
}

void
KdmItem::widgetGone()
{
	myWidget = 0;
}

void
KdmItem::setLayoutItem( TQLayoutItem *item )
{
	myLayoutItem = item;
	// XXX hiding not supported - it think it's pointless here
	if (myLayoutItem->widget())
		connect( myLayoutItem->widget(), TQT_SIGNAL(destroyed()),
		         TQT_SLOT(layoutItemGone()) );
	else if (myLayoutItem->layout())
		connect( myLayoutItem->layout(), TQT_SIGNAL(destroyed()),
		         TQT_SLOT(layoutItemGone()) );
}

void
KdmItem::layoutItemGone()
{
	myLayoutItem = 0;
}

/* This is called as a result of KdmLayout::update, and directly on the root */
void
KdmItem::setGeometry( const TQRect &newGeometry, bool force )
{
	kdDebug() << " KdmItem::setGeometry " << id << newGeometry << endl;
	// check if already 'in place'
	if (!force && area == newGeometry)
		return;

	area = newGeometry;

	if (myWidget) {
            TQRect widGeo = newGeometry;
            if ( widGeo.height() > myWidget->maximumHeight() ) {
                widGeo.moveTop( widGeo.top() + ( widGeo.height() -  myWidget->maximumHeight() ) / 2 );
                widGeo.setHeight( myWidget->maximumHeight() );
            }
            myWidget->setGeometry( widGeo );
        }
	if (myLayoutItem)
		myLayoutItem->setGeometry( newGeometry );

	// recurr to all boxed children
	if (boxManager && !boxManager->isEmpty())
		boxManager->update( newGeometry, force );
 
	// recurr to all fixed children
	if (fixedManager && !fixedManager->isEmpty())
		fixedManager->update( newGeometry, force );

	// TODO send *selective* tqrepaint signal
}

void
KdmItem::paint( TQPainter *p, const TQRect &rect )
{
	if (isHidden())
		return;

	if (myWidget || (myLayoutItem && myLayoutItem->widget())) {
            // KListView because it's missing a Q_OBJECT'
            // FIXME: This is a nice idea in theory, but in practice it is
            // very confusing for the user not to see the empty list box
            // delineated from the rest of the greeter.
            // Maybe set a darker version of the background instead of an exact copy?
            if ( myWidget && myWidget->isA( "KListView" ) ) {
              if ((_compositor.isEmpty()) || (!argb_visual_available)) {
                // Software blend only (no compositing support)
                TQPixmap copy( myWidget->size() );
                kdDebug() <<  myWidget->tqgeometry() << " " << area << " " << myWidget->size() << endl;
                bitBlt( &copy, TQPoint( 0, 0), p->device(), myWidget->geometry(), TQt::CopyROP );
                // Lighten it slightly
                TQImage lightVersion;
                lightVersion = copy.convertToImage();

                register uchar * r = lightVersion.bits();
                register uchar * g = lightVersion.bits() + 1;
                register uchar * b = lightVersion.bits() + 2;
                uchar * end = lightVersion.bits() + lightVersion.numBytes();

                while ( r != end ) {
                    if ((*r) < (255-m_backgroundModifier))
                        *r = (uchar) (*r) + m_backgroundModifier;
                    else
                        *r = 255;
                    if ((*g) < (255-m_backgroundModifier))
                        *g = (uchar) (*g) + m_backgroundModifier;
                    else
                        *g = 255;
                    if ((*b) < (255-m_backgroundModifier))
                        *b = (uchar) (*b) + m_backgroundModifier;
                    else
                        *b = 255;
                    r += 4;
                    g += 4;
                    b += 4;
                }

                copy = lightVersion;
                // Set it
                myWidget->setPaletteBackgroundPixmap( copy );
              }
              else {
		// We have compositing support!
		TQRgb blend_color = tqRgba(m_backgroundModifier, m_backgroundModifier, m_backgroundModifier, 0);   // RGBA overlay
		float alpha = tqAlpha(blend_color) / 255.;
		int pixel = tqAlpha(blend_color) << 24 |
				int(tqRed(blend_color) * alpha) << 16 |
				int(tqGreen(blend_color) * alpha) << 8  |
				int(tqBlue(blend_color) * alpha);

		TQImage img( myWidget->size(), 32 );
		img = img.convertDepth(32);
		img.setAlphaBuffer(true);
		register uchar * rd = img.bits();
		for( int y = 0; y < img.height(); ++y )
		{
			for( short int x = 0; x < img.width(); ++x )
			{
				*reinterpret_cast<TQRgb*>(rd) = blend_color;
				rd += 4;
			}
		}
		myWidget->setPaletteBackgroundPixmap( img );
              }
            }
            return;
        }

	if (area.intersects( rect )) {
		TQRect contentsRect = area.intersect( rect );
		contentsRect.moveBy( QMIN( 0, -area.x() ), QMIN( 0, -area.y() ) );
		drawContents( p, contentsRect );
	}

#ifdef DRAW_OUTLINE
	// Draw bounding rect for this item
	p->setPen( Qt::white );
	p->drawRect( area );
#endif

	if (myLayoutItem)
		return;

	// Dispatch paint events to children
	TQValueList<KdmItem *>::Iterator it;
	for (it = m_children.begin(); it != m_children.end(); ++it)
		(*it)->paint( p, rect );


}

KdmItem *KdmItem::currentActive = 0;

void
KdmItem::mouseEvent( int x, int y, bool pressed, bool released )
{
	if (isShown == ExplicitlyHidden)
		return;

	if (buttonParent && buttonParent != this) {
	        buttonParent->mouseEvent( x, y, pressed, released );
		return;
	}

	ItemState oldState = state;
	if (area.contains( x, y )) {
		if (released && oldState == Sactive) {
			if (buttonParent)
				emit activated( id );
			state = Sprelight;
			currentActive = 0;
		} else if (pressed || currentActive == this) {
			state = Sactive;
			currentActive = this;
		} else if (!currentActive)
			state = Sprelight;
		else
			state = Snormal;
	} else {
		if (released)
			currentActive = 0;
		if (currentActive == this)
			state = Sprelight;
		else
			state = Snormal;
	}

	if (!buttonParent) {
		TQValueList<KdmItem *>::Iterator it;
		for (it = m_children.begin(); it != m_children.end(); ++it)
			(*it)->mouseEvent( x, y, pressed, released );
	}

	if (oldState != state)
		statusChanged();
}

void
KdmItem::statusChanged()
{
	if (buttonParent == this) {
		TQValueList<KdmItem *>::Iterator it;
		for (it = m_children.begin(); it != m_children.end(); ++it) {
			(*it)->state = state;
			(*it)->statusChanged();
		}
	}
}

// BEGIN protected inheritable

TQSize
KdmItem::sizeHint()
{
	if (myWidget)
		return myWidget->size();
	if (myLayoutItem)
		return myLayoutItem->sizeHint();
	int w = pos.wType == DTpixel ? kAbs( pos.width ) : -1,
	    h = pos.hType == DTpixel ? kAbs( pos.height ) : -1;
	return TQSize( w, h );
}

TQRect
KdmItem::placementHint( const TQRect &parentRect )
{
	TQSize hintedSize = sizeHint();
	TQSize boxHint;

	int x = parentRect.left(),
	    y = parentRect.top(),
	    w = parentRect.width(),
	    h = parentRect.height();

	kdDebug() << timestamp() << " KdmItem::placementHint parentRect=" << parentRect << " hintedSize=" << hintedSize << endl;

	// check if width or height are set to "box"
	if (pos.wType == DTbox || pos.hType == DTbox) {
		if (myLayoutItem || myWidget)
			boxHint = hintedSize;
		else {
			if (!boxManager)
				return parentRect;
			boxHint = boxManager->sizeHint();
		}
		kdDebug() << timestamp() << " boxHint " << boxHint << endl;
	}

	if (pos.xType == DTpixel)
		x += pos.x;
	else if (pos.xType == DTnpixel)
		x = parentRect.right() - pos.x;
	else if (pos.xType == DTpercent)
		x += tqRound( parentRect.width() / 100.0 * pos.x );

	if (pos.yType == DTpixel)
		y += pos.y;
	else if (pos.yType == DTnpixel)
		y = parentRect.bottom() - pos.y;
	else if (pos.yType == DTpercent)
		y += tqRound( parentRect.height() / 100.0 * pos.y );

	if (pos.wType == DTpixel)
		w = pos.width;
	else if (pos.wType == DTnpixel)
		w -= pos.width;
	else if (pos.wType == DTpercent)
		w = tqRound( parentRect.width() / 100.0 * pos.width );
	else if (pos.wType == DTbox)
		w = boxHint.width();
	else if (hintedSize.width() > 0)
	        w = hintedSize.width();
	else
		w = 0;

	if (pos.hType == DTpixel)
		h = pos.height;
	else if (pos.hType == DTnpixel)
		h -= pos.height;
	else if (pos.hType == DTpercent)
		h = tqRound( parentRect.height() / 100.0 * pos.height );
	else if (pos.hType == DTbox)
		h = boxHint.height();
	else if (hintedSize.height() > 0) {
	        if (w && pos.wType != DTnone)
	               h = (hintedSize.height() * w) / hintedSize.width();
	        else
	               h = hintedSize.height();
	} else
		h = 0;

	// we choose to take the hinted size, but it's better to listen to the aspect ratio
	if (pos.wType == DTnone && pos.hType != DTnone && h && w) {
	        w = tqRound(float(hintedSize.width() * h) / hintedSize.height());
	}

	// defaults to center
	int dx = -w / 2, dy = -h / 2;

	// anchor the rect to an edge / corner
	if (pos.anchor.length() > 0 && pos.anchor.length() < 3) {
		if (pos.anchor.find( 'n' ) >= 0)
			dy = 0;
		if (pos.anchor.find( 's' ) >= 0)
			dy = -h;
		if (pos.anchor.find( 'w' ) >= 0)
			dx = 0;
		if (pos.anchor.find( 'e' ) >= 0)
			dx = -w;
	}
	// KdmItem *p = static_cast<KdmItem*>( parent() );
	kdDebug() << timestamp() << " placementHint " << this << " x=" << x << " dx=" << dx << " w=" << w << " y=" << y << " dy=" << dy << " h=" << h << " " << parentRect << endl;
	y += dy;
	x += dx;

	// Note: no clipping to parent because this broke many themes!
	return TQRect( x, y, w, h );
}

// END protected inheritable


void
KdmItem::addChildItem( KdmItem *item )
{
	m_children.append( item );
	switch (currentManager) {
	case MNone:		// fallback to the 'fixed' case
		setFixedLayout();
	case MFixed:
		fixedManager->addItem( item );
		break;
	case MBox:
		boxManager->addItem( item );
		break;
	}

	// signal bounce from child to parent
	connect( item, TQT_SIGNAL(needUpdate( int, int, int, int )), TQT_SIGNAL(needUpdate( int, int, int, int )) );
	connect( item, TQT_SIGNAL(activated( const TQString & )), TQT_SIGNAL(activated( const TQString & )) );
}

void
KdmItem::parseAttribute( const TQString &s, int &val, enum DataType &dType )
{
	if (s.isEmpty())
		return;

	int p;
	if (s == "box") {	// box value
		dType = DTbox;
		val = 0;
	} else if ((p = s.find( '%' )) >= 0) {	// percent value
		dType = DTpercent;
		TQString sCopy = s;
		sCopy.remove( p, 1 );
		sCopy.replace( ',', '.' );
		val = (int)sCopy.toDouble();
	} else {		// int value
		dType = DTpixel;
		TQString sCopy = s;
		if (sCopy.tqat( 0 ) == '-') {
			sCopy.remove( 0, 1 );
			dType = DTnpixel;
		}
		sCopy.replace( ',', '.' );
		val = (int)sCopy.toDouble();
	}
}

void
KdmItem::parseFont( const TQString &s, TQFont &font )
{
	int splitAt = s.findRev( ' ' );
	if (splitAt < 1)
		return;
	font.setFamily( s.left( splitAt ) );
	int fontSize = s.mid( splitAt + 1 ).toInt();
	if (fontSize > 1)
		font.setPointSize( fontSize );
}

void
KdmItem::parseColor( const TQString &s, TQColor &color )
{
	if (s.tqat( 0 ) != '#')
		return;
	bool ok;
	TQString sCopy = s;
	int hexColor = sCopy.remove( 0, 1 ).toInt( &ok, 16 );
	if (ok)
		color.setRgb( hexColor );
}

void
KdmItem::setBoxLayout( const TQDomNode &node )
{
	if (!boxManager)
		boxManager = new KdmLayoutBox( node );
	currentManager = MBox;
}

void
KdmItem::setFixedLayout( const TQDomNode &node )
{
	if (!fixedManager)
		fixedManager = new KdmLayoutFixed( node );
	currentManager = MFixed;
}

TQWidget *
KdmItem::parentWidget() const
{
  if (myWidget)
    return myWidget;
  if (!this->parent())
    return 0;

  if (parent()->qt_cast(TQWIDGET_OBJECT_NAME_STRING))
    return (TQWidget*)parent();
  return ((KdmItem*)parent())->parentWidget();
}

#include "kdmitem.moc"
