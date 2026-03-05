/*
    Copyright (C) 2001, S.R.Haque <srhaque@iee.org>. Derived from an
    original by Matthias H�zer-Klpfel released under the QPL.
    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

DESCRIPTION

    KDE Keyboard Tool. Manages XKB keyboard mappings.
*/

#include <unistd.h>
#include <assert.h>

#include <tqregexp.h>
#include <tqfile.h>
#include <tqstringlist.h>
#include <tqimage.h>
#include <tqtimer.h>

#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#ifdef WITH_TDEHWLIB
#include <tdehardwaredevices.h>
#endif
#include <tdeglobal.h>
#include <tdeglobalaccel.h>
#include <tdelocale.h>
#include <tdeprocess.h>
#include <twinmodule.h>
#include <twin.h>
#include <tdetempfile.h>
#include <tdestandarddirs.h>
#include <kipc.h>
#include <tdeaction.h>
#include <tdepopupmenu.h>
#include <kdebug.h>
#include <tdeconfig.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include <dcopref.h>

#include "x11helper.h"
#include "kxkb.h"
#include "extension.h"
#include "rules.h"
#include "kxkbconfig.h"
#include "layoutmap.h"

#include "kxkb.moc"


KXKBApp::KXKBApp(bool allowStyles, bool GUIenabled)
    : TDEUniqueApplication(allowStyles, GUIenabled),
    m_prevWinId(X11Helper::UNKNOWN_WINDOW_ID),
    m_rules(nullptr),
    m_tray(nullptr),
    tWinModule(nullptr)
{
	X11Helper::initializeTranslations();
	XKBExtension *xkb = XKBExtension::the();
	connect(xkb, TQ_SIGNAL(groupChanged(uint)), this, TQ_SLOT(slotGroupChanged(uint)));
	connect(xkb, TQ_SIGNAL(optionsChanged()), this, TQ_SLOT(slotSyncXkbOptions()));

	m_layoutOwnerMap = new LayoutMap(kxkbConfig);

	// keep in sync with kcmlayout.cpp
	keys = new TDEGlobalAccel(this);
#include "kxkbbindings.cpp"

    connect( this, TQ_SIGNAL(settingsChanged(int)), TQ_SLOT(slotSettingsChanged(int)) );
    addKipcEventMask( KIPC::SettingsChanged );

#if WITH_TDEHWLIB
    TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
    connect(hwdevices, TQ_SIGNAL(hardwareAdded(TDEGenericDevice*)), this, TQ_SLOT(hardwareChanged(TDEGenericDevice*)));
    connect(hwdevices, TQ_SIGNAL(hardwareRemoved(TDEGenericDevice*)), this, TQ_SLOT(hardwareChanged(TDEGenericDevice*)));
    connect(hwdevices, TQ_SIGNAL(hardwareUpdated(TDEGenericDevice*)), this, TQ_SLOT(hardwareChanged(TDEGenericDevice*)));
#endif
}

KXKBApp::~KXKBApp()
{
	delete m_tray;
	delete m_rules;
	delete m_layoutOwnerMap;
	delete tWinModule;
	delete keys;
}

int KXKBApp::newInstance()
{
    readSettings();
    return 0;
}

void KXKBApp::readSettings()
{
    // Xkb options
    kxkbConfig.load(KxkbConfig::LOAD_INIT_OPTIONS);

    if (!kxkbConfig.m_useKxkb)
    {
        kdDebug() << "kxkb is disabled, applying xkb options and exiting" << endl;
        applyXkbOptions();
        quit();
        return;
    }

    kdDebug() << "applying xkb options and layouts" << endl;
    kxkbConfig.load(KxkbConfig::LOAD_ALL_OPTIONS);
    applyXkbOptions();

    // Active window watcher
    m_prevWinId = X11Helper::UNKNOWN_WINDOW_ID;

    if (kxkbConfig.m_switchingPolicy == SWITCH_POLICY_GLOBAL)
    {
        delete tWinModule;
        tWinModule = nullptr;
    }

    else
    {
        TQDesktopWidget desktopWidget;
        if (desktopWidget.numScreens() > 1 && !desktopWidget.isVirtualDesktop())
        {
            kdWarning() << "With non-virtual desktop only global switching policy supported on non-primary screens" << endl;
            //TODO: find out how to handle that
        }

        if (!tWinModule)
        {
            tWinModule = new TWinModule(nullptr, TWinModule::INFO_DESKTOP);
            connect(tWinModule, TQ_SIGNAL(activeWindowChanged(WId)), TQ_SLOT(windowChanged(WId)));
        }

        m_prevWinId = tWinModule->activeWindow();
        kdDebug() << "Active window " << m_prevWinId << endl;
    }

    // Init layout owner map
    m_layoutOwnerMap->reset();
    m_layoutOwnerMap->setCurrentWindow( m_prevWinId );

    // Init rules
    if (!m_rules)
    {
        m_rules = new XkbRules(false);
    }

    // Init layouts
    for (int i = 0; i < kxkbConfig.m_layouts.count(); i++)
    {
        LayoutUnit& layoutUnit = kxkbConfig.m_layouts[i];
    }

    m_currentLayout = kxkbConfig.m_layouts[0];
    setLayout(m_currentLayout);

    kdDebug() << "default layout is " << m_currentLayout.toPair() << endl;

    if (kxkbConfig.m_layouts.count() == 1 && !kxkbConfig.m_showSingle)
    {
        quit();
        return;
    }

    TDEGlobal::config()->reparseConfiguration(); // kcontrol modified kdeglobals

    // Init tray
    if (!m_tray)
    {
        m_tray = new KxkbSystemTray(&kxkbConfig);
        connect(m_tray, TQ_SIGNAL(menuActivated(int)), this, TQ_SLOT(menuActivated(int)));
        connect(m_tray, TQ_SIGNAL(toggled()), this, TQ_SLOT(nextLayout()));
    }

    m_tray->initLayoutList(kxkbConfig.m_layouts, *m_rules);
    m_tray->setCurrentLayout(m_currentLayout);
    m_tray->show();

    // Init keybindings
    keys->readSettings();
    keys->updateConnections();
}

