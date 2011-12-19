/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <kapplication.h>
#include <kaccelmanager.h>
#include <kcombobox.h>
#include <klocale.h>

#include <tqbuttongroup.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqradiobutton.h>
#include <tqspinbox.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>

#include "HostConnector.h"

HostConnector::HostConnector( TQWidget *parent, const char *name )
  : KDialogBase( Plain, i18n( "Connect Host" ), Help | Ok | Cancel, Ok,
                 parent, name, true, true )
{
  TQFrame *page = plainPage();
  TQGridLayout *layout = new TQGridLayout( page, 2, 2, 0, spacingHint() );
  layout->setColStretch( 1, 1 );

  TQLabel *label = new TQLabel( i18n( "Host:" ), page );
  layout->addWidget( label, 0, 0 );

  mHostNames = new KComboBox( true, page );
  mHostNames->setMaxCount( 20 );
  mHostNames->setInsertionPolicy( TQComboBox::AtTop );
  mHostNames->setAutoCompletion( true );
  mHostNames->setDuplicatesEnabled( false );
  layout->addWidget( mHostNames, 0, 1 );
  label->setBuddy( mHostNames );
  TQWhatsThis::add( mHostNames, i18n( "Enter the name of the host you want to connect to." ) );

  mHostNameLabel = new TQLabel( page );
  mHostNameLabel->hide();
  layout->addWidget( mHostNameLabel, 0, 1 );

  TQButtonGroup *group = new TQButtonGroup( 0, Qt::Vertical,
                                          i18n( "Connection Type" ), page );
  TQGridLayout *groupLayout = new TQGridLayout( group->tqlayout(), 4, 4,
      spacingHint() );
  groupLayout->setAlignment( Qt::AlignTop );

  mUseSsh = new TQRadioButton( i18n( "ssh" ), group );
  mUseSsh->setEnabled( true );
  mUseSsh->setChecked( true );
  TQWhatsThis::add( mUseSsh, i18n( "Select this to use the secure shell to login to the remote host." ) );
  groupLayout->addWidget( mUseSsh, 0, 0 );

  mUseRsh = new TQRadioButton( i18n( "rsh" ), group );
  TQWhatsThis::add( mUseRsh, i18n( "Select this to use the remote shell to login to the remote host." ) );
  groupLayout->addWidget( mUseRsh, 0, 1 );

  mUseDaemon = new TQRadioButton( i18n( "Daemon" ), group );
  TQWhatsThis::add( mUseDaemon, i18n( "Select this if you want to connect to a ksysguard daemon that is running on the machine you want to connect to, and is listening for client requests." ) );
  groupLayout->addWidget( mUseDaemon, 0, 2 );

  mUseCustom = new TQRadioButton( i18n( "Custom command" ), group );
  TQWhatsThis::add( mUseCustom, i18n( "Select this to use the command you entered below to start ksysguardd on the remote host." ) );
  groupLayout->addWidget( mUseCustom, 0, 3 );

  label = new TQLabel( i18n( "Port:" ), group );
  groupLayout->addWidget( label, 1, 0 );

  mPort = new TQSpinBox( 1, 65535, 1, group );
  mPort->setEnabled( false );
  mPort->setValue( 3112 );
  TQToolTip::add( mPort, i18n( "Enter the port number on which the ksysguard daemon is listening for connections." ) );
  groupLayout->addWidget( mPort, 1, 2 );

  label = new TQLabel( i18n( "e.g.  3112" ), group );
  groupLayout->addWidget( label, 1, 3 );

  label = new TQLabel( i18n( "Command:" ), group );
  groupLayout->addWidget( label, 2, 0 );

  mCommands = new KComboBox( true, group );
  mCommands->setEnabled( false );
  mCommands->setMaxCount( 20 );
  mCommands->setInsertionPolicy( TQComboBox::AtTop );
  mCommands->setAutoCompletion( true );
  mCommands->setDuplicatesEnabled( false );
  TQWhatsThis::add( mCommands, i18n( "Enter the command that runs ksysguardd on the host you want to monitor." ) );
  groupLayout->addMultiCellWidget( mCommands, 2, 2, 2, 3 );
  label->setBuddy( mCommands );

  label = new TQLabel( i18n( "e.g. ssh -l root remote.host.org ksysguardd" ), group );
  groupLayout->addMultiCellWidget( label, 3, 3, 2, 3 );

  layout->addMultiCellWidget( group, 1, 1, 0, 1 );

  connect( mUseCustom, TQT_SIGNAL( toggled( bool ) ),
           mCommands, TQT_SLOT( setEnabled( bool ) ) );
  connect( mUseDaemon, TQT_SIGNAL( toggled( bool ) ),
           mPort, TQT_SLOT( setEnabled( bool ) ) );
  connect( mHostNames->lineEdit(),  TQT_SIGNAL( textChanged ( const TQString & ) ),
           this, TQT_SLOT(  slotHostNameChanged( const TQString & ) ) );
  enableButtonOK( !mHostNames->lineEdit()->text().isEmpty() );
  KAcceleratorManager::manage( this );
}

HostConnector::~HostConnector()
{
}

void HostConnector::slotHostNameChanged( const TQString &_text )
{
    enableButtonOK( !_text.isEmpty() );
}

void HostConnector::setHostNames( const TQStringList &list )
{
  mHostNames->insertStringList( list );
}

TQStringList HostConnector::hostNames() const
{
  TQStringList list;

	for ( int i = 0; i < mHostNames->count(); ++i )
    list.append( mHostNames->text( i ) );

  return list;
}

void HostConnector::setCommands( const TQStringList &list )
{
  mCommands->insertStringList( list );
}

TQStringList HostConnector::commands() const
{
  TQStringList list;

	for ( int i = 0; i < mCommands->count(); ++i )
    list.append( mCommands->text( i ) );

  return list;
}

void HostConnector::setCurrentHostName( const TQString &hostName )
{
  if ( !hostName.isEmpty() ) {
    mHostNames->hide();
    mHostNameLabel->setText( hostName );
    mHostNameLabel->show();
    enableButtonOK( true );//enable true when mHostNames is empty and hidden fix #66955
  } else {
    mHostNameLabel->hide();
    mHostNames->show();
    mHostNames->setFocus();
  }
}

TQString HostConnector::currentHostName() const
{
  return mHostNames->currentText();
}

TQString HostConnector::currentCommand() const
{
  return mCommands->currentText();
}

int HostConnector::port() const
{
  return mPort->value();
}

bool HostConnector::useSsh() const
{
  return mUseSsh->isChecked();
}

bool HostConnector::useRsh() const
{
  return mUseRsh->isChecked();
}

bool HostConnector::useDaemon() const
{
  return mUseDaemon->isChecked();
}

bool HostConnector::useCustom() const
{
  return mUseCustom->isChecked();
}

void HostConnector::slotHelp()
{
  kapp->invokeHelp( "CONNECTINGTOOTHERHOSTS", "ksysguard/the-sensor-browser.html" );
}

#include "HostConnector.moc"
