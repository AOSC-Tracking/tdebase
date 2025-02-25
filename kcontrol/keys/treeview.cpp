/*
 *   Copyright (C) 2000 Matthias Elter <elter@kde.org>
 *   Copyright (C) 2001-2002 Raffaele Sandrini <sandrini@kde.org)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <unistd.h>

#include <tqdir.h>
#include <tqimage.h>
#include <tqstringlist.h>
#include <tqcursor.h>

#include <tdestandarddirs.h>
#include <tdelocale.h>
#include <kiconloader.h>
#include <kservicegroup.h>

#include "treeview.h"
#include "treeview.moc"
#include "khotkeys.h"

AppTreeItem::AppTreeItem(TQListViewItem *parent, const TQString& storageId)
    : TDEListViewItem(parent), m_init(false), m_storageId(storageId) {}

AppTreeItem::AppTreeItem(TQListViewItem *parent, TQListViewItem *after, const TQString& storageId)
    : TDEListViewItem(parent, after), m_init(false), m_storageId(storageId) {}

AppTreeItem::AppTreeItem(TQListView *parent, const TQString& storageId)
    : TDEListViewItem(parent), m_init(false), m_storageId(storageId) {}

AppTreeItem::AppTreeItem(TQListView *parent, TQListViewItem *after, const TQString& storageId)
    : TDEListViewItem(parent, after), m_init(false), m_storageId(storageId) {}

void AppTreeItem::setName(const TQString &name)
{
    m_name = name;
    setText(0, m_name);
}

void AppTreeItem::setAccel(const TQString &accel)
{
    m_accel = accel;
    int temp = accel.find(';');
    if (temp != -1)
    {
        setText(1, accel.left(temp));
        setText(2, accel.right(accel.length() - temp - 1));
    }
    else
    {
        setText(1, m_accel);
        setText(2, TQString::null);
    }
}

void AppTreeItem::setOpen(bool o)
{
    if (o && !m_directoryPath.isEmpty() && !m_init)
    {
       m_init = true;
       AppTreeView *tv = static_cast<AppTreeView *>(listView());
       tv->fillBranch(m_directoryPath, this);
    }
    TQListViewItem::setOpen(o);
}

static TQPixmap appIcon(const TQString &iconName)
{
    TQPixmap normal = SmallIcon( iconName );
    // make sure they are not larger than 20x20
    if (normal.width() > 20 || normal.height() > 20)
    {
       TQImage tmp = normal.convertToImage();
       tmp = tmp.smoothScale(20, 20);
       normal.convertFromImage(tmp);
    }
    return normal;
}


AppTreeView::AppTreeView( TQWidget *parent, const char *name )
    : TDEListView(parent, name)
{
    setFrameStyle(TQFrame::WinPanel | TQFrame::Sunken);
    setAllColumnsShowFocus(true);
    setRootIsDecorated(true);
    setSorting(-1);
    setAcceptDrops(false);
    setDragEnabled(false);
    setMinimumWidth(240);
    setResizeMode(AllColumns);

    addColumn(i18n("Command"));
    addColumn(i18n("Shortcut"));
    addColumn(i18n("Alternate"));

    connect(this, TQ_SIGNAL(clicked( TQListViewItem* )),
            TQ_SLOT(itemSelected( TQListViewItem* )));

    connect(this,TQ_SIGNAL(selectionChanged ( TQListViewItem * )),
            TQ_SLOT(itemSelected( TQListViewItem* )));
}

AppTreeView::~AppTreeView()
{
}

void AppTreeView::fill()
{
    TQApplication::setOverrideCursor(TQt::WaitCursor);
    clear();
    fillBranch(TQString::null, 0);
    TQApplication::restoreOverrideCursor();
}

void AppTreeView::fillBranch(const TQString& rPath, AppTreeItem *parent)
{
    // get rid of leading slash in the relative path
    TQString relPath = rPath;
    if(relPath[0] == '/')
        relPath = relPath.mid(1, relPath.length());

    // We ask KSycoca to give us all services (sorted).
    KServiceGroup::Ptr root = KServiceGroup::group(relPath);

    if (!root || !root->isValid())
        return;

    KServiceGroup::List list = root->entries(true);

    TQListViewItem *after = 0;

    for(KServiceGroup::List::ConstIterator it = list.begin();
        it != list.end(); ++it) 
    {
        KSycocaEntry * e = *it;

        if (e->isType(KST_KServiceGroup)) 
        {
            KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
            TQString groupCaption = g->caption();

            // Item names may contain ampersands. To avoid them being converted
            // to accelerators, replace them with two ampersands.
            groupCaption.replace("&", "&&");

            AppTreeItem *item;
            if (parent == 0)
                item = new AppTreeItem(this,  after, TQString::null);
            else
                item = new AppTreeItem(parent, after, TQString::null);

            item->setName(groupCaption);
            item->setPixmap(0, appIcon(g->icon()));
            item->setDirectoryPath(g->relPath());
            item->setExpandable(true);
            after = item;
        }
        else if (e->isType(KST_KService)) 
        {
            KService::Ptr s(static_cast<KService *>(e));
            TQString serviceCaption = s->name();

            // Item names may contain ampersands. To avoid them being converted
            // to accelerators, replace them with two ampersands.
            serviceCaption.replace("&", "&&");

            AppTreeItem* item;
            if (parent == 0)
                item = new AppTreeItem(this, after, s->storageId());
            else
                item = new AppTreeItem(parent, after, s->storageId());

            item->setName(serviceCaption);
            item->setAccel(KHotKeys::getMenuEntryShortcut(s->storageId()));
            item->setPixmap(0, appIcon(s->icon()));

            after = item;
        }
    }
}

void AppTreeView::itemSelected(TQListViewItem *item)
{
    AppTreeItem *_item = static_cast<AppTreeItem*>(item);

    if(!item) return;

    emit entrySelected(_item->storageId(), _item->accel(), _item->isDirectory());
}

TQStringList AppTreeView::fileList(const TQString& rPath)
{
    TQString relativePath = rPath;

    // truncate "/.directory"
    int pos = relativePath.findRev("/.directory");
    if (pos > 0) relativePath.truncate(pos);

    TQStringList filelist;

    // loop through all resource dirs and build a file list
    TQStringList resdirlist = TDEGlobal::dirs()->resourceDirs("apps");
    for (TQStringList::ConstIterator it = resdirlist.begin(); it != resdirlist.end(); ++it)
    {
        TQDir dir((*it) + "/" + relativePath);
        if(!dir.exists()) continue;

        dir.setFilter(TQDir::Files);
        dir.setNameFilter("*.desktop;*.kdelnk");

        // build a list of files
        TQStringList files = dir.entryList();
        for (TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
            // does not work?!
            //if (filelist.contains(*it)) continue;

            if (relativePath.isEmpty()) {
                filelist.remove(*it); // hack
                filelist.append(*it);
            }
            else {
                filelist.remove(relativePath + "/" + *it); //hack
                filelist.append(relativePath + "/" + *it);
            }
        }
    }
    return filelist;
}

TQStringList AppTreeView::dirList(const TQString& rPath)
{
    TQString relativePath = rPath;

    // truncate "/.directory"
    int pos = relativePath.findRev("/.directory");
    if (pos > 0) relativePath.truncate(pos);

    TQStringList dirlist;

    // loop through all resource dirs and build a subdir list
    TQStringList resdirlist = TDEGlobal::dirs()->resourceDirs("apps");
    for (TQStringList::ConstIterator it = resdirlist.begin(); it != resdirlist.end(); ++it)
    {
        TQDir dir((*it) + "/" + relativePath);
        if(!dir.exists()) continue;
        dir.setFilter(TQDir::Dirs);

        // build a list of subdirs
        TQStringList subdirs = dir.entryList();
        for (TQStringList::ConstIterator it = subdirs.begin(); it != subdirs.end(); ++it) {
            if ((*it) == "." || (*it) == "..") continue;
            // does not work?!
            // if (dirlist.contains(*it)) continue;

            if (relativePath.isEmpty()) {
                dirlist.remove(*it); //hack
                dirlist.append(*it);
            }
            else {
                dirlist.remove(relativePath + "/" + *it); //hack
                dirlist.append(relativePath + "/" + *it);
            }
        }
    }
    return dirlist;
}
