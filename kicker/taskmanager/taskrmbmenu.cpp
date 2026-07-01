/*****************************************************************

Copyright (c) 2001 Matthias Elter <elter@kde.org>
Copyright (c) 2001 John Firebaugh <jfirebaugh@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <assert.h>
#include <unordered_map>

#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <netwm.h>

#include "taskmanager.h"

#if defined(HAVE_XCOMPOSITE) && \
    defined(HAVE_XRENDER) && \
    defined(HAVE_XFIXES)
#include <fixx11h.h>
#endif

#include "taskrmbmenu.h"
#include "taskrmbmenu.moc"

TaskRMBMenu::TaskRMBMenu(const Task::List& theTasks, bool show, TQPopupMenu* moveMenu, TQWidget *parent, const char *name)
	: TQPopupMenu( parent, name )
	, tasks( theTasks )
	, showAll( show )
	, taskMoveMenu( moveMenu )
{
    TDEGlobal::iconLoader()->addAppDir("twin");

    assert(tasks.count() > 0);
    if (tasks.count() == 1)
    {
        fillMenu(tasks.first());
    }
    else
    {
        fillMenu();
    }
}

TaskRMBMenu::TaskRMBMenu(Task::Ptr task, bool show, TQWidget *parent, const char *name)
	: TQPopupMenu( parent, name )
	, showAll( show )
	, taskMoveMenu( NULL )
{
	fillMenu(task);
}

void TaskRMBMenu::fillMenu(Task::Ptr t)
{
    int id;
    setCheckable(true);

    bool checkActions = KWin::allowedActionsSupported();

    insertItem(i18n("Ad&vanced"), makeAdvancedMenu(t));

    id = insertItem(i18n("T&ile"), makeTileMenu(t));
    setItemEnabled(id, !checkActions ||
            (t->info().actionSupported(NET::ActionMove) && t->info().actionSupported(NET::ActionResize)));

    if (TaskManager::the()->numberOfDesktops() > 1)
    {
        id = insertItem(i18n("To &Desktop"), makeDesktopsMenu(t));

        if (showAll)
        {
            id = insertItem(i18n("&To Current Desktop"),
                            t, TQ_SLOT(toCurrentDesktop()));
            setItemEnabled( id, !t->isOnCurrentDesktop() );
        }

        if (checkActions)
        {
            setItemEnabled(id, t->info().actionSupported(NET::ActionChangeDesktop));
        }
    }

    id = insertItem(SmallIconSet("move"), i18n("&Move"), t, TQ_SLOT(move()));
    setItemEnabled(id, !checkActions || t->info().actionSupported(NET::ActionMove));

    id = insertItem(i18n("Re&size"), t, TQ_SLOT(resize()));
    setItemEnabled(id, !checkActions || t->info().actionSupported(NET::ActionResize));

    id = insertItem(i18n("Mi&nimize"), t, TQ_SLOT(toggleIconified()));
    setItemChecked(id, t->isIconified());
    setItemEnabled(id, !checkActions || t->info().actionSupported(NET::ActionMinimize));

    id = insertItem(i18n("Ma&ximize"), t, TQ_SLOT(toggleMaximized()));
    setItemChecked(id, t->isMaximized());
    setItemEnabled(id, !checkActions || t->info().actionSupported(NET::ActionMax));

    id = insertItem(i18n("&Shade"), t, TQ_SLOT(toggleShaded()));
    setItemChecked(id, t->isShaded());
    setItemEnabled(id, !checkActions || t->info().actionSupported(NET::ActionShade));

    insertSeparator();

    if (taskMoveMenu) {
        taskMoveMenu->reparent(this, taskMoveMenu->getWFlags(), taskMoveMenu->geometry().topLeft(), false);
        insertItem(i18n("Move Task Button"), taskMoveMenu);

        insertSeparator();
    }

    id = insertItem(SmallIcon("window-close"), i18n("&Close"), t, TQ_SLOT(close()));
    setItemEnabled(id, !checkActions || t->info().actionSupported(NET::ActionClose));
}

void TaskRMBMenu::fillMenu()
{
    int id;
    setCheckable( true );

    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        Task::Ptr t = (*it);

        id = insertItem( TQIconSet( t->pixmap() ),
                         t->visibleNameWithState(),
		         new TaskRMBMenu(t, showAll, this) );
        setItemChecked( id, t->isActive() );
        connectItem( id, t, TQ_SLOT( activateRaiseOrIconify() ) );
    }

    insertSeparator();

    bool enable = false;

    if (TaskManager::the()->numberOfDesktops() > 1)
    {
        id = insertItem(i18n("All to &Desktop"), makeDesktopsMenu());

        if (showAll) {
            id = insertItem(i18n("All &to Current Desktop"), this, TQ_SLOT(slotAllToCurrentDesktop()));
            Task::List::iterator itEnd = tasks.end();
            for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
            {
                if (!(*it)->isOnCurrentDesktop())
                {
                    enable = true;
                    break;
                }
            }
            setItemEnabled(id, enable);
        }
    }

    enable = false;

    id = insertItem( i18n( "Mi&nimize All" ), this, TQ_SLOT( slotMinimizeAll() ) );
    itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        if( !(*it)->isIconified() ) {
            enable = true;
            break;
        }
    }
    setItemEnabled( id, enable );

    enable = false;

    id = insertItem( i18n( "Ma&ximize All" ), this, TQ_SLOT( slotMaximizeAll() ) );
    itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        if( !(*it)->isMaximized() ) {
            enable = true;
            break;
        }
    }
    setItemEnabled( id, enable );

    enable = false;

    id = insertItem( i18n( "&Restore All" ), this, TQ_SLOT( slotRestoreAll() ) );
    itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        if( (*it)->isIconified() || (*it)->isMaximized() ) {
            enable = true;
            break;
        }
    }
    setItemEnabled( id, enable );

    id = insertItem( SmallIconSet("go-up"  ), i18n( "R&aise All" ), this, TQ_SLOT( slotRaiseAll() ) );
    id = insertItem( SmallIconSet("go-down"), i18n( "&Lower All" ), this, TQ_SLOT( slotLowerAll() ) );

    insertSeparator();

    enable = false;

    if (taskMoveMenu) {
        taskMoveMenu->reparent(this, taskMoveMenu->getWFlags(), taskMoveMenu->geometry().topLeft(), false);
        insertItem(i18n("Move Task Button"), taskMoveMenu);

        insertSeparator();
    }

    insertItem( SmallIcon( "window-close" ), i18n( "&Close All" ), this, TQ_SLOT( slotCloseAll() ) );
}

TQPopupMenu* TaskRMBMenu::makeAdvancedMenu(Task::Ptr t)
{
    int id;
    TQPopupMenu* menu = new TQPopupMenu(this);

    menu->setCheckable(true);

    id = menu->insertItem(SmallIconSet("go-up"),
                          i18n("Keep &Above Others"),
                          t, TQ_SLOT(toggleAlwaysOnTop()));
    menu->setItemChecked(id, t->isAlwaysOnTop());

    id = menu->insertItem(SmallIconSet("go-down"),
                          i18n("Keep &Below Others"),
                          t, TQ_SLOT(toggleKeptBelowOthers()));
    menu->setItemChecked(id, t->isKeptBelowOthers());

    id = menu->insertItem(SmallIconSet("view-fullscreen"),
                          i18n("&Fullscreen"),
                          t, TQ_SLOT(toggleFullScreen()));
    menu->setItemChecked(id, t->isFullScreen());

    if (KWin::allowedActionsSupported())
    {
        menu->setItemEnabled(id, t->info().actionSupported(NET::ActionFullScreen));
    }

    return menu;
}

TQPopupMenu* TaskRMBMenu::makeDesktopsMenu(Task::Ptr t)
{
	TQPopupMenu* m = new TQPopupMenu( this );
	m->setCheckable( true );

	int id = m->insertItem( i18n("&All Desktops"), t, TQ_SLOT( toDesktop(int) ) );
	m->setItemParameter( id, 0 ); // 0 means all desktops
	m->setItemChecked( id, t->isOnAllDesktops() );

	m->insertSeparator();

	for (int i = 1; i <= TaskManager::the()->numberOfDesktops(); i++) {
		TQString name = TQString("&%1 %2").arg(i).arg(TaskManager::the()->desktopName(i).replace('&', "&&"));
		id = m->insertItem( name, t, TQ_SLOT( toDesktop(int) ) );
		m->setItemParameter( id, i );
		m->setItemChecked( id, !t->isOnAllDesktops() && t->desktop() == i );
	}

	return m;
}

TQPopupMenu* TaskRMBMenu::makeDesktopsMenu()
{
	TQPopupMenu* m = new TQPopupMenu( this );
	m->setCheckable( true );

	int id = m->insertItem( i18n("&All Desktops"), this, TQ_SLOT( slotAllToDesktop(int) ) );
	m->setItemParameter( id, 0 ); // 0 means all desktops

	m->insertSeparator();

	for (int i = 1; i <= TaskManager::the()->numberOfDesktops(); i++) {
		TQString name = TQString("&%1 %2").arg(i).arg(TaskManager::the()->desktopName(i).replace('&', "&&"));
		id = m->insertItem( name, this, TQ_SLOT( slotAllToDesktop(int) ) );
		m->setItemParameter( id, i );
	}

	return m;
}

TQPopupMenu* TaskRMBMenu::makeTileMenu(Task::Ptr t)
{
    TQPopupMenu *m = new TQPopupMenu( this );

    // Tile to side (the menu id matched the ActiveBorder index used for tiling)
    int id = m->insertItem( UserIconSet("tile_left"), i18n("&Left"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 6 );
    id = m->insertItem( UserIconSet("tile_right"), i18n("&Right"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 2 );
    id = m->insertItem( UserIconSet("tile_top"), i18n("&Top"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 0 );
    id = m->insertItem( UserIconSet("tile_bottom"), i18n("&Bottom"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 4 );

    // Tile to corner (the menu id matched the ActiveBorder index used for tiling)
    id = m->insertItem( UserIconSet("tile_topleft"), i18n("Top &Left"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 7 );
    id = m->insertItem( UserIconSet("tile_topright"), i18n("Top &Right"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 1 );
    id = m->insertItem( UserIconSet("tile_bottomleft"), i18n("Bottom L&eft"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 5 );
    id = m->insertItem( UserIconSet("tile_bottomright"), i18n("&Bottom R&ight"), t, TQ_SLOT( tileTo(int) ) );
    m->setItemParameter( id, 3 );

    return m;
}

void TaskRMBMenu::slotMinimizeAll()
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        (*it)->setIconified(true);
    }
}

void TaskRMBMenu::slotMaximizeAll()
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        (*it)->setMaximized(true);
    }
}

void TaskRMBMenu::slotRestoreAll()
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        (*it)->restore();
    }
}

/// Returns list of tasks reordered to the order they appear in the window
/// stack; the topmost window will come first
static TQValueVector<Task *> reorderedByStack(const Task::List &tasks)
{
    // Reorganize the list into a hash for faster lookups and keeping track if
    // any windows are missing in the window stack
    std::unordered_map<Window, Task*> winTaskMap( tasks.size()*2 );
    for (const auto& task: tasks)
    {
        winTaskMap.insert( { task->window(), const_cast<Task *>(task.data()) } );
    }

    TQValueVector<Task *> rv;
    rv.reserve(tasks.size());

    // Get the window stack
    NETRootInfo ri( tqt_xdisplay(),  NET::ClientListStacking );
    const Window *windowStack = ri.clientListStacking();
    int windowStackSz = ri.clientListStackingCount();

    // Copy the tasks to the returned vector in the order they are in the window stack
    for (int i = windowStackSz - 1; i >= 0; i--)
    { // Note: reverse the order so the topmost window comes first
        auto taskIt = winTaskMap.find(windowStack[i]);
        if (taskIt != winTaskMap.end())
        {
            rv.append(taskIt->second);
            winTaskMap.erase(taskIt);
        }
    }

    // winTaskMap should be empty at that point, but in a very unlikely case
    // some tasks were missing in the window stack copy them manually
    for (const auto& [_ , task]: winTaskMap)
    {
        rv.append(task);
    }

    return rv;
}

void TaskRMBMenu::slotRaiseAll()
{
    auto stackedTasks = reorderedByStack(tasks);
    // Iterate in reverse to keep the correct order
    for (ssize_t i = stackedTasks.size()-1; i >= 0; i--)
    {
        stackedTasks[i]->raise();
    }
}

void TaskRMBMenu::slotLowerAll()
{
    for (auto& task: reorderedByStack(tasks) )
    {
        task->lower();
    }
}

void TaskRMBMenu::slotShadeAll()
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        (*it)->setShaded( !(*it)->isShaded() );
    }
}

void TaskRMBMenu::slotCloseAll()
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        (*it)->close();
    }
}

void TaskRMBMenu::slotAllToDesktop( int desktop )
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        (*it)->toDesktop( desktop );
    }
}

void TaskRMBMenu::slotAllToCurrentDesktop()
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        (*it)->toCurrentDesktop();
    }
}
