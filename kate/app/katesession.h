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
#include <tdelistview.h>

class KateViewSpace;
class KDirWatch;
class KPushButton;
class TDEListView;
class TQCheckBox;
class KateSessionManager;


//BEGIN KateSession
/**
 * An object representing a Kate's session.
 */
class KateSession
{
	public:

		/**
		 * create a new session and read the config from fileName if it exists
		 * @param manager the session manager handling this session
		 * @param sessionName session name
		 * @param fileName file where session config is saved to/restored from
		 */
		KateSession(const KateSessionManager &manager, const TQString &sessionName, const TQString &fileName);

		/**
		 * duplicate an existing session into a new one with the given new name.
		 * If the existing session is read-only, the new one will *not* be read-only by default
		 * @param session the existing session 
		 * @param newSessionName the name of the new session
		 */
		KateSession(const KateSession &session, const TQString &newSessionName);

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
		* @param readOnly if true, the session status can not be modified
		*/
		void setReadOnly(bool readOnly) { m_readOnly = readOnly; }

		/**
		* @return the session filename if available, otherwise the null string
		*/
		const TQString& getSessionFilename() const { return m_filename; }

		/**
		* @return the number of documents in the session
		*/
		int getDocCount() const { return m_documents.count(); }

		/**
		 * Load session info from the saved file
		 * @param includeGUIInfo if true, also load the information about the GUI elements
		 */
		void load(bool includeGUIInfo);
		
		/**
		 * Save session info
		 * @param saveGUIInfo if true, save also the information about the GUI elements
		 * @param setReadOnly necessary to save a session that has to be turned to read only
		 */
		void save(bool saveGUIInfo, bool setReadOnly = false);

		/**
		 * Activate the session
		 */
		void activate();


	private:

		friend class KateViewSpace;

		/**
		* @return the session config object
		*/
		TDEConfig* getConfig() const { return m_config; }
    
 		/**
		* create a new filename for a session object
		*/
		void createFilename();

    const KateSessionManager &m_manager;  // The session manager that handles this session
		TQString       m_sessionName;
		TQString       m_filename;
		bool           m_readOnly;
		TQStringList   m_documents;           // document URLs
		KSimpleConfig *m_config;              // session config

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
/**
 * The Kate session manager. It takes care of storing and retrieving each session object
 * as well as providing methods to operate on them.
 *
 * @note The Kate session manager takes ownership of each session object it handles.
 */
//FIXME update the sessions.list file when switching to another session or to a new session
//FIXME create a new unnamed session and switch to another session. The first session is saved without
//asking the user for a new session name. This is wrong.
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
		 * @return the session files folder name
		 */
		const TQString& getBaseDir() const { return m_baseDir; }
		
		/**
		 * @return the active session id
		 */
		int getActiveSessionId() const { return m_activeSessionId; }

		/**
		 * @return the active session name
		 */
		const TQString& getActiveSessionName() /*FIXME const*/ { return m_sessions[m_activeSessionId]->getSessionName(); }

		/**
		 * @param sessionId the id of the session of interest
		 * @return the name of the specified session
		 */
		const TQString& getSessionName(int sessionId) /*FIXME const*/;

		/**
		 * @return a reference to the active session
		 */
		KateSession* getActiveSession() { return m_sessions[m_activeSessionId]; }

		/**
		 * @param sessionId the id of the session to return
		 * @return a reference to the specified session
		 */
		KateSession* getSessionFromId(int sessionId);

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
		 * @emit sessionActivated
		 */
		bool activateSession(int sessionId, bool saveCurr = true);

		/**
		 * Create a new session and activate it if required
		 * @param sessionName new session name
		 * @param activate if true, activate the new session after creation
		 * @return the id of the newly created session
		 * @emit sessionCreated
		 */
		int newSession(const TQString &sessionName = TQString::null, bool activate = true);

		/**
		 * Create a new session and activate it if required
		 * @param sessionId the id of the session to clone
		 * @param sessionName the new session name
		 * @param activate if true, activate the new session after creation
		 * @return the id of the newly created session
		 * @emit sessionCreated
		 */
		int cloneSession(int sessionId, const TQString &sessionName = TQString::null, bool activate = true);

		/**
		 * Restore the current active session to the last saved state
		 */
		void reloadActiveSession() { m_sessions[m_activeSessionId]->load(true); }

