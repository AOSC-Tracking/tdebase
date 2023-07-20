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

#ifndef MYKICKOFFSEARCHINTERFACE_H
#define MYKICKOFFSEARCHINTERFACE_H

#include "../interfaces/kickoffsearchinterface.h"

class KMenu;

using namespace KickoffSearch;

class MyKickoffSearchInterface :public KickoffSearchInterface
{
    TQ_OBJECT

public:
    MyKickoffSearchInterface( KMenu*, TQObject* parent, const char* name = 0 );

    bool anotherHitMenuItemAllowed(int cat);
    void addHitMenuItem(HitMenuItem* item);
    void searchOver();
    void initCategoryTitlesUpdate();
    void updateCategoryTitles();

private:
    KMenu* _menu;

};

#endif /* MYKICKOFFSEARCHINTERFACE_H */
