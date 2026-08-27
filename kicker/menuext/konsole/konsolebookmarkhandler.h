// Born as tdelibs/tdeio/tdefile/tdefilebookmarkhandler.h

#ifndef KONSOLEBOOKMARKHANDLER_H
#define KONSOLEBOOKMARKHANDLER_H

#include <kbookmarkmanager.h>
#include "konsolebookmarkmenu.h"


class TQTextStream;
class TDEPopupMenu;
class KonsoleBookmarkMenu;
class KonsoleMenu;

class KonsoleBookmarkHandler : public TQObject, public KBookmarkOwner
{
    TQ_OBJECT

public:
    KonsoleBookmarkHandler( KonsoleMenu *konsole, bool toplevel );

    TQPopupMenu * popupMenu();

    // KBookmarkOwner interface:
    virtual void openBookmarkURL( const TQString& url, const TQString& title )
                                { emit openURL( url, title ); }
    virtual TQString currentURL() const;

    TDEPopupMenu *menu() const { return m_menu; }

signals:
    void openURL( const TQString& url, const TQString& title );

private slots:
    // for importing
    void slotNewBookmark( const TQString& text, const TQCString& url,
                          const TQString& additionalInfo );
    void slotNewFolder( const TQString& text, bool open,
                        const TQString& additionalInfo );
    void slotBookmarksChanged( const TQString &, const TQString & caller );
    void newSeparator();
    void endFolder();

private:
    void importOldBookmarks( const TQString& path, const TQString& destinationPath );

    KonsoleMenu *m_konsoleMenu;
    TDEPopupMenu *m_menu;
    KonsoleBookmarkMenu *m_bookmarkMenu;
    TQTextStream *m_importStream;

protected:
    virtual void virtual_hook( int id, void* data );
private:
    class KonsoleBookmarkHandlerPrivate;
    KonsoleBookmarkHandlerPrivate *d;
};


#endif // KONSOLEBOOKMARKHANDLER_H
