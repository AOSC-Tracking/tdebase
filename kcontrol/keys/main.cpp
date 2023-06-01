/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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

#include <tqlayout.h>

#include <kdebug.h>
#include <tdelocale.h>
#include <ksimpleconfig.h>

#include "commandShortcuts.h"
#include "main.h"
#include "modifiers.h"
#include "shortcuts.h"
#include "khotkeys.h"

/*
| Shortcut Schemes | Modifier Keys |

o Current scheme  o New scheme  o Pre-set scheme
| TDE Traditional |v|| <Save Scheme...> <Remove Scheme>
[] Prefer 4-modifier defaults

o Current scheme
o New scheme       <Save Scheme>
o Pre-set scheme   <Remove Scheme>
  | TDE Traditional |v||
[] Prefer 4-modifier defaults

Global Shortcuts
*/
KeyModule::KeyModule( TQWidget *parent, const char *name )
: TDECModule( parent, name )
{
    setQuickHelp( i18n("<h1>Keyboard Shortcuts</h1> Using shortcuts you can configure certain actions to be"
    " triggered when you press a key or a combination of keys, e.g. Ctrl+C is normally bound to"
    " 'Copy'. TDE allows you to store more than one 'scheme' of shortcuts, so you might want"
    " to experiment a little setting up your own scheme, although you can still change back to the"
    " TDE defaults.<p> In the 'Global Shortcuts' tab you can configure non-application-specific"
    " bindings, like how to switch desktops or maximize a window; in the 'Application Shortcuts' tab"
    " you will find bindings typically used in applications, such as copy and paste."));

	initGUI();
}

KeyModule::~KeyModule()
{
    KHotKeys::cleanup();
}

void KeyModule::initGUI()
{
	m_pTab = new TQTabWidget( this );
	TQVBoxLayout *l = new TQVBoxLayout(this);
	l->addWidget(m_pTab);

	m_pShortcuts = new ShortcutsModule( this );
	m_pTab->addTab( m_pShortcuts, i18n("Shortcut Schemes") );
	connect( m_pShortcuts, TQT_SIGNAL(changed(bool)), TQT_SIGNAL(changed(bool)) );

	m_pCommandShortcuts = new CommandShortcutsModule ( this );
	m_pTab->addTab( m_pCommandShortcuts, i18n("Command Shortcuts") );
	connect( m_pCommandShortcuts, TQT_SIGNAL(changed(bool)), TQT_SIGNAL(changed(bool)) );
    connect( m_pTab, TQT_SIGNAL(currentChanged(TQWidget*)), m_pCommandShortcuts, TQT_SLOT(showing(TQWidget*)) );

	m_pModifiers = new ModifiersModule( this );
	m_pTab->addTab( m_pModifiers, i18n("Modifier Keys") );
	connect( m_pModifiers, TQT_SIGNAL(changed(bool)), TQT_SIGNAL(changed(bool)) );
}

void KeyModule::load()
{
   load( false );
}

// Called when [Reset] is pressed
void KeyModule::load( bool useDefaults )
{
	kdDebug(125) << "KeyModule::load()" << endl;
	m_pShortcuts->load();
	m_pCommandShortcuts->load();
	m_pModifiers->load( useDefaults );

   emit changed( useDefaults );
}

// When [Apply] or [OK] are clicked.
void KeyModule::save()
{
	kdDebug(125) << "KeyModule::save()" << endl;
	m_pShortcuts->save();
	m_pCommandShortcuts->save();
	m_pModifiers->save();
}

void KeyModule::defaults()
{
	kdDebug(125) << "KeyModule::defaults()" << endl;
	m_pShortcuts->defaults();
	m_pCommandShortcuts->defaults();
	m_pModifiers->defaults();
}

void KeyModule::resizeEvent( TQResizeEvent * )
{
	m_pTab->setGeometry( 0, 0, width(), height() );
}

