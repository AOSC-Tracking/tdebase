/*
 * Copyright (C) 2000 by Matthias Kalle Dalheimer <kalle@kde.org>
 *
 * Licensed under the Artistic License.
 */
#include <tdeapplication.h>
#include <tdelocale.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>

#include <dcopclient.h>
#include "kdcopwindow.h"

static const TDECmdLineOptions options[] =
{
    TDECmdLineLastOption
};

int main( int argc, char ** argv )
{
  TDEAboutData aboutData( "kdcop", I18N_NOOP("KDCOP"),
			"0.1", I18N_NOOP( "A graphical DCOP browser/client" ),
			TDEAboutData::License_Artistic,
			"(c) 2000, Matthias Kalle Dalheimer");
  aboutData.addAuthor("Matthias Kalle Dalheimer",0, "kalle@kde.org");
  aboutData.addAuthor("Rik Hemsley",0, "rik@kde.org");
  aboutData.addAuthor("Ian Reinhart Geiser",0,"geiseri@kde.org");
  TDECmdLineArgs::init( argc, argv, &aboutData );
  TDECmdLineArgs::addCmdLineOptions( options );

  TDEApplication a;

  KDCOPWindow* kdcopwindow = new KDCOPWindow;
  a.setMainWidget( kdcopwindow );
  a.dcopClient()->attach();
  kdcopwindow->show();

  return a.exec();
}
