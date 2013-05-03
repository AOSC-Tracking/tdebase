/*****************************************************************

Copyright (c) 2000-2001 Matthias Ettrich <ettrich@kde.org>
              2000-2001 Matthias Elter   <elter@kde.org>
              2001      Carsten Pfeiffer <pfeiffer@kde.org>
              2001      Martijn Klingens <mklingens@yahoo.com>
              2004      Aaron J. Seigo   <aseigo@kde.org>
              2010      Timothy Pearson  <kb9vqf@pearsoncomputing.net>

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
#include <tqpopupmenu.h>
#include <tqtimer.h>
#include <tqpixmap.h>
#include <tqevent.h>
#include <tqstyle.h>
#include <tqgrid.h>
#include <tqpainter.h>
#include <tqimage.h>

#include <dcopclient.h>
#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <krun.h>
#include <kwinmodule.h>
#include <kdialogbase.h>
#include <kactionselector.h>
#include <kiconloader.h>
#include <kwin.h>

#include "kickerSettings.h"

#include "simplebutton.h"

#include "systemtrayapplet.h"
#include "systemtrayapplet.moc"

#include <X11/Xlib.h>

#define ICON_MARGIN 1
#define ICON_END_MARGIN KickerSettings::showDeepButtons()?4:0

extern "C"
{
    KDE_EXPORT KPanelApplet* init(TQWidget *parent, const TQString& configFile)
    {
        KGlobal::locale()->insertCatalogue("ksystemtrayapplet");
        return new SystemTrayApplet(configFile, KPanelApplet::Normal,
                                    KPanelApplet::Preferences, parent, "ksystemtrayapplet");
    }
}

SystemTrayApplet::SystemTrayApplet(const TQString& configFile, Type type, int actions,
                                   TQWidget *parent, const char *name)
  : KPanelApplet(configFile, type, actions, parent, name),
    m_showFrame(KickerSettings::showDeepButtons()?true:false),
    m_showHidden(false),
    m_expandButton(0),
    m_leftSpacer(0),
    m_rightSpacer(0),
    m_clockApplet(0),
    m_settingsDialog(0),
    m_iconSelector(0),
    m_autoRetractTimer(0),
    m_autoRetract(false),
    m_iconSize(24),
    m_showClockInTray(false),
    m_showClockSettingCB(0),
    m_layout(0)
{
    DCOPObject::setObjId("SystemTrayApplet");
    loadSettings();

    m_leftSpacer = new TQWidget(this);
    m_leftSpacer->setFixedSize(ICON_END_MARGIN,1);
    m_rightSpacer = new TQWidget(this);
    m_rightSpacer->setFixedSize(ICON_END_MARGIN,1);

    m_clockApplet = new ClockApplet(configFile, KPanelApplet::Normal, KPanelApplet::Preferences, this, "clockapplet");
    updateClockGeometry();
    connect(m_clockApplet, TQT_SIGNAL(clockReconfigured()), this, TQT_SLOT(updateClockGeometry()));

    setBackgroundOrigin(AncestorOrigin);

    kwin_module = new KWinModule(TQT_TQOBJECT(this));

    // kApplication notifies us of settings changes. added to support
    // disabling of frame effect on mouse hover
    kapp->dcopClient()->setNotifications(true);
    connectDCOPSignal("kicker", "kicker", "configurationChanged()", "loadSettings()", false);

    TQTimer::singleShot(0, this, TQT_SLOT(initialize()));
}

void SystemTrayApplet::updateClockGeometry()
{
    if (m_clockApplet)
        m_clockApplet->setFixedSize(m_clockApplet->widthForHeight(height()-2),height()-2);
}

void SystemTrayApplet::initialize()
{
    // register existing tray windows
    const TQValueList<WId> systemTrayWindows = kwin_module->systemTrayWindows();
    bool existing = false;
    for (TQValueList<WId>::ConstIterator it = systemTrayWindows.begin();
         it != systemTrayWindows.end(); ++it )
    {
        embedWindow(*it, true);
        existing = true;
    }

    showExpandButton(!m_hiddenWins.isEmpty());

    if (existing)
    {
        updateVisibleWins();
        layoutTray();
    }

    // the KWinModule notifies us when tray windows are added or removed
    connect( kwin_module, TQT_SIGNAL( systemTrayWindowAdded(WId) ),
             this, TQT_SLOT( systemTrayWindowAdded(WId) ) );
    connect( kwin_module, TQT_SIGNAL( systemTrayWindowRemoved(WId) ),
             this, TQT_SLOT( updateTrayWindows() ) );

    TQCString screenstr;
    screenstr.setNum(qt_xscreen());
    TQCString trayatom = "_NET_SYSTEM_TRAY_S" + screenstr;

    Display *display = qt_xdisplay();

    net_system_tray_selection = XInternAtom(display, trayatom, false);
    net_system_tray_opcode = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", false);

    // Acquire system tray
    XSetSelectionOwner(display,
                       net_system_tray_selection,
                       winId(),
                       CurrentTime);

    WId root = qt_xrootwin();

    if (XGetSelectionOwner (display, net_system_tray_selection) == winId())
    {
        XClientMessageEvent xev;

        xev.type = ClientMessage;
        xev.window = root;

        xev.message_type = XInternAtom (display, "MANAGER", False);
        xev.format = 32;
        xev.data.l[0] = CurrentTime;
        xev.data.l[1] = net_system_tray_selection;
        xev.data.l[2] = winId();
        xev.data.l[3] = 0;        /* manager specific data */
        xev.data.l[4] = 0;        /* manager specific data */

        XSendEvent (display, root, False, StructureNotifyMask, (XEvent *)&xev);
    }
    
    setBackground();
}

