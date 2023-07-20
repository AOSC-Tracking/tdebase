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

#ifndef _KBOOKMARKHANDLER_H_
#define _KBOOKMARKHANDLER_H_

#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>

class KateFileSelector;

class TDEActionMenu;

class TQTextStream;
class TDEPopupMenu;

class KBookmarkHandler : public TQObject, public KBookmarkOwner
{
    TQ_OBJECT

public:
    KBookmarkHandler( KateFileSelector *parent, TDEPopupMenu *tdepopupmenu=0 );
    ~KBookmarkHandler();

    // KBookmarkOwner interface:
    virtual void openBookmarkURL( const TQString& url ) { emit openURL( url ); }
    virtual TQString currentURL() const;

    TDEPopupMenu *menu() const { return m_menu; }

signals:
    void openURL( const TQString& url );

private slots:
    void slotNewBookmark( const TQString& text, const TQCString& url,
                          const TQString& additionalInfo );
    void slotNewFolder( const TQString& text, bool open,
                        const TQString& additionalInfo );
    void newSeparator();
    void endFolder();

protected:
    virtual void virtual_hook( int id, void* data );

private:
    KateFileSelector *mParent;
    TDEPopupMenu *m_menu;
    KBookmarkMenu *m_bookmarkMenu;

    TQTextStream *m_importStream;

    //class KBookmarkHandlerPrivate *d;
};


#endif // _KBOOKMARKHANDLER_H_
