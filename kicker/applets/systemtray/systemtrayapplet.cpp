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
#include <tqspinbox.h>
#include <tqcheckbox.h>
#include <tqtimer.h>
#include <tqpixmap.h>
#include <tqevent.h>
#include <tqstyle.h>
#include <tqgrid.h>
#include <tqpainter.h>
#include <tqimage.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <krun.h>
#include <twinmodule.h>
#include <kdialogbase.h>
#include <tdeactionselector.h>
#include <kiconloader.h>
#include <twin.h>

#include "kickerSettings.h"

#include "simplebutton.h"

#include "systemtrayapplet.h"
#include "systemtrayapplet.moc"

#include <X11/Xlib.h>

#define ICON_END_MARGIN KickerSettings::showDeepButtons()?4:0

extern "C"
{
    TDE_EXPORT KPanelApplet* init(TQWidget *parent, const TQString& configFile)
    {
        TDEGlobal::locale()->insertCatalogue("ksystemtrayapplet");
        return new SystemTrayApplet(configFile, KPanelApplet::Normal,
                                    KPanelApplet::Preferences, parent, "ksystemtrayapplet");
    }
}

SystemTrayApplet::SystemTrayApplet(const TQString& configFile, Type type, int actions,
                                   TQWidget *parent, const char *name)
  : KPanelApplet(configFile, type, actions, parent, name),
    m_showFrame(KickerSettings::showDeepButtons()),
    m_showHidden(false),
    m_expandButton(nullptr),
    m_leftSpacer(nullptr),
    m_rightSpacer(nullptr),
    m_clockApplet(nullptr),
    m_settingsDialog(nullptr),
    m_iconSelector(0),
    m_autoRetractTimer(nullptr),
    m_autoRetract(false),
    m_iconSize(24),
    m_showClockInTray(false),
    m_showClockSettingCB(nullptr),
    m_iconMargin(1),
    m_iconMarginSB(nullptr),
    m_iconSpacing(1),
    m_iconSpacingSB(nullptr),
    m_layout(nullptr)
{
    DCOPObject::setObjId("SystemTrayApplet");
    loadSettings();

    m_leftSpacer = new TQWidget(this);
    m_leftSpacer->setFixedSize(ICON_END_MARGIN,1);
    m_rightSpacer = new TQWidget(this);
    m_rightSpacer->setFixedSize(ICON_END_MARGIN,1);

    m_clockApplet = new ClockApplet(configFile, KPanelApplet::Normal, KPanelApplet::Preferences, this, "clockapplet");
    updateClockGeometry();
    connect(m_clockApplet, TQ_SIGNAL(clockReconfigured()), this, TQ_SLOT(updateClockGeometry()));
    connect(m_clockApplet, TQ_SIGNAL(updateLayout()), this, TQ_SLOT(updateClockGeometry()));

    setBackgroundOrigin(AncestorOrigin);

    twin_module = new KWinModule(this);

    // kApplication notifies us of settings changes. added to support
    // disabling of frame effect on mouse hover
    kapp->dcopClient()->setNotifications(true);
    connectDCOPSignal("kicker", "kicker", "configurationChanged()", "loadSettings()", false);

    TQTimer::singleShot(0, this, TQ_SLOT(initialize()));
}

void SystemTrayApplet::updateClockGeometry()
{
    if (m_clockApplet)
    {
        m_clockApplet->setPosition(position());
        if (orientation() == TQt::Horizontal)
        {
            m_clockApplet->setFixedSize(m_clockApplet->widthForHeight(height()),height());
        }
        else
        {
            m_clockApplet->setFixedSize(width(),m_clockApplet->heightForWidth(width()));
        }
    }
}

