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

#include "katesessionpanel.h"
#include "katesessionpanel.moc"

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katesession.h"

#include <kiconloader.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tqlistview.h>
#include <tqlabel.h>


namespace
{
  const char *KS_UNNAMED  = "Unnamed";
};


//BEGIN KateSessionNameChooser
//-------------------------------------------
KateSessionNameChooser::KateSessionNameChooser(TQWidget *parent)
 : KDialogBase(parent, "", true, i18n("Session Name Chooser"), KDialogBase::User1 | KDialogBase::User2,
               KDialogBase::User2, true, KStdGuiItem::cancel(), KGuiItem(i18n("Continue"), "document-new"))
{
  TQHBox *page = new TQHBox(this);
  //page->setMinimumSize(300, 100);
  setMainWidget(page);

  TQVBox *vb = new TQVBox(page);
  vb->setSpacing(KDialog::spacingHint());

  TQLabel *label = new TQLabel(vb);
  label->setText("Please type the new session name:");

  m_sessionNameLE = new TQLineEdit(vb);
  m_sessionNameLE->setText(KS_UNNAMED);
  m_sessionNameLE->setFocus();

  m_activateCB = new TQCheckBox(i18n("Switch to the new session"), vb, NULL);
  m_activateCB->setChecked(true);

  connect (m_sessionNameLE, TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(slotTextChanged()));
  slotTextChanged();  // update button status
}

//-------------------------------------------
TQString KateSessionNameChooser::getSessionName()
{
  return m_sessionNameLE->text();
}

//-------------------------------------------
bool KateSessionNameChooser::getActivateFlag()
{
  return m_activateCB->isChecked();
}

//-------------------------------------------
void KateSessionNameChooser::slotUser1()
{
  reject();
}

//-------------------------------------------
void KateSessionNameChooser::slotUser2()
{
  accept();
}

//-------------------------------------------
void KateSessionNameChooser::slotTextChanged()
{
  enableButton(KDialogBase::User2, !m_sessionNameLE->text().isEmpty());
}
//END KateSessionNameChooser


//BEGIN KateSessionPanelToolBarParent
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
//END KateSessionPanelToolBarParent


//BEGIN KateSessionPanel
//-------------------------------------------
KateSessionPanel::KateSessionPanel(KateMainWindow *mainWindow, KateViewManager *viewManager,
    TQWidget *parent, const char *name)
    : TQVBox(parent, name), m_mainWin(mainWindow), m_viewManager(viewManager),
      m_sessionManager(KateSessionManager::self()), m_actionCollection(new TDEActionCollection(this)),
      m_columnName(-1), m_columnPixmap(-1)
{
  // Toolbar
  setup_toolbar();

  // Listview
  m_listview = new TDEListView(this);
  m_listview->header()->hide();
  m_columnName = m_listview->addColumn("Session name");
  m_columnPixmap = m_listview->addColumn("Pixmap", 24);
  m_listview->addColumn("Dummy", 1);  // Dummy column, only for nice resizing
  m_listview->header()->setResizeEnabled(false, m_columnPixmap);
  m_listview->setColumnAlignment(m_columnPixmap, TQt::AlignCenter);
  m_listview->setMinimumWidth(m_listview->sizeHint().width());
  m_listview->setSorting(-1);
  m_listview->setResizeMode(TQListView::LastColumn);
  //m_listview->setRootIsDecorated(true);  // FIXME to enable after inserting doc list
  connect(m_listview, TQT_SIGNAL(executed(TQListViewItem*)),
          this, TQT_SLOT(slotItemExecuted(TQListViewItem*)));
  connect(m_sessionManager, TQT_SIGNAL(sessionActivated(int, int)),
          this, TQT_SLOT(slotSessionActivated(int, int)));
  connect(m_sessionManager, TQT_SIGNAL(sessionCreated(int)),
          this, TQT_SLOT(slotSessionCreated(int)));
  connect(m_sessionManager, TQT_SIGNAL(sessionDeleted(int)),
          this, TQT_SLOT(slotSessionDeleted(int)));
  connect(m_sessionManager, TQT_SIGNAL(sessionsSwapped(int, int)),
          this, TQT_SLOT(slotSessionsSwapped(int, int)));
  connect(m_listview, TQT_SIGNAL(itemRenamed(TQListViewItem*)),
          this, TQT_SLOT(slotSessionRenamed(TQListViewItem*)));

  TQPtrList<KateSession>& sessions = m_sessionManager->getSessionsList();
  for (int idx = sessions.count()-1;  idx >= 0;  --idx)
  {
  	new KateSessionPanelItem(m_listview, sessions[idx]->getSessionName(), idx);
  	if (idx == m_sessionManager->getActiveSessionId())
  	{
  	  m_listview->setSelected(m_listview->firstChild(), true);
  	  m_listview->firstChild()->setPixmap(m_columnPixmap, SmallIcon("ok"));
  	}
  }

}

