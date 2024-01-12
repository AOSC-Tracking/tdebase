/* test_kasbar.cpp
**
** Copyright (C) 2001-2004 Richard Moore <rich@kde.org>
** Contributor: Mosfet
**     All rights reserved.
**
** KasBar is dual-licensed: you can choose the GPL or the BSD license.
** Short forms of both licenses are included below.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/

/*
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

/*
** Bug reports and questions can be sent to kde-devel@kde.org
*/
#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <twin.h>
#include <tdeglobal.h>
#include <tdelocale.h>

#include "kasitem.h"
#include "kastasker.h"
#include "kasclockitem.h"
#include "kasloaditem.h"

#include "version.h"

static TDECmdLineOptions options[] =
{
   { "test", "Test the basic kasbar code", 0 },
   TDECmdLineLastOption
};

int main( int argc, char **argv )
{
  TDECmdLineArgs::init( argc, argv, "kasbar", "KasBar", I18N_NOOP( "An alternative task manager" ), VERSION_STRING );
  TDECmdLineArgs::addCmdLineOptions( options );
  TDEGlobal::locale()->setMainCatalogue( "kasbarextension" );
  TDEApplication app;
  TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

  kdDebug(1345) << "Kasbar starting..." << endl;

  int wflags = TQt::WStyle_Customize | TQt::WX11BypassWM | TQt::WStyle_DialogBorder | TQt::WStyle_StaysOnTop;
  KasBar *kasbar;
  TDEConfig conf( "kasbarrc" );

  if ( args->isSet("test") ) {
      kasbar = new KasBar( TQt::Vertical, 0, "testkas", (TQt::WFlags)wflags );
      kasbar->setItemSize( KasBar::Large );
      kasbar->append( new KasClockItem(kasbar) );
      kasbar->append( new KasItem(kasbar) );
      kasbar->append( new KasLoadItem(kasbar) );
      kasbar->append( new KasItem(kasbar) );
      kasbar->addTestItems();
  }
  else {
      KasTasker *kastasker = new KasTasker( TQt::Vertical, 0, "testkas", (TQt::WFlags)wflags );
      kastasker->setConfig( &conf );
      kastasker->setStandAlone( true );
      kasbar = kastasker;

      kastasker->readConfig();
      kastasker->move( kastasker->detachedPosition() );
      kastasker->connect( kastasker->resources(), TQ_SIGNAL(changed()), TQ_SLOT(readConfig()) );
      kastasker->refreshAll();
  }

  kdDebug(1345) << "Kasbar about to show" << endl;
  app.setMainWidget( kasbar );
  kasbar->show();

  kasbar->setDetached( true );
  KWin::setOnAllDesktops( kasbar->winId(), true );
  kdDebug() << "kasbar: Window id is " << kasbar->winId() << endl;

  TDEApplication::kApplication()->dcopClient()->registerAs( "kasbar" );

  app.connect( &app, TQ_SIGNAL( lastWindowClosed() ), TQ_SLOT(quit()) );

  return app.exec();
}