SystemTrayApplet::~SystemTrayApplet()
{
    for (TrayEmbedList::const_iterator it = m_hiddenWins.constBegin();
         it != m_hiddenWins.constEnd();
         ++it)
    {
        delete *it;
    }

    for (TrayEmbedList::const_iterator it = m_shownWins.constBegin();
         it != m_shownWins.constEnd();
         ++it)
    {
        delete *it;
    }

    if (m_leftSpacer) delete m_leftSpacer;
    if (m_rightSpacer) delete m_rightSpacer;

    KGlobal::locale()->removeCatalogue("ksystemtrayapplet");
}

bool SystemTrayApplet::x11Event( XEvent *e )
{
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2
    if ( e->type == ClientMessage ) {
        if ( e->xclient.message_type == net_system_tray_opcode &&
             e->xclient.data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
            if( isWinManaged( (WId)e->xclient.data.l[2] ) ) // we already manage it
                return true;
            embedWindow( e->xclient.data.l[2], false );
            updateVisibleWins();
            layoutTray();
            return true;
        }
    }
    return KPanelApplet::x11Event( e ) ;
}

void SystemTrayApplet::preferences()
{
    if (m_settingsDialog)
    {
        m_settingsDialog->show();
        m_settingsDialog->raise();
        return;
    }

    m_settingsDialog = new KDialogBase(0, "systrayconfig",
                                       false, i18n("Configure System Tray"),
                                       KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                                       KDialogBase::Ok, true);
    m_settingsDialog->resize(450, 400);
    connect(m_settingsDialog, TQT_SIGNAL(applyClicked()), this, TQT_SLOT(applySettings()));
    connect(m_settingsDialog, TQT_SIGNAL(okClicked()), this, TQT_SLOT(applySettings()));
    connect(m_settingsDialog, TQT_SIGNAL(finished()), this, TQT_SLOT(settingsDialogFinished()));

    TQGrid *settingsGrid = m_settingsDialog->makeGridMainWidget( 2, Qt::Vertical);

    m_showClockSettingCB = new TQCheckBox("Show Clock in Tray", settingsGrid);
    m_showClockSettingCB->setChecked(m_showClockInTray);

    //m_iconSelector = new KActionSelector(m_settingsDialog);
    m_iconSelector = new KActionSelector(settingsGrid);
    m_iconSelector->setAvailableLabel(i18n("Hidden icons:"));
    m_iconSelector->setSelectedLabel(i18n("Visible icons:"));
    //m_settingsDialog->setMainWidget(m_iconSelector);

    TQListBox *hiddenListBox = m_iconSelector->availableListBox();
    TQListBox *shownListBox = m_iconSelector->selectedListBox();

    TrayEmbedList::const_iterator it = m_shownWins.begin();
    TrayEmbedList::const_iterator itEnd = m_shownWins.end();
    for (; it != itEnd; ++it)
    {
        TQString name = KWin::windowInfo((*it)->embeddedWinId()).name();
        if(!shownListBox->findItem(name, TQt::ExactMatch | TQt::CaseSensitive))
        {
            shownListBox->insertItem(KWin::icon((*it)->embeddedWinId(), 22, 22, true), name);
        }
    }

    it = m_hiddenWins.begin();
    itEnd = m_hiddenWins.end();
    for (; it != itEnd; ++it)
    {
        TQString name = KWin::windowInfo((*it)->embeddedWinId()).name();
        if(!hiddenListBox->findItem(name, TQt::ExactMatch | TQt::CaseSensitive))
        {
            hiddenListBox->insertItem(KWin::icon((*it)->embeddedWinId(), 22, 22, true), name);
        }
    }

    m_settingsDialog->show();
}

