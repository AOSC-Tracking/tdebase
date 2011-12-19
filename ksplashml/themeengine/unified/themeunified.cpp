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

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kprogress.h>

#include <tqcheckbox.h>
#include <tqdesktopwidget.h>
#include <tqlabel.h>
#include <tqpainter.h>
#include <tqpixmap.h>
#include <tqwidget.h>
#include <tqtimer.h>
#include <tqcursor.h>

#include "objkstheme.h"
#include "themeengine.h"
#include "themeunified.h"
#include "themeunified.moc"

UnifiedConfig::UnifiedConfig( TQWidget *parent, KConfig *config )
    :ThemeEngineConfig( parent, config )
{
  mConfig->setGroup( TQString("KSplash Theme: Default") );
  TQVBox *hbox = new TQVBox( this );
  mAlwaysShow = new TQCheckBox( i18n("Always show progress bar"), hbox );
  mAlwaysShow->setChecked( mConfig->readBoolEntry("Always Show Progress",true) );
}

void UnifiedConfig::save()
{
  kdDebug() << "UnifiedConfig::save()" << endl;
  mConfig->setGroup( TQString("KSplash Theme: Default") );
  mConfig->writeEntry( "Always Show Progress", mAlwaysShow->isChecked() );
  mConfig->sync();
}

ThemeUnified::ThemeUnified( TQWidget *parent, const char *name, const TQStringList &args )
    :ThemeEngine( parent, name, args )
{

  mState = 0;

  _readSettings();
  _initUi();
}

ThemeUnified::~ThemeUnified()
{
  if (mSysModalDialog) {
    KSMModalDialog* temp = mSysModalDialog;
    mSysModalDialog = NULL;
    temp->closeSMDialog();
  }
}

void ThemeUnified::_initUi()
{
  resize(10,10);

  mSysModalDialog = new KSMModalDialog(this);
  mSysModalDialog->setStatusMessage(i18n("Trinity is starting up").append("..."));
  mSysModalDialog->show();
  mSysModalDialog->setActiveWindow();

  const TQRect rect = kapp->desktop()->screenGeometry( mTheme->xineramaScreen() );

  // Center the dialog
  TQSize sh = sizeHint();
  TQRect rect1 = KGlobalSettings::desktopGeometry(TQCursor::pos());
  move(rect1.x() + (rect1.width() - sh.width())/2, rect1.y() + (rect1.height() - sh.height())/2);
}

// Attempt to find overrides elsewhere?
void ThemeUnified::_readSettings()
{
  if( !mTheme )
    return;

  KConfig *cfg = mTheme->themeConfig();
  if( !cfg )
    return;

  cfg->setGroup( TQString("KSplash Theme: %1").arg(mTheme->theme()) );

  TQColor df(Qt::white);
  mLabelForeground = cfg->readColorEntry( "Label Foreground", &df );
}

/*
 * ThemeUnified::slotUpdateState(): IF in Default mode, THEN adjust the bar
 * pixmap label. Whee, phun!
 *
 * A similar method exists in the old KSplash.
 */
void ThemeUnified::slotUpdateState()
{
  if( mState > 8 )
    mState = 8;

//   mBarLabel->setPixmap( updateBarPixmap( mState ) );

  mState++;
}
