/*
 *  navigatorappitem.h - part of the TDE Help Center
 *
 *  Copyright (C) 2001 Waldo Bastian <bastian@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __navigatorappitem_h___
#define __navigatorappitem_h___

#include "navigatoritem.h"

class KService;

namespace KHC {

class NavigatorAppItem : public NavigatorItem
{
  public:
    NavigatorAppItem( DocEntry *entry, TQListView *parent,
                      const TQString &relPath );
    NavigatorAppItem( DocEntry *entry, TQListViewItem *parent,
                      const TQString &relPath );

    NavigatorAppItem( DocEntry *entry, TQListView *parent,
                      TQListViewItem *after );
    NavigatorAppItem( DocEntry *entry, TQListViewItem *parent,
                      TQListViewItem *after );

    void setRelpath( const TQString & );

    virtual void setOpen(bool);
    bool populate( bool recursive = false );
    virtual TQString key( int column, bool ascending ) const;

  protected:
    TQString documentationURL( KService *s );

  private:
    TQString mRelpath;
    bool mPopulated;
};

}

#endif
