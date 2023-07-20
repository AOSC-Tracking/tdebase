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

#ifndef __THEMEREDMOND_H__
#define __THEMEREDMOND_H__

#include <kdebug.h>
#include <kpixmap.h>

#include <themeengine.h>

class TDEFontCombo;
class TQCheckBox;

class CfgRedmond: public ThemeEngineConfig
{
  TQ_OBJECT
public:
  CfgRedmond( TQWidget *, TDEConfig * );

protected:
  TQCheckBox *mShowUsername;
  TQCheckBox *mShowIcon;
  TQCheckBox *mShowWelcome;
  TDEFontCombo *mWelcomeFont;
  TDEFontCombo *mUsernameFont;
  TDEFontCombo *mActionFont;
};

class ObjKsTheme;
class ThemeRedmond: public ThemeEngine
{
  TQ_OBJECT
public:
  ThemeRedmond( TQWidget *, const char *, const TQStringList& );

  inline const TQString name() { return( TQString("Redmond") );  }
  static TQStringList names()
  {
    TQStringList Names;
    Names << "Redmond";
    return( Names );
  };

public slots:
  inline void slotSetText( const TQString& s )
  {
    if( mText != s )
    {
      mText = s;
      repaint( false );
    }
  };

private:
  void paintEvent( TQPaintEvent * );

  void _initUi();
  void _readSettings();

  TQString mText;
  TQPixmap mPixmap;
  bool mRedrawKonqi;
  TQPoint mMsgPos;
  KPixmap mImage;

  // ThemeEngine configuration.
  bool mShowWelcomeText;
  bool mShowWelcomeTextShadow;
  bool mWelcomeFontItalic;
  bool mShowUsernameText;
  bool mShowActionText;
  bool mShowIcon;
  bool mUseKdmUserIcon;
  TQString mBackgroundImage;
  TQString mWelcomeText;
  TQString mUsernameText; // Leave this undefined to autodetect the username.
  TQString mIcon;
  TQFont mWelcomeFont;
  TQFont mUsernameFont;
  TQFont mActionFont;
  TQColor mWelcomeTextColor;
  TQColor mWelcomeTextShadowColor;
  TQColor mUsernameTextColor;
  TQColor mActionTextColor;
  TQPoint mWelcomeTextPosition; // Set this to (0,0) to autoposition the text.
  TQPoint mUsernameTextPosition; // Likewise.
  TQPoint mActionTextPosition; // Likewise likewise.
  TQPoint mIconPosition; // ...

}
;

#endif
