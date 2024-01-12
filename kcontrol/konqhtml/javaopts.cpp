// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// New configuration scheme for Java/JavaScript
// (c) Kalle Dalheimer 2000
// Redesign and cleanup
// (c) Daniel Molkentin 2000
// Big changes to accommodate per-domain settings
// (c) Leo Savernik 2002-2003

#include <config.h>
#include <tdelistview.h>
#include <kurlrequester.h>
#include <kdebug.h>
#include <klineedit.h>
#include <tdelocale.h>
#include <tdehtml_settings.h>
#include <knuminput.h>

#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqvgroupbox.h>
#include <tqlabel.h>

#include "htmlopts.h"
#include "policydlg.h"
#include "javaopts.h"

// == class JavaPolicies =====

JavaPolicies::JavaPolicies(TDEConfig* config, const TQString &group, bool global,
  		const TQString &domain) :
	Policies(config,group,global,domain,"java.","EnableJava") {
}

JavaPolicies::JavaPolicies() : Policies(0,TQString::null,false,
	TQString::null,TQString::null,TQString::null) {
}

JavaPolicies::~JavaPolicies() {
}

// == class KJavaOptions =====

KJavaOptions::KJavaOptions( TDEConfig* config, TQString group,
                            TQWidget *parent, const char *name )
    : TDECModule( parent, name ),
      _removeJavaScriptDomainAdvice(false),
      m_pConfig( config ),
      m_groupname( group ),
      java_global_policies(config,group,true),
      _removeJavaDomainSettings(false)
{
    TQVBoxLayout* toplevel = new TQVBoxLayout( this, 10, 5 );

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    TQVGroupBox* globalGB = new TQVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enableJavaGloballyCB = new TQCheckBox( i18n( "Enable Ja&va globally" ), globalGB );
    connect( enableJavaGloballyCB, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotChanged() ) );
    connect( enableJavaGloballyCB, TQ_SIGNAL( clicked() ), this, TQ_SLOT( toggleJavaControls() ) );


    /***************************************************************************
     ***************** Domain Specific Settings ********************************
     **************************************************************************/
    domainSpecific = new JavaDomainListView(m_pConfig,m_groupname,this,this);
    connect(domainSpecific,TQ_SIGNAL(changed(bool)),TQ_SLOT(slotChanged()));
    toplevel->addWidget( domainSpecific, 2 );

    /***************************************************************************
     ***************** Java Runtime Settings ***********************************
     **************************************************************************/
    TQVGroupBox* javartGB = new TQVGroupBox( i18n( "Java Runtime Settings" ), this );
    toplevel->addWidget( javartGB );

    TQWidget* checkboxes = new TQWidget( javartGB );
    TQGridLayout* grid = new TQGridLayout( checkboxes, 2, 2 );

    javaSecurityManagerCB = new TQCheckBox( i18n("&Use security manager" ), checkboxes );
    grid->addWidget( javaSecurityManagerCB, 0, 0 );
    connect( javaSecurityManagerCB, TQ_SIGNAL(toggled( bool )), this, TQ_SLOT(slotChanged()) );

    useKioCB = new TQCheckBox( i18n("Use &TDEIO"), checkboxes );
    grid->addWidget( useKioCB, 0, 1 );
    connect( useKioCB, TQ_SIGNAL(toggled( bool )), this, TQ_SLOT(slotChanged()) );

    enableShutdownCB = new TQCheckBox( i18n("Shu&tdown applet server when inactive"), checkboxes );
    grid->addWidget( enableShutdownCB, 1, 0 );
    connect( enableShutdownCB, TQ_SIGNAL(toggled( bool )), this, TQ_SLOT(slotChanged()) );
    connect( enableShutdownCB, TQ_SIGNAL(clicked()), this, TQ_SLOT(toggleJavaControls()) );

    TQHBox* secondsHB = new TQHBox( javartGB );
    serverTimeoutSB = new KIntNumInput( secondsHB );
    serverTimeoutSB->setRange( 0, 1000, 5 );
    serverTimeoutSB->setLabel( i18n("App&let server timeout:"), AlignLeft );
    serverTimeoutSB->setSuffix(i18n(" sec"));
    connect(serverTimeoutSB, TQ_SIGNAL(valueChanged(int)),this,TQ_SLOT(slotChanged()));

    TQHBox* pathHB = new TQHBox( javartGB );
    pathHB->setSpacing( 10 );
    TQLabel* pathLA = new TQLabel( i18n( "&Path to Java executable, or 'java':" ),
                                 pathHB );
    pathED = new  KURLRequester( pathHB );
    connect( pathED, TQ_SIGNAL(textChanged( const TQString& )), this, TQ_SLOT(slotChanged()) );
    pathLA->setBuddy( pathED );

    TQHBox* addArgHB = new TQHBox( javartGB );
    addArgHB->setSpacing( 10 );
    TQLabel* addArgLA = new TQLabel( i18n( "Additional Java a&rguments:" ), addArgHB  );
    addArgED = new TQLineEdit( addArgHB );
    connect( addArgED, TQ_SIGNAL(textChanged( const TQString& )), this, TQ_SLOT(slotChanged()) );
    addArgLA->setBuddy( addArgED );

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    TQWhatsThis::add( enableJavaGloballyCB, i18n("Enables the execution of scripts written in Java "
          "that can be contained in HTML pages. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );
    TQString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific Java policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling Java applets on pages sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    TQWhatsThis::add( domainSpecific->listView(), wtstr );
#if 0
    TQWhatsThis::add( domainSpecific->importButton(), i18n("Click this button to choose the file that contains "
                                          "the Java policies. These policies will be merged "
                                          "with the existing ones. Duplicate entries are ignored.") );
    TQWhatsThis::add( domainSpecific->exportButton(), i18n("Click this button to save the Java policy to a zipped "
                                          "file. The file, named <b>java_policy.tgz</b>, will be "
                                          "saved to a location of your choice." ) );
#endif
    TQWhatsThis::add( domainSpecific, i18n("Here you can set specific Java policies for any particular "
                                            "host or domain. To add a new policy, simply click the <i>New...</i> "
                                            "button and supply the necessary information requested by the "
                                            "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                            "button and choose the new policy from the policy dialog box. Clicking "
                                            "on the <i>Delete</i> button will remove the selected policy, causing the default "
                                            "policy setting to be used for that domain.") );
#if 0
                                            "The <i>Import</i> and <i>Export</i> "
                                            "button allows you to easily share your policies with other people by allowing "
                                            "you to save and retrieve them from a zipped file.") );
