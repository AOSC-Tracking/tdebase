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
#include <stdlib.h>
#include <assert.h>

#include <tqregexp.h>
#include <tqfile.h>
#include <tqstringlist.h>
#include <tqimage.h>

#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdeglobal.h>
#include <kglobalaccel.h>
#include <tdelocale.h>
#include <kprocess.h>
#include <twinmodule.h>
#include <twin.h>
#include <tdetempfile.h>
#include <kstandarddirs.h>
#include <kipc.h>
#include <tdeaction.h>
#include <tdepopupmenu.h>
#include <kdebug.h>
#include <tdeconfig.h>

#include "x11helper.h"
#include "kxkb.h"
#include "extension.h"
#include "rules.h"
#include "kxkbconfig.h"
#include "layoutmap.h"

#include "kxkb.moc"


KXKBApp::KXKBApp(bool allowStyles, bool GUIenabled)
    : KUniqueApplication(allowStyles, GUIenabled),
    m_prevWinId(X11Helper::UNKNOWN_WINDOW_ID),
    m_rules(NULL),
    m_tray(NULL),
    kWinModule(NULL)
{
	m_extension = new XKBExtension();
    if( !m_extension->init() ) {
		kdDebug() << "xkb initialization failed, exiting..." << endl;
		::exit(1);
    }
    connect(m_extension, TQT_SIGNAL(groupChanged(uint)), this, TQT_SLOT(slotGroupChanged(uint)));

	m_layoutOwnerMap = new LayoutMap(kxkbConfig);

    connect( this, TQT_SIGNAL(settingsChanged(int)), TQT_SLOT(slotSettingsChanged(int)) );
    addKipcEventMask( KIPC::SettingsChanged );
}


KXKBApp::~KXKBApp()
{
    delete m_tray;
    delete m_rules;
    delete m_extension;
	delete m_layoutOwnerMap;
	delete kWinModule;
}

int KXKBApp::newInstance()
{
    if (settingsRead()) {
        layoutApply();
    }

    return 0;
}

bool KXKBApp::settingsRead()
{
	XkbOptions options = kxkbConfig.getXkbOptions();
	if( !m_extension->setXkbOptions(options) ) {
		kdDebug() << "Setting XKB options failed!" << endl;
	}

    if ( kxkbConfig.m_useKxkb == false ) {
        kapp->quit();
        return false;
    }
	
	m_prevWinId = X11Helper::UNKNOWN_WINDOW_ID;
	
	if( kxkbConfig.m_switchingPolicy == SWITCH_POLICY_GLOBAL ) {
		delete kWinModule;
		kWinModule = NULL;
	}
	else {
		TQDesktopWidget desktopWidget;
		if( desktopWidget.numScreens() > 1 && desktopWidget.isVirtualDesktop() == false ) {
			kdWarning() << "With non-virtual desktop only global switching policy supported on non-primary screens" << endl;
			//TODO: find out how to handle that
		}
		
		if( kWinModule == NULL ) {
			kWinModule = new KWinModule(0, KWinModule::INFO_DESKTOP);
			connect(kWinModule, TQT_SIGNAL(activeWindowChanged(WId)), TQT_SLOT(windowChanged(WId)));
		}
		m_prevWinId = kWinModule->activeWindow();
		kdDebug() << "Active window " << m_prevWinId << endl;
	}
	
	m_layoutOwnerMap->reset();
	m_layoutOwnerMap->setCurrentWindow( m_prevWinId );

	if( m_rules == NULL )
		m_rules = new XkbRules(false);
	
	for(int ii=0; ii<(int)kxkbConfig.m_layouts.count(); ii++) {
		LayoutUnit& layoutUnit = kxkbConfig.m_layouts[ii];
	}
	
	m_currentLayout = kxkbConfig.m_layouts[0];
	kdDebug() << "default layout is " << m_currentLayout.toPair() << endl;
	
	if( kxkbConfig.m_layouts.count() == 1 && !kxkbConfig.m_showSingle) {
		kapp->quit();
		return false;
 	}

	initTray();
	
	TDEGlobal::config()->reparseConfiguration(); // kcontrol modified kdeglobals

	return true;
}

