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

#ifndef _NOTIFICATIONDIALOG_H_
#define _NOTIFICATIONDIALOG_H_

#include <kdialogbase.h>
#include <tdelistbox.h>
#include <kdirwatch.h>
#include <tdefileitem.h>

#include "notifiersettings.h"
#include "notifieraction.h"
#include "notificationdialogview.h"

class NotificationDialog : public KDialogBase
{
	TQ_OBJECT

public:
	NotificationDialog( KFileItem medium, NotifierSettings *settings,
	                    TQWidget* parent = 0, const char* name = 0 );
	~NotificationDialog();

	KFileItem medium();

private slots:
	void slotOk();
	void slotConfigure();
	void slotActionsChanged(const TQString& dir);

private:
	void launchAction( NotifierAction *action );
	void updateActionsListBox();

	KFileItem m_medium;
	NotifierSettings *m_settings;
	KDirWatch * m_actionWatcher;
	NotificationDialogView *m_view;
};

#endif