#endif

    TQWhatsThis::add( javaSecurityManagerCB, i18n( "Enabling the security manager will cause the jvm to run with a Security "
                                                  "Manager in place. This will keep applets from being able to read and "
                                                  "write to your file system, creating arbitrary sockets, and other actions "
                                                  "which could be used to compromise your system. Disable this option at your "
                                                  "own risk. You can modify your $HOME/.java.policy file with the Java "
                                                  "policytool utility to give code downloaded from certain sites more "
                                                  "permissions." ) );

    TQWhatsThis::add( useKioCB, i18n( "Enabling this will cause the jvm to use TDEIO for network transport ") );

    TQWhatsThis::add( pathED, i18n("Enter the path to the java executable. If you want to use the jre in "
                                  "your path, simply leave it as 'java'. If you need to use a different jre, "
                                  "enter the path to the java executable (e.g. /usr/lib/jdk/bin/java), "
                                  "or the path to the directory that contains 'bin/java' (e.g. /opt/IBMJava2-13).") );

    TQWhatsThis::add( addArgED, i18n("If you want special arguments to be passed to the virtual machine, enter them here.") );

    TQString shutdown = i18n("When all the applets have been destroyed, the applet server should shut down. "
                                           "However, starting the jvm takes a lot of time. If you would like to "
                                           "keep the java process running while you are "
                                           "browsing, you can set the timeout value to whatever you like. To keep "
                                           "the java process running for the whole time that the konqueror process is, "
                                           "leave the Shutdown Applet Server checkbox unchecked.");
    TQWhatsThis::add( serverTimeoutSB, shutdown);
    TQWhatsThis::add( enableShutdownCB, shutdown);
    // Finally do the loading
    load();
}

void KJavaOptions::load()
{
	load( false );
}

