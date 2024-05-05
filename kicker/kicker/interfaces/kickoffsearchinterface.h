/***************************************************************************
 *   Copyright (C) 2006 by Stephan Binner <binner@kde.org>                 *
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

#ifndef KICKOFFSEARCHINTERFACE_H
#define KICKOFFSEARCHINTERFACE_H

#include <tqobject.h>

#include <kdemacros.h>

class HitMenuItem;

namespace KickoffSearch
{
    class TDE_EXPORT KickoffSearchInterface :public TQObject
    {
        TQ_OBJECT

    public:
        KickoffSearchInterface( TQObject* parent, const char* name = 0);

    public:
        virtual bool anotherHitMenuItemAllowed(int cat) = 0;
        virtual void addHitMenuItem(HitMenuItem* item) = 0;
        virtual void searchOver() = 0;
        virtual void initCategoryTitlesUpdate() = 0;
        virtual void updateCategoryTitles() = 0;
    };
}

#endif /* SELECTIONINTERFACE_H */

