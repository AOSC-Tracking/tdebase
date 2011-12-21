/*****************************************************************

Copyright (c) 1996-2004 the kicker authors. See file AUTHORS.

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

#include <unistd.h>

#include <tqdir.h>
#include <tqfile.h>
#include <tqfileinfo.h>
#include <tqpainter.h>
#include <tqpixmap.h>
#include <tqstyle.h>
#include <tqtextstream.h>
#include <tqtimer.h>
#include <tqwmatrix.h>

#include <kapplication.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kprocess.h>
#include <krootpixmap.h>
#include <kpixmap.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kservice.h>
#include <kurldrag.h>

#include "addapplet.h"
#include "browser_dlg.h"
#include "container_applet.h"
#include "container_button.h"
#include "containerarealayout.h"
#include "dirdrop_mnu.h"
#include "exe_dlg.h"
#include "extensionmanager.h"
#include "kicker.h"
#include "kickerSettings.h"
#include "kickertip.h"
#include "paneldrag.h"
#include "pluginmanager.h"

#include "containerarea.h"

// #define INCLUDE_LOCK_APPLET_BY_DEFAULT

// for multihead
extern int kicker_screen_number;

ContainerArea::ContainerArea(KConfig* _c,
                             TQWidget* parent,
                             TQPopupMenu* opMenu,
                             const char* name)
    : Panner(parent, name),
      _moveAC(0),
      _pos(KPanelExtension::Left),
      _config(_c),
      _dragIndicator(0),
      _dragMoveAC(0),
      _dragMoveOffset(TQPoint(0,0)),
      m_opMenu(opMenu),
      _rootPixmap(0),
      _useBgTheme(false),
      _bgSet(false),
      m_canAddContainers(true),
      m_immutable(_c->isImmutable()),
      m_updateBackgroundsCalled(false),
      m_layout(0),
      m_addAppletDialog(0),
      _autoScrollTimer(0, "ContainerArea::autoScrollTimer")
{
    setBackgroundOrigin( WidgetOrigin );

    m_contents = viewport();

    m_layout = new ContainerAreaLayout(m_contents);

    // Install an event filter to propagate layout hints coming from m_contents.
    m_contents->installEventFilter(this);

    setBackground();

    connect(&_autoScrollTimer, TQT_SIGNAL(timeout()), TQT_SLOT(autoScroll()));
    connect(kapp, TQT_SIGNAL(kdisplayPaletteChanged()), TQT_SLOT(setBackground()));
    connect(Kicker::the(), TQT_SIGNAL(immutabilityChanged(bool)),
            TQT_SLOT(immutabilityChanged(bool)));
    connect(this, TQT_SIGNAL(contentsMoving(int, int)), TQT_SLOT(setBackground()));
}

ContainerArea::~ContainerArea()
{
    // don't emit signals from destructor
    blockSignals( true );
    // clear applets
    removeAllContainers();
}

void ContainerArea::initialize(bool useDefaultConfig)
{
    // do we really need to do this?
    removeAllContainers();

    // restore applet layout or load a default panel layout
    _config->setGroup("General");
    if (_config->hasKey("Applets2"))
    {
        if (_config->groupIsImmutable("General"))
        {
            m_immutable = true;
        }

        m_canAddContainers = !m_immutable &&
                             !_config->entryIsImmutable("Applets2");
        loadContainers(_config->readListEntry("Applets2"));
    }
    else if (useDefaultConfig)
    {
        defaultContainerConfig();
    }

    setAcceptDrops(!isImmutable());
    TQTimer::singleShot(0, this, TQT_SLOT(resizeContents()));
}

void ContainerArea::defaultContainerConfig()
{
    //FIXME: make this use a file template so it isn't hardcoded anymore
    BaseContainer::List containers;

    containers.append(new KMenuButtonContainer(m_opMenu, m_contents));

    int dsize;
    if (orientation() == Qt::Horizontal)
    {
        dsize = width();
    }
    else
    {
        dsize = height();
    }

    dsize -= 560;
    TQStringList buttons;

    TQFile f(locate("data", "kicker/default-apps"));
    if (f.open(IO_ReadOnly))
    {
        TQTextStream is(&f);

        while (!is.eof())
            buttons << is.readLine();

        f.close();
    }
    else
    {
        buttons << "kde-Home.desktop"
                << "kde-konqbrowser.desktop";
    }

    //int size = dsize;
    for (TQStringList::ConstIterator it = buttons.begin(); it != buttons.end(); ++it)
    {
        /*size -= 42;
        if (size <= 0)
            break;*/

        BaseContainer *button = 0;
        KService::Ptr service = KService::serviceByStorageId(*it);
        if (!service)
        {
            // look for a special button
            TQString s = locate("appdata", *it);
            if (s.isEmpty()) continue;
            TQString itExt = (*it).section('/', 1);
            button = new ExtensionButtonContainer(itExt, m_opMenu, m_contents);
        }
        else
        {
            button = new ServiceButtonContainer(service, m_opMenu, m_contents);
        }

        if (button->isValid())
        {
            containers.append(button);
        }
        else
        {
            delete button;
        }
    }

    PluginManager* manager = PluginManager::the();

    // pager applet
    AppletContainer* a = manager->createAppletContainer(
        "minipagerapplet.desktop",
        true,
        TQString::null,
        m_opMenu,
        m_contents);
    if (a)
    {
        a->setFreeSpace(0.09);
        containers.append(a);
    }

    // taskbar applet
    a = manager->createAppletContainer(
        "taskbarapplet.desktop",
        true,
        TQString::null,
        m_opMenu,
        m_contents);
    if (a)
    {
        a->setFreeSpace(0.09);
        containers.append(a);
    }

    // system tray applet
    a = manager->createAppletContainer(
        "systemtrayapplet.desktop",
        true,
        TQString::null,
        m_opMenu,
        m_contents );
    if (a)
    {
        a->setFreeSpace(1);
        containers.append(a);
    }

    // clock applet
    a = manager->createAppletContainer(
        "clockapplet.desktop",
        true,
        TQString::null,
        m_opMenu,
        m_contents );
    if (a)
    {
        a->setFreeSpace(1);
        containers.append(a);
    }

