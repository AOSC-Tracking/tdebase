/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2003 Daniel Molkentin <molkentin@kde.org>

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

#include <tqheader.h>
#include <tqcursor.h>

#include <tdelocale.h>
#include <tdestandarddirs.h>
#include <kservicegroup.h>
#include <kiconloader.h>
#include <tdecmoduleloader.h>

#include <kdebug.h>

#include "moduleiconview.h"
#include "moduleiconview.moc"
#include "modules.h"
#include "global.h"


ModuleIconView::ModuleIconView(ConfigModuleList *list, TQWidget * parent, const char * name)
  : TDEListView(parent, name)
  , _path(KCGlobal::baseGroup())
  , _modules(list)
{
  setSorting(1, true);
  addColumn(TQString::null);

  // Needed to enforce a cut of the items label rather than
  // showing a horizontal scrollbar
  setResizeMode(LastColumn);

  header()->hide();

  // This is intentionally _not_ connected with executed(), since
  // honoring doubleclick doesn't make any sense here (changed by
  // large user demand)
  connect(this, TQ_SIGNAL(clicked(TQListViewItem*)),
          this, TQ_SLOT(slotItemSelected(TQListViewItem*)));
}

void ModuleIconView::makeSelected(ConfigModule *m)
{
  if (!m) return;

  for (TQListViewItem *i = firstChild(); i; i = i->nextSibling())
  {
     if(static_cast<ModuleIconItem*>(i)->module() == m)
     {
        setSelected(i, true);
        break;
     }
  }
}

void ModuleIconView::makeVisible(ConfigModule *m)
{
  if (!m) return;
  TQString tmp = _modules->findModule(m);
  if (tmp.isEmpty())
     return;

  _path = tmp;
  fill();
}

void ModuleIconView::fill()
{
  clear();

  TQPixmap icon;
  // add our "up" icon if we aren't top level
  if (_path != KCGlobal::baseGroup())
  {
     icon = loadIcon( "back" );
     // go-back node
     ModuleIconItem *i = new ModuleIconItem(this, i18n("Back"), icon);
     i->setOrderNo(0);
     int last_slash = _path.findRev('/', -2);
     if (last_slash == -1)
        i->setTag(TQString::null);
     else
        i->setTag(_path.left(last_slash+1));
  }

  int c = 0;
  TQStringList submenus = _modules->submenus(_path);
  for (TQStringList::Iterator it = submenus.begin(); it != submenus.end(); ++it )
  {
     TQString path = (*it);

     KServiceGroup::Ptr group = KServiceGroup::group(path);
     if (!group || !group->isValid())
        continue;

     icon = loadIcon( group->icon() );

     ModuleIconItem *i = new ModuleIconItem(this, group->caption(), icon);
     i->setTag(path);
     i->setOrderNo(++c);
  }

  c = 0;
  TQPtrList<ConfigModule> moduleList = _modules->modules(_path);
  for (ConfigModule *module=moduleList.first(); module != 0; module=moduleList.next())
  {
     icon = loadIcon( module->icon() );

     ModuleIconItem *i = new ModuleIconItem(this, module->moduleName(), icon, module);
     i->setOrderNo(++c);
  }
}

void ModuleIconView::slotItemSelected(TQListViewItem* item)
{
  TQApplication::restoreOverrideCursor();
  if (!item) return;

  if (static_cast<ModuleIconItem*>(item)->module())
  {
     emit moduleSelected(static_cast<ModuleIconItem*>(item)->module());
  }
  else
  {
     _path = static_cast<ModuleIconItem*>(item)->tag();
     fill();
     setCurrentItem(firstChild());
  }
}

void ModuleIconView::keyPressEvent(TQKeyEvent *e)
{
  if(   e->key() == Key_Return
     || e->key() == Key_Enter
     || e->key() == Key_Space)
  {
     if (currentItem())
        slotItemSelected(currentItem());
  }
  else
  {
     TDEListView::keyPressEvent(e);
  }
}

TQPixmap ModuleIconView::loadIcon( const TQString &name )
{
  TQPixmap icon = DesktopIcon( name, KCGlobal::iconSize() );

  if(icon.isNull())
     icon = DesktopIcon( "folder", KCGlobal::iconSize() );

  return icon;
}

ModuleIconItem::ModuleIconItem(TQListView *parent, const TQString& text, const TQPixmap& pm, ConfigModule *m)
  : TDEListViewItem(parent, text)
  , _tag(TQString::null)
  , _module(m)
{
  setPixmap(0, pm);

  if (!_module) return;
  setVisible(KCGlobal::showHiddenModules() || !_module->needsTest() || TDECModuleLoader::testModule(*_module));
}