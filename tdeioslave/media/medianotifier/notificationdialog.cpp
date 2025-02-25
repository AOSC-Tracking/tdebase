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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "notificationdialog.h"
#include <tqlayout.h>

#include <krun.h>
#include <tdelocale.h>
#include <tdestandarddirs.h>
#include <tdeio/global.h>
#include <tdelistbox.h>
#include <tqlabel.h>
#include <tqcheckbox.h>
#include <tqpushbutton.h>
#ifdef WITH_TDEHWLIB
#include <tdehardwaredevices.h>
#endif

#include "actionlistboxitem.h"
#include "notificationdialogview.h"

NotificationDialog::NotificationDialog( KFileItem medium, NotifierSettings *settings,
                                        TQWidget* parent, const char* name )
	: KDialogBase( parent, name, false, i18n( "Medium Detected" ), Ok|Cancel|User1, Ok, true),
	  m_medium(medium), m_settings( settings )
{
	setCaption( m_medium.text() );
	clearWState( WState_Polished );

	TQWidget *page = new TQWidget( this );
	setMainWidget(page);
	TQVBoxLayout *topLayout = new TQVBoxLayout( page, 0, spacingHint() );

	m_view = new NotificationDialogView( page );

	topLayout->addWidget(m_view);
	m_view->iconLabel->setPixmap(m_medium.pixmap(64));
	m_view->mimetypeLabel->setText(i18n("<b>Name:</b>") + "&#9;" + m_medium.text() + "<br/>" +
	    i18n("<b>Type:</b>") + "&#9;" + m_medium.mimeTypePtr()->comment() + "<br/>" +
	    i18n("<b>Url:</b>") + "&#9;" + m_medium.url().prettyURL());

	updateActionsListBox();

	resize( TQSize(500,500).expandedTo( minimumSizeHint() ) );


	m_actionWatcher = new KDirWatch();
	TQString services_dir
		= locateLocal( "data", "konqueror/servicemenus", true );
	m_actionWatcher->addDir( services_dir );

	setButtonText( User1, i18n("Configure...") );

	connect( m_actionWatcher, TQ_SIGNAL( dirty( const TQString & ) ),
	         this, TQ_SLOT( slotActionsChanged( const TQString & ) ) );
	connect( this , TQ_SIGNAL( okClicked() ),
	         this, TQ_SLOT( slotOk() ) );
	connect( this, TQ_SIGNAL( user1Clicked() ),
	         this, TQ_SLOT( slotConfigure() ) );
	connect( m_view->actionsList, TQ_SIGNAL( doubleClicked ( TQListBoxItem*, const TQPoint & ) ),
	         this, TQ_SLOT( slotOk() ) );

	connect( this, TQ_SIGNAL( finished() ),
	         this, TQ_SLOT( delayedDestruct() ) );

	m_actionWatcher->startScan();
	TQPushButton * btn = actionButton( Ok );
	btn->setFocus();
}

NotificationDialog::~NotificationDialog()
{
	delete m_actionWatcher;
	delete m_settings;
}

KFileItem NotificationDialog::medium()
{
	return m_medium;
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
	KRun::runCommand("tdecmshell media");
}

#include "notificationdialog.moc"
