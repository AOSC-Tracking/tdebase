/*****************************************************************

Copyright (c) 2001 Matthias Elter <elter@kde.org>
Copyright (c) 2004 Sebastian Wolff
Copyright (c) 2005 Aaron Seigo <aseigo@kde.org>

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
*****************************************************************
*/

#include <math.h>

#include <tqapplication.h>
#include <tqbitmap.h>
#include <tqdesktopwidget.h>
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqstringlist.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdeglobalaccel.h>
#include <kimageeffect.h>
#include <tdelocale.h>
#include <tdestandarddirs.h>

#include "taskbarsettings.h"
#include "taskcontainer.h"
#include "taskmanager.h"

#include "taskbar.h"
#include "taskbar.moc"

#define READ_MERGED_TASKBAR_SETTING(x) ((m_settingsObject->useGlobalSettings())?m_globalSettingsObject->x():m_settingsObject->x())
#define READ_MERGED_TASKBAR_WACTION(x) ((m_settingsObject->useGlobalSettings())?m_globalSettingsObject->wAction(x):m_settingsObject->wAction(x))

TaskBar::TaskBar( TaskBarSettings* settingsObject, TaskBarSettings* globalSettingsObject, TQWidget *parent, const char *name )
    : Panner( parent, name ),
      m_showAllWindows(false),
      m_cycleWindowsAction(0),
      m_scrollTaskbarAction(0),
      m_currentScreen(-1),
      m_showOnlyCurrentScreen(false),
      m_sortByDesktop(false),
      m_displayIconsNText(settingsObject->DisplayIconsAndText),
      m_showOnlyIconified(false),
      m_showTaskStates(0),
      m_textShadowEngine(0),
      m_ignoreUpdates(false),
      m_relayoutTimer(0, "TaskBar::m_relayoutTimer")
{
    arrowType = LeftArrow;
    blocklayout = true;

    m_settingsObject = settingsObject;
    if (m_settingsObject)
    {
        m_settingsObject->readConfig();
    }
    m_globalSettingsObject = globalSettingsObject;
    if (m_globalSettingsObject)
    {
        m_globalSettingsObject->readConfig();
    }

    // init
    setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Expanding ) );
    m_sortByAppPrev = READ_MERGED_TASKBAR_SETTING(sortByApp);

    // setup animation frames
    for (int i = 1; i < 11; i++)
    {
        frames.append(new TQPixmap(locate("data", "kicker/pics/disk" + TQString::number(i) + ".png")));
    }

    // configure
    configure();

    connect(&m_relayoutTimer, TQ_SIGNAL(timeout()),
            this, TQ_SLOT(reLayout()));

    connect(this, TQ_SIGNAL(contentsMoving(int, int)), TQ_SLOT(setBackground()));

    // connect manager
    connect(TaskManager::the(), TQ_SIGNAL(taskAdded(Task::Ptr)),
            this, TQ_SLOT(add(Task::Ptr)));
    connect(TaskManager::the(), TQ_SIGNAL(taskRemoved(Task::Ptr)),
            this, TQ_SLOT(remove(Task::Ptr)));
    connect(TaskManager::the(), TQ_SIGNAL(startupAdded(Startup::Ptr)),
            this, TQ_SLOT(add(Startup::Ptr)));
    connect(TaskManager::the(), TQ_SIGNAL(startupRemoved(Startup::Ptr)),
            this, TQ_SLOT(remove(Startup::Ptr)));
    connect(TaskManager::the(), TQ_SIGNAL(desktopChanged(int)),
            this, TQ_SLOT(desktopChanged(int)));
    connect(TaskManager::the(), TQ_SIGNAL(windowChanged(Task::Ptr)),
            this, TQ_SLOT(windowChanged(Task::Ptr)));

    isGrouping = shouldGroup();

    // register existant tasks
    Task::Dict tasks = TaskManager::the()->tasks();
    Task::Dict::iterator taskEnd = tasks.end();
    for (Task::Dict::iterator it = tasks.begin(); it != taskEnd; ++it)
    {
        add(it.data());
    }

    // register existant startups
    Startup::List startups = TaskManager::the()->startups();
    Startup::List::iterator startupEnd = startups.end();
    for (Startup::List::iterator sIt = startups.begin(); sIt != startupEnd; ++sIt)
    {
        add((*sIt));
    }

    blocklayout = false;

    connect(tdeApp, TQ_SIGNAL(settingsChanged(int)), TQ_SLOT(slotSettingsChanged(int)));
    keys = new TDEGlobalAccel( this );
#include "taskbarbindings.cpp"
    keys->readSettings();
    keys->updateConnections();

    reLayout();
}

TaskBar::~TaskBar()
{
    for (TaskContainer::Iterator it = m_hiddenContainers.begin();
         it != m_hiddenContainers.end();
         ++it)
    {
        (*it)->deleteLater();
    }

    for (TaskContainer::List::const_iterator it = containers.constBegin();
         it != containers.constEnd();
         ++it)
    {
        (*it)->deleteLater();
    }

    for (PixmapList::const_iterator it = frames.constBegin();
         it != frames.constEnd();
         ++it)
    {
        delete *it;
    }

    delete m_textShadowEngine;
}

KTextShadowEngine *TaskBar::textShadowEngine()
{
    if (!m_textShadowEngine)
        m_textShadowEngine = new KTextShadowEngine();

    return m_textShadowEngine;
}

