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
#include <tdecmdlineargs.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdestandarddirs.h>

#include <tqcolor.h>
#include <tqcursor.h>
#include <tqdesktopwidget.h>
#include <tqfont.h>
#include <tqpixmap.h>
#include <tqrect.h>
#include <tqstring.h>

#include "objkstheme.h"
#include "objkstheme.moc"

ObjKsTheme::ObjKsTheme( const TQString& theme )
  :mActiveTheme (theme), mThemeDir("/"), mThemeConfig (0L), mThemePrefix( "Themes/" ), d(0)
{
  // Get Xinerama config.
  TDEConfig *config = tdeApp->config();
  config->setGroup( "Xinerama" );
  TQDesktopWidget *desktop = tdeApp->desktop();
  mXineramaScreen = config->readNumEntry("KSplashScreen", desktop->primaryScreen());

  // For Xinerama, let's put the mouse on the first head.  Otherwise it could appear anywhere!
  if (desktop->isVirtualDesktop() && mXineramaScreen != -2)
  {
    TQRect rect = desktop->screenGeometry( mXineramaScreen );
    if (!rect.contains(TQCursor::pos()))
      TQCursor::setPos(rect.center());
  }

  // Does the active theme exist?
  if( !loadThemeRc( mActiveTheme, false ) )
    if( !loadLocalConfig( mActiveTheme, false ) )
      if( !loadThemeRc( "Default", false ) )
        loadLocalConfig( "Default", true ); //force: we need some defaults
  loadCmdLineArgs(TDECmdLineArgs::parsedArgs());
  mThemePrefix += ( mActiveTheme + "/" );
}

ObjKsTheme::~ObjKsTheme()
{
}

bool ObjKsTheme::loadThemeRc( const TQString& activeTheme, bool force )
{
  //kdDebug() << "ObjKsTheme::loadThemeRc: " << activeTheme << endl;
  TQString prefix("Themes/");
  TQString themeFile;
  TDEConfig *cf = 0L;

  // Try our best to find a theme file.
  themeFile = locate( "appdata", prefix + activeTheme + "/" + TQString("Theme.rc") );
  themeFile = themeFile.isEmpty() ? locate("appdata",prefix+activeTheme+"/"+TQString("Theme.RC")):themeFile;
  themeFile = themeFile.isEmpty() ? locate("appdata",prefix+activeTheme+"/"+TQString("theme.rc")):themeFile;
  themeFile = themeFile.isEmpty() ? locate("appdata",prefix+activeTheme+"/"+activeTheme+TQString(".rc")):themeFile;

  if( !themeFile.isEmpty() )
     cf = new TDEConfig( themeFile );

  if( cf )
  {
    mActiveTheme = activeTheme;
    mThemeDir = prefix + activeTheme+"/";
    if( loadTDEConfig( cf, activeTheme, force ) )
    {
      mThemeConfig = cf;
      return true;
    }
    else
      delete cf;
  }
  return false;
}

bool ObjKsTheme::loadLocalConfig( const TQString& activeTheme, bool force )
{
  //kdDebug() << "ObjKsTheme::loadLocalConfig" << endl;
  TDEConfig *cfg = tdeApp->config();
  return( loadTDEConfig( cfg, activeTheme, force ) );
}

// ObjKsConfig::loadTDEConfig(): Load our settings from a TDEConfig object.
bool ObjKsTheme::loadTDEConfig( TDEConfig *cfg, const TQString& activeTheme, bool force )
{
  //kdDebug() << "ObjKsTheme::loadTDEConfig" << endl;
  if( !cfg )
    return false;

  // Themes are always stored in the group [KSplash Theme: ThemeName],
  // and ThemeName should always be the same name as the themedir, if any.
  // If we can't find this theme group, then we can't load.
  if( !cfg->hasGroup( TQString("KSplash Theme: %1").arg(activeTheme) ) && !force )
    return false;

  cfg->setGroup( TQString("KSplash Theme: %1").arg(activeTheme) );
  mThemeConfig = cfg;

  mThemeEngine = cfg->readEntry( "Engine", "Default" );

  m_icons.clear();
  m_icons.append( cfg->readEntry( "Icon1", "filetypes" ) );
  m_icons.append( cfg->readEntry( "Icon2", "application-x-executable" ) );
  m_icons.append( cfg->readEntry( "Icon3", "key_bindings" ) );
  m_icons.append( cfg->readEntry( "Icon4", "window_duplicate" ) );
  m_icons.append( cfg->readEntry( "Icon5", "desktop" ) );
  m_icons.append( cfg->readEntry( "Icon6", "style" ) );
  m_icons.append( cfg->readEntry( "Icon7", "preferences-system" ) );
  m_icons.append( cfg->readEntry( "Icon8", "go" ) );

  m_text.clear();
  m_text.append( cfg->readEntry( "Message1", i18n("Setting up interprocess communication") ) );
  m_text.append( cfg->readEntry( "Message2", i18n("Initializing system services") ) );
  m_text.append( cfg->readEntry( "Message3", i18n("Initializing peripherals") ) );
  m_text.append( cfg->readEntry( "Message4", i18n("Loading the window manager") ) );
  m_text.append( cfg->readEntry( "Message5", i18n("Loading the desktop") ) );
  m_text.append( cfg->readEntry( "Message6", i18n("Loading the panel") ) );
  m_text.append( cfg->readEntry( "Message7", i18n("Restoring session") ) );
  m_text.append( cfg->readEntry( "Message8", i18n("Trinity is up and running") ) );

  return true;
}

/*
 * ObjKsTheme::loadCmdLineArgs(): Handle any overrides which the user might have
 * specified.
 */
void ObjKsTheme::loadCmdLineArgs( TDECmdLineArgs *args )
{

  mManagedMode = args->isSet( "managed" );
  mTesting = args->isSet("test");
  mLoColor = ( TQPixmap::defaultDepth() <= 8 );
  TQString theme = args->getOption( "theme" );
  if( theme != mActiveTheme && !theme.isNull() )
    if( loadThemeRc( theme, false ) )
      mActiveTheme = theme;
  //args->clear();
}

TQString ObjKsTheme::locateThemeData( const TQString &resource )
{
  if ( !mLoColor )
    return locate( "appdata", mThemePrefix+resource );
  else
  {
    TQString res = locate( "appdata", mThemePrefix+"locolor/"+resource );
    if ( res.isEmpty() )
      res = locate( "appdata", mThemePrefix+resource );
    return res;
  }
}