void KXKBApp::applyXkbOptions()
{
    XkbOptions options = kxkbConfig.getKXkbOptions();
    if (!XKBExtension::the()->setXkbOptions(options)) {
        kdWarning() << "Setting XKB options failed!" << endl;
    }
}

void KXKBApp::hardwareChanged(TDEGenericDevice *dev)
{
#   if WITH_TDEHWLIB
    if (dev->type() == TDEGenericDeviceType::Keyboard)
    {
        TQTimer::singleShot(500, this, TQ_SLOT(applyXkbOptions()));
    }
#   endif
}

// kdcop
bool KXKBApp::setLayout(const TQString& layoutPair)
{
    return setLayout((LayoutUnit)layoutPair);
}

// Activates the keyboard layout specified by 'layoutUnit'
bool KXKBApp::setLayout(const LayoutUnit& layoutUnit)
{
    const int group = kxkbConfig.m_layouts.findIndex(layoutUnit);
    if (group >= 0) {
        return setLayout(group);
    }
    return false;
}

// Activates the keyboard layout specified by group number
bool KXKBApp::setLayout(const uint group)
{
    // If this group is already set, just show the notification and return
    if (XKBExtension::the()->getGroup() == group) {
        if (kxkbConfig.m_enableNotify) {
            showLayoutNotification();
        }
        return true;
    }

    bool ok = XKBExtension::the()->setGroup(group);
    if (!ok) {
        TQString layout = kxkbConfig.m_layouts[group].toPair();
        if (m_tray) {
            m_tray->setError(layout);
        }

        if (kxkbConfig.m_enableNotify) {
            showErrorNotification(layout);
        }
    }
    return ok;
}

void KXKBApp::nextLayout()
{
    const LayoutUnit& layout = m_layoutOwnerMap->getNextLayout().layoutUnit;
    setLayout(layout);
}

void KXKBApp::prevLayout()
{
    const LayoutUnit& layout = m_layoutOwnerMap->getPrevLayout().layoutUnit;
    setLayout(layout);
}

void KXKBApp::menuActivated(int id)
{
    if (id >= KxkbSystemTray::START_MENU_ID &&
        id <  KxkbSystemTray::START_MENU_ID + kxkbConfig.m_layouts.count())
    {
        setLayout(id - KxkbSystemTray::START_MENU_ID);
    }
    else if (id == KxkbSystemTray::CONFIG_MENU_ID)
    {
        TDEProcess p;
        p << "tdecmshell" << "keyboard_layout";
        p.start(TDEProcess::DontCare);
    }
    else if (id == KxkbSystemTray::HELP_MENU_ID)
    {
        invokeHelp(0, "kxkb");
    }
    else
    {
        quit();
    }
}

void KXKBApp::slotGroupChanged(uint group)
{
    if (!kxkbConfig.m_layouts.count()) {
      kdError() << "[kxkb] no layout found!" << endl;
      return;
    }

    if (group >= kxkbConfig.m_layouts.count()) {
        kdError() << "[kxkb] unknown group requested: " << group << endl;
        if (m_tray)
        {
            m_tray->setError(i18n("Unknown"));
        }
        if (kxkbConfig.m_enableNotify)
        {
            showErrorNotification(i18n("Unknown"));
        }
        return;
    }

    m_currentLayout = kxkbConfig.m_layouts[group];
    m_layoutOwnerMap->setCurrentLayout(m_currentLayout);

    if (m_tray) {
        m_tray->setCurrentLayout(m_currentLayout);
    }

    if (kxkbConfig.m_enableNotify) {
        showLayoutNotification();
    }
}