int TaskBar::buttonHeight() const
{
    TQFontMetrics fm(TDEGlobalSettings::taskbarFont());
    int bh = TQMAX(fm.height(), READ_MERGED_TASKBAR_SETTING(minimumButtonHeight));

    if(showIcons())
    {
        bh = TQMAX(bh, (int) READ_MERGED_TASKBAR_SETTING(iconSize));
    }

    return bh + 2;
}

int TaskBar::buttonWidth() const
{
    return TQMAX(BUTTON_MIN_WIDTH, READ_MERGED_TASKBAR_SETTING(iconSize)) + 2;
}


TQSize TaskBar::sizeHint() const
{
    return TQSize(buttonWidth(), buttonHeight());
}

TQSize TaskBar::sizeHint( KPanelExtension::Position p, TQSize maxSize) const
{
    // get our minimum height based on the minimum button height, the icon size or the
    // height of the font in use, whichever is largest

    if ( p == KPanelExtension::Left || p == KPanelExtension::Right )
    {
        // Vertical layout
        // Minimum space allows for one icon, the window list button and the up/down scrollers
        int minHeight = buttonHeight()*3;
        if (minHeight > maxSize.height())
            return maxSize;
        return TQSize(maxSize.width(), minHeight);
    }
    else
    {
        // Horizontal layout
        // Minimum space allows for one column of icons, the window list button and the left/right scrollers
        int min_width=buttonWidth()*3;
        if (min_width > maxSize.width())
            return maxSize;
        return TQSize(min_width, maxSize.height());
    }
}

bool TaskBar::showIcons() const
{
  return (m_displayIconsNText==m_settingsObject->DisplayIconsAndText ||
          m_displayIconsNText==m_settingsObject->DisplayIconsOnly);
}
bool TaskBar::showText() const
{
  return (m_displayIconsNText==m_settingsObject->DisplayIconsAndText ||
          m_displayIconsNText==m_settingsObject->DisplayTextOnly);
}

void TaskBar::configure()
{
    bool wasShowWindows = m_showAllWindows;
    bool wasSortByDesktop = m_sortByDesktop;
    int  wasCycleWindowsAction = m_cycleWindowsAction;
    int  wasScrollTaskbarAction = m_scrollTaskbarAction;
    bool wasDisplayIconsNText = m_displayIconsNText;
    bool wasShowOnlyIconified = m_showOnlyIconified;
    int  wasShowTaskStates = m_showTaskStates;
    int  wasIconSize = m_iconSize;

    m_showAllWindows = READ_MERGED_TASKBAR_SETTING(showAllWindows);
    m_sortByDesktop = m_showAllWindows && READ_MERGED_TASKBAR_SETTING(sortByDesktop);
    m_displayIconsNText = READ_MERGED_TASKBAR_SETTING(displayIconsNText);
    m_showOnlyIconified = READ_MERGED_TASKBAR_SETTING(showOnlyIconified);
    m_cycleWindowsAction = READ_MERGED_TASKBAR_WACTION(TaskBarSettings::CycleWindows);
    m_scrollTaskbarAction = READ_MERGED_TASKBAR_WACTION(TaskBarSettings::ScrollTaskbar);
    m_showTaskStates = READ_MERGED_TASKBAR_SETTING(showTaskStates);
    m_iconSize = READ_MERGED_TASKBAR_SETTING(iconSize);
    m_wheelScrollStep = READ_MERGED_TASKBAR_SETTING(wheelScrollStep);

    m_currentScreen = -1;    // Show all screens or re-get our screen
    m_showOnlyCurrentScreen = (READ_MERGED_TASKBAR_SETTING(showCurrentScreenOnly) &&
                              TQApplication::desktop()->isVirtualDesktop() &&
                              TQApplication::desktop()->numScreens() > 1);

    // we need to watch geometry issues if we aren't showing windows when we
    // are paying attention to the current Xinerama screen
    // disconnect first in case we've been here before
    // to avoid multiple connections
    disconnect(TaskManager::the(), TQ_SIGNAL(windowChangedGeometry(Task::Ptr)),
                this, TQ_SLOT(windowChangedGeometry(Task::Ptr)));
    if (m_showOnlyCurrentScreen)
    {
        connect(TaskManager::the(), TQ_SIGNAL(windowChangedGeometry(Task::Ptr)),
                 this, TQ_SLOT(windowChangedGeometry(Task::Ptr)));
    }
    TaskManager::the()->trackGeometry(m_showOnlyCurrentScreen);

    if (wasShowWindows != m_showAllWindows ||
        wasSortByDesktop != m_sortByDesktop ||
        wasDisplayIconsNText != m_displayIconsNText ||
        wasCycleWindowsAction != m_cycleWindowsAction ||
        wasScrollTaskbarAction != m_scrollTaskbarAction ||
        wasShowOnlyIconified != m_showOnlyIconified ||
        wasShowTaskStates != m_showTaskStates ||
        wasIconSize != m_iconSize)
    {
        // relevant settings changed, update our task containers
        for (TaskContainer::Iterator it = containers.begin();
             it != containers.end();
             ++it)
        {
            (*it)->settingsChanged();
        }
    }

    if (m_sortByAppPrev != READ_MERGED_TASKBAR_SETTING(sortByApp)) {
        m_sortByAppPrev = READ_MERGED_TASKBAR_SETTING(sortByApp);
        reSort();
    }

    TaskManager::the()->setXCompositeEnabled(READ_MERGED_TASKBAR_SETTING(showThumbnails));

    reLayoutEventually();
}