TQString KeyModule::handbookSection() const
{
	int index = m_pTab->currentPageIndex();
	if (index == 0) {
		return "key-bindings-intro";
	}
	else if (index == 1) {
		return "key-bindings-shortcuts";
	}
	else if (index == 2) {
		return "key-bindings-modifiers";
	}
	else {
		return TQString::null;
	}
}

//----------------------------------------------------

extern "C"
{
  KDE_EXPORT TDECModule *create_keys(TQWidget *parent, const char * /*name*/)
  {
	// What does this do?  Why not insert klipper and kxkb, too? --ellis, 2002/01/15
	TDEGlobal::locale()->insertCatalogue("twin");
	TDEGlobal::locale()->insertCatalogue("kdesktop");
	TDEGlobal::locale()->insertCatalogue("kicker");
	return new KeyModule(parent, "kcmkeys");
  }

  KDE_EXPORT void initModifiers()
  {
	kdDebug(125) << "KeyModule::initModifiers()" << endl;

	TDEConfigGroupSaver cgs( TDEGlobal::config(), "Keyboard" );
	bool bMacSwap = TDEGlobal::config()->readBoolEntry( "Mac Modifier Swap", false );
	if( bMacSwap )
		ModifiersModule::setupMacModifierKeys();
  }

  KDE_EXPORT void init_keys()
  {
	kdDebug(125) << "KeyModule::init()\n";

	/*kdDebug(125) << "KKeyModule::init() - Initialize # Modifier Keys Settings\n";
	TDEConfigGroupSaver cgs( TDEGlobal::config(), "Keyboard" );
	TQString fourMods = TDEGlobal::config()->readEntry( "Use Four Modifier Keys", TDEAccel::keyboardHasMetaKey() ? "true" : "false" );
	TDEAccel::useFourModifierKeys( fourMods == "true" );
	bool bUseFourModifierKeys = TDEAccel::useFourModifierKeys();
	TDEGlobal::config()->writeEntry( "User Four Modifier Keys", bUseFourModifierKeys ? "true" : "false", true, true );
	*/
	TDEAccelActions* keys = new TDEAccelActions();

	kdDebug(125) << "KeyModule::init() - Load Included Bindings\n";
// this should match the included files above
#define NOSLOTS
#define SHIFT Qt::SHIFT
#define CTRL Qt::CTRL
#define ALT Qt::ALT
#include "../../klipper/klipperbindings.cpp"
#include "../../twin/twinbindings.cpp"
#define KICKER_ALL_BINDINGS
#include "../../kicker/kicker/core/kickerbindings.cpp"
#include "../../kicker/taskbar/taskbarbindings.cpp"
#include "../../kdesktop/kdesktopbindings.cpp"
#include "../../kxkb/kxkbbindings.cpp"

  // Write all the global keys to kdeglobals.
  // This is needed to be able to check for conflicts with global keys in app's keyconfig
  // dialogs, kdeglobals is empty as long as you don't apply any change in controlcenter/keys.
  // However, avoid writing at every KDE startup, just update them after every rebuild of this file.
        TDEConfigGroup group( TDEGlobal::config(), "Global Shortcuts" );
        if( group.readEntry( "Defaults timestamp" ) != __DATE__ __TIME__ ) {
	    kdDebug(125) << "KeyModule::init() - Read Config Bindings\n";
	    // Check for old group,
	    if( TDEGlobal::config()->hasGroup( "Global Keys" ) ) {
		keys->readActions( "Global Keys" );
		TDEGlobal::config()->deleteGroup( "Global Keys", true, true );
	    }
	    keys->readActions( "Global Shortcuts" );
            TDEGlobal::config()->deleteGroup( "Global Shortcuts", true, true );

	    kdDebug(125) << "KeyModule::init() - Write Config Bindings\n";
	    keys->writeActions( "Global Shortcuts", 0, true, true );
            group.writeEntry( "Defaults timestamp", __DATE__ __TIME__, true, true );
        }
	delete keys;

	initModifiers();
  }
}

#include "main.moc"
