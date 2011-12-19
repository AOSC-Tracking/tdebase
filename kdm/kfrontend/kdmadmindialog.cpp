    /*

    Admin dialog

    Copyright (C) 1997, 1998, 2000 Steffen Hansen <hansen@kde.org>
    Copyright (C) 2000-2003 Oswald Buddenhagen <ossi@kde.org>


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    */

#include "kdmadmindialog.h"
#include "kdmconfig.h"
#include "kgdialog.h"
#include "kdm_greet.h"
#include <stdlib.h>

#include <kapplication.h>
#include <kseparator.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>

#include <tqcombobox.h>
#include <tqvbuttongroup.h>
#include <tqstyle.h>
#include <tqlayout.h>
#include <tqaccel.h>
#include <tqpopupmenu.h>

int KDMAdmin::curPlugin = -1;
PluginList KDMAdmin::pluginList;

KDMAdmin::KDMAdmin( const TQString &user, TQWidget *_parent )
    : inherited( _parent )
    , verify( 0 ), curUser(user)
{
    TQSizePolicy fp( TQSizePolicy::Fixed, TQSizePolicy::Fixed );

    TQVBoxLayout *box = new TQVBoxLayout( this, 10 );

    TQHBoxLayout *hlay = new TQHBoxLayout( box );

    GSendInt( G_ReadDmrc );
    GSendStr( "root" );
    GRecvInt(); // ignore status code ...

    if (curPlugin < 0) {
       curPlugin = 0;
       pluginList = KGVerify::init( "classic" );
    }
    verify = new KGStdVerify( this, this,
			      this, "root",
			      pluginList, KGreeterPlugin::Authenticate,
			      KGreeterPlugin::Shutdown );
    verify->selectPlugin( curPlugin );
    box->addLayout( verify->getLayout() );
    TQAccel *accel = new TQAccel( this );
    accel->insertItem( ALT+Key_A, 0 );
    connect( accel, TQT_SIGNAL(activated(int)), TQT_SLOT(slotActivatePlugMenu()) );

    box->addWidget( new KSeparator( KSeparator::HLine, this ) );

    okButton = new KPushButton( KStdGuiItem::ok(), this );
    okButton->setSizePolicy( fp );
    okButton->setDefault( true );
    cancelButton = new KPushButton( KStdGuiItem::cancel(), this );
    cancelButton->setSizePolicy( fp );

    hlay = new TQHBoxLayout( box );
    hlay->addStretch( 1 );
    hlay->addWidget( okButton );
    hlay->addStretch( 1 );
    hlay->addWidget( cancelButton );
    hlay->addStretch( 1 );

    connect( okButton, TQT_SIGNAL(clicked()), TQT_SLOT(accept()) );
    connect( cancelButton, TQT_SIGNAL(clicked()), TQT_SLOT(reject()) );

    slotWhenChanged();
}

KDMAdmin::~KDMAdmin()
{
    hide();
    delete verify;
}

void
KDMAdmin::slotActivatePlugMenu()
{
    TQPopupMenu *cmnu = verify->getPlugMenu();
    TQSize sh( cmnu->sizeHint() / 2 );
    cmnu->exec( geometry().center() - TQPoint( sh.width(), sh.height() ) );
}

void
KDMAdmin::accept()
{
    verify->accept();
}

void
KDMAdmin::slotWhenChanged()
{
    verify->abort();
    verify->setEnabled( 1 );
    verify->start();
}

void
KDMAdmin::bye_bye()
{
  GSendInt( G_GetDmrc );
  GSendStr( "Session" );
  char *sess = GRecvStr();
  if (sess && strcmp(sess, "admin")) {
    GSendInt( G_PutDmrc );
    GSendStr( "OrigSession");
    GSendStr( sess);
    free(sess);
  }

  GSendInt( G_PutDmrc );
  GSendStr( "Session" );
  GSendStr( "admin" );
  inherited::accept();
}

void
KDMAdmin::verifyPluginChanged( int id )
{
    curPlugin = id;
    adjustSize();
}

void
KDMAdmin::verifyOk()
{
    bye_bye();
}

void
KDMAdmin::verifyFailed()
{
    okButton->setEnabled( false );
    cancelButton->setEnabled( false );
}

void
KDMAdmin::verifyRetry()
{
    okButton->setEnabled( true );
    cancelButton->setEnabled( true );
}

void
KDMAdmin::verifySetUser( const TQString & )
{
}


#include "kdmadmindialog.moc"
