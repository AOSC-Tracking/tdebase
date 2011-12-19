/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __konqdrag_h__
#define __konqdrag_h__

#include <tqdragobject.h>
#include <tqrect.h>
#include <tqstring.h>
#include <tqiconview.h>

#include <libkonq_export.h>

#include <kurl.h>

/*****************************************************************************
 *
 * Class KonqIconDrag
 *
 *****************************************************************************/

// Clipboard/dnd data for: Icons + URLS + isCut
class LIBKONQ_EXPORT KonqIconDrag : public TQIconDrag
{
    Q_OBJECT

public:
    KonqIconDrag( TQWidget * dragSource, const char* name = 0 );
    virtual ~KonqIconDrag() {}

    const char* format( int i ) const;
    TQByteArray encodedData( const char* mime ) const;

    void append( const TQIconDragItem &item, const TQRect &pr,
                 const TQRect &tr, const TQString &url );

    void setMoveSelection( bool move ) { m_bCutSelection = move; }

    static bool canDecode( const TQMimeSource* e );

protected: // KDE4: private. And d pointer...
    TQStringList urls;
    bool m_bCutSelection;
};

/**
 * Clipboard/dnd data for: Icons + URLs + MostLocal URLs + isCut
 * KDE4: merge with KonqIconDrag
 * @since 3.5
 */
class LIBKONQ_EXPORT KonqIconDrag2 : public KonqIconDrag
{
    Q_OBJECT

public:
    KonqIconDrag2( TQWidget * dragSource );
    virtual ~KonqIconDrag2() {}

    virtual const char* format( int i ) const;
    virtual TQByteArray encodedData( const char* mime ) const;

    void append( const TQIconDragItem &item, const TQRect &pr,
                 const TQRect &tr, const TQString &url, const KURL &mostLocalURL );

private:
    TQStringList m_kdeURLs;
};

// Clipboard/dnd data for: URLS + isCut
class LIBKONQ_EXPORT KonqDrag : public TQUriDrag
{
public:
    // KDE4: remove, use KonqDrag constructor instead
    static KonqDrag * newDrag( const KURL::List & urls,
                               bool move, TQWidget * dragSource = 0, const char* name = 0 );

    /**
     * Create a KonqDrag object.
     * @param urls a list of URLs, which can use KDE-specific protocols, like system:/
     * @param mostLocalUrls a list of URLs, which should be resolved to most-local urls, i.e. file:/
     * @param cut false for copying, true for "cutting"
     * @param dragSource parent object
     * @since 3.5
     */
    KonqDrag( const KURL::List & urls, const KURL::List& mostLocalUrls, bool cut, TQWidget * dragSource = 0 );

protected:
    // KDE4: remove
    KonqDrag( const TQStrList & urls, bool cut, TQWidget * dragSource, const char* name );

public:
    virtual ~KonqDrag() {}

    virtual const char* format( int i ) const;
    virtual TQByteArray encodedData( const char* mime ) const;

    void setMoveSelection( bool move ) { m_bCutSelection = move; }

    // Returns true if the data was cut (used for KonqIconDrag too)
    static bool decodeIsCutSelection( const TQMimeSource *e );

protected: // KDE4: private. And d pointer...
    bool m_bCutSelection;
    TQStrList m_urls; // this is set to the "most local urls". KDE4: KURL::List
};

#endif
