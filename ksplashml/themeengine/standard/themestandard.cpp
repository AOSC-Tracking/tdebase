/***************************************************************************
 *   Copyright Brian Ledbetter 2001-2003 <brian@shadowcom.net>             *
 *   Copyright Ravikiran Rajagopal 2003 <ravi@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation. (The original KSplash/ML   *
 *   codebase (upto version 0.95.3) is BSD-licensed.)                      *
 *                                                                         *
 ***************************************************************************/

#include <tdeapplication.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <tdeglobalsettings.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <tdeconfig.h>

#include <tqdesktopwidget.h>
#include <tqlabel.h>
#include <tqpalette.h>
#include <tqpixmap.h>
#include <tqwidget.h>

#include <objkstheme.h>
#include "themestandard.h"
#include "themestandard.moc"
#include "wndicon.h"
#include "wndstatus.h"

ThemeStandard::ThemeStandard( TQWidget *parent, const char *name, const TQStringList &args )
  :ThemeEngine( parent, name, args ), mIcon(0L), mPrevIcon(0L), mIconCount(0), mStdIconWidth(-1),
  mIconPos(WndIcon::HBottomLeft), mSbAtTop(false), mSbVisible(true), mSbPbVisible(true), mSbFontName("helvetica"),
  mSbFontSz(16), mSbFontBold(true), mSbFontItalic(false), mSbFont(TQFont()), mSbFg(TQColor()), mSbBg(TQColor()),
  mSbIcon("system-run"), mIconsVisible(true), mIconsJumping(true), mSplashScreen("(Default)")
{
  _readSettings();
  _initUi();
}

void ThemeStandard::_initUi()
{
  setFrameStyle( TQFrame::NoFrame );

  TQString pixName = mTheme->locateThemeData( mSplashScreen );

  if( mSplashScreen == "(Default)" || pixName.isEmpty() )
  {
    TQString resource_prefix = "pics/";
    if ( mTheme->loColor() )
      resource_prefix += "locolor/";
    pixName = locate( "appdata", resource_prefix + "splash.png");
  }

  TQPixmap px = TQPixmap( pixName );

  if( !px.isNull() )
  {
    //kdDebug() << "Loaded splash " << mSplashScreen << endl;
    int pw = px.width();
    int ph = px.height();

    TQLabel *lbl = new TQLabel( this );
    lbl->setBackgroundMode( NoBackground );
    lbl->setFixedSize( pw, ph );
    lbl->setPixmap( px );

    resize( pw, ph );
  }
  else
  {
    //kdDebug() << "Couldn't load splash " << mSplashScreen << endl;
    resize( 0, 0 );
  }

  const TQRect rect = tdeApp->desktop()->screenGeometry( mTheme->xineramaScreen() );
  // TDEGlobalSettings::splashScreenDesktopGeometry(); cannot be used here.

  move( rect.x() + (rect.width() - size().width())/2,
        rect.y() + (rect.height() - size().height())/2 );

  mStatus = new WndStatus( TQPalette(), mTheme->xineramaScreen(), mSbAtTop, mSbPbVisible, mSbFont, mSbFg, mSbBg, mSbIcon );
}

void ThemeStandard::showEvent( TQShowEvent * )
{

  ThemeEngine::show();

  if( mSbVisible )
  {
    mStatus->show();
    mStatusBarHeight = mStatus->height();
  }
  else
  {
    mStatusBarHeight = 0L;
  }
}

// Adjust the visible icon.
void ThemeStandard::slotSetPixmap( const TQString& pxn )
{
  TQPixmap px = DesktopIcon( pxn );

  if ( px.isNull() )
    px = DesktopIcon( "go" );

  if ( !mIconsVisible )
    return;

  /* (We only use prev_i if jumping is enabled...) */
  if ( mIconsJumping && mPrevIcon )
    emit mPrevIcon->slotStopJumping();

  if ( mStdIconWidth < 0 )
    mStdIconWidth = DesktopIcon( "go" ).width();

  mIcon = new WndIcon( ++mIconCount, mStdIconWidth, mStatusBarHeight, mTheme->xineramaScreen(),
                       px, TQString::null, mIconPos, mSbAtTop, mIconsJumping );
  mIcon->show();

  if( mIconsJumping )
  {
    emit mIcon->slotJump();
    mPrevIcon = mIcon;
  }
}

void ThemeStandard::_readSettings()
{

  if ( !mTheme )
    return;

  TDEConfig *cfg = mTheme->themeConfig();
  if ( !cfg )
    return;

  //if ( !cfg->hasGroup( TQString("KSplash Theme: %1").arg(mTheme->theme()) ) )
  //  return;
  cfg->setGroup( TQString("KSplash Theme: %1").arg(mTheme->theme()) );

  TQString sbpos = cfg->readEntry( "Statusbar Position", "Bottom" ).upper();
  mSbAtTop = ( sbpos == "TOP" );
  mSbVisible = cfg->readBoolEntry( "Statusbar Visible", true);
  mSbPbVisible = cfg->readBoolEntry( "Progress Visible", true);

  mSbFontName = cfg->readEntry( "Statusbar Font", "Sans Serif" );
  mSbFontSz = cfg->readNumEntry( "Statusbar Font Size", 16 );
  mSbFontBold = cfg->readBoolEntry( "Statusbar Font Bold", true );
  mSbFontItalic = cfg->readBoolEntry( "Statusbar Font Italic", false );
  mSbFont = TQFont( mSbFontName, mSbFontSz, ( mSbFontBold? TQFont::Bold : TQFont::Normal ) );
  if( mSbFontItalic )
    mSbFont.setItalic( true );

  mSbFg = cfg->readColorEntry( "Statusbar Foreground", &TQt::white );
  mSbBg = cfg->readColorEntry( "Statusbar Background", &TQt::black );
  mSbIcon = cfg->readEntry( "Statusbar Icon", "system-run" );
  mIconsVisible = cfg->readBoolEntry( "Icons Visible", true);
  mIconsJumping = cfg->readBoolEntry( "Icons Jumping", true);
  mIconPos = (WndIcon::Position)cfg->readNumEntry( "Icon Position", 0 );
  mSplashScreen = cfg->readEntry( "Splash Screen", "(Default)");
  // cfg->readBoolEntry( "Allow Configuration", true );
}
