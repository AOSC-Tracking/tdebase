/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateapp.h"
#include "kateapp.moc"

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateviewmanager.h"
#include "kateappIface.h"
#include "katesession.h"
#include "katemainwindow.h"

#include "../interfaces/application.h"

#include <tdecmdlineargs.h>
#include <dcopclient.h>
#include <tdeconfig.h>
#include <twin.h>
#include <ktip.h>
#include <kdebug.h>
#include <klibloader.h>
#include <tdemessagebox.h>
#include <tdelocale.h>
#include <ksimpleconfig.h>
#include <tdestartupinfo.h>

#include <tqfile.h>
#include <tqtimer.h>
#include <tqdir.h>
#include <tqtextcodec.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

KateApp::KateApp (TDECmdLineArgs *args)
 : TDEApplication ()
 , m_args (args)
 , m_shouldExit (false)
{
  // Don't handle DCOP requests yet
  dcopClient()->suspend();

  // insert right translations for the katepart
  TDEGlobal::locale()->insertCatalogue("katepart");

  // some global default
  Kate::Document::setFileChangedDialogsActivated (true);

  // application interface
  m_application = new Kate::Application (this);

  // doc + project man
  m_docManager = new KateDocManager (this);

  // init all normal plugins
  m_pluginManager = new KatePluginManager (this);

  // session manager up
  m_sessionManager = KateSessionManager::self();

  // application dcop interface
  m_obj = new KateAppDCOPIface (this);

  kdDebug()<<"Setting KATE_PID: '"<<getpid()<<"'"<<endl;
  ::setenv( "KATE_PID", TQString(TQString("%1").arg(getpid())).latin1(), 1 );

  // handle restore different
  if (isRestored())
  {
    restoreKate ();
  }
  else
  {
    // let us handle our command line args and co ;)
    // we can exit here if session chooser decides
    if (!startupKate ())
    {
      m_shouldExit = true;
      return;
    }
  }

  // Ok. We are ready for DCOP requests.
  dcopClient()->resume();
}

KateApp::~KateApp ()
{
  delete m_obj;            // cu dcop interface
  delete m_pluginManager;  // cu plugin manager
  delete m_sessionManager; // delete session manager
  delete m_docManager;     // delete document manager. Do this now, or we crash
}

KateApp *KateApp::self ()
{
  return (KateApp *) kapp;
}

Kate::Application *KateApp::application ()
{
  return m_application;
}

/**
 * Has always been the Kate Versioning Scheme:
 * KDE X.Y.Z contains Kate X-1.Y.Z
 */
TQString KateApp::kateVersion (bool fullVersion)
{
//   return fullVersion ? TQString ("%1.%2.%3").arg(KDE::versionMajor() - 1).arg(KDE::versionMinor()).arg(KDE::versionRelease())
//            : TQString ("%1.%2").arg(KDE::versionMajor() - 1).arg(KDE::versionMinor());
  /** The previous version computation scheme (commented out above) worked fine in the 3.5.x days.
      With the new Trinity Rx.y.z scheme the version number gets weird.
      We now hard-code the first two numbers to match the 3.5.x days and only update the last number. */
  return fullVersion ? TQString ("2.5.%1").arg(KDE::versionMajor()) : TQString ("%1.%2").arg(2.5);
}

void KateApp::restoreKate()
{
  // restore the nice files ;) we need it
  Kate::Document::setOpenErrorDialogsActivated(false);

  // restore last session
  sessionManager()->restoreLastSession();
  m_docManager->restoreDocumentList(sessionConfig());

  Kate::Document::setOpenErrorDialogsActivated(true);

  // no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow();

  // Do not notify about start there: this makes kicker crazy and kate go to a wrong desktop.
  // TDEStartupInfo::setNewStartupId( activeMainWindow(), startupId());
}