void KXKBApp::initTray()
{
	if( !m_tray )
	{
		KSystemTray* sysTray = new KxkbSystemTray();
		TDEPopupMenu* popupMenu = sysTray->contextMenu();
	//	popupMenu->insertTitle( kapp->miniIcon(), kapp->caption() );

		m_tray = new KxkbLabelController(sysTray, popupMenu);
 		connect(popupMenu, TQT_SIGNAL(activated(int)), this, TQT_SLOT(menuActivated(int)));
		connect(sysTray, TQT_SIGNAL(toggled()), this, TQT_SLOT(nextLayout()));
	}
	
	m_tray->setShowFlag(kxkbConfig.m_showFlag);
	m_tray->initLayoutList(kxkbConfig.m_layouts, *m_rules);
	m_tray->setCurrentLayout(m_currentLayout);
	m_tray->show();
}

// This function activates the keyboard layout specified by the
// configuration members (m_currentLayout)
void KXKBApp::layoutApply()
{
    setLayout(m_currentLayout);
}

// kdcop
bool KXKBApp::setLayout(const TQString& layoutPair)
{
	const LayoutUnit layoutUnitKey(layoutPair);
	if( kxkbConfig.m_layouts.contains(layoutUnitKey) ) {
		return setLayout( *kxkbConfig.m_layouts.find(layoutUnitKey) );
	}
	return false;
}

// Activates the keyboard layout specified by 'layoutUnit'
bool KXKBApp::setLayout(const LayoutUnit& layoutUnit)
{
	uint group = kxkbConfig.m_layouts.findIndex(layoutUnit);
	bool res = m_extension->setGroup(group);
	if (res) {
		m_currentLayout = layoutUnit;
	}

    if (m_tray) {
		if (res) {
			m_tray->setCurrentLayout(layoutUnit);
		} else {
			m_tray->setError(layoutUnit.toPair());
		}
	}

	return res;
}
// Activates the keyboard layout specified by group number
bool KXKBApp::setLayout(const uint group)
{
	bool res = m_extension->setGroup(group);
	if (res) {
		m_currentLayout = kxkbConfig.m_layouts[group];
	}

	if (m_tray) {
		if (res)
			m_tray->setCurrentLayout(m_currentLayout);
		else
			m_tray->setError(m_currentLayout.toPair());
	}

	return res;
}


void KXKBApp::nextLayout()
{
	const LayoutUnit& layout = m_layoutOwnerMap->getNextLayout().layoutUnit;
	setLayout(layout);
}

void KXKBApp::menuActivated(int id)
{
	if( KxkbLabelController::START_MENU_ID <= id 
		   && id < KxkbLabelController::START_MENU_ID + (int)kxkbConfig.m_layouts.count() )
	{
		const LayoutUnit& layout = kxkbConfig.m_layouts[id - KxkbLabelController::START_MENU_ID];
		m_layoutOwnerMap->setCurrentLayout( layout );
		setLayout( layout );
	}
	else if (id == KxkbLabelController::CONFIG_MENU_ID)
    {
        TDEProcess p;
        p << "tdecmshell" << "keyboard_layout";
        p.start(TDEProcess::DontCare);
	}
	else if (id == KxkbLabelController::HELP_MENU_ID)
	{
		TDEApplication::kApplication()->invokeHelp(0, "kxkb");
	}
//	else
//	{
//		quit();
//	}
}

void KXKBApp::slotGroupChanged(uint group) {
	m_currentLayout = kxkbConfig.m_layouts[group];
	m_tray->setCurrentLayout(m_currentLayout);
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
    if ( category != TDEApplication::SETTINGS_SHORTCUTS)
		return;

    TDEGlobal::config()->reparseConfiguration(); // kcontrol modified kdeglobals
}

bool KXKBApp::x11EventFilter(XEvent *e) {
    // let the extension process the event and emit signals if necessary
    m_extension->processXEvent(e);
    return TDEApplication::x11EventFilter(e);
}

const char * DESCRIPTION =
  I18N_NOOP("A utility to switch keyboard maps");

extern "C" KDE_EXPORT int kdemain(int argc, char *argv[])
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