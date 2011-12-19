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

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kfontcombo.h>
#include <kgenericfactory.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kuser.h>
#include <kemailsettings.h>

#include <tqcheckbox.h>
#include <tqdesktopwidget.h>
#include <tqlabel.h>
#include <tqpainter.h>
#include <tqrect.h>
#include <tqstringlist.h>
#include <tqwidget.h>

#include <objkstheme.h>
#include "themeredmond.h"
#include "previewredmond.h"
#include "themeredmond.moc"

K_EXPORT_COMPONENT_FACTORY( ksplashredmond, KGenericFactory<ThemeRedmond>( "ksplash" ) )

CfgRedmond::CfgRedmond( TQWidget *p, KConfig *c )
  :ThemeEngineConfig( p, c )
{
  TQVBox *vbox = new TQVBox( this );
  vbox->setSpacing( KDialog::spacingHint() );

  TQFont defaultFont( "Arial", 48, TQFont::Bold );
  defaultFont.setItalic( true );
  TQFont defaultUsernameFont( "Arial", 16, TQFont::Bold );
  TQFont defaultActionFont( "Arial", 12, TQFont::Bold );
  TQColor defaultDarkColor( 3, 47, 156 );
  TQColor defaultWhiteColor( Qt::white );

  TQHBox *hbox = new TQHBox( vbox );
  hbox->setFrameStyle( TQFrame::WinPanel );
  hbox->setFrameShadow( TQFrame::Sunken );
  PreviewRedmond* _preview = new PreviewRedmond( hbox );
  _preview->setFixedSize( 320, 200 );

  _preview->setWelcomeString( c->readEntry( "Welcome Text", i18n("Welcome") ) );

  _preview->setWelcomeFont( c->readFontEntry( "Welcome Font", &defaultFont ) );
  _preview->setUserFont( c->readFontEntry( "Username Font", &defaultUsernameFont ) );
  _preview->setStatusFont( c->readFontEntry( "Action Font", &defaultActionFont ) );

  _preview->setWelcomeColor( c->readColorEntry( "Welcome Text Color", &defaultWhiteColor ) );
  _preview->setWelcomeShadowColor( c->readColorEntry( "Welcome Shadow Color", &defaultDarkColor ) );
  _preview->setUserColor( c->readColorEntry( "Username Text Color", &defaultWhiteColor ) );
  _preview->setStatusColor( c->readColorEntry( "Action Text Color", &defaultDarkColor ) );

  _preview->setIcon( c->readEntry( "User Icon", "kmenu" ) );

  TQLabel *lbl = new TQLabel( vbox );
  lbl->setText( i18n("(Sorry, but I haven't finished writing this one yet...)") );
}

ThemeRedmond::ThemeRedmond( TQWidget *parent, const char *name, const TQStringList &args )
    :ThemeEngine( parent, name, args )
{
  _readSettings();
  _initUi();
}

