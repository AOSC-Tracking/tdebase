/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#include <config.h>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <dcopclient.h>
#include <tqmessagebox.h>
#include <tqdir.h>

#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include "server.h"


static const char version[] = "0.4";
static const char description[] = I18N_NOOP( "The reliable TDE session manager that talks the standard X11R6 \nsession management protocol (XSMP)." );

static const TDECmdLineOptions options[] =
{
   { "r", 0, 0 },
   { "restore", I18N_NOOP("Restores the saved user session if available"), 0},
   { "w", 0, 0 },
   { "windowmanager <wm>", I18N_NOOP("Starts 'wm' in case no other window manager is \nparticipating in the session. Default is 'twin'"), 0},
   { "windowmanageraddargs <wm>", I18N_NOOP("Pass additional arguments to the window manager. Default is ''"), 0},
   { "nolocal", I18N_NOOP("Also allow remote connections"), 0},
   TDECmdLineLastOption
};

extern KSMServer* the_server;

void IoErrorHandler ( IceConn iceConn)
{
    the_server->ioError( iceConn );
}

bool writeTest(TQCString path)
{
   path += "/XXXXXX";
   int fd = mkstemp(path.data());
   if (fd == -1)
      return false;
   if (write(fd, "Hello World\n", 12) == -1)
   {
      int save_errno = errno;
      close(fd);
      unlink(path.data());
      errno = save_errno;
      return false;
   }
   close(fd);
   unlink(path.data());
   return true;
}

void sanity_check( int argc, char* argv[] )
{
  TQCString msg;
  TQCString path = getenv("HOME");
  TQCString readOnly = getenv("TDE_HOME_READONLY");
  if (path.isEmpty())
  {
     msg = "$HOME not set!";
  }
  if (msg.isEmpty() && access(path.data(), W_OK))
  {
     if (errno == ENOENT)
        msg = "$HOME directory (%s) does not exist.";
     else if (readOnly.isEmpty())
        msg = "No write access to $HOME directory (%s).";
  }
  if (msg.isEmpty() && access(path.data(), R_OK))
  {
     if (errno == ENOENT)
        msg = "$HOME directory (%s) does not exist.";
     else
        msg = "No read access to $HOME directory (%s).";
  }
  if (msg.isEmpty() && readOnly.isEmpty() && !writeTest(path))
  {
     if (errno == ENOSPC)
        msg = "$HOME directory (%s) is out of disk space.";
     else
        msg = "Writing to the $HOME directory (%s) failed with\n    "
              "the error '"+TQCString(strerror(errno))+"'";
  }
  if (msg.isEmpty())
  {
     path = IceAuthFileName();
     if (path.isEmpty())
        msg = "Unable to determine path for ICEauthority file.";
     else if (access(path.data(), W_OK) && (errno != ENOENT))
        msg = "No write access to '%s'.";
     else if (access(path.data(), R_OK) && (errno != ENOENT))
        msg = "No read access to '%s'.";
  }
  if (msg.isEmpty())
  {
     path = DCOPClient::dcopServerFile();
     if (access(path.data(), R_OK) && (errno == ENOENT))
     {
        // Check iceauth
        if (DCOPClient::iceauthPath().isEmpty())
           msg = "Could not find 'iceauth' in path.";
     }
  }
  if (msg.isEmpty())
  {
     path = getenv("TDETMP");
     if (path.isEmpty())
        path = "/tmp";
     if (!writeTest(path))
     {
        if (errno == ENOSPC)
           msg = "Temp directory (%s) is out of disk space.";
        else
           msg = "Writing to the temp directory (%s) failed with\n    "
                 "the error '"+TQCString(strerror(errno))+"'";
     }
  }
  if (msg.isEmpty() && (path != "/tmp"))
  {
     path = "/tmp";
     if (!writeTest(path))
     {
        if (errno == ENOSPC)
           msg = "Temp directory (%s) is out of disk space.";
        else
           msg = "Writing to the temp directory (%s) failed with\n    "
                 "the error '"+TQCString(strerror(errno))+"'";
     }
  }
  if (msg.isEmpty())
  {
     path += ".ICE-unix";
     if (access(path.data(), W_OK) && (errno != ENOENT))
        msg = "No write access to '%s'.";
     else if (access(path.data(), R_OK) && (errno != ENOENT))
        msg = "No read access to '%s'.";
  }
  if (!msg.isEmpty())
  {
    const char *msg_pre =
             "The following installation problem was detected\n"
             "while trying to start TDE:"
             "\n\n    ";
    const char *msg_post = "\n\nTDE is unable to start.\n";
    fputs(msg_pre, stderr);
    fprintf(stderr, msg.data(), path.data());
    fputs(msg_post, stderr);

    TQApplication a(argc, argv);
    TQCString qmsg(256+path.length());
    qmsg.sprintf(msg.data(), path.data());
    qmsg = msg_pre+qmsg+msg_post;
    TQMessageBox::critical(0, "TDE Installation Problem!",
        TQString::fromLatin1(qmsg.data()));
    exit(255);
  }
}