void SystemTrayApplet::settingsDialogFinished()
{
    m_settingsDialog->delayedDestruct();
    m_settingsDialog = 0;
    m_iconSelector = 0;
}

void SystemTrayApplet::applySettings()
{
    if (!m_iconSelector)
    {
        return;
    }

    m_showClockInTray = m_showClockSettingCB->isChecked();

    KConfig *conf = config();

    // Save the sort order and hidden status using the window class (WM_CLASS) rather
    // than window name (caption) - window name is i18n-ed, so it's for example
    // not possible to create default settings.
    // For backwards compatibility, name is kept as it is, class is preceded by '!'.
    TQMap< TQString, TQString > windowNameToClass;
    for( TrayEmbedList::ConstIterator it = m_shownWins.begin();
         it != m_shownWins.end();
         ++it ) {
        KWin::WindowInfo info = KWin::windowInfo( (*it)->embeddedWinId(), NET::WMName, NET::WM2WindowClass);
        windowNameToClass[ info.name() ] = '!' + info.windowClassClass();
    }
    for( TrayEmbedList::ConstIterator it = m_hiddenWins.begin();
         it != m_hiddenWins.end();
         ++it ) {
        KWin::WindowInfo info = KWin::windowInfo( (*it)->embeddedWinId(), NET::WMName, NET::WM2WindowClass);
        windowNameToClass[ info.name() ] = '!' + info.windowClassClass();
    }

    conf->setGroup("SortedTrayIcons");
    m_sortOrderIconList.clear();
    for(TQListBoxItem* item = m_iconSelector->selectedListBox()->firstItem();
        item;
        item = item->next())
    {
        if( windowNameToClass.contains(item->text()))
            m_sortOrderIconList.append(windowNameToClass[item->text()]);
        else
            m_sortOrderIconList.append(item->text());
    }
    conf->writeEntry("SortOrder", m_sortOrderIconList);

    conf->setGroup("HiddenTrayIcons");
    m_hiddenIconList.clear();
    for(TQListBoxItem* item = m_iconSelector->availableListBox()->firstItem();
        item;
        item = item->next())
    {
        if( windowNameToClass.contains(item->text()))
            m_hiddenIconList.append(windowNameToClass[item->text()]);
        else
            m_hiddenIconList.append(item->text());
    }
    conf->writeEntry("Hidden", m_hiddenIconList);

    conf->setGroup("System Tray");
    conf->writeEntry("ShowClockInTray", m_showClockInTray);

    conf->sync();

    TrayEmbedList::iterator it = m_shownWins.begin();
    while (it != m_shownWins.end())
    {
        if (shouldHide((*it)->embeddedWinId()))
        {
            m_hiddenWins.append(*it);
            it = m_shownWins.erase(it);
        }
        else
        {
            ++it;
        }
    }

    it = m_hiddenWins.begin();
    while (it != m_hiddenWins.end())
    {
        if (!shouldHide((*it)->embeddedWinId()))
        {
            m_shownWins.append(*it);
            it = m_hiddenWins.erase(it);
        }
        else
        {
            ++it;
        }
    }

    showExpandButton(!m_hiddenWins.isEmpty());

    updateVisibleWins();
    layoutTray();
}

void SystemTrayApplet::checkAutoRetract()
{
    if (!m_autoRetractTimer)
    {
        return;
    }

    if (!geometry().contains(mapFromGlobal(TQCursor::pos())))
    {
        m_autoRetractTimer->stop();
        if (m_autoRetract)
        {
            m_autoRetract = false;

            if (m_showHidden)
            {
                retract();
            }
        }
        else
        {
            m_autoRetract = true;
            m_autoRetractTimer->start(2000, true);
        }

    }
    else
    {
        m_autoRetract = false;
        m_autoRetractTimer->start(250, true);
    }
}

