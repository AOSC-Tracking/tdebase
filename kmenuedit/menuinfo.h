/*
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

#ifndef __menuinfo_h__
#define __menuinfo_h__

#include <tqstring.h>

#include <tdeshortcut.h>
#include <kservice.h>

class MenuFile;
class MenuEntryInfo;

class MenuInfo
{
public:
    MenuInfo() {}
    virtual ~MenuInfo() {}
};

class MenuSeparatorInfo : public MenuInfo
{
public:
    MenuSeparatorInfo() {}
};

class MenuFolderInfo : public MenuInfo
{
public:
    MenuFolderInfo() : dirty(false), hidden(false) { subFolders.setAutoDelete(true); }

    // Add separator
    void add(MenuSeparatorInfo *, bool initial=false);

    // Add sub menu
    void add(MenuFolderInfo *, bool initial=false);

    // Remove sub menu (without deleting it)
    void take(MenuFolderInfo *);

    // Remove sub menu (without deleting it)
    // @return true if found
    bool takeRecursive(MenuFolderInfo *info);
    
    // Add entry
    void add(MenuEntryInfo *, bool initial = false);
    
    // Remove entry (without deleteing it)
    void take(MenuEntryInfo *);

    // Return a unique sub-menu caption inspired by @p caption
    TQString uniqueMenuCaption(const TQString &caption);

    // Return a unique item caption inspired by @p caption but different
    // from @p exclude
    TQString uniqueItemCaption(const TQString &caption, const TQString &exclude = TQString::null);

    // Update full id's for this item and all submenus
    void updateFullId(const TQString &parentId);

    // Return a list of existing submenu ids
    TQStringList existingMenuIds();

    void setCaption(const TQString &_caption)
    {
       if (_caption == caption) return;
       caption = _caption;
       setDirty();
    }

    void setIcon(const TQString &_icon)
    {
       if (_icon == icon) return;
       icon = _icon;
       setDirty();
    }

    void setGenericName(const TQString &_description)
    {
       if (_description == genericname) return;
       genericname = _description;
       setDirty();
    }

    void setComment(const TQString &_comment)
    {
       if (_comment == comment) return;
       comment = _comment;
       setDirty();
    }

    // Mark menu as dirty
    void setDirty();
    
    // Return whether this menu or any entry or submenu contained in it is dirty.
    bool hasDirt();

    // Return whether this menu should be explicitly added to its parent menu
    bool needInsertion();
    
    // Save menu and all its entries and submenus
    void save(MenuFile *);

    // Search service by shortcut
    KService::Ptr findServiceShortcut(const TDEShortcut&);

    // Set whether the entry is in active use (as opposed to in the clipboard/deleted)
    void setInUse(bool inUse);
    
public:
    TQString id; // Relative to parent
    TQString fullId; // Name in tree
    TQString caption; // Visible name
    TQString genericname; // Generic description
    TQString comment; // Comment
    TQString directoryFile; // File describing this folder.
    TQString icon; // Icon
    TQPtrList<MenuFolderInfo> subFolders; // Sub menus in this folder
    TQPtrList<MenuEntryInfo> entries; // Menu entries in this folder
    TQPtrList<MenuInfo> initialLayout; // Layout of menu entries according to sycoca
    bool dirty;
    bool hidden;
};

class MenuEntryInfo : public MenuInfo
{
public:
    MenuEntryInfo(const KService::Ptr &_service, TDEDesktopFile *_df = 0) 
     : service(_service), df(_df), 
       shortcutLoaded(false), shortcutDirty(false), dirty(_df != 0), hidden(false)
    {
       caption = service->name();
       description = service->genericName();
       icon = service->icon();
    }
    ~MenuEntryInfo();

    void setCaption(const TQString &_caption);
    void setDescription(const TQString &_description);
    void setIcon(const TQString &_icon);
    
    TQString menuId() const { return service->menuId(); }
    
    TQString file() const { return service->desktopEntryPath(); }
    
    TDEShortcut shortcut();
    void setShortcut(const TDEShortcut &_shortcut);
    bool isShortcutAvailable(const TDEShortcut &_shortcut);
    
    void setDirty();

    // Set whether the entry is in active use (as opposed to in the clipboard/deleted)
    void setInUse(bool inUse);

    // Return whether this menu should be explicitly added to its parent menu
    bool needInsertion();
    
    void save();

    TDEDesktopFile *desktopFile();

public:
    TQString caption;
    TQString description;
    TQString icon;
    KService::Ptr service;
    TDEDesktopFile *df;
    TDEShortcut shortCut;
    bool shortcutLoaded;
    bool shortcutDirty;
    bool dirty;
    bool hidden;
};

#endif