void KJavaOptions::load(bool useDefaults)
{
	 m_pConfig->setReadDefaults( useDefaults );

    // *** load ***
    java_global_policies.load();
    bool bJavaGlobal      = java_global_policies.isFeatureEnabled();
    bool bSecurityManager = m_pConfig->readBoolEntry( "UseSecurityManager", true );
    bool bUseKio = m_pConfig->readBoolEntry( "UseKio", false );
    bool bServerShutdown  = m_pConfig->readBoolEntry( "ShutdownAppletServer", true );
    int  serverTimeout    = m_pConfig->readNumEntry( "AppletServerTimeout", 60 );
#if defined(PATH_JAVA)
    TQString sJavaPath     = m_pConfig->readPathEntry( "JavaPath", PATH_JAVA );
#else
    TQString sJavaPath     = m_pConfig->readPathEntry( "JavaPath", "/usr/bin/java" );
#endif

    if( sJavaPath == "/usr/lib/jdk" )
        sJavaPath = "java";

    if( m_pConfig->hasKey( "JavaDomains" ) )
    	domainSpecific->initialize(m_pConfig->readListEntry("JavaDomains"));
    else if( m_pConfig->hasKey( "JavaDomainSettings" ) ) {
        domainSpecific->updateDomainListLegacy( m_pConfig->readListEntry("JavaDomainSettings") );
	_removeJavaDomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy( m_pConfig->readListEntry("JavaScriptDomainAdvice") );
	_removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    enableJavaGloballyCB->setChecked( bJavaGlobal );
    javaSecurityManagerCB->setChecked( bSecurityManager );
    useKioCB->setChecked( bUseKio );

    addArgED->setText( m_pConfig->readEntry( "JavaArgs" ) );
    pathED->lineEdit()->setText( sJavaPath );

    enableShutdownCB->setChecked( bServerShutdown );
    serverTimeoutSB->setValue( serverTimeout );

    toggleJavaControls();
    emit changed( useDefaults );
}

void KJavaOptions::defaults()
{
	load( true );
}

void KJavaOptions::save()
{
    java_global_policies.save();
    m_pConfig->writeEntry( "JavaArgs", addArgED->text() );
    m_pConfig->writePathEntry( "JavaPath", pathED->lineEdit()->text() );
    m_pConfig->writeEntry( "UseSecurityManager", javaSecurityManagerCB->isChecked() );
    m_pConfig->writeEntry( "UseKio", useKioCB->isChecked() );
    m_pConfig->writeEntry( "ShutdownAppletServer", enableShutdownCB->isChecked() );
    m_pConfig->writeEntry( "AppletServerTimeout", serverTimeoutSB->value() );

    domainSpecific->save(m_groupname,"JavaDomains");

    if (_removeJavaDomainSettings) {
      m_pConfig->deleteEntry("JavaDomainSettings");
      _removeJavaDomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
    emit changed( false );
}

void KJavaOptions::slotChanged()
{
    emit changed(true);
}


void KJavaOptions::toggleJavaControls()
{
    bool isEnabled = true; //enableJavaGloballyCB->isChecked();

    java_global_policies.setFeatureEnabled( enableJavaGloballyCB->isChecked() );
    javaSecurityManagerCB->setEnabled( isEnabled );
    useKioCB->setEnabled( isEnabled );
    addArgED->setEnabled( isEnabled );
    pathED->setEnabled( isEnabled );
    enableShutdownCB->setEnabled( isEnabled );

    serverTimeoutSB->setEnabled( enableShutdownCB->isChecked() && isEnabled );
}

// == class JavaDomainListView =====

JavaDomainListView::JavaDomainListView(TDEConfig *config,const TQString &group,
	KJavaOptions *options,TQWidget *parent,const char *name)
	: DomainListView(config,i18n( "Doma&in-Specific" ), parent, name),
	group(group), options(options) {
}

JavaDomainListView::~JavaDomainListView() {
}

void JavaDomainListView::updateDomainListLegacy(const TQStringList &domainConfig)
{
    domainSpecificLV->clear();
    JavaPolicies pol(config,group,false);
    pol.defaults();
    for ( TQStringList::ConstIterator it = domainConfig.begin();
          it != domainConfig.end(); ++it)
    {
        TQString domain;
        TDEHTMLSettings::KJavaScriptAdvice javaAdvice;
        TDEHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
        TDEHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
	if (javaAdvice != TDEHTMLSettings::KJavaScriptDunno) {
          TQListViewItem* index = new TQListViewItem( domainSpecificLV, domain,
                                                  i18n(TDEHTMLSettings::adviceToStr(javaAdvice))  );
          pol.setDomain(domain);
          pol.setFeatureEnabled(javaAdvice != TDEHTMLSettings::KJavaScriptReject);
          domainPolicies[index] = new JavaPolicies(pol);
	}
    }
}

void JavaDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies *pol) {
  TQString caption;
  switch (trigger) {
    case AddButton: caption = i18n( "New Java Policy" );
      pol->setFeatureEnabled(!options->enableJavaGloballyCB->isChecked());
      break;
    case ChangeButton: caption = i18n( "Change Java Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("&Java policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a Java policy for "
                                    "the above host or domain."));
  pDlg.refresh();
}

JavaPolicies *JavaDomainListView::createPolicies() {
  return new JavaPolicies(config,group,false);
}

JavaPolicies *JavaDomainListView::copyPolicies(Policies *pol) {
  return new JavaPolicies(*static_cast<JavaPolicies *>(pol));
}

#include "javaopts.moc"
