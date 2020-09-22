/*
 *
 * This file is part of the KDE project, module kdesktop.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#ifndef __CROSSFADE_H__
#define __CROSSFADE_H__

#include <tqtimer.h>
#include <tqpainter.h>
#include <tqpixmap.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <kdebug.h>
#include <unistd.h>



inline TQPixmap crossFade(const TQPixmap &pix1, const TQPixmap &pix2, double r_alpha,
	                bool sync = false){

    TQPixmap pix = TQPixmap(1,1,8); 
    int mw,mh;
    mw = pix1.width();
    mh = pix1.height();

    short unsigned int alpha = 0xffff * (1-r_alpha);

    XRenderColor clr = { 0, 0, 0, alpha }; 
    XRenderPictureAttributes pa;
    pa.repeat = True;
    Picture pic = XRenderCreatePicture(pix.x11Display(), pix.handle(),
	    XRenderFindStandardFormat (pix.x11Display(), PictStandardA8),
	    CPRepeat, &pa);
    XRenderFillRectangle(pix.x11Display(), PictOpSrc, pic,
			 &clr, 0, 0, 1, 1);
    TQPixmap dst(pix1);
    dst.detach();
    XRenderComposite(pix.x11Display(), PictOpOver, pix2.x11RenderHandle(),
	    pic, dst.x11RenderHandle(),0,0, 0,0, 0,0, mw,mh);

    if (sync) {
       XSync(pix.x11Display(), false);	
    }
    XRenderFreePicture(pix.x11Display(), pic);
    return dst;
}

#endif // __CROSSFADE_H__
