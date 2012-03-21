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

#ifndef _NOTIFIERSETTINGS_H_
#define _NOTIFIERSETTINGS_H_

#include <tqvaluelist.h>
#include <tqmap.h>

#include "notifieraction.h"
#include "notifierserviceaction.h"


class NotifierSettings
{
public:
	NotifierSettings();
	~NotifierSettings();

	TQValueList<NotifierAction*> actions();
	TQValueList<NotifierAction*> actionsForMimetype( const TQString &mimetype );
	
	bool addAction( NotifierServiceAction *action );
	bool deleteAction( NotifierServiceAction *action );

	void setAutoAction( const TQString &mimetype, NotifierAction *action );
	void resetAutoAction( const TQString &mimetype );
	void clearAutoActions();
	NotifierAction *autoActionForMimetype( const TQString &mimetype );

	const TQStringList &supportedMimetypes();
	
	void reload();
	void save();
	
private:
	TQValueList<NotifierServiceAction*> listServices( const TQString &mimetype = TQString() ) const;
	bool shouldLoadActions( KDesktopFile &desktop, const TQString &mimetype ) const;
	TQValueList<NotifierServiceAction*> loadActions( KDesktopFile &desktop ) const;

	TQStringList m_supportedMimetypes;
	TQValueList<NotifierAction*> m_actions;
	TQValueList<NotifierServiceAction*> m_deletedActions;
	TQMap<TQString,NotifierAction*> m_idMap;
	TQMap<TQString,NotifierAction*> m_autoMimetypesMap;
};
#endif