void TaskBar::setOrientation( Orientation o )
{
    Panner::setOrientation( o );
    reLayoutEventually();
}

void TaskBar::moveEvent( TQMoveEvent* e )
{
    Panner::moveEvent(e);
    setViewportBackground();
}

void TaskBar::resizeEvent( TQResizeEvent* e )
{
    if (m_showOnlyCurrentScreen)
    {
        TQPoint topLeft = mapToGlobal(this->geometry().topLeft());
        if (m_currentScreen != TQApplication::desktop()->screenNumber(topLeft))
        {
            // we have been moved to another screen!
            m_currentScreen = -1;
            reGroup();
        }
    }

    Panner::resizeEvent(e);
    reLayoutEventually();
    setViewportBackground();
}

void TaskBar::add(Task::Ptr task)
{
    if (!task ||
        (m_showOnlyCurrentScreen &&
         !TaskManager::isOnScreen(showScreen(), task->window())))
    {
        return;
    }

    // try to group
    if (isGrouping)
    {
        for (TaskContainer::Iterator it = containers.begin();
             it != containers.end();
             ++it)
        {
            TaskContainer* c = *it;

            if (idMatch(task->classClass(), c->id()))
            {
                c->add(task);
                reLayoutEventually();
                return;
            }
        }
    }

    // create new container
    TaskContainer *container = new TaskContainer(task, this, m_settingsObject, m_globalSettingsObject, viewport());
    m_hiddenContainers.append(container);

    // even though there is a signal to listen to, we have to add this
    // immediately to ensure grouping doesn't break (primarily on startup)
    // we still add the container to m_hiddenContainers in case the event
    // loop gets re-entered here and something bizarre happens. call it
    // insurance =)
    showTaskContainer(container);
}

void TaskBar::add(Startup::Ptr startup)
{
    if (!startup)
    {
        return;
    }

    for (TaskContainer::Iterator it = containers.begin();
         it != containers.end();
         ++it)
    {
        if ((*it)->contains(startup))
        {
            return;
        }
    }

    // create new container
    TaskContainer *container = new TaskContainer(startup, frames, this, m_settingsObject, m_globalSettingsObject, viewport());
    m_hiddenContainers.append(container);
    connect(container, TQ_SIGNAL(showMe(TaskContainer*)), this, TQ_SLOT(showTaskContainer(TaskContainer*)));
}

void TaskBar::reSort()
{
    TaskContainer::List originalContainers = containers;
    TaskContainer::Iterator it = originalContainers.begin();
    for (; it != originalContainers.end(); ++it)
    {
        removeChild(*it);
    }
    containers.clear();
    it = originalContainers.begin();
    for (; it != originalContainers.end(); ++it)
    {
        showTaskContainer(*it);
    }
    reLayoutEventually();
    emit containerCountChanged();
}

void TaskBar::showTaskContainer(TaskContainer* container)
{
    TaskContainer::List::iterator it = m_hiddenContainers.find(container);
    if (it != m_hiddenContainers.end())
    {
        m_hiddenContainers.erase(it);
    }

    if (container->isEmpty())
    {
        return;
    }

    // try to place the container after one of the same app
    if (READ_MERGED_TASKBAR_SETTING(sortByApp))
    {
        TaskContainer::Iterator it = containers.begin();
        for (; it != containers.end(); ++it)
        {
            TaskContainer* c = *it;

            if (container->id().lower() == c->id().lower())
            {
                // search for the last occurrence of this app
                for (; it != containers.end(); ++it)
                {
                    c = *it;

                    if (container->id().lower() != c->id().lower())
                    {
                        break;
                    }
                }
                break;
            }
        }

        // alphabetize containers
        it = containers.begin();
        for (; it != containers.end(); ++it)
        {
            TaskContainer* c = *it;
            if (TQString::localeAwareCompare(container->id().lower(), c->id().lower()) < 0) {
                break;
            }
        }

        if (it != containers.end())
        {
            containers.insert(it, container);
        }
        else
        {
            containers.append(container);
        }
    }
    else
    {
        containers.append(container);
    }

    addChild(container);
    reLayoutEventually();
    emit containerCountChanged();
}

void TaskBar::remove(Task::Ptr task, TaskContainer* container)
{
    for (TaskContainer::Iterator it = m_hiddenContainers.begin();
         it != m_hiddenContainers.end();
         ++it)
    {
        if ((*it)->contains(task))
        {
            (*it)->finish();
            m_deletableContainers.append(*it);
            m_hiddenContainers.erase(it);
            break;
        }
    }

    if (!container)
    {
        for (TaskContainer::Iterator it = containers.begin();
             it != containers.end();
             ++it)
        {
            if ((*it)->contains(task))
            {
                container = *it;
                break;
            }
        }

        if (!container)
        {
            return;
        }
    }

    container->remove(task);

    if (container->isEmpty())
    {
        TaskContainer::List::iterator it = containers.find(container);
        if (it != containers.end())
        {
            containers.erase(it);
        }

        removeChild(container);
        container->finish();
        m_deletableContainers.append(container);

        reLayoutEventually();
        emit containerCountChanged();
    }
    else if (container->filteredTaskCount() < 1)
    {
        reLayoutEventually();
        emit containerCountChanged();
    }
}