#ifdef INCLUDE_LOCK_APPLET_BY_DEFAULT
    // lock applet
    a = manager->createAppletContainer(
        "lockout.desktop",
        true,
        TQString::null,
        m_opMenu,
        m_contents );
    if (a)
    {
        a->setFreeSpace(1);
        containers.append(a);
    }
#endif

    for (BaseContainer::Iterator it = containers.begin();
         it != containers.end();
         ++it)
    {
         addContainer(*it);
    }

    saveContainerConfig();
}

void ContainerArea::loadContainers(const TQStringList& containers)
{
    // read applet list
    bool badApplets = false;

    // now restore the applets
    TQStringList::const_iterator it = containers.constBegin();
    TQStringList::const_iterator itEnd = containers.constEnd();
    for (; it != itEnd; ++it)
    {
        TQString appletId(*it);

        // is there a config group for this applet?
        if (!_config->hasGroup(appletId))
        {
            continue;
        }

        KConfigGroup group(_config, appletId.latin1());

        BaseContainer* a = 0;

        int sep = appletId.findRev('_');
        Q_ASSERT(sep != -1);
        TQString appletType = appletId.left(sep);

        // create a matching applet container
        if (appletType == "KMenuButton")
            a = new KMenuButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "DesktopButton")
            a = new DesktopButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "WindowListButton")
            a = new WindowListButtonContainer(group, m_opMenu, m_contents);
        else if ((appletType == "BookmarksButton") && kapp->authorizeKAction("bookmarks"))
            a = new BookmarksButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "ServiceButton")
            a = new ServiceButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "URLButton")
            a = new URLButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "BrowserButton")
            a = new BrowserButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "ServiceMenuButton")
            a = new ServiceMenuButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "ExecButton")
            a = new NonKDEAppButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "ExtensionButton")
            a = new ExtensionButtonContainer(group, m_opMenu, m_contents);
        else if (appletType == "Applet")
        {
            bool immutable = Kicker::the()->isImmutable() ||
                             group.groupIsImmutable() ||
                             group.entryIsImmutable("ConfigFile");
            a = PluginManager::the()->createAppletContainer(
                   group.readPathEntry("DesktopFile"),
                   true, // isStartup
                   group.readPathEntry("ConfigFile"),
                   m_opMenu,
                   m_contents,
                   immutable);
        }

        if (a && a->isValid())
        {
            a->setAppletId(appletId);
            a->loadConfiguration(group);
            addContainer(a);
        }
        else
        {
            badApplets = true;
            delete a;
        }
    }

    if (badApplets)
    {
        // since we may have had Bad Applets in our list
        // let's save it again, just in case
        saveContainerConfig();
    }

    // while this is also called in addContainer (well, resizeContents()),
    // it gets executed too soon. we need to wait until the containers are
    // actually resized, but we enter the event loop prior to that happening
    // above.
    TQTimer::singleShot(0, this, TQT_SLOT(updateContainersBackground()));
}

void ContainerArea::saveContainerConfig(bool layoutOnly)
{
    if (!canAddContainers())
    {
        return;
    }

    // Save the applet list
    TQStringList alist;
    TQLayoutIterator it2 = m_layout->iterator();
    for (; it2.current(); ++it2)
    {
        BaseContainer* a = dynamic_cast<BaseContainer*>(it2.current()->widget());
        if (a)
        {
            KConfigGroup group(_config, a->appletId().latin1());
            a->saveConfiguration(group, layoutOnly);
            alist.append(a->appletId());
        }
    }

    KConfigGroup group( _config, "General" );
    group.writeEntry("Applets2", alist);

    _config->sync();
}

void ContainerArea::removeAllContainers()
{
    for (BaseContainer::List::const_iterator it = m_containers.constBegin();
         it != m_containers.constEnd();
         ++it)
    {
        delete *it;
    }
    m_containers.clear();
}

void ContainerArea::configure()
{
    setBackground();

    for (BaseContainer::Iterator it = m_containers.begin();
         it != m_containers.end();
         ++it)
    {
        (*it)->configure();
    }

    resizeContents();
}

const TQWidget* ContainerArea::addButton(const AppletInfo& info)
{
    TQString buttonType = info.library();

    if (buttonType == "BookmarksButton")
    {
        if (kapp->authorizeKAction("bookmarks"))
        {
            return addBookmarksButton();
        }
    }
    else if (buttonType == "BrowserButton")
    {
        return addBrowserButton();
    }
    else if (buttonType == "DesktopButton")
    {
        return addDesktopButton();
    }
    else if (buttonType == "ExecButton")
    {
        return addNonKDEAppButton();
    }
    else if (buttonType == "KMenuButton")
    {
        return addKMenuButton();
    }
    else if (buttonType == "WindowListButton")
    {
        return addWindowListButton();
    }
    else // ExtensionButton
    {
        return addExtensionButton(info.desktopFile());
    }

    return 0;
}

