/*
 * shortcuts.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 * Copyright (c) 2001 Ellis Whitehead <ellis@kde.org>
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

#include "shortcuts.h"

#include <stdlib.h>

#include <tqdir.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqcheckbox.h>

#include <tdeapplication.h>
#include <kdebug.h>
#include <kipc.h>
#include <kkeynative.h>
#include <kinputdialog.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdeshortcutlist.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

ShortcutsModule::ShortcutsModule( TQWidget *parent, const char *name )
: TQWidget( parent, name )
{
	initGUI();
}

ShortcutsModule::~ShortcutsModule()
{
	delete m_pListGeneral;
	delete m_pListSequence;
	delete m_pListApplication;
}

// Called when [Reset] is pressed
void ShortcutsModule::load()
{
	kdDebug(125) << "ShortcutsModule::load()" << endl;
	slotSchemeCur();
}

// When [Apply] or [OK] are clicked.
void ShortcutsModule::save()
{
	kdDebug(125) << "ShortcutsModule::save()" << endl;

	// FIXME: This isn't working.  Why? -- ellis, 2002/01/27
	// Check for old group,
	if( TDEGlobal::config()->hasGroup( "Keys" ) ) {
		TDEGlobal::config()->deleteGroup( "Keys", true, true );
	}
	TDEGlobal::config()->sync();

	m_pkcGeneral->commitChanges();
	m_pkcSequence->commitChanges();
	m_pkcApplication->save();

	m_actionsGeneral.writeActions( "Global Shortcuts", 0, true, true );
	m_actionsSequence.writeActions( "Global Shortcuts", 0, true, true );

	KIPC::sendMessageAll( KIPC::SettingsChanged, TDEApplication::SETTINGS_SHORTCUTS );
}

void ShortcutsModule::defaults()
{
	m_pkcGeneral->allDefault();
	m_pkcSequence->allDefault();
	m_pkcApplication->allDefault();
}

TQString ShortcutsModule::quickHelp() const
{
  return i18n("<h1>Key Bindings</h1> Using key bindings you can configure certain actions to be"
    " triggered when you press a key or a combination of keys, e.g. Ctrl+C is normally bound to"
    " 'Copy'. TDE allows you to store more than one 'scheme' of key bindings, so you might want"
    " to experiment a little setting up your own scheme while you can still change back to the"
    " TDE defaults.<p> In the tab 'Global Shortcuts' you can configure non-application specific"
    " bindings like how to switch desktops or maximize a window. In the tab 'Application Shortcuts'"
    " you will find bindings typically used in applications, such as copy and paste.");
}

void ShortcutsModule::initGUI()
{
	TQString kde_winkeys_env_dir = TDEGlobal::dirs()->localtdedir() + "/env/";

	kdDebug(125) << "A-----------" << endl;
	TDEAccelActions* keys = &m_actionsGeneral;
// see also TDEShortcutsModule::init() below !!!
#define NOSLOTS
#define KICKER_ALL_BINDINGS
#include "../../twin/twinbindings.cpp"
#include "../../kicker/kicker/core/kickerbindings.cpp"
#include "../../kicker/taskbar/taskbarbindings.cpp"
#include "../../kdesktop/kdesktopbindings.cpp"
#include "../../klipper/klipperbindings.cpp"
#include "../../kxkb/kxkbbindings.cpp"

	kdDebug(125) << "B-----------" << endl;
	m_actionsSequence.init( m_actionsGeneral );

	kdDebug(125) << "C-----------" << endl;
	createActionsGeneral();
	kdDebug(125) << "D-----------" << endl;
	createActionsSequence();
	kdDebug(125) << "E-----------" << endl;

	kdDebug(125) << "F-----------" << endl;
	TQVBoxLayout* pVLayout = new TQVBoxLayout( this, KDialog::marginHint() );

	pVLayout->addSpacing( KDialog::marginHint() );

	// (o) [Current      ] <Remove>   ( ) New <Save>

	TQHBoxLayout *pHLayout = new TQHBoxLayout( pVLayout, KDialog::spacingHint() );
	TQButtonGroup* pGroup = new TQButtonGroup( this );
	pGroup->hide();

	m_prbPre = new TQRadioButton( "", this );
	connect( m_prbPre, TQT_SIGNAL(clicked()), TQT_SLOT(slotSchemeCur()) );
	pGroup->insert( m_prbPre );
	pHLayout->addWidget( m_prbPre );

	m_pcbSchemes = new KComboBox( this );
	m_pcbSchemes->setMinimumWidth( 100 );
	m_pcbSchemes->setSizePolicy( TQSizePolicy::Preferred, TQSizePolicy::Fixed );
	connect( m_pcbSchemes, TQT_SIGNAL(activated(int)), TQT_SLOT(slotSelectScheme(int)) );
	pHLayout->addWidget( m_pcbSchemes );

	pHLayout->addSpacing( KDialog::marginHint() );

	m_pbtnRemove = new TQPushButton( i18n("&Remove"), this );
	m_pbtnRemove->setEnabled( false );
	connect( m_pbtnRemove, TQT_SIGNAL(clicked()), TQT_SLOT(slotRemoveScheme()) );
	TQWhatsThis::add( m_pbtnRemove, i18n("Click here to remove the selected key bindings scheme. You cannot"
		" remove the standard system-wide schemes 'Current scheme' and 'TDE default'.") );
	pHLayout->addWidget( m_pbtnRemove );

	pHLayout->addSpacing( KDialog::marginHint() * 3 );

	m_prbNew = new TQRadioButton( i18n("New scheme"), this );
	m_prbNew->setEnabled( false );
	pGroup->insert( m_prbNew );
	pHLayout->addWidget( m_prbNew );

	m_pbtnSave = new TQPushButton( i18n("&Save..."), this );
	m_pbtnSave->setEnabled( false );
	TQWhatsThis::add( m_pbtnSave, i18n("Click here to add a new key bindings scheme. You will be prompted for a name.") );
	connect( m_pbtnSave, TQT_SIGNAL(clicked()), TQT_SLOT(slotSaveSchemeAs()) );
	pHLayout->addWidget( m_pbtnSave );

	pHLayout->addStretch( 1 );

	m_pTab = new TQTabWidget( this );
	m_pTab->setMargin( KDialog::marginHint() );
	pVLayout->addWidget( m_pTab );

	// See if ~/.trinity/env/win-key.sh exists
	TQFile f( kde_winkeys_env_dir + "win-key.sh" );
	if ( f.exists() == false ) {
		// No, it does not, so Win is a modifier
		m_bUseRmWinKeys = true;
	}
	else {
		// Yes, it does, so Win is a key
		m_bUseRmWinKeys = false;
	}
	m_pListGeneral = new TDEAccelShortcutList( m_actionsGeneral, true );

	m_pkcGeneral = new KKeyChooser( m_pListGeneral, this, KKeyChooser::Global, false );
	m_pkcGeneral->resize (m_pkcGeneral->sizeHint() );
	if (system("xmodmap 1> /dev/null 2> /dev/null") == 0) {
		m_useRmWinKeys = new TQCheckBox( i18n("Use Win key as modifier (uncheck to bind Win key to Menu)"), this );
		m_useRmWinKeys->resize( m_useRmWinKeys->sizeHint() );
		m_useRmWinKeys->setChecked( m_bUseRmWinKeys );
		pVLayout->addWidget( m_useRmWinKeys, 1, 0 );
		connect( m_useRmWinKeys, TQT_SIGNAL(clicked()), TQT_SLOT(slotUseRmWinKeysClicked()) );
	}
	m_pTab->addTab( m_pkcGeneral, i18n("&Global Shortcuts") );
	connect( m_pkcGeneral, TQT_SIGNAL(keyChange()), TQT_SLOT(slotKeyChange()) );

	m_pListSequence = new TDEAccelShortcutList( m_actionsSequence, true );
	m_pkcSequence = new KKeyChooser( m_pListSequence, this, KKeyChooser::Global, false );
	m_pTab->addTab( m_pkcSequence, i18n("Shortcut Se&quences") );
	connect( m_pkcSequence, TQT_SIGNAL(keyChange()), TQT_SLOT(slotKeyChange()) );

	m_pListApplication = new TDEStdAccel::ShortcutList;
	m_pkcApplication = new KKeyChooser( m_pListApplication, this, KKeyChooser::Standard, false );
	m_pTab->addTab( m_pkcApplication, i18n("App&lication Shortcuts") );
	connect( m_pkcApplication, TQT_SIGNAL(keyChange()), TQT_SLOT(slotKeyChange()) );

	kdDebug(125) << "G-----------" << endl;
	readSchemeNames();

	kdDebug(125) << "I-----------" << endl;
	slotSchemeCur();

	kdDebug(125) << "J-----------" << endl;
}

void ShortcutsModule::createActionsGeneral()
{
	TDEAccelActions& actions = m_actionsGeneral;

	for( uint i = 0; i < actions.count(); i++ ) {
		TQString sConfigKey = actions[i].name();
		//kdDebug(125) << "sConfigKey: " << sConfigKey << endl;
		int iLastSpace = sConfigKey.findRev( ' ' );
		bool bIsNum = false;
		if( iLastSpace >= 0 )
			sConfigKey.mid( iLastSpace+1 ).toInt( &bIsNum );

		//kdDebug(125) << "sConfigKey: " << sConfigKey
		//	<< " bIsNum: " << bIsNum << endl;
		if( bIsNum && !sConfigKey.contains( ':' ) ) {
			actions[i].setConfigurable( false );
			actions[i].setName( TQString::null );
		}
	}
}

void ShortcutsModule::createActionsSequence()
{
	TDEAccelActions& actions = m_actionsSequence;

	for( uint i = 0; i < actions.count(); i++ ) {
		TQString sConfigKey = actions[i].name();
		//kdDebug(125) << "sConfigKey: " << sConfigKey << endl;
		int iLastSpace = sConfigKey.findRev( ' ' );
		bool bIsNum = false;
		if( iLastSpace >= 0 )
			sConfigKey.mid( iLastSpace+1 ).toInt( &bIsNum );

		//kdDebug(125) << "sConfigKey: " << sConfigKey
		//	<< " bIsNum: " << bIsNum << endl;
		if( !bIsNum && !sConfigKey.contains( ':' ) ) {
			actions[i].setConfigurable( false );
			actions[i].setName( TQString::null );
		}
	}
}

void ShortcutsModule::readSchemeNames()
{
	TQStringList schemes = TDEGlobal::dirs()->findAllResources("data", "kcmkeys/*.kksrc");

	m_pcbSchemes->clear();
	m_rgsSchemeFiles.clear();

	i18n("User-Defined Scheme");
	m_pcbSchemes->insertItem( i18n("Current Scheme") );
	m_rgsSchemeFiles.append( "cur" );

	// This for system files
	for ( TQStringList::ConstIterator it = schemes.begin(); it != schemes.end(); ++it) {
	// KPersonalizer relies on .kksrc files containing all the keyboard shortcut
	//  schemes for various setups.  It also requires the TDE defaults to be in
	//  a .kksrc file.  The TDE defaults shouldn't be listed here.
		//if( r.search( *it ) != -1 )
		//   continue;

		KSimpleConfig config( *it, true );
		config.setGroup( "Settings" );
		TQString str = config.readEntry( "Name" );

		m_pcbSchemes->insertItem( str );
		m_rgsSchemeFiles.append( *it );
	}
}

void ShortcutsModule::resizeEvent( TQResizeEvent * )
{
	//m_pTab->setGeometry(0,0,width(),height());
}

void ShortcutsModule::slotSchemeCur()
{
	kdDebug(125) << "ShortcutsModule::slotSchemeCur()" << endl;
	//m_pcbSchemes->setCurrentItem( 0 );
	slotSelectScheme();
}

void ShortcutsModule::slotKeyChange()
{
	kdDebug(125) << "ShortcutsModule::slotKeyChange()" << endl;
	m_prbNew->setEnabled( true );
	m_prbNew->setChecked( true );
	m_pbtnSave->setEnabled( true );
	emit changed( true );
}

void ShortcutsModule::slotSelectScheme( int )
{
	i18n("Your current changes will be lost if you load another scheme before saving this one.");
	kdDebug(125) << "ShortcutsModule::slotSelectScheme( " << m_pcbSchemes->currentItem() << " )" << endl;
	TQString sFilename = m_rgsSchemeFiles[ m_pcbSchemes->currentItem() ];

	if( sFilename == "cur" ) {
		// TODO: remove nulls params
		m_pkcGeneral->syncToConfig( "Global Shortcuts", 0, true );
		m_pkcSequence->syncToConfig( "Global Shortcuts", 0, true );
		m_pkcApplication->syncToConfig( "Shortcuts", 0, false );
	} else {
		KSimpleConfig config( sFilename );
		config.setGroup( "Settings" );
		//m_sBaseSchemeFile = config.readEntry( "Name" );

		// If the user's keyboard layout doesn't support the Win key,
		//  but this layout scheme requires it,
		if( !KKeyNative::keyboardHasWinKey()
		    && config.readBoolEntry( "Uses Win Modifier", false ) ) {
			// TODO: change "Win" to Win's label.
			int ret = KMessageBox::warningContinueCancel( this,
				i18n("This scheme requires the \"%1\" modifier key, which is not "
				"available on your keyboard layout. Do you wish to view it anyway?" )
				.arg(i18n("Win")) );
			if( ret == KMessageBox::Cancel )
				return;
		}

		m_pkcGeneral->syncToConfig( "Global Shortcuts", &config, true );
		m_pkcSequence->syncToConfig( "Global Shortcuts", &config, true );
		m_pkcApplication->syncToConfig( "Shortcuts", &config, false );
	}

	m_prbPre->setChecked( true );
	m_prbNew->setEnabled( false );
	m_pbtnSave->setEnabled( false );
	emit changed(true);
}

void ShortcutsModule::slotSaveSchemeAs()
{
	TQString sName, sFile;
	bool bNameValid, ok;
	int iScheme = -1;

	sName = m_pcbSchemes->currentText();

	do {
		bNameValid = true;

		sName = KInputDialog::getText( i18n( "Save Key Scheme" ),
			i18n( "Enter a name for the key scheme:" ), sName, &ok, this );

		if( ok ) {
			sName = sName.simplifyWhiteSpace();
			sFile = sName;

			int ind = 0;
			while( ind < (int) sFile.length() ) {
				// parse the string for first white space
				ind = sFile.find(" ");
				if( ind == -1 ) {
					ind = sFile.length();
					break;
				}

				// remove from string
				sFile.remove( ind, 1 );

				// Make the next letter upper case
				TQString s = sFile.mid( ind, 1 );
				s = s.upper();
				sFile.replace( ind, 1, s );
			}

			iScheme = -1;
			for( int i = 0; i < (int) m_pcbSchemes->count(); i++ ) {
				if( sName.lower() == (m_pcbSchemes->text(i)).lower() ) {
					iScheme = i;

					int result = KMessageBox::warningContinueCancel( 0,
					i18n("A key scheme with the name '%1' already exists;\n"
						"do you want to overwrite it?\n").arg(sName),
					i18n("Save Key Scheme"),
					i18n("Overwrite"));
					bNameValid = (result == KMessageBox::Continue);
				}
			}
		} else
			return;
	} while( !bNameValid );

	disconnect( m_pcbSchemes, TQT_SIGNAL(activated(int)), this, TQT_SLOT(slotSelectScheme(int)) );

	TQString kksPath = TDEGlobal::dirs()->saveLocation( "data", "kcmkeys/" );

	TQDir dir( kksPath );
	if( !dir.exists() && !dir.mkdir( kksPath ) ) {
		tqWarning("TDEShortcutsModule: Could not make directory to store user info.");
		return;
	}

	sFile.prepend( kksPath );
	sFile += ".kksrc";
	if( iScheme == -1 ) {
		m_pcbSchemes->insertItem( sName );
		//m_pcbSchemes->setFocus();
		m_pcbSchemes->setCurrentItem( m_pcbSchemes->count()-1 );
		m_rgsSchemeFiles.append( sFile );
	} else {
		//m_pcbSchemes->setFocus();
		m_pcbSchemes->setCurrentItem( iScheme );
	}

	KSimpleConfig *config = new KSimpleConfig( sFile );

	config->setGroup( "Settings" );
	config->writeEntry( "Name", sName );
	delete config;

	saveScheme();

	connect( m_pcbSchemes, TQT_SIGNAL(activated(int)), TQT_SLOT(slotSelectScheme(int)) );
	slotSelectScheme();
}

void ShortcutsModule::saveScheme()
{
	TQString sFilename = m_rgsSchemeFiles[ m_pcbSchemes->currentItem() ];
	KSimpleConfig config( sFilename );

	m_pkcGeneral->commitChanges();
	m_pkcSequence->commitChanges();
	m_pkcApplication->commitChanges();

	m_pListGeneral->writeSettings( "Global Shortcuts", &config, true );
	m_pListSequence->writeSettings( "Global Shortcuts", &config, true );
	m_pListApplication->writeSettings( "Shortcuts", &config, true );
}

void ShortcutsModule::slotRemoveScheme()
{
}

void ShortcutsModule::slotUseRmWinKeysClicked()
{
	TQString kde_winkeys_env_dir = TDEGlobal::dirs()->localtdedir() + "/env/";

	// See if ~/.trinity/env/win-key.sh exists
	TQFile f( kde_winkeys_env_dir + "win-key.sh" );
	if ( f.exists() == false ) {
		// No, it does not, so Win is currently a modifier
		if (m_useRmWinKeys->isChecked() == false) {
			// Create the file
			if ( f.open( IO_WriteOnly ) ) {
				TQTextStream stream( &f );
				stream << "xmodmap -e 'keycode 133=Menu'" << "\n";
				stream << "xmodmap -e 'keycode 134=Menu'" << "\n";
				f.close();
				system("xmodmap -e 'keycode 133=Menu'");
				system("xmodmap -e 'keycode 134=Menu'");
			}
		}
	}
	else {
		// Yes, it does, so Win is currently a key
		m_bUseRmWinKeys = false;
		if (m_useRmWinKeys->isChecked() == true) {
			// Remove the file
			f.remove();
			// Update key mappings
			system("xmodmap -e 'keycode 133=Super_L'");
			system("xmodmap -e 'keycode 134=Super_R'");
		}
	}
}

#include "shortcuts.moc"
