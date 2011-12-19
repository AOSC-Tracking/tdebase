/*****************************************************************

Copyright (c) 1996-2002 the kicker authors. See file AUTHORS.

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

#include <tqcursor.h>
#include <tqtimer.h>

#include <kapplication.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kservice.h>
#include <kservicegroup.h>
#include <kstandarddirs.h>
#include <ksycoca.h>
#include <kurl.h>
#include <kurldrag.h>

#include "global.h"
#include "kickerSettings.h"
#include "prefmenu.h"

K_EXPORT_KICKER_MENUEXT(prefmenu, PrefMenu)

const int idStart = 4242;

PrefMenu::PrefMenu(TQWidget *parent,
                   const char *name,
                   const TQStringList &/*args*/)
    : KPanelMenu(i18n("Settings"), parent, name),
      m_clearOnClose(false)
{
}

PrefMenu::PrefMenu(const TQString& label,
                   const TQString& root,
                   TQWidget *parent)
    : KPanelMenu(label, parent),
      m_clearOnClose(false),
      m_root(root)
{
    m_subMenus.setAutoDelete(true);

    connect(KSycoca::self(),  TQT_SIGNAL(databaseChanged()),
            this, TQT_SLOT(clearOnClose()));

    connect(this, TQT_SIGNAL(aboutToHide()),
            this, TQT_SLOT(aboutToClose()));
}

PrefMenu::~PrefMenu()
{
}

void PrefMenu::insertMenuItem(KService::Ptr& s,
                              int nId,
                              int nIndex,
                              const TQStringList *suppressGenericNames)
{
    TQString serviceName = s->name();
    // add comment
    TQString comment = s->genericName();
    if (!comment.isEmpty())
    {
        if (KickerSettings::menuEntryFormat() == KickerSettings::NameAndDescription)
        {
            if (!suppressGenericNames ||
                !suppressGenericNames->contains(s->untranslatedGenericName()))
            {
                serviceName = TQString("%1 (%2)").arg(serviceName).arg(comment);
            }
        }
        else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName)
        {
            serviceName = TQString("%1 (%2)").arg(comment).arg(serviceName);
        }
        else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly)
        {
            serviceName = comment;
        }
    }

    // restrict menu entries to a sane length
    if (serviceName.length() > 60)
    {
        serviceName.truncate(57);
        serviceName += "...";
    }

    // check for NoDisplay
    if (s->noDisplay())
    {
        return;
    }

    // ignore dotfiles.
    if ((serviceName.at(0) == '.'))
    {
        return;
    }

    // item names may contain ampersands. To avoid them being converted
    // to accelerators, replace them with two ampersands.
    serviceName.replace("&", "&&");

    int newId = insertItem(KickerLib::menuIconSet(s->icon()), serviceName, nId, nIndex);
    m_entryMap.insert(newId, static_cast<KSycocaEntry*>(s));
}

void PrefMenu::mousePressEvent(TQMouseEvent * ev)
{
    m_dragStartPos = ev->pos();
    KPanelMenu::mousePressEvent(ev);
}

void PrefMenu::mouseMoveEvent(TQMouseEvent * ev)
{
    KPanelMenu::mouseMoveEvent(ev);

    if ((ev->state() & Qt::LeftButton) != Qt::LeftButton)
    {
        return;
    }

    TQPoint p = ev->pos() - m_dragStartPos;
    if (p.manhattanLength() <= TQApplication::startDragDistance())
    {
        return;
    }

    int id = idAt(m_dragStartPos);

    // Don't drag items we didn't create.
    if (id < idStart)
    {
        return;
    }

    if (!m_entryMap.contains(id))
    {
        kdDebug(1210) << "Cannot find service with menu id " << id << endl;
        return;
    }

    KSycocaEntry * e = m_entryMap[id];

    TQPixmap icon;
    KURL url;

    switch (e->sycocaType())
    {
        case KST_KService:
        {
            icon = static_cast<KService *>(e)->pixmap(KIcon::Small);
            TQString filePath = static_cast<KService *>(e)->desktopEntryPath();
            if (filePath[0] != '/')
            {
                filePath = locate("apps", filePath);
            }
            url.setPath(filePath);
            break;
        }

        case KST_KServiceGroup:
        {
            icon = KGlobal::iconLoader()->loadIcon(static_cast<KServiceGroup*>(e)->icon(),
                                                   KIcon::Small);
            url = "programs:/" + static_cast<KServiceGroup*>(e)->relPath();
            break;
        }

        default:
        {
            return;
            break;
        }
    }

    // If the path to the desktop file is relative, try to get the full
    // path from KStdDirs.
    KURLDrag *d = new KURLDrag(KURL::List(url), this);
    connect(d, TQT_SIGNAL(destroyed()), this, TQT_SLOT(dragObjectDestroyed()));
    d->setPixmap(icon);
    d->dragCopy();

    // Set the startposition outside the panel, so there is no drag initiated
    // when we use drag and click to select items. A drag is only initiated when
    // you click to open the menu, and then press and drag an item.
    m_dragStartPos = TQPoint(-1,-1);
}

