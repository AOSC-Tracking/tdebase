/***************************************************************************
 *   Copyright Brian Ledbetter 2001-2003 <brian@shadowcom.net>             *
 *   Copyright Ravikiran Rajagopal 2003                                    *
 *   ravi@ee.eng.ohio-state.edu                                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation. (The original KSplash/ML   *
 *   codebase (upto version 0.95.3) is BSD-licensed.)                      *
 *                                                                         *
 ***************************************************************************/

#include <tdeapplication.h>
#include <kcursor.h>
#include <kdebug.h>
#include <tdeglobalsettings.h>
#include <kiconloader.h>
#include <kprogress.h>
#include <twin.h>

#include <tqdesktopwidget.h>
#include <tqhbox.h>
#include <tqlabel.h>
#include <tqpoint.h>
#include <tqrect.h>

#include "wndstatus.h"
#include "wndstatus.moc"

// WndStatus::WndStatus(): Display a nifty status bar at
// the bottom of the screen, so the user always knows what's
// happening to his system.
WndStatus::WndStatus( TQPalette /*pal*/,
                      int xineramaScreen,
                      bool atTop, bool pbVisible,
                      const TQFont& font,
                      const TQColor& fgc, const TQColor & bgc,
                      const TQString& icon
                    )
    :TQHBox( 0, "wndStatus", (WFlags)(WStyle_Customize|WX11BypassWM) )
{
  setFrameStyle( TQFrame::NoFrame );
  //setPalette( pal );
  setPaletteBackgroundColor( bgc );
  setPaletteForegroundColor( fgc );
  setCursor( KCursor::blankCursor() );
  setSpacing( 5 );

  const TQRect rect = tdeApp->desktop()->screenGeometry( xineramaScreen );
  // TDEGlobalSettings::splashScreenDesktopGeometry(); cannot be used here.

  TQLabel *pix = new TQLabel( this );
  TQPixmap _icon( SmallIcon(icon.isNull()||icon.isEmpty()?TQString("system-run"):icon) );
  pix->setPixmap( _icon );
  setStretchFactor(pix,0);
  pix->setFixedWidth(16);

  m_label = new TQLabel( this );
  m_label->setFont( font );
  m_label->setPaletteBackgroundColor( bgc );
  m_label->setPaletteForegroundColor( fgc );
  //TQFontMetrics metrics( font );
  //m_label->setFixedHeight( metrics.height() );
  m_label->setText(TQString(""));
  m_label->setFixedWidth(rect.width()-105-16-10); // What's this magic number?
  m_label->show();

  m_progress = new KProgress( this );
  setStretchFactor(m_progress,0);
  m_progress->setFixedWidth(100);

  TQWidget *widg = new TQWidget( this );
  setStretchFactor(widg,50);

  setFixedSize( rect.width(), TQMAX(m_progress->height(),m_label->height()) );

  if ( atTop )
    move( rect.topLeft() );
  else
    move( rect.bottomLeft().x(), rect.bottomLeft().y()-height()+1 ); // The +1 is to work around a bug in screenGeometry().

  if (!pbVisible)
    m_progress->hide();
}

void WndStatus::slotSetMessage( const TQString& msg )
{
  raise();
  m_label->setText( msg );
}

void WndStatus::slotUpdateProgress( int i )
{
  raise();
  m_progress->setProgress( i );
}

void WndStatus::slotUpdateSteps( int i )
{
  m_progress->setTotalSteps( i );
}
