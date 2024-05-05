
/***************************************************************************
                          tdeio_finger.cpp  -  description
                             -------------------
    begin                : Sun Aug 12 2000
    copyright            : (C) 2000 by Andreas Schlapbach
    email                : schlpbch@iam.unibe.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include <tqtextstream.h>
#include <tqdict.h>
#include <tqcstring.h>
#include <tqfile.h>
#include <tqregexp.h>

#include <kdebug.h>
#include <kinstance.h>
#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <tdelocale.h>
#include <kurl.h>

#include "tdeio_finger.h"


using namespace TDEIO;

static const TQString defaultRefreshRate = "60";

extern "C"
{
  TDE_EXPORT int kdemain( int argc, char **argv )
  {
    TDEInstance instance( "tdeio_finger" );

    //kdDebug() << "*** Starting tdeio_finger " << getpid() << endl;

    if (argc != 4)
      {
	fprintf(stderr, "Usage: tdeio_finger protocol domain-socket1 domain-socket2\n");
	exit(-1);
      }

    FingerProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    //kdDebug() << "*** tdeio_finger Done" << endl;
    return 0;
  }
}


/* ---------------------------------------------------------------------------------- */


FingerProtocol::FingerProtocol(const TQCString &pool_socket, const TQCString &app_socket)
  : TQObject(), SlaveBase("finger", pool_socket, app_socket)
{
  myStdStream = new TQString();
  getProgramPath();
}


/* ---------------------------------------------------------------------------------- */


FingerProtocol::~FingerProtocol()
{
  //kdDebug() << "FingerProtocol::~FingerProtocol()" << endl;
  delete myURL;
  delete myPerlPath;
  delete myFingerPath;
  delete myFingerPerlScript;
  delete myFingerCSSFile;
  delete myStdStream;
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::get(const KURL& url )
{
  //kdDebug() << "tdeio_finger::get(const KURL& url)" << endl ;

  this->parseCommandLine(url);

  //kdDebug() << "myURL: " << myURL->prettyURL() << endl;

  // Reset the stream
  *myStdStream="";

  TQString query = myURL->query();
  TQString refreshRate = defaultRefreshRate;

  //kdDebug() << "query: " << query << endl;

  // Check the validity of the query

  TQRegExp regExp("?refreshRate=[0-9][0-9]*", true, true);
  if (query.contains(regExp)) {
    //kdDebug() << "looks like a valid query" << endl;
    TQRegExp regExp( "([0-9]+)" );
    regExp.search(query);
    refreshRate = regExp.cap(0);
  }

  //kdDebug() << "Refresh rate: " << refreshRate << endl;

  myTDEProcess = new TDEProcess();
  *myTDEProcess << *myPerlPath << *myFingerPerlScript
	      << *myFingerPath << *myFingerCSSFile
	      << refreshRate << myURL->host() << myURL->user() ;

  connect(myTDEProcess, TQ_SIGNAL(receivedStdout(TDEProcess *, char *, int)),
	  this, TQ_SLOT(slotGetStdOutput(TDEProcess *, char *, int)));
  //connect(myTDEProcess, TQ_SIGNAL(receivedStderr(TDEProcess *, char *, int)),
  //	  this, TQ_SLOT(slotGetStdOutput(TDEProcess *, char *, int)));

  myTDEProcess->start(TDEProcess::Block, TDEProcess::All);

  data(TQCString(TQString(*myStdStream).local8Bit()));

  data(TQByteArray());
  finished();

  //clean up

  delete myTDEProcess;
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::slotGetStdOutput(TDEProcess* /* p */, char *s, int len)
{
  //kdDebug() <<  "void FingerProtocol::slotGetStdoutOutput()" << endl;
  *myStdStream += TQString::fromLocal8Bit(s, len);
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::mimetype(const KURL & /*url*/)
{
  mimeType("text/html");
  finished();
}


/* ---------------------------------------------------------------------------------- */


void FingerProtocol::getProgramPath()
{
  //kdDebug() << "kfingerMainWindow::getProgramPath()" << endl;
  // Not to sure wether I'm using the right error number here. - schlpbch -

  myPerlPath = new TQString(TDEGlobal::dirs()->findExe("perl"));
  if (myPerlPath->isEmpty())
    {
      //kdDebug() << "Perl command not found" << endl;
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("Could not find the Perl program on your system, please install."));
      exit();
    }
  else
    {
      //kdDebug() << "Perl command found:" << *myPerlPath << endl;
    }

  myFingerPath = new TQString(TDEGlobal::dirs()->findExe("finger"));
  if ((myFingerPath->isEmpty()))
    {
      //kdDebug() << "Finger command not found" << endl;
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("Could not find the Finger program on your system, please install."));
      exit();
    }
  else
    {
      //kdDebug() << "Finger command found:" << *myFingerPath << endl;
    }

  myFingerPerlScript = new TQString(locate("data","tdeio_finger/tdeio_finger.pl"));
  if (myFingerPerlScript->isEmpty())
    {
      //kdDebug() << "tdeio_finger.pl script not found" << endl;
      this->error(ERR_CANNOT_LAUNCH_PROCESS,
		  i18n("tdeio_finger Perl script not found."));
      exit();
    }
  else
    {
      //kdDebug() << "tdeio_finger perl script found: " << *myFingerPerlScript << endl;
    }

  myFingerCSSFile = new TQString(locate("data","tdeio_finger/tdeio_finger.css"));
  if (myFingerCSSFile->isEmpty())
    {
      //kdDebug() << "tdeio_finger.css file not found" << endl;
      this->warning(i18n("tdeio_finger CSS script not found. Output will look ugly."));
    }
  else
    {
      //kdDebug() << "tdeio_finger CSS file found: " << *myFingerCSSFile << endl;
    }
}


/* --------------------------------------------------------------------------- */


void FingerProtocol::parseCommandLine(const KURL& url)
{
  myURL = new KURL(url);

  /*
   * Generate a valid finger url
   */

  if(myURL->isEmpty() || !myURL->isValid() ||
     (myURL->user().isEmpty() && myURL->host().isEmpty()))
    {
      myURL->setProtocol("finger");
      myURL->setUser("");
      myURL->setHost("localhost");
    }

  /*
   * If no specific port is specified, set it to 79.
   */

  if(myURL->port() == 0) {
    myURL->setPort(79);
  }

  /*
   * If no refresh rate is given, set it to defaultRefreshRate
   */

  if (myURL->query().isEmpty()) {
    myURL->setQuery("?refreshRate="+defaultRefreshRate);
  }
}

/* ---------------------------------------------------------------------------------- */
#include "tdeio_finger.moc"
/* ---------------------------------------------------------------------------------- */

