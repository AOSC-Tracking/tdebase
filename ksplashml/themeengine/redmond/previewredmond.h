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

#ifndef __PREVIEWREDMOND_H__
#define __PREVIEWREDMOND_H__

#include <kiconloader.h>

#include <tqcolor.h>
#include <tqfont.h>
#include <tqwidget.h>

/*
 * class PreviewRedmond: Provides a sneak peek at how certain Redmond
 * settings will look. This will not be able to render any background
 * images, so we'll just use a nice shade of gray or black as the
 * background...
 */
class PreviewRedmond: public TQWidget
{
  TQ_OBJECT
public:

  PreviewRedmond( TQWidget* );

  inline void setWelcomeString( const TQString& s )
  {
    m_welcomeString = s;
    _updateCache();
  }
  inline void setUserString( const TQString& s )
  {
    m_userString = s;
    _updateCache();
  }

  inline void setWelcomeFont( const TQFont& f )
  {
    m_welcomeFont = f;
    _updateCache();
  }
  inline void setUserFont( const TQFont& f )
  {
    m_userFont = f;
    _updateCache();
  }
  inline void setStatusFont( const TQFont& f )
  {
    m_statusFont = f;
    _updateCache();
  }

  inline void setWelcomeColor( const TQColor& c )
  {
    m_welcomeColor = c;
    _updateCache();
  }
  inline void setWelcomeShadowColor( const TQColor& c )
  {
    m_welcomeShadowColor = c;
    _updateCache();
  }
  inline void setUserColor( const TQColor& c )
  {
    m_userColor = c;
    _updateCache();
  }
  inline void setStatusColor( const TQColor& c )
  {
    m_statusColor = c;
    _updateCache();
  }

  inline void setIcon( const TQString& s )
  {
    m_icon = DesktopIcon( s );
    _updateCache();
  }

protected:
  void _updateCache();
  void paintEvent( TQPaintEvent* );
  void resizeEvent( TQResizeEvent* );

  TQPixmap m_cache;

  TQString m_welcomeString, m_userString;
  TQFont m_welcomeFont, m_userFont, m_statusFont;
  TQColor m_welcomeColor, m_welcomeShadowColor, m_userColor, m_statusColor;
  TQPixmap m_icon;

  bool m_showWelcomeString, m_showUserString, m_showUserIcon, m_showStatusString;
};

#endif
