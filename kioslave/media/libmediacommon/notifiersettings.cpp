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

#include "notifiersettings.h"

#include <kglobal.h>
#include <kdesktopfile.h>
#include <kstandarddirs.h>
#include <tqdir.h>
#include <tqfile.h>

#include "notifieropenaction.h"
#include "notifiernothingaction.h"


NotifierSettings::NotifierSettings()
{
	m_supportedMimetypes.append( "media/removable_unmounted" );
	m_supportedMimetypes.append( "media/removable_unmounted_encrypted" );
	m_supportedMimetypes.append( "media/removable_unmounted_decrypted" );
	m_supportedMimetypes.append( "media/removable_mounted" );
	m_supportedMimetypes.append( "media/removable_mounted_decrypted" );
	m_supportedMimetypes.append( "media/camera_unmounted" );
	m_supportedMimetypes.append( "media/camera_mounted" );
	m_supportedMimetypes.append( "media/gphoto2camera" );
	m_supportedMimetypes.append( "media/cdrom_unmounted" );
	m_supportedMimetypes.append( "media/cdrom_unmounted_encrypted" );
	m_supportedMimetypes.append( "media/cdrom_unmounted_decrypted" );
	m_supportedMimetypes.append( "media/cdrom_mounted" );
	m_supportedMimetypes.append( "media/cdrom_mounted_decrypted" );
	m_supportedMimetypes.append( "media/dvd_unmounted" );
	m_supportedMimetypes.append( "media/dvd_unmounted_encrypted" );
	m_supportedMimetypes.append( "media/dvd_unmounted_decrypted" );
	m_supportedMimetypes.append( "media/dvd_mounted" );
	m_supportedMimetypes.append( "media/dvd_mounted_decrypted" );
	m_supportedMimetypes.append( "media/cdwriter_unmounted" );
	m_supportedMimetypes.append( "media/cdwriter_unmounted_encrypted" );
	m_supportedMimetypes.append( "media/cdwriter_unmounted_decrypted" );
	m_supportedMimetypes.append( "media/cdwriter_mounted" );
	m_supportedMimetypes.append( "media/cdwriter_mounted_decrypted" );
	m_supportedMimetypes.append( "media/blankcd" );
	m_supportedMimetypes.append( "media/blankdvd" );
	m_supportedMimetypes.append( "media/audiocd" );
	m_supportedMimetypes.append( "media/dvdvideo" );
	m_supportedMimetypes.append( "media/vcd" );
	m_supportedMimetypes.append( "media/svcd" );
	
	reload();
}

NotifierSettings::~NotifierSettings()
{
	while ( !m_actions.isEmpty() )
	{
		NotifierAction *a = m_actions.first();
		m_actions.remove( a );
		delete a;
	}

	while ( !m_deletedActions.isEmpty() )
	{
		NotifierServiceAction *a = m_deletedActions.first();
		m_deletedActions.remove( a );
		delete a;
	}
}

TQValueList<NotifierAction*> NotifierSettings::actions()
{
	return m_actions;
}

const TQStringList &NotifierSettings::supportedMimetypes()
{
	return m_supportedMimetypes;
}

TQValueList<NotifierAction*> NotifierSettings::actionsForMimetype( const TQString &mimetype )
{
	TQValueList<NotifierAction*> result;

	TQValueList<NotifierAction*>::iterator it = m_actions.begin();
	TQValueList<NotifierAction*>::iterator end = m_actions.end();

	for ( ; it!=end; ++it )
	{
		if ( (*it)->supportsMimetype( mimetype ) )
		{
			result.append( *it );
		}
	}
	
	return result;
}

bool NotifierSettings::addAction( NotifierServiceAction *action )
{
	if ( !m_idMap.contains( action->id() ) )
	{
		m_actions.insert( --m_actions.end(), action );
		m_idMap[ action->id() ] = action;
		return true;
	}
	return false;
}

