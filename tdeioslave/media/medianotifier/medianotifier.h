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

#ifndef _MEDIANOTIFIER_H_
#define _MEDIANOTIFIER_H_

#include <kdedmodule.h>
#include <tdefileitem.h>
#include <tdeio/job.h>
#include <tdemessagebox.h>

#include <tqstring.h>
#include <tqmap.h>

class KDialogBase;
class NotificationDialog;
typedef TQPtrList<NotificationDialog> NotificationDialogList;

class MediaNotifier:  public KDEDModule
{
	TQ_OBJECT
	K_DCOP

public:
	MediaNotifier( const TQCString &name );
	virtual ~MediaNotifier();

k_dcop:
	void onMediumChange( const TQString &name, bool allowNotification );
	void onMediumRemove( const TQString &name, bool allowNotification );

private slots:
	void slotStatResult( TDEIO::Job *job );
	void checkFreeDiskSpace();
	void slotFreeFinished( KMessageBox::ButtonCode );
	void slotFreeContinue();
	void slotFreeCancel();
	void notificationDialogDestroyed( TQObject* );
	
private:
	bool autostart( const KFileItem &medium );
	void notify( KFileItem &medium );
	
	bool execAutorun( const KFileItem &medium, const TQString &path,
	                  const TQString &autorunFile );
	bool execAutoopen( const KFileItem &medium, const TQString &path,
	                   const TQString &autoopenFile );

	TQMap<TDEIO::Job*,bool> m_allowNotificationMap;
	TQTimer * m_freeTimer;
	KDialogBase * m_freeDialog;
	NotificationDialogList m_notificationDialogList;
};
#endif

