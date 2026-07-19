/* This file is part of the KDE project
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

#include <tdestandarddirs.h>
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
//       all the time (doc open, doc closed, doc renamed).
//       To be done when doc list software is developed

// String constants
namespace
{
  // Kate session
  const char *KS_COUNT             = "Count";
  const char *KS_DOCCOUNT          = "Document count";
  const char *KS_DOCLIST           = "Document list";
  const char *KS_GENERAL           = "General";
  const char *KS_NAME              = "Name";
  const char *KS_OPENDOC           = "Open Documents";
  const char *KS_READONLY          = "ReadOnly";
  const char *KS_OPEN_MAINWINDOWS  = "Open MainWindows";
  const char *KS_UNNAMED           = "Unnamed";

  // Kate session manager
  const char *KSM_DIR              = "kate/sessions";
  const char *KSM_FILE             = "sessions.list";
  const char *KSM_SESSIONS_COUNT   = "Sessions count";
  const char *KSM_LAST_SESSION_ID  = "Last session id";
  const char *KSM_SESSIONS_LIST    = "Sessions list";

  // Kate app
  const char *KAPP_GENERAL         = "General";
  const char *KAPP_LAST_SESSION    = "Last Session";
  const char *KAPP_STARTUP_SESSION = "Startup Session";
  const char *KAPP_NEW             = "new";
  const char *KAPP_LAST            = "last";
  const char *KAPP_MANUAL          = "manual";
  const char *KAPP_SESSION_EXIT    = "Session Exit";
  const char *KAPP_DISCARD         = "discard";
  const char *KAPP_SAVE            = "save";
  const char *KAPP_ASK             = "ask";
}

//BEGIN Kate session
KateSession::KateSession(const KateSessionManager &manager, const TQString &sessionName, 
                         const TQString &fileName) :
  m_manager(manager), m_sessionName(sessionName), m_filename(fileName),
  m_readOnly(false), m_documents(), m_config(NULL)
{
  load(false);
}

//------------------------------------
KateSession::KateSession(const KateSession &session, const TQString &newSessionName) :
  m_manager(session.m_manager), m_sessionName(newSessionName), m_filename(),
  m_readOnly(false), m_documents(session.m_documents), m_config(NULL)
{
  createFilename();
  if (session.m_config)
  {
    m_config = new TDESimpleConfig(m_filename);
    session.m_config->copyTo(m_filename, m_config);
    m_config->sync();
  }
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
bool KateSession::isStillVolatile() const
{ 
  return m_filename.isEmpty() && m_sessionName == i18n(KS_UNNAMED); 
}

//------------------------------------
void KateSession::load(bool includeGUIInfo)
{
  if (m_config)
  {
    delete m_config;
  }
  
  if (TDEGlobal::dirs()->exists(m_filename))
  {
    // Create config object if the session file already exists
    m_config = new TDESimpleConfig(m_filename, m_readOnly);
    m_config->setGroup(KS_GENERAL);
    // Session general properties
    m_sessionName = m_config->readEntry(KS_NAME, i18n(KS_UNNAMED));
    m_readOnly = m_config->readBoolEntry(KS_READONLY, false);
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
  else
  {
    m_filename = TQString::null;
  }
  if (m_sessionName.isEmpty())
  {
    m_sessionName = i18n(KS_UNNAMED);
  }
    
  // Update all current documents if necessary
  if (includeGUIInfo)
  {
    activate();
  }
}

//------------------------------------
void KateSession::save(bool saveGUIInfo, bool setReadOnly)
{
  if (m_readOnly && !setReadOnly)
  {
    return;
  }

  // create a new session filename if needed
  if (m_filename.isEmpty())
  {
    createFilename();
  }

  // save session config info
  if (!m_config)
  {
    m_config = new TDESimpleConfig(m_filename);
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
    
    // load main windows info, if it exists
    if (m_config->hasGroup(KS_OPEN_MAINWINDOWS))
    {
      m_config->setGroup(KS_OPEN_MAINWINDOWS);
      int mwCount = m_config->readUnsignedNumEntry(KS_COUNT, 1);
      for (int i = 0;  i < mwCount;  ++i)
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
  }
  Kate::Document::setOpenErrorDialogsActivated(true);
}

//------------------------------------
void KateSession::createFilename()
{
  // create a new session filename if needed
  if (!m_filename.isEmpty())
  {
    return;
  }
  
  int s = time(0);
  TQCString tname;
  TQString tmpName;
  while (true)
  {
    tname.setNum(s++);
    KMD5 md5(tname);
    tmpName = m_manager.getBaseDir() + TQString("%1.katesession").arg(md5.hexDigest().data());
    if (!TDEGlobal::dirs()->exists(tmpName))
    {
      m_filename = tmpName;
      break;
    }
  }
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
  m_activeSessionId(INVALID_SESSION), m_lastSessionId(INVALID_SESSION), m_sessions(),
  m_config(NULL), m_startupOption(STARTUP_NEW), m_switchOption(SWITCH_DISCARD)
{
  // Session startup and switch options
  updateSessionOptions(SO_ALL);
  
  // Sessions configuration
  m_sessions.setAutoDelete(true);
  int sessionsCount = 0;
  if (TDEGlobal::dirs()->exists(m_configFile))
  {
    // Read new style configuration (from TDE R14.1.0)
    m_config = new TDESimpleConfig(m_configFile);
    m_config->setGroup(KSM_SESSIONS_LIST);
    sessionsCount = m_config->readNumEntry(KSM_SESSIONS_COUNT, 0);
    m_lastSessionId = m_config->readNumEntry(KSM_LAST_SESSION_ID, INVALID_SESSION);
    for (int i = 0;  i < sessionsCount;  ++i)
    {
      TQString urlStr = m_config->readEntry(TQString("URL_%1").arg(i));
      if (!urlStr.isEmpty() && TDEGlobal::dirs()->exists(urlStr))
      {
        // Filter out empty URLs or non existing sessions
        m_sessions.append(new KateSession(*this, TQString::null, urlStr));
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
      m_sessions.append(new KateSession(*this, TQString::null, m_baseDir+sessionDir[i]));
    }
  }
  sessionsCount = (int)m_sessions.count();
  if (sessionsCount == 0)  // In the worst case, there is no valid session at all
  {
    m_sessions.append(new KateSession(*this, TQString::null, TQString::null));
  }
  if (m_lastSessionId < 0 || m_lastSessionId >= (int)m_sessions.count())
  {
    m_lastSessionId = 0;  // Invalid last session was detected. Use first in the list
  }
  
  //NOTE do not activate any session in the KateSessionManager costructor
  //     since Kate's main window may not be ready yet. The initial session
  //     will be activated by KateApp::startupKate() or KateApp::restoreKate()
}

//------------------------------------
KateSessionManager::~KateSessionManager()
{
  if (m_config)
  {
    delete m_config;
  }
  m_sessions.clear();
}

//------------------------------------
void KateSessionManager::updateSessionOptions(int optionType)
{
  // Session startup and switch options
  TDEConfig *kateCfg = KateApp::self()->config();
  kateCfg->setGroup(KAPP_GENERAL);
  
  if (optionType == SO_STARTUP || optionType == SO_ALL)
  {
    if (kateCfg->hasKey(KAPP_LAST_SESSION))
    {
      // Delete no longer used entry (pre R14.1.0)
      kateCfg->deleteEntry(KAPP_LAST_SESSION);
    }
    TQString startupOption(kateCfg->readEntry(KAPP_STARTUP_SESSION, KAPP_MANUAL));
    if (startupOption == KAPP_LAST)
    {
      m_startupOption = STARTUP_LAST;
    }
    else if (startupOption == KAPP_NEW)
    {
      m_startupOption = STARTUP_NEW;
    }
    else // startupOption == "manual"
    {
      m_startupOption = STARTUP_MANUAL;
    }
  }
  
  if (optionType == SO_SWITCH || optionType == SO_ALL)
  {
    TQString switchOption(kateCfg->readEntry(KAPP_SESSION_EXIT, KAPP_ASK));
    if (switchOption == KAPP_DISCARD)
    {
      m_switchOption = SWITCH_DISCARD;
    }
    else if (switchOption == KAPP_SAVE)
    {
      m_switchOption = SWITCH_SAVE;
    }
    else // switchOption == "ask"
    {
      m_switchOption = SWITCH_ASK;
    }
  }
}

//------------------------------------
void KateSessionManager::saveSessionOptions(int optionType)
{
  TDEConfig *kateCfg = KateApp::self()->config();
  kateCfg->setGroup(KAPP_GENERAL);
  if (optionType == SO_STARTUP || optionType == SO_ALL)
  {
    if (m_startupOption == STARTUP_LAST)
    {
      kateCfg->writeEntry(KAPP_STARTUP_SESSION, KAPP_LAST);
    }
    else if (m_startupOption == STARTUP_NEW)
    {
      kateCfg->writeEntry(KAPP_STARTUP_SESSION, KAPP_NEW);
    }
    else // m_startupOption == STARTUP_MANUAL
    {
      kateCfg->writeEntry(KAPP_STARTUP_SESSION, KAPP_MANUAL);
    }
  }
  
  if (optionType == SO_SWITCH || optionType == SO_ALL)
  {
    if (m_switchOption == SWITCH_DISCARD)
    {
      kateCfg->writeEntry(KAPP_SESSION_EXIT, KAPP_DISCARD);
    }
    else if (m_switchOption == SWITCH_SAVE)
    {
      kateCfg->writeEntry(KAPP_SESSION_EXIT, KAPP_SAVE);
    }
    else // m_switchOption == SWITCH_ASK
    {
      kateCfg->writeEntry(KAPP_SESSION_EXIT, KAPP_ASK);
    }
  }
  kateCfg->sync();
}

//------------------------------------
void KateSessionManager::saveConfig(bool saveSessions)
{
  // Session startup and switch options
  updateSessionOptions(SO_ALL);
  saveSessionOptions(SO_ALL);
  
  // Sessions configuration
  if (!saveSessions)
  {
    // delete all session files if they exist
    for (int i = 0;  i < (int)m_sessions.count();  ++i)
    {
      const TQString &filename = m_sessions[i]->getSessionFilename();
      if (filename != TQString::null && TQFile::exists(filename))
      {
        TQFile::remove(filename);
      }
    }
  
    m_sessions.clear();
    m_activeSessionId = INVALID_SESSION;
  }
  
  if (!m_config)
  {
    m_config = new TDESimpleConfig(m_configFile);
  }
  if (m_config->hasGroup(KSM_SESSIONS_LIST))
  {
    m_config->deleteGroup(KSM_SESSIONS_LIST);
  }
  m_config->setGroup(KSM_SESSIONS_LIST);
  m_config->writeEntry(KSM_SESSIONS_COUNT, m_sessions.count());
  m_config->writeEntry(KSM_LAST_SESSION_ID, m_activeSessionId);
  for (int i = 0;  i < (int)m_sessions.count();  ++i)
  {
    saveSession(i, false, false);
    m_config->writeEntry(TQString("URL_%1").arg(i), m_sessions[i]->getSessionFilename());
  }
  m_config->sync();
}

//------------------------------------
const int KateSessionManager::getStartupOption()
{
  updateSessionOptions(SO_STARTUP);
  return m_startupOption;
}

//------------------------------------
const int KateSessionManager::getSwitchOption()
{
  updateSessionOptions(SO_SWITCH);
  return m_switchOption;
}

//------------------------------------
void KateSessionManager::setSwitchOption(int option)
{ 
  m_switchOption = (option == SWITCH_DISCARD || option == SWITCH_SAVE) ? option : SWITCH_ASK; 
  saveSessionOptions(SO_SWITCH);
  emit switchOptionChanged();
}

//------------------------------------
const TQString& KateSessionManager::getSessionName(int sessionId)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return TQString::null;
  }

  return m_sessions[sessionId]->getSessionName();
}

//------------------------------------
KateSession* KateSessionManager::getSessionFromId(int sessionId)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return NULL;
  }

  return m_sessions[sessionId];
}

//------------------------------------
int KateSessionManager::getSessionIdFromName(const TQString &name)
{
  if (name.isEmpty())
    return INVALID_SESSION;

  for (int i = 0;  i < (int)m_sessions.count();  ++i)
  {
    if (m_sessions[i]->getSessionName() == name)
      return i;
  }

  return INVALID_SESSION;
}

//------------------------------------
bool KateSessionManager::activateSession(int sessionId, bool saveCurr)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return false;
  }

  if (sessionId == m_activeSessionId)
  {
    return true;
  }

  int oldSessionId = m_activeSessionId;
  if (m_activeSessionId != INVALID_SESSION)
  {
    // Do this only if a session has already been activated earlier,
    if (KateApp::self()->activeMainWindow())
    {
      // First check if all documents can be closed safely
      if (!KateApp::self()->activeMainWindow()->queryCloseAllDocuments())
        return false;
    }
    if (saveCurr) 
    {
      saveSession(m_activeSessionId, true);
    }
    else
    {
      // Delete current session before activating the new one
      const TQString &filename = m_sessions[m_activeSessionId]->getSessionFilename();
      if (filename != TQString::null && TQFile::exists(filename))
      {
        TQFile::remove(filename);
      }
      m_sessions.remove(m_activeSessionId);  // this also deletes the KateSession item since auto-deletion is enabled
      m_activeSessionId = INVALID_SESSION;
      if (sessionId > oldSessionId)
      {
        --sessionId;
      }
      emit sessionDeleted(oldSessionId);
      oldSessionId = INVALID_SESSION;
    }
  }

  m_activeSessionId = sessionId;
  m_sessions[sessionId]->activate();
  m_lastSessionId = INVALID_SESSION;
  emit sessionActivated(m_activeSessionId, oldSessionId);
  return true;
}

//------------------------------------
int KateSessionManager::newSession(const TQString &sessionName, bool saveCurr)
{
  m_sessions.append(new KateSession(*this, sessionName, TQString::null));
  int newSessionId = m_sessions.count() - 1;
  emit sessionCreated(newSessionId);
  activateSession(newSessionId, saveCurr);
  return newSessionId;
}

//------------------------------------
int KateSessionManager::cloneSession(int sessionId, const TQString &sessionName, bool activate, bool deleteCurr)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return INVALID_SESSION;
  }

  m_sessions.append(new KateSession(*m_sessions[sessionId], sessionName));
  int newSessionId = m_sessions.count() - 1;
  emit sessionCreated(newSessionId);
  
  // If cloning the active session, the new session will contain the current status
  // and the original session will be restored to the last saved state (save as... functionality)
  saveSession(newSessionId, sessionId == m_activeSessionId);
  if (sessionId == m_activeSessionId)
  {
    reloadActiveSession();
  }
  
  if (activate)
  {
    activateSession(newSessionId, m_activeSessionId != INVALID_SESSION && !deleteCurr);
  }
  return newSessionId;
}

//------------------------------------
bool KateSessionManager::restoreLastSession()
{
  if (m_activeSessionId != INVALID_SESSION)
  {
    return false;
  }
  return activateSession(m_lastSessionId, false);
}

//-------------------------------------------
void KateSessionManager::saveSession(int sessionId, bool saveGUIInfo, bool setReadOnly)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return;
  }
  m_sessions[sessionId]->save(saveGUIInfo, setReadOnly);
  emit sessionSaved(sessionId);
}

//-------------------------------------------
bool KateSessionManager::deleteSession(int sessionId, int actSessId)
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
  if (m_activeSessionId == INVALID_SESSION)
  {
    if (m_sessions.count() > 0 && actSessId >= 0 && actSessId < (int)m_sessions.count())
    {
      activateSession(actSessId, false);
    }
    else
    {
      newSession();
    }
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
  emit sessionRenamed(sessionId);
}

//-------------------------------------------
void KateSessionManager::setSessionReadOnlyStatus(int sessionId, bool readOnly)
{
  if (sessionId < 0 || sessionId >= (int)m_sessions.count())
  {
    return;
  }

  m_sessions[sessionId]->setReadOnly(readOnly);
  // Session is saved one last time when making it read only
  saveSession(sessionId, sessionId == m_activeSessionId, true);
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

  connect (m_listview, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(slotSelectionChanged()));
  connect (m_listview, TQ_SIGNAL(executed(TQListViewItem*)), this, TQ_SLOT(slotUser2()));

  TQPtrList<KateSession> &sessions = KateSessionManager::self()->getSessionsList();
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

#include "katesession.moc"
