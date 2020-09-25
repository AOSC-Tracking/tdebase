/* kasbar.h
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

#ifndef KASRESOURCES_H
#define KASRESOURCES_H

#include <tqbitmap.h>
#include <tqcolor.h>
#include <tqbrush.h>
#include <tqpen.h>
#include <kpixmap.h>

#include <tqobject.h>
#include <tqvaluevector.h>

class KasBar;

/**
 * Central class that holds the graphical resources for the bar.
 *
 * @author Richard Moore, rich@kde.org
 */
class KasResources : public TQObject
{
    Q_OBJECT

public:
    KasResources( KasBar *parent, const char *name=0 );
    virtual ~KasResources();

    TQColor labelPenColor() const    { return labelPenColor_; }
    TQColor labelBgColor() const     { return labelBgColor_; }
    TQColor inactivePenColor() const { return inactivePenColor_; }
    TQColor inactiveBgColor() const  { return inactiveBgColor_; }
    TQColor activePenColor() const   { return activePenColor_; }
    TQColor activeBgColor() const    { return activeBgColor_; }

    TQColor progressColor() const    { return progressColor_; }      
    TQColor attentionColor() const    { return attentionColor_; }      

    /** Accessor for the min icon (singleton). */
    TQBitmap minIcon();

    /** Accessor for the max icon (singleton). */
    TQBitmap maxIcon();

    /** Accessor for the shade icon (singleton). */
    TQBitmap shadeIcon();

    /** Accessor for the attention icon (singleton). */
    TQBitmap attentionIcon();

    /** Accessor for the modified icon (singleton). */
    TQPixmap modifiedIcon();

    /** Accessor for the micro min icon (singleton). */
    TQPixmap microMinIcon();

    /** Accessor for the micro max icon (singleton). */
    TQPixmap microMaxIcon();

    /** Accessor for the micro shade icon (singleton). */
    TQPixmap microShadeIcon();

    /** Accessor used by items to get the active bg fill. */
    KPixmap activeBg();
    
    /** Accessor used by items to get the inactive bg fill. */
    KPixmap inactiveBg();

    TQValueVector<TQPixmap> startupAnimation();

public slots:
    void setLabelPenColor( const TQColor &color );
    void setLabelBgColor( const TQColor &color );
    void setInactivePenColor( const TQColor &color );
    void setInactiveBgColor( const TQColor &color );
    void setActivePenColor( const TQColor &color );
    void setActiveBgColor( const TQColor &color );

    void setProgressColor( const TQColor &color );
    void setAttentionColor( const TQColor &color );

    void itemSizeChanged();

signals:
    void changed();

private:
    KasBar *kasbar;

    TQBitmap minPix;
    TQBitmap maxPix;
    TQBitmap shadePix;
    TQBitmap attentionPix;
    TQPixmap modifiedPix;
    TQPixmap microShadePix;
    TQPixmap microMaxPix;
    TQPixmap microMinPix;

    TQColor labelPenColor_;
    TQColor labelBgColor_;
    TQColor activePenColor_;
    TQColor activeBgColor_;
    TQColor inactivePenColor_;
    TQColor inactiveBgColor_;

    TQColor progressColor_;
    TQColor attentionColor_;

    KPixmap actBg;
    KPixmap inactBg;

    TQValueVector<TQPixmap> startupFrames_;
};

#endif // KASRESOURCES_H

