/* This file is part of the KDE project
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>
   Copyright (c) 1998, 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kapplication.h"

#include "konq_xmlguiclient.h"
#include <kdebug.h>

class KonqXMLGUIClient::Private
{
public:
    Private() : attrName( "name" ), separatorPending( false ), hasAction( false ) {}
    TQString attrName;
    bool separatorPending;
    bool hasAction;
};

KonqXMLGUIClient::KonqXMLGUIClient( ) : KXMLGUIClient( )
{
  d = new Private;
  prepareXMLGUIStuff( );
}

KonqXMLGUIClient::KonqXMLGUIClient( KXMLGUIClient *parent ) : KXMLGUIClient(parent )
{
  d = new Private;
  prepareXMLGUIStuff( );
}

void KonqXMLGUIClient::prepareXMLGUIStuff()
{
  m_doc = TQDomDocument( "kpartgui" );

  TQDomElement root = m_doc.createElement( "kpartgui" );
  m_doc.appendChild( root );
  root.setAttribute( d->attrName, "popupmenu" );

  m_menuElement = m_doc.createElement( "Menu" );
  root.appendChild( m_menuElement );
  m_menuElement.setAttribute( d->attrName, "popupmenu" );

  /*m_builder = new KonqPopupMenuGUIBuilder( this );
  m_factory = new KXMLGUIFactory( m_builder ); */
}

TQDomElement KonqXMLGUIClient::DomElement() const
{
  return m_menuElement;
}

TQDomDocument KonqXMLGUIClient::domDocument() const
{
  return m_doc;
}

void KonqXMLGUIClient::addAction( KAction *act, const TQDomElement &menu )
{
  addAction( act->name(), menu );
}

void KonqXMLGUIClient::addAction( const char *name, const TQDomElement &menu )
{
  static const TQString& tagAction = KGlobal::staticQString( "action" );

  if (!kapp->authorizeKAction(name))
     return;

  handlePendingSeparator();
  TQDomElement parent = menu;
  if ( parent.isNull() ) {
    parent = m_menuElement;
  }

  TQDomElement e = m_doc.createElement( tagAction );
  parent.appendChild( e );
  e.setAttribute( d->attrName, name );
  d->hasAction = true;
}

void KonqXMLGUIClient::addSeparator( const TQDomElement &menu )
{
  static const TQString& tagSeparator = KGlobal::staticQString( "separator" );

  TQDomElement parent = menu;
  if ( parent.isNull() ) {
    parent = m_menuElement;
  }

  parent.appendChild( m_doc.createElement( tagSeparator ) );

  d->separatorPending = false;
}

//void KonqXMLGUIClient::addWeakSeparator()
//{
//  static const TQString& tagWeakSeparator = KGlobal::staticQString( "weakSeparator" );
//  m_menuElement.appendChild( m_doc.createElement( tagWeakSeparator ) );
//}

void KonqXMLGUIClient::addMerge( const TQString &name )
{
  // can't call handlePendingSeparator. Merge could be empty
  // (testcase: RMB in embedded katepart)
  TQDomElement merge = m_doc.createElement( "merge" );
  m_menuElement.appendChild( merge );
  if ( !name.isEmpty() )
    merge.setAttribute( d->attrName, name );
}

void KonqXMLGUIClient::addGroup( const TQString &grp )
{
  handlePendingSeparator();
  TQDomElement group = m_doc.createElement( "definegroup" );
  m_menuElement.appendChild( group );
  group.setAttribute( d->attrName, grp );
}

KonqXMLGUIClient::~KonqXMLGUIClient()
{
  delete d;
}

void KonqXMLGUIClient::handlePendingSeparator()
{
  if ( d->separatorPending ) {
    addSeparator();
  }
}

void KonqXMLGUIClient::addPendingSeparator()
{
  d->separatorPending = true;
}

bool KonqXMLGUIClient::hasAction() const
{
  return d->hasAction;
}


