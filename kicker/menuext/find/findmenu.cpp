/*
    This file is part of Kicker.
    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <tdeapplication.h>
#include <kiconloader.h>
#include <tdesimpleconfig.h>
#include <tdestandarddirs.h>

#include "findmenu.h"

K_EXPORT_KICKER_MENUEXT( find, FindMenu )


FindMenu::FindMenu( TQWidget *parent, const char *name,
                    const TQStringList &/*args*/)
  : KPanelMenu( "", parent, name )
{
}

FindMenu::~FindMenu()
{
    mConfigList.clear();
}

void FindMenu::initialize()
{
  TQStringList list = TDEGlobal::dirs()->findAllResources( "data", "kicker/menuext/find/*.desktop", false, true );

  list.sort();

  int id = 0;

  mConfigList.clear();
  for ( TQStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
    TDESimpleConfig config( *it, true );
    config.setDesktopGroup();

    mConfigList.append( *it );
    TQString text = config.readEntry( "Name" );

    insertItem( SmallIconSet( config.readEntry( "Icon" ) ), text, id );
    id++;
  }
}

void FindMenu::slotExec( int pos )
{
  TQString app = mConfigList[ pos ];

  tdeApp->propagateSessionManager();

  TDESimpleConfig config(app, true);
  config.setDesktopGroup();
  if (tdeApp && config.readEntry("Type") == "Link")
  {
    tdeApp->invokeBrowser(config.readEntry("URL"));
  }
  else
  {
    TDEApplication::startServiceByDesktopPath( app );
  }
}

#include "findmenu.moc"
