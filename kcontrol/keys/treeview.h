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

#ifndef __treeview_h__
#define __treeview_h__

#include <tqstring.h>
#include <tdelistview.h>

class TQPopupMenu;
class TDEActionCollection;
class TDEDesktopFile;

class AppTreeItem : public TDEListViewItem
{

public:
    AppTreeItem(TQListViewItem *parent, const TQString& storageId);
    AppTreeItem(TQListViewItem *parent, TQListViewItem *after, const TQString& storageId);
    AppTreeItem(TQListView *parent, const TQString& storageId);
    AppTreeItem(TQListView *parent, TQListViewItem* after, const TQString& storageId);

    TQString storageId() const { return m_storageId; }
    void setDirectoryPath(const TQString& path) { m_directoryPath = path; }

    TQString name() const { return m_name; }
    void setName(const TQString &name);

    TQString accel() const { return m_accel; }
    void setAccel(const TQString &accel);

    bool isDirectory() const { return !m_directoryPath.isEmpty(); }

    virtual void setOpen(bool o);

private:
    bool m_init : 1;
    TQString m_storageId;
    TQString m_name;
    TQString m_directoryPath;
    TQString m_accel;
};

class AppTreeView : public TDEListView
{
    friend class AppTreeItem;
    TQ_OBJECT
public:
    AppTreeView(TQWidget *parent=0, const char *name=0);
    ~AppTreeView();
    void fill();

signals:
    void entrySelected(const TQString&, const TQString &, bool);

protected slots:
    void itemSelected(TQListViewItem *);

protected:
    void fillBranch(const TQString& relPath, AppTreeItem* parent);

    TQStringList fileList(const TQString& relativePath);
    TQStringList dirList(const TQString& relativePath);
};


#endif
