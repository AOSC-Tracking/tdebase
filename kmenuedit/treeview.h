/*
 *   Copyright (C) 2000 Matthias Elter <elter@kde.org>
 *   Copyright (C) 2001-2002 Raffaele Sandrini <sandrini@kde.org>
 *   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
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

#ifndef __treeview_h__
#define __treeview_h__

#include <tqstring.h>
#include <tdelistview.h>
#include <kservice.h>
#include <kservicegroup.h>

class TQPopupMenu;
class TDEActionCollection;
class TDEDesktopFile;
class MenuFile;
class MenuFolderInfo;
class MenuEntryInfo;
class MenuSeparatorInfo;
class TDEShortcut;

class TreeItem : public TQListViewItem
{
public:
  TreeItem(TQListViewItem *parent, TQListViewItem *after, const TQString &menuIdn, bool __init = false);
    TreeItem(TQListView *parent, TQListViewItem* after, const TQString &menuId, bool __init = false);

    TQString menuId() const { return _menuId; }

    TQString directory() const { return _directoryPath; }
    void setDirectoryPath(const TQString& path) { _directoryPath = path; }

    MenuFolderInfo *folderInfo() { return m_folderInfo; }
    void setMenuFolderInfo(MenuFolderInfo *folderInfo) { m_folderInfo = folderInfo; }

    MenuEntryInfo *entryInfo() { return m_entryInfo; }
    void setMenuEntryInfo(MenuEntryInfo *entryInfo) { m_entryInfo = entryInfo; }

    TQString name() const { return _name; }
    void setName(const TQString &name);

    bool isDirectory() const { return m_folderInfo; }
    bool isEntry() const { return m_entryInfo; }

    bool isHidden() const { return _hidden; }
    void setHidden(bool b);

    bool isLayoutDirty() { return _layoutDirty; }
    void setLayoutDirty() { _layoutDirty = true; }
    TQStringList layout();

    virtual void setOpen(bool o);
    void load();

    virtual void paintCell(TQPainter * p, const TQColorGroup & cg, int column, int width, int align);
    virtual void setup();

private:
    void update();

    bool _hidden : 1;
    bool _init : 1;
    bool _layoutDirty : 1;
    TQString _menuId;
    TQString _name;
    TQString _directoryPath;
    MenuFolderInfo *m_folderInfo;
    MenuEntryInfo *m_entryInfo;
};

class TreeView : public TDEListView
{
    friend class TreeItem;
    TQ_OBJECT
public:
    TreeView(bool controlCenter, TDEActionCollection *ac, TQWidget *parent=0, const char *name=0);
    ~TreeView();

    void readMenuFolderInfo(MenuFolderInfo *folderInfo=0, KServiceGroup::Ptr folder=0, const TQString &prefix=TQString::null);
    void setViewMode(bool showHidden);
    bool save();

    bool dirty();

    void selectMenu(const TQString &menu);
    void selectMenuEntry(const TQString &menuEntry);
    
public slots:
    void currentChanged(MenuFolderInfo *folderInfo);
    void currentChanged(MenuEntryInfo *entryInfo);
    void findServiceShortcut(const TDEShortcut&, KService::Ptr &);

signals:
    void entrySelected(MenuFolderInfo *folderInfo);
    void entrySelected(MenuEntryInfo *entryInfo);
    void disableAction();
protected slots:
    void itemSelected(TQListViewItem *);
    void slotDropped(TQDropEvent *, TQListViewItem *, TQListViewItem *);
    void slotRMBPressed(TQListViewItem*, const TQPoint&);

    void newsubmenu();
    void newitem();
    void newsep();

    void cut();
    void copy();
    void paste();
    void del();

protected:
    TreeItem *createTreeItem(TreeItem *parent, TQListViewItem *after, MenuFolderInfo *folderInfo, bool _init = false);
    TreeItem *createTreeItem(TreeItem *parent, TQListViewItem *after, MenuEntryInfo *entryInfo, bool _init = false);
    TreeItem *createTreeItem(TreeItem *parent, TQListViewItem *after, MenuSeparatorInfo *sepInfo, bool _init = false);

    void del(TreeItem *, bool deleteInfo);
    void fill();
    void fillBranch(MenuFolderInfo *folderInfo, TreeItem *parent);
    TQString findName(TDEDesktopFile *df, bool deleted);

    void closeAllItems(TQListViewItem *item);

    // moving = src will be removed later
    void copy( bool moving );

    void cleanupClipboard();
    
    bool isLayoutDirty();
    void setLayoutDirty(TreeItem *);
    void saveLayout();

    TQStringList fileList(const TQString& relativePath);
    TQStringList dirList(const TQString& relativePath);

    virtual bool acceptDrag(TQDropEvent* event) const;
    virtual TQDragObject *dragObject();
    virtual void startDrag();

private:
    TDEActionCollection *m_ac;
    TQPopupMenu        *m_rmb;
    int                m_clipboard;
    MenuFolderInfo    *m_clipboardFolderInfo;
    MenuEntryInfo     *m_clipboardEntryInfo;
    int                m_drag;
    MenuFolderInfo    *m_dragInfo;
    TreeItem          *m_dragItem;
    TQString            m_dragPath;
    bool               m_showHidden;
    bool               m_controlCenter;
    MenuFile          *m_menuFile;
    MenuFolderInfo    *m_rootFolder;
    MenuSeparatorInfo *m_separator;
    TQStringList        m_newMenuIds;
    TQStringList        m_newDirectoryList;
    bool               m_detailedMenuEntries;
    bool               m_detailedEntriesNamesFirst;
    bool               m_layoutDirty;
};


#endif
