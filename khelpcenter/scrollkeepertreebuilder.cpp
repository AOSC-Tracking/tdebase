/*
 *  This file is part of the TDE Help Center
 *
 *  Copyright (C) 1999 Matthias Elter (me@kde.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "scrollkeepertreebuilder.h"

#include "navigatoritem.h"
#include "docentry.h"

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdeprocio.h>

#include <tqdom.h>
#include <tqfile.h>
#include <tqregexp.h>

using namespace KHC;

ScrollKeeperTreeBuilder::ScrollKeeperTreeBuilder( TQObject *parent, const char *name )
	: TQObject( parent, name )
{
	loadConfig();
}

void ScrollKeeperTreeBuilder::loadConfig()
{
  TDEConfig *cfg = kapp->config();
  {
    TDEConfigGroupSaver groupSaver( cfg, "ScrollKeeper" );
    mShowEmptyDirs = cfg->readBoolEntry( "ShowEmptyDirs", false );
  }
}

NavigatorItem *ScrollKeeperTreeBuilder::build( NavigatorItem *parent,
                                               NavigatorItem *after )
{
  TQString lang = TDEGlobal::locale()->language();

  kdDebug(1400) << "ScrollKeeper language: " << lang << endl;

  TDEProcIO proc;
  proc << "scrollkeeper-get-content-list";
  proc << lang;
  connect(&proc,TQ_SIGNAL(readReady(TDEProcIO *)),TQ_SLOT(getContentsList(TDEProcIO *)));
  if (!proc.start(TDEProcess::Block)) {
    kdDebug(1400) << "Could not execute scrollkeeper-get-content-list" << endl;
    return 0;
  }

  if (!TQFile::exists(mContentsList)) {
    kdDebug(1400) << "Scrollkeeper contents file '" << mContentsList
      << "' does not exist." << endl;
    return 0;
  }

  TQDomDocument doc("ScrollKeeperContentsList");
  TQFile f(mContentsList);
  if ( !f.open( IO_ReadOnly ) )
    return 0;
  if ( !doc.setContent( &f ) ) {
    f.close();
    return 0;
  }
  f.close();

  // Create top-level item
  mItems.append(parent);

  TQDomElement docElem = doc.documentElement();

  NavigatorItem *result = 0;

  TQDomNode n = docElem.firstChild();
  while( !n.isNull() ) {
    TQDomElement e = n.toElement();
    if( !e.isNull() ) {
      if (e.tagName() == "sect") {
        NavigatorItem *createdItem;
        insertSection( parent, after, e, createdItem );
        if ( createdItem ) result = createdItem;
      }
    }
    n = n.nextSibling();
  }

  return result;
}

void ScrollKeeperTreeBuilder::getContentsList( TDEProcIO *proc )
{
  TQString filename;
  proc->readln( filename, true );

  mContentsList = filename;
}

int ScrollKeeperTreeBuilder::insertSection( NavigatorItem *parent,
                                            NavigatorItem *after,
                                            const TQDomNode &sectNode,
                                            NavigatorItem *&sectItem )
{
  DocEntry *entry = new DocEntry( "", "", "contents2" );
  sectItem = new NavigatorItem( entry, parent, after );
  sectItem->setAutoDeleteDocEntry( true );
  mItems.append( sectItem );

  int numDocs = 0;  // Number of docs created in this section

  TQDomNode n = sectNode.firstChild();
  while( !n.isNull() ) {
    TQDomElement e = n.toElement();
    if( !e.isNull() ) {
      if ( e.tagName() == "title" ) {
        entry->setName( e.text() );
        sectItem->updateItem();
      } else if (e.tagName() == "sect") {
        NavigatorItem *created;
        numDocs += insertSection( sectItem, 0, e, created );
      } else if (e.tagName() == "doc") {
        insertDoc(sectItem,e);
        ++numDocs;
      }
    }
    n = n.nextSibling();
  }

  // Remove empty sections
  if (!mShowEmptyDirs && numDocs == 0) {
    delete sectItem;
    sectItem = 0;
  }

  return numDocs;
}

void ScrollKeeperTreeBuilder::insertDoc( NavigatorItem *parent,
                                         const TQDomNode &docNode )
{
  DocEntry *entry = new DocEntry( "", "", "text-x-generic-template" );
  NavigatorItem *docItem = new NavigatorItem( entry, parent );
  docItem->setAutoDeleteDocEntry( true );
  mItems.append( docItem );

  TQString url;

  TQDomNode n = docNode.firstChild();
  while( !n.isNull() ) {
    TQDomElement e = n.toElement();
    if( !e.isNull() ) {
      if ( e.tagName() == "doctitle" ) {
        entry->setName( e.text() );
        docItem->updateItem();
      } else if ( e.tagName() == "docsource" ) {
        url.append( e.text() );
      } else if ( e.tagName() == "docformat" ) {
        TQString mimeType = e.text();
        if ( mimeType == "text/html") {
          // Let the HTML part figure out how to get the doc
        } else if ( mimeType == "text/xml" ) {
          if ( url.left( 5 ) == "file:" ) url = url.mid( 5 );
          url.prepend( "ghelp:" );
#if 0
          url.replace( TQRegExp( ".xml$" ), ".html" );
#endif
        } else if ( mimeType == "text/sgml" ) {
          // GNOME docs use this type. We don't have a real viewer for this.
          url.prepend( "file:" );
        } else if ( mimeType.left(5) == "text/" ) {
          url.prepend( "file:" );
        }
      }
    }
    n = n.nextSibling();
  }

  entry->setUrl( url );
}

#include "scrollkeepertreebuilder.moc"
