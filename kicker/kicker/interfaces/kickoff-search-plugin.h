/***************************************************************************
 *   Copyright (C) 2006 by Stephan Binner <binner@kde.org>                 *
 *   Copyright (c) 2006 Debajyoti Bera <dbera.web@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef KICKOFF_SEARCH_PLUGIN_H
#define KICKOFF_SEARCH_PLUGIN_H

#include "kickoffsearchinterface.h"

#include <tqobject.h>
#include <kurl.h>
#include <kservice.h>
#include <tdemacros.h>

typedef enum {
    ACTIONS = 0,
    APPS,
    BOOKMARKS,
    NOTES,
    MAILS,
    FILES,
    MUSIC,
    WEBHIST,
    CHATS,
    FEEDS,
    PICS,
    VIDEOS,
    DOCS,
    OTHER,
    num_categories
} CATEGORY;

class HitMenuItem
{
public:
    HitMenuItem (int id, int category)
	: id (id), category (category),score(0) { } /* dummy */
    HitMenuItem (TQString name, TQString info, KURL uri, TQString mimetype, int id, int category, TQString icon=TQString::null, int score = 0)
	: display_name (name)
	, display_info (info)
	, uri (uri)
	, mimetype (mimetype)
	, id (id)
	, category (category)
	, icon (icon)
	, score (score)
        , service (NULL) { }

    ~HitMenuItem () { }

    bool operator< (HitMenuItem item)
    {
	return ((category == item.category && score > item.score) || (category == item.category && id < item.id) ||
		(category < item.category));
    }

    // FIXME: We dont really need to store display_name and display_info
    TQString display_name; // name to display
    TQString display_info; // other information to display
    KURL uri; // uri to open when clicked
    TQString mimetype;
    int id; // id of the item in the menu
    int category;
    TQString icon;
    int score;
    KService::Ptr service;

    TQString quotedPath () const
    {
	return uri.path ().replace ('"', "\\\"");
    }
};

namespace KickoffSearch {

    class TDE_EXPORT Plugin : public TQObject
    {
        TQ_OBJECT

    public:
        Plugin(TQObject *parent, const char* name=0);
        virtual ~Plugin();

        virtual bool daemonRunning()=0;
        virtual void query(TQString,bool)=0;

        KickoffSearchInterface * kickoffSearchInterface();
    };
};

#endif /* KICKOFF_SEARCH_PLUGIN_H */