void SystemTrayApplet::showExpandButton(bool show)
{
    if (show)
    {
        if (!m_expandButton)
        {
            m_expandButton = new SimpleArrowButton(this, Qt::UpArrow, 0, KickerSettings::showDeepButtons());
            m_expandButton->installEventFilter(this);
            refreshExpandButton();

            if (orientation() == Qt::Vertical)
            {
                m_expandButton->setFixedSize(width() - 4,
                                             m_expandButton->sizeHint()
                                                            .height());
            }
            else
            {
                m_expandButton->setFixedSize(m_expandButton->sizeHint()
                                                            .width(),
                                             height() - 4);
            }
            connect(m_expandButton, TQT_SIGNAL(clicked()),
                    this, TQT_SLOT(toggleExpanded()));

            m_autoRetractTimer = new TQTimer(this, "m_autoRetractTimer");
            connect(m_autoRetractTimer, TQT_SIGNAL(timeout()),
                    this, TQT_SLOT(checkAutoRetract()));
        }
        else
        {
            refreshExpandButton();
        }

        m_expandButton->show();
    }
    else if (m_expandButton)
    {
        m_expandButton->hide();
    }
}

void SystemTrayApplet::orientationChange( Orientation /*orientation*/ )
{
    refreshExpandButton();
}

void SystemTrayApplet::iconSizeChanged() {
	loadSettings();
	updateVisibleWins();
	layoutTray();

	TrayEmbedList::iterator emb = m_shownWins.begin();
	while (emb != m_shownWins.end()) {
		(*emb)->setFixedSize(m_iconSize, m_iconSize);
		++emb;
	}
	
	emb = m_hiddenWins.begin();
	while (emb != m_hiddenWins.end()) {
		(*emb)->setFixedSize(m_iconSize, m_iconSize);
		++emb;
	}
}

void SystemTrayApplet::loadSettings()
{
    // set our defaults
    setFrameStyle(NoFrame);
    m_showFrame = KickerSettings::showDeepButtons()?true:false;

    KConfig *conf = config();
    conf->reparseConfiguration();
    conf->setGroup("General");

    if (conf->readBoolEntry("ShowPanelFrame", false) || m_showFrame)	// Does ShowPanelFrame even exist?
    {
        setFrameStyle(Panel | Sunken);
    }

    conf->setGroup("HiddenTrayIcons");
    m_hiddenIconList = conf->readListEntry("Hidden");

    conf->setGroup("SortedTrayIcons");
    m_sortOrderIconList = conf->readListEntry("SortOrder");

    //Note This setting comes from kdeglobal.
    conf->setGroup("System Tray");
    m_iconSize = conf->readNumEntry("systrayIconWidth", 22);
    m_showClockInTray = conf->readNumEntry("ShowClockInTray", false);
}

void SystemTrayApplet::systemTrayWindowAdded( WId w )
{
    if (isWinManaged(w))
    {
        // we already manage it
        return;
    }

    embedWindow(w, true);
    updateVisibleWins();
    layoutTray();

    if (m_showFrame && frameStyle() == NoFrame)
    {
        setFrameStyle(Panel|Sunken);
    }
}

void SystemTrayApplet::embedWindow( WId w, bool kde_tray )
{
    TrayEmbed* emb = new TrayEmbed(kde_tray, this);
    emb->setAutoDelete(false);

    if (kde_tray)
    {
        static Atom hack_atom = XInternAtom( qt_xdisplay(), "_KDE_SYSTEM_TRAY_EMBEDDING", False );
        XChangeProperty( qt_xdisplay(), w, hack_atom, hack_atom, 32, PropModeReplace, NULL, 0 );
        emb->embed(w);
        XDeleteProperty( qt_xdisplay(), w, hack_atom );
    }
    else
    {
        emb->embed(w);
    }

    if (emb->embeddedWinId() == 0)  // error embedding
    {
        delete emb;
        return;
    }
    
    connect(emb, TQT_SIGNAL(embeddedWindowDestroyed()), TQT_SLOT(updateTrayWindows()));
    emb->getIconSize(m_iconSize);

    if (shouldHide(w))
    {
        emb->hide();
        m_hiddenWins.append(emb);
        showExpandButton(true);
    }
    else
    {
        //emb->hide();
        emb->setBackground();
        emb->show();
        m_shownWins.append(emb);
    }
}

bool SystemTrayApplet::isWinManaged(WId w)
{
    TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
    {
        if ((*emb)->embeddedWinId() == w) // we already manage it
        {
            return true;
        }
    }

    lastEmb = m_hiddenWins.end();
    for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
    {
        if ((*emb)->embeddedWinId() == w) // we already manage it
        {
            return true;
        }
    }

    return false;
}

bool SystemTrayApplet::shouldHide(WId w)
{
    return m_hiddenIconList.find(KWin::windowInfo(w).name()) != m_hiddenIconList.end()
        || m_hiddenIconList.find('!'+KWin::windowInfo(w,0,NET::WM2WindowClass).windowClassClass())
            != m_hiddenIconList.end();
}

