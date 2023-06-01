//
// KDE Shortcut config module
//
// Copyright (c)  Mark Donohoe 1998
// Copyright (c)  Matthias Ettrich 1998
// Converted to generic key configuration module, Duncan Haldane 1998.
// Layout fixes copyright (c) 2000 Preston Brown <pbrown@kde.org>

#include <config.h>
#include <stdlib.h>

#include <unistd.h>

#include <tqlabel.h>
#include <tqdir.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqcheckbox.h>
#include <tqregexp.h>

#include <kdebug.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>
#include <tdemessagebox.h>
#include <kseparator.h>
#include <dcopclient.h>
#include <tdeapplication.h>
#include <kkey_x11.h>	// Used in KKeyModule::init()

#include "keyconfig.h"
#include "keyconfig.moc"

#define KICKER_ALL_BINDINGS

//----------------------------------------------------------------------------

KKeyModule::KKeyModule( TQWidget *parent, bool isGlobal, bool bSeriesOnly, bool bSeriesNone, const char *name )
  : TQWidget( parent, name )
{
	init( isGlobal, bSeriesOnly, bSeriesNone );
}

KKeyModule::KKeyModule( TQWidget *parent, bool isGlobal, const char *name )
  : TQWidget( parent, name )
{
	init( isGlobal, false, false );
}

void KKeyModule::init( bool isGlobal, bool _bSeriesOnly, bool bSeriesNone )
{
  TQString wtstr;

  KeyType = isGlobal ? "global" : "standard";

  bSeriesOnly = _bSeriesOnly;

  kdDebug(125) << "KKeyModule::init() - Get default key bindings." << endl;
  if ( KeyType == "global" ) {
    TDEAccelActions* keys = &actions;
// see also KKeyModule::init() below !!!
#define NOSLOTS
#define TDEShortcuts TDEAccelShortcuts
#include "../../twin/twinbindings.cpp"
#include "../../kicker/kicker/core/kickerbindings.cpp"
#include "../../kicker/taskbar/taskbarbindings.cpp"
#include "../../kdesktop/kdesktopbindings.cpp"
#include "../../klipper/klipperbindings.cpp"
#include "../../kxkb/kxkbbindings.cpp"
#undef TDEShortcuts
    KeyScheme = "Global Key Scheme";
    KeySet    = "Global Keys";
    // Sorting Hack: I'll re-write the module once feature-adding begins again.
    if( bSeriesOnly || bSeriesNone ) {
	for( uint i = 0; i < actions.size(); i++ ) {
		TQString sConfigKey = actions[i].m_sName;
		//kdDebug(125) << "sConfigKey: " << sConfigKey << endl;
		int iLastSpace = sConfigKey.findRev( ' ' );
		bool bIsNum = false;
		if( iLastSpace >= 0 )
			sConfigKey.mid( iLastSpace+1 ).toInt( &bIsNum );

		kdDebug(125) << "sConfigKey: " << sConfigKey
			<< " bIsNum: " << bIsNum
			<< " bSeriesOnly: " << bSeriesOnly << endl;
		if( ((bSeriesOnly && !bIsNum) || (bSeriesNone && bIsNum)) && !sConfigKey.contains( ':' ) ) {
			actions.removeAction( sConfigKey );
			i--;
		}
	}
    }
  }

  if ( KeyType == "standard" ) {
    for(uint i=0; i<TDEStdAccel::NB_STD_ACCELS; i++) {
      TDEStdAccel::StdAccel id = (TDEStdAccel::StdAccel)i;
      actions.insertAction( TDEStdAccel::action(id),
                          TDEStdAccel::description(id),
                          TDEStdAccel::defaultKey3(id),
                          TDEStdAccel::defaultKey4(id) );
    }

    KeyScheme = "Standard Key Scheme";
    KeySet    = "Keys";
  }

  //kdDebug(125) << "KKeyModule::init() - Read current key bindings from config." << endl;
  //actions.readActions( KeySet );

  sFileList = new TQStringList();
  sList = new TQListBox( this );

  //readSchemeNames();
  sList->setCurrentItem( 0 );
  connect( sList, TQT_SIGNAL( highlighted( int ) ),
           TQT_SLOT( slotPreviewScheme( int ) ) );

  TQLabel *label = new TQLabel( sList, i18n("&Key Scheme"), this );

  wtstr = i18n("Here you can see a list of the existing key binding schemes with 'Current scheme'"
    " referring to the settings you are using right now. Select a scheme to use, remove or"
    " change it.");
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( sList, wtstr );

  addBt = new TQPushButton(  i18n("&Save Scheme..."), this );
  connect( addBt, TQT_SIGNAL( clicked() ), TQT_SLOT( slotAdd() ) );
  TQWhatsThis::add(addBt, i18n("Click here to add a new key bindings scheme. You will be prompted for a name."));

  removeBt = new TQPushButton(  i18n("&Remove Scheme"), this );
  removeBt->setEnabled(FALSE);
  connect( removeBt, TQT_SIGNAL( clicked() ), TQT_SLOT( slotRemove() ) );
  TQWhatsThis::add( removeBt, i18n("Click here to remove the selected key bindings scheme. You can not"
    " remove the standard system wide schemes, 'Current scheme' and 'TDE default'.") );

  // Hack to get this setting only displayed once.  It belongs in main.cpp instead.
  // That move will take a lot of UI redesigning, though, so i'll do it once CVS
  //  opens up for feature commits again. -- ellis
  /* Needed to remove because this depended upon non-BC changes in KeyEntry.*/
  // If this is the "Global Keys" section of the Trinity Control Center:
  if( isGlobal && !bSeriesOnly ) {
	preferMetaBt = new TQCheckBox( i18n("Prefer 4-modifier defaults"), this );
	if( !KKeySequence::keyboardHasMetaKey() )
		preferMetaBt->setEnabled( false );
	preferMetaBt->setChecked( KKeySequence::useFourModifierKeys() );
	connect( preferMetaBt, TQT_SIGNAL(clicked()), TQT_SLOT(slotPreferMeta()) );
	TQWhatsThis::add( preferMetaBt, i18n("If your keyboard has a Meta key, but you would "
		"like TDE to prefer the 3-modifier configuration defaults, then this option "
		"should be unchecked.") );
  } else
	preferMetaBt = 0;

  KSeparator* line = new KSeparator( KSeparator::HLine, this );

  kc = new KeyChooserSpec( actions, this, isGlobal );
  connect( kc, TQT_SIGNAL(keyChange()), this, TQT_SLOT(slotKeyChange()) );

  readScheme();

  TQGridLayout *topLayout = new TQGridLayout( this, 6, 2,
                                            KDialog::marginHint(),
                                            KDialog::spacingHint());
  topLayout->addWidget(label, 0, 0);
  topLayout->addMultiCellWidget(sList, 1, 2, 0, 0);
  topLayout->addWidget(addBt, 1, 1);
  topLayout->addWidget(removeBt, 2, 1);
  if( preferMetaBt )
    topLayout->addWidget(preferMetaBt, 3, 0);
  topLayout->addMultiCellWidget(line, 4, 4, 0, 1);
  topLayout->addRowSpacing(3, 15);
  topLayout->addMultiCellWidget(kc, 5, 5, 0, 1);

  setMinimumSize(topLayout->sizeHint());
}

