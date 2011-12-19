/*
 * Menu Transparency Preview Widget
 * Copyright (C) 2002 Karol Szwed <gallium@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "menupreview.h"

#include <tqpainter.h>
#include <tqimage.h>

#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <klocale.h>
#include <kimageeffect.h>
#include <kiconloader.h>


MenuPreview::MenuPreview( TQWidget* parent, int opacity, PreviewMode pvm )
	: TQWidget( parent, 0, (WFlags)(WStyle_Customize | WRepaintNoErase) ),
	pixBackground(NULL), pixOverlay(NULL), pixBlended(NULL)
{
	setFixedSize(150, 150);
	setFocusPolicy( TQ_NoFocus );

	mode = pvm;
	if (opacity < 0)   opacity = 0;
	if (opacity > 100) opacity = 100;
	menuOpacity = opacity/100.0;

	pixBackground = new KPixmap();
	pixOverlay = new KPixmap();
	pixBlended = new KPixmap();

	createPixmaps();
	blendPixmaps();
}

MenuPreview::~MenuPreview()
{
	delete pixBackground;
	delete pixOverlay;
	delete pixBlended;
}

void MenuPreview::createPixmaps()
{
	int w = width()-2;
	int h = height()-2;

	if (pixBackground)
		pixBackground->resize( w, h );
	if (pixOverlay)
		pixOverlay->resize( w, h );	
	if (pixBlended)
		pixBlended->resize( w, h );
	
	TQColorGroup cg = colorGroup();
	TQColor c1 = cg.background();
	TQColor c2 = cg.mid();

	if (pixBackground) {
		// Paint checkerboard
		TQPainter p;
		p.begin(pixBackground);
		for(int x=0; x < pixBackground->width(); x+=5)
			for(int y=0; y < pixBackground->height(); y+=5)
				p.fillRect( x, y, 5, 5, 
								 (x % 2) ? 
								((y % 2) ?  c2 : c1  ) : 	// See the grid? ;-)
								((y % 2) ?  c1 : c2  ) );	
		KIconLoader* icl = KGlobal::iconLoader();
		TQPixmap pix = icl->loadIcon("go", KIcon::Desktop, KIcon::SizeLarge, KIcon::ActiveState);
		p.drawPixmap( (width()-2-pix.width())/2, (height()-2-pix.height())/2, pix );
	}

	if (pixOverlay) {
		c1 = cg.button().light(110);
		c2 = cg.button().dark(110);
		KPixmapEffect::gradient( *pixOverlay, c1, c2, KPixmapEffect::VerticalGradient );
	}
}

void MenuPreview::blendPixmaps()
{
	// Rebuild pixmaps, and tqrepaint
	if (pixBlended && pixBackground) 
	{
		if (mode == Blend && pixOverlay) {
			TQImage src = pixOverlay->convertToImage();
			TQImage dst = pixBackground->convertToImage();
			KImageEffect::blend(src, dst, menuOpacity);
			pixBlended->convertFromImage( dst );
		} else if (mode == Tint) {
			TQColor clr = colorGroup().button();
			TQImage dst = pixBackground->convertToImage();
			KImageEffect::blend(clr, dst, menuOpacity);
			pixBlended->convertFromImage( dst );
		}
	}
}

void MenuPreview::setOpacity( int opacity )
{
	if (opacity < 0 || opacity > 100)
		return;

	if ((int)(menuOpacity*100) != opacity) {
		menuOpacity = opacity/100.0;
		blendPixmaps();
		tqrepaint( false );
	}
}

void MenuPreview::setPreviewMode( PreviewMode pvm )
{
	if (mode != pvm) {
		mode = pvm;
		blendPixmaps();
		tqrepaint( false );
	}
}

void MenuPreview::paintEvent( TQPaintEvent* /* pe */ )
{
	// Paint the frame and blended pixmap
	TQColorGroup cg = colorGroup();
	int x2 = width()-1;
	int y2 = height()-1;

	TQPainter p(this);
	p.setPen(cg.dark());
	p.drawLine(0, 0, x2, 0);
	p.drawLine(0, 0, 0, y2);
	p.setPen(cg.light());
	p.drawLine(1, y2, x2, y2);
	p.drawLine(x2, 1, x2, y2);

	if (mode == NoEffect)
		p.fillRect(1, 1, --x2, --y2, cg.button());
	else if (mode != NoEffect && pixBlended)
		p.drawPixmap(1, 1, *pixBlended, 0, 0, --x2, --y2);

	TQRect r = rect();
	r.moveBy(6,3);
	p.setPen( cg.text() );
	p.drawText( r, AlignTop | AlignLeft, TQString::number((int)(menuOpacity*100))+i18n("%") );
}

#include "menupreview.moc"

// vim: set noet ts=4:

