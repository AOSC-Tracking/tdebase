/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "tdedebugdialog.h"
#include "tdelistdebugdialog.h"
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>
#include <kstandarddirs.h>
#include <tqtextstream.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <tdeuniqueapplication.h>
#include <tdeconfig.h>

#include <tqfile.h>

TQStringList readAreaList()
{
  TQStringList lst;
  lst.append( "0 (generic)" );

  TQString confAreasFile = locate( "config", "kdebug.areas" );
  TQFile file( confAreasFile );
  if (!file.open(IO_ReadOnly)) {
    kdWarning() << "Couldn't open " << confAreasFile << endl;
    file.close();
  }
  else
  {
    TQString data;

    TQTextStream *ts = new TQTextStream(&file);
    ts->setEncoding( TQTextStream::Latin1 );
    while (!ts->eof()) {
      data = ts->readLine().simplifyWhiteSpace();

      int pos = data.find("#");
      if ( pos != -1 )
        data.truncate( pos );

      if (data.isEmpty())
        continue;

      lst.append( data );
    }

    delete ts;
    file.close();
  }

  return lst;
}

static TDECmdLineOptions options[] =
{
  { "fullmode", I18N_NOOP("Show the fully-fledged dialog instead of the default list dialog"), 0 },
  { "on <area>", /*I18N_NOOP TODO*/ "Turn area on", 0 },
  { "off <area>", /*I18N_NOOP TODO*/ "Turn area off", 0 },
  TDECmdLineLastOption
};

int main(int argc, char ** argv)
{
  TDEAboutData data( "tdedebugdialog", I18N_NOOP( "TDEDebugDialog"),
    "1.0", I18N_NOOP("A dialog box for setting preferences for debug output"),
    TDEAboutData::License_GPL, "(c) 2009,2010, Timothy Pearson <kb9vqf@pearsoncomputing.net>");
  data.addAuthor("Timothy Pearson", I18N_NOOP("Maintainer"), "kb9vqf@pearsoncomputing.net");
  data.addAuthor("David Faure", I18N_NOOP("Original maintainer/developer"), "faure@kde.org");
  TDECmdLineArgs::init( argc, argv, &data );
  TDECmdLineArgs::addCmdLineOptions( options );
  TDEUniqueApplication::addCmdLineOptions();
  TDEUniqueApplication app;
  TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

  TQStringList areaList ( readAreaList() );
  KAbstractDebugDialog * dialog;
  if (args->isSet("fullmode"))
      dialog = new TDEDebugDialog(areaList, 0L);
  else
  {
      TDEListDebugDialog * listdialog = new TDEListDebugDialog(areaList, 0L);
      if (args->isSet("on"))
      {
          listdialog->activateArea( args->getOption("on"), true );
          /*listdialog->save();
          listdialog->config()->sync();
          return 0;*/
      } else if ( args->isSet("off") )
      {
          listdialog->activateArea( args->getOption("off"), false );
          /*listdialog->save();
          listdialog->config()->sync();
          return 0;*/
      }
      dialog = listdialog;
  }

  /* Show dialog */
  int nRet = dialog->exec();
  if( nRet == TQDialog::Accepted )
  {
      dialog->save();
      dialog->config()->sync();
  }
  else
    dialog->config()->rollback( true );

  return 0;
}
