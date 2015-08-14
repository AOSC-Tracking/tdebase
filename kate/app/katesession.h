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

#ifndef __KATE_SESSION_H__
#define __KATE_SESSION_H__

#include "katemain.h"

#include <kdialogbase.h>
#include <ksimpleconfig.h>
#include <ksharedptr.h>
#include <tdeaction.h>

#include <tqobject.h>
#include <tqvaluelist.h>

class OldKateSessionManager;

class KDirWatch;
class TDEListView;
class KPushButton;

class TQCheckBox;

class OldKateSession  : public TDEShared
{
  public:
    /**
     * Define a Shared-Pointer type
     */
    typedef TDESharedPtr<OldKateSession> Ptr;

  public:
    /**
     * create a session from given file
     * @param fileName session filename, relative
     * @param name session name
     * @param manager pointer to the manager
     */
    OldKateSession (OldKateSessionManager *manager, const TQString &fileName, const TQString &name);

    /**
     * init the session object, after construction or create
     */
    void init ();

    /**
     * destruct me
     */
    ~OldKateSession ();

    /**
     * session filename, absolute, calculated out of relative filename + session dir
     * @return absolute path to session file
     */
    TQString sessionFile () const;

    /**
     * relative session filename
     * @return relative filename for this session
     */
    const TQString &sessionFileRelative () const { return m_sessionFileRel; }

    /**
     * session name
     * @return name for this session
     */
    const TQString &sessionName () const { return m_sessionName; }

    /**
     * is this a valid session? if not, don't use any session if this is
     * the active one
     */
    bool isNew () const { return m_sessionName.isEmpty(); }

    /**
     * create the session file, if not existing
     * @param name name for this session
     * @param force force to create new file
     * @return true if created, false if no creation needed
     */
    bool create (const TQString &name, bool force = false);

    /**
     * rename this session
     * @param name new name
     * @return success
     */
    bool rename (const TQString &name);

    /**
     * config to read
     * on first access, will create the config object, delete will be done automagic
     * return 0 if we have no file to read config from atm
     * @return config to read from
     */
    TDEConfig *configRead ();

    /**
     * config to write
     * on first access, will create the config object, delete will be done automagic
     * return 0 if we have no file to write config to atm
     * @return config to write from
     */
    TDEConfig *configWrite ();

    /**
     * count of documents in this session
     * @return documents count
     */
    unsigned int documents () const { return m_documents; }

  private:
    /**
     * session filename, in local location we can write to
     * relative filename to the session dirs :)
     */
    TQString m_sessionFileRel;

    /**
     * session name, extracted from the file, to display to the user
     */
    TQString m_sessionName;

    /**
     * number of document of this session
     */
    unsigned int m_documents;

    /**
     * OldKateSessionMananger
     */
    OldKateSessionManager *m_manager;

    /**
     * simpleconfig to read from
     */
    KSimpleConfig *m_readConfig;

    /**
     * simpleconfig to write to
     */
    KSimpleConfig *m_writeConfig;
};

typedef TQValueList<OldKateSession::Ptr> OldKateSessionList;

class OldKateSessionManager : public TQObject
{
  Q_OBJECT

  public:
    OldKateSessionManager(TQObject *parent);
    ~OldKateSessionManager();

    /**
     * allow access to this :)
     * @return instance of the session manager
     */
    static OldKateSessionManager *self();

    /**
     * allow access to the session list
     * kept up to date by watching the dir
     */
    inline OldKateSessionList & sessionList () { updateSessionList (); return m_sessionList; }

    /**
     * activate a session
     * first, it will look if a session with this name exists in list
     * if yes, it will use this session, else it will create a new session file
     * @param session session to activate
     * @param closeLast try to close last session or not?
     * @param saveLast try to save last session or not?
     * @param loadNew load new session stuff?
     */
    void activateSession (OldKateSession::Ptr session, bool closeLast = true, bool saveLast = true, bool loadNew = true);

    /**
     * create a new session
     * @param name session name
     */
    OldKateSession::Ptr createSession (const TQString &name);

