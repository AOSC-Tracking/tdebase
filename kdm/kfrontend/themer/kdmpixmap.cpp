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

#include <config.h>

#include "kdmpixmap.h"
#include <kdmconfig.h>

#include <kimageeffect.h>
#ifdef HAVE_LIBART
#include <ksvgiconengine.h>
#endif

#include <kdebug.h>
#include <kstandarddirs.h>

#include <tqfile.h>
#include <tqpainter.h>
#include <tqpixmap.h>
#include <tqimage.h>

extern bool argb_visual_available;

KdmPixmap::KdmPixmap( KdmItem *parent, const TQDomNode &node, const char *name )
    : KdmItem( parent, node, name )
{
	itemType = "pixmap";

	// Set default values for pixmap (note: strings are already Null)
	pixmap.normal.tint.setRgb( 0x800000 );
	pixmap.normal.alpha = 0.0;
	pixmap.active.present = false;
	pixmap.prelight.present = false;
	bool true_transparency = false;

	// Read PIXMAP ID
	// it rarely happens that a pixmap can be a button too!
	TQDomNode n = node;
	TQDomElement elPix = n.toElement();

	// Read PIXMAP TAGS
	TQDomNodeList childList = node.childNodes();
	for (uint nod = 0; nod < childList.count(); nod++) {
		TQDomNode child = childList.item( nod );
		TQDomElement el = child.toElement();
		TQString tagName = el.tagName();

		if (tagName == "normal") {
			pixmap.normal.fullpath = fullPath( el.attribute( "file", "" ) );
			parseColor( el.attribute( "tint", "#ffffff" ), pixmap.normal.tint );
			pixmap.normal.alpha = el.attribute( "alpha", "1.0" ).toFloat();

			if (el.attribute( "file", "" ) == "@@@KDMBACKGROUND@@@") {
				if ((_compositor.isEmpty()) || (!argb_visual_available)) {
					// Software blend only (no compositing support)
					// Use the preset KDM background...
					KStandardDirs *m_pDirs = KGlobal::dirs();
					KSimpleConfig *config = new KSimpleConfig( TQFile::decodeName( _backgroundCfg ) );
					config->setGroup("Desktop0");
					pixmap.normal.fullpath = m_pDirs->findResource("wallpaper", config->readPathEntry("Wallpaper"));
					// TODO: Detect when there is no wallpaper and use the background settings instead
					delete config;
				}
				else {
					true_transparency = true;
					pixmap.normal.alpha = 0.0;
				}
			}

		} else if (tagName == "active") {
			pixmap.active.present = true;
			pixmap.active.fullpath = fullPath( el.attribute( "file", "" ) );
			parseColor( el.attribute( "tint", "#ffffff" ), pixmap.active.tint );
			pixmap.active.alpha = el.attribute( "alpha", "1.0" ).toFloat();
		} else if (tagName == "prelight") {
			pixmap.prelight.present = true;
			pixmap.prelight.fullpath = fullPath(el.attribute( "file", "" ) );
			parseColor( el.attribute( "tint", "#ffffff" ), pixmap.prelight.tint );
			pixmap.prelight.alpha = el.attribute( "alpha", "1.0" ).toFloat();
		}
	}

	// look if we have to have the aspect ratio ready
	if (((pos.wType == DTnone && pos.hType != DTnone) ||
	     (pos.wType != DTnone && pos.hType == DTnone) ||
	     (pos.wType == DTnone && pos.hType == DTnone)) &&
	    !pixmap.normal.fullpath.endsWith( ".svg" ))
	  loadPixmap( &pixmap.normal );
}

TQSize
KdmPixmap::sizeHint()
{
	// choose the correct pixmap class
	PixmapStruct::PixmapClass * pClass = &pixmap.normal;
	if (state == Sactive && pixmap.active.present)
		pClass = &pixmap.active;
	if (state == Sprelight && pixmap.prelight.present)
		pClass = &pixmap.prelight;
	// use the pixmap size as the size hint
	if (!pClass->pixmap.isNull())
		return pClass->pixmap.size();
	return KdmItem::sizeHint();
}