void TaskBar::remove(Startup::Ptr startup, TaskContainer* container)
{
    for (TaskContainer::Iterator it = m_hiddenContainers.begin();
         it != m_hiddenContainers.end();
         ++it)
    {
        if ((*it)->contains(startup))
        {
            (*it)->remove(startup);

            if ((*it)->isEmpty())
            {
                (*it)->finish();
                m_deletableContainers.append(*it);
                m_hiddenContainers.erase(it);
            }

            break;
        }
    }

    if (!container)
    {
        for (TaskContainer::Iterator it = containers.begin();
             it != containers.end();
             ++it)
        {
            if ((*it)->contains(startup))
            {
                container = *it;
                break;
            }
        }

        if (!container)
        {
            return;
        }
    }

    container->remove(startup);
    if (!container->isEmpty())
    {
        return;
    }

    TaskContainer::List::iterator it = containers.find(container);
    if (it != containers.end())
    {
        containers.erase(it);
    }

    // startup containers only ever contain that one item. so
    // just delete the poor bastard.
    container->finish();
    m_deletableContainers.append(container);
    reLayoutEventually();
    emit containerCountChanged();
}

void TaskBar::desktopChanged(int desktop)
{
    if (m_showAllWindows)
    {
        return;
    }

    m_relayoutTimer.stop();
    m_ignoreUpdates = true;
    for (TaskContainer::Iterator it = containers.begin();
         it != containers.end();
         ++it)
    {
        (*it)->desktopChanged(desktop);
    }

    m_ignoreUpdates = false;
    reLayout();
    emit containerCountChanged();
}

void TaskBar::windowChanged(Task::Ptr task)
{
    if (m_showOnlyCurrentScreen &&
        !TaskManager::isOnScreen(showScreen(), task->window()))
    {
        return; // we don't care about this window
    }

    TaskContainer* container = 0;
    for (TaskContainer::List::const_iterator it = containers.constBegin();
         it != containers.constEnd();
         ++it)
    {
        TaskContainer* c = *it;

        if (c->contains(task))
        {
            container = c;
            break;
        }
    }

    // if we don't have a container or we're showing only windows on this
    // desktop and the container is neither on the desktop nor currently visible
    // just skip it
    if (!container ||
        (!m_showAllWindows &&
         !container->onCurrentDesktop() &&
         !container->isVisibleTo(this)))
    {
        return;
    }

    bool wasShown = container->isVisibleTo(this);
    container->windowChanged(task);   // updates the button's label/icon/active-state repaint
    // New status, mirroring 'TaskBar::filteredContainers()' logic
    bool isShown = (m_showAllWindows || container->onCurrentDesktop()) &&
                   (!m_showOnlyIconified || container->isIconified()) &&
                   ((showScreen() == -1) || container->isOnScreen()) &&
                   (!container->isHidden());

    // We only call 'reLayoutEventually()' if the number of visible buttons
    // has changed. This avoids unnecessary redrawing and annoying flickering
    // when the taskbar has been scrolled.
    if (wasShown != isShown)
    {
        emit containerCountChanged();
        reLayoutEventually();
    }
}

void TaskBar::windowChangedGeometry(Task::Ptr task)
{
    //TODO: this gets called every time a window's geom changes
    //      when we are in "show only on the same Xinerama screen"
    //      mode it would be Good(tm) to compress these events so this
    //      gets run less often, but always when needed
    TaskContainer* container = 0;
    for (TaskContainer::Iterator it = containers.begin();
         it != containers.end();
         ++it)
    {
        TaskContainer* c = *it;
        if (c->contains(task))
        {
            container = c;
            break;
        }
    }

    if ((!!container) == TaskManager::isOnScreen(showScreen(), task->window()))
    {
        // we have this window covered, so we don't need to do anything
        return;
    }

    if (container)
    {
        remove(task, container);
    }
    else
    {
        add(task);
    }
}

void TaskBar::reLayoutEventually()
{
    m_relayoutTimer.stop();

    if (!blocklayout && !m_ignoreUpdates)
    {
        m_relayoutTimer.start(25, true);
    }
}

