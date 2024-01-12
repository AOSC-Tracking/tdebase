/***********************************************************************
 *
 *  Kfind.cpp
 *
 * This is KFind, released under GPL
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 * KFind (c) 1998-2003 The KDE Developers
  Martin Hartig
  Stephan Kulow <coolo@kde.org>
  Mario Weilguni <mweilguni@sime.com>
  Alex Zepeda <zipzippy@sonic.net>
  Miroslav Flídr <flidr@kky.zcu.cz>
  Harri Porten <porten@kde.org>
  Dima Rogozin <dima@mercury.co.il>
  Carsten Pfeiffer <pfeiffer@kde.org>
  Hans Petter Bieker <bieker@kde.org>
  Waldo Bastian <bastian@kde.org>
  Beppe Grimaldi <grimalkin@ciaoweb.it>
  Eric Coquelle <coquelle@caramail.com>

 **********************************************************************/

#include <kpushbutton.h>
#include <tqlayout.h>
#include <tqvbox.h>

#include <kdialog.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <kseparator.h>
#include <tqlineedit.h>
#include <tqcheckbox.h>
#include <kstdguiitem.h>

#include "kftabdlg.h"
#include "kquery.h"

#include "kfind.moc"

Kfind::Kfind(TQWidget *parent, const char *name)
  : TQWidget( parent, name )
{
  kdDebug() << "Kfind::Kfind " << this << endl;
  TQBoxLayout * mTopLayout = new TQBoxLayout( this, TQBoxLayout::LeftToRight,
                                            KDialog::marginHint(), KDialog::spacingHint() );

  // create tabwidget
  tabWidget = new KfindTabWidget( this );
  mTopLayout->addWidget(tabWidget);

  /*
   * This is ugly.  Might be a KSeparator bug, but it makes a small black
   * pixel for me which is visually distracting (GS).
  // create separator
  KSeparator * mActionSep = new KSeparator( this );
  mActionSep->setFocusPolicy( TQWidget::ClickFocus );
  mActionSep->setOrientation( TQFrame::VLine );
  mTopLayout->addWidget(mActionSep);
  */

  // create button box
  TQVBox * mButtonBox = new TQVBox( this );
  TQVBoxLayout *lay = (TQVBoxLayout*)mButtonBox->layout();
  lay->addStretch(1);
  mTopLayout->addWidget(mButtonBox);

  mSearch = new KPushButton( KGuiItem(i18n("&Find"), "edit-find"), mButtonBox );
  mButtonBox->setSpacing( (tabWidget->sizeHint().height()-4*mSearch->sizeHint().height()) / 4);
  connect( mSearch, TQ_SIGNAL(clicked()), this, TQ_SLOT( startSearch() ) );
  mStop = new KPushButton( KGuiItem(i18n("Stop"), "process-stop"), mButtonBox );
  connect( mStop, TQ_SIGNAL(clicked()), this, TQ_SLOT( stopSearch() ) );
  mSave = new KPushButton( KStdGuiItem::saveAs(), mButtonBox );
  connect( mSave, TQ_SIGNAL(clicked()), this, TQ_SLOT( saveResults() ) );

  KPushButton * mClose = new KPushButton( KStdGuiItem::close(), mButtonBox );
  connect( mClose, TQ_SIGNAL(clicked()), this, TQ_SIGNAL( destroyMe() ) );

  // react to search requests from widget
  connect( tabWidget, TQ_SIGNAL(startSearch()), this, TQ_SLOT( startSearch() ) );

  mSearch->setEnabled(true); // Enable "Search"
  mStop->setEnabled(false);  // Disable "Stop"
  mSave->setEnabled(false);  // Disable "Save..."

  dirlister=new KDirLister();
}

Kfind::~Kfind()
{
  stopSearch();
  dirlister->stop();
  delete dirlister;
  kdDebug() << "Kfind::~Kfind" << endl;
}

void Kfind::setURL( const KURL &url )
{
  tabWidget->setURL( url );
}

void Kfind::startSearch()
{
  tabWidget->setQuery(query);
  emit started();

  //emit resultSelected(false);
  //emit haveResults(false);

  mSearch->setEnabled(false); // Disable "Search"
  mStop->setEnabled(true);  // Enable "Stop"
  mSave->setEnabled(false);  // Disable "Save..."

  tabWidget->beginSearch();

  dirlister->openURL(KURL(tabWidget->dirBox->currentText().stripWhiteSpace()));

  query->start();
}

void Kfind::stopSearch()
{
  // will call KFindPart::slotResult, which calls searchFinished here
  query->kill();
}

/*
void Kfind::newSearch()
{
  // WABA: Not used any longer?
  stopSearch();

  tabWidget->setDefaults();

  emit haveResults(false);
  emit resultSelected(false);

  setFocus();
}
*/

void Kfind::searchFinished()
{
  mSearch->setEnabled(true); // Enable "Search"
  mStop->setEnabled(false);  // Disable "Stop"
  // ## TODO mSave->setEnabled(true);  // Enable "Save..."

  tabWidget->endSearch();
  setFocus();
}


void Kfind::saveResults()
{
  // TODO
}

void Kfind::setFocus()
{
  tabWidget->setFocus();
}

void Kfind::saveState( TQDataStream *stream )
{
  query->kill();
  *stream << tabWidget->nameBox->currentText();
  *stream << tabWidget->dirBox->currentText();
  *stream << tabWidget->typeBox->currentItem();
  *stream << tabWidget->textEdit->text();
  *stream << (int)( tabWidget->subdirsCb->isChecked() ? 0 : 1 );
}

void Kfind::restoreState( TQDataStream *stream )
{
  TQString namesearched, dirsearched,containing;
  int typeIdx;
  int subdirs;
  *stream >> namesearched;
  *stream >> dirsearched;
  *stream >> typeIdx;
  *stream >> containing;
  *stream >> subdirs;
  tabWidget->nameBox->insertItem( namesearched, 0);
  tabWidget->dirBox->insertItem ( dirsearched, 0);
  tabWidget->typeBox->setCurrentItem(typeIdx);
  tabWidget->textEdit->setText ( containing );
  tabWidget->subdirsCb->setChecked( ( subdirs==0 ? true : false ));
}
