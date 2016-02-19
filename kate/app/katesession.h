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
#include <tqptrlist.h>
#include <tqvaluelist.h>
#include <tqstringlist.h>

class KateViewSpace;

class OldKateSessionManager;  // Michele - to be removed with OldKateSession

class KDirWatch;
class TDEListView;
class KPushButton;

class TQCheckBox;

//BEGIN KateSession
class KateSession
{
	public:

		/**
		 * create a new session and read the config from fileName if it exists
		 * @param sessionName session name
		 * @param fileName file where session config is saved to/restored from
		 * @param isFullName true  -> filename is a full filename, used to load/save the session configuration
		 *                   false -> filename is a folder name. This is used for new unsaved sessions
		 *                            to inject the location where the configuration file should be saved
		 */
		KateSession(const TQString &sessionName, const TQString &fileName, bool isFullName);

		/**
		 * Destructor
		 */
		~KateSession();

		/**
		* @return the session name
		*/
		const TQString& getSessionName() const { return m_sessionName; }
		/**
		* Set the new session name
		* @param sessionName the new session name
		*/
		void setSessionName(const TQString &sessionName);

		/**
		* @return whether the session is read only or not
		*/
		bool isReadOnly() const { return m_readOnly; }
		/**
		* Set session read only status
		* @param readOnly if true, the session config can not be saved to file
		*/
		void setReadOnly(bool readOnly);

		/**
		* @return the session filename
		*/
		const TQString& getSessionFilename() const { return m_filename; }

		/**
		* @return the number of documents in the session
		*/
		int getDocCount() const { return m_docCount; }

		/**
		 * Save session info
		 * @param saveGUIInfo if true, save also the information about the GUI elements
		 */
		void save(bool saveGUIInfo);

		/**
		 * Activate the session
		 */
		void activate();

	private:

		friend class KateViewSpace;
		/**
		* @return the session config object
		*/
		TDEConfig* getConfig() { return m_config; }


		TQString       m_sessionName;
		TQString       m_filename;
		bool           m_isFullName;   // true  -> m_filename is a full filename
		// false -> m_filename is a folder name.
		bool           m_readOnly;
		int	 					 m_docCount;		 // number of documents in the session
		TQStringList   m_documents;    // document URLs
		KSimpleConfig *m_config;       // session config

};

//END KateSession

//BEGIN KateSessionManager
//FIXME (advanced)
//There should be only one session manager regardless of how many instances of Kate are running.
//Changes should propagate to all session panels. Different Kate instances should run different
//sessions. If the user switches to a session already opened in another Kate instance, the current
//session should be saved and then the focus switched to the other instance.
//This would allow a safe use of multiple Kate instances without overwriting session information
//among them. Currently the last instance to be closed will overwrite the information previously
//saved by other Kate instances.
//------------------------------------
class KateSessionManager : public TQObject
{
  Q_OBJECT

	public:

    enum
    {
      INVALID_SESSION = -1
    };

		/**
		 * get a pointer to the unique KateSessionManager instance.
		 * If the manager does not exist yet, create it.
		 */
		static KateSessionManager* self();

		/**
		 * Destructor
		 */
		~KateSessionManager();

		/**
		 * Save session manager info
		 */
		void saveConfig();

		/**
		 * @return the active session id
		 */
		int getActiveSessionId() const { return m_activeSessionId; }

		/**
		 * @return the active session name
		 */
		const TQString& getActiveSessionName() /*FIXME const*/ { return m_sessions[m_activeSessionId]->getSessionName(); }

		/**
		 * @return a reference to the active session
		 */
		KateSession* getActiveSession() { return m_sessions[m_activeSessionId]; }

    /**
     * Return the session id of the first session whose name matches the
     * provided one. In case multiple sessions share the same name,
     * the id of the first one found will be returned.
     * @param name the session name to look for
     * @return the session id of the matching session if it is found,
     *         otherwise KateSessionManager::INVALID_SESSION.
     */
    int getSessionIdFromName(const TQString &name);

		/**
		 * @return a reference to the sessions list
		 */
		TQPtrList<KateSession>& getSessionsList() { return m_sessions; }

