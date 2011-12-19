/* This file is proposed to be part of the KDE base.
 * Copyright (C) 2003 Laur Ivan <laurivan@eircom.net>
 *
 * Many thanks to:
 *  - Bernardo Hung <deciare@gta.igs.net> for the enhanced shadow
 *  algorithm (currently used)
 *  - Tim Jansen <tim@tjansen.de> for the API updates and fixes.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <stdio.h>

#include <tqcolor.h>
#include <tqpalette.h>
#include <tqstring.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <kwordwrap.h>
#include <kiconview.h>
#include <kdebug.h>
#include <kglobalsettings.h>

#include <kshadowengine.h>
#include "kdesktopshadowsettings.h"
#include "kfileividesktop.h"

//#define DEBUG

/* Changelog:
 */

KFileIVIDesktop::KFileIVIDesktop(KonqIconViewWidget *iconview, KFileItem* fileitem,
				 int size, KShadowEngine *shadow) :
  KFileIVI(iconview, fileitem, size),
  m_selectedImage(0L),
  m_normalImage(0L),
  _selectedUID(0),
  _normalUID(0)
{
  m_shadow = shadow;
  oldText = "";

  calcRect( text() ); // recalculate rect including shadow
}

KFileIVIDesktop::~KFileIVIDesktop()
{
  delete m_selectedImage;
  delete m_normalImage;
}

void KFileIVIDesktop::calcRect( const TQString& _text )
{
    KIconViewItem::calcRect( _text );

    if ( !iconView() || !m_shadow ||
         !wordWrap() || !( static_cast<KDesktopShadowSettings *>
             ( m_shadow->shadowSettings() ) )->isEnabled() )
        return;

    int spread = shadowThickness();
    TQRect itemTextRect = textRect();
    TQRect itemRect = rect();

    itemTextRect.setBottom( itemTextRect.bottom() + spread );
    itemTextRect.setRight( itemTextRect.right() + spread );
    itemRect.setBottom( itemRect.bottom() + spread );
    itemRect.setRight( itemRect.right() + spread );

    setTextRect( itemTextRect );
    setItemRect( itemRect );
}

void KFileIVIDesktop::paintItem( TQPainter *p, const TQColorGroup &cg)
{
  TQColorGroup colors = updateColors(cg);

  TQIconView* view = iconView();
  Q_ASSERT( view );

  if ( !view )
    return;

  if ( !wordWrap() )
    return;

  p->save();

  // draw the pixmap as in KIconViewItem::paintItem(...)
  paintPixmap(p, colors);

  //
  // Paint the text as shadowed if the shadow is available
  //
  if (m_shadow != 0L && (static_cast<KDesktopShadowSettings *> (m_shadow->shadowSettings()))->isEnabled())
    drawShadowedText(p, colors);
  else {
    paintFontUpdate(p);
    paintText(p, colors);
  }

  p->restore();

  paintOverlay(p);
}

bool KFileIVIDesktop::shouldUpdateShadow(bool selected)
{
  unsigned long uid = (static_cast<KDesktopShadowSettings *> (m_shadow->shadowSettings()))->UID();
  TQString wrapped = wordWrap()->wrappedString();

  if (wrapped != oldText){
    oldText = wrapped;
    _selectedUID = _normalUID = 0;
  }

  if (selected == true)
    return (uid != _selectedUID);
  else
    return (uid != _normalUID);

  return false;
}



void KFileIVIDesktop::drawShadowedText( TQPainter *p, const TQColorGroup &cg )
{
  bool drawRoundedRect = KGlobalSettings::iconUseRoundedRect();

  int textX;
  if (drawRoundedRect == true)
    textX = textRect( FALSE ).x() + 4;
  else
    textX = textRect( FALSE ).x() + 2;
  int textY = textRect( FALSE ).y();
  int align = ((KIconView *) iconView())->itemTextPos() == TQIconView::Bottom
    ? AlignHCenter : AlignAuto;
  bool rebuild = shouldUpdateShadow(isSelected());

  KDesktopShadowSettings *settings = (KDesktopShadowSettings *) (m_shadow->shadowSettings());

  unsigned long uid = settings->UID();

  p->setFont(iconView()->font());
  paintFontUpdate(p);
  TQColor shadow;
  TQColor text;
  int spread = shadowThickness();

  if ( isSelected() && settings->selectionType() != KShadowSettings::InverseVideoOnSelection ) {
    text = cg.highlightedText();
    TQRect rect = textRect( false );
    rect.setRight( rect.right() - spread );
    rect.setBottom( rect.bottom() - spread + 1 );
    if (drawRoundedRect == true) {
      p->setBrush( TQBrush( cg.highlight() ) );
      p->setPen( TQPen( cg.highlight() ) );
      p->drawRoundRect( rect,
		      1000 / rect.width(),
		      1000 / rect.height() );
    }
    else {
      p->fillRect( textRect( false ), cg.highlight() );
    }
  }
  else {
    // use shadow
    if ( isSelected() ) {
      // inverse text and shadow colors
      shadow = settings->textColor();
      text = settings->bgColor();
      if ( rebuild ) {
        setSelectedImage( buildShadow( p, align, shadow ) );
        _selectedUID = uid;
      }
    }
    else {
      text = settings->textColor();
      shadow = ( settings->bgColor().isValid() ) ? settings->bgColor() :
               ( tqGray( text.rgb() ) > 127 ) ? black : white;
      if (rebuild) {
        setNormalImage(buildShadow(p, align, shadow));
        _normalUID = uid;
      }
    }

    // draw the shadow
    int shadowX = textX - spread + settings->offsetX();
    int shadowY = textY - spread + settings->offsetY();

    p->drawImage(shadowX, shadowY,
      (isSelected()) ? *selectedImage() : *normalImage(),
      0, 0, -1, -1, DITHER_FLAGS);
  }

  // draw the text
  p->setPen(text);
  wordWrap()->drawText( p, textX, textY, align | KWordWrap::Truncate );
}


TQImage *KFileIVIDesktop::buildShadow( TQPainter *p, const int align,
                                      TQColor &shadowColor )
{
  TQPainter pixPainter;
  int spread = shadowThickness();

  TQPixmap textPixmap(textRect( FALSE ).width() + spread * 2 + 2,
    textRect( FALSE ).height() + spread * 2 + 2);

  textPixmap.fill(TQColor(0,0,0));
  textPixmap.setMask( textPixmap.createHeuristicMask(TRUE) );

  pixPainter.begin(&textPixmap);
  pixPainter.setPen(white);    // get the pen from the root painter
  pixPainter.setFont(p->font()); // get the font from the root painter
  wordWrap()->drawText( &pixPainter, spread, spread, align | KWordWrap::Truncate );
  pixPainter.end();

  return new TQImage(m_shadow->makeShadow(textPixmap, shadowColor));
}

int KFileIVIDesktop::shadowThickness() const
{
  return ( ( m_shadow->shadowSettings()->thickness() + 1 ) >> 1 ) + 1;
}

