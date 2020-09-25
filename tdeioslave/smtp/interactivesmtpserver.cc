/*
    interactivesmtpserver.cc

    Code based on the serverSocket example by Jesper Pedersen.

    This file is part of the testsuite of tdeio_smtp, the KDE SMTP tdeioslave.
    Copyright (c) 2004 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config.h>

#include <tqserversocket.h>
#include <tqsocket.h>
#include <tqwidget.h>
#include <tqapplication.h>
#include <tqhostaddress.h>
#include <tqtextedit.h>
#include <tqlineedit.h>
#include <tqlabel.h>
#include <tqstring.h>
#include <tqlayout.h>
#include <tqpushbutton.h>

#include <cassert>

#include "interactivesmtpserver.h"

static const TQHostAddress localhost( 0x7f000001 ); // 127.0.0.1

InteractiveSMTPServerWindow::~InteractiveSMTPServerWindow() {
    if ( mSocket ) {
        mSocket->close();
        if ( mSocket->state() == TQSocket::Closing )
            connect( mSocket, TQT_SIGNAL(delayedCloseFinished()),
                     mSocket, TQT_SLOT(deleteLater()) );
        else
            mSocket->deleteLater();
        mSocket = 0;
    }
}

void InteractiveSMTPServerWindow::slotSendResponse()
{
        const TQString line = mLineEdit->text();
    mLineEdit->clear();
    TQTextStream s( mSocket );
    s << line + "\r\n";
    slotDisplayServer( line );
}

InteractiveSMTPServer::InteractiveSMTPServer( TQObject* parent )
    : TQServerSocket( localhost, 2525, 1, parent )
{
}

int main( int argc, char * argv[] ) {
  TQApplication app( argc, argv );

  InteractiveSMTPServer server;

  tqDebug( "Server should now listen on localhost:2525" );
  tqDebug( "Hit CTRL-C to quit." );
  return app.exec();
};


InteractiveSMTPServerWindow::InteractiveSMTPServerWindow( TQSocket * socket, TQWidget * parent, const char * name, WFlags f )
  : TQWidget( parent, name, f ), mSocket( socket )
{
  TQPushButton * but;
  assert( socket );

  TQVBoxLayout * vlay = new TQVBoxLayout( this, 6 );

  mTextEdit = new TQTextEdit( this );
  mTextEdit->setTextFormat( TQTextEdit::LogText );
  vlay->addWidget( mTextEdit, 1 );

  TQHBoxLayout * hlay = new TQHBoxLayout( vlay );

  mLineEdit = new TQLineEdit( this );
  but = new TQPushButton( "&Send", this );
  hlay->addWidget( new TQLabel( mLineEdit, "&Response:", this ) );
  hlay->addWidget( mLineEdit, 1 );
  hlay->addWidget( but );

  connect( mLineEdit, TQT_SIGNAL(returnPressed()), TQT_SLOT(slotSendResponse()) );
  connect( but, TQT_SIGNAL(clicked()), TQT_SLOT(slotSendResponse()) );

  but = new TQPushButton( "&Close Connection", this );
  vlay->addWidget( but );

  connect( but, TQT_SIGNAL(clicked()), TQT_SLOT(slotConnectionClosed()) );

  connect( socket, TQT_SIGNAL(connectionClosed()), TQT_SLOT(slotConnectionClosed()) );
  connect( socket, TQT_SIGNAL(error(int)), TQT_SLOT(slotError(int)) );
  connect( socket, TQT_SIGNAL(readyRead()), TQT_SLOT(slotReadyRead()) );

  mLineEdit->setText( "220 hi there" );
  mLineEdit->setFocus();
}

#include "interactivesmtpserver.moc"
