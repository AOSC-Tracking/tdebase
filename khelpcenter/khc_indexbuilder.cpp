/*
  This file is part of the TDE Help Center
 
  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
 
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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA  02110-1301, USA
*/

#include "khc_indexbuilder.h"

#include "tdeversion.h"
#include <tdeaboutdata.h>
#include <tdelocale.h>
#include <tdecmdlineargs.h>
#include <tdeuniqueapplication.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <tdeprocess.h>
#include <tdestandarddirs.h>
#include <tdeconfig.h>

#include <tqfile.h>
#include <tqfileinfo.h>
#include <tqdir.h>
#include <tqtextstream.h>

#include <unistd.h>
#include <stdlib.h>
#include <iostream>

using namespace KHC;

IndexBuilder::IndexBuilder(const TQString& cmdFile)
{
  m_cmdFile = cmdFile;
  kdDebug(1402) << "IndexBuilder()" << endl;
}

void IndexBuilder::buildIndices()
{
  TQFile f( m_cmdFile );
  if ( !f.open( IO_ReadOnly ) ) {
    kdError() << "Unable to open file '" << m_cmdFile << "'" << endl;
    exit( 1 );
  }
  kdDebug(1402) << "Opened file '" << m_cmdFile << "'" << endl;
  TQTextStream ts( &f );
  TQString line = ts.readLine();
  while ( !line.isNull() ) {
    kdDebug(1402) << "LINE: " << line << endl;
    mCmdQueue.append( line );
    line = ts.readLine();
  }

  processCmdQueue();
}

void IndexBuilder::processCmdQueue()
{
  kdDebug(1402) << "IndexBuilder::processCmdQueue()" << endl;

  TQStringList::Iterator it = mCmdQueue.begin();

  if ( it == mCmdQueue.end() ) {
    quit();
    return;
  }

  TQString cmd = *it;

  kdDebug(1402) << "PROCESS: " << cmd << endl;

  TDEProcess *proc = new TDEProcess;
  proc->setRunPrivileged( true );

  TQStringList args = TQStringList::split( " ", cmd );
  *proc << args;


  connect( proc, TQ_SIGNAL( processExited( TDEProcess * ) ),
           TQ_SLOT( slotProcessExited( TDEProcess * ) ) );
  connect( proc, TQ_SIGNAL( receivedStdout(TDEProcess *, char *, int ) ),
           TQ_SLOT( slotReceivedStdout(TDEProcess *, char *, int ) ) );
  connect( proc, TQ_SIGNAL( receivedStderr(TDEProcess *, char *, int ) ),
           TQ_SLOT( slotReceivedStderr(TDEProcess *, char *, int ) ) );

  mCmdQueue.remove( it );

  if ( !proc->start( TDEProcess::NotifyOnExit, TDEProcess::AllOutput ) ) {
    sendErrorSignal( i18n("Unable to start command '%1'.").arg( cmd ) );
    processCmdQueue();
  }
}

void IndexBuilder::slotProcessExited( TDEProcess *proc )
{
  kdDebug(1402) << "IndexBuilder::slotIndexFinished()" << endl;

  if ( !proc->normalExit() ) {
    kdError(1402) << "Process failed" << endl;
  } else {
    int status = proc->exitStatus();
    kdDebug(1402) << "Exit status: " << status << endl;
  }

  delete proc;

  sendProgressSignal();

  processCmdQueue();
}

void IndexBuilder::slotReceivedStdout( TDEProcess *, char *buffer, int buflen )
{
  TQString text = TQString::fromLocal8Bit( buffer, buflen );
  std::cout << text.local8Bit().data() << std::flush;
}

void IndexBuilder::slotReceivedStderr( TDEProcess *, char *buffer, int buflen )
{
  TQString text = TQString::fromLocal8Bit( buffer, buflen );
  std::cerr << text.local8Bit().data() << std::flush;
}

void IndexBuilder::sendErrorSignal( const TQString &error )
{
  kdDebug(1402) << "IndexBuilder::sendErrorSignal()" << endl;
  
  TQByteArray params;
  TQDataStream stream( params, IO_WriteOnly );
  stream << error;
  tdeApp->dcopClient()->emitDCOPSignal("buildIndexError(TQString)", params );  
}

void IndexBuilder::sendProgressSignal()
{
  kdDebug(1402) << "IndexBuilder::sendProgressSignal()" << endl;
 
  tdeApp->dcopClient()->emitDCOPSignal("buildIndexProgress()", TQByteArray() );  
}

void IndexBuilder::quit()
{
  kdDebug(1402) << "IndexBuilder::quit()" << endl;

  tdeApp->quit();
}


static TDECmdLineOptions options[] =
{
  { "+cmdfile", I18N_NOOP("Document to be indexed"), 0 },
  { "+indexdir", I18N_NOOP("Index directory"), 0 },
  TDECmdLineLastOption
};

int main( int argc, char **argv )
{
  TDEAboutData aboutData( "khc_indexbuilder",
                        I18N_NOOP("KHelpCenter Index Builder"),
                        TDE_VERSION_STRING,
                        I18N_NOOP("The TDE Help Center"),
                        TDEAboutData::License_GPL,
                        I18N_NOOP("(c) 2003, The KHelpCenter developers") );

  aboutData.addAuthor( "Cornelius Schumacher", 0, "schumacher@kde.org" );

  TDECmdLineArgs::init( argc, argv, &aboutData );
  TDECmdLineArgs::addCmdLineOptions( options );
  TDEUniqueApplication::addCmdLineOptions();

  TDEApplication app;

  TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

  if ( args->count() != 2 ) {
    kdDebug(1402) << "Wrong number of arguments." << endl;
    return 1;
  }

  TQString cmdFile = args->arg( 0 );
  TQString indexDir = args->arg( 1 );

  kdDebug(1402) << "cmdFile: " << cmdFile << endl;
  kdDebug(1402) << "indexDir: " << indexDir << endl;

  TQFile file( indexDir + "/testaccess" );
  if ( !file.open( IO_WriteOnly ) || file.putch( ' ' ) < 0 ) {
    kdDebug(1402) << "access denied" << endl;
    return 2;
  } else {
    kdDebug(1402) << "can access" << endl;
    file.remove();
  }
  
  app.dcopClient()->registerAs( "khc_indexbuilder", false );

  IndexBuilder builder(cmdFile);

  TQTimer::singleShot(0, &builder, TQ_SLOT(buildIndices()));

  return app.exec();
}

#include "khc_indexbuilder.moc"
