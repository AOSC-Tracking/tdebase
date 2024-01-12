/* kasstartupitem.cpp
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
#include <tqpainter.h>
#include <tqbitmap.h>
#include <tqdrawutil.h>
#include <tqtimer.h>

#include <kdebug.h>
#include <tdeglobal.h>
#include <twin.h>
#include <kiconloader.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <tdelocale.h>
#include <taskmanager.h>

#include "kaspopup.h"

#include "kasstartupitem.h"
#include "kasstartupitem.moc"

KasStartupItem::KasStartupItem( KasBar *parent, Startup::Ptr startup )
    : KasItem( parent ),
      startup_(startup), frame(0)
{
    setText( startup_->text() );
    setIcon( icon() );
    setShowFrame( false );
    setAnimation( resources()->startupAnimation() );

    aniTimer = new TQTimer( this, "aniTimer" );
    connect( aniTimer, TQ_SIGNAL( timeout() ), TQ_SLOT( aniTimerFired() ) );
    aniTimer->start( 100 );
}

KasStartupItem::~KasStartupItem()
{
}

TQPixmap KasStartupItem::icon() const
{
   /**
    * This icon stuff should all be handled by the task manager api, but isn't yet.
    */
   TQPixmap pixmap;

   switch( kasbar()->itemSize() ) {
   case KasBar::Small:
     /* ***** NOP ******
	pixmap = TDEGlobal::iconLoader()->loadIcon( startup_->icon(),
						  TDEIcon::NoGroup,
						  TDEIcon::SizeSmall );
     */
      break;
   case KasBar::Medium:
	pixmap = TDEGlobal::iconLoader()->loadIcon( startup_->icon(),
						  TDEIcon::NoGroup,
						  TDEIcon::SizeMedium );
      break;
   case KasBar::Large:
	pixmap = TDEGlobal::iconLoader()->loadIcon( startup_->icon(),
						  TDEIcon::NoGroup,
						  TDEIcon::SizeLarge );
      break;
   case KasBar::Huge:
	pixmap = TDEGlobal::iconLoader()->loadIcon( startup_->icon(),
						  TDEIcon::NoGroup,
						  TDEIcon::SizeHuge );
      break;
   case KasBar::Enormous:
	pixmap = TDEGlobal::iconLoader()->loadIcon( startup_->icon(),
						  TDEIcon::NoGroup,
						  TDEIcon::SizeEnormous );
      break;
   default:
	pixmap = TDEGlobal::iconLoader()->loadIcon( "error",
						  TDEIcon::NoGroup,
						  TDEIcon::SizeSmall );
   }

   return pixmap;
}

void KasStartupItem::aniTimerFired()
{

    if ( frame == 40 )
	frame = 0;
    else
	frame++;

    advanceAnimation();
}

void KasStartupItem::paint( TQPainter *p )
{
    p->save();

    p->setClipRect( 0, 0, extent(), extent(), TQPainter::CoordPainter );
    p->translate( extent()/2, extent()/2 );
    p->rotate( 9.0L * frame );
    p->scale( 0.7L, 0.7L );
    p->translate( -extent()/2, -extent()/2 );

    KasItem::paint( p );

    p->restore();
    paintAnimation( p );
}

