/*
    kwrited is a write(1) receiver for KDE.
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// System
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

// X11
#include <X11/Xlib.h>
#include <fixx11h.h>

// Qt
#undef FontChange	// Qt4
#include <dcopclient.h>
#include <tqsocketnotifier.h>

// TDE 
#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <tdeglobalsettings.h>
#include <kdebug.h>
#include <tdecrash.h>
#include <kpty.h>
#include <kuser.h>
#include <tdeglobal.h>

// kwrited
#include "kwrited.h"
#include <config.h>

/* TODO
   for anyone who likes to do improvements here, go ahead.
   - check FIXMEs below
   - add Menu
     - accept messages (on/off)
     - pop up on incoming messages
     - clear messages
     - allow max. lines
   - add CORBA interface?
   - add session awareness.
   - add client complements.
*/

KWrited::KWrited() : TQTextEdit()
{
  int pref_width, pref_height;

  setFont(TDEGlobalSettings::fixedFont());
  pref_width = (2 * TDEGlobalSettings::desktopGeometry(0).width()) / 3;
  pref_height = fontMetrics().lineSpacing() * 10;
  setMinimumWidth(pref_width);
  setMinimumHeight(pref_height);
  setReadOnly(true);
  setFocusPolicy(TQWidget::NoFocus);
  setWordWrap(TQTextEdit::WidgetWidth);
  setTextFormat(TQt::PlainText);

  pty = new KPty();
  pty->open();
  pty->login(KUser().loginName().local8Bit().data(), getenv("DISPLAY"));
  TQSocketNotifier *sn = new TQSocketNotifier(pty->masterFd(), TQSocketNotifier::Read, this);
  connect(sn, TQ_SIGNAL(activated(int)), this, TQ_SLOT(block_in(int)));

  TQString txt = i18n("KWrited - Listening on Device %1").arg(pty->ttyName());
  setCaption(txt);
  
  kdDebug() << txt << endl;
}

KWrited::~KWrited()
{
    pty->logout();
    delete pty;
}

void KWrited::block_in(int fd)
{
  char buf[4096];
  int len = read(fd, buf, 4096);
  if (len <= 0)
     return;

  insert( TQString::fromLocal8Bit( buf, len ).remove('\r') );
  show();
  raise();
}

void KWrited::clearText()
{
   clear();
}

TQPopupMenu *KWrited::createPopupMenu( const TQPoint &pos )
{
   TQPopupMenu *menu = TQTextEdit::createPopupMenu( pos );

   menu->insertItem( i18n( "Clear Messages" ),
                     this, TQ_SLOT( clearText() ), 
                     0, -1, 0 );

   return menu;
}

KWritedModule::KWritedModule( const TQCString& obj )
    : KDEDModule( obj )
{
    TDEGlobal::locale()->insertCatalogue("konsole");
    pro = new KWrited;
}

KWritedModule::~KWritedModule()
{
    delete pro;
    TDEGlobal::locale()->removeCatalogue("konsole");
}

extern "C"
TDE_EXPORT KDEDModule* create_kwrited( const TQCString& obj )
    {
    return new KWritedModule( obj );
    }

#include "kwrited.moc"