    /**
     * return session with given name
     * if no existing session matches, create new one with this name
     * @param name session name
     */
    OldKateSession::Ptr giveSession (const TQString &name);

    /**
     * save current session
     * for sessions without filename: save nothing
     * @param tryAsk should we ask user if needed?
     * @param rememberAsLast remember this session as last used?
     * @return success
     */
    bool saveActiveSession (bool tryAsk = false, bool rememberAsLast = false);

    /**
     * return the current active session
     * sessionFile == empty means we have no session around for this instance of kate
     * @return session active atm
     */
    inline OldKateSession::Ptr activeSession () { return m_activeSession; }

    /**
     * session dir
     * @return global session dir
     */
    inline const TQString &sessionsDir () const { return m_sessionsDir; }

    /**
     * initial session chooser, on app start
     * @return success, if false, app should exit
     */
    bool chooseSession ();

  public slots:
    /**
     * try to start a new session
     * asks user first for name
     */
    void sessionNew ();

    /**
     * try to open a existing session
     */
    void sessionOpen ();

    /**
     * try to save current session
     */
    void sessionSave ();

    /**
     * try to save as current session
     */
    void sessionSaveAs ();

    /**
     * show dialog to manage our sessions
     */
    void sessionManage ();

  private slots:
    void dirty (const TQString &path);

  public:
    /**
     * trigger update of session list
     */
    void updateSessionList ();

  private:
    /**
     * absolute path to dir in home dir where to store the sessions
     */
    TQString m_sessionsDir;

    /**
     * list of current available sessions
     */
    OldKateSessionList m_sessionList;

    /**
     * current active session
     */
    OldKateSession::Ptr m_activeSession;
};

class OldKateSessionChooser : public KDialogBase
{
  Q_OBJECT

  public:
    OldKateSessionChooser (TQWidget *parent, const TQString &lastSession);
    ~OldKateSessionChooser ();

    OldKateSession::Ptr selectedSession ();

    bool reopenLastSession ();

    enum {
      resultQuit = TQDialog::Rejected,
      resultOpen,
      resultNew,
      resultNone
    };

  protected slots:
    /**
     * open session
     */
    void slotUser1 ();

    /**
     * new session
     */
    void slotUser2 ();

    /**
     * quit kate
     */
    void slotUser3 ();

    /**
     * selection has changed
     */
    void selectionChanged ();

  private:
    TDEListView *m_sessions;
    TQCheckBox *m_useLast;
};

class OldKateSessionOpenDialog : public KDialogBase
{
  Q_OBJECT

  public:
    OldKateSessionOpenDialog (TQWidget *parent);
    ~OldKateSessionOpenDialog ();

    OldKateSession::Ptr selectedSession ();

    enum {
      resultOk,
      resultCancel
    };

  protected slots:
    /**
     * cancel pressed
     */
    void slotUser1 ();

    /**
     * ok pressed
     */
    void slotUser2 ();

  private:
    TDEListView *m_sessions;
};

class OldKateSessionManageDialog : public KDialogBase
{
  Q_OBJECT

  public:
    OldKateSessionManageDialog (TQWidget *parent);
    ~OldKateSessionManageDialog ();

  protected slots:
    /**
     * close pressed
     */
    void slotUser1 ();

    /**
     * selection has changed
     */
    void selectionChanged ();

    /**
     * try to rename session
     */
    void rename ();

    /**
     * try to delete session
     */
    void del ();

  private:
    /**
     * update our list
     */
    void updateSessionList ();

  private:
    TDEListView *m_sessions;
    KPushButton *m_rename;
    KPushButton *m_del;
};

class OldKateSessionsAction : public TDEActionMenu
{
  Q_OBJECT

  public:
    OldKateSessionsAction(const TQString& text, TQObject* parent = 0, const char* name = 0);
    ~OldKateSessionsAction (){;};

  public  slots:
    void slotAboutToShow();

    void openSession (int i);
};

#endif
