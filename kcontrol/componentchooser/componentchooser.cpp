/***************************************************************************
                          componentchooser.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License verstion 2 as    *
 *   published by the Free Software Foundation                             *
 *                                                                         *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>

#include "componentchooser.h"
#include "componentchooser.moc"

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqradiobutton.h>
#include <tqwidgetstack.h>

#include <dcopclient.h>

#include <kapplication.h>
#include <kemailsettings.h>
#include <kipc.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kopenwith.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <ktrader.h>
#include <kurlrequester.h>

class MyListBoxItem: public TQListBoxText
{
public:
	MyListBoxItem(const TQString& text, const TQString &file):TQListBoxText(text),File(file){}
	virtual ~MyListBoxItem(){;}
	TQString File;
};


//BEGIN  General kpart based Component selection

CfgComponent::CfgComponent(TQWidget *parent):ComponentConfig_UI(parent),CfgPlugin(){
	m_lookupDict.setAutoDelete(true);
	m_revLookupDict.setAutoDelete(true);
	connect(ComponentSelector,TQT_SIGNAL(activated(const TQString&)),this,TQT_SLOT(slotComponentChanged(const TQString&)));
}

CfgComponent::~CfgComponent(){}

void CfgComponent::slotComponentChanged(const TQString&) {
	emit changed(true);
}

void CfgComponent::save(KConfig *cfg) {
		// yes, this can happen if there are NO KTrader offers for this component
		if (!m_lookupDict[ComponentSelector->currentText()])
			return;

		TQString ServiceTypeToConfigure=cfg->readEntry("ServiceTypeToConfigure");
		KConfig *store = new KConfig(cfg->readPathEntry("storeInFile","null"));
		store->setGroup(cfg->readEntry("valueSection"));
		store->writePathEntry(cfg->readEntry("valueName","kcm_componenchooser_null"),*m_lookupDict[ComponentSelector->currentText()]);
		store->sync();
		delete store;
		emit changed(false);
}

void CfgComponent::load(KConfig *cfg) {

	ComponentSelector->clear();
	m_lookupDict.clear();
	m_revLookupDict.clear();

	TQString ServiceTypeToConfigure=cfg->readEntry("ServiceTypeToConfigure");

	TQString MimeTypeOfInterest=cfg->readEntry("MimeTypeOfInterest");
	KTrader::OfferList offers = KTrader::self()->query(MimeTypeOfInterest, "'"+ServiceTypeToConfigure+"' in ServiceTypes");

	for (KTrader::OfferList::Iterator tit = offers.begin(); tit != offers.end(); ++tit)
        {
		ComponentSelector->insertItem((*tit)->name());
		m_lookupDict.insert((*tit)->name(),new TQString((*tit)->desktopEntryName()));
		m_revLookupDict.insert((*tit)->desktopEntryName(),new TQString((*tit)->name()));
	}

	KConfig *store = new KConfig(cfg->readPathEntry("storeInFile","null"));
        store->setGroup(cfg->readEntry("valueSection"));
	TQString setting=store->readEntry(cfg->readEntry("valueName","kcm_componenchooser_null"));
        delete store;
	if (setting.isEmpty()) setting=cfg->readEntry("defaultImplementation");
	TQString *tmp=m_revLookupDict[setting];
	if (tmp)
		for (int i=0;i<ComponentSelector->count();i++)
			if ((*tmp)==ComponentSelector->text(i))
			{
				ComponentSelector->setCurrentItem(i);
				break;
			}
	emit changed(false);
}

void CfgComponent::defaults()
{
    //todo
}

//END  General kpart based Component selection






//BEGIN Email client config
CfgEmailClient::CfgEmailClient(TQWidget *parent):EmailClientConfig_UI(parent),CfgPlugin(){
	pSettings = new KEMailSettings();

	connect(kmailCB, TQT_SIGNAL(toggled(bool)), TQT_SLOT(configChanged()) );
	connect(txtEMailClient, TQT_SIGNAL(textChanged(const TQString&)), TQT_SLOT(configChanged()) );
	connect(chkRunTerminal, TQT_SIGNAL(clicked()), TQT_SLOT(configChanged()) );
}

CfgEmailClient::~CfgEmailClient() {
	delete pSettings;
}

void CfgEmailClient::defaults()
{
    load(0L);
}

void CfgEmailClient::load(KConfig *)
{
	TQString emailClient = pSettings->getSetting(KEMailSettings::ClientProgram);
	bool useKMail = (emailClient.isEmpty());

	kmailCB->setChecked(useKMail);
	otherCB->setChecked(!useKMail);
	txtEMailClient->setText(emailClient);
	txtEMailClient->setFixedHeight(txtEMailClient->sizeHint().height());
	chkRunTerminal->setChecked((pSettings->getSetting(KEMailSettings::ClientTerminal) == "true"));

	emit changed(false);

}

void CfgEmailClient::configChanged()
{
	emit changed(true);
}

void CfgEmailClient::selectEmailClient()
{
	KURL::List urlList;
	KOpenWithDlg dlg(urlList, i18n("Select preferred email client:"), TQString::null, this);
	// hide "Do not &close when command exits" here, we don't need it for a mail client
	dlg.hideNoCloseOnExit();
	if (dlg.exec() != TQDialog::Accepted) return;
	TQString client = dlg.text();

	// get the preferred Terminal Application 
	KConfigGroup confGroup( KGlobal::config(), TQString::fromLatin1("General") );
	TQString preferredTerminal = confGroup.readPathEntry("TerminalApplication", TQString::fromLatin1("konsole"));
	preferredTerminal += TQString::fromLatin1(" -e ");
	
	int len = preferredTerminal.length();
	bool b = client.left(len) == preferredTerminal;
	if (b) client = client.mid(len);
	if (!client.isEmpty())
	{
		chkRunTerminal->setChecked(b);
		txtEMailClient->setText(client);
	}
}


void CfgEmailClient::save(KConfig *)
{
	if (kmailCB->isChecked())
	{
		pSettings->setSetting(KEMailSettings::ClientProgram, TQString::null);
		pSettings->setSetting(KEMailSettings::ClientTerminal, "false");
	}
	else
	{
		pSettings->setSetting(KEMailSettings::ClientProgram, txtEMailClient->text());
		pSettings->setSetting(KEMailSettings::ClientTerminal, (chkRunTerminal->isChecked()) ? "true" : "false");
	}

	// insure proper permissions -- contains sensitive data
	TQString cfgName(KGlobal::dirs()->findResource("config", "emails"));
	if (!cfgName.isEmpty())
		::chmod(TQFile::encodeName(cfgName), 0600);

	kapp->dcopClient()->emitDCOPSignal("KDE_emailSettingsChanged()", TQByteArray());

	emit changed(false);
}


//END Email client config



//BEGIN Terminal Emulator Configuration

CfgTerminalEmulator::CfgTerminalEmulator(TQWidget *parent):TerminalEmulatorConfig_UI(parent),CfgPlugin(){
	connect(terminalLE,TQT_SIGNAL(textChanged(const TQString &)), this, TQT_SLOT(configChanged()));
	connect(terminalCB,TQT_SIGNAL(toggled(bool)),this,TQT_SLOT(configChanged()));
	connect(otherCB,TQT_SIGNAL(toggled(bool)),this,TQT_SLOT(configChanged()));
}

CfgTerminalEmulator::~CfgTerminalEmulator() {
}

void CfgTerminalEmulator::configChanged()
{
	emit changed(true);
}

void CfgTerminalEmulator::defaults()
{
    load(0L);
}


void CfgTerminalEmulator::load(KConfig *) {
	KConfig *config = new KConfig("kdeglobals", true);
	config->setGroup("General");
	TQString terminal = config->readPathEntry("TerminalApplication","konsole");
	if (terminal == "konsole")
	{
	   terminalLE->setText("xterm");
	   terminalCB->setChecked(true);
	}
	else
	{
	  terminalLE->setText(terminal);
	  otherCB->setChecked(true);
	}
	delete config;

	emit changed(false);
}

void CfgTerminalEmulator::save(KConfig *) {

	KConfig *config = new KConfig("kdeglobals");
	config->setGroup("General");
	config->writePathEntry("TerminalApplication",terminalCB->isChecked()?"konsole":terminalLE->text(), true, true);
	config->sync();
	delete config;

	KIPC::sendMessageAll(KIPC::SettingsChanged);
	kapp->dcopClient()->send("klauncher", "klauncher","reparseConfiguration()", TQString::null);

	emit changed(false);
}

void CfgTerminalEmulator::selectTerminalApp()
{
	KURL::List urlList;
	KOpenWithDlg dlg(urlList, i18n("Select preferred terminal application:"), TQString::null, this);
	// hide "Run in &terminal" here, we don't need it for a Terminal Application
	dlg.hideRunInTerminal();
	if (dlg.exec() != TQDialog::Accepted) return;
	TQString client = dlg.text();

	if (!client.isEmpty())
	{
		terminalLE->setText(client);
	}
}

//END Terminal Emulator Configuration

//BEGIN Browser Configuration

CfgBrowser::CfgBrowser(TQWidget *parent) : BrowserConfig_UI(parent),CfgPlugin(){
        connect(lineExec,TQT_SIGNAL(textChanged(const TQString &)),this,TQT_SLOT(configChanged()));
	connect(radioKIO,TQT_SIGNAL(toggled(bool)),this,TQT_SLOT(configChanged()));
	connect(radioExec,TQT_SIGNAL(toggled(bool)),this,TQT_SLOT(configChanged()));
}

CfgBrowser::~CfgBrowser() {
}

void CfgBrowser::configChanged()
{
	emit changed(true);
}

void CfgBrowser::defaults()
{
	load(0L);
}


void CfgBrowser::load(KConfig *) {
	KConfig *config = new KConfig("kdeglobals", true);
	config->setGroup("General");
	TQString exec = config->readEntry("BrowserApplication");
	if (exec.isEmpty())
	{
	   radioKIO->setChecked(true);
	   m_browserExec = exec;
	   m_browserService = 0;
	}
	else
	{
	   radioExec->setChecked(true);
	   if (exec.startsWith("!"))
	   {
	      m_browserExec = exec.mid(1);
	      m_browserService = 0;
	   }
	   else
	   {
	      m_browserService = KService::serviceByStorageId( exec );
	      if (m_browserService)
  	         m_browserExec = m_browserService->desktopEntryName();
  	      else
  	         m_browserExec = TQString::null;
	   }
	}
	
	lineExec->setText(m_browserExec);
	delete config;

	emit changed(false);
}

void CfgBrowser::save(KConfig *) {

	KConfig *config = new KConfig("kdeglobals");
	config->setGroup("General");
	TQString exec;
	if (radioExec->isChecked())
	{
	   exec = lineExec->text();
	   if (m_browserService && (exec == m_browserExec))
	      exec = m_browserService->storageId(); // Use service
	   else
	      exec = "!" + exec; // Litteral command
	}
	config->writePathEntry("BrowserApplication", exec, true, true);
	config->sync();
	delete config;

	KIPC::sendMessageAll(KIPC::SettingsChanged);

	emit changed(false);
}

void CfgBrowser::selectBrowser()
{
	KURL::List urlList;
	KOpenWithDlg dlg(urlList, i18n("Select preferred Web browser application:"), TQString::null, this);
	if (dlg.exec() != TQDialog::Accepted) return;
	m_browserService = dlg.service();
	if (m_browserService)
	   m_browserExec = m_browserService->desktopEntryName();
	else
	   m_browserExec = dlg.text();

	lineExec->setText(m_browserExec);
}

//END Terminal Emulator Configuration

ComponentChooser::ComponentChooser(TQWidget *parent, const char *name):
	ComponentChooser_UI(parent,name), configWidget(0) {

	ComponentChooser_UILayout->setRowStretch(1, 1);
	somethingChanged=false;
	latestEditedService="";

	TQStringList dummy;
	TQStringList services=KGlobal::dirs()->findAllResources( "data","kcm_componentchooser/*.desktop",false,true,dummy);
	for (TQStringList::Iterator it=services.begin();it!=services.end();++it)
	{
		KSimpleConfig cfg(*it);
		ServiceChooser->insertItem(new MyListBoxItem(cfg.readEntry("Name",i18n("Unknown")),(*it)));

	}
	ServiceChooser->setFixedWidth(ServiceChooser->sizeHint().width());
	ServiceChooser->sort();
	connect(ServiceChooser,TQT_SIGNAL(highlighted(TQListBoxItem*)),this,TQT_SLOT(slotServiceSelected(TQListBoxItem*)));
	ServiceChooser->setSelected(0,true);
	slotServiceSelected(ServiceChooser->item(0));

}

void ComponentChooser::slotServiceSelected(TQListBoxItem* it) {
	if (!it) return;

	if (somethingChanged) {
		if (KMessageBox::questionYesNo(this,i18n("<qt>You changed the default component of your choice. Do you want to save that change now?</qt>"),TQString::null,KStdGuiItem::save(),KStdGuiItem::discard())==KMessageBox::Yes) save();
	}
	KSimpleConfig cfg(static_cast<MyListBoxItem*>(it)->File);

	ComponentDescription->setText(cfg.readEntry("Comment",i18n("No description available")));
	ComponentDescription->setMinimumSize(ComponentDescription->sizeHint());


	TQString cfgType=cfg.readEntry("configurationType");
	TQWidget *newConfigWidget = 0;
	if (cfgType.isEmpty() || (cfgType=="component"))
	{
		if (!(configWidget && configWidget->qt_cast("CfgComponent")))
		{
			CfgComponent* cfgcomp = new CfgComponent(configContainer);
                        cfgcomp->ChooserDocu->setText(i18n("Choose from the list below which component should be used by default for the %1 service.").arg(it->text()));
			newConfigWidget = cfgcomp;
		}
                else
                {
                        static_cast<CfgComponent*>(configWidget)->ChooserDocu->setText(i18n("Choose from the list below which component should be used by default for the %1 service.").arg(it->text()));
                }
	}
	else if (cfgType=="internal_email")
	{
		if (!(configWidget && configWidget->qt_cast("CfgEmailClient")))
		{
			newConfigWidget = new CfgEmailClient(configContainer);
		}

	}
	else if (cfgType=="internal_terminal")
	{
		if (!(configWidget && configWidget->qt_cast("CfgTerminalEmulator")))
		{
			newConfigWidget = new CfgTerminalEmulator(configContainer);
		}

	}
	else if (cfgType=="internal_browser")
	{
		if (!(configWidget && configWidget->qt_cast("CfgBrowser")))
		{
			newConfigWidget = new CfgBrowser(configContainer);
		}

	}

	if (newConfigWidget)
	{
		configContainer->addWidget(newConfigWidget);
		configContainer->raiseWidget(newConfigWidget);
		configContainer->removeWidget(configWidget);
		delete configWidget;
		configWidget=newConfigWidget;
		connect(configWidget,TQT_SIGNAL(changed(bool)),this,TQT_SLOT(emitChanged(bool)));
	        configContainer->setMinimumSize(configWidget->sizeHint());
	}
	
	if (configWidget)
		static_cast<CfgPlugin*>(configWidget->qt_cast("CfgPlugin"))->load(&cfg);
	
	emitChanged(false);
	latestEditedService=static_cast<MyListBoxItem*>(it)->File;
}


void ComponentChooser::emitChanged(bool val) {
	somethingChanged=val;
	emit changed(val);
}


ComponentChooser::~ComponentChooser()
{
	delete configWidget;
}

void ComponentChooser::load() {
	if( configWidget )
	{
		CfgPlugin * plugin = static_cast<CfgPlugin*>(
				configWidget->qt_cast( "CfgPlugin" ) );
		if( plugin )
		{
			KSimpleConfig cfg(latestEditedService);
			plugin->load( &cfg );
		}
	}
}

void ComponentChooser::save() {
	if( configWidget )
	{
		CfgPlugin * plugin = static_cast<CfgPlugin*>(
				configWidget->qt_cast( "CfgPlugin" ) );
		if( plugin )
		{
			KSimpleConfig cfg(latestEditedService);
			plugin->save( &cfg );
		}
	}
}

void ComponentChooser::restoreDefault() {
    if (configWidget)
    {
        static_cast<CfgPlugin*>(configWidget->qt_cast("CfgPlugin"))->defaults();
        emitChanged(true);
    }

/*
	txtEMailClient->setText("kmail");
	chkRunTerminal->setChecked(false);

	// Check if -e is needed, I do not think so
	terminalLE->setText("xterm");  //No need for i18n
	terminalCB->setChecked(true);
	emitChanged(false);
*/
}

// vim: sw=4 ts=4 noet
