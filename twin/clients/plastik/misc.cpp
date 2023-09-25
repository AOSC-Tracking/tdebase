/* Plastik KWin window decoration
  Copyright (C) 2003 Sandro Giessl <ceebx@users.sourceforge.net>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
 */

#include <kpixmap.h>
#include <kpixmapeffect.h>

#include <tqcolor.h>
#include <tqimage.h>
#include <tqpainter.h>

#include "misc.h"

TQColor hsvRelative(const TQColor& baseColor, int relativeH, int relativeS, int relativeV)
{
    int h, s, v;
    baseColor.hsv(&h, &s, &v);

    h += relativeH;
    s += relativeS;
    v += relativeV;

    if(h < 0) { h = 0; }
    else if(h > 359) { h = 359; }
    if(s < 0) { s = 0; }
    else if(s > 255) { s = 255; }
    if(v < 0) { v = 0; }
    else if(v > 255) { v = 255; }

    TQColor c;
    c.setHsv( h, s, v );
    return c;
}

TQColor alphaBlendColors(const TQColor &bgColor, const TQColor &fgColor, const int a)
{

    // normal button...
    TQRgb rgb = bgColor.rgb();
    TQRgb rgb_b = fgColor.rgb();
    int alpha = a;
    if(alpha>255) alpha = 255;
    if(alpha<0) alpha = 0;
    int inv_alpha = 255 - alpha;

    TQColor result  = TQColor( tqRgb(tqRed(rgb_b)*inv_alpha/255 + tqRed(rgb)*alpha/255,
                                  tqGreen(rgb_b)*inv_alpha/255 + tqGreen(rgb)*alpha/255,
                                  tqBlue(rgb_b)*inv_alpha/255 + tqBlue(rgb)*alpha/255) );

    return result;
}

TQImage recolorImage(TQImage *img, TQColor color) {
    TQImage destImg(img->width(),img->height(),32);
    destImg.setAlphaBuffer(true);
    for (int x = 0; x < img->width(); x++) {
        for (int y = 0; y < img->height(); y++) {
            if(img->pixel(x,y) == tqRgb(0,0,255) ) {
                destImg.setPixel(x,y,color.rgb() ); // set to the new color
            } else {
                destImg.setPixel(x,y,tqRgba(0,0,0,0) ); // set transparent...
            }
        }
    }

    return destImg;
}
