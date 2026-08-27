// Born as tdelibs/tdeio/tdefile/tdefilebookmarkhandler.cpp

#include <stdio.h>
#include <stdlib.h>

#include <tqtextstream.h>

#include <kbookmarkimporter.h>
#include <kmimetype.h>
#include <tdepopupmenu.h>
#include <ksavefile.h>
#include <tdestandarddirs.h>

#include "konsole_mnu.h"
#include "konsolebookmarkmenu.h"
#include "konsolebookmarkhandler.h"

KonsoleBookmarkHandler::KonsoleBookmarkHandler( KonsoleMenu *konsoleMenu, bool )
    : TQObject( konsoleMenu, "KonsoleBookmarkHandler" ),
      KBookmarkOwner(),
      m_konsoleMenu( konsoleMenu ),
      m_importStream( 0L )
{
    m_menu = new TDEPopupMenu( konsoleMenu, "bookmark menu" );

    TQString file = locate( "data", "konsole/bookmarks.xml" );
    if ( file.isEmpty() )
        file = locateLocal( "data", "konsole/bookmarks.xml" );

    // import old bookmarks
    if ( !TDEStandardDirs::exists( file ) ) {
        TQString oldFile = locate( "data", "tdefile/bookmarks.html" );
        if ( !oldFile.isEmpty() )
            importOldBookmarks( oldFile, file );
    }

    KBookmarkManager *manager = KBookmarkManager::managerForFile( file, false);
    manager->setUpdate( true );
    manager->setShowNSBookmarks( false );

    connect( manager, TQ_SIGNAL( changed(const TQString &, const TQString &) ),
             TQ_SLOT( slotBookmarksChanged(const TQString &, const TQString &) ) );
    m_bookmarkMenu = new KonsoleBookmarkMenu( manager, this, m_menu,
                             NULL, false, /*Not toplevel*/
			     false /*No 'Add Bookmark'*/ );
}

TQString KonsoleBookmarkHandler::currentURL() const
{
    return m_konsoleMenu->baseURL().url();
}

void KonsoleBookmarkHandler::importOldBookmarks( const TQString& path,
                                                 const TQString& destinationPath )
{
    KSaveFile file( destinationPath );
    if ( file.status() != 0 )
        return;

    m_importStream = file.textStream();
    *m_importStream << "<!DOCTYPE xbel>\n<xbel>\n";

    KNSBookmarkImporter importer( path );
    connect( &importer,
             TQ_SIGNAL( newBookmark( const TQString&, const TQCString&, const TQString& )),
             TQ_SLOT( slotNewBookmark( const TQString&, const TQCString&, const TQString& )));
    connect( &importer,
             TQ_SIGNAL( newFolder( const TQString&, bool, const TQString& )),
             TQ_SLOT( slotNewFolder( const TQString&, bool, const TQString& )));
    connect( &importer, TQ_SIGNAL( newSeparator() ), TQ_SLOT( newSeparator() ));
    connect( &importer, TQ_SIGNAL( endMenu() ), TQ_SLOT( endMenu() ));

    importer.parseNSBookmarks( false );

    *m_importStream << "</xbel>";

    file.close();
    m_importStream = 0L;
}

void KonsoleBookmarkHandler::slotNewBookmark( const TQString& /*text*/,
                                            const TQCString& url,
                                            const TQString& additionalInfo )
{
    *m_importStream << "<bookmark icon=\"" << KMimeType::iconForURL( KURL( url ) );
    *m_importStream << "\" href=\"" << TQString::fromUtf8(url) << "\">\n";
    *m_importStream << "<title>" << (additionalInfo.isEmpty() ? TQString(TQString::fromUtf8(url)) : additionalInfo) << "</title>\n</bookmark>\n";
}

void KonsoleBookmarkHandler::slotNewFolder( const TQString& text, bool /*open*/,
                                          const TQString& /*additionalInfo*/ )
{
    *m_importStream << "<folder icon=\"bookmark_folder\">\n<title=\"";
    *m_importStream << text << "\">\n";
}

void KonsoleBookmarkHandler::slotBookmarksChanged( const TQString &,
                                                   const TQString & )
{
    // This is called when someone changes bookmarks in konsole....
    m_bookmarkMenu->slotBookmarksChanged("");
}

void KonsoleBookmarkHandler::newSeparator()
{
    *m_importStream << "<separator/>\n";
}

void KonsoleBookmarkHandler::endFolder()
{
    *m_importStream << "</folder>\n";
}

void KonsoleBookmarkHandler::virtual_hook( int id, void* data )
{ KBookmarkOwner::virtual_hook( id, data ); }

#include "konsolebookmarkhandler.moc"