void SystemTrayApplet::updateVisibleWins()
{
    TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
    TrayEmbedList::const_iterator emb = m_hiddenWins.begin();

    if (m_showHidden)
    {
        for (; emb != lastEmb; ++emb)
        {
            (*emb)->setBackground();
            (*emb)->show();
        }
    }
    else
    {
        for (; emb != lastEmb; ++emb)
        {
            (*emb)->hide();
        }
    }
    
    TQMap< QXEmbed*, TQString > names; // cache window names and classes
    TQMap< QXEmbed*, TQString > classes;
    for( TrayEmbedList::const_iterator it = m_shownWins.begin();
         it != m_shownWins.end();
         ++it ) {
        KWin::WindowInfo info = KWin::windowInfo((*it)->embeddedWinId(),NET::WMName,NET::WM2WindowClass);
        names[ *it ] = info.name();
        classes[ *it ] = '!'+info.windowClassClass();
    }
    TrayEmbedList newList;
    for( TQStringList::const_iterator it1 = m_sortOrderIconList.begin();
         it1 != m_sortOrderIconList.end();
         ++it1 ) {
        for( TrayEmbedList::iterator it2 = m_shownWins.begin();
             it2 != m_shownWins.end();
             ) {
            if( (*it1).startsWith("!") ? classes[ *it2 ] == *it1 : names[ *it2 ] == *it1 ) {
                newList.append( *it2 ); // don't bail out, there may be multiple ones
                it2 = m_shownWins.erase( it2 );
            } else
                ++it2;
        }
    }
    for( TrayEmbedList::const_iterator it = m_shownWins.begin();
         it != m_shownWins.end();
         ++it )
        newList.append( *it ); // append unsorted items
    m_shownWins = newList;
}

void SystemTrayApplet::toggleExpanded()
{
    if (m_showHidden)
    {
        retract();
    }
    else
    {
        expand();
    }
}

void SystemTrayApplet::refreshExpandButton()
{
    if (!m_expandButton)
    {
        return;
    }

    Qt::ArrowType a;

    if (orientation() == Qt::Vertical)
        a = m_showHidden ? Qt::DownArrow : Qt::UpArrow;
    else
        a = (m_showHidden ^ kapp->reverseLayout()) ? Qt::RightArrow : Qt::LeftArrow;
    
    m_expandButton->setArrowType(a);
}

void SystemTrayApplet::expand()
{
    m_showHidden = true;
    refreshExpandButton();

    updateVisibleWins();
    layoutTray();

    if (m_autoRetractTimer)
    {
        m_autoRetractTimer->start(250, true);
    }
}

void SystemTrayApplet::retract()
{
    if (m_autoRetractTimer)
    {
        m_autoRetractTimer->stop();
    }

    m_showHidden = false;
    refreshExpandButton();

    updateVisibleWins();
    layoutTray();
}

void SystemTrayApplet::updateTrayWindows()
{
    TrayEmbedList::iterator emb = m_shownWins.begin();
    while (emb != m_shownWins.end())
    {
        WId wid = (*emb)->embeddedWinId();
        if ((wid == 0) ||
            ((*emb)->kdeTray() &&
             !kwin_module->systemTrayWindows().contains(wid)))
        {
            (*emb)->deleteLater();
            emb = m_shownWins.erase(emb);
        }
        else
        {
            ++emb;
        }
    }

    emb = m_hiddenWins.begin();
    while (emb != m_hiddenWins.end())
    {
        WId wid = (*emb)->embeddedWinId();
        if ((wid == 0) ||
            ((*emb)->kdeTray() &&
             !kwin_module->systemTrayWindows().contains(wid)))
        {
            (*emb)->deleteLater();
            emb = m_hiddenWins.erase(emb);
        }
        else
        {
            ++emb;
        }
    }

    showExpandButton(!m_hiddenWins.isEmpty());
    updateVisibleWins();
    layoutTray();
}

int SystemTrayApplet::maxIconWidth() const
{
    int largest = m_iconSize;

    TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
    {
        if (*emb == 0)
        {
            continue;
        }

        int width = (*emb)->width();
        if (width > largest)
        {
            largest = width;
        }
    }

    if (m_showHidden)
    {
        lastEmb = m_hiddenWins.end();
        for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
        {
            int width = (*emb)->width();
            if (width > largest)
            {
                largest = width;
            }
        }
    }

    return largest;
}

