////////////////////////////////////////////////////////////////////////////////
//
// Class Name    : KFI::CFontPreview
// Author        : Craig Drummond
// Project       : K Font Installer
// Creation Date : 04/11/2001
// Version       : $Revision$ $Date$
//
////////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
////////////////////////////////////////////////////////////////////////////////
// (C) Craig Drummond, 2001, 2002, 2003, 2004
////////////////////////////////////////////////////////////////////////////////

#include "FontPreview.h"
#include <kapplication.h>
#include <klocale.h>
#include <tqpainter.h>
#include <tqimage.h>
#include <stdlib.h>

namespace KFI
{

CFontPreview::CFontPreview(TQWidget *parent, const char *name)
            : TQWidget(parent, name),
              itsCurrentFace(1),
              itsLastWidth(0),
              itsLastHeight(0),
              itsBgndCol(eraseColor())
{
}

void CFontPreview::showFont(const KURL &url)
{
    itsCurrentUrl=url;
    showFace(1);
}

void CFontPreview::showFace(int face)
{
    itsCurrentFace=face;
    showFont();
}

void CFontPreview::showFont()
{
    itsLastWidth=width();
    itsLastHeight=height();

    if(!itsCurrentUrl.isEmpty() &&
       itsEngine.draw(itsCurrentUrl, itsLastWidth, itsLastHeight, itsPixmap, itsCurrentFace-1, false))
    {
        setEraseColor(Qt::white);
        update();
        emit status(true);
    }
    else
    {
        TQPixmap nullPix;

        setEraseColor(itsBgndCol);
        itsPixmap=nullPix;
        update();
        emit status(false);
    }
}

void CFontPreview::paintEvent(TQPaintEvent *)
{
    TQPainter paint(this);
 
    if(itsPixmap.isNull())
    {
        if(!itsCurrentUrl.isEmpty())
        {
            paint.setPen(kapp->tqpalette().active().text());
            paint.drawText(rect(), AlignCenter, i18n(" No preview available"));
        }
    }
    else
    {
        static const int constStepSize=16;

        if(abs(width()-itsLastWidth)>constStepSize || abs(height()-itsLastHeight)>constStepSize)
            showFont();
        else            
            paint.drawPixmap(0, 0, itsPixmap);
    }
}

TQSize CFontPreview::sizeHint() const
{
    return TQSize(132, 132);
}

TQSize CFontPreview::minimumSizeHint() const
{
    return TQSize(32, 32);
}

}

#include "FontPreview.moc"
