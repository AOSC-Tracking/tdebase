#ifndef KONQBOOKMARKMANAGER_H
#define KONQBOOKMARKMANAGER_H

#include <kbookmarkmanager.h>
#include <tdestandarddirs.h>
#include <kurl.h>
#include <tdeio/job.h>
#include <libkonq_export.h>
#include <tdeapplication.h>

class LIBKONQ_EXPORT KonqBookmarkManager
{
public:
    static KBookmarkManager * self()
    {
        if ( !s_bookmarkManager )
        {
            TQString globalBookmarkFile = locate( "data",  TQString::fromLatin1( "konqueror/bookmarks.xml" ) );
            TQString bookmarksFile = locateLocal( "data", TQString::fromLatin1("konqueror/bookmarks.xml" ), true);
            if (globalBookmarkFile != TQString::null && bookmarksFile != TQString::null &&
                globalBookmarkFile != bookmarksFile)
            {
                TDEIO::file_copy(KURL::fromPathOrURL(globalBookmarkFile),
                                 KURL::fromPathOrURL(bookmarksFile));
                tdeApp->processEvents(3000);   // Allows up to 3 seconds to copy the file
            }
            s_bookmarkManager = KBookmarkManager::managerForFile( bookmarksFile );
        }
        return s_bookmarkManager;
    }

private:
    static KBookmarkManager *s_bookmarkManager;
};

#endif