void
KdmPixmap::setGeometry( const TQRect &newGeometry, bool force )
{
	KdmItem::setGeometry( newGeometry, force );
	pixmap.active.readyPixmap.resize( 0, 0 );
	pixmap.prelight.readyPixmap.resize( 0, 0 );
	pixmap.normal.readyPixmap.resize( 0, 0 );
}


TQString
KdmPixmap::fullPath( const TQString &fileName)
{
        if (fileName.isEmpty())
		return TQString::null;

	TQString fullName = fileName;
	if (fullName.at( 0 ) != '/')
		fullName = baseDir() + "/" + fileName;
	return fullName;
}

void
KdmPixmap::renderSvg( PixmapStruct::PixmapClass *pClass, const TQRect &area )
{
#ifdef HAVE_LIBART
	// Special stuff for SVG icons
	KSVGIconEngine *svgEngine = new KSVGIconEngine();

	if (svgEngine->load( area.width(), area.height(), pClass->fullpath )) {
		TQImage *t = svgEngine->image();
		pClass->pixmap = *t;
		pClass->readyPixmap.resize( 0, 0 );
		delete t;
	} else {
		kdWarning() << "failed to load " << pClass->fullpath << endl;
		pClass->fullpath = TQString::null;
	}

	delete svgEngine;
#else
        Q_UNUSED(pClass);
        Q_UNUSED(area);
#endif
}

void
KdmPixmap::loadPixmap( PixmapStruct::PixmapClass *pClass )
{
  TQString fullpath = pClass->fullpath;

  kdDebug() << timestamp() << " load " << fullpath << endl;
  int index = fullpath.findRev('.');
  TQString ext = fullpath.right(fullpath.length() - index);
  fullpath = fullpath.left(index);
  kdDebug() << timestamp() << " ext " << ext << " " << fullpath << endl;
  TQString testpath = TQString("-%1x%2").arg(area.width()).arg(area.height()) + ext;
  kdDebug() << timestamp() << " testing for " << fullpath + testpath << endl;
  if (KStandardDirs::exists(fullpath + testpath)) 
    pClass->pixmap.load(fullpath + testpath);
  else
    pClass->pixmap.load( fullpath + ext );
  kdDebug() << timestamp() << " done\n";
}