bool NotifierSettings::deleteAction( NotifierServiceAction *action )
{
	if ( action->isWritable() )
	{
		m_actions.remove( action );
		m_idMap.remove( action->id() );
		m_deletedActions.append( action );

		TQStringList auto_mimetypes = action->autoMimetypes();
		TQStringList::iterator it = auto_mimetypes.begin();
		TQStringList::iterator end = auto_mimetypes.end();

		for ( ; it!=end; ++it )
		{
			action->removeAutoMimetype( *it );
			m_autoMimetypesMap.remove( *it );
		}
		
		return true;
	}
	return false;
}

void NotifierSettings::setAutoAction( const TQString &mimetype, NotifierAction *action )
{
	resetAutoAction( mimetype );
	m_autoMimetypesMap[mimetype] = action;
	action->addAutoMimetype( mimetype );
}


void NotifierSettings::resetAutoAction( const TQString &mimetype )
{
	if ( m_autoMimetypesMap.contains( mimetype ) )
	{
		NotifierAction *action = m_autoMimetypesMap[mimetype];
		action->removeAutoMimetype( mimetype );
		m_autoMimetypesMap.remove(mimetype);
	}
}

void NotifierSettings::clearAutoActions()
{
	TQMap<TQString,NotifierAction*>::iterator it = m_autoMimetypesMap.begin();
	TQMap<TQString,NotifierAction*>::iterator end = m_autoMimetypesMap.end();

	for ( ; it!=end; ++it )
	{
		NotifierAction *action = it.data();
		TQString mimetype = it.key();

		if ( action )
			action->removeAutoMimetype( mimetype );
		m_autoMimetypesMap[mimetype] = 0L;
	}
}

NotifierAction *NotifierSettings::autoActionForMimetype( const TQString &mimetype )
{
	if ( m_autoMimetypesMap.contains( mimetype ) )
	{
		return m_autoMimetypesMap[mimetype];
	}
	else
	{
		return 0L;
	}
}

void NotifierSettings::reload()
{
	while ( !m_actions.isEmpty() )
	{
		NotifierAction *a = m_actions.first();
		m_actions.remove( a );
		delete a;
	}

	while ( !m_deletedActions.isEmpty() )
	{
		NotifierServiceAction *a = m_deletedActions.first();
		m_deletedActions.remove( a );
		delete a;
	}

	m_idMap.clear();
	m_autoMimetypesMap.clear();
	
	NotifierOpenAction *open = new NotifierOpenAction();
	m_actions.append( open );
	m_idMap[ open->id() ] = open;

	TQValueList<NotifierServiceAction*> services = listServices();

	TQValueList<NotifierServiceAction*>::iterator serv_it = services.begin();
	TQValueList<NotifierServiceAction*>::iterator serv_end = services.end();
	
	for ( ; serv_it!=serv_end; ++serv_it )
	{
		m_actions.append( *serv_it );
		m_idMap[ (*serv_it)->id() ] = *serv_it;
	}
	
	NotifierNothingAction *nothing = new NotifierNothingAction();
	m_actions.append( nothing );
	m_idMap[ nothing->id() ] = nothing;

	KConfig config( "medianotifierrc", true );
	TQMap<TQString,TQString> auto_actions_map = config.entryMap( "Auto Actions" );

	TQMap<TQString,TQString>::iterator auto_it = auto_actions_map.begin();
	TQMap<TQString,TQString>::iterator auto_end = auto_actions_map.end();
	
	for ( ; auto_it!=auto_end; ++auto_it )
	{
		TQString mime = auto_it.key();
		TQString action_id = auto_it.data();

		if ( m_idMap.contains( action_id ) )
		{
			setAutoAction( mime, m_idMap[action_id] );
		}
		else
		{
			config.deleteEntry( mime );
		}
	}
}
void NotifierSettings::save()
{
	TQValueList<NotifierAction*>::iterator act_it = m_actions.begin();
	TQValueList<NotifierAction*>::iterator act_end = m_actions.end();

	for ( ; act_it!=act_end; ++act_it )
	{
		NotifierServiceAction *service;
		if ( ( service=dynamic_cast<NotifierServiceAction*>( *act_it ) )
		  && service->isWritable() )
		{
			service->save();
		}
	}
	
	while ( !m_deletedActions.isEmpty() )
	{
		NotifierServiceAction *a = m_deletedActions.first();
		m_deletedActions.remove( a );
		TQFile::remove( a->filePath() );
		delete a;
	}
	
	KSimpleConfig config( "medianotifierrc" );
	config.setGroup( "Auto Actions" );
	
	TQMap<TQString,NotifierAction*>::iterator auto_it = m_autoMimetypesMap.begin();
	TQMap<TQString,NotifierAction*>::iterator auto_end = m_autoMimetypesMap.end();

	for ( ; auto_it!=auto_end; ++auto_it )
	{
		if ( auto_it.data()!=0L )
		{
			config.writeEntry( auto_it.key(), auto_it.data()->id() );
		}
		else
		{
			config.deleteEntry( auto_it.key() );
		}
	}
}