int SystemTrayApplet::maxIconHeight() const
{
    int largest = m_iconSize;

    TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != m_shownWins.end(); ++emb)
    {
        if (*emb == 0)
        {
            continue;
        }

        int height = (*emb)->height();
        if (height > largest)
        {
            largest = height;
        }
    }

    if (m_showHidden)
    {
        lastEmb = m_hiddenWins.end();
        for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != m_hiddenWins.end(); ++emb)
        {
            if (*emb == 0)
            {
                continue;
            }

            int height = (*emb)->height();
            if (height > largest)
            {
                largest = height;
            }
        }
    }

    return largest;
}

bool SystemTrayApplet::eventFilter(TQObject* watched, TQEvent* e)
{
    if (TQT_BASE_OBJECT(watched) == TQT_BASE_OBJECT(m_expandButton))
    {
        TQPoint p;
        if (e->type() == TQEvent::ContextMenu)
        {
            p = TQT_TQCONTEXTMENUEVENT(e)->globalPos();
        }
        else if (e->type() == TQEvent::MouseButtonPress)
        {
            TQMouseEvent* me = TQT_TQMOUSEEVENT(e);
            if (me->button() == Qt::RightButton)
            {
                p = me->globalPos();
            }
        }

        if (!p.isNull())
        {
            TQPopupMenu* contextMenu = new TQPopupMenu(this);
            contextMenu->insertItem(SmallIcon("configure"), i18n("Configure System Tray..."),
                                    this, TQT_SLOT(configure()));

            contextMenu->exec(TQT_TQCONTEXTMENUEVENT(e)->globalPos());

            delete contextMenu;
            return true;
        }
    }

    return false;
}

int SystemTrayApplet::widthForHeight(int h) const
{
    if (orientation() == Qt::Vertical)
    {
        return width();
    }

    int currentHeight = height();
    if (currentHeight != h)
    {
        SystemTrayApplet* me = const_cast<SystemTrayApplet*>(this);
        me->setMinimumSize(0, 0);
        me->setMaximumSize(32767, 32767);
        me->setFixedHeight(h);
    }

    return sizeHint().width(); 
}

int SystemTrayApplet::heightForWidth(int w) const
{
    if (orientation() == Qt::Horizontal)
    {
        return height();
    }

    int currentWidth = width();
    if (currentWidth != w)
    {
        SystemTrayApplet* me = const_cast<SystemTrayApplet*>(this);
        me->setMinimumSize(0, 0);
        me->setMaximumSize(32767, 32767);
        me->setFixedWidth(w);
    }

    return sizeHint().height(); 
}

void SystemTrayApplet::moveEvent( TQMoveEvent* )
{
    setBackground();
}


void SystemTrayApplet::resizeEvent( TQResizeEvent* )
{
    layoutTray();
    // we need to give ourselves a chance to adjust our size before calling this
    TQTimer::singleShot(0, this, TQT_SIGNAL(updateLayout()));
}

