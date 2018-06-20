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
		* @return whether the session is still volatile, i.e. it has never
		*         been saved and never been named
		*/
		bool isStillVolatile() const;
		
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
//FIXME (advanced - multiple main windows or multiple Kate instances)
//There should be only one session manager regardless of how many main windows of Kate are open.
//Changes should propagate to all session panels. Different Kate main windows should run different
//sessions. If the user switches to a session already opened in another Kate window, the other window
//should be brought up to the screen (eventually ask user confirmation first).
//This would allow a safe use of multiple Kate main windows/instances without overwriting session information
//among them. Currently the last instance/main window to be closed will overwrite the information previously
//saved by other Kate instances.
/**
 * The Kate session manager. It takes care of storing and retrieving each session object
 * as well as providing methods to operate on them.
 *
 * @note The Kate session manager takes ownership of each session object it handles.
 */
//FIXME update the sessions.list file when switching to another session or to a new session
//
//FIXME create new unnamed session, choose 'save' as session switch option. Exit Kate. 
//      Session is saved without asking for a name
//FIXME improve getStartupOption/getSwitchOption/setSwitchOption using new signal
//      KateApp::optionsChanged()
//FIXME add kdDebug statement to ease debugging
class KateSessionManager : public TQObject
{
  Q_OBJECT

	public:

    enum
    {
      INVALID_SESSION = -1
    };

    // Session options on Kate startup
    enum 
    {
      STARTUP_NEW = 0,    // New session
      STARTUP_LAST,       // Use last session
      STARTUP_MANUAL      // Manually choose a session
    };
		
    // Session options on session switch or Kate shutdown
    enum 
    {
      SWITCH_DISCARD = 0, // Don't save current session
      SWITCH_SAVE,        // Save current session
      SWITCH_ASK          // Ask user what to do
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
		 * @param saveSessions true  = sessions info will be saved
		 *                     false = all sessions will be discarded
		 */
		void saveConfig(bool saveSessions);
		
		/**
		 * @return the session startup option
		 * The function checks the config file to see if there was any value update 
		 */
		const int getStartupOption();
		
		/**
		 * @return the session switch option
		 * The function checks the config file to see if there was any value update 
		 */
		const int getSwitchOption();
		
		/**
		 * Set the new session switch preference
		 * @param option the new option value. Defaults to SWITCH_ASK if the value is invalid.
		 * @emit switchOptionChanged
		 */
		void setSwitchOption(int option);
		
		/**
		 * @return the session files folder name
		 */
		const TQString& getBaseDir() const { return m_baseDir; }
		
		/**
		 * @return the number of existing sessions
		 */
		int getSessionCount() const { return m_sessions.count(); }
		
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
		 * Create a new session and activate it
		 * @param sessionName new session name
		 * @param saveCurr if true, save the current session before activating the new one
		 * @return the id of the newly created session
		 * @emit sessionCreated
     * @emit sessionDeleted  (only when leaving an unstored and unnamed session)
		 */
		int newSession(const TQString &sessionName = TQString::null, bool saveCurr = true);

		/**
		 * Create a new session and activate it if required
		 * @param sessionId the id of the session to clone
		 * @param sessionName the new session name
		 * @param activate if true, activate the new session after creation
		 * @param deleteCurr if true, delete the current session after switching
		 * @return the id of the newly created session
		 * @emit sessionCreated
		 */
		int cloneSession(int sessionId, const TQString &sessionName = TQString::null, 
		                 bool activate = true, bool deleteCurr = false);

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
		void saveSession(int sessionId) { saveSession(sessionId, sessionId == m_activeSessionId); }

		/**
		 * Delete the specified session
		 * @param sessionId the id of the session to delete
		 * @param actSessId the id of the next session to activate.
		 *        If INVALID_SESSION or invalid, create a new empty session.
		 *        This is only meaningful when deleting the current active session.
     * @return whether the session has been deleted or not
     * @emit sessionDeleted
		 */
		bool deleteSession(int sessionId, int actSessId);

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
		 * Emitted when the session switch option has been set/changed
		 */
		void switchOptionChanged();
		
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

    // Session options on Kate startup
    enum 
    {
      SO_STARTUP = 0,     // session startup option only
      SO_SWITCH,          // session switch option only
      SO_ALL,             // session startup and switch options
    };
		
		/**
		 * Updated the session startup and switch options
		 * @param optionType specifies which options needs to be updated
     */
		void updateSessionOptions(int optionType);
		
		/**
		 * Save the session startup and switch options to the config file
		 * @param optionType specifies which options needs to be saved
     */
		void saveSessionOptions(int optionType);
		
		/**
		 * Swap the position of the two specified sessions in the session list
		 * @param sessionId1 the id of the first session
		 * @param sessionId2 the id of the second session
		 * @emit sessionsSwapped
		 */
		void swapSessionsPosition(int sessionId1, int sessionId2);

		/**
		 * Save the specified session
		 * @param sessionId the id of the session to save
		 * @param saveGUIInfo if true, save also the information about the GUI elements
		 * @param setReadOnly necessary to save a session that has to be turned to read only
     * @emit sessionSaved
     */
		void saveSession(int sessionId, bool saveGUIInfo, bool setReadOnly = false);


		TQString m_baseDir;       					// folder where session files are stored
		TQString m_configFile;     					// file where the session list config is stored
		int m_activeSessionId;              // id of the active session
    int m_lastSessionId;                // id of the last active session before closing Kate
		TQPtrList<KateSession> m_sessions;  // session list
		KSimpleConfig *m_config;        		// session manager config
		int m_startupOption;                // session option on Kate startup
		int m_switchOption;                 // session option on session switch or Kate shutdown
		
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
//FIXME create one single KateSessionChooser and reuse it all the time
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

#endif