void PrefMenu::dragEnterEvent(TQDragEnterEvent *event)
{
    // Set the DragObject's target to this widget. This is needed because the
    // widget doesn't accept drops, but we want to determine if the drag object
    // is dropped on it. This avoids closing on accidental drags. If this
    // widget accepts drops in the future, these lines can be removed.
    if (event->source() == this)
    {
        KURLDrag::setTarget(this);
    }
    event->ignore();
}

void PrefMenu::dragLeaveEvent(TQDragLeaveEvent */*event*/)
{
    // see PrefMenu::dragEnterEvent why this is nescessary
    if (!TQT_TQRECT_OBJECT(frameGeometry()).contains(TQCursor::pos()))
    {
        KURLDrag::setTarget(0);
    }
}

void PrefMenu::initialize()
{
    if (initialized())
    {
        return;
    }

    // Set the startposition outside the panel, so there is no drag initiated
    // when we use drag and click to select items. A drag is only initiated when
    // you click to open the menu, and then press and drag an item.
    m_dragStartPos = TQPoint(-1,-1);

    if (m_root.isEmpty())
    {
        insertItem(KickerLib::menuIconSet("kcontrol"),
                   i18n("Control Center"),
                   this, TQT_SLOT(launchControlCenter()));
        insertSeparator();
    }

    // We ask KSycoca to give us all services under Settings/
    KServiceGroup::Ptr root = KServiceGroup::group(m_root.isEmpty() ? "Settings/" : m_root);

    if (!root || !root->isValid())
    {
        return;
    }

    KServiceGroup::List list = root->entries(true, true, true,
                                             KickerSettings::menuEntryFormat() == KickerSettings:: NameAndDescription);

    if (list.isEmpty())
    {
        setItemEnabled(insertItem(i18n("No Entries")), false);
        return;
    }

    int id = idStart;

    TQStringList suppressGenericNames = root->suppressGenericNames();

    KServiceGroup::List::ConstIterator it = list.begin();
    for (; it != list.end(); ++it)
    {
        KSycocaEntry* e = *it;

        if (e->isType(KST_KServiceGroup))
        {

            KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
            TQString groupCaption = g->caption();

            // Avoid adding empty groups.
            KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(g->relPath());
            if (subMenuRoot->childCount() == 0)
            {
                continue;
            }

            // Ignore dotfiles.
            if ((g->name().at(0) == '.'))
            {
                continue;
            }

            // Item names may contain ampersands. To avoid them being converted
            // to accelerators, replace each ampersand with two ampersands.
            groupCaption.replace("&", "&&");

            PrefMenu* m = new PrefMenu(g->name(), g->relPath(), this);
            m->setCaption(groupCaption);

            int newId = insertItem(KickerLib::menuIconSet(g->icon()), groupCaption, m, id++);
            m_entryMap.insert(newId, static_cast<KSycocaEntry*>(g));
            // We have to delete the sub menu our selves! (See Qt docs.)
            m_subMenus.append(m);
        }
        else if (e->isType(KST_KService))
        {
            KService::Ptr s(static_cast<KService *>(e));
            insertMenuItem(s, id++, -1, &suppressGenericNames);
        }
        else if (e->isType(KST_KServiceSeparator))
        {
            insertSeparator();
        }
    }

    setInitialized(true);
}

void PrefMenu::slotExec(int id)
{
    if (!m_entryMap.contains(id))
    {
        return;
    }

    kapp->propagateSessionManager();
    KSycocaEntry *e = m_entryMap[id];
    KService::Ptr service = static_cast<KService *>(e);
    KApplication::startServiceByDesktopPath(service->desktopEntryPath(),
                                            TQStringList(), 0, 0, 0, "", true);
    m_dragStartPos = TQPoint(-1,-1);
}

void PrefMenu::clearOnClose()
{
    if (!initialized())
    {
        return;
    }

    m_clearOnClose = isVisible();
    if (!m_clearOnClose)
    {
        // we aren't visible right now so clear immediately
        slotClear();
    }
}

void PrefMenu::slotClear()
{
    if( isVisible())
    {
        // QPopupMenu's aboutToHide() is emitted before the popup is really hidden,
        // and also before a click in the menu is handled, so do the clearing
        // only after that has been handled
        TQTimer::singleShot( 100, this, TQT_SLOT( slotClear()));
        return;
    }

    m_entryMap.clear();
    KPanelMenu::slotClear();
    m_subMenus.clear();
}

void PrefMenu::aboutToClose()
{
    if (m_clearOnClose)
    {
        m_clearOnClose = false;
        slotClear();
    }
}

void PrefMenu::launchControlCenter()
{
    KApplication::startServiceByDesktopName("kcontrol", TQStringList(),
                                            0, 0, 0, "", true);
}


void PrefMenu::dragObjectDestroyed()
{
    if (KURLDrag::target() != this)
    {
        close();
    }
}

#include "prefmenu.moc"