void SystemTrayApplet::layoutTray()
{
    setUpdatesEnabled(false);

    int iconCount = m_shownWins.count();

    if (m_showHidden)
    {
        iconCount += m_hiddenWins.count();
    }

    /* heightWidth = height or width in pixels (depends on orientation())
     * nbrOfLines = number of rows or cols (depends on orientation())
     * line = what line to draw an icon in */
    int i = 0, line, nbrOfLines, heightWidth;
    bool showExpandButton = m_expandButton && m_expandButton->isVisibleTo(this);
    delete m_layout;
    m_layout = new TQGridLayout(this, 1, 1, ICON_MARGIN, ICON_MARGIN);

    if (m_expandButton)
    {
        if (orientation() == Qt::Vertical)
        {
            m_expandButton->setFixedSize(width() - 4, m_expandButton->sizeHint().height());
        }
        else
        {
            m_expandButton->setFixedSize(m_expandButton->sizeHint().width(), height() - 4);
        }
    }

    // col = column or row, depends on orientation(),
    // the opposite direction of line
    int col = 0;

    // 
    // The margin and spacing specified in the layout implies that:
    // [-- ICON_MARGIN pixels --] [-- first icon --] [-- ICON_MARGIN pixels --] ... [-- ICON_MARGIN pixels --] [-- last icon --] [-- ICON_MARGIN pixels --]
    //
    // So, if we say that iconWidth is the icon width plus the ICON_MARGIN pixels spacing, then the available width for the icons
    // is the widget width minus ICON_MARGIN pixels margin. Forgetting these ICON_MARGIN pixels broke the layout algorithm in KDE <= 3.5.9.
    //
    // This fix makes the workarounds in the heightForWidth() and widthForHeight() methods unneeded.
    //

    if (orientation() == Qt::Vertical)
    {
        int iconWidth = maxIconWidth() + ICON_MARGIN; // +2 for the margins that implied by the layout
        heightWidth = width() - ICON_MARGIN;
        // to avoid nbrOfLines=0 we ensure heightWidth >= iconWidth!
        heightWidth = heightWidth < iconWidth ? iconWidth : heightWidth;
        nbrOfLines = heightWidth / iconWidth;

        m_layout->addMultiCellWidget(m_leftSpacer,
                                     0, 0,
                                     0, nbrOfLines - 1,
                                     Qt::AlignHCenter | Qt::AlignVCenter);
        col = 1;

        if (showExpandButton)
        {
            m_layout->addMultiCellWidget(m_expandButton,
                                         1, 1,
                                         0, nbrOfLines - 1,
                                         Qt::AlignHCenter | Qt::AlignVCenter);
            col = 2;
        }

        if (m_showHidden)
        {
            TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
            for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin();
                 emb != lastEmb; ++emb)
            {
                line = i % nbrOfLines;
                (*emb)->show();
                m_layout->addWidget((*emb), col, line,
                                    Qt::AlignHCenter | Qt::AlignVCenter);

                if ((line + 1) == nbrOfLines)
                {
                    ++col;
                }

                ++i;
            }
        }

        TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
        for (TrayEmbedList::const_iterator emb = m_shownWins.begin();
             emb != lastEmb; ++emb)
        {
            line = i % nbrOfLines;
            (*emb)->show();
            m_layout->addWidget((*emb), col, line,
                                Qt::AlignHCenter | Qt::AlignVCenter);

            if ((line + 1) == nbrOfLines)
            {
                ++col;
            }

            ++i;
        }

        m_layout->addMultiCellWidget(m_rightSpacer,
                                     col, col,
                                     0, nbrOfLines - 1,
                                     Qt::AlignHCenter | Qt::AlignVCenter);

        if (m_clockApplet) {
            if (m_showClockInTray)
                m_clockApplet->show();
            else
                m_clockApplet->hide();

            m_layout->addMultiCellWidget(m_clockApplet,
                                     col+1, col+1,
                                     0, nbrOfLines - 1,
                                     Qt::AlignHCenter | Qt::AlignVCenter);
        }
    }
    else // horizontal
    {
        int iconHeight = maxIconHeight() + ICON_MARGIN; // +2 for the margins that implied by the layout
        heightWidth = height() - ICON_MARGIN;
        heightWidth = heightWidth < iconHeight ? iconHeight : heightWidth; // to avoid nbrOfLines=0
        nbrOfLines = heightWidth / iconHeight;

        m_layout->addMultiCellWidget(m_leftSpacer,
                                     0, nbrOfLines - 1,
                                     0, 0,
                                     Qt::AlignHCenter | Qt::AlignVCenter);
        col = 1;

        if (showExpandButton)
        {
            m_layout->addMultiCellWidget(m_expandButton,
                                         0, nbrOfLines - 1,
                                         1, 1,
                                         Qt::AlignHCenter | Qt::AlignVCenter);
            col = 2;
        }

        if (m_showHidden)
        {
            TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
            for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
            {
                line = i % nbrOfLines;
                (*emb)->show();
                m_layout->addWidget((*emb), line, col,
                                    Qt::AlignHCenter | Qt::AlignVCenter);

                if ((line + 1) == nbrOfLines)
                {
                    ++col;
                }

                ++i;
            }
        }

        TrayEmbedList::const_iterator lastEmb = m_shownWins.end();
        for (TrayEmbedList::const_iterator emb = m_shownWins.begin();
             emb != lastEmb; ++emb)
        {
            line = i % nbrOfLines;
            (*emb)->show();
            m_layout->addWidget((*emb), line, col,
                                Qt::AlignHCenter | Qt::AlignVCenter);

            if ((line + 1) == nbrOfLines)
            {
                ++col;
            }

            ++i;
        }

        m_layout->addMultiCellWidget(m_rightSpacer,
                                     0, nbrOfLines - 1,
                                     col, col,
                                     Qt::AlignHCenter | Qt::AlignVCenter);

        if (m_clockApplet) {
            if (m_showClockInTray)
                m_clockApplet->show();
            else
                m_clockApplet->hide();

            m_layout->addMultiCellWidget(m_clockApplet,
                                         0, nbrOfLines - 1,
                                         col+1, col+1,
                                         Qt::AlignHCenter | Qt::AlignVCenter);
        }
    }

    setUpdatesEnabled(true);
    updateGeometry();
    setBackground();

    updateClockGeometry();
}

