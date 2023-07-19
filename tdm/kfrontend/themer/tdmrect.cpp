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

#include "tdmrect.h"
#include "tdmthemer.h"
#include "tdmconfig.h"

#include <kimageeffect.h>
#include <kdebug.h>

#include <tqimage.h>
#include <tqpainter.h>
#include <tqwidget.h>
#include <tqlayout.h>

extern bool argb_visual_available;

KdmRect::KdmRect( KdmItem *parent, const TQDomNode &node, const char *name )
    : KdmItem( parent, node, name )
{
  init( node, name );
}

KdmRect::KdmRect( TQWidget *parent, const TQDomNode &node, const char *name )
    : KdmItem( parent, node, name )
{
  init( node, name );
}

void
KdmRect::init( const TQDomNode &node, const char * )
{
	itemType = "rect";

	// Set default values for rect (note: strings are already Null)
	rect.normal.alpha = 1;
	rect.active.present = false;
	rect.prelight.present = false;
	rect.hasBorder = false;

	// A rect can have no properties (defaults to parent ones)
	if (node.isNull()) {
		return;
	}

	// Read RECT ID
	TQDomNode n = node;
	TQDomElement elRect = n.toElement();

	// Read RECT TAGS
	TQDomNodeList childList = node.childNodes();
	for (uint nod = 0; nod < childList.count(); nod++) {
		TQDomNode child = childList.item( nod );
		TQDomElement el = child.toElement();
		TQString tagName = el.tagName();

		if (tagName == "normal") {
			parseColor( el.attribute( "color", TQString::null ), rect.normal.color );
			rect.normal.alpha = el.attribute( "alpha", "1.0" ).toFloat();
			parseFont( el.attribute( "font", "Sans 14" ), rect.normal.font );
		}
		else if (tagName == "active") {
			rect.active.present = true;
			parseColor( el.attribute( "color", TQString::null ), rect.active.color );
			rect.active.alpha = el.attribute( "alpha", "1.0" ).toFloat();
			parseFont( el.attribute( "font", "Sans 14" ), rect.active.font );
		}
		else if (tagName == "prelight") {
			rect.prelight.present = true;
			parseColor( el.attribute( "color", TQString::null ), rect.prelight.color );
			rect.prelight.alpha = el.attribute( "alpha", "1.0" ).toFloat();
			parseFont( el.attribute( "font", "Sans 14" ), rect.prelight.font );
		}
		else if (tagName == "border") {
			rect.hasBorder = true;
		}
	}
}

void
KdmRect::drawContents( TQPainter *p, const TQRect &r )
{
	// choose the correct rect class
	RectStruct::RectClass *rClass = &rect.normal;
	if (state == Sactive && rect.active.present) {
		rClass = &rect.active;
	}
	if (state == Sprelight && rect.prelight.present) {
		rClass = &rect.prelight;
	}

	if (rClass->alpha <= 0 || !rClass->color.isValid()) {
		return;
	}

	if (rClass->alpha == 1) {
		p->fillRect( area, TQBrush( rClass->color ) );
	}
	else {
// 		if (!argb_visual_available) {
			// Software blend only (no compositing support)
			TQRect backRect = r;
			backRect.moveBy( area.x(), area.y() );
			TQPixmap backPixmap( backRect.size() );
			bitBlt( &backPixmap, TQPoint( 0, 0 ), p->device(), backRect );
			TQImage backImage = backPixmap.convertToImage();
			KImageEffect::blend( rClass->color, backImage, rClass->alpha );
			p->drawImage( backRect.x(), backRect.y(), backImage );
			//  area.moveBy(1,1);
// 		}
// 		else {
// 			// We have compositing support!
// 		}
	}
}

void
KdmRect::statusChanged()
{
	KdmItem::statusChanged();
	if (!rect.active.present && !rect.prelight.present) {
		return;
	}
	if ((state == Sprelight && !rect.prelight.present) ||
	    (state == Sactive && !rect.active.present)) {
		return;
	}
	needUpdate();
}

/*
void
KdmRect::setAttribs( TQWidget *widget )
{
	widget->setFont( rect.normal.font );
}

void
KdmRect::recursiveSetAttribs( TQLayoutItem *li )
{
    TQWidget *w;
    TQLayout *l;

    if ((w = li->widget()))
	setAttribs( w );
    else if ((l = li->layout())) {
	TQLayoutIterator it = l->iterator();
	for (TQLayoutItem *itm = it.current(); itm; itm = ++it)
	     recursiveSetAttribs( itm );
    }
}

void
KdmRect::setLayoutItem( TQLayoutItem *item )
{
	KdmItem::setLayoutItem( item );
	recursiveSetAttribs( item );
}
*/

void
KdmRect::setWidget( TQWidget *widget )
{
        if ( rect.normal.color.isValid() && widget ) 
        {
     	     TQPalette p = widget->palette();
	     p.setColor( TQPalette::Active, TQColorGroup::Text, rect.normal.color );
	     widget->setPalette(p);
	}
	KdmItem::setWidget( widget );
	//setAttribs( widget );
}

#include "tdmrect.moc"