void KXKBApp::slotSyncXkbOptions()
{
	// Make sure the X11 server has had enough time to apply the change
	TQTimer::singleShot(100, this, TQ_SLOT(syncXkbOptions()));
}

void KXKBApp::syncXkbOptions()
{
	XkbOptions options = XKBExtension::the()->getServerOptions();
	if (kxkbConfig.setFromXkbOptions(options))
	{
		m_layoutOwnerMap->reset();
		if (m_tray)
		{
			m_tray->initLayoutList(kxkbConfig.m_layouts, *m_rules);
		}
	}
	slotGroupChanged(XKBExtension::the()->getGroup());
}

void KXKBApp::showLayoutNotification()
{
    bool useKMilo = kxkbConfig.m_notifyUseKMilo && isKMiloAvailable(),
         notificationSent = false;

    TQString layoutName(m_rules->getLayoutName(m_currentLayout));

    if (useKMilo) {
        DCOPRef kmilo("kded", "kmilod");
        if (kmilo.send("displayText(TQString,TQPixmap)", layoutName, miniIcon())) {
            notificationSent = true;
        }
    }

    if (!notificationSent) {
        WId wid = (m_tray ? m_tray->winId() : 0);
        KNotifyClient::event(wid, "LayoutChange", layoutName);
    }
}

void KXKBApp::showErrorNotification(TQString layout) {
    bool useKMilo = kxkbConfig.m_notifyUseKMilo && isKMiloAvailable(),
         notificationSent = false;

    if (useKMilo) {
        DCOPRef kmilo("kded", "kmilod");
        if (kmilo.send("displayText(TQString,TQPixmap)", i18n("Error changing keyboard layout to '%1'").arg(layout), miniIcon())) {
            notificationSent = true;
        }
    }

    if (!notificationSent) {
        WId wid = (m_tray ? m_tray->winId() : 0);
        KNotifyClient::event(wid, "Error");
    }
}

bool KXKBApp::isKMiloAvailable() {
    QCStringList modules;
    TQCString replyType;
    TQByteArray replyData;
    if (dcopClient()->call("kded", "kded", "loadedModules()",
                                 TQByteArray(), replyType, replyData))
    {
        if (replyType == "QCStringList") {
            TQDataStream reply(replyData, IO_ReadOnly);
            reply >> modules;
            return modules.contains("kmilod");
        }
    }
    return false;
}

// TODO: we also have to handle deleted windows
void KXKBApp::windowChanged(WId winId)
{
//	kdDebug() << "window switch" << endl;
	if( kxkbConfig.m_switchingPolicy == SWITCH_POLICY_GLOBAL ) { // should not happen actually
		kdDebug() << "windowChanged() signal in GLOBAL switching policy" << endl;
		return;
	}

	kdDebug() << "old WinId: " << m_prevWinId << ", new WinId: " << winId << endl;

	if( m_prevWinId != X11Helper::UNKNOWN_WINDOW_ID ) {	// saving layout from previous window
// 		m_layoutOwnerMap->setCurrentWindow(m_prevWinId);
		m_layoutOwnerMap->setCurrentLayout(m_currentLayout);
	}

	m_prevWinId = winId;

	if( winId != X11Helper::UNKNOWN_WINDOW_ID ) {
		m_layoutOwnerMap->setCurrentWindow(winId);
		const LayoutState& layoutState = m_layoutOwnerMap->getCurrentLayout();

		if( layoutState.layoutUnit != m_currentLayout ) {
			kdDebug() << "switching to " << layoutState.layoutUnit.toPair() << " for "  << winId << endl;
			setLayout(layoutState.layoutUnit);
		}
	}
}

void KXKBApp::slotSettingsChanged(int category)
{
	if (category == TDEApplication::SETTINGS_SHORTCUTS) {
		TDEGlobal::config()->reparseConfiguration(); // kcontrol modified kdeglobals
		keys->readSettings();
		keys->updateConnections();
    }
}

bool KXKBApp::x11EventFilter(XEvent *e) {
    // let the extension process the event and emit signals if necessary
    XKBExtension::the()->processXEvent(e);
    return TDEApplication::x11EventFilter(e);
}

const char *DESCRIPTION = I18N_NOOP("A utility to switch keyboard maps");

extern "C" TDE_EXPORT int kdemain(int argc, char *argv[])
{
    TDEAboutData about("kxkb", I18N_NOOP("TDE Keyboard Tool"), "1.0",
                     DESCRIPTION, TDEAboutData::License_LGPL,
                     "Copyright (C) 2001, S.R.Haque\n(C) 2002-2003, 2006 Andriy Rysin");
    TDECmdLineArgs::init(argc, argv, &about);
    KXKBApp::addCmdLineOptions();

    if (!KXKBApp::start())
        return 0;

    KXKBApp app;
    app.disableSessionManagement();
    app.exec();
    return 0;
}
