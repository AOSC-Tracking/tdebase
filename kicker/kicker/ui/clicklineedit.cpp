/*
    This file is part of libkdepim.
    Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org>
    based on code by Cornelius Schumacher <schumacher@kde.org> 

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/


#include "clicklineedit.h"

#include "tqpainter.h"

using namespace KPIM;

ClickLineEdit::ClickLineEdit(TQWidget *parent, const TQString &msg, const char* name) :
  KLineEdit(parent, name) 
{
  mDrawClickMsg = true;
  setClickMessage( msg ); 
}

ClickLineEdit::~ClickLineEdit() {}


void ClickLineEdit::setClickMessage( const TQString &msg )
{
  mClickMessage = msg;
  repaint();
}

void ClickLineEdit::setText( const TQString &txt )
{
  mDrawClickMsg = txt.isEmpty();
  repaint();
  KLineEdit::setText( txt );
}

void ClickLineEdit::drawContents( TQPainter *p )
{
  KLineEdit::drawContents( p );

  if ( mDrawClickMsg == true && !hasFocus() ) {
    TQPen tmp = p->pen();
    p->setPen( gray );
    TQRect cr = contentsRect();
    p->drawText( cr, AlignAuto|AlignVCenter, mClickMessage );
    p->setPen( tmp );
  }
}

void ClickLineEdit::focusInEvent( TQFocusEvent *ev )
{
  if ( mDrawClickMsg == true ) 
  { 
    mDrawClickMsg = false;
    repaint();
  }
  TQLineEdit::focusInEvent( ev );
}

void ClickLineEdit::focusOutEvent( TQFocusEvent *ev )
{
  if ( text().isEmpty() )
  {
    mDrawClickMsg = true;
    repaint();
  }
  TQLineEdit::focusOutEvent( ev );
}

#include "clicklineedit.moc"