void
KdmPixmap::drawContents( TQPainter *p, const TQRect &r )
{
	// choose the correct pixmap class
	PixmapStruct::PixmapClass *pClass = &pixmap.normal;
	if (state == Sactive && pixmap.active.present)
		pClass = &pixmap.active;
	if (state == Sprelight && pixmap.prelight.present)
		pClass = &pixmap.prelight;

	kdDebug() << "draw " << id << " " << pClass->pixmap.isNull() << endl;
 
	if (pClass->pixmap.isNull()) {
	        if (pClass->fullpath.isEmpty())	// if neither is set, we're empty
			return;
		
		if (!pClass->fullpath.endsWith( ".svg" ) ) {
		  loadPixmap(pClass);
		} else {
		  kdDebug() << timestamp() << " renderSVG\n";
		  renderSvg( pClass, area );
		  kdDebug() << timestamp() << " done\n";
		}
	}

	int px = area.left() + r.left();
	int py = area.top() + r.top();
	int sx = r.x();
	int sy = r.y();
	int sw = r.width();
	int sh = r.height();
	if (px < 0) {
		px *= -1;
		sx += px;
		px = 0;
	}
	if (py < 0) {
		py *= -1;
		sy += py;
		py = 0;
	}


	if (pClass->readyPixmap.isNull()) {
	  
		bool haveTint = pClass->tint.rgb() != 0xFFFFFF;
		bool haveAlpha = pClass->alpha < 1.0;

		TQImage scaledImage;
		
		// use the loaded pixmap or a scaled version if needed

		kdDebug() << timestamp() << " prepare readyPixmap " << pClass->fullpath << " " << area.size() << " " << pClass->pixmap.size() << endl;
		if (area.size() != pClass->pixmap.size()) {
			if (pClass->fullpath.endsWith( ".svg" )) {
				kdDebug() << timestamp() << " renderSVG\n";
				renderSvg( pClass, area );
				scaledImage = pClass->pixmap.convertToImage();
			} else {
				kdDebug() << timestamp() << " convertFromImage smoothscale\n";
				if (pClass->pixmap.isNull()) {
					scaledImage = TQImage();
				}
				else {
					TQImage tempImage = pClass->pixmap.convertToImage();
					kdDebug() << timestamp() << " convertToImage done\n";
					scaledImage = tempImage.smoothScale( area.width(), area.height() );
				}
				kdDebug() << timestamp() << " done\n";
			}
		} else {
		  if (haveTint || haveAlpha)
                  {
			scaledImage = pClass->pixmap.convertToImage();
                        // enforce rgba values for the latter
                        if (!scaledImage.isNull()) scaledImage = scaledImage.convertDepth( 32 );
                  }
		  else
		    pClass->readyPixmap = pClass->pixmap;
		}

		if (haveTint || haveAlpha) {
			// blend image(pix) with the given tint

			if (!scaledImage.isNull()) scaledImage = scaledImage.convertDepth( 32 );
			int w = scaledImage.width();
			int h = scaledImage.height();
			float tint_red = float( pClass->tint.red() ) / 255;
			float tint_green = float( pClass->tint.green() ) / 255;
			float tint_blue = float( pClass->tint.blue() ) / 255;
			float tint_alpha = pClass->alpha;

			for (int y = 0; y < h; ++y) {
				QRgb *ls = (QRgb *)scaledImage.scanLine( y );
				for (int x = 0; x < w; ++x) {
					QRgb l = ls[x];
					int r = int( tqRed( l ) * tint_red );
					int g = int( tqGreen( l ) * tint_green );
					int b = int( tqBlue( l ) * tint_blue );
					int a = int( tqAlpha( l ) * tint_alpha );
					ls[x] = tqRgba( r, g, b, a );
				}
			}
		}
		if ((_compositor.isEmpty()) || (!argb_visual_available)) {
			// Software blend only (no compositing support)
		}
		else {
			// We have a compositor!
			// Apply the alpha in the same manner as above, exept we are now
			// using the hardware blending engine for all painting
			scaledImage = pClass->readyPixmap;
			if (!scaledImage.isNull()) scaledImage = scaledImage.convertDepth( 32 );
			int w = scaledImage.width();
			int h = scaledImage.height();

			for (int y = 0; y < h; ++y) {
				QRgb *ls = (QRgb *)scaledImage.scanLine( y );
				for (int x = 0; x < w; ++x) {
					QRgb l = ls[x];
					float alpha_adjust = (tqAlpha( l )/256.0);
					int r = int( tqRed( l ) * alpha_adjust );
					int g = int( tqGreen( l ) * alpha_adjust );
					int b = int( tqBlue( l ) * alpha_adjust );
					int a = int( tqAlpha( l ) * 1 );
					ls[x] = tqRgba( r, g, b, a );
				}
			}
		}

		if (!scaledImage.isNull()) {
		  kdDebug() << timestamp() << " convertFromImage " << id << " " << area << endl;
		  pClass->readyPixmap.convertFromImage( scaledImage );
		}
	}
	kdDebug() << timestamp() << " Pixmap::drawContents " << pClass->readyPixmap.size() << " " << px << " " << py << " " << sx << " " << sy << " " << sw << " " << sh << endl;
	p->drawPixmap( px, py, pClass->readyPixmap, sx, sy, sw, sh );
}

void
KdmPixmap::statusChanged()
{
	KdmItem::statusChanged();
	if (!pixmap.active.present && !pixmap.prelight.present)
		return;
	if ((state == Sprelight && !pixmap.prelight.present) ||
	    (state == Sactive && !pixmap.active.present))
		return;
	needUpdate();
}

#include "kdmpixmap.moc"
