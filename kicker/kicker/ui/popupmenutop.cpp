/*****************************************************************

Copyright (c) 2007 Montel Laurent <lmontel@mandriva.com>

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

#include "popupmenutop.h"
#include "kickerSettings.h"
#include <tdestandarddirs.h>
#include <tqimage.h>
#include <kdebug.h>

PopupMenuTop::PopupMenuTop() : 
        TQCustomMenuItem()
{
  init();
}

void PopupMenuTop::init()
{
  TQString leftSideName = KickerSettings::leftSideTopPixmapName();
  TQString rightSideName = KickerSettings::rightSideTopPixmapName();
  TQString sideTileName = KickerSettings::sideTopTileName();

  leftSidePixmap.load(locate("data", "kicker/pics/" + leftSideName));
  rightSidePixmap.load(locate("data", "kicker/pics/" + rightSideName));
  sideTilePixmap.load(locate("data", "kicker/pics/" + sideTileName));
  
  if (sideTilePixmap.isNull())
    {
      kdDebug(1210) << "Can't find a side tile pixmap" << endl;
      return;
    }
  
  if (leftSidePixmap.height() != sideTilePixmap.height() || 
      leftSidePixmap.height() != rightSidePixmap.height())
    {
      kdDebug(1210) << "Pixmaps have to be the same size" << endl;
      return;
    }
  if (sideTilePixmap.width() < 100)
    {
      int tiles = (int)(100 / sideTilePixmap.width()) + 1;
      TQPixmap preTiledPixmap(sideTilePixmap.width()*tiles, sideTilePixmap.height());
      TQPainter p2(&preTiledPixmap);
      p2.drawTiledPixmap(preTiledPixmap.rect(), sideTilePixmap);
      sideTilePixmap = preTiledPixmap;
    }
}

void PopupMenuTop::paint(TQPainter* p, const TQColorGroup& cg, 
	   bool /* act */, bool /*enabled*/, 
	   int x, int y, int w, int h)
{
  //kdDebug()<<" PopupMenuTop::paint\n";
  p->save();
  p->drawPixmap( x,y, leftSidePixmap );
  p->drawTiledPixmap(TQRect(x+leftSidePixmap.width(),y,w,h),sideTilePixmap);
  p->drawPixmap( x + w - rightSidePixmap.width(), y, rightSidePixmap );
  p->restore();
}

TQSize PopupMenuTop::sizeHint()
{
  TQSize size = TQSize(50,sideTilePixmap.height());
  return size;
}