extern "C" KDE_EXPORT int kdemain( int argc, char* argv[] )
{
    sanity_check(argc, argv);

    TDEAboutData aboutData( "ksmserver", I18N_NOOP("The TDE Session Manager"),
       version, description, TDEAboutData::License_BSD,
       "(C) 2000, The KDE Developers");
    aboutData.addAuthor("Matthias Ettrich",0, "ettrich@kde.org");
    aboutData.addAuthor("Luboš Luňák", I18N_NOOP( "Maintainer" ), "l.lunak@kde.org" );

    TDECmdLineArgs::init(argc, argv, &aboutData);
    TDECmdLineArgs::addCmdLineOptions( options );

    putenv((char*)"SESSION_MANAGER=");
    TDEApplication a(TDEApplication::openX11RGBADisplay(), false); // Disable styles until we need them.
    fcntl(ConnectionNumber(tqt_xdisplay()), F_SETFD, 1);


    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

    kapp->dcopClient()->registerAs("ksmserver", false);
    if (!kapp->dcopClient()->isRegistered())
    {
       tqWarning("[KSMServer] Could not register with DCOPServer. Aborting.");
       return 1;
    }

    TQCString wm = args->getOption("windowmanager");
    TQCString wmaddargs = args->getOption("windowmanageraddargs");
    if ( wm.isEmpty() )
	wm = "twin";

    bool only_local = args->isSet("local");
#ifndef HAVE__ICETRANSNOLISTEN
    /* this seems strange, but the default is only_local, so if !only_local
     * the option --nolocal was given, and we warn (the option --nolocal
     * does nothing on this platform, as here the default is reversed)
     */
    if (!only_local) {
        tqWarning("[KSMServer] --[no]local is not supported on your platform. Sorry.");
    }
    only_local = false;
#endif

    KSMServer *server = new KSMServer( TQString::fromLatin1(wm), TQString::fromLatin1(wmaddargs), only_local);
    kapp->dcopClient()->setDefaultObject( server->objId() );

    IceSetIOErrorHandler( IoErrorHandler );

    TDEConfig *config = TDEGlobal::config();
    config->setGroup( "General" );

    int realScreenCount = ScreenCount( tqt_xdisplay() );
    bool screenCountChanged =
         ( config->readNumEntry( "screenCount", realScreenCount ) != realScreenCount );

    TQString loginMode = config->readEntry( "loginMode", "restorePreviousLogout" );

    if ( args->isSet("restore") && ! screenCountChanged )
	server->restoreSession( SESSION_BY_USER );
    else if ( loginMode == "default" || screenCountChanged )
	server->startDefaultSession();
    else if ( loginMode == "restorePreviousLogout" )
	server->restoreSession( SESSION_PREVIOUS_LOGOUT );
    else if ( loginMode == "restoreSavedSession" )
	server->restoreSession( SESSION_BY_USER );
    else
	server->startDefaultSession();
    return a.exec();
}

