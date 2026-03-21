 /*
 *  This file is part of the Trinity Help Center
 *
 *  Copyright (c) 2002 Frerich Raabe <raabe@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "application.h"
#include "mainwindow.h"
#include "tdeversion.h"
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>

using namespace KHC;

Application::Application() : TDEUniqueApplication(), mMainWindow( 0 )
{
}

int Application::newInstance()
{
  if (restoringSession()) return 0;

  TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

  KURL url;
  if ( args->count() )
    url = args->url( 0 );

  if( !mMainWindow ) {
    mMainWindow = new MainWindow;
    setMainWidget( mMainWindow );
    mMainWindow->show();
  }

  mMainWindow->openUrl( url );

  return TDEUniqueApplication::newInstance();
}

static TDECmdLineOptions options[] =
{
  { "+[url]", I18N_NOOP("URL to display"), "" },
  TDECmdLineLastOption
};

extern "C" int TDE_EXPORT kdemain( int argc, char **argv )
{
  TDEAboutData aboutData( "khelpcenter", I18N_NOOP("Trinity Help Center"),
                        TDE_VERSION_STRING,
                        I18N_NOOP("The Trinity Help Center"),
                        TDEAboutData::License_GPL,
                        I18N_NOOP("(c) 1999-2003, The KHelpCenter developers") );

  aboutData.addAuthor( "Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net" );
  aboutData.addAuthor( "Cornelius Schumacher", 0, "schumacher@kde.org" );
  aboutData.addAuthor( "Frerich Raabe", 0, "raabe@kde.org" );
  aboutData.addAuthor( "Matthias Elter", I18N_NOOP("Original Author"),
                       "me@kde.org" );
  aboutData.addAuthor( "Wojciech Smigaj", I18N_NOOP("Info page support"),
                       "achu@klub.chip.pl" );

  TDECmdLineArgs::init( argc, argv, &aboutData );
  TDECmdLineArgs::addCmdLineOptions( options );
  TDEApplication::addCmdLineOptions();

  KHC::Application app;

  if ( app.isRestored() ) {
     RESTORE( MainWindow );
  }

  return app.exec();
}