TQValueList<NotifierServiceAction*> NotifierSettings::loadActions( KDesktopFile &desktop ) const
{
	desktop.setDesktopGroup();

	TQValueList<NotifierServiceAction*> services;
	
	const TQString filename = desktop.fileName();
	const TQStringList mimetypes = desktop.readListEntry( "ServiceTypes" );

	TQValueList<KDEDesktopMimeType::Service> type_services
		= KDEDesktopMimeType::userDefinedServices(filename, true);

	TQValueList<KDEDesktopMimeType::Service>::iterator service_it = type_services.begin();
	TQValueList<KDEDesktopMimeType::Service>::iterator service_end = type_services.end();
	for (; service_it!=service_end; ++service_it)
	{
		NotifierServiceAction *service_action
			= new NotifierServiceAction();
		
		service_action->setService( *service_it );
		service_action->setFilePath( filename );
		service_action->setMimetypes( mimetypes );
		
		services += service_action;
	}

	return services;
}


bool NotifierSettings::shouldLoadActions( KDesktopFile &desktop, const TQString &mimetype ) const
{
	desktop.setDesktopGroup();

	if ( desktop.hasKey( "Actions" )
	  && desktop.hasKey( "ServiceTypes" )
	  && !desktop.readBoolEntry( "X-KDE-MediaNotifierHide", false )  )
	{
		const TQStringList actions = desktop.readListEntry( "Actions" );

		if ( actions.size()!=1 )
		{
			return false;
		}
		
		const TQStringList types = desktop.readListEntry( "ServiceTypes" );

		if ( mimetype.isEmpty() )
		{
			TQStringList::ConstIterator type_it = types.begin();
			TQStringList::ConstIterator type_end = types.end();
			for (; type_it != type_end; ++type_it)
			{
				if ( (*type_it).startsWith( "media/" ) )
				{
					return true;
				}
			}
		}
		else if ( types.contains(mimetype) )
		{
			return true;
		}
	}

	return false;
}

TQValueList<NotifierServiceAction*> NotifierSettings::listServices( const TQString &mimetype ) const
{
	TQValueList<NotifierServiceAction*> services;
	TQStringList dirs = KGlobal::dirs()->findDirs("data", "konqueror/servicemenus/");
	
	TQStringList::ConstIterator dir_it = dirs.begin();
	TQStringList::ConstIterator dir_end = dirs.end();
	for (; dir_it != dir_end; ++dir_it)
	{
		TQDir dir( *dir_it );

		TQStringList entries = dir.entryList( "*.desktop", TQDir::Files );

		TQStringList::ConstIterator entry_it = entries.begin();
		TQStringList::ConstIterator entry_end = entries.end();

		for (; entry_it != entry_end; ++entry_it )
		{
			TQString filename = *dir_it + *entry_it;
			
			KDesktopFile desktop( filename, true );
			
			if ( shouldLoadActions(desktop, mimetype) )
			{
				services+=loadActions(desktop);
			}
		}
	}

	return services;
}