void TaskBar::reLayout()
{
    // Because TQPopupMenu::exec() creates its own event loop, deferred deletes
    // via TQObject::deleteLater() may be prematurely executed when a container's
    // popup menu is visible.
    //
    // To get around this, we collect the containers and delete them manually
    // when doing a relayout. (kling)
    if (!m_deletableContainers.isEmpty()) {
        TaskContainer::List::iterator it = m_deletableContainers.begin();
        for (; it != m_deletableContainers.end(); ++it)
            delete *it;
        m_deletableContainers.clear();
    }

    // Remember where the taskbar is scrolled to in order to restore the
    // same position at the end of the relayouting operation.
    int oldContentsX = contentsX();
    int oldContentsY = contentsY();

    // filter task container list
    TaskContainer::List list = filteredContainers();

    if (list.count() < 1)
    {
        resizeContents(contentsRect().width(), contentsRect().height());
        return;
    }

    if (isGrouping != shouldGroup())
    {
        reGroup();
        return;
    }

    // sort container list by desktop
    if (m_sortByDesktop)
    {
        sortContainersByDesktop(list);
    }

    // needed because Panner doesn't know how big it's contents are so it's
    // up to us to initialize it. =(
    resizeContents(contentsRect().width(), contentsRect().height());

    // number of rows simply depends on our height which is either the
    // minimum button height or the height of the font in use, whichever is
    // largest
    int minButtonHeight = buttonHeight();

    // horizontal layout
    if (orientation() == TQt::Horizontal)
    {
        int bwidth=buttonWidth();
        int rows = contentsRect().height() / minButtonHeight;
        if (rows<1)
          rows=1;

        // actual button height
        int bheight = contentsRect().height() / rows;
        if (bheight<1)  // avoid zero devision later
          bheight=1;

        // buttons per row
        int bpr = static_cast<int>(ceil(static_cast<double>(list.count()) / rows));

        // adjust content size
        if ( contentsRect().width() < bpr * bwidth )
        {
            resizeContents( bpr * bwidth, contentsRect().height() );
        }

        // maximum number of buttons per row
        int mbpr = contentsRect().width() / bwidth;

        // expand button width if space permits and the taskbar is not in 'icons only' mode
        if (mbpr > bpr)
        {
            if (!showIcons() || showText())
              bwidth = contentsRect().width() / bpr;
            int maxWidth = READ_MERGED_TASKBAR_SETTING(maximumButtonWidth);
            if (maxWidth > 0 && bwidth > maxWidth)
            {
                bwidth = maxWidth;
            }
        }

        // layout containers

        // for taskbars at the bottom, we need to ensure that the bottom
        // buttons touch the bottom of the screen. since we layout from
        // top to bottom this means seeing if we have any padding and
        // popping it on the top. this preserves Fitt's Law behaviour
        // for taskbars on the bottom
        int topPadding = 0;
        if (arrowType == UpArrow)
        {
            topPadding = contentsRect().height() % (rows * bheight);
        }

        int i = 0;
        bool reverseLayout = TQApplication::reverseLayout();
        for (TaskContainer::Iterator it = list.begin();
             it != list.end();
             ++it, i++)
        {
            TaskContainer* c = *it;

            int row = i % rows;

            int x = ( i / rows ) * bwidth;
            if (reverseLayout)
            {
                x = contentsRect().width() - x - bwidth;
            }
            int y = (row * bheight) + topPadding;

            c->setArrowType(arrowType);

            if (childX(c) != x || childY(c) != y)
                moveChild(c, x, y);

            if (c->width() != bwidth || c->height() != bheight)
                c->resize( bwidth, bheight );

            c->setBackground();
        }
    }
    else // vertical layout
    {
        // Adjust min button height to keep gaps into account
        int minButtonHeightAdjusted=minButtonHeight+BUTTON_VERTICAL_GAP;
        // adjust content size
        if (contentsRect().height() < (int)list.count() * minButtonHeightAdjusted)
        {
            resizeContents(contentsRect().width(), list.count() * minButtonHeightAdjusted);
        }

        // layout containers
        int i = 0;
        for (TaskContainer::Iterator it = list.begin();
             it != list.end();
             ++it)
        {
            TaskContainer* c = *it;

            c->setArrowType(arrowType);

            if (c->width() != contentsRect().width() || c->height() != minButtonHeightAdjusted)
                c->resize(contentsRect().width(), minButtonHeightAdjusted);

            if (childX(c) != 0 || childY(c) != (i * minButtonHeightAdjusted))
                moveChild(c, 0, i * minButtonHeightAdjusted);

            c->setBackground();
            i++;
        }
    }

    TQTimer::singleShot(100, this, TQ_SLOT(publishIconGeometry()));

    // Restore the taskbar scroll position as it was before the relayout.
    setContentsPos(oldContentsX, oldContentsY);
}

void TaskBar::setViewportBackground()
{
    const TQPixmap *bg = parentWidget()->backgroundPixmap();

    if (bg)
    {
        TQPixmap pm(parentWidget()->size());
        pm.fill(parentWidget(), pos() + viewport()->pos());
        viewport()->setPaletteBackgroundPixmap(pm);
        viewport()->setBackgroundOrigin(WidgetOrigin);
    }
    else
        viewport()->setPaletteBackgroundColor(paletteBackgroundColor());
}

void TaskBar::setBackground()
{
    setViewportBackground();

    TaskContainer::List list = filteredContainers();

    for (TaskContainer::Iterator it = list.begin();
            it != list.end();
            ++it)
    {
        TaskContainer* c = *it;
        c->setBackground();
    }
}

void TaskBar::setArrowType(TQt::ArrowType at)
{
    if (arrowType == at)
    {
        return;
    }

    arrowType = at;
    for (TaskContainer::Iterator it = containers.begin();
         it != containers.end();
         ++it)
    {
        (*it)->setArrowType(arrowType);
    }
}

void TaskBar::publishIconGeometry()
{
    TQPoint p = mapToGlobal(TQPoint(0,0)); // roundtrip, don't do that too often

    for (TaskContainer::Iterator it = containers.begin();
         it != containers.end();
         ++it)
    {
        (*it)->publishIconGeometry(p);
    }
}

void TaskBar::viewportMousePressEvent( TQMouseEvent* e )
{
    propagateMouseEvent( e );
}

void TaskBar::viewportMouseReleaseEvent( TQMouseEvent* e )
{
    propagateMouseEvent( e );
}

