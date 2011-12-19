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

#include "objkstheme.h"
#include "themeengine.h"
#include "themelegacy.h"
#include "themelegacy.moc"

DefaultConfig::DefaultConfig( TQWidget *parent, KConfig *config )
    :ThemeEngineConfig( parent, config )
{
  mConfig->setGroup( TQString("KSplash Theme: Default") );
  TQVBox *hbox = new TQVBox( this );
  mFlash = new TQCheckBox( i18n("Icons flash while they are starting"), hbox );
  mFlash->setChecked( mConfig->readBoolEntry("Icons Flashing",true) );
  mAlwaysShow = new TQCheckBox( i18n("Always show progress bar"), hbox );
  mAlwaysShow->setChecked( mConfig->readBoolEntry("Always Show Progress",true) );
}

void DefaultConfig::save()
{
  kdDebug() << "DefaultConfig::save()" << endl;
  mConfig->setGroup( TQString("KSplash Theme: Default") );
  mConfig->writeEntry( "Icons Flashing", mFlash->isChecked() );
  mConfig->writeEntry( "Always Show Progress", mAlwaysShow->isChecked() );
  mConfig->sync();
}

#define BIDI 0

ThemeDefault::ThemeDefault( TQWidget *parent, const char *name, const TQStringList &args )
    :ThemeEngine( parent, name, args )
{

  mActivePixmap = mInactivePixmap = 0L;
  mState = 0;

  _readSettings();
  _initUi();

  if( mIconsFlashing )
  {
    mFlashTimer = new TQTimer( this );
    connect( mFlashTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(flash()) );
    mFlashPixmap1 = new TQPixmap();
    mFlashPixmap2 = new TQPixmap();

  }
  else
  {
    mFlashTimer = 0L;
    mFlashPixmap1 = 0L;
    mFlashPixmap2 = 0L;
  }
}

ThemeDefault::~ThemeDefault()
{
    delete mFlashPixmap1;
    delete mFlashPixmap2;
}

void ThemeDefault::_initUi()
{
  TQString resource_prefix;

  TQVBox *vbox = new TQVBox( this );
  vbox->setBackgroundMode(NoBackground);


  TQString activePix, inactivePix;
#if BIDI
  if ( TQApplication::reverseLayout() )
  {
      activePix = _findPicture(TQString("splash_active_bar_bidi.png"));
      inactivePix = _findPicture(TQString("splash_inactive_bar_bidi.png"));
  }
  else
#endif
  {
    activePix = _findPicture(TQString("splash_active_bar.png"));
    inactivePix = _findPicture(TQString("splash_inactive_bar.png"));
  }
  kdDebug() << "Inactive pixmap: " << inactivePix << endl;
  kdDebug() << "Active pixmap:   " <<   activePix << endl;

  mActivePixmap = new TQPixmap( activePix );
  mInactivePixmap = new TQPixmap( inactivePix );

  if (mActivePixmap->isNull())
  {
    mActivePixmap->resize(200,100);
    mActivePixmap->fill(Qt::blue);
  }
  if (mInactivePixmap->isNull())
  {
    mInactivePixmap->resize(200,100);
    mInactivePixmap->fill(Qt::black);
  }

  TQPixmap tlimage( _findPicture(TQString("splash_top.png")) );
  if (tlimage.isNull())
  {
    tlimage.resize(200,100);
    tlimage.fill(Qt::blue);
  }
  TQLabel *top_label = new TQLabel( vbox );
  top_label->setPixmap( tlimage );
  top_label->setFixedSize( tlimage.width(), tlimage.height() );
  top_label->setBackgroundMode(NoBackground);

  mBarLabel = new TQLabel( vbox );
  mBarLabel->setPixmap(*mInactivePixmap);
  mBarLabel->setBackgroundMode(NoBackground);

  TQPixmap blimage( _findPicture(TQString("splash_bottom.png")) );
  if (blimage.isNull())
  {
    blimage.resize(200,100);
    blimage.fill(Qt::black);
  }
  TQLabel *bottom_label = new TQLabel( vbox );
  bottom_label->setPaletteBackgroundPixmap( blimage );

  
      mLabel = new TQLabel( bottom_label );
      mLabel->setBackgroundOrigin( TQWidget::ParentOrigin );
      mLabel->setPaletteForegroundColor( mLabelForeground );
      mLabel->setPaletteBackgroundPixmap( blimage );
      TQFont f(mLabel->font());
      f.setBold(TRUE);
      mLabel->setFont(f);

      mProgressBar = new KProgress( mLabel );
      int h, s, v;
      mLabelForeground.getHsv( &h, &s, &v );
      mProgressBar->setPalette( TQPalette( v > 128 ? black : white ));
      mProgressBar->setBackgroundOrigin( TQWidget::ParentOrigin );
      mProgressBar->setPaletteBackgroundPixmap( blimage );

      bottom_label->setFixedWidth( QMAX(blimage.width(),tlimage.width()) );
      bottom_label->setFixedHeight( mLabel->sizeHint().height()+4 );

      // 3 pixels of whitespace between the label and the progressbar.
      mLabel->resize( bottom_label->width(), bottom_label->height() );

      mProgressBar->setFixedSize( 120, mLabel->height() );
      
      if (TQApplication::reverseLayout()){
            mProgressBar->move( 2, 0 );   
//	    mLabel->move( mProgressBar->width() + 4, 0);
      }
      else{
            mProgressBar->move( bottom_label->width() - mProgressBar->width() - 4, 0);
	    mLabel->move( 2, 0 );
      }
      
      mProgressBar->hide();
  
  setFixedWidth( mInactivePixmap->width() );
  setFixedHeight( mInactivePixmap->height() +
                  top_label->height() + bottom_label->height() );

  const TQRect rect = kapp->desktop()->screenGeometry( mTheme->xineramaScreen() );
  // KGlobalSettings::splashScreenDesktopGeometry(); cannot be used here.
  // kdDebug() << "ThemeDefault::_initUi" << rect << endl;

  move( rect.x() + (rect.width() - size().width())/2,
        rect.y() + (rect.height() - size().height())/2 );
}