KKeyModule::~KKeyModule (){
  //kdDebug() << "KKeyModule destructor" << endl;
    delete kc;
    delete sFileList;
}

bool KKeyModule::writeSettings( const TQString& sGroup, TDEConfig* pConfig )
{
	kc->commitChanges();
	actions.writeActions( sGroup, pConfig, true, false );
	return true;
}

bool KKeyModule::writeSettingsGlobal( const TQString& sGroup )
{
	kc->commitChanges();
	actions.writeActions( sGroup, 0, true, true );
	return true;
}

void KKeyModule::load()
{
  kc->listSync();
}

/*void KKeyModule::save()
{
  if( preferMetaBt )
    KKeySequence::useFourModifierKeys( preferMetaBt->isChecked() );

  kc->commitChanges();
  actions.writeActions( KeySet, 0, true, true );
  if ( KeyType == "global" ) {
    if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
    // TODO: create a reconfigureKeys() method.
    kapp->dcopClient()->send("twin", "", "reconfigure()", "");
    kapp->dcopClient()->send("kdesktop", "", "configure()", "");
    kapp->dcopClient()->send("kicker", "Panel", "configure()", "");
  }
}*/

void KKeyModule::defaults()
{
  if( preferMetaBt )
    preferMetaBt->setChecked( false );
  KKeySequence::useFourModifierKeys( false );
  kc->allDefault();
}

/*void KKeyModule::slotRemove()
{
  TQString kksPath =
        TDEGlobal::dirs()->saveLocation("data", "kcmkeys/" + KeyType);

  TQDir d( kksPath );
  if (!d.exists()) // what can we do?
    return;

  d.setFilter( TQDir::Files );
  d.setSorting( TQDir::Name );
  d.setNameFilter("*.kksrc");

  uint ind = sList->currentItem();

  if ( !d.remove( *sFileList->at( ind ) ) ) {
    KMessageBox::sorry( 0,
                        i18n("This key scheme could not be removed.\n"
                             "Perhaps you do not have permission to alter the file "
                             "system where the key scheme is stored." ));
    return;
  }

  sList->removeItem( ind );
  sFileList->remove( sFileList->at(ind) );
}*/

