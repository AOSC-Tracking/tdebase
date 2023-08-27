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

#ifndef __WNDMAIN_H__
#define __WNDMAIN_H__

#include <tdeapplication.h>

#include <tqptrlist.h>
#include <tqstring.h>
#include <tqobject.h>

#include "ksplashiface.h"

// MAKE SURE THAT THIS MATCHES WHAT'S IN ../kcmksplash/kcmksplash.h!!!
#define N_ACTIONITEMS 8

// Action: This represents an "action entry" to any object which is interested
// in knowing this.
typedef struct
{
  TQString ItemPixmap;
  TQString ItemText;
} Action;

class WndStatus;
class ObjKsTheme;
class ThemeEngine;
class TDEConfig;

class KSplash: public TQWidget, virtual public KSplashIface
{
  TQ_OBJECT
  

public:
  KSplash(const char *name = "ksplash");
  ~KSplash();

  TQPtrList<Action> actionList();

  // DCOP interface
  ASYNC upAndRunning( TQString );
  ASYNC setMaxProgress(int);
  ASYNC setProgress(int);
  ASYNC setStartupItemCount( int count );
  ASYNC programStarted( TQString programIcon, TQString programName, TQString description );
  ASYNC startupComplete();
  ASYNC show();
  ASYNC hide();
  ASYNC close();

signals:
  void stepsChanged(int);
  void progressChanged(int);
  void actionListChanged();

protected:
  bool eventFilter( TQObject *o, TQEvent *e );

public slots:
  void slotUpdateSteps( int );
  void slotUpdateProgress( int );

private slots:
  void initDcop();
  void prepareIconList();
  void prepareSplashScreen();
  void slotExec();
  void nextIcon();
  void slotInsertAction( const TQString&, const TQString& );
  void slotReadProperties( TDEConfig * );

  void slotSetText( const TQString& );
  void slotSetTextIndex( const int );
  void slotSetPixmap( const TQString& );

  void loadTheme( const TQString& );

private:
  ThemeEngine *_loadThemeEngine( const TQString& pluginName, const TQString& theme );
  void updateState( unsigned int state );

protected:
  unsigned int mState;
  unsigned int mMaxProgress;
  unsigned int mStep; // ??
  TQTimer* close_timer;

  bool mSessMgrCalled;
  bool mTimeToGo;

  TQString mTheme;
  ObjKsTheme *mKsTheme;

  ThemeEngine *mThemeEngine;
  TQPtrList<Action> mActionList;
  Action *mCurrentAction, *mPreviousAction;

  TQString mThemeLibName;
};

#endif