void SystemTrayApplet::initialize()
{
    // register existing tray windows
    const TQValueList<WId> systemTrayWindows = twin_module->systemTrayWindows();
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
    connect( twin_module, TQ_SIGNAL( systemTrayWindowAdded(WId) ),
             this, TQ_SLOT( systemTrayWindowAdded(WId) ) );
    connect( twin_module, TQ_SIGNAL( systemTrayWindowRemoved(WId) ),
             this, TQ_SLOT( updateTrayWindows() ) );

    TQCString screenstr;
    screenstr.setNum(tqt_xscreen());
    TQCString trayatom = "_NET_SYSTEM_TRAY_S" + screenstr;

    Display *display = tqt_xdisplay();

    net_system_tray_selection = XInternAtom(display, trayatom, false);
    net_system_tray_opcode = XInternAtom(display, "_NET_SYSTEM_TRAY_OPCODE", false);

    // Acquire system tray
    XSetSelectionOwner(display,
                       net_system_tray_selection,
                       winId(),
                       CurrentTime);

    WId root = tqt_xrootwin();

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

    TDEGlobal::locale()->removeCatalogue("ksystemtrayapplet");
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
    connect(m_settingsDialog, TQ_SIGNAL(applyClicked()), this, TQ_SLOT(applySettings()));
    connect(m_settingsDialog, TQ_SIGNAL(okClicked()), this, TQ_SLOT(applySettings()));
    connect(m_settingsDialog, TQ_SIGNAL(finished()), this, TQ_SLOT(settingsDialogFinished()));

    TQWidget *settingsWidget = m_settingsDialog->makeMainWidget();
    TQVBoxLayout *settingsLayout = new TQVBoxLayout(settingsWidget);

    m_showClockSettingCB = new TQCheckBox(i18n("Show clock in tray"), settingsWidget);
    m_showClockSettingCB->setChecked(m_showClockInTray);
    settingsLayout->addWidget(m_showClockSettingCB);
    settingsLayout->setSpacing(m_settingsDialog->spacingHint());

    m_iconSelector = new TDEActionSelector(settingsWidget);
    m_iconSelector->setAvailableLabel(i18n("Hidden icons:"));
    m_iconSelector->setSelectedLabel(i18n("Visible icons:"));
    settingsLayout->addWidget(m_iconSelector);

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

    TQGridLayout *marginGrid = new TQGridLayout(2, 3);
    marginGrid->setSpacing(m_settingsDialog->spacingHint());
    marginGrid->setColStretch(2,1); // last empty column operates as a spacer

    TQLabel *iconMarginL = new TQLabel(i18n("Icon &margin:"), settingsWidget);
    m_iconMarginSB = new TQSpinBox(0, 20, 1, settingsWidget);
    m_iconMarginSB->setSuffix(i18n(" px"));
    m_iconMarginSB->setValue(m_iconMargin);
    iconMarginL->setBuddy(m_iconMarginSB);
    TQWhatsThis::add(iconMarginL, i18n( "Preferred margin between icons in the system tray and outer border.<br>"
                                        "Note that the actual margins can be either larger (in case there is "
                                        "extra free space on the panel) or smaller (in case the panel is "
                                        "not able to fit a single row).") );

    marginGrid->addWidget(iconMarginL, 0, 0);
    marginGrid->addWidget(m_iconMarginSB, 0, 1);

    TQLabel *iconSpacingL = new TQLabel(i18n("Icon &spacing:"), settingsWidget);
    m_iconSpacingSB = new TQSpinBox(0, 20, 1, settingsWidget);
    m_iconSpacingSB->setSuffix(i18n(" px"));
    m_iconSpacingSB->setValue(m_iconSpacing);
    iconSpacingL->setBuddy(m_iconSpacingSB);
    TQWhatsThis::add(iconSpacingL, i18n( "Minimal spacing between adjacent icons in the system tray.<br>"
                                         "Note that the actual spacing can be larger (in case there is "
                                         "extra free space on the panel).") );


    marginGrid->addWidget(iconSpacingL, 1, 0);
    marginGrid->addWidget(m_iconSpacingSB, 1, 1);

    settingsLayout->addLayout(marginGrid);

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
    m_iconMargin = m_iconMarginSB->value();
    m_iconSpacing = m_iconSpacingSB->value();

    TDEConfig *conf = config();

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
    conf->writeEntry("IconMargin", m_iconMargin);
    conf->writeEntry("IconSpacing", m_iconSpacing);

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
            m_expandButton = new SimpleArrowButton(this, TQt::UpArrow, 0, KickerSettings::showDeepButtons());
            m_expandButton->installEventFilter(this);
            refreshExpandButton();

            if (orientation() == TQt::Vertical)
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
            connect(m_expandButton, TQ_SIGNAL(clicked()),
                    this, TQ_SLOT(toggleExpanded()));

            m_autoRetractTimer = new TQTimer(this, "m_autoRetractTimer");
            connect(m_autoRetractTimer, TQ_SIGNAL(timeout()),
                    this, TQ_SLOT(checkAutoRetract()));
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

    TDEConfig *conf = config();
    conf->reparseConfiguration();
    conf->setGroup("General");

    if (conf->readBoolEntry("ShowPanelFrame", false) || m_showFrame) // Does ShowPanelFrame even exist?
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
    m_iconMargin = conf->readNumEntry("IconMargin", 1);
    m_iconSpacing = conf->readNumEntry("IconSpacing", 1);
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
        static Atom hack_atom = XInternAtom( tqt_xdisplay(), "_TDE_SYSTEM_TRAY_EMBEDDING", False );
        XChangeProperty( tqt_xdisplay(), w, hack_atom, hack_atom, 32, PropModeReplace, NULL, 0 );
        emb->embed(w);
        XDeleteProperty( tqt_xdisplay(), w, hack_atom );
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

    connect(emb, TQ_SIGNAL(embeddedWindowDestroyed()), TQ_SLOT(updateTrayWindows()));
    emb->setFixedSize(m_iconSize, m_iconSize);

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

    TQt::ArrowType a;

    if (orientation() == TQt::Vertical)
        a = m_showHidden ? TQt::DownArrow : TQt::UpArrow;
    else
        a = (m_showHidden ^ kapp->reverseLayout()) ? TQt::RightArrow : TQt::LeftArrow;
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
             !twin_module->systemTrayWindows().contains(wid)))
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
             !twin_module->systemTrayWindows().contains(wid)))
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
    for (TrayEmbedList::const_iterator emb = m_shownWins.begin(); emb != lastEmb; ++emb)
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
    if (watched == m_expandButton)
    {
        TQPoint p;
        if (e->type() == TQEvent::ContextMenu)
        {
            p = static_cast<TQContextMenuEvent*>(e)->globalPos();
        }
        else if (e->type() == TQEvent::MouseButtonPress)
        {
            TQMouseEvent* me = static_cast<TQMouseEvent*>(e);
            if (me->button() == TQt::RightButton)
            {
                p = me->globalPos();
            }
        }

        if (!p.isNull())
        {
            TQPopupMenu* contextMenu = new TQPopupMenu(this);
            contextMenu->insertItem(SmallIcon("configure"), i18n("Configure System Tray..."),
                                    this, TQ_SLOT(configure()));

            contextMenu->exec(static_cast<TQContextMenuEvent*>(e)->globalPos());

            delete contextMenu;
            return true;
        }
    }

    return false;
}