void SystemTrayApplet::paletteChange(const TQPalette & /* oldPalette */)
{
    setBackground();
}

void SystemTrayApplet::setBackground()
{
    TrayEmbedList::const_iterator lastEmb;
    
    lastEmb = m_shownWins.end();
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
        (*emb)->setBackground();
    
    lastEmb = m_hiddenWins.end();
    for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
        (*emb)->setBackground();
}


TrayEmbed::TrayEmbed( bool kdeTray, TQWidget* parent )
    : QXEmbed( parent ), kde_tray( kdeTray ), m_ensureBackgroundSetTimerCount(0)
{
    m_ensureBackgroundSetTimer = new TQTimer();
    connect(m_ensureBackgroundSetTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(ensureBackgroundSet()));
    hide();
    m_scaledWidget = new TQWidget(parent);
    m_scaledWidget->hide();
}

TrayEmbed::~TrayEmbed()
{
    delete m_ensureBackgroundSetTimer;
}

void TrayEmbed::getIconSize(int defaultIconSize)
{
    TQSize minSize = minimumSizeHint();
    
    int width = minSize.width();
    int height = minSize.height();
    
    if (width < 1 || width > defaultIconSize)
        width = defaultIconSize;
    if (height < 1 || height > defaultIconSize)
        height = defaultIconSize;
    
    setFixedSize(width, height);
    setBackground();
}

void TrayEmbed::setBackground()
{
    const TQPixmap *pbg = parentWidget()->backgroundPixmap();

    if (pbg)
    {
        TQPixmap bg(width(), height());
        bg.fill(parentWidget(), pos());
        setPaletteBackgroundPixmap(bg);
        setBackgroundOrigin(WidgetOrigin);
    }
    else
        unsetPalette();

    if (!isHidden())
    {
        XClearArea(x11Display(), embeddedWinId(), 0, 0, 0, 0, True);
        m_ensureBackgroundSetTimerCount = 0;
        ensureBackgroundSet();
    }
}

void TrayEmbed::ensureBackgroundSet()
{
	XWindowAttributes winprops;
	XGetWindowAttributes(x11Display(), embeddedWinId(), &winprops);
	if (winprops.depth == 32) {
		// This is a nasty little hack to make sure that tray icons / applications which do not match our QXEmbed native depth are still displayed properly,
		// i.e without irritating white/grey borders where the tray icon's transparency is supposed to be...
		// Essentially it converts a 24 bit Xlib Pixmap to a 32 bit Xlib Pixmap

		TQPixmap bg(width(), height());

		// Get the RGB background image
		bg.fill(parentWidget(), pos());
		TQImage bgImage = bg.convertToImage();
		
		// Create the ARGB pixmap
		Pixmap argbpixmap = XCreatePixmap(x11Display(), embeddedWinId(), width(), height(), 32);
		GC gc;
		gc = XCreateGC(x11Display(), embeddedWinId(), 0, 0);
		int w = bgImage.width();
		int h = bgImage.height();
		for (int y = 0; y < h; ++y) {
			TQRgb *ls = (TQRgb *)bgImage.scanLine( y );
			for (int x = 0; x < w; ++x) {
				TQRgb l = ls[x];
				int r = int( tqRed( l ) );
				int g = int( tqGreen( l ) );
				int b = int( tqBlue( l ) );
				int a = int( tqAlpha( l ) );
				ls[x] = tqRgba( r, g, b, a );
				XSetForeground(x11Display(), gc, (r << 16) | (g << 8) | b );
				XDrawPoint(x11Display(), argbpixmap, gc, x, y);
			}
		}
		XFlush(x11Display()); 
		XSetWindowBackgroundPixmap(x11Display(), embeddedWinId(), argbpixmap);
		XFreePixmap(x11Display(), argbpixmap);
		XFreeGC(x11Display(), gc);

		// Repaint
		XClearArea(x11Display(), embeddedWinId(), 0, 0, 0, 0, True);

		// HACK
		// Clear background artifacts in first available timeslot after initial icon display
		if (m_ensureBackgroundSetTimerCount < 1) {
			m_ensureBackgroundSetTimerCount++;
			m_ensureBackgroundSetTimer->stop();
			m_ensureBackgroundSetTimer->start(0, TRUE);
		}
	}
}
