/* This file is part of the KDE project
   Copyright (C) 2015-2016 Michele Calgaro <micheleDOTcalgaro__AT__yahooDOTit>
   partially based on previous work from
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#include "katesession.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "katedocmanager.h"

#include <kstandarddirs.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <tdemessagebox.h>
#include <kmdcodec.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>
#include <tdepopupmenu.h>

#include <tqdir.h>
#include <tqfile.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqvbox.h>
#include <tqhbox.h>
#include <tqdatetime.h>
#include <tqmap.h>

#include <unistd.h>
#include <time.h>

// FIXME general: need to keep doc list and current session's m_documents in synchro
//       all the time (doc open, doc closed, doc renamed)
// FIXME add code to handle the various options in Configure Kate -> Application -> Sessions

// String constants
namespace
{
  // Kate session
  const char *KS_COUNT    = "Count";
  const char *KS_DOCCOUNT = "Document count";
  const char *KS_DOCLIST  = "Document list";
  const char *KS_GENERAL  = "General";
  const char *KS_NAME     = "Name";
  const char *KS_OPENDOC  = "Open Documents";
  const char *KS_READONLY = "ReadOnly";
  const char *KS_UNNAMED  = "Unnamed";
  const char *KS_OPEN_MAINWINDOWS = "Open MainWindows";

  // Kate session manager
  const char *KSM_DIR     = "kate/sessions";
  const char *KSM_FILE    = "sessions.list";
  const char *KSM_SESSIONS_COUNT = "Sessions count";
  const char *KSM_ACTIVE_SESSION_ID = "Active session id";
  const char *KSM_SESSIONS_LIST  = "Sessions list";
}

//BEGIN Kate session
KateSession::KateSession(const TQString &sessionName, const TQString &filename, bool isFullName) :
  m_sessionName(sessionName), m_filename(filename), m_isFullName(isFullName),
  m_readOnly(false), m_documents(), m_config(NULL)
{
  if (m_isFullName && TDEGlobal::dirs()->exists(m_filename))
  {
    // Create config object if the session file already exists
    m_config = new KSimpleConfig(m_filename, m_readOnly);
    m_config->setGroup(KS_GENERAL);
    // Session name
    if (m_sessionName.isEmpty())
    {
      m_sessionName = m_config->readEntry(KS_NAME, i18n(KS_UNNAMED));
    }
    // Read only
    m_readOnly = m_config->readBoolEntry(KS_READONLY, false);
    m_config->setReadOnly(m_readOnly);
    // Document list
    if (m_config->hasGroup(KS_DOCLIST))
    {
      // Read new style document list (from TDE R14.1.0)
      m_config->setGroup(KS_DOCLIST);
      int docCount = m_config->readNumEntry(KS_DOCCOUNT, 0);
      for (int i = 0;  i < docCount;  ++i)
      {
        TQString urlStr = m_config->readEntry(TQString("URL_%1").arg(i));
        if (!urlStr.isEmpty())
        {
          // Filter out empty URLs
          m_documents.append(urlStr);
        }
      }
    }
    else
    {
      // Create document list from old session configuration
      // to effortlessly import existing sessions
      m_config->setGroup(KS_OPENDOC);
      int docCount = m_config->readNumEntry(KS_COUNT, 0);
      for (int i = 0;  i < docCount;  ++i)
      {
        m_config->setGroup(TQString("Document %1").arg(i));
        TQString urlStr = m_config->readEntry("URL");
        if (!urlStr.isEmpty())
        {
          // Filter out empty URLs
          m_documents.append(urlStr);
        }
      }
    }
  }
  if (m_sessionName.isEmpty())
  {
    m_sessionName = i18n(KS_UNNAMED);
  }
  // FIXME: needs to make sure doc list and m_documents are in synchro here
}

//------------------------------------
KateSession::~KateSession()
{
  if (m_config)
  {
    delete m_config;
  }
}

//------------------------------------
void KateSession::setSessionName(const TQString &sessionName)
{
  m_sessionName = sessionName.isEmpty() ? i18n(KS_UNNAMED) : sessionName;
}

//------------------------------------
void KateSession::setReadOnly(bool readOnly)
{
  m_readOnly = readOnly;
  if (m_config)
  {
    m_config->setReadOnly(m_readOnly);
  }
}

//------------------------------------
void KateSession::save(bool saveGUIInfo)
{
  if (m_readOnly)
  {
    return;
  }

  // create a new session filename if needed
  if (!m_isFullName)
  {
    int s = time(0);
    TQCString tname;
    TQString tmpName;
    while (true)
    {
      tname.setNum(s++);
      KMD5 md5(tname);
      tmpName = m_filename + TQString("%1.katesession").arg(md5.hexDigest().data());
      if (!TDEGlobal::dirs()->exists(tmpName))
      {
        m_filename = tmpName;
        m_isFullName = true;
        break;
      }
    }
  }

  // save session config info
  if (!m_config)
  {
    m_config = new KSimpleConfig(m_filename);
  }

  if (m_config->hasGroup(KS_GENERAL))
  {
    m_config->deleteGroup(KS_GENERAL);
  }
  m_config->setGroup(KS_GENERAL);
  m_config->writeEntry(KS_NAME, m_sessionName);
  m_config->writeEntry(KS_READONLY, m_readOnly);

  if (m_config->hasGroup(KS_DOCLIST))
  {
    m_config->deleteGroup(KS_DOCLIST);
  }
  m_config->setGroup(KS_DOCLIST);
  m_config->writeEntry(KS_DOCCOUNT, m_documents.count());
  for (int i = 0;  i < (int)m_documents.count();  ++i)
  {
    m_config->writeEntry(TQString("URL_%1").arg(i), m_documents[i]);
  }

  // save GUI elements info
  if (saveGUIInfo)
  {
    KateDocManager::self()->saveDocumentList(m_config);
    // save main windows info
    int mwCount = KateApp::self()->mainWindows();
    m_config->setGroup(KS_OPEN_MAINWINDOWS);
    m_config->writeEntry(KS_COUNT, mwCount);
    for (int i = 0;  i < mwCount;  ++i)
    {
      m_config->setGroup(TQString("MainWindow%1").arg(i));
      KateApp::self()->mainWindow(i)->saveProperties(m_config);
    }
  }

  m_config->sync();
}

//------------------------------------
void KateSession::activate()
{
  if (KateDocManager::self()->documents() > 0)
  {
    KateDocManager::self()->closeAllDocuments();
  }
  Kate::Document::setOpenErrorDialogsActivated(false);
  if (m_config)
  {
    KateApp::self()->documentManager()->restoreDocumentList(m_config);
  }

  // load main windows info, if it exists
  if (m_config && m_config->hasGroup(KS_OPEN_MAINWINDOWS))
  {
    m_config->setGroup(KS_OPEN_MAINWINDOWS);
    int mwCount = m_config->readUnsignedNumEntry(KS_COUNT, 1);
    for (int i=0; i<mwCount; ++i)
    {
      if (i >= (int)KateApp::self()->mainWindows())
      {
        KateApp::self()->newMainWindow(m_config, TQString("MainWindow%1").arg(i));
      }
      else
      {
        m_config->setGroup(TQString("MainWindow%1").arg(i));
        KateApp::self()->mainWindow(i)->readProperties(m_config);
      }
    }
  }

  Kate::Document::setOpenErrorDialogsActivated(true);
}
//END Kate session


//BEGIN KateSessionManager
//------------------------------------
KateSessionManager *KateSessionManager::ksm_instance = NULL;

//------------------------------------
KateSessionManager* KateSessionManager::self()
{
  if (!KateSessionManager::ksm_instance)
  {
    KateSessionManager::ksm_instance = new KateSessionManager();
  }
  return KateSessionManager::ksm_instance;
}

//------------------------------------
KateSessionManager::KateSessionManager() :
  m_baseDir(locateLocal("data", KSM_DIR)+"/"), m_configFile(m_baseDir + KSM_FILE),
  m_activeSessionId(0), m_firstActivation(true), m_sessions(), m_config(NULL)
{
  m_sessions.setAutoDelete(true);

  int sessionsCount = 0;
  if (TDEGlobal::dirs()->exists(m_configFile))
  {
    // Read new style configuration (from TDE R14.1.0)
    m_config = new KSimpleConfig(m_configFile);
    m_config->setGroup(KSM_SESSIONS_LIST);
    sessionsCount = m_config->readNumEntry(KSM_SESSIONS_COUNT, 0);
    m_activeSessionId = m_config->readNumEntry(KSM_ACTIVE_SESSION_ID, 0);
    for (int i = 0;  i < sessionsCount;  ++i)
    {
      TQString urlStr = m_config->readEntry(TQString("URL_%1").arg(i));
      if (!urlStr.isEmpty() && TDEGlobal::dirs()->exists(urlStr))
      {
        // Filter out empty URLs or non existing sessions
        m_sessions.append(new KateSession(TQString::null, urlStr, true));
      }
    }
  }
  else
  {
    // Create sessions list from session files
    // to effortlessly import existing sessions
    TQDir sessionDir(m_baseDir, "*.katesession");
    for (unsigned int i = 0;  i < sessionDir.count();  ++i)
    {
      m_sessions.append(new KateSession(TQString::null, m_baseDir+sessionDir[i], true));
    }
  }
  sessionsCount = (int)m_sessions.count();
  if (sessionsCount == 0)  // In the worst case, there is no valid session at all
  {
    m_sessions.append(new KateSession(TQString::null, m_baseDir, false));
  }
  if (m_activeSessionId < 0 || m_activeSessionId >= (int)m_sessions.count())
  {
    m_activeSessionId = 0;  // Invalid active session was detected. Use first in the list
  }
  //NOTE do not activate any session in the KateSessionManager costructor
  //     since Kate's main window may not be ready yet. The initial session
  //     will be activated by KateApp::startupKate() or void KateApp::restoreKate()
}

//------------------------------------
KateSessionManager::~KateSessionManager()
{
  saveConfig();
  if (m_config)
  {
    delete m_config;
  }
  if (!m_sessions.isEmpty())
  {
    m_sessions.clear();
  }
}

//------------------------------------
// FIXME Unnamed sessions should not be saved by default, to allow users who do not bother
// about sessions to open-use-close Kate seemlessly.
// FIXME An option need to be added to Configure Kate -> Sessions to allow Kate to ask about
// saving unnamed sessions before closing the current session. Default value is off as per
// point above.

void KateSessionManager::saveConfig()
{
  if (!m_config)
  {
    m_config = new KSimpleConfig(m_configFile);
  }
  if (m_config->hasGroup(KSM_SESSIONS_LIST))
  {
    m_config->deleteGroup(KSM_SESSIONS_LIST);
  }
  m_config->setGroup(KSM_SESSIONS_LIST);
  m_config->writeEntry(KSM_SESSIONS_COUNT, m_sessions.count());
  m_config->writeEntry(KSM_ACTIVE_SESSION_ID, m_activeSessionId);
  for (int i = 0;  i < (int)m_sessions.count();  ++i)
  {
    // Save the session first, to make sure a new session has an associated file
    m_sessions[i]->save(false);
    m_config->writeEntry(TQString("URL_%1").arg(i), m_sessions[i]->getSessionFilename());
  }
  m_config->sync();
}

//------------------------------------
int KateSessionManager::getSessionIdFromName(const TQString &name)
{
  if (name.isEmpty())
    return KateSessionManager::INVALID_SESSION;

  for (int i = 0;  i < (int)m_sessions.count();  ++i)
  {
    if (m_sessions[i]->getSessionName() == name)
      return i;
  }

  return KateSessionManager::INVALID_SESSION;
}

//------------------------------------
bool KateSessionManager::activateSession(int sessionId, bool saveCurr)
{
  if (sessionId < 0)
  {
    return false;
  }

  if (!m_firstActivation && sessionId == m_activeSessionId)
  {
    return true;
  }

  if (!m_firstActivation)
  {
    // Do this only if a session has already been activated earlier,
    if (KateApp::self()->activeMainWindow())
    {
      // First check if all documents can be closed safely
      if (!KateApp::self()->activeMainWindow()->queryClose_internal())
        return false;
    }
    if (saveCurr && m_activeSessionId != INVALID_SESSION)
    {
      m_sessions[m_activeSessionId]->save(true);
    }
  }

  int oldSessionId = m_activeSessionId;
  m_activeSessionId = sessionId;
  m_sessions[sessionId]->activate();
  m_firstActivation = false;
  emit sessionActivated(m_activeSessionId, oldSessionId);
  return true;
}

//------------------------------------
int KateSessionManager::newSession(const TQString &sessionName, bool activate)
{
  m_sessions.append(new KateSession(sessionName, m_baseDir, false));
  int newSessionId = m_sessions.count() - 1;
  emit sessionCreated(newSessionId);
  if (activate)
  {
    activateSession(newSessionId, m_activeSessionId != INVALID_SESSION);
  }
  return newSessionId;
}

//------------------------------------
bool KateSessionManager::restoreLastSession()
{
  if (!m_firstActivation)
  {
    return false;
  }
  // NOTE: m_activeSessionId contains the id of the last active session
  return activateSession(m_activeSessionId, false);
}

//-------------------------------------------
bool KateSessionManager::deleteSession(int sessionId)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return false;
  }

  // delete session file if it exists
  const TQString &filename = m_sessions[sessionId]->getSessionFilename();
  if (filename != TQString::null && TQFile::exists(filename))
  {
    TQFile::remove(filename);
  }
  // delete session
  m_sessions.remove(sessionId);  // this also deletes the KateSession item since auto-deletion is enabled
  if (m_activeSessionId > sessionId)
  {
    --m_activeSessionId;
  }
  else if (m_activeSessionId == sessionId)
  {
    m_activeSessionId = INVALID_SESSION;
  }
  emit sessionDeleted(sessionId);
  // if the active session was deleted, create a new unnamed session and activate it
  if (m_activeSessionId == INVALID_SESSION)
  {
    newSession();
  }

  return true;
}

//-------------------------------------------
void KateSessionManager::swapSessionsPosition(int sessionId1, int sessionId2)
{
  if (sessionId1 < 0 || sessionId1 >= (int)m_sessions.count() ||
      sessionId2 < 0 || sessionId2 >= (int)m_sessions.count() ||
      sessionId1 == sessionId2)
  {
    return;
  }

  int idxMin, idxMax;
  if (sessionId1 < sessionId2)
  {
    idxMin = sessionId1;
    idxMax = sessionId2;
  }
  else
  {
    idxMin = sessionId2;
    idxMax = sessionId1;
  }

  KateSession *sessMax = m_sessions.take(idxMax);
  KateSession *sessMin = m_sessions.take(idxMin);
  m_sessions.insert(idxMin, sessMax);
  m_sessions.insert(idxMax, sessMin);
  if (m_activeSessionId == sessionId1)
  {
    m_activeSessionId = sessionId2;
  }
  else if (m_activeSessionId == sessionId2)
  {
    m_activeSessionId = sessionId1;
  }

  emit sessionsSwapped(idxMin, idxMax);
}

//-------------------------------------------
void KateSessionManager::moveSessionForward(int sessionId)
{
  if (sessionId < 0 || sessionId >= ((int)m_sessions.count() - 1))
  {
    return;
  }

  swapSessionsPosition(sessionId, sessionId + 1);
}

//-------------------------------------------
void KateSessionManager::moveSessionBackward(int sessionId)
{
  if (sessionId < 1 || sessionId >= (int)m_sessions.count())
  {
    return;
  }

  swapSessionsPosition(sessionId, sessionId - 1);
}

//-------------------------------------------
void KateSessionManager::renameSession(int sessionId, const TQString &newSessionName)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return;
  }

  m_sessions[sessionId]->setSessionName(newSessionName);
}
//END KateSessionManager


//BEGIN KateSessionChooser
//-------------------------------------------
KateSessionChooser::KateSessionChooser(TQWidget *parent)
 : KDialogBase(parent, "", true, i18n("Session Chooser"),
               KDialogBase::User1 | KDialogBase::User2 | KDialogBase::User3, KDialogBase::User2,
               true, KStdGuiItem::quit(), KGuiItem(i18n("Open Session"), "document-open"),
               KGuiItem(i18n("New Session"), "document-new")), m_listview(NULL)
{
  TQHBox *page = new TQHBox(this);
  page->setMinimumSize(400, 200);
  setMainWidget(page);

  TQHBox *hb = new TQHBox(page);
  hb->setSpacing(KDialog::spacingHint());

  TQLabel *label = new TQLabel(hb);
  label->setPixmap(UserIcon("sessionchooser"));
  label->setFrameStyle (TQFrame::Panel | TQFrame::Sunken);

  TQVBox *vb = new TQVBox(hb);
  vb->setSpacing (KDialog::spacingHint());

  m_listview = new TDEListView(vb);
  m_listview->addColumn(i18n("Session Name"));
  m_listview->addColumn(i18n("Open Documents"));
  m_listview->setSelectionMode(TQListView::Single);
  m_listview->setAllColumnsShowFocus(true);
  m_listview->setSorting(-1);
  m_listview->setResizeMode(TQListView::LastColumn);

  connect (m_listview, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(slotSelectionChanged()));
  connect (m_listview, TQT_SIGNAL(executed(TQListViewItem*)), this, TQT_SLOT(slotUser2()));

  TQPtrList<KateSession>& sessions = KateSessionManager::self()->getSessionsList();
  for (int idx = sessions.count()-1;  idx >= 0;  --idx)
  {
    new KateSessionChooserItem(m_listview, sessions[idx]->getSessionName(),
                               TQString("%1").arg(sessions[idx]->getDocCount()), idx);
  }

  setResult(RESULT_NO_OP);
  slotSelectionChanged();  // update button status
}

//-------------------------------------------
int KateSessionChooser::getSelectedSessionId()
{
  KateSessionChooserItem *selectedItem = dynamic_cast<KateSessionChooserItem*>(m_listview->selectedItem());
  if (!selectedItem)
    return KateSessionManager::INVALID_SESSION;

  return selectedItem->getSessionId();
}

//-------------------------------------------
void KateSessionChooser::slotUser1()
{
  done(RESULT_QUIT_KATE);
}

//-------------------------------------------
void KateSessionChooser::slotUser2()
{
  done(RESULT_OPEN_EXISTING);
}

//-------------------------------------------
void KateSessionChooser::slotUser3()
{
  done(RESULT_OPEN_NEW);
}

//-------------------------------------------
void KateSessionChooser::slotSelectionChanged()
{
  enableButton(KDialogBase::User2, m_listview->selectedItem());
}
//END KateSessionChooser





//------------------------------------
//------------------------------------
//------------------------------------
//------------------------------------
OldKateSession::OldKateSession (OldKateSessionManager *manager, const TQString &fileName, const TQString &name)
  : m_sessionFileRel (fileName)
  , m_sessionName (name)
  , m_documents (0)
  , m_manager (manager)
  , m_readConfig (0)
  , m_writeConfig (0)
{
  init ();
}

void OldKateSession::init ()
{
  // given file exists, use it to load some stuff ;)
  if (!m_sessionFileRel.isEmpty() && TDEGlobal::dirs()->exists(sessionFile ()))
  {
    KSimpleConfig config (sessionFile (), true);

    if (m_sessionName.isEmpty())
    {
      // get the name out of the file
      if (m_sessionFileRel == "default.katesession")
        m_sessionName = i18n("Default Session");
      else
      {
        config.setGroup ("General");
        m_sessionName = config.readEntry ("Name", i18n ("Unnamed Session"));
      }
    }

    // get the document count
    config.setGroup ("Open Documents");
    m_documents = config.readUnsignedNumEntry("Count", 0);

    return;
  }

  // filename not empty, create the file
  // anders: When will this ever happen???
  if (!m_sessionFileRel.isEmpty())
  {
    kdDebug(13001)<<"Kate::Session: initializing unexisting file!"<<endl;
     // uhh, no name given
    if (m_sessionName.isEmpty())
    {
      if (m_sessionFileRel == "default.katesession")
        m_sessionName = i18n("Default Session");
      else
        m_sessionName = i18n("Session (%1)").arg(TQTime::currentTime().toString(Qt::LocalDate));
    }

    // create the file, write name to it!
    KSimpleConfig config (sessionFile ());
    config.setGroup ("General");
    config.writeEntry ("Name", m_sessionName);

    config.sync ();
  }
}

OldKateSession::~OldKateSession ()
{
  delete m_readConfig;
  delete m_writeConfig;
}

TQString OldKateSession::sessionFile () const
{
  return m_manager->sessionsDir() + "/" + m_sessionFileRel;
}

bool OldKateSession::create (const TQString &name, bool force)
{
  if (!force && (name.isEmpty() || !m_sessionFileRel.isEmpty()))
    return false;

  delete m_writeConfig;
  m_writeConfig = 0;

  delete m_readConfig;
  m_readConfig = 0;

  m_sessionName = name;

  // get a usable filename
  int s = time(0);
  TQCString tname;
  while (true)
  {
    tname.setNum (s++);
    KMD5 md5 (tname);
    m_sessionFileRel = TQString ("%1.katesession").arg (md5.hexDigest().data());

    if (!TDEGlobal::dirs()->exists(sessionFile ()))
      break;
  }

  // create the file, write name to it!
  KSimpleConfig config (sessionFile ());
  config.setGroup ("General");
  config.writeEntry ("Name", m_sessionName);
  config.sync ();

  // reinit ourselfs ;)
  init ();

  return true;
}

bool OldKateSession::rename (const TQString &name)
{
  if (name.isEmpty () || m_sessionFileRel.isEmpty() || m_sessionFileRel == "default.katesession")
    return false;

  m_sessionName = name;

  TDEConfig config (sessionFile (), false, false);
  config.setGroup ("General");
  config.writeEntry ("Name", m_sessionName);
  config.sync ();

  return true;
}

TDEConfig *OldKateSession::configRead ()
{
  if (m_sessionFileRel.isEmpty())
    return 0;

  if (m_readConfig)
    return m_readConfig;

  return m_readConfig = new KSimpleConfig (sessionFile (), true);
}

TDEConfig *OldKateSession::configWrite ()
{
  if (m_sessionFileRel.isEmpty())
    return 0;

  if (m_writeConfig)
    return m_writeConfig;

  m_writeConfig = new KSimpleConfig (sessionFile ());
  m_writeConfig->setGroup ("General");
  m_writeConfig->writeEntry ("Name", m_sessionName);

  return m_writeConfig;
}

OldKateSessionManager::OldKateSessionManager (TQObject *parent)
 : TQObject (parent)
 , m_sessionsDir (locateLocal( "data", "kate/sessions"))
 , m_activeSession (new OldKateSession (this, "", ""))
{
  kdDebug() << "LOCAL SESSION DIR: " << m_sessionsDir << endl;

  // create dir if needed
  TDEGlobal::dirs()->makeDir (m_sessionsDir);
}

OldKateSessionManager::~OldKateSessionManager()
{
}

OldKateSessionManager *OldKateSessionManager::self()
{
  return (OldKateSessionManager*)KateApp::self()->sessionManager();
}

void OldKateSessionManager::dirty (const TQString &)
{
  updateSessionList ();
}

void OldKateSessionManager::updateSessionList ()
{
  m_sessionList.clear ();

  // Let's get a list of all session we have atm
  TQDir dir (m_sessionsDir, "*.katesession");

  bool foundDefault = false;
  for (unsigned int i=0; i < dir.count(); ++i)
  {
    OldKateSession *session = new OldKateSession (this, dir[i], "");
    m_sessionList.append (session);

    kdDebug () << "FOUND SESSION: " << session->sessionName() << " FILE: " << session->sessionFile() << endl;

    if (!foundDefault && (dir[i] == "default.katesession"))
      foundDefault = true;
  }

  // add default session, if not there
  if (!foundDefault)
    m_sessionList.append (new OldKateSession (this, "default.katesession", i18n("Default Session")));

  qHeapSort(m_sessionList);
}

void OldKateSessionManager::activateSession (OldKateSession::Ptr session, bool closeLast, bool saveLast, bool loadNew)
{
  // don't reload.
  // ### comparing the pointers directly is b0rk3d :(
   if ( ! session->sessionName().isEmpty() && session->sessionName() == m_activeSession->sessionName() )
     return;
  // try to close last session
  if (closeLast)
  {
    if (KateApp::self()->activeMainWindow())
    {
      if (!KateApp::self()->activeMainWindow()->queryClose_internal())
        return;
    }
  }

  // save last session or not?
  if (saveLast)
    saveActiveSession (true);

  // really close last
  if (closeLast)
  {
    KateDocManager::self()->closeAllDocuments ();
  }

  // set the new session
  m_activeSession = session;

  if (loadNew)
  {
    // open the new session
    Kate::Document::setOpenErrorDialogsActivated (false);

    TDEConfig *sc = activeSession()->configRead();

    if (sc)
      KateApp::self()->documentManager()->restoreDocumentList (sc);

    // if we have no session config object, try to load the default
    // (anonymous/unnamed sessions)
    if ( ! sc )
      sc = new KSimpleConfig( sessionsDir() + "/default.katesession" );

    // window config
    if (sc)
    {
      TDEConfig *c = KateApp::self()->config();
      c->setGroup("General");

      if (c->readBoolEntry("Restore Window Configuration", true))
      {
        // a new, named session, read settings of the default session.
        if ( ! sc->hasGroup("Open MainWindows") )
          sc = new KSimpleConfig( sessionsDir() + "/default.katesession" );

        sc->setGroup ("Open MainWindows");
        unsigned int wCount = sc->readUnsignedNumEntry("Count", 1);

        for (unsigned int i=0; i < wCount; ++i)
        {
          if (i >= KateApp::self()->mainWindows())
          {
            KateApp::self()->newMainWindow(sc, TQString ("MainWindow%1").arg(i));
          }
          else
          {
            sc->setGroup(TQString ("MainWindow%1").arg(i));
            KateApp::self()->mainWindow(i)->readProperties (sc);
          }
        }

        if (wCount > 0)
        {
          while (wCount < KateApp::self()->mainWindows())
          {
            KateMainWindow *w = KateApp::self()->mainWindow(KateApp::self()->mainWindows()-1);
            KateApp::self()->removeMainWindow (w);
            delete w;
          }
        }
      }
    }

    Kate::Document::setOpenErrorDialogsActivated (true);
  }
}

OldKateSession::Ptr OldKateSessionManager::createSession (const TQString &name)
{
  OldKateSession::Ptr s = new OldKateSession (this, "", "");
  s->create (name);

  return s;
}

OldKateSession::Ptr OldKateSessionManager::giveSession (const TQString &name)
{
  if (name.isEmpty())
    return new OldKateSession (this, "", "");

  updateSessionList();

  for (unsigned int i=0; i < m_sessionList.count(); ++i)
  {
    if (m_sessionList[i]->sessionName() == name)
      return m_sessionList[i];
  }

  return createSession (name);
}

bool OldKateSessionManager::saveActiveSession (bool tryAsk, bool rememberAsLast)
{
  if (tryAsk)
  {
    // app config
    TDEConfig *c = KateApp::self()->config();
    c->setGroup("General");

    TQString sesExit (c->readEntry ("Session Exit", "save"));

    if (sesExit == "discard")
      return true;

    if (sesExit == "ask")
    {
      KDialogBase* dlg = new KDialogBase(i18n ("Save Session?")
                     , KDialogBase::Yes | KDialogBase::No
                     , KDialogBase::Yes, KDialogBase::No
                     );

      bool dontAgain = false;
      int res = KMessageBox::createKMessageBox(dlg, TQMessageBox::Question,
                              i18n("Save current session?"), TQStringList(),
                              i18n("Do not ask again"), &dontAgain, KMessageBox::Notify);

      // remember to not ask again with right setting
      if (dontAgain)
      {
        c->setGroup("General");

        if (res == KDialogBase::No)
          c->writeEntry ("Session Exit", "discard");
        else
          c->writeEntry ("Session Exit", "save");
      }

      if (res == KDialogBase::No)
        return true;
    }
  }

  TDEConfig *sc = activeSession()->configWrite();

  if (!sc)
    return false;

  KateDocManager::self()->saveDocumentList (sc);

  sc->setGroup ("Open MainWindows");
  sc->writeEntry ("Count", KateApp::self()->mainWindows ());

  // save config for all windows around ;)
  for (unsigned int i=0; i < KateApp::self()->mainWindows (); ++i )
  {
    sc->setGroup(TQString ("MainWindow%1").arg(i));
    KateApp::self()->mainWindow(i)->saveProperties (sc);
  }

  sc->sync();

  if (rememberAsLast)
  {
    TDEConfig *c = KateApp::self()->config();
    c->setGroup("General");
    c->sync ();
  }

  return true;
}

void OldKateSessionManager::sessionNew ()
{
  activateSession (new OldKateSession (this, "", ""));
}

void OldKateSessionManager::sessionOpen ()
{
  OldKateSessionOpenDialog *chooser = new OldKateSessionOpenDialog (0);

  int res = chooser->exec ();

  if (res == OldKateSessionOpenDialog::resultCancel)
  {
    delete chooser;
    return;
  }

  OldKateSession::Ptr s = chooser->selectedSession ();

  if (s)
    activateSession (s);

  delete chooser;
}

void OldKateSessionManager::sessionSave ()
{
  // if the active session is valid, just save it :)
  if (saveActiveSession ())
    return;

  bool ok = false;
  TQString name = KInputDialog::getText (i18n("Specify Name for Current Session"), i18n("Session name:"), "", &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To save a new session, you must specify a name."), i18n ("Missing Session Name"));
    return;
  }

  activeSession()->create (name);
  saveActiveSession ();
}

void OldKateSessionManager::sessionSaveAs ()
{
  bool ok = false;
  TQString name = KInputDialog::getText (i18n("Specify New Name for Current Session"), i18n("Session name:"), "", &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));
    return;
  }

  activeSession()->create (name, true);
  saveActiveSession ();
}


void OldKateSessionManager::sessionManage ()
{
  OldKateSessionManageDialog *dlg = new OldKateSessionManageDialog (0);

  dlg->exec ();

  delete dlg;
}

//BEGIN CHOOSER DIALOG

class OldKateSessionChooserItem : public TQListViewItem
{
  public:
    OldKateSessionChooserItem (TDEListView *lv, OldKateSession::Ptr s)
     : TQListViewItem (lv, s->sessionName())
     , session (s)
    {
      TQString docs;
      docs.setNum (s->documents());
      setText (1, docs);
    }

    OldKateSession::Ptr session;
};


//END CHOOSER DIALOG

//BEGIN OPEN DIALOG

OldKateSessionOpenDialog::OldKateSessionOpenDialog (TQWidget *parent)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Open Session")
                  , KDialogBase::User1 | KDialogBase::User2
                  , KDialogBase::User2
                  , false
                  , KStdGuiItem::cancel ()
                  , KGuiItem( i18n("&Open"), "document-open")
                )
{
  TQHBox *page = new TQHBox (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  TQHBox *hb = new TQHBox (page);

  TQVBox *vb = new TQVBox (hb);

  m_sessions = new TDEListView (vb);
  m_sessions->addColumn (i18n("Session Name"));
  m_sessions->addColumn (i18n("Open Documents"));
  m_sessions->setResizeMode (TQListView::AllColumns);
  m_sessions->setSelectionMode (TQListView::Single);
  m_sessions->setAllColumnsShowFocus (true);

  connect (m_sessions, TQT_SIGNAL(doubleClicked(TQListViewItem *, const TQPoint &, int)), this, TQT_SLOT(slotUser2()));

  OldKateSessionList &slist (OldKateSessionManager::self()->sessionList());
  for (unsigned int i=0; i < slist.count(); ++i)
  {
    new OldKateSessionChooserItem (m_sessions, slist[i]);
  }

  setResult (resultCancel);
}

OldKateSessionOpenDialog::~OldKateSessionOpenDialog ()
{
}

OldKateSession::Ptr OldKateSessionOpenDialog::selectedSession ()
{
  OldKateSessionChooserItem *item = (OldKateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item)
    return 0;

  return item->session;
}

void OldKateSessionOpenDialog::slotUser1 ()
{
  done (resultCancel);
}

void OldKateSessionOpenDialog::slotUser2 ()
{
  done (resultOk);
}

//END OPEN DIALOG

//BEGIN MANAGE DIALOG

OldKateSessionManageDialog::OldKateSessionManageDialog (TQWidget *parent)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Manage Sessions")
                  , KDialogBase::User1
                  , KDialogBase::User1
                  , false
                  , KStdGuiItem::close ()
                )
{
  TQHBox *page = new TQHBox (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  TQHBox *hb = new TQHBox (page);
  hb->setSpacing (KDialog::spacingHint());

  m_sessions = new TDEListView (hb);
  m_sessions->addColumn (i18n("Session Name"));
  m_sessions->addColumn (i18n("Open Documents"));
  m_sessions->setResizeMode (TQListView::AllColumns);
  m_sessions->setSelectionMode (TQListView::Single);
  m_sessions->setAllColumnsShowFocus (true);

  connect (m_sessions, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(selectionChanged()));

  updateSessionList ();

  TQWidget *vb = new TQWidget (hb);
  TQVBoxLayout *vbl = new TQVBoxLayout (vb);
  vbl->setSpacing (KDialog::spacingHint());

  m_rename = new KPushButton (i18n("&Rename..."), vb);
  connect (m_rename, TQT_SIGNAL(clicked()), this, TQT_SLOT(rename()));
  vbl->addWidget (m_rename);

  m_del = new KPushButton (KStdGuiItem::del (), vb);
  connect (m_del, TQT_SIGNAL(clicked()), this, TQT_SLOT(del()));
  vbl->addWidget (m_del);

  vbl->addStretch ();

  // trigger action update
  selectionChanged ();
}

OldKateSessionManageDialog::~OldKateSessionManageDialog ()
{
}

void OldKateSessionManageDialog::slotUser1 ()
{
  done (0);
}


void OldKateSessionManageDialog::selectionChanged ()
{
  OldKateSessionChooserItem *item = (OldKateSessionChooserItem *) m_sessions->selectedItem ();

  m_rename->setEnabled (item && item->session->sessionFileRelative() != "default.katesession");
  m_del->setEnabled (item && item->session->sessionFileRelative() != "default.katesession");
}

void OldKateSessionManageDialog::rename ()
{
  OldKateSessionChooserItem *item = (OldKateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item || item->session->sessionFileRelative() == "default.katesession")
    return;

  bool ok = false;
  TQString name = KInputDialog::getText (i18n("Specify New Name for Session"), i18n("Session name:"), item->session->sessionName(), &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));
    return;
  }

  item->session->rename (name);
  updateSessionList ();
}

void OldKateSessionManageDialog::del ()
{
  OldKateSessionChooserItem *item = (OldKateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item || item->session->sessionFileRelative() == "default.katesession")
    return;

  TQFile::remove (item->session->sessionFile());
  OldKateSessionManager::self()->updateSessionList ();
  updateSessionList ();
}

void OldKateSessionManageDialog::updateSessionList ()
{
  m_sessions->clear ();

  OldKateSessionList &slist (OldKateSessionManager::self()->sessionList());
  for (unsigned int i=0; i < slist.count(); ++i)
  {
    new OldKateSessionChooserItem (m_sessions, slist[i]);
  }
}

//END MANAGE DIALOG


OldKateSessionsAction::OldKateSessionsAction(const TQString& text, TQObject* parent, const char* name )
  : TDEActionMenu(text, parent, name)
{
  connect(popupMenu(),TQT_SIGNAL(aboutToShow()),this,TQT_SLOT(slotAboutToShow()));
}

void OldKateSessionsAction::slotAboutToShow()
{
  popupMenu()->clear ();

  OldKateSessionList &slist (OldKateSessionManager::self()->sessionList());
  for (unsigned int i=0; i < slist.count(); ++i)
  {
      popupMenu()->insertItem (
          slist[i]->sessionName(),
          this, TQT_SLOT (openSession (int)), 0,
          i );
  }
}

void OldKateSessionsAction::openSession (int i)
{
  OldKateSessionList &slist (OldKateSessionManager::self()->sessionList());

  if ((uint)i >= slist.count())
    return;

  OldKateSessionManager::self()->activateSession(slist[(uint)i]);
}

#include "katesession.moc"

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
