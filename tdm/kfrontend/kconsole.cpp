/*

xconsole widget for TDM

Copyright (C) 2002-2003 Oswald Buddenhagen <ossi@kde.org>


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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <config.h>

#ifdef WITH_TDM_XCONSOLE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#ifdef HAVE_TERMIOS_H
extern "C" {
#include <termios.h>
}
#endif
#ifdef HAVE_TERMIO_H
/* needed at least on AIX */
#include <termio.h>
#endif

#include "kconsole.h"
#include "tdmconfig.h"
#include "tdm_greet.h"

#include <tdelocale.h>
#include <kpty.h>

#include <tqsocketnotifier.h>

KConsole::KConsole( TQWidget *_parent )
	: inherited( _parent )
	, pty( 0 )
	, notifier( 0 )
	, fd( -1 )
{
	setReadOnly( true );
	setWordWrap( NoWrap );
	setTextFormat( PlainText );

	if (!OpenConsole())
		append( i18n("Cannot open console") );
}

KConsole::~KConsole()
{
	CloseConsole();
}

int
KConsole::OpenConsole()
{
#ifdef TIOCCONS
	static const char on = 1;
#endif

	if (*_logSource) {
		if ((fd = open( _logSource, O_RDONLY | O_NONBLOCK )) >= 0)
			goto gotcon;
		LogError( "Cannot open log source %s, "
		          "falling back to /dev/console.\n", _logSource );
	}

	pty = new KPty;
	if (!pty->open()) {
		delete pty;
		pty = 0;
		return 0;
	}

#ifdef TIOCCONS
	if (ioctl( pty->slaveFd(), TIOCCONS, &on ) < 0) {
		perror( "ioctl TIOCCONS" );
		delete pty;
		pty = 0;
		return 0;
	}
#else
	int consfd;
	if ((consfd = open( "/dev/console", O_RDONLY )) < 0) {
		perror( "opening /dev/console" );
		delete pty;
		pty = 0;
		return 0;
	}
	if (ioctl( consfd, SRIOCSREDIR, slave_fd ) < 0) {
		perror( "ioctl SRIOCSREDIR" );
		::close( consfd );
		delete pty;
		pty = 0;
		return 0;
	}
	::close( consfd );
#endif
	fd = pty->masterFd();

  gotcon:
	notifier = new TQSocketNotifier( fd, TQSocketNotifier::Read, this );
	connect( notifier, TQ_SIGNAL(activated( int )), TQ_SLOT(slotData()) );
	return 1;
}

void
KConsole::CloseConsole()
{
	delete notifier;
	notifier = 0;
	if (pty) {
		delete pty;
		pty = 0;
	} else
		::close( fd );
	fd = -1;
}

void
KConsole::slotData()
{
	int n;
	char buffer[1024];

	if ((n = read( fd, buffer, sizeof(buffer) )) <= 0) {
		CloseConsole();
		if (!n)
			if (!OpenConsole())
				append( i18n("\n*** Cannot open console log source ***") );
	} else {
		bool as = !verticalScrollBar()->isVisible() ||
		          (verticalScrollBar()->value() ==
		           verticalScrollBar()->maxValue());
		TQString str( TQString::fromLocal8Bit( buffer, n ).remove( '\r' ) );
		int pos, opos;
		for (opos = 0; (pos = str.find( '\n', opos )) >= 0; opos = pos + 1) {
			if (paragraphs() == 100)
				removeParagraph( 0 );
			if (!leftover.isEmpty()) {
				append( leftover + str.mid( opos, pos - opos ) );
				leftover = TQString::null;
			} else
				append( str.mid( opos, pos - opos ) );
		}
		leftover += str.mid( opos );
		if (as)
			scrollToBottom();
	}
}

#include "kconsole.moc"

#endif