bool KateApp::startupKate()
{
  if (m_args->isSet("start"))
  {
    // the user has specified the session to open. If the session does not exist,
    // a new session with the specified name will be created
    TQCString sessName = m_args->getOption("start");
    int sessId = sessionManager()->getSessionIdFromName(sessName);
    if (sessId != KateSessionManager::INVALID_SESSION)
    {
      sessionManager()->activateSession(sessId);
    }
    else
    {
      sessionManager()->newSession(sessName);
    }
  }
  else
  {
    // check Kate session startup options
    int startupOption = sessionManager()->getStartupOption();
    if (startupOption == KateSessionManager::STARTUP_NEW)
    {
      sessionManager()->newSession();
    }
    else if (startupOption == KateSessionManager::STARTUP_LAST)
    {
      sessionManager()->restoreLastSession();
    }
    else // startupOption == KateSessionManager::STARTUP_MANUAL
    {
      KateSessionChooser *chooser = new KateSessionChooser(NULL);
      int result = chooser->exec();
      switch (result)
      {
        case KateSessionChooser::RESULT_OPEN_NEW:
          sessionManager()->newSession();
          break;

        case KateSessionChooser::RESULT_OPEN_EXISTING:
          if (!m_sessionManager->activateSession(chooser->getSelectedSessionId()))
          {
            // Open a new session in case of error
            sessionManager()->newSession();
          }
          break;

        default: // KateSessionChooser::RESULT_QUIT_KATE:
          // Kate will exit now and notify it is done
          TDEStartupInfo::appStarted(startupId());
          return false;
          break;
      }
      delete chooser;
    }
  }

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // notify about start
  TDEStartupInfo::setNewStartupId( activeMainWindow(), startupId());

  TQTextCodec *codec = m_args->isSet("encoding") ? TQTextCodec::codecForName(m_args->getOption("encoding")) : 0;

  bool tempfileSet = TDECmdLineArgs::isTempFileSet();

  Kate::Document::setOpenErrorDialogsActivated (false);
  uint id = 0;
  for (int z=0; z<m_args->count(); z++)
  {
    // this file is no local dir, open it, else warn
    bool noDir = !m_args->url(z).isLocalFile() || !TQDir (m_args->url(z).path()).exists();

    if (noDir)
    {
      // open a normal file
      if (codec)
        id = activeMainWindow()->viewManager()->openURL( m_args->url(z), codec->name(), false, tempfileSet );
      else
        id = activeMainWindow()->viewManager()->openURL( m_args->url(z), TQString::null, false, tempfileSet );
    }
    else
      KMessageBox::sorry( activeMainWindow(),
                          i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(m_args->url(z).pathOrURL()) );
  }

  Kate::Document::setOpenErrorDialogsActivated (true);

  // handle stdin input
  if( m_args->isSet( "stdin" ) )
  {
    TQTextIStream input(stdin);

    // set chosen codec
    if (codec)
      input.setCodec (codec);

    TQString line;
    TQString text;

    do
    {
      line = input.readLine();
      text.append( line + "\n" );
    } while( !line.isNull() );

    openInput (text);
  }
  else if ( id )
    activeMainWindow()->viewManager()->activateView( id );

  if ( activeMainWindow()->viewManager()->viewCount () == 0 )
    activeMainWindow()->viewManager()->activateView(m_docManager->firstDocument()->documentNumber());

  int line = 0;
  int column = 0;
  bool nav = false;

  if (m_args->isSet ("line"))
  {
    line = m_args->getOption ("line").toInt();
    nav = true;
  }

  if (m_args->isSet ("column"))
  {
    column = m_args->getOption ("column").toInt();
    nav = true;
  }

  if (nav)
    activeMainWindow()->viewManager()->activeView ()->setCursorPosition (line, column);

  // show the nice tips
  KTipDialog::showTip(activeMainWindow());

  return true;
}

void KateApp::shutdownKate(KateMainWindow *win)
{
  if (!win->queryClose_internal())
    return;

  // detach the dcopClient
  dcopClient()->detach();

  // cu main windows
  while (!m_mainWindows.isEmpty())
    delete m_mainWindows[0];

  quit();
}