void TaskBar::viewportMouseDoubleClickEvent( TQMouseEvent* e )
{
    propagateMouseEvent( e );
}

void TaskBar::viewportMouseMoveEvent( TQMouseEvent* e )
{
    propagateMouseEvent( e );
}

void TaskBar::propagateMouseEvent( TQMouseEvent* e )
{
    if ( !isTopLevel()  )
    {
        TQMouseEvent me( e->type(), mapTo( topLevelWidget(), e->pos() ),
                        e->globalPos(), e->button(), e->state() );
        TQApplication::sendEvent( topLevelWidget(), &me );
    }
}

bool TaskBar::idMatch( const TQString& id1, const TQString& id2 )
{
    if ( id1.isEmpty() || id2.isEmpty() )
        return false;

    return id1.lower() == id2.lower();
}

int TaskBar::containerCount() const
{
    int i = 0;

    for (TaskContainer::List::const_iterator it = containers.constBegin();
         it != containers.constEnd();
         ++it)
    {
        if ((m_showAllWindows || (*it)->onCurrentDesktop()) &&
            ((showScreen() == -1) || ((*it)->isOnScreen())))
        {
            if (!(*it)->isHidden())
            {
                i++;
            }
        }
    }

    return i;
}

int TaskBar::taskCount() const
{
    int i = 0;

    for (TaskContainer::List::const_iterator it = containers.constBegin();
         it != containers.constEnd();
         ++it)
    {
        if ((m_showAllWindows || (*it)->onCurrentDesktop()) &&
            ((showScreen() == -1) || ((*it)->isOnScreen())))
        {
            if (!(*it)->isHidden())
            {
                i += (*it)->filteredTaskCount();
            }
        }
    }

    return i;
}

int TaskBar::maximumButtonsWithoutShrinking() const
{
    int minButtonHeight = buttonHeight();
    int rows = contentsRect().height() / minButtonHeight;

    if (rows < 1)
    {
        rows = 1;
    }

    if ( orientation() == TQt::Horizontal ) {
        // maxWidth of 0 means no max width, drop back to default
        int maxWidth = READ_MERGED_TASKBAR_SETTING(maximumButtonWidth);
        if (maxWidth == 0)
        {
            maxWidth = BUTTON_MAX_WIDTH;
        }

        // They squash a bit before they pop, hence the 2
        return rows * (contentsRect().width() / maxWidth) + 2;
    }
    else
    {
        // Overlap slightly and ugly arrows appear, hence -1
        return rows - 1;
    }
}

bool TaskBar::shouldGroup() const
{
    return READ_MERGED_TASKBAR_SETTING(groupTasks) == m_settingsObject->GroupAlways ||
           ((READ_MERGED_TASKBAR_SETTING(groupTasks) == m_settingsObject->GroupWhenFull &&
             taskCount() > maximumButtonsWithoutShrinking()));
}

void TaskBar::reGroup()
{
    isGrouping = shouldGroup();
    blocklayout = true;

    TaskContainer::Iterator lastContainer = m_hiddenContainers.end();
    for (TaskContainer::Iterator it = m_hiddenContainers.begin();
         it != lastContainer;
         ++it)
    {
        (*it)->finish();
        m_deletableContainers.append(*it);
    }
    m_hiddenContainers.clear();

    for (TaskContainer::List::const_iterator it = containers.constBegin();
         it != containers.constEnd();
         ++it)
    {
        (*it)->finish();
        m_deletableContainers.append(*it);
    }
    containers.clear();

    Task::Dict tasks = TaskManager::the()->tasks();
    Task::Dict::iterator lastTask = tasks.end();
    for (Task::Dict::iterator it = tasks.begin(); it != lastTask; ++it)
    {
        Task::Ptr task = it.data();
        if (showScreen() == -1 || task->isOnScreen(showScreen()))
        {
            add(task);
        }
    }

    Startup::List startups = TaskManager::the()->startups();
    Startup::List::iterator itEnd = startups.end();
    for (Startup::List::iterator sIt = startups.begin(); sIt != itEnd; ++sIt)
    {
        add(*sIt);
    }

    blocklayout = false;
    reLayoutEventually();
}


TaskContainer::List TaskBar::filteredContainers()
{
    // filter task container list
    TaskContainer::List list;

    for (TaskContainer::List::const_iterator it = containers.constBegin();
        it != containers.constEnd();
        ++it)
    {
        TaskContainer* c = *it;
        if ((m_showAllWindows || c->onCurrentDesktop()) &&
            (!m_showOnlyIconified || c->isIconified()) &&
            ((showScreen() == -1) || c->isOnScreen()) &&
            (!c->isHidden()))
        {
            list.append(c);
            c->show();
        }
        else
        {
            c->hide();
        }
    }

    return list;
}