// Attempt to find overrides elsewhere?
void ThemeDefault::_readSettings()
{
  if( !mTheme )
    return;

  KConfig *cfg = mTheme->themeConfig();
  if( !cfg )
    return;

  cfg->setGroup( TQString("KSplash Theme: %1").arg(mTheme->theme()) );

  mIconsFlashing = cfg->readBoolEntry( "Icons Flashing", true );
  TQColor df(Qt::white);
  mLabelForeground = cfg->readColorEntry( "Label Foreground", &df );
}

/*
 * ThemeDefault::slotUpdateState(): IF in Default mode, THEN adjust the bar
 * pixmap label. Whee, phun!
 *
 * A similar method exists in the old KSplash.
 */
void ThemeDefault::slotUpdateState()
{
  if( mState > 8 )
    mState = 8;

  if( mIconsFlashing )
  {

    *mFlashPixmap1 = updateBarPixmap( mState );
    *mFlashPixmap2 = updateBarPixmap( mState+1 );
    mBarLabel->setPixmap(*mFlashPixmap2);
    mFlashTimer->stop();

    if( mState < 8 )
      mFlashTimer->start(400);
  }
  else
    mBarLabel->setPixmap( updateBarPixmap( mState ) );

  mState++;
}

/*
 * ThemeDefault::updateBarPixmap(): IF in Default mode, THEN adjust the
 * bar pixmap to reflect the current state. WARNING! KSplash Default
 * does NOT support our "Restoring Session..." state. We will need
 * to reflect that somehow.
 */
TQPixmap ThemeDefault::updateBarPixmap( int state )
{
  int offs;

  TQPixmap x;
  if( !mActivePixmap ) return( x );
#if BIDI


  if( TQApplication::reverseLayout() )
    {
      if ( state > 7 ) 
	return (  x );
    }
#endif

  offs = state * 58;
  if (state == 3)
    offs += 8;
  else if (state == 6)
    offs -= 8;

  TQPixmap tmp(*mActivePixmap);
  TQPainter p(&tmp);
#if BIDI
  if ( TQApplication::reverseLayout() )
    p.drawPixmap(0, 0, *mInactivePixmap, 0, 0, tmp.width()-offs );
  else
#endif
    p.drawPixmap(offs, 0, *mInactivePixmap, offs, 0);
  return tmp ;
}

void ThemeDefault::flash()
{
  if( !mIconsFlashing )
    return;
  TQPixmap *swap = mFlashPixmap1;
  mFlashPixmap1 = mFlashPixmap2;
  mFlashPixmap2 = swap;
  mBarLabel->setPixmap(*mFlashPixmap2);
}

TQString ThemeDefault::_findPicture( const TQString &pic )
{
  // Don't use ObjKsTheme::locateThemeData here for compatibility reasons.
  TQString f = pic;
  if (mTheme->loColor())
    f = TQString("locolor/")+f;
  //kdDebug() << "Searching for " << f << endl;
  //kdDebug() << "Theme directory: " << mTheme->themeDir() << endl;
  //kdDebug() << "Theme name:      " << mTheme->theme() << endl;
  TQString p = TQString::null;
  if ((p = locate("appdata",mTheme->themeDir()+f)).isEmpty())
    if ((p = locate("appdata",mTheme->themeDir()+"pics/"+f)).isEmpty())
      if ((p = locate("appdata", TQString("pics/")+mTheme->theme()+"/"+f)).isEmpty())
        if ((p = locate("appdata",f)).isEmpty())
          if ((p = locate("appdata",TQString("pics/")+f)).isEmpty())
            if ((p = locate("data",TQString("pics/")+f)).isEmpty()) {
              ; // No more places to search
            }
  return p;
}
