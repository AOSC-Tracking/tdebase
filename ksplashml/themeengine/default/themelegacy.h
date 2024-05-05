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

#ifndef __THEMELEGACY_H__
#define __THEMELEGACY_H__

#include <kprogress.h>

#include <tqlabel.h>
#include <tqwidget.h>

#include "themeengine.h"

static const int MAX_ICONS=7;

class TQPixmap;
class TQTimer;

class TQCheckBox;

class DefaultConfig: public ThemeEngineConfig
{
  TQ_OBJECT
public:
  DefaultConfig( TQWidget *, TDEConfig * );
  void save();
protected:
  TQCheckBox *mFlash, *mAlwaysShow;
};

/**
 * @short Traditional Trinity splash screen.
 */
class ObjKsTheme;
class TDE_EXPORT ThemeDefault : public ThemeEngine
{
  TQ_OBJECT
public:
  ThemeDefault( TQWidget *, const char *, const TQStringList& );
   virtual ~ThemeDefault();

  inline const DefaultConfig *config( TQWidget *p, TDEConfig *c )
  {
    return new DefaultConfig( p, c );
  };

  static TQStringList names()
  {
    TQStringList Names;
    Names << "Default";
    Names << "Classic";
    Names << "Klassic";
    return( Names );
  }

public slots:
  inline void slotSetText( const TQString& s )
  {
    if( mLabel )
      mLabel->setText( s );
    slotUpdateState();
  };
  inline void slotUpdateSteps( int s )
  {
    mProgressBar->show();
    mProgressBar->setTotalSteps( s );
  }
  inline void slotUpdateProgress( int i )
  {
    mProgressBar->setProgress( i );
  }


private slots:
  void slotUpdateState();
  TQPixmap updateBarPixmap( int );
  void flash();

private:
  void _initUi();
  void _readSettings();
  TQString _findPicture( const TQString &pic );

  // Configurable Options
  bool    mIconsFlashing;
  TQColor mLabelForeground;
  int     mIconOffsets[MAX_ICONS];

  // Internals.
  KProgress *mProgressBar;
  TQLabel *mLabel, *mBarLabel;
  TQPixmap *mActivePixmap, *mInactivePixmap;
  int mState;
  TQTimer *mFlashTimer;
  TQPixmap *mFlashPixmap1, *mFlashPixmap2;
};

#endif