//-------------------------------------------
void KateSessionPanel::setup_toolbar()
{
  // Toolbar widget and frame
  KateSessionPanelToolBarParent *tbarParent = new KateSessionPanelToolBarParent(this);
  m_toolbar = new TDEToolBar(tbarParent, "Kate Session Panel Toolbar", true);
  tbarParent->setToolBar(m_toolbar);
  m_toolbar->setMovingEnabled(false);
  m_toolbar->setFlat(true);
  m_toolbar->setIconText(TDEToolBar::IconOnly);
  m_toolbar->setIconSize(16);
  m_toolbar->setEnableContextMenu(false);

//FIXME : uncomment and activate as long as the new session manager gets fixed
  // Toolbar actions
  TDEAction *a;

  a = new TDEAction(i18n("New"), SmallIcon("list-add"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotNewSession()), m_actionCollection, "session_new");
  a->setWhatsThis(i18n("Create a new session."));
  a->plug(m_toolbar);
/*
  a = new TDEAction(i18n("Save"), SmallIcon("document-save"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotSaveSession()), m_actionCollection, "session_save");
  a->setWhatsThis(i18n("Save the current session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Save as..."), SmallIcon("document-save-as"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotSaveSessionAs()), m_actionCollection, "session_save_as");
  a->setWhatsThis(i18n("Save the current session with a different name."));
  a->plug(m_toolbar);
*/
  a = new TDEAction(i18n("Rename"), SmallIcon("edit_user"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotRenameSession()), m_actionCollection, "session_rename");
  a->setWhatsThis(i18n("Rename the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Delete"), SmallIcon("edit-delete"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotDeleteSession()), m_actionCollection, "session_delete");
  a->setWhatsThis(i18n("Delete the selected session."));
  a->plug(m_toolbar);

  m_toolbar->insertLineSeparator();

  a = new TDEAction(i18n("Activate"), SmallIcon("forward"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotActivateSession()), m_actionCollection, "session_activate");
  a->setWhatsThis(i18n("Activate the selected session."));
  a->plug(m_toolbar);
/*
	TDEToggleAction *tglA = new TDEToggleAction(i18n("Toggle read only"), SmallIcon("encrypted"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotSessionToggleReadOnly()), m_actionCollection, "session_toggle_read_only");
  tglA->setWhatsThis(i18n("Toggle read only status for the selected session.<p>"
					"In a read only session, you can work as usual but the list of documents in the session "
          "will not be saved when you exit Kate or switch to another session.<p>"
          "You can use this option to create template sessions that you wish to keep unchanged over time."));
  tglA->plug(m_toolbar);
*/

  a = new TDEAction(i18n("Move Up"), SmallIcon("go-up"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotSessionMoveUp()), m_actionCollection, "session_move_up");
  a->setWhatsThis(i18n("Move up the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Move Down"), SmallIcon("go-down"), 0,
          TQT_TQOBJECT(this), TQT_SLOT(slotSessionMoveDown()), m_actionCollection, "session_move_down");
  a->setWhatsThis(i18n("Move down the selected session."));
  a->plug(m_toolbar);
}

//-------------------------------------------
void KateSessionPanel::slotNewSession()
{
	KateSessionNameChooser *nameChooser = new KateSessionNameChooser(this);
  int result = nameChooser->exec();
  if (result == TQDialog::Accepted)
  {
	  m_sessionManager->newSession(nameChooser->getSessionName(), nameChooser->getActivateFlag());
	}
  delete nameChooser;
}

//-------------------------------------------
void KateSessionPanel::slotSaveSession()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::slotSaveSessionAs()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::slotRenameSession()
{
  TQListViewItem *sessionItem = m_listview->selectedItem();
  if (!sessionItem)
    return;

  m_listview->rename(m_listview->selectedItem(), m_columnName);
}

//-------------------------------------------
void KateSessionPanel::slotDeleteSession()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
    return;

  int result = KMessageBox::warningContinueCancel(this,
             	i18n("Do you really want to delete the session '%1'?").arg(sessionItem->text(0)),
							i18n("Delete session"), KStdGuiItem::del());
  if (result == KMessageBox::Continue)
  {
	  m_sessionManager->deleteSession(sessionItem->getSessionId());
	}
}

//-------------------------------------------
void KateSessionPanel::slotActivateSession()
{
  KateSessionPanelItem *newSessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!newSessionItem)
    return;

  int currSessionId = m_sessionManager->getActiveSessionId();
  int newSessionId = newSessionItem->getSessionId();
  if (newSessionId != currSessionId)
  {
    m_sessionManager->activateSession(newSessionId);
  }
}

