/*
   This file is part of the TDE project

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

#include "kateapp.h"
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
KateSessionNameChooser::KateSessionNameChooser(TQWidget *parent, bool showSwitchTo)
 : KDialogBase(parent, "", true, i18n("Session Name Chooser"), KDialogBase::User1 | KDialogBase::User2,
               KDialogBase::User2, true, KStdGuiItem::cancel(), KGuiItem(i18n("Continue"), "document-new")),
   m_showSwitchTo(showSwitchTo)          
{
  TQHBox *page = new TQHBox(this);
  //page->setMinimumSize(300, 100);
  setMainWidget(page);

  TQVBox *vb = new TQVBox(page);
  vb->setSpacing(KDialog::spacingHint());

  TQLabel *label = new TQLabel(vb);
  label->setText("Please type the new session name:");

  m_sessionNameLE = new TQLineEdit(vb);
  m_sessionNameLE->setText(i18n(KS_UNNAMED));
  m_sessionNameLE->setFocus();
  
  if (m_showSwitchTo)
  {
		m_activateCB = new TQCheckBox(i18n("Switch to the new session"), vb, NULL);
		m_activateCB->setChecked(true);
	}

  connect(m_sessionNameLE, TQ_SIGNAL(textChanged(const TQString&)), this, TQ_SLOT(slotTextChanged()));
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
  if (m_showSwitchTo)
  {
	  return m_activateCB->isChecked();
	}
	return false;
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
    : TQVBox(parent, name), m_sessionManager(KateSessionManager::self()), 
      m_actionCollection(new TDEActionCollection(this)), m_columnName(-1), m_columnPixmap(-1)
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
  //m_listview->setRootIsDecorated(true);  // FIXME disabled until doc list software is developed


  connect(m_listview, TQ_SIGNAL(selectionChanged()),
          this, TQ_SLOT(slotSelectionChanged()));
  connect(m_listview, TQ_SIGNAL(executed(TQListViewItem*)),
          this, TQ_SLOT(slotItemExecuted(TQListViewItem*)));
  connect(m_listview, TQ_SIGNAL(returnPressed(TQListViewItem*)),
          this, TQ_SLOT(slotItemExecuted(TQListViewItem*)));
  connect(KateApp::self(), TQ_SIGNAL(optionsChanged()),
          this, TQ_SLOT(slotSelectionChanged()));
  connect(m_sessionManager, TQ_SIGNAL(switchOptionChanged()),
          this, TQ_SLOT(slotSelectionChanged()));
  connect(m_sessionManager, TQ_SIGNAL(sessionActivated(int, int)),
          this, TQ_SLOT(slotSessionActivated(int, int)));
  connect(m_sessionManager, TQ_SIGNAL(sessionCreated(int)),
          this, TQ_SLOT(slotSessionCreated(int)));
  connect(m_sessionManager, TQ_SIGNAL(sessionDeleted(int)),
          this, TQ_SLOT(slotSessionDeleted(int)));
  connect(m_sessionManager, TQ_SIGNAL(sessionsSwapped(int, int)),
          this, TQ_SLOT(slotSessionsSwapped(int, int)));
  connect(m_sessionManager, TQ_SIGNAL(sessionRenamed(int)),
          this, TQ_SLOT(slotSessionRenamed(int)));
  connect(m_listview, TQ_SIGNAL(itemRenamed(TQListViewItem*)),
          this, TQ_SLOT(slotLVSessionRenamed(TQListViewItem*)));

  TQPtrList<KateSession>& sessions = m_sessionManager->getSessionsList();
  for (int idx = sessions.count() - 1;  idx >= 0;  --idx)
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

  // Toolbar actions
  TDEAction *a;

  a = new TDEAction(i18n("New"), SmallIcon("list-add"), 0,
          this, TQ_SLOT(slotNewSession()), m_actionCollection, "session_new");
  a->setWhatsThis(i18n("Create a new session and switch to it."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Save"), SmallIcon("document-save"), 0,
          this, TQ_SLOT(slotSaveSession()), m_actionCollection, "session_save");
  a->setWhatsThis(i18n("Save the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Save as..."), SmallIcon("document-save-as"), 0,
          this, TQ_SLOT(slotSaveSessionAs()), m_actionCollection, "session_save_as");
  a->setWhatsThis(i18n("Save an unsaved session with a new name or clone an already saved session "
                       "into a new session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Rename"), SmallIcon("edit_user"), 0,
          this, TQ_SLOT(slotRenameSession()), m_actionCollection, "session_rename");
  a->setWhatsThis(i18n("Rename the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Delete"), SmallIcon("edit-delete"), 0,
          this, TQ_SLOT(slotDeleteSession()), m_actionCollection, "session_delete");
  a->setWhatsThis(i18n("Delete the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Reload"), SmallIcon("reload"), 0,
          this, TQ_SLOT(slotReloadSession()), m_actionCollection, "session_reload");
  a->setWhatsThis(i18n("Reload the last saved state of the selected session."));
  a->plug(m_toolbar);

  m_toolbar->insertLineSeparator();

  a = new TDEAction(i18n("Activate"), SmallIcon("forward"), 0,
          this, TQ_SLOT(slotActivateSession()), m_actionCollection, "session_activate");
  a->setWhatsThis(i18n("Activate the selected session."));
  a->plug(m_toolbar);

	TDEToggleAction *tglA = new TDEToggleAction(i18n("Toggle read only"), SmallIcon("encrypted"), 0,
          this, TQ_SLOT(slotSessionToggleReadOnly()), m_actionCollection, "session_toggle_read_only");
  tglA->setWhatsThis(i18n("Toggle read only status for the selected session.<p>"
					"In a read only session, you can work as usual but the list of documents in the session "
          "will not be saved when you exit Kate or switch to another session.<p>"
          "You can use this option to create template sessions that you wish to keep unchanged over time."));
  tglA->plug(m_toolbar);

  a = new TDEAction(i18n("Move Up"), SmallIcon("go-up"), 0,
          this, TQ_SLOT(slotSessionMoveUp()), m_actionCollection, "session_move_up");
  a->setWhatsThis(i18n("Move up the selected session."));
  a->plug(m_toolbar);

  a = new TDEAction(i18n("Move Down"), SmallIcon("go-down"), 0,
          this, TQ_SLOT(slotSessionMoveDown()), m_actionCollection, "session_move_down");
  a->setWhatsThis(i18n("Move down the selected session."));
  a->plug(m_toolbar);
}

//-------------------------------------------
void KateSessionPanel::slotNewSession()
{
	KateSessionNameChooser *nameChooser = new KateSessionNameChooser(this, false);
  int result = nameChooser->exec();
  if (result == TQDialog::Accepted)
  {
    int res = handleSessionSwitch();
    if (res == KMessageBox::Cancel)
    {
    	return;
    }
    else
    {
		  m_sessionManager->newSession(nameChooser->getSessionName(), res == KMessageBox::Yes);
    }
	}
}

//-------------------------------------------
void KateSessionPanel::slotSaveSession()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
  {
    return;
  }

	int sessId = sessionItem->getSessionId();
  const KateSession *ks = m_sessionManager->getSessionFromId(sessId);
  if (!ks)
  {
    return;
  }

  if (ks->isStillVolatile())
  {
    // Session has never been saved before. Ask user for a session name first
	  slotSaveSessionAs();
  }
  else
  {
	  m_sessionManager->saveSession(sessId);
  	slotSelectionChanged();  // Update the toolbar button status
  }
}

//-------------------------------------------
void KateSessionPanel::slotSaveSessionAs()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
  {
    return;
  }
	int sessId = sessionItem->getSessionId();
  KateSession *ks = m_sessionManager->getSessionFromId(sessId);
  if (!ks)
  {
    return;
  }
  
	// If the session was never saved or named before, the session will be saved with a new name.
	// Otherwise it will be cloned into a new session.
	bool cloneSession = !ks->isStillVolatile();
	// Get new session name
	KateSessionNameChooser *nameChooser = new KateSessionNameChooser(this, cloneSession);
  int result = nameChooser->exec();
  if (result == TQDialog::Accepted)
  {
  	if (!cloneSession)
  	{
  	  // Save unsaved session
		  m_sessionManager->renameSession(sessId, nameChooser->getSessionName());
		  m_sessionManager->saveSession(sessId);
  	}
  	else
  	{
  	  // Clone session
  	  bool activate = nameChooser->getActivateFlag();
			int activeSessionId = m_sessionManager->getActiveSessionId();
			int res = KMessageBox::Yes;
			if (activate && sessId != activeSessionId)
			{
			  // Cloning another session and switching to it at the same time,
			  // handle session switch correctly
				res = handleSessionSwitch();
				if (res == KMessageBox::Cancel)
				{
					return;
				}
			}
	  	m_sessionManager->cloneSession(sessId, nameChooser->getSessionName(), activate, res == KMessageBox::No);
  	}
	}
	
	slotSelectionChanged();  // Update the toolbar button status
}

//-------------------------------------------
void KateSessionPanel::slotRenameSession()
{
  TQListViewItem *sessionItem = m_listview->selectedItem();
  if (!sessionItem)
  {
    return;
  }

  m_listview->rename(m_listview->selectedItem(), m_columnName);
}

//-------------------------------------------
void KateSessionPanel::slotDeleteSession()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
  {
    return;
  }

  int result = KMessageBox::warningContinueCancel(this,
             	i18n("Do you really want to delete the session \"%1\"?").arg(sessionItem->text(0)),
							i18n("Delete session"), KStdGuiItem::del());
  if (result == KMessageBox::Continue)
  {
    int sessionId = sessionItem->getSessionId();
    if (sessionId == m_sessionManager->getActiveSessionId())
    {
			// First check if all documents can be closed safely
			if (KateApp::self()->activeMainWindow())
			{
				if (!KateApp::self()->activeMainWindow()->queryCloseAllDocuments())
					return;
			}
		}
		//FIXME add options to let user decide what to do when deleting the current session
		//(open previous/next session, create new empty session)
	  m_sessionManager->deleteSession(sessionId, KateSessionManager::INVALID_SESSION);
	}
}

//-------------------------------------------
void KateSessionPanel::slotReloadSession()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
  {
    return;
  }
	int sessId = sessionItem->getSessionId();
  if (sessId != m_sessionManager->getActiveSessionId())
  {
    return;
  }

	// Restore active session to the last saved state
	m_sessionManager->reloadActiveSession();
}

//-------------------------------------------
void KateSessionPanel::slotActivateSession()
{
  KateSessionPanelItem *newSessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!newSessionItem)
  {
    return;
  }

  int currSessionId = m_sessionManager->getActiveSessionId();
  int newSessionId = newSessionItem->getSessionId();
  if (newSessionId != currSessionId)
  {
    int res = handleSessionSwitch();
    if (res == KMessageBox::Cancel)
    {
    	return;
    }
    else
    {
    	m_sessionManager->activateSession(newSessionId, res == KMessageBox::Yes);
    }
  }
}

//-------------------------------------------
void KateSessionPanel::slotSessionToggleReadOnly()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
	const KateSession *ks(NULL);
  if (sessionItem)
  {
		ks = m_sessionManager->getSessionFromId(sessionItem->getSessionId());
  }
  if (!sessionItem || !ks)
  {
    return;
  }
  
	m_sessionManager->setSessionReadOnlyStatus(sessionItem->getSessionId(), !ks->isReadOnly());
 	slotSelectionChanged();  // Update the toolbar button status
}

//-------------------------------------------
void KateSessionPanel::slotSessionMoveUp()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
  {
    return;
  }

  m_sessionManager->moveSessionBackward(sessionItem->getSessionId());
}

//-------------------------------------------
void KateSessionPanel::slotSessionMoveDown()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
  if (!sessionItem)
  {
    return;
  }

  m_sessionManager->moveSessionForward(sessionItem->getSessionId());
}

//-------------------------------------------
void KateSessionPanel::slotItemExecuted(TQListViewItem *item)
{
	if (!item)
  {
    return;
  }

  // First level items are sessions. Executing one, will switch to that session.
  // This is only allow when the 'Activate' toolbar button is enabled
  if (!item->parent() && 
      m_actionCollection->action("session_activate")->isEnabled())
  {
    slotActivateSession();
    return;
  }
}

//-------------------------------------------
void KateSessionPanel::slotSelectionChanged()
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(m_listview->selectedItem());
	const KateSession *ks(NULL);
  if (sessionItem)
  {
		ks = m_sessionManager->getSessionFromId(sessionItem->getSessionId());
  }

  TDEToggleAction *readOnlyAction = dynamic_cast<TDEToggleAction*>(
				       					m_actionCollection->action("session_toggle_read_only"));
  if (!sessionItem || !ks || 
      m_sessionManager->getSwitchOption() == KateSessionManager::SWITCH_DISCARD)
  {
    m_actionCollection->action("session_save")->setEnabled(false);
    m_actionCollection->action("session_save_as")->setEnabled(false);
    m_actionCollection->action("session_rename")->setEnabled(false);
    m_actionCollection->action("session_delete")->setEnabled(false);
    m_actionCollection->action("session_reload")->setEnabled(false);
    m_actionCollection->action("session_activate")->setEnabled(false);
    m_actionCollection->action("session_move_up")->setEnabled(false);
    m_actionCollection->action("session_move_down")->setEnabled(false);
    readOnlyAction->setEnabled(false);
    readOnlyAction->setChecked(false);
  }
	else
	{
		if (ks->isReadOnly())
		{
		  // Read only sessions can not be saved or renamed
			m_actionCollection->action("session_save")->setEnabled(false);
	    m_actionCollection->action("session_rename")->setEnabled(false);
      m_actionCollection->action("session_delete")->setEnabled(false);
		}
		else
		{
			m_actionCollection->action("session_save")->setEnabled(true);
	    m_actionCollection->action("session_rename")->setEnabled(true);
      m_actionCollection->action("session_delete")->setEnabled(true);
		}
		if (ks->getSessionFilename().isEmpty())
		{
		  // Unstored sessions can not be made readonly
		  readOnlyAction->setEnabled(false);
	    readOnlyAction->setChecked(false);
		}
		else
		{
		  readOnlyAction->setEnabled(true);
      readOnlyAction->setChecked(ks->isReadOnly());
		}
		int sessId = sessionItem->getSessionId();
		int activeSessId = m_sessionManager->getActiveSessionId();
		m_actionCollection->action("session_save_as")->setEnabled(true);
    m_actionCollection->action("session_reload")->setEnabled(sessId == activeSessId);
    m_actionCollection->action("session_activate")->setEnabled(sessId != activeSessId);
    m_actionCollection->action("session_move_up")->setEnabled(sessId > 0);
    m_actionCollection->action("session_move_down")->setEnabled(sessId < (m_sessionManager->getSessionCount() - 1));
	}

	emit selectionChanged();
}

//-------------------------------------------
void KateSessionPanel::slotSessionActivated(int newSessionId, int oldSessionId)
{
  // Move the active session marker
	TQListViewItem *item = NULL;
  if (oldSessionId != KateSessionManager::INVALID_SESSION)
  {
    // Old volatile sessions may have already been deleted. 
    // Remove the marker only for valid sessions.
		item = m_listview->firstChild();
		for (int idx = 0;  idx < oldSessionId;  ++idx)
		{
			item = item->nextSibling();
		}
		item->setPixmap(m_columnPixmap, TQPixmap());
	}	

	item = m_listview->firstChild();
	for (int idx = 0;  idx < newSessionId;  ++idx)
	{
		item = item->nextSibling();
	}
	item->setPixmap(m_columnPixmap, SmallIcon("ok"));
	m_listview->setSelected(item, true);
 	slotSelectionChanged();  // Update the toolbar button status
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
void KateSessionPanel::slotSessionRenamed(int sessionId)
{
	TQListViewItem *item = m_listview->firstChild();
	for (int idx = 0;  idx < sessionId;  ++idx)
	{
		item = item->nextSibling();
	}
	item->setText(m_columnName, m_sessionManager->getSessionName(sessionId));
}

//-------------------------------------------
void KateSessionPanel::slotLVSessionRenamed(TQListViewItem *item)
{
  KateSessionPanelItem *sessionItem = dynamic_cast<KateSessionPanelItem*>(item);
  if (!sessionItem)
  {
    return;
  }

  m_sessionManager->renameSession(sessionItem->getSessionId(), sessionItem->text(m_columnName));
}

//-------------------------------------------
int KateSessionPanel::handleSessionSwitch()
{
  const KateSession *ks = m_sessionManager->getActiveSession();
  int switchOption = m_sessionManager->getSwitchOption();
  if (!ks || switchOption == KateSessionManager::SWITCH_DISCARD)
  {
    return KMessageBox::No;
  }
  
  if (switchOption == KateSessionManager::SWITCH_ASK)
  {
    KDialogBase *dlg = new KDialogBase(i18n("Save Session"),
            KDialogBase::Yes | KDialogBase::No | KDialogBase::Cancel,
            KDialogBase::Cancel, KDialogBase::Cancel, NULL, NULL, true, false,
            KStdGuiItem::save(), KStdGuiItem::del(), KStdGuiItem::cancel());
    bool dontAgain = false;
    int res = KMessageBox::createKMessageBox(dlg, TQMessageBox::Warning,
                    i18n("<p>Do you want to save the current session?<p>!!NOTE!!"
                         "<p>The session will be removed if you choose \"Delete\""), TQStringList(),
                    i18n("Do not ask again"), &dontAgain, KMessageBox::Notify);
    if (res == KDialogBase::Cancel)
    {
      return KMessageBox::Cancel;
    }
    if (dontAgain)
    {
      if (res == KDialogBase::No)
      {
        m_sessionManager->setSwitchOption(KateSessionManager::SWITCH_DISCARD);
      }
      else
      {
        m_sessionManager->setSwitchOption(KateSessionManager::SWITCH_SAVE);
      }
    }
		if (res == KDialogBase::No)
		{
			return KMessageBox::No;
		}
  }
  
  // At this point the session needs to be saved. 
  // Make sure to handle volatile sessions correctly.
  if (ks->isStillVolatile())
  {
		KateSessionNameChooser *nameChooser = new KateSessionNameChooser(this, false);
		int res = nameChooser->exec();
		if (res == TQDialog::Accepted)
		{
			m_sessionManager->renameSession(m_sessionManager->getActiveSessionId(), nameChooser->getSessionName());
		}
		else
		{
			return KMessageBox::Cancel;
		}
  }

	return KMessageBox::Yes;
}
//END KateSessionPanel
