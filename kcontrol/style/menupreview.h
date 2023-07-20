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

#ifndef __MENUPREVIEW_H
#define __MENUPREVIEW_H

#include <tqwidget.h>

class KPixmap;

class MenuPreview : public TQWidget
{
	TQ_OBJECT

public:
	enum PreviewMode {
		NoEffect = 0,
		Tint,
		Blend 
	};

	MenuPreview( TQWidget* parent, int opacity, PreviewMode pvm );
	~MenuPreview();

public slots:
	void setOpacity( int opacity );
	void setPreviewMode( PreviewMode pvm );

protected:
	void paintEvent( TQPaintEvent* pe );

private:
	void createPixmaps();
	void blendPixmaps();

	KPixmap* pixBackground;
	KPixmap* pixOverlay;
	KPixmap* pixBlended;
	float menuOpacity;
	PreviewMode mode;
};

#endif // __MENUPREVIEW_H