const TQWidget* ContainerArea::addKMenuButton()
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new KMenuButtonContainer(m_opMenu, m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addDesktopButton()
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new DesktopButtonContainer(m_opMenu, m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addWindowListButton()
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new WindowListButtonContainer(m_opMenu, m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addBookmarksButton()
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new BookmarksButtonContainer(m_opMenu, m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addServiceButton(const TQString& desktopFile)
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new ServiceButtonContainer(desktopFile,m_opMenu,
                                                  m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addURLButton(const TQString &url)
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new URLButtonContainer(url, m_opMenu, m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addBrowserButton()
{
    if (!canAddContainers())
    {
        return 0;
    }

    PanelBrowserDialog *dlg = new PanelBrowserDialog(TQDir::home().path(),
                                                     "kdisknav");

    if (dlg->exec() == TQDialog::Accepted)
    {
        return addBrowserButton(dlg->path(), dlg->icon());
    }

    return 0;
}

const TQWidget* ContainerArea::addBrowserButton(const TQString &startDir,
                                       const TQString& icon)
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new BrowserButtonContainer(startDir, m_opMenu,
                                                  icon, m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addServiceMenuButton(const TQString& relPath)
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new ServiceMenuButtonContainer(relPath, m_opMenu,
                                                      m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addNonKDEAppButton()
{
    if (!canAddContainers())
    {
        return 0;
    }

    PanelExeDialog dlg(TQString::null, TQString::null, TQString::null,
                       TQString::null, TQString::null, false, 0);

    if (dlg.exec() == TQDialog::Accepted)
    {
        return addNonKDEAppButton(dlg.title(), dlg.description(),
                                  dlg.command(), dlg.iconPath(),
                                  dlg.commandLine(),
                                  dlg.useTerminal());
    }

    return 0;
}

const TQWidget* ContainerArea::addNonKDEAppButton(const TQString &name,
                                                 const TQString &description,
                                                 const TQString& filePath,
                                                 const TQString &icon,
                                                 const TQString &cmdLine,
                                                 bool inTerm)
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer *b = new NonKDEAppButtonContainer(name,
                                                    description,
                                                    filePath, icon,
                                                    cmdLine, inTerm,
                                                    m_opMenu, m_contents);
    completeContainerAddition(b);
    return b;
}

const TQWidget* ContainerArea::addExtensionButton(const TQString& df)
{
    if (!canAddContainers())
    {
        return 0;
    }

    BaseContainer* b = new ExtensionButtonContainer(df,
                                                    m_opMenu,
                                                    m_contents);
    completeContainerAddition(b);
    return b;
}

void ContainerArea::completeContainerAddition(BaseContainer* container,
                                              int index)
{
    //FIXME: the original comment was:
    //       Set freespace to one since the container will be added at the end.
    //       yet this is not always true =/
    container->setFreeSpace(1);
    addContainer(container, true, index);
    scrollTo(container);
    saveContainerConfig();
}

AppletContainer* ContainerArea::addApplet(const AppletInfo& info,
                                          bool isImmutable,
                                          int insertionIndex)
{
    if (!canAddContainers())
    {
        return 0;
    }

    AppletContainer* a = PluginManager::the()->createAppletContainer(
        info.desktopFile(),
        false,         // not startup
        TQString::null, // no config
        m_opMenu,
        m_contents,
        isImmutable);

    if (!a || !a->isValid())
    {
        delete a;
        return 0;
    }

    completeContainerAddition(a, insertionIndex);
    return a;
}

void ContainerArea::addContainer(BaseContainer* a, bool arrange, int index)
{
    if (!a)
    {
        return;
    }

    if (a->appletId().isNull())
    {
        a->setAppletId(createUniqueId(a->appletType()));
    }

    m_containers.append(a);

    if (arrange)
    {
        TQWidget* w = m_layout->widgetAt(index);
        TQPoint oldInsertionPoint = Kicker::the()->insertionPoint();
        if (w)
        {
            // let's set the insertion point to where the widget asked to be
            // put in front of is
            Kicker::the()->setInsertionPoint(w->geometry().topLeft());
        }

        if (Kicker::the()->insertionPoint().isNull())
        {
            m_layout->insertIntoFreeSpace(a, TQPoint());
        }
        else
        {
            m_layout->insertIntoFreeSpace(a, mapFromGlobal(Kicker::the()->insertionPoint()));
        }

        if (w)
        {
            Kicker::the()->setInsertionPoint(oldInsertionPoint);
        }
    }
    else
    {
        m_layout->add(a);
    }

    connect(a, TQT_SIGNAL(moveme(BaseContainer*)),
            TQT_SLOT(startContainerMove(BaseContainer*)));
    connect(a, TQT_SIGNAL(removeme(BaseContainer*)),
            TQT_SLOT(removeContainer(BaseContainer*)));
    connect(a, TQT_SIGNAL(takeme(BaseContainer*)),
            TQT_SLOT(takeContainer(BaseContainer*)));
    connect(a, TQT_SIGNAL(requestSave()),
            TQT_SLOT(slotSaveContainerConfig()));
    connect(a, TQT_SIGNAL(maintainFocus(bool)),
            this, TQT_SIGNAL(maintainFocus(bool)));

    if (dynamic_cast<AppletContainer*>(a))
    {
        connect(a, TQT_SIGNAL(updateLayout()), TQT_SLOT(resizeContents()));
    }

    a->configure(orientation(), popupDirection());
    a->show();
    resizeContents();
}

bool ContainerArea::removeContainer(BaseContainer *a)
{
    if (!a || isImmutable() || a->isImmutable())
    {
        return false;
    }

    a->slotRemoved(_config);
    m_containers.remove(a);
    m_layout->remove(a);
    a->deleteLater();
    saveContainerConfig(true);
    resizeContents();
    return true;
}

bool ContainerArea::removeContainer(int index)
{
    if (isImmutable())
    {
        return false;
    }

    BaseContainer* a = dynamic_cast<BaseContainer*>(m_layout->widgetAt(index));
    if (!a || a->isImmutable())
    {
        return false;
    }

    a->slotRemoved(_config);
    m_containers.remove(a);
    m_layout->remove(a);
    a->deleteLater();
    saveContainerConfig(true);
    resizeContents();
    return true;
}

void ContainerArea::removeContainers(BaseContainer::List containers)
{
    if (isImmutable())
    {
        return;
    }

    m_layout->setEnabled(false);

    for (BaseContainer::List::const_iterator it = containers.constBegin();
         it != containers.constEnd();
         ++it)
    {
        BaseContainer* a = *it;
        if (a->isImmutable())
        {
            continue;
        }

        a->slotRemoved(_config);
        m_containers.remove(a);
        m_layout->remove(a);
        a->deleteLater();
    }

    m_layout->setEnabled(true);
    saveContainerConfig(true);
    resizeContents();
}

void ContainerArea::takeContainer(BaseContainer* a)
{
    if (!a)
    {
        return;
    }

    disconnect(a, TQT_SIGNAL(moveme(BaseContainer*)),
               this, TQT_SLOT(startContainerMove(BaseContainer*)));
    disconnect(a, TQT_SIGNAL(removeme(BaseContainer*)),
               this, TQT_SLOT(removeContainer(BaseContainer*)));
    disconnect(a, TQT_SIGNAL(takeme(BaseContainer*)),
               this, TQT_SLOT(takeContainer(BaseContainer*)));
    disconnect(a, TQT_SIGNAL(requestSave()),
               this, TQT_SLOT(slotSaveContainerConfig()));
    disconnect(a, TQT_SIGNAL(maintainFocus(bool)),
               this, TQT_SIGNAL(maintainFocus(bool)));

    // Just remove the group from our own config file. Leave separate config
    // files untouched.
    _config->deleteGroup(a->appletId().latin1());
    _config->sync();
    m_containers.remove(a);
    m_layout->remove(a);
    saveContainerConfig(true);
    resizeContents();
}

void ContainerArea::resizeContents()
{
    int w = width();
    int h = height();

    if (orientation() == Qt::Horizontal)
    {
        int newWidth = m_layout->widthForHeight(h);
        if (newWidth > w)
        {
            resizeContents(newWidth, h);
        }
        else
        {
            resizeContents(w, h);
        }
    }
    else
    {
        int newHeight = m_layout->heightForWidth(w);

        if (newHeight > h)
        {
            resizeContents(w, newHeight);
        }
        else
        {
            resizeContents(w, h);
        }
    }
}

TQString ContainerArea::createUniqueId(const TQString& appletType) const
{
    TQString idBase = appletType + "_%1";
    TQString newId;
    int i = 0;
    bool unique = false;

    while (!unique)
    {
        i++;
        newId = idBase.arg(i);

        unique = true;
        for (BaseContainer::ConstIterator it = m_containers.begin();
             it != m_containers.end();
             ++it)
        {
            BaseContainer* b = *it;
            if (b->appletId() == newId)
            {
                unique = false;
                break;
            }
        }
    }

    return newId;
}

bool ContainerArea::canAddContainers() const
{
    return m_canAddContainers && Kicker::the()->canAddContainers();
}

void ContainerArea::startContainerMove(BaseContainer *a)
{
    if (!a || isImmutable())
    {
        return;
    }

    _moveAC = a;

    KickerTip::enableTipping(false);
    emit maintainFocus(true);
    setMouseTracking(true);
    grabMouse(tqsizeAllCursor);

    m_layout->setStretchEnabled(false);
    a->raise();
}

void ContainerArea::mouseReleaseEvent(TQMouseEvent *)
{
    if (!_moveAC)
    {
        return;
    }

    // start container move was caled successfuly
    // so we need to complete the move here
    _autoScrollTimer.stop();
    releaseMouse();
    setCursor(tqarrowCursor);
    setMouseTracking(false);

    _moveAC->completeMoveOperation();
    KickerTip::enableTipping(true);

    _moveAC = 0;

    emit maintainFocus(false);
    m_layout->setStretchEnabled(true);
    updateContainersBackground();
    saveContainerConfig(true);
}

void ContainerArea::mouseMoveEvent(TQMouseEvent *ev)
{
    if (!_moveAC)
    {
        Panner::mouseMoveEvent(ev);
        return;
    }

    if (ev->state() == Qt::LeftButton && !TQT_TQRECT_OBJECT(rect()).contains(ev->pos()))
    {
        // leaveEvent() doesn't work, while grabbing the mouse
        _autoScrollTimer.stop();
        releaseMouse();
        setCursor(tqarrowCursor);
        setMouseTracking(false);

        _moveAC->completeMoveOperation();
        KickerTip::enableTipping(true);

        emit maintainFocus(false);
        m_layout->setStretchEnabled(true);
        updateContainersBackground();
        saveContainerConfig(true);

        PanelDrag *dd = new PanelDrag(_moveAC, this);
        dd->setPixmap(kapp->iconLoader()->loadIcon(_moveAC->icon(), KIcon::Small));
        grabKeyboard();
        dd->drag();
        releaseKeyboard();
        return;
    }

    if (orientation() == Qt::Horizontal)
    {
        int oldX = _moveAC->x() + _moveAC->moveOffset().x();
        int x = ev->pos().x() + contentsX();
        if (ev->state() & ShiftButton)
        {
            m_layout->moveContainerPush(_moveAC, x - oldX);
        }
        else
        {
            m_layout->moveContainerSwitch(_moveAC, x - oldX);
            /* FIXME: Scrolling when the container moves out of the viewport
            bool scroll = false;
            if (rtl)
                if (newPos - 80 <= 0)
                    scroll = true;
            else
                if (newPos + 80 >= (horizontal ? geometry().width()  - moving->geometry().width()
                                               : geometry().height() - moving->geometry().height()))
                    scroll = true;
            [...]
            if (scroll) {
                if (!_autoScrollTimer.isActive())
                    _autoScrollTimer.start(50);

                if (horizontal)
                    scrollBy(dir*10, 0);
                else
                    scrollBy(0, dir*10);
            }
            */
        }
    }
    else
    {
        int oldY = _moveAC->y() + _moveAC->moveOffset().y();
        int y = ev->pos().y() + contentsY();
        if (ev->state() & ShiftButton)
        {
            m_layout->moveContainerPush(_moveAC, y - oldY);
        }
        else
        {
            m_layout->moveContainerSwitch(_moveAC, y - oldY);
            // TODO: Scrolling
        }
    }

    ensureVisible(ev->pos().x() + contentsX(), ev->pos().y() + contentsY());
    updateContainersBackground();
}

int ContainerArea::position() const
{
    return static_cast<int>(_pos);
}

KPanelApplet::Direction ContainerArea::popupDirection() const
{
    return KickerLib::positionToDirection(_pos);
}

bool ContainerArea::isImmutable() const
{
    return m_immutable || Kicker::the()->isImmutable();
}

void ContainerArea::dragEnterEvent(TQDragEnterEvent *ev)
{
    bool canAccept = !isImmutable() &&
                     (PanelDrag::canDecode(ev) ||
                      AppletInfoDrag::canDecode(ev) ||
                      KURLDrag::canDecode(ev));
    ev->accept(canAccept);

    if (!canAccept)
    {
        return;
    }

    m_layout->setStretchEnabled(false);

    if (!_dragIndicator)
    {
        _dragIndicator = new DragIndicator(m_contents);
    }

    BaseContainer *draggedContainer = 0;
    int preferedWidth = height();
    int preferedHeight = width();
    if (PanelDrag::decode(ev, &draggedContainer))
    {
        preferedWidth = draggedContainer->widthForHeight(height());
        preferedHeight = draggedContainer->heightForWidth(width());
    }

    if (orientation() == Qt::Horizontal)
    {
        _dragIndicator->setPreferredSize(TQSize(preferedWidth, height()));
    }
    else
    {
        _dragIndicator->setPreferredSize(TQSize(width(), preferedHeight));
    }
    _dragMoveOffset = TQPoint(_dragIndicator->width()/2,
                             _dragIndicator->height()/2);

    // Find the container before the position of the dragindicator.
    BaseContainer::Iterator it = m_containers.end();

    if (it != m_containers.begin())
    {
        do
        {
            --it;
            BaseContainer* a = *it;

            if ((orientation() == Qt::Horizontal &&
                 a->x() < (ev->pos().x() + contentsX()) - _dragMoveOffset.x()) ||
                (orientation() == Qt::Vertical &&
                 a->y() < (ev->pos().y() + contentsY()) - _dragMoveOffset.y()))
            {
                _dragMoveAC = a;
                break;
            }
        } while (it != m_containers.begin());
    }

    if (orientation() == Qt::Horizontal)
    {
        moveDragIndicator(ev->pos().x() + contentsX() - _dragMoveOffset.x());
    }
    else
    {
        moveDragIndicator(ev->pos().y() + contentsY() - _dragMoveOffset.y());
    }

    _dragIndicator->show();
}

void ContainerArea::dragMoveEvent(TQDragMoveEvent* ev)
{
    if (ev->source() == this)
    {
        // Abort the drag and go back to container sliding.
        // Actually, this should be placed in dragEnterEvent(), but
        // then it does work only on every second event.

        // Cancel the drag by faking an Escape keystroke.
        TQKeyEvent fakedKeyPress(TQEvent::KeyPress, Key_Escape, 0, 0);
        TQKeyEvent fakedKeyRelease(TQEvent::KeyRelease, Key_Escape, 0, 0);
        TQApplication::sendEvent(this, &fakedKeyPress);
        TQApplication::sendEvent(this, &fakedKeyRelease);
        tqApp->processEvents();
        startContainerMove(_moveAC);

        // Align the container to the mouse position.
        if (orientation() == Qt::Horizontal)
        {
            m_layout->moveContainerSwitch(_moveAC, ev->pos().x() + contentsX() - _moveAC->x());
        }
        else
        {
            m_layout->moveContainerSwitch(_moveAC, ev->pos().y() + contentsY() - _moveAC->y());
        }
        return;
    }

    if (!_dragIndicator)
    {
        return;
    }

    if (orientation() == Qt::Horizontal)
    {
        moveDragIndicator(ev->pos().x() + contentsX() - _dragMoveOffset.x());
    }
    else
    {
        moveDragIndicator(ev->pos().y() + contentsY() - _dragMoveOffset.y());
    }
}

void ContainerArea::dragLeaveEvent(TQDragLeaveEvent*)
{
    if (_dragIndicator)
    {
        _dragIndicator->hide();
    }
    m_layout->setStretchEnabled(true);
    _dragMoveAC = 0;
}

void ContainerArea::dropEvent(TQDropEvent *ev)
{
    if (!_dragIndicator)
    {
        // we assume that this is the result of a successful drag enter
        // which means we'll have a _dragIndicator. if for
        // some reason we don't, let's not go down this code path
        return;
    }

    BaseContainer *a = 0;
    if (PanelDrag::decode(ev, &a))
    {
        if (!a)
        {
            _dragMoveAC = 0;
            _dragIndicator->hide();
            m_layout->setStretchEnabled(true);
            return;
        }

        TQObject *parent = ev->source() ? ev->source()->parent() : 0;
        while (parent && (TQT_BASE_OBJECT(parent) != TQT_BASE_OBJECT(this)))
        {
            parent = parent->parent();
        }

        if (parent)
        {
            // Move container a
            if (orientation() == Qt::Horizontal)
            {
                int oldX = a->x();
                int x = _dragIndicator->x();
                m_layout->moveContainerSwitch(a, x - oldX);
            }
            else if (orientation() == Qt::Vertical)
            {
                int oldY = a->y();
                int y = _dragIndicator->y();
                m_layout->moveContainerSwitch(a, y - oldY);
            }

            _dragMoveAC = 0;
            _dragIndicator->hide();
            m_layout->setEnabled(true);
            m_layout->setStretchEnabled(true);
            saveContainerConfig(true);
            return;
        }

        // it came from another panel
        Kicker::the()->setInsertionPoint(_dragIndicator->pos());
        a->reparent(m_contents, 0, _dragIndicator->pos(), true);
        a->setAppletId(createUniqueId(a->appletType()));
        addContainer(a, true);
        Kicker::the()->setInsertionPoint(TQPoint());
        m_layout->updateFreeSpaceValues();
        _dragMoveAC = 0;
        _dragIndicator->hide();
        m_layout->setStretchEnabled(true);
        saveContainerConfig();
        return;
    }

    // is it an applet info?
    AppletInfo info;
    if (AppletInfoDrag::decode(ev, info))
    {
        Kicker::the()->setInsertionPoint(_dragIndicator->pos());
        _dragIndicator->hide();
        m_layout->setStretchEnabled(true);

        if (info.type() & AppletInfo::Button)
        {
            addButton(info);
        }
        else if (info.type() == AppletInfo::Applet)
        {
           addApplet(info);
        }

        Kicker::the()->setInsertionPoint(TQPoint());
        return;
    }

    // ok, let's try a KURL drag
    KURL::List uriList;
    if (!KURLDrag::decode(ev, uriList))
    {
        _dragMoveAC = 0;
        _dragIndicator->hide();
        m_layout->setStretchEnabled(true);
        return;
    }

    Kicker::the()->setInsertionPoint(_dragIndicator->pos());

    KURL::List::ConstIterator it(uriList.begin());
    for (; it != uriList.end(); ++it)
    {
        const KURL &url = *it;

        // Create a new PanelButton for this URL.

        // see if it's a executable or directory
        if (url.protocol() == "programs")
        {
            TQString relPath = url.path();
            if (relPath[0] == '/')
            {
                relPath = relPath.right(relPath.length() - 1);
            }
            a = new ServiceMenuButtonContainer(relPath, m_opMenu, m_contents);
        }
        else if (url.isLocalFile())
        {
            TQFileInfo fi(url.path());
            TQString foundMimeName = KMimeType::findByURL(url)->name();
            if (fi.isDir())
            { // directory
                switch (PanelDirDropMenu().exec(mapToGlobal(ev->pos())))
                {
                    case PanelDirDropMenu::Browser:
                        a = new BrowserButtonContainer(url.path(), m_opMenu,
                                    KMimeType::iconForURL(url), m_contents);
                        break;
                    case PanelDirDropMenu::Url:
                        a = new URLButtonContainer(url.url(), m_opMenu, m_contents);
                        break;
                    default: ;
                }
            }
            else if ( (foundMimeName == "application/x-desktop")
                   || (foundMimeName == "media/builtin-mydocuments")
                   || (foundMimeName == "media/builtin-mycomputer")
                   || (foundMimeName == "media/builtin-mynetworkplaces")
                   || (foundMimeName == "media/builtin-printers")
                   || (foundMimeName == "media/builtin-trash")
                   || (foundMimeName == "media/builtin-webbrowser") )
            {
                // a local desktop file being dragged from an external program.
                // Make a copy first.
                KDesktopFile df(url.path());
                KURL newUrl;
                newUrl.setPath(KickerLib::copyDesktopFile(url));
                if (df.readType() == "Link")
                    a = new URLButtonContainer(newUrl.url(), m_opMenu, m_contents);
                else
                    a = new ServiceButtonContainer(newUrl.path(), m_opMenu, m_contents);
            }
            else if (fi.isExecutable())
            {
                // non-KDE executable
                TQString pixmapFile;
                KMimeType::pixmapForURL(url, 0, KIcon::Panel, 0,
                                        KIcon::DefaultState, &pixmapFile);
                PanelExeDialog dlg(TQString::null, TQString::null, url.path(),
                                   pixmapFile, TQString::null, false, 0);
                if (dlg.exec() == TQDialog::Accepted)
                {
                    // KIconloader returns a full path, we only want the name
                    TQFileInfo iconfi(dlg.iconPath());
                    a = new NonKDEAppButtonContainer(dlg.title(),
                                                     dlg.description(),
                                                     dlg.command(),
                                                     iconfi.fileName(),
                                                     dlg.commandLine(),
                                                     dlg.useTerminal(),
                                                     m_opMenu,
                                                     m_contents);
                }
            }
            else // some unknown local file
            {
                a = new URLButtonContainer(url.url(), m_opMenu, m_contents);
            }
        }
        else // a internet URL
        {
            a = new URLButtonContainer(url.url(), m_opMenu, m_contents);
        }

        if (!a)
        {
            _dragIndicator->hide();
            Kicker::the()->setInsertionPoint(TQPoint());
            m_layout->setStretchEnabled(true);
            return;
        }

        addContainer(a, true);
        m_layout->updateFreeSpaceValues();
    }

    saveContainerConfig();
    _dragMoveAC = 0;
    _dragIndicator->hide();
    Kicker::the()->setInsertionPoint(TQPoint());
    m_layout->setStretchEnabled(true);
}

bool ContainerArea::eventFilter(TQObject* o, TQEvent* e)
{
    // Propagate the layout hints which m_contents receives. This way widgets
    // which contain a ContainerArea can react to layout changes of its
    // contents. For example: If an applets grows, the top level widget may
    // want to grow as well.
    if (TQT_BASE_OBJECT(o) == TQT_BASE_OBJECT(m_contents))
    {
        if (e->type() == TQEvent::LayoutHint)
        {
            updateGeometry(); // Posts a new layout hint to our parent.
        }
        return false;
    }

    return Panner::eventFilter(o, e);
}

void ContainerArea::resizeEvent(TQResizeEvent *ev)
{
    Panner::resizeEvent(ev);
    setBackground();
}

void ContainerArea::viewportResizeEvent(TQResizeEvent* ev)
{
    Panner::viewportResizeEvent(ev);
    if (orientation() == Qt::Horizontal)
    {
        m_contents->resize(kMax(widthForHeight(ev->size().height()),
                                ev->size().width()),
                           ev->size().height());
    }
    else
    {
        m_contents->resize(ev->size().width(),
                           kMax(heightForWidth(ev->size().width()),
                                ev->size().height()));
    }
    resizeContents(m_contents->width(), m_contents->height());
}

void ContainerArea::setBackground()
{
    _bgSet = false;
    m_cachedGeometry.clear();

    if (KickerSettings::transparent() &&
        (KickerSettings::menubarPanelTransparent() ||
        !ExtensionManager::the()->isMenuBar(topLevelWidget())))
    {
        if (!_rootPixmap)
        {
            _rootPixmap = new KRootPixmap(this);
            _rootPixmap->setCustomPainting(true);
            connect(_rootPixmap, TQT_SIGNAL(backgroundUpdated(const TQPixmap&)),
                    TQT_SLOT(updateBackground(const TQPixmap&)));
        }
        else
        {
            _rootPixmap->repaint(true);
        }

        double tint = double(KickerSettings::tintValue()) / 100;
        _rootPixmap->setFadeEffect(tint, KickerSettings::tintColor());
        if (KickerSettings::menubarPanelBlurred()) {
            _rootPixmap->setBlurEffect(0.0, 4.0);
        }
        else {
            _rootPixmap->setBlurEffect(0.0, 0.0);
        }
        _rootPixmap->start();
        _bgSet = true;
        return;
    }
    else if (_rootPixmap)
    {
        delete _rootPixmap;
        _rootPixmap = 0;
    }

    unsetPalette();

    if (KickerSettings::useBackgroundTheme())
    {
        // by keeping the src image static, we can share it among panels and only
        // reload from disk when it actually changes in the config, not every time we
        // get a resize or configure event
        static TQString bgStr;
        static TQImage srcImage;
        TQString newBgStr = locate("appdata", KickerSettings::backgroundTheme());

        if (bgStr != newBgStr)
        {
            bgStr = newBgStr;
            srcImage.load(bgStr);
        }

        if (srcImage.isNull())
        {
            KickerSettings::setUseBackgroundTheme(false);
        }
        else
        {
            TQImage bgImage = srcImage;

            if (orientation() == Qt::Vertical)
            {
                if (KickerSettings::rotateBackground())
                {
                    TQWMatrix matrix;
                    matrix.rotate(position() == KPanelExtension::Left ? 90: 270);
                    bgImage = bgImage.xForm(matrix);
                }

                bgImage = bgImage.scaleWidth( size().width() );
            }
            else
            {
                if (position() == KPanelExtension::Top &&
                    KickerSettings::rotateBackground())
                {
                    TQWMatrix matrix;
                    matrix.rotate(180);
                    bgImage = bgImage.xForm(matrix);
                }

                bgImage = bgImage.scaleHeight( size().height() );
            }

            if (KickerSettings::colorizeBackground())
            {
                KickerLib::colorize(bgImage);
            }
            setPaletteBackgroundPixmap(TQPixmap(bgImage));
            TQTimer::singleShot(0, this, TQT_SLOT(updateContainersBackground()));
        }
    }

    _bgSet = true;
}

void ContainerArea::immutabilityChanged(bool immutable)
{
    // we set all the child container's immutability here instead of connecting
    // the immutabilityChanged signal up everywhere so that we can control the
    // order of immutability changing and the background being updated. since
    // immutability implies applet handle visibility, those things must happen
    // first before updating our background.
    for (BaseContainer::ConstIterator it = m_containers.constBegin();
         it != m_containers.constEnd();
         ++it)
    {
        (*it)->setImmutable(immutable);
    }

    setAcceptDrops(!isImmutable());
    TQTimer::singleShot(0, this, TQT_SLOT(setBackground()));
}

TQRect ContainerArea::availableSpaceFollowing(BaseContainer* a)
{
    TQRect availableSpace = rect();
    BaseContainer* b = 0;

    if (a)
    {
        BaseContainer::Iterator it = m_containers.find(a);
        if (it != m_containers.end() &&
            ++it != m_containers.end())
        {
            b = (*it);
        }
    }

    if (!b)
    {
        BaseContainer::Iterator it = m_containers.begin();
        if (it != m_containers.end())
        {
            b = (*it);
        }
    }

    if (orientation() == Qt::Horizontal)
    {
        if (a)
        {
            availableSpace.setLeft(a->x() + a->width());
        }

        if (b)
        {
            availableSpace.setRight(b->x() - 1);
        }
    }
    else
    {
        if (a)
        {
            availableSpace.setTop(a->y() + a->height());
        }

        if (b)
        {
            availableSpace.setBottom(b->y() - 1);
        }
    }

    return availableSpace;
}

void ContainerArea::moveDragIndicator(int pos)
{
    TQRect availableSpace = availableSpaceFollowing(_dragMoveAC);

    // Move _dragIndicator to position pos, restricted by availableSpace.
    // Resize _dragIndicator if necessary.
    if (orientation() == Qt::Horizontal)
    {
        if (availableSpace.size().width() <
            _dragIndicator->preferredSize().width())
        {
            _dragIndicator->resize(availableSpace.size());
            _dragIndicator->move(availableSpace.topLeft());
        }
        else
        {
            int newX = pos;
            _dragIndicator->resize(_dragIndicator->preferredSize());
            newX = QMAX(newX, availableSpace.left());
            newX = QMIN(newX,
                availableSpace.right() + 1 - _dragIndicator->width() );
            _dragIndicator->move(newX, availableSpace.top());
        }
    }
    else
    {
        if (availableSpace.size().height() <
            _dragIndicator->preferredSize().height())
        {
            _dragIndicator->resize(availableSpace.size());
            _dragIndicator->move(availableSpace.topLeft());
        }
        else
        {
            int newY = pos;
            _dragIndicator->resize(_dragIndicator->preferredSize());
            newY = QMAX(newY, availableSpace.top());
            newY = QMIN(newY,
                availableSpace.bottom() + 1 - _dragIndicator->height() );
            _dragIndicator->move(availableSpace.left(), newY);
        }
    }
}

void ContainerArea::updateBackground( const TQPixmap& pm )
{
    TQBrush bgBrush(colorGroup().background(), pm);
    TQPalette pal = kapp->palette();
    pal.setBrush(TQColorGroup::Background, bgBrush);
    setPalette(pal);

    // because the Pixmap can be smaller as the containerarea
    // we construct a pixmap the same size as we are that every
    // applet or button can use to cut out its background
    _completeBg.resize(width(), height());
    _completeBg.fill(this, 0, 0);

    m_cachedGeometry.clear();
    updateContainersBackground();
}

void ContainerArea::resizeContents(int w, int h)
{
    // this looks silly but is required otherwise (some?) c++ compilers can't see
    // Panner::resizeContents(int, int) due to the overloaded ContainerArea::resizeContents()
    Panner::resizeContents(w, h);

    if (!m_updateBackgroundsCalled)
    {
        m_updateBackgroundsCalled = true;
        TQTimer::singleShot(0, this, TQT_SLOT(updateContainersBackground()));
    }
}

void ContainerArea::slotSaveContainerConfig()
{
    saveContainerConfig();
}

void ContainerArea::setPosition(KPanelExtension::Position p)
{
    if (_pos == p)
    {
        return;
    }

    _pos = p;
    Qt::Orientation o = (p == KPanelExtension::Top ||
                         p == KPanelExtension::Bottom) ?
                        Qt::Horizontal : Qt::Vertical;
    bool orientationChanged = (orientation() != o);
    m_layout->setEnabled(false);

    if (orientationChanged)
    {
        setOrientation(o);
        m_layout->setOrientation(o);

        // when we change orientation, we will resize the "width"
        // component down to 0, forcing a resize in resizeContents()
        // when that gets called AFTER we've been moved
        // it's not always safe to do the resize here, as scroll buttons
        // from the panner may get in our way. =/
        if (o == Qt::Horizontal)
        {
            resizeContents(0, height());
        }
        else
        {
            resizeContents(width(), 0);
        }
    }

    for (BaseContainer::ConstIterator it = m_containers.constBegin();
         it != m_containers.constEnd();
         ++it)
    {
        if (orientationChanged)
        {
            (*it)->setOrientation(o);
        }

        (*it)->setPopupDirection( popupDirection() );
    }

    m_layout->setEnabled(true);

    setContentsPos(0, 0);
    m_contents->move(0, 0);
    setBackground();

    // container extension repaints for us!
    //repaint();
}

void ContainerArea::setAlignment(KPanelExtension::Alignment a)
{
    for (BaseContainer::ConstIterator it = m_containers.begin();
         it != m_containers.end();
         ++it)
    {
        (*it)->setAlignment(a);
    }
}

void ContainerArea::autoScroll()
{
    if(!_moveAC) return;

    if(orientation() == Qt::Horizontal) {
        if(_moveAC->pos().x() <= 80)
            scrollBy(-10, 0);
        else if(_moveAC->pos().x() >= width() - _moveAC->width() - 80)
            scrollBy(10, 0);
    }
    else {
        if(_moveAC->pos().y() <= 80)
            scrollBy(0, -10);
        else if(_moveAC->pos().y() >= height() - _moveAC->height() - 80)
            scrollBy(0, 10);
    }
}

void ContainerArea::scrollTo(BaseContainer* b)
{
    if (!b)
    {
        return;
    }

    int x, y;
    viewportToContents(b->pos().x(), b->pos().y(), x, y);
    ensureVisible(x, y);
}

void ContainerArea::updateContainersBackground()
{
    m_updateBackgroundsCalled = false;

    if (!_bgSet)
    {
        return;
    }

    for (BaseContainer::ConstIterator it = m_containers.constBegin();
         it != m_containers.constEnd();
         ++it)
    {
    // A rather ugly hack. The code calls updateContainersBackground() all over
    // the place even when nothing in fact has changed. Updating the background
    // on every single unrelated change however means that e.g. the systray
    // flickers when a new window is opened/closed (because the taskbar is relayouted,
    // which triggers updateContainersBackground() even though the geometry
    // of the taskbar does not change in fact. I'm apparently unable to fix this
    // properly, so just cache the geometry and update background only when
    // the geometry changes or when the background really changes (in which
    // case the cached is cleared).
        if( !m_cachedGeometry.contains( *it ))
        {
            m_cachedGeometry[ *it ] = TQRect();
            connect( *it, TQT_SIGNAL( destroyed()), TQT_SLOT( destroyCachedGeometry()));
        }
        if( m_cachedGeometry[ *it ] != (*it)->geometry())
        {
            (*it)->setBackground();
            m_cachedGeometry[ *it ] = (*it)->geometry();
        }
    }
}

void ContainerArea::destroyCachedGeometry()
{
    m_cachedGeometry.remove(const_cast<TQWidget*>(static_cast<const TQWidget*>(sender())));
}

BaseContainer::List ContainerArea::containers(const TQString& type) const
{
    if (type.isEmpty() || type == "All")
    {
        return m_containers;
    }

    BaseContainer::List list;

    if (type == "Special Button")
    {
        for (BaseContainer::ConstIterator it = m_containers.constBegin();
             it != m_containers.constEnd();
             ++it)
        {
            TQString type = (*it)->appletType();
            if (type == "KMenuButton" ||
                type == "WindowListButton" ||
                type == "BookmarksButton" ||
                type == "DesktopButton" ||
                type == "BrowserButton" ||
                type == "ExecButton" ||
                type == "ExtensionButton")
            {
                list.append(*it);
            }
        }

        return list;
    }

    for (BaseContainer::ConstIterator it = m_containers.constBegin();
         it != m_containers.constEnd();
         ++it)
    {
        if ((*it)->appletType() == type)
        {
            list.append(*it);
        }
    }

    return list;
}

int ContainerArea::containerCount(const TQString& type) const
{
    if (type.isEmpty() || type == "All")
    {
        return m_containers.count();
    }

    int count = 0;
    if (type == "Special Button")
    {
        for (BaseContainer::ConstIterator it = m_containers.begin();
             it != m_containers.end();
             ++it)
        {
            TQString type = (*it)->appletType();
            if (type == "KMenuButton" ||
                type == "WindowListButton" ||
                type == "BookmarksButton" ||
                type == "DesktopButton" ||
                type == "BrowserButton" ||
                type == "ExecButton" ||
                type == "ExtensionButton")
            {
                ++count;
            }
        }

        return count;
    }

    for (BaseContainer::ConstIterator it = m_containers.begin();
         it != m_containers.end();
         ++it)
    {
        if ((*it)->appletType() == type)
        {
            ++count;
        }
    }

    return count;
}

TQStringList ContainerArea::listContainers() const
{
    return m_layout->listItems();
}

void ContainerArea::repaint()
{
    Panner::repaint();
}

void ContainerArea::showAddAppletDialog()
{
    if (!m_addAppletDialog)
    {
        m_addAppletDialog = new AddAppletDialog(this, this, 0);
        connect(m_addAppletDialog, TQT_SIGNAL(finished()), this, TQT_SLOT(addAppletDialogDone()));
    }
    else
    {
        // this ensures that if we get shown again via the menu
        // that the dialog picks up
        // the new place to insert things
        m_addAppletDialog->updateInsertionPoint();
    }

    KWin::setOnDesktop(m_addAppletDialog->winId(), KWin::currentDesktop());
    m_addAppletDialog->show();
    m_addAppletDialog->raise();
}

void ContainerArea::addAppletDialogDone()
{
    m_addAppletDialog->deleteLater();
    m_addAppletDialog = 0;
}

const TQPixmap* ContainerArea::completeBackgroundPixmap() const
{
    return &_completeBg;
}

int ContainerArea::widthForHeight(int h) const
{
    return m_layout->widthForHeight(h);
}

int ContainerArea::heightForWidth(int w) const
{
    return m_layout->heightForWidth(w);
}


DragIndicator::DragIndicator(TQWidget* parent, const char* name)
    : TQWidget(parent, name)
{
    setBackgroundOrigin(AncestorOrigin);
}


void DragIndicator::paintEvent(TQPaintEvent*)
{
    TQPainter painter(this);
    TQRect rect(0, 0, width(), height());
    style().tqdrawPrimitive( TQStyle::PE_FocusRect, &painter, rect, colorGroup(),
                           TQStyle::Style_Default, colorGroup().base() );
}

void DragIndicator::mousePressEvent(TQMouseEvent*)
{
    hide();
}

#include "containerarea.moc"