void KKeyModule::slotKeyChange()
{
	emit keyChange();
	//emit keysChanged( &dict );
}

/*void KKeyModule::slotSave( )
{
    KSimpleConfig config(*sFileList->at( sList->currentItem() ) );
    //  global=true is necessary in order to
    //  let both 'Global Shortcuts' and 'Shortcut Sequences' be
    //  written to the same scheme file.
    kc->commitChanges();
    actions.writeActions( KeyScheme, &config, KeyType == "global", KeyType == "global" );
}*/

void KKeyModule::slotPreferMeta()
{
	kc->setPreferFourModifierKeys( preferMetaBt->isChecked() );
}

void KKeyModule::readScheme( int index )
{
  kdDebug(125) << "readScheme( " << index << " )\n";
  if( index == 1 )
    kc->allDefault( false );
  //else if( index == 2 )
  //  kc->allDefault( true );
  else {
    TDEConfigBase* config = 0;
    if( index == 0 )	config = new TDEConfig( "kdeglobals" );
    //else		config = new KSimpleConfig( *sFileList->at( index ), true );

    actions.readActions( (index == 0) ? KeySet : KeyScheme, config );
    kc->listSync();
    delete config;
  }
}

/*void KKeyModule::slotAdd()
{
  TQString sName;

  if ( sList->currentItem() >= nSysSchemes )
     sName = sList->currentText();
  SaveScm ss( 0,  "save scheme", sName );

  bool nameValid;
  TQString sFile;
  int exists = -1;

  do {

    nameValid = TRUE;

    if ( ss.exec() ) {
      sName = ss.nameLine->text();
      if ( sName.stripWhiteSpace().isEmpty() )
        return;

      sName = sName.simplifyWhiteSpace();
      sFile = sName;

      int ind = 0;
      while ( ind < (int) sFile.length() ) {

        // parse the string for first white space

        ind = sFile.find(" ");
        if (ind == -1) {
          ind = sFile.length();
          break;
        }

        // remove from string

        sFile.remove( ind, 1);

        // Make the next letter upper case

        TQString s = sFile.mid( ind, 1 );
        s = s.upper();
        sFile.replace( ind, 1, s );

      }

      exists = -1;
      for ( int i = 0; i < (int) sList->count(); i++ ) {
        if ( sName.lower() == (sList->text(i)).lower() ) {
          exists = i;

          int result = KMessageBox::warningContinueCancel( 0,
               i18n("A key scheme with the name '%1' already exists.\n"
                    "Do you want to overwrite it?\n").arg(sName),
		   i18n("Save Key Scheme"),
                   i18n("Overwrite"));
          if (result == KMessageBox::Continue)
             nameValid = true;
          else
             nameValid = false;
        }
      }
    } else return;

  } while ( nameValid == FALSE );

  disconnect( sList, TQT_SIGNAL( highlighted( int ) ), this,
              TQT_SLOT( slotPreviewScheme( int ) ) );


  TQString kksPath = TDEGlobal::dirs()->saveLocation("data", "kcmkeys/");

  TQDir d( kksPath );
  if ( !d.exists() )
    if ( !d.mkdir( kksPath ) ) {
      tqWarning("KKeyModule: Could not make directory to store user info.");
      return;
    }

  kksPath +=  KeyType ;
  kksPath += "/";

  d.setPath( kksPath );
  if ( !d.exists() )
    if ( !d.mkdir( kksPath ) ) {
      tqWarning("KKeyModule: Could not make directory to store user info.");
      return;
    }

  sFile.prepend( kksPath );
  sFile += ".kksrc";
  if (exists == -1)
  {
     sList->insertItem( sName );
     sList->setFocus();
     sList->setCurrentItem( sList->count()-1 );
     sFileList->append( sFile );
  }
  else
  {
     sList->setFocus();
     sList->setCurrentItem( exists );
  }

  KSimpleConfig *config =
    new KSimpleConfig( sFile );

  config->setGroup( KeyScheme );
  config->writeEntry( "Name", sName );
  delete config;

  slotSave();

  connect( sList, TQT_SIGNAL( highlighted( int ) ), this,
           TQT_SLOT( slotPreviewScheme( int ) ) );

  slotPreviewScheme( sList->currentItem() );
}*/

/*void KKeyModule::slotPreviewScheme( int indx )
{
  readScheme( indx );

  // Set various appropriate for the scheme

  if ( indx < nSysSchemes ||
       (*sFileList->at(indx)).contains( "/global-" ) ||
       (*sFileList->at(indx)).contains( "/app-" ) ) {
    removeBt->setEnabled( FALSE );
  } else {
    removeBt->setEnabled( TRUE );
  }
}*/

