/* This file is part of the TDE project
   Copyright (C) 2015-2016 Michele Calgaro <micheleDOTcalgaro__AT__yahooDOTit>

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

#ifndef __KATE_SESSIONPANEL_H__
#define __KATE_SESSIONPANEL_H__

/*
  The kate session panel displays the available sessions (and their documents)
  in a treeview list and allows for quick switching among them.
  A toolbar on the top also provides quick access to actions needed
  to manage sessions.
*/

#include <tqvbox.h>
#include <tdetoolbar.h>
#include <tdelistview.h>
#include <tqframe.h>
#include <tqlineedit.h>
#include <tqcheckbox.h>
#include <kdialogbase.h>

class KateMainWindow;
class KateViewManager;
class KateSessionManager;
class TDEActionCollection;


//BEGIN KateSessionNameChooser
class KateSessionNameChooser : public KDialogBase
{
	Q_OBJECT

	public:

		KateSessionNameChooser(TQWidget *parent);
	 ~KateSessionNameChooser() {}

		TQString getSessionName(); // return the session name typed by the user
		bool getActivateFlag();     // return whether to switch to the new session or not

	protected slots:

		void slotUser1();          // create new session
		void slotUser2();          // cancel
		void slotTextChanged();    // session name has changed

	protected:
    TQLineEdit *m_sessionNameLE;
    TQCheckBox *m_activateCB;
};
//BEGIN KateSessionNameChooser


//BEGIN KateSessionPanelToolBarParent
class KateSessionPanelToolBarParent: public TQFrame
{
  Q_OBJECT

  public:
    KateSessionPanelToolBarParent(TQWidget *parent) : TQFrame(parent), m_tbar(0) {}
   ~KateSessionPanelToolBarParent() {}
    void setToolBar(TDEToolBar *tbar);

  protected:
    virtual void resizeEvent (TQResizeEvent*);

  private:
    TDEToolBar *m_tbar;
};
//END KateSessionPanelToolBarParent


//BEGIN KateSessionPanelItem
class KateSessionPanelItem : public TDEListViewItem
{
  public:
    KateSessionPanelItem(TQListView *listview, const TQString &sessionName, int sessionId)
      : TDEListViewItem(listview, sessionName), m_sessionId(sessionId) {}
    KateSessionPanelItem(TQListView *listview, TQListViewItem *after, const TQString &sessionName, int sessionId)
      : TDEListViewItem(listview, after, sessionName), m_sessionId(sessionId) {}

		int  getSessionId() { return m_sessionId; }
		void setSessionId(int sessionId) { m_sessionId = sessionId; }

	protected:
    int m_sessionId;
};
//END KateSessionPanelItem


//BEGIN KateSessionPanel
class KateSessionPanel : public TQVBox
{
  Q_OBJECT

  public:

    KateSessionPanel(KateMainWindow *mainWindow=0, KateViewManager *viewManager=0,
                     TQWidget *parent=0, const char *name=0);
    ~KateSessionPanel() {}


  public slots:
    void slotNewSession();
    void slotSaveSession();
    void slotSaveSessionAs();
    void slotRenameSession();
    void slotDeleteSession();
    void slotActivateSession();
    void slotSessionToggleReadOnly();
    void slotSessionMoveUp();
    void slotSessionMoveDown();
    void slotItemExecuted(TQListViewItem *item);

    void slotSelectionChanged();
    void slotSessionActivated(int newSessionId, int oldSessionId);
    void slotSessionCreated(int sessionId);
    void slotSessionDeleted(int sessionId);
    void slotSessionsSwapped(int sessionIdMin, int sessionIdMax);
    void slotSessionRenamed(TQListViewItem *item);

  private:
    void setup_toolbar();

    KateMainWindow *m_mainWin;
    KateViewManager *m_viewManager;
    KateSessionManager *m_sessionManager;
    TDEActionCollection *m_actionCollection;
    TDEToolBar *m_toolbar;
    TDEListView *m_listview;
    int m_columnName;
    int m_columnPixmap;
};
//END KateSessionPanel


#endif //__KATE_SESSIONPANEL_H__
