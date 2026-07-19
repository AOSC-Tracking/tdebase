/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_MAINWINDOW_H__
#define __KATE_MAINWINDOW_H__

#include "katemain.h"
#include "katemdi.h"

#include <kate/view.h>
#include <kate/document.h>

#include <tdeparts/part.h>

#include <tdeaction.h>

class KateTabWidget;
class GrepTool;

namespace Kate {
  class MainWindow;
  class ToolViewManager;
}

class KFileItem;
class TDERecentFilesAction;
class DCOPObject;

class KateExternalToolsMenuAction;

class KateMainWindow : public KateMDI::MainWindow, virtual public KParts::PartBase
{
  TQ_OBJECT

  friend class KateConfigDialog;
  friend class KateViewManager;

  public:
    /**
     * Construct the window and restore it's state from given config if any
     * @param sconfig session config for this window, 0 if none
     * @param sgroup session config group to use
     */
    KateMainWindow (TDEConfig *sconfig, const TQString &sgroup);

    /**
     * Destruct the nice window
     */
    ~KateMainWindow();

  /**
   * Accessor methodes for interface and child objects
   */
    Kate::MainWindow *mainWindow () { return m_mainWindow; }
    Kate::ToolViewManager *toolViewManager () { return m_toolViewManager; }

    KateViewManager *viewManager () { return m_viewManager; }

    DCOPObject *dcopObject () { return m_dcop; }

  /**
   * various methodes to get some little info out of this
   */
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KURL activeDocumentUrl();

    uint mainWindowNumber () const { return myID; }

    /**
     * Prompts the user for what to do with files that are modified on disk if any.
     * This is optionally run when the window receives focus, and when the last
     * window is closed.
     * @return true if no documents are modified on disk, or all documents were
     * handled by the dialog; otherwise (the dialog was canceled) false.
     */
    bool showModOnDiskPrompt();

    /**
     * central tabwidget ;)
     * @return tab widget
     */
    KateTabWidget *tabWidget ();

    void readProperties(TDEConfig *config);
    void saveProperties(TDEConfig *config);

    bool queryCloseAllDocuments();
    bool queryClose_internal();

    void openURL (const TQString &name=0L);

  public slots:
    /**
     * update "Sessions" menu status when selection in session panel has changed
     */
    void slotSelectionChanged();

    /**
     * activate the specified session. When there is the need to activate a session
     * from the outside (for example from DCOP), using this method assures that
     * the session activation is consistent with the behavior of the session panel
     * @param sessionId the id of the session to activate
     */
    void activateSession(int sessionId);

  private:
    void setupMainWindow();
    void setupActions();
    bool queryClose();

    /**
     * read some global options from katerc
     */
    void readOptions();

    /**
     * save some global options to katerc
     */
    void saveOptions();

    void dragEnterEvent( TQDragEnterEvent * );
    void dropEvent( TQDropEvent * );

  private slots:
  /**
   * slots used for actions in the menus/toolbars
   * or internal signal connections
   */
    void newWindow ();

    void slotConfigure();

    void slotOpenWithMenuAction(int idx);

    void slotGrepToolItemSelected ( const TQString &filename, int linenumber );
    void slotMail();

    void slotFileQuit();
    void slotEditToolbars();
    void slotNewToolbarConfig();
    void slotWindowActivated ();
    void slotUpdateOpenWith();
    void documentMenuAboutToShow();
    void slotDropEvent(TQDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();

    void fileSelected(const KFileItem *file);

    void tipOfTheDay();

    /* to update the caption */
    void slotDocumentCreated(Kate::Document *doc);
    void slotNameChanged(Kate::Document *doc);
    void updateCaption(Kate::Document *doc);

    void pluginHelp();
    void slotFullScreen(bool);

    void updateGrepDir (bool visible);
    void slotDocumentCloseAll();

  protected:
    bool event( TQEvent * );
    bool eventFilter(TQObject *obj, TQEvent *ev);

  private:
    static uint uniqueID;
    uint myID;

    Kate::MainWindow *m_mainWindow;
    Kate::ToolViewManager *m_toolViewManager;

    bool showSessionName;
    bool syncKonsole;
    bool useInstance;
    bool modNotification;

    DCOPObject *m_dcop;

    // console
    KateConsole *console;

    // management items
    KateViewManager *m_viewManager;

    TDERecentFilesAction *fileOpenRecent;

    KateFileList *filelist;
    KateFileSelector *fileselector;
    KateSessionPanel *m_sessionpanel;

    TDEActionMenu* documentOpenWith;

    TQPopupMenu *documentMenu;

    TDEToggleAction* settingsShowFilelist;
    TDEToggleAction* settingsShowFileselector;

    KateExternalToolsMenuAction *externalTools;
    GrepTool * greptool;
    bool m_modignore;

    KateTabWidget *m_tabWidget;
};

class KateSessionListActionMenu : public TDEActionMenu
{
  TQ_OBJECT

  public:
    KateSessionListActionMenu(KateMainWindow *mw, const TQString &text, TQObject *parent = NULL, const char *name = NULL);
   ~KateSessionListActionMenu() {}

  public slots:
    void slotAboutToShow();

  protected:
    KateMainWindow *m_mainWindow;
};

#endif