void TaskBar::activateNextTask(bool forward)
{
    bool forcenext = false;
    TaskContainer::List list = filteredContainers();

    // this is necessary here, because 'containers' is unsorted and
    // we want to iterate over the _shown_ task containers in a linear way
    if (m_sortByDesktop)
    {
        sortContainersByDesktop(list);
    }

    int numContainers = list.count();
    TaskContainer::List::iterator it;
    for (int i = 0; i < numContainers; ++i)
    {
        it = forward ? list.at(i) : list.at(numContainers - i - 1);

        if (it != list.end() && (*it)->activateNextTask(forward, forcenext))
        {
            return;
        }
    }

    if (forcenext)
    {
        // moving forward from the last, or backward from the first, loop around
        for (int i = 0; i < numContainers; ++i)
        {
            it = forward ? list.at(i) : list.at(numContainers - i - 1);

            if (it != list.end() && (*it)->activateNextTask(forward, forcenext))
            {
                return;
            }
        }

        return;
    }

    forcenext = true; // select first
    for (int i = 0; i < numContainers; ++i)
    {
        it = forward ? list.at(i) : list.at(numContainers - i - 1);

        if (it == list.end())
        {
            break;
        }

        TaskContainer* c = *it;
        if (m_sortByDesktop)
        {
            if (forward ? c->desktop() < TaskManager::the()->currentDesktop()
                        : c->desktop() > TaskManager::the()->currentDesktop())
            {
                continue;
            }
        }

        if (c->activateNextTask(forward, forcenext))
        {
            return;
        }
    }
}

void TaskBar::wheelEvent(TQWheelEvent* e)
{
    bool altPressed = (e->state() & TQt::AltButton);
    bool ctrlPressed = (e->state() & TQt::ControlButton);
    bool shiftPressed = (e->state() & TQt::ShiftButton);
    bool noMod = !altPressed && !ctrlPressed && !shiftPressed;

    int cycleAction = READ_MERGED_TASKBAR_WACTION(TaskBarSettings::CycleWindows);
    int scrollAction = READ_MERGED_TASKBAR_WACTION(TaskBarSettings::ScrollTaskbar);

    if ((cycleAction == m_settingsObject->ModNone && noMod) ||
        (cycleAction == m_settingsObject->ModAlt && altPressed) ||
        (cycleAction == m_settingsObject->ModCtrl && ctrlPressed) ||
        (cycleAction == m_settingsObject->ModShift && shiftPressed))
    {
        if (e->delta() > 0)
        {
            activateNextTask(false);
        }
        else
        {
            activateNextTask(true);
        }
    }

    if ((scrollAction == m_settingsObject->ModNone && noMod) ||
        (scrollAction == m_settingsObject->ModAlt && altPressed) ||
        (scrollAction == m_settingsObject->ModCtrl && ctrlPressed) ||
        (scrollAction == m_settingsObject->ModShift && shiftPressed))
    {
        int wheelScrollStep;
        if (READ_MERGED_TASKBAR_SETTING(useCustomWheelScrollStep)) {
            wheelScrollStep = m_wheelScrollStep;
        } else {
            int btnSz = orientation() == TQt::Horizontal ?
                buttonWidth() : (buttonHeight() + BUTTON_VERTICAL_GAP);
            wheelScrollStep = btnSz * TQApplication::wheelScrollLines();
        }
        // Avoid scrolling more than there is avaliable space
        wheelScrollStep = TQMIN(wheelScrollStep,
                orientation() == TQt::Horizontal ? visibleWidth() : visibleHeight());

        // the delta is defined as multiples of WHEEL_DELTA,
        // which is set to 120. See TQETWidget::translateMouseEvent()
        // in tqapplication_x11.cpp for more details.
        int step = e->delta() * wheelScrollStep / 120;
        if ( orientation() == TQt::Horizontal )
        {
            scrollBy( -step, 0 );
        }
        else
        {
            scrollBy( 0, -step );
        }
    }
}

void TaskBar::slotActivateNextTask()
{
    activateNextTask( true );
}

void TaskBar::slotActivatePreviousTask()
{
    activateNextTask( false );
}

void TaskBar::slotSettingsChanged( int category )
{
    if( category == (int) TDEApplication::SETTINGS_SHORTCUTS )
    {
        keys->readSettings();
        keys->updateConnections();
    }
}

int TaskBar::showScreen() const
{
    if (m_showOnlyCurrentScreen && m_currentScreen == -1)
    {
        const_cast<TaskBar*>(this)->m_currentScreen =
            TQApplication::desktop()->screenNumber(mapToGlobal(this->geometry().topLeft()));
    }

    return m_currentScreen;
}

TQImage* TaskBar::blendGradient(const TQSize& size)
{
    if (m_blendGradient.isNull() || m_blendGradient.size() != size)
    {
        TQPixmap bgpm(size);
        TQPainter bgp(&bgpm);
        bgpm.fill(black);

        if (TQApplication::reverseLayout())
        {
            TQImage gradient = KImageEffect::gradient(
                    TQSize(30, size.height()),
                    TQColor(255,255,255),
                    TQColor(0,0,0),
                    KImageEffect::HorizontalGradient);
            bgp.drawImage(0, 0, gradient);
        }
        else
        {
            TQImage gradient = KImageEffect::gradient(
                    TQSize(30, size.height()),
                    TQColor(0,0,0),
                    TQColor(255,255,255),
                    KImageEffect::HorizontalGradient);
            bgp.drawImage(size.width() - 30, 0, gradient);
        }

        m_blendGradient = bgpm.convertToImage();
    }

    return &m_blendGradient;
}