void ThemeRedmond::_initUi()
{
  const TQRect screen = kapp->desktop()->screenGeometry( mTheme->xineramaScreen() );
  //TQRect fullScreen = KGlobalSettings::desktopGeometry(0L);

  mImage.resize( screen.width(), screen.height() );

  TQPainter p;
  p.begin( &mImage );
  p.fillRect( screen, TQColor(3,47,156) );
  p.setPen( mActionTextColor );

  TQString bgimg;

  // Start by seeing if the theme prefers a particular image.
  if( !mBackgroundImage.isEmpty() )
    bgimg = mTheme->locateThemeData( mBackgroundImage );

  /*
  * See if there is a resolution-specific background in THEMEDIR
  * before looking for the "generic" one. Having a Background.png
  * file for each resolution will greatly reduce the amount of time
  * it takes to initialize this ThemeEngine when running, due to
  * the fact that no scaling will be necessary to display the image.
  *
  * File must be named THEMEDIR/Background-WWWxHHH.png -- for example,
  * Mytheme/Background-1024x768.png
  *
  * ADDITIONAL NOTE: The resolution you specify will be obtained from
  * the PRIMARY SCREEN ONLY when running in XINERAMA mode. Be sure to
  * provide backgrounds using common resolutions (I recommend at least
  * providing 640x480 [unofficially unsupported by KDE], 800x600, and
  * 1024x768 images.)
  */
  if( bgimg.isEmpty() )
    bgimg = mTheme->locateThemeData( TQString( "Background-%2x%3.png" ).arg( screen.width() ).arg( screen.height() ) );

  // If that can't be found, look for THEMEDIR/Background.png
  if( bgimg.isNull() && !mTheme->themeDir().isNull() )
    bgimg = mTheme->locateThemeData( "Background.png" );

  if( mPixmap.isNull() )
    mPixmap = DesktopIcon( "kmenu", 48 );

  TQPixmap pix( bgimg );

  if( !pix.isNull() )
  {

    TQPixmap tmp( TQSize(screen.width(), screen.height() ) );
    float sw = (float)screen.width() / pix.width();
    float sh = (float)(screen.height()) / pix.height();

    TQWMatrix matrix;
    matrix.scale( sw, sh );
    tmp = pix.xForm( matrix );

    p.drawPixmap( 0, 0, tmp );
  }

  TQFont f = mWelcomeFont;
  if( mWelcomeFontItalic )
    f.setItalic( true ); // this SHOULD BE stored in the TQFont entry, dang it.
  p.setFont( f );
  TQFontMetrics met( f );
  TQSize fmet = met.size( 0L, mWelcomeText );

  // Paint the "Welcome" message, if we are instructed to. Optionally dispense with the
  // shadow.
  if ( mShowWelcomeText )
  {
    if( mWelcomeTextPosition == TQPoint( 0, 0 ) )
    {
      mWelcomeTextPosition = TQPoint( (screen.width()/2) - fmet.width() - 25,
              (screen.height()/2) - (fmet.height()/2) + fmet.height() );
    }
  }

  if( mShowWelcomeText )
  {
    if( mShowWelcomeTextShadow )
    {
      p.setPen( mWelcomeTextShadowColor );
      p.drawText( mWelcomeTextPosition+TQPoint(2,2), mWelcomeText );
    }
    p.setPen( mWelcomeTextColor );
    p.drawText( mWelcomeTextPosition, mWelcomeText );
  }

  // The current theme wants to say something in particular, rather than display the
  // account's fullname.
  KUser user;
  TQString greetingString = ( !mUsernameText.isNull() ) ? mUsernameText : user.fullName();
  // when we use KUser (system account data) we should also check KEMailSettings (e-mail settings and kcm_useraccount)
  // people often write real names only in e-mail settings
  if ( greetingString.isEmpty() )
  {
    KEMailSettings kes;
    greetingString = kes.getSetting( KEMailSettings::RealName );
  }

  // Try to load the user's KDM icon... TODO: Make this overridable by the Theme.
  if( mUseKdmUserIcon )
  {
    const TQString defSys( ".default.face.icon" );  // The system-wide default image
    const int fAdminOnly  = 1;
    const int fAdminFirst = fAdminOnly+1;
    const int fUserFirst  = fAdminFirst+1;
    const int fUserOnly   = fUserFirst+1;

    int faceSource = fAdminOnly;
    KConfig *kdmconfig = new KConfig("kdm/kdmrc", true);
    kdmconfig->setGroup("X-*-Greeter");
    TQString userPicsDir = kdmconfig->readEntry( "FaceDir", KGlobal::dirs()->resourceDirs("data").last() + "kdm/faces" ) + '/';
    TQString fs = kdmconfig->readEntry( "FaceSource" );
    if (fs == TQString::fromLatin1("UserOnly"))
      faceSource = fUserOnly;
    else if (fs == TQString::fromLatin1("PreferUser"))
      faceSource = fUserFirst;
    else if (fs == TQString::fromLatin1("PreferAdmin"))
      faceSource = fAdminFirst;
    else
      faceSource = fAdminOnly; // Admin Only
    delete kdmconfig;

    TQPixmap userp;
    if ( faceSource == fAdminFirst )
    {
      // If the administrator's choice takes preference
      userp = TQPixmap( userPicsDir + user.loginName() + ".face.icon" );
      if ( userp.isNull() )
        faceSource = fUserOnly;
    }
    if ( faceSource >= fUserFirst)
    {
      // If the user's choice takes preference
      userp = TQPixmap( user.homeDir() + "/.face.icon" );
      if ( userp.isNull() && faceSource == fUserFirst ) // The user has no face, should we check for the admin's setting?
        userp = TQPixmap( userPicsDir + user.loginName() + ".face.icon" );
      if ( userp.isNull() )
        userp = TQPixmap( userPicsDir + defSys );
    }
    else if ( faceSource <= fAdminOnly )
    {
      // Admin only
      userp = TQPixmap( userPicsDir + user.loginName() + ".face.icon" );
      if ( userp.isNull() )
        userp = TQPixmap( userPicsDir + defSys );
    }
    if( !userp.isNull() )
      mPixmap = userp;
  }

  if( mShowIcon )
  {
    TQPoint pos = mIconPosition;
    if( pos == TQPoint( 0, 0 ) )
    {
      pos = TQPoint( (screen.width()/2) + 10, (screen.height()/2) );
    }
    p.drawPixmap( pos, mPixmap );
  }

  // User name font. Leave this nailed-up for now.
  f = mUsernameFont;
  p.setFont( f );
  met = TQFontMetrics( f );
  fmet = met.size( 0L, greetingString );

  if( mShowUsernameText )
  {
    TQPoint pos = mUsernameTextPosition;
    if( pos == TQPoint( 0, 0 ) )
    {
      pos = TQPoint(
              (screen.width()/2) + mPixmap.width() + 20,
              (screen.height()/2) - (fmet.height()/2) + fmet.height()
            );
    }
    p.setPen( mUsernameTextColor );
    p.drawText( pos, greetingString );
  }

  p.end();

  setFixedSize( screen.width(), screen.height() );
  move( screen.topLeft() );
}

