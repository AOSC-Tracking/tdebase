/* This file is part of the KDE libraries
   Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)
   Copyright (C) 1999 David Faure (faure@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqgroupbox.h>
#include <tqcheckbox.h>
#include <tqpushbutton.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kseparator.h>
#include <kapplication.h>
#include <dcopclient.h>

#include "kdebugdialog.h"

KDebugDialog::KDebugDialog( TQStringList areaList, TQWidget *parent, const char *name, bool modal )
  : KAbstractDebugDialog( parent, name, modal )
{
  setCaption(i18n("Debug Settings"));

  TQVBoxLayout *topLayout = new TQVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
  if( topLayout == 0 ) { return; } // can this happen ?

  TQLabel * tmpLabel = new TQLabel( i18n("Debug area:"), this );
  tmpLabel->setFixedHeight( fontMetrics().lineSpacing() );
  topLayout->addWidget( tmpLabel );

  // Build combo of debug areas
  pDebugAreas = new TQComboBox( false, this );
  pDebugAreas->setFixedHeight( pDebugAreas->sizeHint().height() );
  pDebugAreas->insertStringList( areaList );
  topLayout->addWidget( pDebugAreas );

  TQGridLayout *gbox = new TQGridLayout( 2, 2, KDialog::marginHint() );
  if( gbox == 0 ) { return; }
  topLayout->addLayout( TQT_TQLAYOUT(gbox) );

  TQStringList destList;
  destList.append( i18n("File") );
  destList.append( i18n("Message Box") );
  destList.append( i18n("Shell") );
  destList.append( i18n("Syslog") );
  destList.append( i18n("None") );

  //
  // Upper left frame
  //
  pInfoGroup = new TQGroupBox( i18n("Information"), this );
  gbox->addWidget( pInfoGroup, 0, 0 );
  TQVBoxLayout *vbox = new TQVBoxLayout( pInfoGroup, KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pInfoLabel1 = new TQLabel( i18n("Output to:"), pInfoGroup );
  vbox->addWidget( pInfoLabel1 );
  pInfoCombo = new TQComboBox( false, pInfoGroup );
  connect(pInfoCombo, TQT_SIGNAL(activated(int)),
	  this, TQT_SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pInfoCombo );
  pInfoCombo->insertStringList( destList );
  pInfoLabel2 = new TQLabel( i18n("Filename:"), pInfoGroup );
  vbox->addWidget( pInfoLabel2 );
  pInfoFile = new TQLineEdit( pInfoGroup );
  vbox->addWidget( pInfoFile );
  /*
  pInfoLabel3 = new TQLabel( i18n("Show only area(s):"), pInfoGroup );
  vbox->addWidget( pInfoLabel3 );
  pInfoShow = new TQLineEdit( pInfoGroup );
  vbox->addWidget( pInfoShow );
  */

  //
  // Upper right frame
  //
  pWarnGroup = new TQGroupBox( i18n("Warning"), this );
  gbox->addWidget( pWarnGroup, 0, 1 );
  vbox = new TQVBoxLayout( pWarnGroup, KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pWarnLabel1 = new TQLabel( i18n("Output to:"), pWarnGroup );
  vbox->addWidget( pWarnLabel1 );
  pWarnCombo = new TQComboBox( false, pWarnGroup );
  connect(pWarnCombo, TQT_SIGNAL(activated(int)),
	  this, TQT_SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pWarnCombo );
  pWarnCombo->insertStringList( destList );
  pWarnLabel2 = new TQLabel( i18n("Filename:"), pWarnGroup );
  vbox->addWidget( pWarnLabel2 );
  pWarnFile = new TQLineEdit( pWarnGroup );
  vbox->addWidget( pWarnFile );
  /*
  pWarnLabel3 = new TQLabel( i18n("Show only area(s):"), pWarnGroup );
  vbox->addWidget( pWarnLabel3 );
  pWarnShow = new TQLineEdit( pWarnGroup );
  vbox->addWidget( pWarnShow );
  */

  //
  // Lower left frame
  //
  pErrorGroup = new TQGroupBox( i18n("Error"), this );
  gbox->addWidget( pErrorGroup, 1, 0 );
  vbox = new TQVBoxLayout( pErrorGroup, KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pErrorLabel1 = new TQLabel( i18n("Output to:"), pErrorGroup );
  vbox->addWidget( pErrorLabel1 );
  pErrorCombo = new TQComboBox( false, pErrorGroup );
  connect(pErrorCombo, TQT_SIGNAL(activated(int)),
	  this, TQT_SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pErrorCombo );
  pErrorCombo->insertStringList( destList );
  pErrorLabel2 = new TQLabel( i18n("Filename:"), pErrorGroup );
  vbox->addWidget( pErrorLabel2 );
  pErrorFile = new TQLineEdit( pErrorGroup );
  vbox->addWidget( pErrorFile );
  /*
  pErrorLabel3 = new TQLabel( i18n("Show only area(s):"), pErrorGroup );
  vbox->addWidget( pErrorLabel3 );
  pErrorShow = new TQLineEdit( pErrorGroup );
  vbox->addWidget( pErrorShow );
  */

  //
  // Lower right frame
  //
  pFatalGroup = new TQGroupBox( i18n("Fatal Error"), this );
  gbox->addWidget( pFatalGroup, 1, 1 );
  vbox = new TQVBoxLayout( pFatalGroup, KDialog::spacingHint() );
  vbox->addSpacing( fontMetrics().lineSpacing() );
  pFatalLabel1 = new TQLabel( i18n("Output to:"), pFatalGroup );
  vbox->addWidget( pFatalLabel1 );
  pFatalCombo = new TQComboBox( false, pFatalGroup );
  connect(pFatalCombo, TQT_SIGNAL(activated(int)),
	  this, TQT_SLOT(slotDestinationChanged(int)));
  vbox->addWidget( pFatalCombo );
  pFatalCombo->insertStringList( destList );
  pFatalLabel2 = new TQLabel( i18n("Filename:"), pFatalGroup );
  vbox->addWidget( pFatalLabel2 );
  pFatalFile = new TQLineEdit( pFatalGroup );
  vbox->addWidget( pFatalFile );
  /*
  pFatalLabel3 = new TQLabel( i18n("Show only area(s):"), pFatalGroup );
  vbox->addWidget( pFatalLabel3 );
  pFatalShow = new TQLineEdit( pFatalGroup );
  vbox->addWidget( pFatalShow );
  */


  pAbortFatal = new TQCheckBox( i18n("Abort on fatal errors"), this );
  topLayout->addWidget(pAbortFatal);

  topLayout->addStretch();
  KSeparator *hline = new KSeparator( KSeparator::HLine, this );
  topLayout->addWidget( hline );

  buildButtons( topLayout );

  connect( pDebugAreas, TQT_SIGNAL( activated( const TQString &) ),
           TQT_SLOT( slotDebugAreaChanged( const TQString & ) ) );

  // Get initial values ("initial" is understood by the slot)
  slotDebugAreaChanged( "0 initial" );
  slotDestinationChanged(0);

  resize( 300, height() );
}

KDebugDialog::~KDebugDialog()
{
}

void KDebugDialog::slotDebugAreaChanged( const TQString & text )
{
  // Save settings from previous page
  if ( text != "0 initial" ) // except on first call
    save();

  TQString data = text.simplifyWhiteSpace();
  int space = data.find(" ");
  if (space == -1)
      kdError() << "No space:" << data << endl;

  bool longOK;
  unsigned long number = data.left(space).toULong(&longOK);
  if (!longOK)
      kdError() << "The first part wasn't a number : " << data << endl;

  /* Fill dialog fields with values from config data */
  pConfig->setGroup( TQString::number( number ) ); // Group name = debug area code
  pInfoCombo->setCurrentItem( pConfig->readNumEntry( "InfoOutput", 2 ) );
  pInfoFile->setText( pConfig->readPathEntry( "InfoFilename","kdebug.dbg" ) );
  //pInfoShow->setText( pConfig->readEntry( "InfoShow" ) );
  pWarnCombo->setCurrentItem( pConfig->readNumEntry( "WarnOutput", 2 ) );
  pWarnFile->setText( pConfig->readPathEntry( "WarnFilename","kdebug.dbg" ) );
  //pWarnShow->setText( pConfig->readEntry( "WarnShow" ) );
  pErrorCombo->setCurrentItem( pConfig->readNumEntry( "ErrorOutput", 2 ) );
  pErrorFile->setText( pConfig->readPathEntry( "ErrorFilename","kdebug.dbg") );
  //pErrorShow->setText( pConfig->readEntry( "ErrorShow" ) );
  pFatalCombo->setCurrentItem( pConfig->readNumEntry( "FatalOutput", 2 ) );
  pFatalFile->setText( pConfig->readPathEntry("FatalFilename","kdebug.dbg") );
  //pFatalShow->setText( pConfig->readEntry( "FatalShow" ) );
  pAbortFatal->setChecked( pConfig->readNumEntry( "AbortFatal", 1 ) );
  slotDestinationChanged(0);
}

void KDebugDialog::save()
{
  pConfig->writeEntry( "InfoOutput", pInfoCombo->currentItem() );
  pConfig->writePathEntry( "InfoFilename", pInfoFile->text() );
  //pConfig->writeEntry( "InfoShow", pInfoShow->text() );
  pConfig->writeEntry( "WarnOutput", pWarnCombo->currentItem() );
  pConfig->writePathEntry( "WarnFilename", pWarnFile->text() );
  //pConfig->writeEntry( "WarnShow", pWarnShow->text() );
  pConfig->writeEntry( "ErrorOutput", pErrorCombo->currentItem() );
  pConfig->writePathEntry( "ErrorFilename", pErrorFile->text() );
  //pConfig->writeEntry( "ErrorShow", pErrorShow->text() );
  pConfig->writeEntry( "FatalOutput", pFatalCombo->currentItem() );
  pConfig->writePathEntry( "FatalFilename", pFatalFile->text() );
  //pConfig->writeEntry( "FatalShow", pFatalShow->text() );
  pConfig->writeEntry( "AbortFatal", pAbortFatal->isChecked() );

  TQByteArray data;
  if (!kapp->dcopClient()->send("*", "KDebug", "notifyKDebugConfigChanged()", data))
  {
    kdError() << "Unable to send DCOP message" << endl;
  }
}

void KDebugDialog::slotDestinationChanged(int) {
    pInfoFile->setEnabled(pInfoCombo->currentItem() == 0);
    pWarnFile->setEnabled(pWarnCombo->currentItem() == 0);
    pErrorFile->setEnabled(pErrorCombo->currentItem() == 0);
    pFatalFile->setEnabled(pFatalCombo->currentItem() == 0);
}

#include "kdebugdialog.moc"