void TaskBar::sortContainersByDesktop(TaskContainer::List& list)
{
    typedef TQValueVector<TQPair<int, TQPair<int, TaskContainer*> > > SortVector;
    SortVector sorted;
    sorted.resize(list.count());
    int i = 0;

    TaskContainer::List::ConstIterator lastUnsorted(list.constEnd());
    for (TaskContainer::List::ConstIterator it = list.constBegin();
            it != lastUnsorted;
            ++it)
    {
        sorted[i] = qMakePair((*it)->desktop(), qMakePair(i, *it));
        ++i;
    }

    qHeapSort(sorted);

    list.clear();
    SortVector::const_iterator lastSorted(sorted.constEnd());
    for (SortVector::const_iterator it = sorted.constBegin();
         it != lastSorted;
         ++it)
    {
        list.append((*it).second.second);
    }
}

TaskMoveDestination::TaskMoveDestination TaskBar::taskMoveCapabilities(TaskContainer* movingContainer) {
    TaskMoveDestination::TaskMoveDestination ret = TaskMoveDestination::Null;

    bool before = false;
    bool after = false;
    bool movingFound = false;

    if (movingContainer) {
        // Check to see if there are any visible containers before or after the movingContainer
        TaskContainer::Iterator it = containers.begin();
        for (; it != containers.end(); ++it)
        {
            TaskContainer* c = *it;
            if (!c->isVisibleTo(this)) {
                continue;
            }
            if (c == movingContainer) {
                movingFound = true;
            }
            else {
                if (movingFound) {
                    after = true;
                }
                else {
                    before = true;
                }
            }
        }
        if (before) {
            ret = ret | TaskMoveDestination::Left;
        }
        if (after) {
            ret = ret | TaskMoveDestination::Right;
        }
    }

    return ret;
}

int TaskBar::taskMoveHandler(TaskMoveDestination::TaskMoveDestination dest, Task::List taskList, const TQPoint pos) {
    TaskContainer* movingContainer = NULL;
    TaskContainer* destContainer = NULL;
    bool movingRight = true;

    TaskContainer::Iterator it = containers.begin();
    for (; it != containers.end(); ++it)
    {
        TaskContainer* c = *it;
        if (!c->isVisibleTo(this)) {
            continue;
        }
        if (c->taskList() == taskList) {
            movingContainer = c;
            break;
        }
    }

    if (movingContainer) {
        if (dest == TaskMoveDestination::Position) {
            // Find the best place for the container to go...
            it = containers.begin();
            for (; it != containers.end(); ++it)
            {
                TaskContainer* c = *it;
                if (!c->isVisibleTo(this)) {
                    continue;
                }
                TQPoint containerPos = c->pos();
                TQSize containerSize = c->size();
                TQRect containerRect(containerPos.x(), containerPos.y(), containerSize.width(), containerSize.height());
                if (containerRect.contains(pos)) {
                    destContainer = c;
                    // Figure out if the mobile container is moving towards the end of the container list (i.e. right or down)
                    for (; it != containers.end(); ++it)
                    {
                        if (movingContainer == (*it)) {
                            movingRight = false;
                        }
                    }
                    break;
                }
            }
        }
        else if (dest == TaskMoveDestination::Beginning) {
            // Move to beginning
            it = containers.begin();
            while ((it != containers.end()) && (!(*it)->isVisibleTo(this))) {
                it++;
            }
            if (it == containers.end()) {
                return false;
            }
            destContainer = *it;
            movingRight = false;
        }
        else if (dest == TaskMoveDestination::Left) {
            // Move left
            it = containers.begin();
            while ((it != containers.end()) && (!(*it)->isVisibleTo(this))) {
                it++;
            }
            if (it == containers.end()) {
                return false;
            }
            TaskContainer* prev = *it;
            destContainer = prev;
            for (; it != containers.end(); ++it)
            {
                TaskContainer* c = *it;
                if (!c->isVisibleTo(this)) {
                    continue;
                }
                if (movingContainer == c) {
                    destContainer = prev;
                    break;
                }
                prev = c;
            }
            movingRight = false;
        }
        else if (dest == TaskMoveDestination::Right) {
            // Move right
            it = containers.begin();
            destContainer = NULL;
            for (; it != containers.end(); ++it)
            {
                TaskContainer* c = *it;
                if (!c->isVisibleTo(this)) {
                    continue;
                }
                if (movingContainer == c) {
                    if (it != containers.end()) {
                        it++;
                        while ((it != containers.end()) && (!(*it)->isVisibleTo(this))) {
                            it++;
                        }
                    }
                    if ((it != containers.end()) && ((*it)->isVisibleTo(this))) {
                        destContainer = *it;
                    }
                    break;
                }
            }
            movingRight = true;
        }
        else if (dest == TaskMoveDestination::End) {
            // Move to end
            destContainer = NULL;
            movingRight = true;
        }

        if (destContainer == movingContainer) {
            return false;
        }

        removeChild(movingContainer);
        containers.remove(movingContainer);

        if (destContainer) {
            it = containers.find(destContainer);
            if ((it != containers.end()) && (movingRight)) {
                it++;
                while ((it != containers.end()) && (!(*it)->isVisibleTo(this))) {
                    it++;
                }
            }
            if ((it != containers.end()) && ((*it)->isVisibleTo(this))) {
                containers.insert(it, movingContainer);
            }
            else {
                containers.append(movingContainer);
            }
        }
        else {
            containers.append(movingContainer);
        }

        addChild(movingContainer);
        reLayoutEventually();
        emit containerCountChanged();

        return true;
    }

    return false;
}