void ThemeRedmond::paintEvent( TQPaintEvent *pe )
{
  const TQRect screen = kapp->desktop()->screenGeometry( mTheme->xineramaScreen() );

  TQPainter p;
  p.begin( this );

  TQRect r = pe->rect();

  bitBlt( this, r.x(), r.y(),
          &mImage, r.x(), r.y(), r.width(), r.height() );

  if (mShowActionText)
  {
    p.setPen( mActionTextColor );
    TQFont f = mActionFont;
    p.setFont( f );
    TQFontMetrics met( f );
    TQSize fmet = met.size( 0L, mText );

    mMsgPos = mActionTextPosition;
    if( mMsgPos == TQPoint( 0, 0 ) )
    {
      mMsgPos = TQPoint(
        (screen.width()/2) + mPixmap.width() + 20,
        (screen.height()/2) + (int)(fmet.height()*0.85) + 15
        );
    }
    p.drawText( mMsgPos, mText );
  }
  p.end();
}

void ThemeRedmond::_readSettings()
{
  const TQRect screen = kapp->desktop()->screenGeometry( mTheme->xineramaScreen() );
  //TQRect fullScreen = KGlobalSettings::desktopGeometry(0L);

  if( !mTheme )
    return;
  KConfig *cfg = mTheme->themeConfig();
  if( !cfg )
    return;

  //if( !cfg->hasGroup( TQString("KSplash Theme: %1").arg(mTheme->theme()) ) )
  //  return;
  cfg->setGroup( TQString("KSplash Theme: %1").arg(mTheme->theme()) );

  // Overall appearance
  mBackgroundImage = cfg->readEntry( "Background Image", TQString::null );
  mIcon = cfg->readEntry( "User Icon", "kmenu" );
  mWelcomeText = cfg->readEntry( "Welcome Text", i18n("Welcome") );
  mUsernameText = cfg->readEntry( "Username Text", TQString::null );

  // If any of these are set to (0,0), then we will autoposition the text later (and it _will_
  // be centered on the screen!). The Theme may move this text however the author desires.
  TQPoint absZero( 0, 0 );
  mWelcomeTextPosition  = cfg->readPointEntry( TQString("Welcome Text Position %1").arg(screen.width()), &absZero );
  mUsernameTextPosition = cfg->readPointEntry( TQString("Username Text Position %1").arg(screen.width()), &absZero );
  mActionTextPosition   = cfg->readPointEntry( TQString("Action Text Position %1").arg(screen.width()), &absZero );
  mIconPosition         = cfg->readPointEntry( TQString("Icon Position %1").arg(screen.width()), &absZero );

  // Allow the Theme to hide particular components.
  mShowWelcomeText       = cfg->readBoolEntry( "Show Welcome Text", true );
  mShowWelcomeTextShadow = cfg->readBoolEntry( "Show Welcome Shadow", true );
  mShowUsernameText      = cfg->readBoolEntry( "Show Username", true );
  mShowActionText        = cfg->readBoolEntry( "Show Action", true );
  mShowIcon              = cfg->readBoolEntry( "Show Icon", true );
  mUseKdmUserIcon        = cfg->readBoolEntry( "Use KDM User Icon", true );

  // Setup our fonts. There are only 3 elements which use 'em, so this is fairly
  // straightforward.
  TQFont defaultFont( "Arial", 48, TQFont::Bold );
  defaultFont.setItalic( true );
  TQFont defaultUsernameFont( "Arial", 16, TQFont::Bold );
  TQFont defaultActionFont( "Arial", 12, TQFont::Bold );

  mWelcomeFont       = cfg->readFontEntry( "Welcome Font", &defaultFont );
  mWelcomeFontItalic = cfg->readBoolEntry( "Welcome Font Italic", true );
  mUsernameFont      = cfg->readFontEntry( "Username Font", &defaultUsernameFont );
  mActionFont        = cfg->readFontEntry( "Action Font", &defaultActionFont );

  TQColor defaultDarkColor( 3, 47, 156 );
  TQColor defaultWhiteColor( Qt::white );

  mWelcomeTextColor       = cfg->readColorEntry( "Welcome Text Color", &defaultWhiteColor );
  mWelcomeTextShadowColor = cfg->readColorEntry( "Welcome Shadow Color", &defaultDarkColor );
  mUsernameTextColor      = cfg->readColorEntry( "Username Text Color", &defaultWhiteColor );
  mActionTextColor        = cfg->readColorEntry( "Action Text Color", &defaultWhiteColor );
}
