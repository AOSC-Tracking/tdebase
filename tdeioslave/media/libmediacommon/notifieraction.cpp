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

#include "notifieraction.h"

#include <tqfile.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <kicontheme.h>

NotifierAction::NotifierAction()
{
}

NotifierAction::~NotifierAction()
{
}

void NotifierAction::setIconName(const TQString &iconName)
{
	m_iconName = iconName;
}

void NotifierAction::setLabel(const TQString &label)
{
	m_label = label;
}

TQString NotifierAction::iconName() const
{
	return m_iconName;
}

TQPixmap NotifierAction::pixmap() const
{
	TQFile f( m_iconName );
	
	if ( f.exists() )
	{
		return TQPixmap( m_iconName );
	}
	else
	{
	  return TDEGlobal::iconLoader()->loadIcon(m_iconName, TDEIcon::NoGroup, TDEIcon::SizeSmall);
	}
}

TQString NotifierAction::label() const
{
	return m_label;
}

void NotifierAction::addAutoMimetype( const TQString &mimetype )
{
	if ( !m_autoMimetypes.contains( mimetype ) )
	{
		m_autoMimetypes.append( mimetype );
	}
}

void NotifierAction::removeAutoMimetype( const TQString &mimetype )
{
	m_autoMimetypes.remove( mimetype );
}

TQStringList NotifierAction::autoMimetypes()
{
	return m_autoMimetypes;
}

bool NotifierAction::isWritable() const
{
	return false;
}

bool NotifierAction::supportsMimetype(const TQString &/*mimetype*/) const
{
	return true;
}

