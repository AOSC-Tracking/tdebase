/*
 *  This file is part of the TDE Help Center
 *
 *  Copyright (C) 2001 Waldo Bastian <bastian@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as 
 *  published by the Free Software Foundation.
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

#include "navigatorappitem.h"

#include "docentry.h"

#include <kdebug.h>
#include <kservicegroup.h>

using namespace KHC;

NavigatorAppItem::NavigatorAppItem( DocEntry *entry, TQListView *parent,
                  const TQString &relPath )
  : NavigatorItem( entry, parent ),
    mRelpath( relPath ),
    mPopulated( false )
{
  setExpandable( true );
}

NavigatorAppItem::NavigatorAppItem( DocEntry *entry, TQListViewItem *parent,
                  const TQString &relPath )
  : NavigatorItem( entry, parent ),
    mRelpath( relPath ),
    mPopulated( false )
{
  setExpandable( true );
}

NavigatorAppItem::NavigatorAppItem( DocEntry *entry, TQListView *parent,
                  TQListViewItem *after )
  : NavigatorItem( entry, parent, after ),
    mPopulated( false )
{
  setExpandable( true );
}

NavigatorAppItem::NavigatorAppItem( DocEntry *entry, TQListViewItem *parent,
                  TQListViewItem *after )
  : NavigatorItem( entry, parent, after ),
    mPopulated( false )
{
  setExpandable( true );
}

void NavigatorAppItem::setRelpath( const TQString &relpath )
{
  mRelpath = relpath;
}

void NavigatorAppItem::setOpen(bool open)
{
  kdDebug() << "NavigatorAppItem::setOpen()" << endl;

  if ( open && (childCount() == 0) && !mPopulated )
  {
     kdDebug() << "NavigatorAppItem::setOpen(" << this << ", "
               << mRelpath << ")" << endl;
     populate();
  }
  TQListViewItem::setOpen(open); 
}

bool NavigatorAppItem::populate( bool recursive )
{
  bool entriesAdded = false;

  if ( mPopulated ) return false;

  KServiceGroup::Ptr root = KServiceGroup::group(mRelpath);
  if ( !root ) {
    kdWarning() << "No Service groups\n";
    return false;
  }
  KServiceGroup::List list = root->entries();


  for ( KServiceGroup::List::ConstIterator it = list.begin(); it != list.end(); ++it )
  {
    KSycocaEntry * e = *it;
    KService::Ptr s;
    NavigatorItem *item;
    KServiceGroup::Ptr g;
    TQString url;

    switch ( e->sycocaType() ) {
      case KST_KService:
      {
        s = static_cast<KService*>(e);
        url = documentationURL( s );
        if ( !url.isEmpty() ) {
          DocEntry *entry = new DocEntry( s->name(), url, s->icon() );
          item = new NavigatorItem( entry, this );
          item->setAutoDeleteDocEntry( true );
          item->setExpandable( false );
          entriesAdded = true;
        }
        break;
      }
      case KST_KServiceGroup:
      {
        g = static_cast<KServiceGroup*>(e);
        if ( ( g->childCount() == 0 ) || g->name().startsWith( "." ) ) {
          continue;
        }
        KServiceGroup::List entryList = g->entries(false, true, false, false);
        if (entryList.count() > 0) {
          int entryCount = 0;
          for( KServiceGroup::List::ConstIterator it2 = entryList.begin(); it2 != entryList.end(); it2++)
          {
            KSycocaEntry *p = (*it2);
            if (p->isType(KST_KService))
            {
              KService *s = static_cast<KService *>(p);
              url = documentationURL( s );
              if ( !url.isEmpty() ){
                entryCount++;
              }
            }
          }
          if (entryCount > 0) {
            DocEntry *entry = new DocEntry( g->caption(), "", g->icon() );
            NavigatorAppItem *appItem;
            appItem = new NavigatorAppItem( entry, this, g->relPath() );
            appItem->setAutoDeleteDocEntry( true );
            if ( recursive ) appItem->populate( recursive );
            entriesAdded = true;
          }
        }
        break;
      }
      default:
        break;
    }
  }
  sortChildItems( 0, true /* ascending */ );
  mPopulated = true;

  return entriesAdded;
}

TQString NavigatorAppItem::key( int column, bool ascending ) const
{
  return text( column ).lower();
}

TQString NavigatorAppItem::documentationURL( KService *s )
{
  TQString docPath = s->property( "X-DocPath" ).toString();
  if ( docPath.isEmpty() )
    return TQString::null;
  
  if ( docPath.startsWith( "file:") || docPath.startsWith( "http:" ) )
    return docPath;
  
  return TQString( "help:/" ) + docPath;
}
