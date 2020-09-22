/*
 *  This file is part of the TDE Help Center
 *
 *  Copyright (C) 2002 Frerich Raabe (raabe@kde.org)
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

#include "infotree.h"

#include "navigatoritem.h"
#include "docentry.h"

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <tqfile.h>
#include <tqtextstream.h>

#include <stdlib.h>  // for getenv()

using namespace KHC;

class InfoCategoryItem : public NavigatorItem
{
  public:
    InfoCategoryItem( NavigatorItem *parent, const TQString &text );
	
    virtual void setOpen( bool open );
};

class InfoNodeItem : public NavigatorItem
{
  public:
    InfoNodeItem( InfoCategoryItem *parent, const TQString &text );
};

InfoCategoryItem::InfoCategoryItem( NavigatorItem *parent, const TQString &text )
  : NavigatorItem( new DocEntry( text ), parent )
{
  setAutoDeleteDocEntry( true );
  setOpen( false );
//  kdDebug(1400) << "Got category: " << text << endl;
}

void InfoCategoryItem::setOpen( bool open )
{
  NavigatorItem::setOpen( open );

  if ( open && childCount() > 0 ) setPixmap( 0, SmallIcon( "contents" ) );
  else setPixmap( 0, SmallIcon( "contents2" ) );
}

InfoNodeItem::InfoNodeItem( InfoCategoryItem *parent, const TQString &text )
  : NavigatorItem( new DocEntry( text ), parent )
{
  setAutoDeleteDocEntry( true );
//  kdDebug( 1400 ) << "Created info node item: " << text << endl;
}

InfoTree::InfoTree( TQObject *parent, const char *name )
  : TreeBuilder( parent, name ),
    m_parentItem( 0 )
{
}

void InfoTree::build( NavigatorItem *parent )
{
  kdDebug( 1400 ) << "Populating info tree." << endl;

  m_parentItem = parent;

  DocEntry *entry = new DocEntry( i18n( "Alphabetically" ) );
  m_alphabItem = new NavigatorItem( entry, parent );
  m_alphabItem->setAutoDeleteDocEntry( true );
  entry = new DocEntry( i18n( "By Category" ) );
  m_categoryItem = new NavigatorItem( entry, parent );
  m_categoryItem->setAutoDeleteDocEntry( true );

  TDEConfig *cfg = kapp->config();
  cfg->setGroup( "Info pages" );
  TQStringList infoDirFiles = cfg->readListEntry( "Search paths" );
  // Default paths taken fron tdebase/tdeioslave/info/kde-info2html.conf
  if ( infoDirFiles.isEmpty() ) { 
    infoDirFiles << "/usr/share/info";
    infoDirFiles << "/usr/info";
    infoDirFiles << "/usr/lib/info";
    infoDirFiles << "/usr/local/info";
    infoDirFiles << "/usr/local/lib/info";
    infoDirFiles << "/usr/X11R6/info";
    infoDirFiles << "/usr/X11R6/lib/info";
    infoDirFiles << "/usr/X11R6/lib/xemacs/info";
  }

  TQString infoPath = ::getenv( "INFOPATH" );
  if ( !infoPath.isEmpty() )
    infoDirFiles += TQStringList::split( ':', infoPath );

  TQStringList::ConstIterator it = infoDirFiles.begin();
  TQStringList::ConstIterator end = infoDirFiles.end();
  for ( ; it != end; ++it ) {
    TQString infoDirFileName = *it + "/dir";
    if ( TQFile::exists( infoDirFileName ) )
      parseInfoDirFile( infoDirFileName );
  }

  m_alphabItem->sortChildItems( 0, true /* ascending */ );
}

void InfoTree::parseInfoDirFile( const TQString &infoDirFileName )
{
  kdDebug( 1400 ) << "Parsing info dir file " << infoDirFileName << endl;

  TQFile infoDirFile( infoDirFileName );
  if ( !infoDirFile.open( IO_ReadOnly ) )
    return;

  TQTextStream stream( &infoDirFile );
  // Skip introduction blurb.
  while ( !stream.eof() && !stream.readLine().startsWith( "* Menu:" ) );

  while ( !stream.eof() ) {
    TQString s = stream.readLine();
    if ( s.stripWhiteSpace().isEmpty() )
      continue;

    InfoCategoryItem *catItem = new InfoCategoryItem( m_categoryItem, s );
    while ( !stream.eof() && !s.stripWhiteSpace().isEmpty() ) {
      s = stream.readLine();
      if ( s[ 0 ] == '*' ) {
        const int colon = s.find( ":" );
        const int openBrace = s.find( "(", colon );
        const int closeBrace = s.find( ")", openBrace );
        const int dot = s.find( ".", closeBrace );

        TQString appName = s.mid( 2, colon - 2 );
        TQString url = "info:/" + s.mid( openBrace + 1, closeBrace - openBrace - 1 );
        if ( dot - closeBrace > 1 )
          url += "/" + s.mid( closeBrace + 1, dot - closeBrace - 1 );
        else
          url += "/Top";

        InfoNodeItem *item = new InfoNodeItem( catItem, appName );
        item->entry()->setUrl( url );

        InfoCategoryItem *alphabSection = 0;
        for ( TQListViewItem* it=m_alphabItem->firstChild(); it; it=it->nextSibling() ) {
          if ( it->text( 0 ) == appName[ 0 ].upper() ) {
            alphabSection = static_cast<InfoCategoryItem *>( it );
            break;
          }
        }

        if ( alphabSection == 0 )
          alphabSection = new InfoCategoryItem( m_alphabItem, appName[ 0 ].upper() );

        item = new InfoNodeItem( alphabSection, appName );
        item->entry()->setUrl( url );
      }
    }
  }
  infoDirFile.close();
}

#include "infotree.moc"