//-------------------------------------------
void KateSessionPanel::slotSessionToggleReadOnly()
{
//TODO
}

//-------------------------------------------
void KateSessionPanel::slotSessionMoveUp()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
    return;

  m_sessionManager->moveSessionBackward(sessionItem->getSessionId());
}

//-------------------------------------------
void KateSessionPanel::slotSessionMoveDown()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
    return;

  m_sessionManager->moveSessionForward(sessionItem->getSessionId());
}

void KateSessionPanel::slotItemExecuted(TQListViewItem *item)
{
	if (!item)
	  return;

  // First level items are sessions. Executing one, will switch to that session
  if (!item->parent())
  {
    slotActivateSession();
    return;
  }
}

//-------------------------------------------
void KateSessionPanel::slotSessionActivated(int newSessionId, int oldSessionId)
{
  // Move the active session marker
	TQListViewItem *item = m_listview->firstChild();
	for (int idx = 0;  idx < oldSessionId;  ++idx)
	{
		item = item->nextSibling();
	}
	item->setPixmap(m_columnPixmap, TQPixmap());

	item = m_listview->firstChild();
	for (int idx = 0;  idx < newSessionId;  ++idx)
	{
		item = item->nextSibling();
	}
	item->setPixmap(m_columnPixmap, SmallIcon("ok"));
	m_listview->setSelected(item, true);
}

//-------------------------------------------
void KateSessionPanel::slotSessionCreated(int sessionId)
{
  TQPtrList<KateSession>& sessions = m_sessionManager->getSessionsList();
	new KateSessionPanelItem(m_listview, m_listview->lastItem(), sessions[sessionId]->getSessionName(),
	                         sessionId);
}

//-------------------------------------------
void KateSessionPanel::slotSessionDeleted(int sessionId)
{
	// delete item from listview
	TQListViewItem *item = m_listview->firstChild();
	int idx = 0;
	for (;  idx < sessionId;  ++idx)
	{
		item = item->nextSibling();
	}
	TQListViewItem *nextItem = item->nextSibling();
	delete item;
	// update session id of all following items
	item = nextItem;
	while (item)
  {
	  dynamic_cast<KateSessionPanelItem*>(item)->setSessionId(idx++);
		item = item->nextSibling();
	}
}

//-------------------------------------------
void KateSessionPanel::slotSessionsSwapped(int sessionIdMin, int sessionIdMax)
{
  if (sessionIdMin == sessionIdMax)
  {
  	return;
  }

	if (sessionIdMin > sessionIdMax)
	{
	  // this is not executed when the slot is connected to m_sessionManager's
	  // sessionsSwapped(int, int) signal
		int tmp = sessionIdMin;
		sessionIdMin = sessionIdMax;
		sessionIdMax = tmp;
	}

	TQListViewItem *selectedItem = m_listview->selectedItem();

	// Looks for the previous siblings of the two items
	TQListViewItem *siblMin(NULL), *siblMax(NULL), *itemMin(NULL), *itemMax(NULL);
	TQListViewItem *currItem = m_listview->firstChild();
	TQListViewItem *nextItem(NULL);
	while (currItem)
	{
	  nextItem = currItem->nextSibling();
		KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(nextItem);
		if (sessionItem->getSessionId() == sessionIdMin)
		{
			siblMin = currItem;
			itemMin = nextItem;
		}
		else if (sessionItem->getSessionId() == sessionIdMax)
		{
			siblMax = currItem;
			itemMax = nextItem;
			break;
		}
		currItem = nextItem;
	}
	if (!itemMin)
	{
		// The sessionIdMin item was the first of the list
		itemMin = m_listview->firstChild();
	}
	// Remove the two items and place them in their new positions
	m_listview->takeItem(itemMax);
	m_listview->takeItem(itemMin);
	m_listview->insertItem(itemMin);
	m_listview->insertItem(itemMax);
  itemMax->moveItem(siblMin);
  if (siblMax != itemMin)
  {
  	itemMin->moveItem(siblMax);
  }
  else
  {
  	itemMin->moveItem(itemMax);
  }
  // Update item's session id
  (dynamic_cast<KateSessionPanelItem*>(itemMax))->setSessionId(sessionIdMin);
  (dynamic_cast<KateSessionPanelItem*>(itemMin))->setSessionId(sessionIdMax);

  m_listview->setSelected(selectedItem, true);
}

//-------------------------------------------
void KateSessionPanel::slotSessionRenamed(TQListViewItem *item)
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(item);
  if (!sessionItem)
  {
    return;
  }
  m_sessionManager->renameSession(sessionItem->getSessionId(), sessionItem->text(m_columnName));
}
//END KateSessionPanel