bool KateApp::query_session_close()
{
  bool saveSessions = false;
  int switchOption = m_sessionManager->getSwitchOption();
  if (switchOption == KateSessionManager::SWITCH_SAVE)
  {
    saveSessions = true;
  }
  else if (switchOption == KateSessionManager::SWITCH_ASK)
  {
    KDialogBase *dlg = new KDialogBase(i18n("Save Sessions"),
            KDialogBase::Yes | KDialogBase::No | KDialogBase::Cancel,
            KDialogBase::Cancel, KDialogBase::Cancel, NULL, NULL, true, false,
            KStdGuiItem::save(), KStdGuiItem::del(), KStdGuiItem::cancel());
    bool dontAgain = false;
    int res = KMessageBox::createKMessageBox(dlg, TQMessageBox::Warning,
                    i18n("<p>Do you want to save the existing sessions?<p>!!NOTE!!"
                         "<p>All existing sessions will be removed "
                         "if you choose \"Delete\""), TQStringList(),
                    i18n("Do not ask again"), &dontAgain, KMessageBox::Notify);
    if (res == KDialogBase::Cancel)
    {
      return false;
    }
    if (dontAgain)
    {
      if (res == KDialogBase::No)
      {
        m_sessionManager->setSwitchOption(KateSessionManager::SWITCH_DISCARD);
      }
      else
      {
        m_sessionManager->setSwitchOption(KateSessionManager::SWITCH_SAVE);
      }
    }
    if (res == KDialogBase::Yes)
    {
      saveSessions = true;
    }
  }

  if (saveSessions)
  {
    m_sessionManager->saveActiveSession();
  }
  m_sessionManager->saveConfig(saveSessions);
  return true;
}

void KateApp::reparse_config()
{
  emit optionsChanged();
}

KatePluginManager *KateApp::pluginManager()
{
  return m_pluginManager;
}

KateDocManager *KateApp::documentManager ()
{
  return m_docManager;
}

KateSessionManager* KateApp::sessionManager()
{
  return m_sessionManager;
}

bool KateApp::openURL (const KURL &url, const TQString &encoding, bool isTempFile)
{
  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  TQTextCodec *codec = encoding.isEmpty() ? 0 : TQTextCodec::codecForName(encoding.latin1());

  kdDebug () << "OPEN URL "<< encoding << endl;

  // this file is no local dir, open it, else warn
  bool noDir = !url.isLocalFile() || !TQDir (url.path()).exists();

  if (noDir)
  {
    // open a normal file
    if (codec)
      mainWindow->viewManager()->openURL( url, codec->name(), true, isTempFile );
    else
      mainWindow->viewManager()->openURL( url, TQString::null, true, isTempFile );
  }
  else
    KMessageBox::sorry( mainWindow,
                        i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(url.pathOrURL()) );

  return true;
}

bool KateApp::setCursor (int line, int column)
{
  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  mainWindow->viewManager()->activeView ()->setCursorPosition (line, column);

  return true;
}

bool KateApp::openInput (const TQString &text)
{
  activeMainWindow()->viewManager()->openURL( "", "", true );

  if (!activeMainWindow()->viewManager()->activeView ())
    return false;

  activeMainWindow()->viewManager()->activeView ()->getDoc()->setText (text);

  return true;
}

KateMainWindow *KateApp::newMainWindow (TDEConfig *sconfig, const TQString &sgroup)
{
  KateMainWindow *mainWindow = new KateMainWindow (sconfig, sgroup);
  m_mainWindows.push_back (mainWindow);

  if ((mainWindows() > 1) && m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView())
    mainWindow->viewManager()->activateView ( m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView()->getDoc()->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() > 0))
    mainWindow->viewManager()->activateView ( (m_docManager->document(m_docManager->documents()-1))->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() < 1))
    mainWindow->viewManager()->openURL ( KURL() );

  mainWindow->show ();

  return mainWindow;
}

void KateApp::removeMainWindow (KateMainWindow *mainWindow)
{
  m_mainWindows.remove (mainWindow);
}

KateMainWindow *KateApp::activeMainWindow ()
{
  if (m_mainWindows.isEmpty())
    return 0;

  int n = m_mainWindows.findIndex ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return m_mainWindows[n];
}

uint KateApp::mainWindows () const
{
  return m_mainWindows.size();
}

KateMainWindow *KateApp::mainWindow (uint n)
{
  if (n < m_mainWindows.size())
    return m_mainWindows[n];

  return 0;
}
