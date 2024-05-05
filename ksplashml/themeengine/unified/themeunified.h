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

#ifndef __THEMEUNIFIED_H__
#define __THEMEUNIFIED_H__

#include <kprogress.h>

#include <tqlabel.h>
#include <tqwidget.h>

#include <kdialog.h>

#include "themeengine.h"
class TQPixmap;
class TQTimer;

class TQCheckBox;

class UnifiedConfig: public ThemeEngineConfig
{
  TQ_OBJECT
public:
  UnifiedConfig( TQWidget *, TDEConfig * );
  void save();
protected:
  TQCheckBox *mAlwaysShow;
};

/**
 * @short Traditional Trinity splash screen.
 */
class ObjKsTheme;
class TDE_EXPORT ThemeUnified : public ThemeEngine
{
  TQ_OBJECT
public:
  ThemeUnified( TQWidget *, const char *, const TQStringList& );
   virtual ~ThemeUnified();

  inline const UnifiedConfig *config( TQWidget *p, TDEConfig *c )
  {
    return new UnifiedConfig( p, c );
  };

  static TQStringList names()
  {
    TQStringList Names;
    Names << "Unified";
    return( Names );
  }

public slots:
  inline void slotSetText( const TQString& s )
  {
      close();
      if (mSysModalDialog) mSysModalDialog->setStatusMessage(TQString(s).append("..."));
  }
  inline void slotSetTextIndex( const int i )
  {
      if (i == 3) {
          if (mSysModalDialog) {
              KSMModalDialog* temp = mSysModalDialog;
              mSysModalDialog = NULL;
              temp->closeSMDialog();
          }
      }
  }
  inline void slotUpdateSteps( int s )
  {
  }
  inline void slotUpdateProgress( int i )
  {
  }


private slots:
  void slotUpdateState();

private:
  void _initUi();
  void _readSettings();

  // Configurable Options
  TQColor mLabelForeground;

  // Internals.
  KSMModalDialog* mSysModalDialog;
  int mState;
};

#endif
