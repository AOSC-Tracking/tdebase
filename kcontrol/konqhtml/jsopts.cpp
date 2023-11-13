// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// New configuration scheme for JavaScript
// (C) Kalle Dalheimer 2000
// Major cleanup & Java/JS settings splitted
// (c) Daniel Molkentin 2000
// Big changes to accommodate per-domain settings
// (c) Leo Savernik 2002-2003

#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqvgroupbox.h>
#include <tdeconfig.h>
#include <tdelistview.h>
#include <kdebug.h>
#include <kurlrequester.h>

#if defined Q_WS_X11 && !defined K_WS_QTONLY
#include <X11/Xlib.h>
#endif

#include "htmlopts.h"
#include "policydlg.h"

#include <tdelocale.h>

#include "jsopts.h"

#include "jsopts.moc"

// == class KJavaScriptOptions =====

KJavaScriptOptions::KJavaScriptOptions( TDEConfig* config, TQString group, TQWidget *parent,
										const char *name ) :
  TDECModule( parent, name ),
  _removeJavaScriptDomainAdvice(false),
   m_pConfig( config ), m_groupname( group ),
  js_global_policies(config,group,true,TQString::null),
  _removeECMADomainSettings(false)
{
  TQVBoxLayout* toplevel = new TQVBoxLayout( this, 10, 5 );

  // the global checkbox
  TQGroupBox* globalGB = new TQGroupBox( 2, TQt::Vertical, i18n( "Global Settings" ), this );
  toplevel->addWidget( globalGB );

  enableJavaScriptGloballyCB = new TQCheckBox( i18n( "Ena&ble JavaScript globally" ), globalGB );
  TQWhatsThis::add( enableJavaScriptGloballyCB, i18n("Enables the execution of scripts written in ECMA-Script "
        "(also known as JavaScript) that can be contained in HTML pages. "
        "Note that, as with any browser, enabling scripting languages can be a security problem.") );
  connect( enableJavaScriptGloballyCB, TQT_SIGNAL( clicked() ), TQT_SLOT( changed() ) );
  connect( enableJavaScriptGloballyCB, TQT_SIGNAL( clicked() ), this, TQT_SLOT( slotChangeJSEnabled() ) );

  reportErrorsCB = new TQCheckBox( i18n( "Report &errors" ), globalGB );
  TQWhatsThis::add( reportErrorsCB, i18n("Enables the reporting of errors that occur when JavaScript "
	"code is executed.") );
  connect( reportErrorsCB, TQT_SIGNAL( clicked() ), TQT_SLOT( changed() ) );

  jsDebugWindow = new TQCheckBox( i18n( "Enable debu&gger" ), globalGB );
  TQWhatsThis::add( jsDebugWindow, i18n( "Enables builtin JavaScript debugger." ) );
  connect( jsDebugWindow, TQT_SIGNAL( clicked() ), TQT_SLOT( changed() ) );

  // the domain-specific listview
  domainSpecific = new JSDomainListView(m_pConfig,m_groupname,this,this);
  connect(domainSpecific,TQT_SIGNAL(changed(bool)),TQT_SLOT(changed()));
  toplevel->addWidget( domainSpecific, 2 );

  TQWhatsThis::add( domainSpecific, i18n("Here you can set specific JavaScript policies for any particular "
                                          "host or domain. To add a new policy, simply click the <i>New...</i> "
                                          "button and supply the necessary information requested by the "
                                          "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                          "button and choose the new policy from the policy dialog box. Clicking "
                                          "on the <i>Delete</i> button will remove the selected policy causing the default "
                                          "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                          "button allows you to easily share your policies with other people by allowing "
                                          "you to save and retrieve them from a zipped file.") );

  TQString wtstr = i18n("This box contains the domains and hosts you have set "
                       "a specific JavaScript policy for. This policy will be used "
                       "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
                       "domains or hosts. <p>Select a policy and use the controls on "
                       "the right to modify it.");
  TQWhatsThis::add( domainSpecific->listView(), wtstr );

  TQWhatsThis::add( domainSpecific->importButton(), i18n("Click this button to choose the file that contains "
                                        "the JavaScript policies. These policies will be merged "
                                        "with the existing ones. Duplicate entries are ignored.") );
  TQWhatsThis::add( domainSpecific->exportButton(), i18n("Click this button to save the JavaScript policy to a zipped "
                                        "file. The file, named <b>javascript_policy.tgz</b>, will be "
                                        "saved to a location of your choice." ) );

  // the frame containing the JavaScript policies settings
  js_policies_frame = new JSPoliciesFrame(&js_global_policies,
  		i18n("Global JavaScript Policies"),this);
  toplevel->addWidget(js_policies_frame);
  connect(js_policies_frame, TQT_SIGNAL(changed()), TQT_SLOT(changed()));

  // Finally do the loading
  load();
}

