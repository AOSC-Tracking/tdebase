/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

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

#include "notificationdialog.h"
#include <tqlayout.h>

#include <krun.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <klistbox.h>
#include <tqlabel.h>
#include <tqcheckbox.h>
#include <tqpushbutton.h>

#include "actionlistboxitem.h"
#include "notificationdialogview.h"

NotificationDialog::NotificationDialog( KFileItem medium, NotifierSettings *settings,
                                        TQWidget* parent, const char* name )
	: KDialogBase( parent, name, false, i18n( "Medium Detected" ), Ok|Cancel|User1, Ok, true),
	  m_medium(medium), m_settings( settings )
{
	setCaption( KIO::decodeFileName(m_medium.name()) );
	clearWState( WState_Polished );

	TQWidget *page = new TQWidget( this );
	setMainWidget(page);
	TQVBoxLayout *topLayout = new TQVBoxLayout( page, 0, spacingHint() );

	m_view = new NotificationDialogView( page );

	topLayout->addWidget(m_view);
	m_view->iconLabel->setPixmap( m_medium.pixmap(64) );
	m_view->mimetypeLabel->setText( i18n( "<b>Medium type:</b>" ) + " "
	                              + m_medium.mimeTypePtr()->comment() );

	updateActionsListBox();

	resize( TQSize(400,400).expandedTo( minimumSizeHint() ) );


	m_actionWatcher = new KDirWatch();
	TQString services_dir
		= locateLocal( "data", "konqueror/servicemenus", true );
	m_actionWatcher->addDir( services_dir );

	setButtonText( User1, i18n("Configure...") );

	connect( m_actionWatcher, TQT_SIGNAL( dirty( const TQString & ) ),
	         this, TQT_SLOT( slotActionsChanged( const TQString & ) ) );
	connect( this , TQT_SIGNAL( okClicked() ),
	         this, TQT_SLOT( slotOk() ) );
	connect( this, TQT_SIGNAL( user1Clicked() ),
	         this, TQT_SLOT( slotConfigure() ) );
	connect( m_view->actionsList, TQT_SIGNAL( doubleClicked ( TQListBoxItem*, const TQPoint & ) ),
	         this, TQT_SLOT( slotOk() ) );

	connect( this, TQT_SIGNAL( finished() ),
	         this, TQT_SLOT( delayedDestruct() ) );

	m_actionWatcher->startScan();
	TQPushButton * btn = actionButton( Ok );
	btn->setFocus();
}

NotificationDialog::~NotificationDialog()
{
	delete m_actionWatcher;
	delete m_settings;
}

void NotificationDialog::updateActionsListBox()
{
	m_view->actionsList->clear();

	TQValueList<NotifierAction*> actions
		= m_settings->actionsForMimetype( m_medium.mimetype() );

	TQValueList<NotifierAction*>::iterator it = actions.begin();
	TQValueList<NotifierAction*>::iterator end = actions.end();

	for ( ; it!=end; ++it )
	{
		new ActionListBoxItem( *it, m_medium.mimetype(),
		                       m_view->actionsList );
	}

	m_view->actionsList->setSelected( 0, true );
}


void NotificationDialog::slotActionsChanged(const TQString &/*dir*/)
{
	m_settings->reload();
	updateActionsListBox();
}

void NotificationDialog::slotOk()
{
	TQListBoxItem *item = m_view->actionsList->selectedItem();

	if ( item != 0L )
	{
		ActionListBoxItem *action_item
			= static_cast<ActionListBoxItem*>( item );
		NotifierAction *action = action_item->action();

		launchAction( action );
	}
}

void NotificationDialog::launchAction( NotifierAction *action )
{
	if ( m_view->autoActionCheck->isChecked() )
	{
		m_settings->setAutoAction(  m_medium.mimetype(), action );
		m_settings->save();
	}

	action->execute(m_medium);

	TQDialog::accept();
}

void NotificationDialog::slotConfigure()
{
	KRun::runCommand("kcmshell media");
}

#include "notificationdialog.moc"
