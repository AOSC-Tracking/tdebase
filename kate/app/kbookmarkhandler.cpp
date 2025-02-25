/* This file is part of the KDE project
   Copyright (C) xxxx KFile Authors
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#include "katefileselector.h"

#include <stdio.h>
#include <stdlib.h>

#include <tqtextstream.h>

#include <kbookmarkimporter.h>
#include <tdepopupmenu.h>
#include <ksavefile.h>
#include <tdestandarddirs.h>
#include <tdediroperator.h>
#include <tdeaction.h>

#include "kbookmarkhandler.h"
#include "kbookmarkhandler.moc"


KBookmarkHandler::KBookmarkHandler( KateFileSelector *parent, TDEPopupMenu* tdepopupmenu )
    : TQObject( parent, "KBookmarkHandler" ),
      KBookmarkOwner(),
      mParent( parent ),
      m_menu( tdepopupmenu ),
      m_importStream( 0L )
{
    if (!m_menu)
      m_menu = new TDEPopupMenu( parent, "bookmark menu" );

    TQString file = locate( "data", "kate/fsbookmarks.xml" );
    if ( file.isEmpty() )
        file = locateLocal( "data", "kate/fsbookmarks.xml" );

    KBookmarkManager *manager = KBookmarkManager::managerForFile( file, false);
    manager->setUpdate( true );
    manager->setShowNSBookmarks( false );

    m_bookmarkMenu = new KBookmarkMenu( manager, this, m_menu, 0, true );
}

KBookmarkHandler::~KBookmarkHandler()
{
    //     delete m_bookmarkMenu; ###
}

TQString KBookmarkHandler::currentURL() const
{
    return mParent->dirOperator()->url().url();
}


void KBookmarkHandler::slotNewBookmark( const TQString& /*text*/,
                                            const TQCString& url,
                                            const TQString& additionalInfo )
{
    *m_importStream << "<bookmark icon=\"" << KMimeType::iconForURL( KURL( url ) );
    *m_importStream << "\" href=\"" << TQString::fromUtf8(url) << "\">\n";
    *m_importStream << "<title>" << (additionalInfo.isEmpty() ? TQString(TQString::fromUtf8(url)) : additionalInfo) << "</title>\n</bookmark>\n";
}

void KBookmarkHandler::slotNewFolder( const TQString& text, bool /*open*/,
                                          const TQString& /*additionalInfo*/ )
{
    *m_importStream << "<folder icon=\"bookmark_folder\">\n<title=\"";
    *m_importStream << text << "\">\n";
}

void KBookmarkHandler::newSeparator()
{
    *m_importStream << "<separator/>\n";
}

void KBookmarkHandler::endFolder()
{
    *m_importStream << "</folder>\n";
}

void KBookmarkHandler::virtual_hook( int id, void* data )
{ KBookmarkOwner::virtual_hook( id, data ); }