void KJavaScriptOptions::load()
{
	load( false );
}

void KJavaScriptOptions::load( bool useDefaults )
{
	 m_pConfig->setReadDefaults( useDefaults );

    // *** load ***
    m_pConfig->setGroup(m_groupname);

    if( m_pConfig->hasKey( "ECMADomains" ) )
	domainSpecific->initialize(m_pConfig->readListEntry("ECMADomains"));
    else if( m_pConfig->hasKey( "ECMADomainSettings" ) ) {
        domainSpecific->updateDomainListLegacy( m_pConfig->readListEntry( "ECMADomainSettings" ) );
	_removeECMADomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy(m_pConfig->readListEntry("JavaScriptDomainAdvice") );
	_removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    js_policies_frame->load();
    enableJavaScriptGloballyCB->setChecked(
    		js_global_policies.isFeatureEnabled());
    reportErrorsCB->setChecked( m_pConfig->readBoolEntry("ReportJavaScriptErrors",false));
    jsDebugWindow->setChecked( m_pConfig->readBoolEntry( "EnableJavaScriptDebug",false ) );
    
	 emit changed(useDefaults);
}

void KJavaScriptOptions::defaults()
{
	load( true );
}

void KJavaScriptOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "ReportJavaScriptErrors", reportErrorsCB->isChecked() );
    m_pConfig->writeEntry( "EnableJavaScriptDebug", jsDebugWindow->isChecked() );

    domainSpecific->save(m_groupname,"ECMADomains");
    js_policies_frame->save();

    if (_removeECMADomainSettings) {
      m_pConfig->deleteEntry("ECMADomainSettings");
      _removeECMADomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
    emit changed(false);
}

void KJavaScriptOptions::slotChangeJSEnabled() {
  js_global_policies.setFeatureEnabled(enableJavaScriptGloballyCB->isChecked());
}

// == class JSDomainListView =====

JSDomainListView::JSDomainListView(TDEConfig *config,const TQString &group,
	KJavaScriptOptions *options, TQWidget *parent,const char *name)
	: DomainListView(config,i18n( "Do&main-Specific" ), parent, name),
	group(group), options(options) {
}

JSDomainListView::~JSDomainListView() {
}

void JSDomainListView::updateDomainListLegacy(const TQStringList &domainConfig)
{
    domainSpecificLV->clear();
    JSPolicies pol(config,group,false);
    pol.defaults();
    for (TQStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it) {
      TQString domain;
      TDEHTMLSettings::KJavaScriptAdvice javaAdvice;
      TDEHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
      TDEHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
      if (javaScriptAdvice != TDEHTMLSettings::KJavaScriptDunno) {
        TQListViewItem *index =
          new TQListViewItem( domainSpecificLV, domain,
                i18n(TDEHTMLSettings::adviceToStr(javaScriptAdvice)) );

        pol.setDomain(domain);
        pol.setFeatureEnabled(javaScriptAdvice != TDEHTMLSettings::KJavaScriptReject);
        domainPolicies[index] = new JSPolicies(pol);
      }
    }
}

void JSDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies *pol) {
  JSPolicies *jspol = static_cast<JSPolicies *>(pol);
  TQString caption;
  switch (trigger) {
    case AddButton:
      caption = i18n( "New JavaScript Policy" );
      jspol->setFeatureEnabled(!options->enableJavaScriptGloballyCB->isChecked());
      break;
    case ChangeButton: caption = i18n( "Change JavaScript Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("JavaScript policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a JavaScript policy for "
                                          "the above host or domain."));
  JSPoliciesFrame *panel = new JSPoliciesFrame(jspol,i18n("Domain-Specific "
  				"JavaScript Policies"),pDlg.mainWidget());
  panel->refresh();
  pDlg.addPolicyPanel(panel);
  pDlg.refresh();
}

JSPolicies *JSDomainListView::createPolicies() {
  return new JSPolicies(config,group,false);
}

JSPolicies *JSDomainListView::copyPolicies(Policies *pol) {
  return new JSPolicies(*static_cast<JSPolicies *>(pol));
}


