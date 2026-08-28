/* This file was part of the KDE libraries
    Copyright (C) 2002 Carsten Pfeiffer <pfeiffer@kde.org>

    library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation, version 2.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

// Born as tdelibs/tdeio/tdefile/tdefilebookmarkhandler.cpp

#include <tdepopupmenu.h>
#include <tdestandarddirs.h>
#include <kshell.h>
#include <tdeio/job.h>
#include <tdeio/netaccess.h>
#include <kdebug.h>
#include <tqfile.h>

#include "KonsoleApp.h"
#include "konsolebookmarkmenu.h"
#include "konsolebookmarkhandler.h"

namespace Konsole {

KonsoleBookmarkHandler::KonsoleBookmarkHandler( KonsoleApp *konsole, bool toplevel )
    : TQObject( konsole, "KonsoleBookmarkHandler" ),
      KBookmarkOwner(),
      m_konsoleApp( konsole )
{
    m_menu = new TDEPopupMenu( konsole, "bookmark menu" );

    // KDE3.5 - Konsole's bookmarks are now in konsole/bookmarks.xml
    // TODO: Consider removing for KDE4
    TQString new_bm_file = locateLocal( "data", "konsole/bookmarks.xml" );
    if ( !TQFile::exists( new_bm_file ) ) {
        TQString old_bm_file = locateLocal( "data", "tdefile/bookmarks.xml" );
        if ( TQFile::exists( old_bm_file ) )
            // We want sync here... 
            if ( !TDEIO::NetAccess::copy( KURL( old_bm_file ), 
                                   KURL ( new_bm_file ), 0 ) ) {
                kdWarning()<<TDEIO::NetAccess::lastErrorString()<<endl;
            }
    }

    m_file = locate( "data", "konsole/bookmarks.xml" );
    if ( m_file.isEmpty() )
        m_file = locateLocal( "data", "konsole/bookmarks.xml" );

    KBookmarkManager *manager = KBookmarkManager::managerForFile( m_file, false);
    manager->setEditorOptions(tdeApp->caption(), false);
    manager->setUpdate( true );
    manager->setShowNSBookmarks( false );
    
    connect( manager, TQ_SIGNAL( changed(const TQString &, const TQString &) ),
             TQ_SLOT( slotBookmarksChanged(const TQString &, const TQString &) ) );

    if (toplevel) {
        m_bookmarkMenu = new KonsoleBookmarkMenu( manager, this, m_menu,
                                            konsole->actionCollection(), true );
    } else {
        m_bookmarkMenu = new KonsoleBookmarkMenu( manager, this, m_menu,
                                            NULL, false /* Not toplevel */
					    ,false      /* No 'Add Bookmark' */);
    }
}

KonsoleBookmarkHandler::~KonsoleBookmarkHandler()
{
    delete m_bookmarkMenu;
}

TQString KonsoleBookmarkHandler::currentURL() const
{
    return m_konsoleApp->baseURL().prettyURL();
}

TQString KonsoleBookmarkHandler::currentTitle() const
{
    const KURL &u = m_konsoleApp->baseURL();
    if (u.isLocalFile())
    {
       TQString path = u.path();
       path = KShell::tildeExpand(path);
       return path;
    }
    return u.prettyURL();
}

void KonsoleBookmarkHandler::slotBookmarksChanged( const TQString &,
                                                   const TQString &)
{
    // This is called when someone changes bookmarks in konsole....
    m_bookmarkMenu->slotBookmarksChanged("");
}

}

#include "konsolebookmarkhandler.moc"
