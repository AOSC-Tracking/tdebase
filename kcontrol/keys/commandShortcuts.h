/*
 * commandShortcuts.h
 *
 * Copyright (c) 2003 Aaron J. Seigo
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifndef __COMMAND_SHORTCUTS_MODULE_H
#define __COMMAND_SHORTCUTS_MODULE_H

#include <tqtabwidget.h>
#include <tdeshortcut.h>
#include <tqptrlist.h>

class AppTreeView;
class AppTreeItem;
class TQButtonGroup;
class TQRadioButton;
class KKeyButton;
class TQListViewItem;

typedef TQPtrList<AppTreeItem> treeItemList;
typedef TQPtrListIterator<AppTreeItem> treeItemListIterator;

class CommandShortcutsModule : public TQWidget
{
    TQ_OBJECT
    public:
        CommandShortcutsModule( TQWidget *parent = 0, const char *name = 0 );
        ~CommandShortcutsModule();

        void load();
        void save();
        void defaults();
        TQString quickHelp() const;

    signals:
        void changed( bool );

    public slots:
        void showing(TQWidget*);

    protected slots:
        void commandSelected(const TQString&, const TQString &, bool);
        void shortcutChanged(const TDEShortcut& shortcut);
        void shortcutRadioToggled(bool remove);
        void commandDoubleClicked(TQListViewItem *item, const TQPoint &, int);
        void launchMenuEditor();

    protected:
        void initGUI();

    private:
        AppTreeView* m_tree;
        TQButtonGroup* m_shortcutBox;
        TQRadioButton* m_noneRadio;
        TQRadioButton* m_customRadio;
        KKeyButton* m_shortcutButton;
        treeItemList m_changedItems;
};

#endif // __COMMAND_SHORTCUTS_MODULE_H