		/**
		 * Activate the selected session.
		 * @param sessionId the id of the session to activate
		 * @param saveCurr if true, save the current session before activating the new one
		 * @return whether the session was activated or not
		 */
		bool activateSession(int sessionId, bool saveCurr = true);

		/**
		 * Create a new session and activate it if required
		 * @param sessionName new session name
		 * @param activate if true, activate the new session after creation
		 * @return the id of the newly created session
		 */
		int newSession(const TQString &sessionName = TQString::null, bool activate = true);

    /**
     * Restore the last saved session. Can only be used before
     * any other session has been activated, i.e. on Kate's startup
     * @return whether the session was activated or not
     */
    bool restoreLastSession();

		/**
		 * Saves the active session
		 */
		void saveActiveSession() { m_sessions[m_activeSessionId]->save(true); }

  signals:
		/**
		 * Emitted once a session has been activated
		 * @param newSessionId the id of the previous active session
		 * @param oldSessionId the id of the new active session
		 */
		void sessionActivated(int newSessionId, int oldSessionId);

		/**
		 * Emitted once a session has been created
		 * @param newSessionId the id of the new session
		 */
		void sessionCreated(int newSessionId);


  public slots:
		/**
		 * Slot to create a new session
		 */
    void slotNewSession();


	private:
		KateSessionManager();

		TQString m_baseDir;       					// folder where session files are stored
		TQString m_configFile;     					// file where the session list config is stored
		int m_sessionsCount;	     					// number of sessions
		int m_activeSessionId;              // index of the active session
    bool m_firstActivation;             // true until at least one session has been activated
		TQPtrList<KateSession> m_sessions;  // session list
		KSimpleConfig *m_config;        		// session manager config

		static KateSessionManager *ksm_instance;  // the only KateSessionManager instance
};

//END KateSessionManager

//BEGIN KateSessionChooser
//------------------------------------
class KateSessionChooser : public KDialogBase
{
	Q_OBJECT

	public:
		enum Result
		{
			RESULT_NO_OP = TQDialog::Rejected,
			RESULT_OPEN_EXISTING,
			RESULT_OPEN_NEW,
			RESULT_QUIT_KATE
		};

		KateSessionChooser(TQWidget *parent);
		KateSessionChooser() {}

		int getSelectedSessionId();  //return the session id of the selected session

	protected slots:

		void slotUser1();   // open existing session
		void slotUser2();   // open new session
		void slotUser3();   // quit kate
		void slotSelectionChanged();  // list selection has changed

	protected:
		TDEListView *m_sessionList;
		int m_columnSessionId;
};

//BEGIN KateSessionChooser





//------------------------------------
//------------------------------------
//------------------------------------
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
		OldKateSession ( OldKateSessionManager *manager, const TQString &fileName, const TQString &name );

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
		bool create ( const TQString &name, bool force = false );

		/**
		 * rename this session
		 * @param name new name
		 * @return success
		 */
		bool rename ( const TQString &name );

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
		OldKateSessionManager ( TQObject *parent );
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
		void activateSession ( OldKateSession::Ptr session, bool closeLast = true, bool saveLast = true, bool loadNew = true );

		/**
		 * create a new session
		 * @param name session name
		 */
		OldKateSession::Ptr createSession ( const TQString &name );

		/**
		 * return session with given name
		 * if no existing session matches, create new one with this name
		 * @param name session name
		 */
		OldKateSession::Ptr giveSession ( const TQString &name );

		/**
		 * save current session
		 * for sessions without filename: save nothing
		 * @param tryAsk should we ask user if needed?
		 * @param rememberAsLast remember this session as last used?
		 * @return success
		 */
		bool saveActiveSession ( bool tryAsk = false, bool rememberAsLast = false );

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
		void dirty ( const TQString &path );

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


class OldKateSessionOpenDialog : public KDialogBase
{
		Q_OBJECT

	public:
		OldKateSessionOpenDialog ( TQWidget *parent );
		~OldKateSessionOpenDialog ();

		OldKateSession::Ptr selectedSession ();

		enum
		{
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
		OldKateSessionManageDialog ( TQWidget *parent );
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
		OldKateSessionsAction ( const TQString& text, TQObject* parent = 0, const char* name = 0 );
		~OldKateSessionsAction () {;};

	public  slots:
		void slotAboutToShow();

		void openSession ( int i );
};

#endif
