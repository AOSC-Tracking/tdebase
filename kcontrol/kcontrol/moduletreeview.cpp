/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

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
#include <tqimage.h>
#include <tqpainter.h>
#include <tqbitmap.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kservicegroup.h>
#include <kdebug.h>
#include <tqwhatsthis.h>
#include <tqbitmap.h>

#include "moduletreeview.h"
#include "moduletreeview.moc"
#include "modules.h"
#include "global.h"

static TQPixmap appIcon(const TQString &iconName)
{
     TQString path;
     TQPixmap normal = KGlobal::iconLoader()->loadIcon(iconName, KIcon::Small, 0, KIcon::DefaultState, &path, true);
     // make sure they are not larger than KIcon::SizeSmall
     if (normal.width() > KIcon::SizeSmall || normal.height() > KIcon::SizeSmall)
     {
         TQImage tmp = normal.convertToImage();
         tmp = tmp.smoothScale(KIcon::SizeSmall, KIcon::SizeSmall);
         normal.convertFromImage(tmp);
     }
     return normal;
}

class ModuleTreeWhatsThis : public TQWhatsThis
{
public:
    ModuleTreeWhatsThis( ModuleTreeView* tree)
        : TQWhatsThis( tree ), treeView( tree ) {}
    ~ModuleTreeWhatsThis(){};


    TQString text( const TQPoint & p) {
        ModuleTreeItem* i = (ModuleTreeItem*)  treeView->itemAt( p );
        if ( i && i->module() )  {
            return i->module()->comment();
        } else if ( i ) {
            return i18n("The %1 configuration group. Click to open it.").arg( i->text(0) );
        }
        return i18n("This treeview displays all available control modules. Click on one of the modules to receive more detailed information.");
    }

private:
    ModuleTreeView* treeView;
};

ModuleTreeView::ModuleTreeView(ConfigModuleList *list, TQWidget * parent, const char * name)
  : KListView(parent, name)
  , _modules(list)
{
  addColumn(TQString::null);
  setColumnWidthMode (0, TQListView::Maximum);
  setAllColumnsShowFocus(true);
  setResizeMode(TQListView::AllColumns);
  setRootIsDecorated(true);
  setHScrollBarMode(AlwaysOff);
  header()->hide();

  new ModuleTreeWhatsThis( this );

  connect(this, TQT_SIGNAL(clicked(TQListViewItem*)),
                  this, TQT_SLOT(slotItemSelected(TQListViewItem*)));
}

void ModuleTreeView::fill()
{
  clear();

  TQStringList subMenus = _modules->submenus(KCGlobal::baseGroup());
  for(TQStringList::ConstIterator it = subMenus.begin();
      it != subMenus.end(); ++it)
  {
     TQString path = *it;
     ModuleTreeItem*  menu = new ModuleTreeItem(this);
     menu->setGroup(path);
     fill(menu, path);
  }

  ConfigModule *module;
  TQPtrList<ConfigModule> moduleList = _modules->modules(KCGlobal::baseGroup());
  for (module=moduleList.first(); module != 0; module=moduleList.next())
  {
     new ModuleTreeItem(this, module);
  }
}

void ModuleTreeView::fill(ModuleTreeItem *parent, const TQString &parentPath)
{
  TQStringList subMenus = _modules->submenus(parentPath);
  for(TQStringList::ConstIterator it = subMenus.begin();
      it != subMenus.end(); ++it)
  {
     TQString path = *it;
     ModuleTreeItem*  menu = new ModuleTreeItem(parent);
     menu->setGroup(path);
     fill(menu, path);
  }

  ConfigModule *module;
  TQPtrList<ConfigModule> moduleList = _modules->modules(parentPath);
  for (module=moduleList.first(); module != 0; module=moduleList.next())
  {
     new ModuleTreeItem(parent, module);
  }
}



TQSize ModuleTreeView::sizeHint() const
{
    return TQListView::sizeHint().boundedTo( 
	TQSize( fontMetrics().maxWidth()*35, TQWIDGETSIZE_MAX) );
}

void ModuleTreeView::makeSelected(ConfigModule *module)
{
  ModuleTreeItem *item = static_cast<ModuleTreeItem*>(firstChild());

  updateItem(item, module);
}

void ModuleTreeView::updateItem(ModuleTreeItem *item, ConfigModule *module)
{
  while (item)
    {
          if (item->childCount() != 0)
                updateItem(static_cast<ModuleTreeItem*>(item->firstChild()), module);
          if (item->module() == module)
                {
                  setSelected(item, true);
                  break;
                }
          item = static_cast<ModuleTreeItem*>(item->nextSibling());
    }
}

