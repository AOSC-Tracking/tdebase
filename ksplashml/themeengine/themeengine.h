/***************************************************************************
 *   Copyright Brian Ledbetter 2001-2003 <brian@shadowcom.net>             *
 *   Copyright Ravikiran Rajagopal 2003  <ravi@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation. (The original KSplash/ML   *
 *   codebase (upto version 0.95.3) is BSD-licensed.)                      *
 *                                                                         *
 ***************************************************************************/

#ifndef THEMEENGINE_H
#define THEMEENGINE_H

#include <tqstringlist.h>
#include <tqvbox.h>
#include <tqwidget.h>

#include <kdemacros.h>

class TDEConfig;
class ObjKsTheme;
class TQMouseEvent;

/**
 * @short The base for the ThemeEngine's configuration widget.
 */
class TDE_EXPORT ThemeEngineConfig: public TQVBox
{
  TQ_OBJECT
public:

  ThemeEngineConfig( TQWidget *p, TDEConfig *c )
      :TQVBox( p ), mConfig( c )
  {}

  TDEConfig* config()const { return mConfig; }

public slots:
  virtual void load() {}
  virtual void save() {}

protected:
  TDEConfig *mConfig;
};

/**
 * @short Base class for all theme engines. Member functions need to be
 * overridden by derived classes in order to provide actual functionality.
 */
class TDE_EXPORT ThemeEngine: public TQVBox
{
  TQ_OBJECT
public:
  ThemeEngine( TQWidget *parent, const char *name, const TQStringList &args );
  virtual ~ThemeEngine() = 0;
  virtual const ThemeEngineConfig *config( TQWidget *, TDEConfig * ) { return 0L; }
  virtual ObjKsTheme *ksTheme() { return mTheme; }
  virtual bool eventFilter( TQObject* o, TQEvent* e );

public slots:
  virtual void slotUpdateProgress( int ) {}
  virtual void slotUpdateSteps( int ) {}
  virtual void slotSetText( const TQString& ) {}
  virtual void slotSetTextIndex( const int ) {}
  virtual void slotSetPixmap( const TQString& ) {} // use DesktopIcon() to load this.

protected:
  void addSplashWindow( TQWidget* );

protected:
  ObjKsTheme *mTheme;
  virtual bool x11Event( XEvent* );

private slots:
  void splashWindowDestroyed( TQObject* );

private:
  class ThemeEnginePrivate;
  ThemeEnginePrivate *d;
  bool mUseWM;
};

#endif