int SystemTrayApplet::widthForHeight(int h) const
{
    if (orientation() == TQt::Vertical)
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
    if (orientation() == TQt::Horizontal)
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
    TQTimer::singleShot(0, this, TQ_SIGNAL(updateLayout()));
}

void SystemTrayApplet::layoutTray()
{
    setUpdatesEnabled(false);

    int i = 0;
    bool showExpandButton = m_expandButton && m_expandButton->isVisibleTo(this);
    delete m_layout;
    m_layout = new TQGridLayout(this, 1, 1, m_iconMargin, m_iconSpacing);

    if (m_expandButton)
    {
        if (orientation() == TQt::Vertical)
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
    // The margin and spacing specified in the layout implies that it looks like:
    //
    // [-- m_iconMargin px --] [-- first icon --] [-- m_iconSpacing px --] ... [-- m_iconSpacing px --] [-- last icon --] [-- m_iconMargin px --]
    //
    // So, the panel size with this layout should conform to the relation:
    //
    //   panelSize == iconSize*nbrOfLines + 2*m_iconMargin + (nbrOfLines-1)*m_iconSpacing.
    //
    // Solving it for number of lines we get:
    //
    //                  panelSize - 2*m_iconMargin + m_iconSpacing
    //   nbrOfLines =  --------------------------------------------
    //                           iconSize + m_iconSpacing

    if (orientation() == TQt::Vertical)
    {
        int nbrOfLines = (width() - 2*m_iconMargin + m_iconSpacing) /
                         (maxIconWidth() + m_iconSpacing);
        if (nbrOfLines < 1) {
            nbrOfLines = 1;  // avoid nbrOfLines==0 or negative (in case m_iconMargin is unreasonably large)
            m_layout->setMargin( (width() - maxIconWidth()) / 2); // also adjust the margins, so the icons won't get shifted beyond visibility
        }

        m_layout->addMultiCellWidget(m_leftSpacer,
                                     0, 0,
                                     0, nbrOfLines - 1,
                                     TQt::AlignHCenter | TQt::AlignVCenter);
        col = 1;

        if (showExpandButton)
        {
            m_layout->addMultiCellWidget(m_expandButton,
                                         1, 1,
                                         0, nbrOfLines - 1,
                                         TQt::AlignHCenter | TQt::AlignVCenter);
            col = 2;
        }

        if (m_showHidden)
        {
            TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
            for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin();
                 emb != lastEmb; ++emb)
            {
                int line = i % nbrOfLines;
                (*emb)->show();
                m_layout->addWidget((*emb), col, line,
                                    TQt::AlignHCenter | TQt::AlignVCenter);

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
            int line = i % nbrOfLines;
            (*emb)->show();
            m_layout->addWidget((*emb), col, line,
                                TQt::AlignHCenter | TQt::AlignVCenter);

            if ((line + 1) == nbrOfLines)
            {
                ++col;
            }

            ++i;
        }

        m_layout->addMultiCellWidget(m_rightSpacer,
                                     col, col,
                                     0, nbrOfLines - 1,
                                     TQt::AlignHCenter | TQt::AlignVCenter);

        if (m_clockApplet) {
            if (m_showClockInTray)
                m_clockApplet->show();
            else
                m_clockApplet->hide();

            m_layout->addMultiCellWidget(m_clockApplet,
                                     col+1, col+1,
                                     0, nbrOfLines - 1,
                                     TQt::AlignHCenter | TQt::AlignVCenter);
        }
    }
    else // horizontal
    {
        int nbrOfLines = (height() - 2*m_iconMargin + m_iconSpacing) /
                         (maxIconHeight() + m_iconSpacing);
        if (nbrOfLines < 1) {
            nbrOfLines = 1;  // avoid nbrOfLines==0 or negative (in case m_iconMargin is unreasonably large)
            m_layout->setMargin( (height() - maxIconHeight()) / 2); // also adjust the margins, so the icons won't shift beyond viability
        }

        m_layout->addMultiCellWidget(m_leftSpacer,
                                     0, nbrOfLines - 1,
                                     0, 0,
                                     TQt::AlignHCenter | TQt::AlignVCenter);
        col = 1;

        if (showExpandButton)
        {
            m_layout->addMultiCellWidget(m_expandButton,
                                         0, nbrOfLines - 1,
                                         1, 1,
                                         TQt::AlignHCenter | TQt::AlignVCenter);
            col = 2;
        }

        if (m_showHidden)
        {
            TrayEmbedList::const_iterator lastEmb = m_hiddenWins.end();
            for (TrayEmbedList::const_iterator emb = m_hiddenWins.begin(); emb != lastEmb; ++emb)
            {
                int line = i % nbrOfLines;
                (*emb)->show();
                m_layout->addWidget((*emb), line, col,
                                    TQt::AlignHCenter | TQt::AlignVCenter);

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
            int line = i % nbrOfLines;
            (*emb)->show();
            m_layout->addWidget((*emb), line, col,
                                TQt::AlignHCenter | TQt::AlignVCenter);

            if ((line + 1) == nbrOfLines)
            {
                ++col;
            }

            ++i;
        }

        m_layout->addMultiCellWidget(m_rightSpacer,
                                     0, nbrOfLines - 1,
                                     col, col,
                                     TQt::AlignHCenter | TQt::AlignVCenter);

        if (m_clockApplet) {
            if (m_showClockInTray)
                m_clockApplet->show();
            else
                m_clockApplet->hide();

            m_layout->addMultiCellWidget(m_clockApplet,
                                         0, nbrOfLines - 1,
                                         col+1, col+1,
                                         TQt::AlignHCenter | TQt::AlignVCenter);
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
    : QXEmbed( parent ), kde_tray( kdeTray )
{
    hide();
    m_scaledWidget = new TQWidget(parent);
    m_scaledWidget->hide();
}

TrayEmbed::~TrayEmbed()
{
    //
}

void TrayEmbed::setBackground()
{
    const TQPixmap *pbg = parentWidget()->backgroundPixmap();

    if (pbg) {
        TQPixmap bg(width(), height());
        bg.fill(parentWidget(), pos());
        setPaletteBackgroundPixmap(bg);
        setBackgroundOrigin(WidgetOrigin);
    }
    else {
        unsetPalette();
    }

    if (!isHidden())
    {
        XClearArea(x11Display(), embeddedWinId(), 0, 0, 0, 0, True);
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
                XSetForeground(x11Display(), gc, (a << 24) | (r << 16) | (g << 8) | b );
                XDrawPoint(x11Display(), argbpixmap, gc, x, y);
            }
        }
        XFlush(x11Display());
        XSetWindowBackgroundPixmap(x11Display(), embeddedWinId(), argbpixmap);
        XFreePixmap(x11Display(), argbpixmap);
        XFreeGC(x11Display(), gc);

        // Repaint
        XClearArea(x11Display(), embeddedWinId(), 0, 0, 0, 0, True);
    }
}