/*
void ModuleTreeView::expandItem(TQListViewItem *item, TQPtrList<TQListViewItem> *parentList)
{
  while (item)
    {
      setOpen(item, parentList-.contains(item));

          if (item->childCount() != 0)
                expandItem(item->firstChild(), parentList);
      item = item->nextSibling();
    }
}
*/
void ModuleTreeView::makeVisible(ConfigModule *module)
{
  TQString path = _modules->findModule(module);
  if (path.startsWith(KCGlobal::baseGroup()))
     path = path.mid(KCGlobal::baseGroup().length());

  TQStringList groups = TQStringList::split('/', path);

  ModuleTreeItem *item = 0;
  TQStringList::ConstIterator it;
  for (it=groups.begin(); it != groups.end(); ++it)
  {
     if (item)
        item = static_cast<ModuleTreeItem*>(item->firstChild());
     else
        item = static_cast<ModuleTreeItem*>(firstChild());

     while (item)
     {
        if (item->tag() == *it)
        {
           setOpen(item, true);
           break;
        }
        item = static_cast<ModuleTreeItem*>(item->nextSibling());
     }
     if (!item)
        break; // Not found (?)
  }

  // make the item visible
  if (item)
    ensureItemVisible(item);
}

void ModuleTreeView::slotItemSelected(TQListViewItem* item)
{
  if (!item) return;

  if (static_cast<ModuleTreeItem*>(item)->module())
    {
      emit moduleSelected(static_cast<ModuleTreeItem*>(item)->module());
      return;
    }
  else
    {
      emit categorySelected(item);
    }

  setOpen(item, !item->isOpen());

  /*
  else
    {
      TQPtrList<TQListViewItem> parents;

      TQListViewItem* i = item;
      while(i)
        {
          parents.append(i);
          i = i->parent();
        }

      //int oy1 = item->itemPos();
      //int oy2 = mapFromGlobal(TQCursor::pos()).y();
      //int offset = oy2 - oy1;

      expandItem(firstChild(), &parents);

      //int x =mapFromGlobal(TQCursor::pos()).x();
      //int y = item->itemPos() + offset;
      //TQCursor::setPos(mapToGlobal(TQPoint(x, y)));
    }
  */
}

void ModuleTreeView::keyPressEvent(TQKeyEvent *e)
{
  if (!currentItem()) return;

  if(e->key() == Key_Return
     || e->key() == Key_Enter
        || e->key() == Key_Space)
    {
      //TQCursor::setPos(mapToGlobal(TQPoint(10, currentItem()->itemPos()+5)));
      slotItemSelected(currentItem());
    }
  else
    KListView::keyPressEvent(e);
}


ModuleTreeItem::ModuleTreeItem(TQListViewItem *parent, ConfigModule *module)
  : TQListViewItem(parent)
  , _module(module)
  , _tag(TQString::null)
  , _maxChildIconWidth(0)
{
  if (_module)
        {
          setText(0, " " + module->moduleName());
          _icon = module->icon();
          setPixmap(0, appIcon(_icon));
        }
}

ModuleTreeItem::ModuleTreeItem(TQListView *parent, ConfigModule *module)
  : TQListViewItem(parent)
  , _module(module)
  , _tag(TQString::null)
  , _maxChildIconWidth(0)
{
  if (_module)
        {
          setText(0, " " + module->moduleName());
          _icon = module->icon();
          setPixmap(0, appIcon(_icon));
        }
}

ModuleTreeItem::ModuleTreeItem(TQListViewItem *parent, const TQString& text)
  : TQListViewItem(parent, " " + text)
  , _module(0)
  , _tag(TQString::null)
  , _maxChildIconWidth(0)
  {}

ModuleTreeItem::ModuleTreeItem(TQListView *parent, const TQString& text)
  : TQListViewItem(parent, " " + text)
  , _module(0)
  , _tag(TQString::null)
  , _maxChildIconWidth(0)
  {}

void ModuleTreeItem::setPixmap(int column, const TQPixmap& pm)
{
  if (!pm.isNull())
  {
    ModuleTreeItem* p = dynamic_cast<ModuleTreeItem*>(parent());
    if (p)
      p->regChildIconWidth(pm.width());
  }

  TQListViewItem::setPixmap(column, pm);
}

void ModuleTreeItem::regChildIconWidth(int width)
{
  if (width > _maxChildIconWidth)
    _maxChildIconWidth = width;
}

void ModuleTreeItem::paintCell( TQPainter * p, const TQColorGroup & cg, int column, int width, int align )
{
  if (!pixmap(0))
  {
    int offset = 0;
    ModuleTreeItem* parentItem = dynamic_cast<ModuleTreeItem*>(parent());
    if (parentItem)
    {
      offset = parentItem->maxChildIconWidth();
    }

    if (offset > 0)
    {
      TQPixmap pixmap(offset, offset);
      pixmap.fill(Qt::color0);
      pixmap.setMask(pixmap.createHeuristicMask());
      TQBitmap mask( pixmap.size(), true );
      pixmap.setMask( mask );
      TQListViewItem::setPixmap(0, pixmap);
    }
  }

  TQListViewItem::paintCell( p, cg, column, width, align );
}


void ModuleTreeItem::setGroup(const TQString &path)
{
  KServiceGroup::Ptr group = KServiceGroup::group(path);
  TQString defName = path.left(path.length()-1);
  int pos = defName.findRev('/');
  if (pos >= 0)
     defName = defName.mid(pos+1);
  if (group && group->isValid())
  {
     _icon = group->icon();
     setPixmap(0, appIcon(_icon));
     setText(0, " " + group->caption());
     setTag(defName);
     setCaption(group->caption());
  }
  else
  {
     // Should not happen: Installation problem
     // Let's try to fail softly.
     setText(0, " " + defName);
     setTag(defName);
  }
}
