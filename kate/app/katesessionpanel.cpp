/* This file is part of the TDE project
   Copyright (C) 2015 Michele Calgaro <micheleDOTcalgaro__AT__yahooDOTit>

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

#include "katesessionpanel.h"
#include "katesessionpanel.moc"

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katesession.h"

#include <kiconloader.h>
#include <tdelocale.h>


void KateSessionPanelToolBarParent::setToolBar(TDEToolBar *tbar)
{
	m_tbar = tbar;
}

//-------------------------------------------
void KateSessionPanelToolBarParent::resizeEvent (TQResizeEvent*)
{
	if (m_tbar)
	{
		setMinimumHeight(m_tbar->sizeHint().height());
		m_tbar->resize(width(),height());
	}
}

//-------------------------------------------
KateSessionPanel::KateSessionPanel(KateMainWindow *mainWindow, KateViewManager *viewManager,
    TQWidget *parent, const char *name)
    : TQVBox(parent, name), m_mainWin(mainWindow), m_viewManager(viewManager),
      m_sessionManager(KateSessionManager::self()), m_actionCollection(new TDEActionCollection(this))
{
  // Toolbar
  setup_toolbar();

  // Listview
  m_listview = new TDEListView(this);
  m_listview->setRootIsDecorated(true);
  m_listview->setSorting(-1);
  m_listview->setMinimumWidth(m_listview->sizeHint().width());
}

//-------------------------------------------
void KateSessionPanel::setup_toolbar()
{
  // Toolbar widget and frame
  KateSessionPanelToolBarParent *tbarParent=new KateSessionPanelToolBarParent(this);
  m_toolbar = new TDEToolBar(tbarParent, "Kate Session Panel Toolbar", true);
  tbarParent->setToolBar(m_toolbar);
  m_toolbar->setMovingEnabled(false);
  m_toolbar->setFlat(true);
  m_toolbar->setIconText(TDEToolBar::IconOnly);
  m_toolbar->setIconSize(16);
  m_toolbar->setEnableContextMenu(false);

  // Toolbar actions
  TDEAction *a;
  a = new TDEAction(i18n("New"), SmallIcon("list-add"), 0,
          TQT_TQOBJECT(m_sessionManager), TQT_SLOT(sessionNew()), m_actionCollection, "session_new");
  a->setWhatsThis(i18n("Create a new session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Save"), SmallIcon("document-save"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(saveSession()), m_actionCollection, "session_save");
  a->setWhatsThis(i18n("Save the current session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Save as..."), SmallIcon("document-save-as"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(saveSessionAs()), m_actionCollection, "session_save_as");
  a->setWhatsThis(i18n("Save the current session with a different name."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Rename"), SmallIcon("edit_user"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(renameSession()), m_actionCollection, "session_rename");
  a->setWhatsThis(i18n("Rename the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Delete"), SmallIcon("edit-delete"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(deleteSession()), m_actionCollection, "session_delete");
  a->setWhatsThis(i18n("Delete the selected session."));
  a->plug(m_toolbar);

  m_toolbar->insertLineSeparator();

  a = new TDEAction(i18n("Activate"), SmallIcon("forward"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(sessionActivate()), m_actionCollection, "session_activate");
  a->setWhatsThis(i18n("Activate the selected session."));
  a->plug(m_toolbar);

	TDEToggleAction *tglA = new TDEToggleAction(i18n("Toggle read only"), SmallIcon("encrypted"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(sessionToggleReadOnly()), m_actionCollection, "session_toggle_read_only");
  tglA->setWhatsThis(i18n("Toggle read only status for the selected session.<p>"
					"In a read only session, you can work as usual but the list of documents in the session "
          "will not be saved when you exit Kate or switch to another session.<p>"
          "You can use this option to create template sessions that you wish to keep unchanged over time."));
  tglA->plug(m_toolbar);

  a = new TDEAction(i18n("Move Up"), SmallIcon("go-up"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(sessionMoveUp()), m_actionCollection, "session_move_up");
  a->setWhatsThis(i18n("Move up the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Move Down"), SmallIcon("go-down"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(sessionMoveDown()), m_actionCollection, "session_move_down");
  a->setWhatsThis(i18n("Move down the selected session."));
  a->plug(m_toolbar);

  m_toolbar->insertLineSeparator();

  a = new TDEAction(i18n("Open"), SmallIcon("document-open"), 0,
          TQT_TQOBJECT(m_sessionManager), TQT_SLOT(sessionOpen()), m_actionCollection, "session_open");
  a->setWhatsThis(i18n("Switch to another session chosen from a list of existing ones."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Manage"), SmallIcon("view_choose"), 0,
          TQT_TQOBJECT(m_sessionManager), TQT_SLOT(sessionManage()), m_actionCollection, "session_manage");
  a->setWhatsThis(i18n("Manage existing sessions."));
  a->plug(m_toolbar);
}

//-------------------------------------------
void KateSessionPanel::saveSession()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::saveSessionAs()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::renameSession()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::deleteSession()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::sessionActivate()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::sessionToggleReadOnly()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::sessionMoveUp()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::sessionMoveDown()
{
//TODO
}