    /**
     * Restore the last saved session. Can only be used before
     * any other session has been activated, i.e. on Kate's startup
     * @return whether the session was activated or not
     */
    bool restoreLastSession();

		/**
		 * Saves the active session
     * @emit sessionSaved (through invoked "void saveSession(int)" method)
		 */
		void saveActiveSession() { saveSession(m_activeSessionId); }

		/**
		 * Save the specified session
		 * @param sessionId the id of the session to save
     * @emit sessionSaved
     */
		void saveSession(int sessionId);

		/**
		 * Delete the specified session
		 * @param sessionId the id of the session to delete
     * @return whether the session has been deleted or not
     * @emit sessionDeleted
		 */
		bool deleteSession(int sessionId);

		/**
		 * Move the specified session forward in the session list (by one position)
		 * @param sessionId the id of the session to move
     */
		void moveSessionForward(int sessionId);

		/**
		 * Move the specified session backward in the session list (by one position)
		 * @param sessionId the id of the session to move
     */
		void moveSessionBackward(int sessionId);

		/**
		 * Rename the specified session
		 * @param sessionId the id of the session to rename
		 * @param newSessionName the new session name
     * @emit sessionRenamed
     */
		void renameSession(int sessionId, const TQString &newSessionName);

		/**
		 * Set the read only status of the specified session
		 * @param sessionId the id of the session to modify
		 * @param readOnly the new read only status
		 */
		void setSessionReadOnlyStatus(int sessionId, bool readOnly);

  signals:
		/**
		 * Emitted once a session has been activated
		 * @param newSessionId the id of the previous active session
		 * @param oldSessionId the id of the new active session
		 */
		void sessionActivated(int newSessionId, int oldSessionId);

		/**
		 * Emitted once a session has been created
		 * @param sessionId the id of the new session
		 */
		void sessionCreated(int sessionId);

		/**
		 * Emitted once a session has been saved
		 * @param sessionId the id of the saved session
		 */
		void sessionSaved(int sessionId);

		/**
		 * Emitted once a session has been deleted
		 * @param sessionId the id of the deleted session
		 */
		void sessionDeleted(int sessionId);

		/**
		 * Emitted once the position of the two sessions have been swapped
		 * @param sessionIdMin the smallest id of the session couple
		 * @param sessionIdMax the biggest id of the session couple
		 */
		void sessionsSwapped(int sessionIdMin, int sessionIdMax);

		/**
		 * Emitted once a session has been renamed
		 * @param sessionId the id of the new session
		 */
		void sessionRenamed(int sessionId);


	protected:
		KateSessionManager();

		/**
		 * Swap the position of the two specified sessions in the session list
		 * @param sessionId1 the id of the first session
		 * @param sessionId2 the id of the second session
		 * @emit sessionsSwapped
		 */
		void swapSessionsPosition(int sessionId1, int sessionId2);


		TQString m_baseDir;       					// folder where session files are stored
		TQString m_configFile;     					// file where the session list config is stored
		int m_activeSessionId;              // index of the active session
    bool m_firstActivation;             // true until at least one session has been activated
		TQPtrList<KateSession> m_sessions;  // session list
		KSimpleConfig *m_config;        		// session manager config

		static KateSessionManager *ksm_instance;  // the only KateSessionManager instance
};
//END KateSessionManager


//BEGIN KateSessionChooserItem
class KateSessionChooserItem : public TDEListViewItem
{
  public:
    KateSessionChooserItem(TQListView *listview, const TQString &sessionName, const TQString &nDoc, int sessionId)
      : TDEListViewItem(listview, sessionName, nDoc), m_sessionId(sessionId) {}

		int  getSessionId() { return m_sessionId; }
		void setSessionId(int sessionId) { m_sessionId = sessionId; }

	protected:
    int m_sessionId;
};
//END KateSessionChooserItem


//BEGIN KateSessionChooser
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
	 ~KateSessionChooser() {}

		int getSelectedSessionId();  // return the session id of the selected session

	protected slots:

		void slotUser1();   // open existing session
		void slotUser2();   // open new session
		void slotUser3();   // quit kate
		void slotSelectionChanged();  // list selection has changed

	protected:
		TDEListView *m_listview;
};
//END KateSessionChooser




//------------------------------------
//------------------------------------
//------------------------------------
class OldKateSessionManager;
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