/*void KKeyModule::readSchemeNames( )
{
  TQStringList schemes = TDEGlobal::dirs()->findAllResources("data", "kcmkeys/" + KeyType + "/*.kksrc");
  //TQRegExp r( "-kde[34].kksrc$" );
  TQRegExp r( "-trinity.kksrc$" );

  sList->clear();
  sFileList->clear();
  sList->insertItem( i18n("Current Scheme"), 0 );
  sFileList->append( "Not a kcsrc file" );
  sList->insertItem( i18n("TDE Traditional"), 1 );
  sFileList->append( "Not a kcsrc file" );
  //sList->insertItem( i18n("TDE Extended (With 'Win' Key)"), 2 );
  //sList->insertItem( i18n("TDE Default for 4 Modifiers (Meta/Alt/Ctrl/Shift)"), 2 );
  //sFileList->append( "Not a kcsrc file" );
  nSysSchemes = 2;

  // This for system files
  for ( TQStringList::ConstIterator it = schemes.begin(); it != schemes.end(); ++it) {
    // KPersonalizer relies on .kksrc files containing all the keyboard shortcut
    //  schemes for various setups.  It also requires the TDE defaults to be in
    //  a .kksrc file.  The TDE defaults shouldn't be listed here.
    //if( r.search( *it ) != -1 )
    //   continue;

    KSimpleConfig config( *it, true );
    // TODO: Put 'Name' in "Settings" group
    config.setGroup( KeyScheme );
    TQString str = config.readEntry( "Name" );

    sList->insertItem( str );
    sFileList->append( *it );
  }
}*/

/*void KKeyModule::updateKeys( const TDEAccelActions* map_P )
    {
    kc->updateKeys( map_P );
    }*/

// write all the global keys to kdeglobals
// this is needed to be able to check for conflicts with global keys in app's keyconfig
// dialogs, kdeglobals is empty as long as you don't apply any change in controlcenter/keys
void KKeyModule::init()
{
  kdDebug(125) << "KKeyModule::init()\n";

  /*kdDebug(125) << "KKeyModule::init() - Initialize # Modifier Keys Settings\n";
  TDEConfigGroupSaver cgs( TDEGlobal::config(), "Keyboard" );
  TQString fourMods = TDEGlobal::config()->readEntry( "Use Four Modifier Keys", TDEAccel::keyboardHasMetaKey() ? "true" : "false" );
  TDEAccel::useFourModifierKeys( fourMods == "true" );
  bool bUseFourModifierKeys = TDEAccel::useFourModifierKeys();
  TDEGlobal::config()->writeEntry( "User Four Modifier Keys", bUseFourModifierKeys ? "true" : "false", true, true );
  */
  TDEAccelActions* keys = new TDEAccelActions();

  kdDebug(125) << "KKeyModule::init() - Load Included Bindings\n";
// this should match the included files above
#define NOSLOTS
#define TDEShortcuts TDEAccelShortcuts
#include "../../klipper/klipperbindings.cpp"
#include "../../twin/twinbindings.cpp"
#include "../../kicker/kicker/core/kickerbindings.cpp"
#include "../../kicker/taskbar/taskbarbindings.cpp"
#include "../../kdesktop/kdesktopbindings.cpp"
#include "../../kxkb/kxkbbindings.cpp"
#undef TDEShortcuts

  kdDebug(125) << "KKeyModule::init() - Read Config Bindings\n";
  keys->readActions( "Global Keys" );

  {
    KSimpleConfig cfg( "kdeglobals" );
    cfg.deleteGroup( "Global Keys" );
  }

  kdDebug(125) << "KKeyModule::init() - Write Config Bindings\n";
  keys->writeActions( "Global Keys", 0, true, true );
}

//-----------------------------------------------------------------
// KeyChooserSpec
//-----------------------------------------------------------------

KeyChooserSpec::KeyChooserSpec( TDEAccelActions& actions, TQWidget* parent, bool bGlobal )
    : KKeyChooser( actions, parent, bGlobal, false, true ), m_bGlobal( bGlobal )
    {
    //if( global )
    //    globalDict()->clear(); // don't check against global keys twice
    }

/*void KeyChooserSpec::updateKeys( const TDEAccelActions* map_P )
    {
    if( global )
        {
        stdDict()->clear();
        for( TDEAccelActions::ConstIterator gIt( map_P->begin());
             gIt != map_P->end();
             ++gIt )
            {
            int* keyCode = new int;
            *keyCode = ( *gIt ).aConfigKeyCode;
            stdDict()->insert( gIt.key(), keyCode);
            }
        }
    else
        {
        globalDict()->clear();
        for( TDEAccelActions::ConstIterator gIt( map_P->begin());
             gIt != map_P->end();
             ++gIt )
            {
            int* keyCode = new int;
            *keyCode = ( *gIt ).aConfigKeyCode;
            globalDict()->insert( gIt.key(), keyCode);
            }
        }
    }
*/
