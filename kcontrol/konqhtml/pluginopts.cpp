// (c) 2002-2003 Leo Savernik, per-domain settings
// (c) 2001, Daniel Naber, based on javaopts.cpp
// (c) 2000 Stefan Schimanski <1Stein@gmx.de>, Netscape parts


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <tqlayout.h>
#include <tqprogressdialog.h>
#include <tqregexp.h>
#include <tqslider.h>
#include <tqvgroupbox.h>
#include <tqwhatsthis.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <tdefiledialog.h>
#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdeprocio.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

#include "htmlopts.h"
#include "pluginopts.h"
#include "policydlg.h"


// == class PluginPolicies =====

PluginPolicies::PluginPolicies(TDEConfig* config, const TQString &group, bool global,
  		const TQString &domain) :
	Policies(config,group,global,domain,"plugins.","EnablePlugins") {
}

PluginPolicies::~PluginPolicies() {
}

// == class KPluginOptions =====

KPluginOptions::KPluginOptions( TDEConfig* config, TQString group, TQWidget *parent,
                            const char *)
    : TDECModule( parent, "kcmkonqhtml" ),
      m_pConfig( config ),
      m_groupname( group ),
      m_nspluginscan (0),
      global_policies(config,group,true)
{
    TQVBoxLayout* toplevel = new TQVBoxLayout( this, 0, KDialog::spacingHint() );

    /**************************************************************************
     ******************** Global Settings *************************************
     *************************************************************************/
    TQVGroupBox* globalGB = new TQVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enablePluginsGloballyCB = new TQCheckBox( i18n( "&Enable plugins globally" ), globalGB );
    enableHTTPOnly = new TQCheckBox( i18n( "Only allow &HTTP and HTTPS URLs for plugins" ), globalGB );
    enableUserDemand = new TQCheckBox( i18n( "&Load plugins on demand only" ), globalGB );
    priorityLabel = new TQLabel(i18n("CPU priority for plugins: %1").arg(TQString()), globalGB);
    priority = new TQSlider(5, 100, 5, 100, TQt::Horizontal, globalGB);
    connect( enablePluginsGloballyCB, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotChanged() ) );
    connect( enablePluginsGloballyCB, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotTogglePluginsEnabled() ) );
    connect( enableHTTPOnly, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotChanged() ) );
    connect( enableUserDemand, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotChanged() ) );
    connect( priority, TQ_SIGNAL( valueChanged(int) ), this, TQ_SLOT( slotChanged() ) );
    connect( priority, TQ_SIGNAL( valueChanged(int) ), this, TQ_SLOT( updatePLabel(int) ) );

    TQFrame *hrule = new TQFrame(globalGB);
    hrule->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
    hrule->setSizePolicy(TQSizePolicy::MinimumExpanding,TQSizePolicy::Fixed);

    /**************************************************************************
     ********************* Domain-specific Settings ***************************
     *************************************************************************/
    TQPushButton *domainSpecPB = new TQPushButton(i18n("Domain-Specific Settin&gs"),
    						globalGB);
    domainSpecPB->setSizePolicy(TQSizePolicy::Fixed,TQSizePolicy::Fixed);
    connect(domainSpecPB,TQ_SIGNAL(clicked()),TQ_SLOT(slotShowDomainDlg()));

    domainSpecificDlg = new KDialogBase(KDialogBase::Swallow,
    			i18n("Domain-Specific Policies"),KDialogBase::Close,
			KDialogBase::Close,this,"domainSpecificDlg", true);

    domainSpecific = new PluginDomainListView(config,group,this,domainSpecificDlg);
    domainSpecific->setMinimumSize(320,200);
    connect(domainSpecific,TQ_SIGNAL(changed(bool)),TQ_SLOT(slotChanged()));

    domainSpecificDlg->setMainWidget(domainSpecific);

    /**************************************************************************
     ********************** WhatsThis? items **********************************
     *************************************************************************/
    TQWhatsThis::add( enablePluginsGloballyCB, i18n("Enables the execution of plugins "
          "that can be contained in HTML pages, e.g. Macromedia Flash. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );

    TQString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific plugin policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling plugins on pages sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    TQWhatsThis::add( domainSpecific->listView(), wtstr );
    TQWhatsThis::add( domainSpecific->importButton(), i18n("Click this button to choose the file that contains "
                                          "the plugin policies. These policies will be merged "
                                          "with the existing ones. Duplicate entries are ignored.") );
    TQWhatsThis::add( domainSpecific->exportButton(), i18n("Click this button to save the plugin policy to a zipped "
                                          "file. The file, named <b>plugin_policy.tgz</b>, will be "
                                          "saved to a location of your choice." ) );
    TQWhatsThis::add( domainSpecific, i18n("Here you can set specific plugin policies for any particular "
                                            "host or domain. To add a new policy, simply click the <i>New...</i> "
                                            "button and supply the necessary information requested by the "
                                            "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                            "button and choose the new policy from the policy dialog box. Clicking "
                                            "on the <i>Delete</i> button will remove the selected policy causing the default "
                                            "policy setting to be used for that domain.") );
#if 0
                                            "The <i>Import</i> and <i>Export</i> "
                                            "button allows you to easily share your policies with other people by allowing "
                                            "you to save and retrieve them from a zipped file.") );
#endif

/*****************************************************************************/

    TQVGroupBox* netscapeGB = new TQVGroupBox( i18n( "Netscape Plugins" ), this );
    toplevel->addWidget( netscapeGB );

    // create Designer made widget
    m_widget = new NSConfigWidget( netscapeGB, "configwidget" );
	m_widget->dirEdit->setMode(KFile::ExistingOnly | KFile::LocalOnly | KFile::Directory);

    // setup widgets
    connect( m_widget->scanAtStartup, TQ_SIGNAL(clicked()), TQ_SLOT(change()) );
    connect( m_widget->scanButton, TQ_SIGNAL(clicked()), TQ_SLOT(scan()) );

    m_changed = false;

    dirInit();
    pluginInit();

    // Finally do the loading
    load();
}

KPluginOptions::~KPluginOptions()
{
  delete m_pConfig;
}


void KPluginOptions::updatePLabel(int p) {
    TQString level;
    p = (100 - p)/5;
    if (p > 15) {
            level = i18n("lowest priority", "lowest");
    } else if (p > 11) {
            level = i18n("low priority", "low");
    } else if (p > 7) {
            level = i18n("medium priority", "medium");
    } else if (p > 3) {
            level = i18n("high priority", "high");
    } else {
            level = i18n("highest priority", "highest");
    }

    priorityLabel->setText(i18n("CPU priority for plugins: %1").arg(level));
}

void KPluginOptions::load()
{
	load( false );
}

void KPluginOptions::load( bool useDefaults )
{
	 

    // *** load ***
    global_policies.load();
    bool bPluginGlobal = global_policies.isFeatureEnabled();

    // *** apply to GUI ***
    enablePluginsGloballyCB->setChecked( bPluginGlobal );

    domainSpecific->initialize(m_pConfig->readListEntry("PluginDomains"));

/****************************************************************************/

  TDEConfig *config = new TDEConfig("kcmnspluginrc", true);

  config->setReadDefaults( useDefaults );

  config->setGroup("Misc");
  m_widget->scanAtStartup->setChecked( config->readBoolEntry( "starttdeScan", false ) );

  m_widget->dirEdit->setURL("");
  m_widget->dirEdit->setEnabled( false );
  m_widget->dirRemove->setEnabled( false );
  m_widget->dirUp->setEnabled( false );
  m_widget->dirDown->setEnabled( false );
  enableHTTPOnly->setChecked( config->readBoolEntry("HTTP URLs Only", false) );
  enableUserDemand->setChecked( config->readBoolEntry("demandLoad", false) );
  priority->setValue(100 - KCLAMP(config->readNumEntry("Nice Level", 0), 0, 19) * 5);
  updatePLabel(priority->value());

  dirLoad( config, useDefaults );
  pluginLoad( config );

  delete config;

  emit changed( useDefaults );
}

void KPluginOptions::defaults()
{
    load( true );
}

void KPluginOptions::save()
{
    global_policies.save();

    domainSpecific->save(m_groupname,"PluginDomains");

    m_pConfig->sync();	// I need a sync here, otherwise "apply" won't work
    			// instantly

  TQByteArray data;
  if ( !tdeApp->dcopClient()->isAttached() )
    tdeApp->dcopClient()->attach();
  tdeApp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

/*****************************************************************************/

    TDEConfig *config= new TDEConfig("kcmnspluginrc", false);

    dirSave( config );
    pluginSave( config );

    config->setGroup("Misc");
    config->writeEntry( "starttdeScan", m_widget->scanAtStartup->isChecked() );
    config->writeEntry( "HTTP URLs Only", enableHTTPOnly->isChecked() );
    config->writeEntry( "demandLoad", enableUserDemand->isChecked() );
    config->writeEntry("Nice Level", (int)(100 - priority->value()) / 5);
    config->sync();
    delete config;

    change( false );
}

TQString KPluginOptions::quickHelp() const
{
      return i18n("<h1>Konqueror Plugins</h1> The Konqueror web browser can use Netscape"
        " plugins to show special content, just like the Navigator does. Please note that"
        " the way you have to install Netscape plugins may depend on your distribution. A typical"
        " place to install them is, for example, '/opt/netscape/plugins'.");
}

void KPluginOptions::slotChanged()
{
    emit changed(true);
}

void KPluginOptions::slotTogglePluginsEnabled() {
  global_policies.setFeatureEnabled(enablePluginsGloballyCB->isChecked());
}

void KPluginOptions::slotShowDomainDlg() {
  domainSpecificDlg->show();
}

/***********************************************************************************/

void KPluginOptions::scan()
{
    m_widget->scanButton->setEnabled(false);
    if ( m_changed ) {
        int ret = KMessageBox::warningYesNoCancel( this,
                                                    i18n("Do you want to apply your changes "
                                                         "before the scan? Otherwise the "
                                                         "changes will be lost."), TQString(), KStdGuiItem::save(), KStdGuiItem::discard() );
        if ( ret==KMessageBox::Cancel ) {
            m_widget->scanButton->setEnabled(true);
            return;
        }
        if ( ret==KMessageBox::Yes )
             save();
    }

    m_nspluginscan = new TDEProcIO;
    TQString scanExe = TDEGlobal::dirs()->findExe("nspluginscan");
    if (!scanExe) {
        kdDebug() << "can't find nspluginviewer" << endl;
        delete m_nspluginscan; 
        m_nspluginscan = 0L;

        KMessageBox::sorry ( this,
                             i18n("The nspluginscan executable cannot be found. "
                                  "Netscape plugins will not be scanned.") );
        m_widget->scanButton->setEnabled(true);
        return;
    }

    // find nspluginscan executable
    m_progress = new TQProgressDialog( i18n("Scanning for plugins"), i18n("Cancel"), 100, this );
    m_progress->setProgress( 5 );

    // start nspluginscan
    *m_nspluginscan << scanExe << "--verbose";
    kdDebug() << "Running nspluginscan" << endl;
    connect(m_nspluginscan, TQ_SIGNAL(readReady(TDEProcIO*)),
            this, TQ_SLOT(progress(TDEProcIO*)));
    connect(m_nspluginscan, TQ_SIGNAL(processExited(TDEProcess *)),
            this, TQ_SLOT(scanDone()));
    connect(m_progress, TQ_SIGNAL(cancelled()), this, TQ_SLOT(scanDone()));

    m_nspluginscan->start();
}

void KPluginOptions::progress(TDEProcIO *proc)
{
    TQString line;
    while(proc->readln(line) > 0)
        ;
    m_progress->setProgress(line.stripWhiteSpace().toInt());
}

void KPluginOptions::scanDone()
{
    m_progress->setProgress(100);
    load();

    delete m_progress; m_progress = 0L;
    m_nspluginscan->deleteLater(); m_nspluginscan = 0L;
    m_widget->scanButton->setEnabled(true);
}

/***********************************************************************************/


void KPluginOptions::dirInit()
{
    m_widget->dirEdit->setCaption(i18n("Select Plugin Scan Folder"));
    connect( m_widget->dirNew, TQ_SIGNAL(clicked()), TQ_SLOT(dirNew()));
    connect( m_widget->dirRemove, TQ_SIGNAL(clicked()), TQ_SLOT(dirRemove()));
    connect( m_widget->dirUp, TQ_SIGNAL(clicked()), TQ_SLOT(dirUp()));
    connect( m_widget->dirDown, TQ_SIGNAL(clicked()), TQ_SLOT(dirDown()) );
    connect( m_widget->useArtsdsp, TQ_SIGNAL(clicked()),TQ_SLOT(change()));
    connect( m_widget->dirEdit,
             TQ_SIGNAL(textChanged(const TQString&)),
             TQ_SLOT(dirEdited(const TQString &)) );

    connect( m_widget->dirList,
             TQ_SIGNAL(executed(TQListBoxItem*)),
             TQ_SLOT(dirSelect(TQListBoxItem*)) );

    connect( m_widget->dirList,
             TQ_SIGNAL(selectionChanged(TQListBoxItem*)),
             TQ_SLOT(dirSelect(TQListBoxItem*)) );
}


void KPluginOptions::dirLoad( TDEConfig *config, bool useDefault )
{
    TQStringList paths;

    // read search paths

    config->setGroup("Misc");
    if ( config->hasKey( "scanPaths" ) && !useDefault )
        paths = config->readListEntry( "scanPaths" );
    else {//keep sync with tdebase/nsplugins
        paths.append("$HOME/.mozilla/plugins");
        paths.append("$HOME/.netscape/plugins");
	paths.append("/usr/lib/iceweasel/plugins");
        paths.append("/usr/lib/iceape/plugins");
        paths.append("/usr/lib/firefox/plugins");
        paths.append("/usr/lib64/browser-plugins");
        paths.append("/usr/lib/browser-plugins");
        paths.append("/usr/local/netscape/plugins");
        paths.append("/opt/mozilla/plugins");
	paths.append("/opt/mozilla/lib/plugins");
        paths.append("/opt/netscape/plugins");
        paths.append("/opt/netscape/communicator/plugins");
        paths.append("/usr/lib/netscape/plugins");
        paths.append("/usr/lib/netscape/plugins-libc5");
        paths.append("/usr/lib/netscape/plugins-libc6");
        paths.append("/usr/lib/mozilla/plugins");
	paths.append("/usr/lib64/netscape/plugins");
	paths.append("/usr/lib64/mozilla/plugins");
        paths.append("$MOZILLA_HOME/plugins");

    }

    // fill list
    m_widget->dirList->clear();
    m_widget->dirList->insertStringList( paths );

    // setup other widgets
    bool useArtsdsp = config->readBoolEntry( "useArtsdsp", false );
    m_widget->useArtsdsp->setChecked( useArtsdsp );
}


void KPluginOptions::dirSave( TDEConfig *config )
{
    // create stringlist
    TQStringList paths;
    TQListBoxItem *item = m_widget->dirList->firstItem();
    for ( ; item!=0; item=item->next() )
        if ( !item->text().isEmpty() )
            paths << item->text();

    // write entry
    config->setGroup( "Misc" );
    config->writeEntry( "scanPaths", paths );
    config->writeEntry( "useArtsdsp", m_widget->useArtsdsp->isOn() );
}


void KPluginOptions::dirSelect( TQListBoxItem *item )
{
    m_widget->dirEdit->setEnabled( item!=0 );
    m_widget->dirRemove->setEnabled( item!=0 );

    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    m_widget->dirDown->setEnabled( item!=0 && cur<m_widget->dirList->count()-1 );
    m_widget->dirUp->setEnabled( item!=0 && cur>0 );
    m_widget->dirEdit->setURL( item!=0 ? item->text() : TQString() );
 }


void KPluginOptions::dirNew()
{
    m_widget->dirList->insertItem( TQString(), 0 );
    m_widget->dirList->setCurrentItem( 0 );
    dirSelect( m_widget->dirList->selectedItem() );
    m_widget->dirEdit->setURL(TQString());
    m_widget->dirEdit->setFocus();
    change();
}


void KPluginOptions::dirRemove()
{
    m_widget->dirEdit->setURL(TQString());
    delete m_widget->dirList->selectedItem();
    m_widget->dirRemove->setEnabled( false );
    m_widget->dirUp->setEnabled( false );
    m_widget->dirDown->setEnabled( false );
    m_widget->dirEdit->setEnabled( false );
    change();
}


void KPluginOptions::dirUp()
{
    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    if ( cur>0 ) {
        TQString txt = m_widget->dirList->text(cur-1);
        m_widget->dirList->removeItem( cur-1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( cur-1>0 );
        m_widget->dirDown->setEnabled( true );
        change();
    }
}


void KPluginOptions::dirDown()
{
    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    if ( cur < m_widget->dirList->count()-1 ) {
        TQString txt = m_widget->dirList->text(cur+1);
        m_widget->dirList->removeItem( cur+1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( true );
        m_widget->dirDown->setEnabled( cur+1<m_widget->dirList->count()-1 );
        change();
    }
}


void KPluginOptions::dirEdited(const TQString &txt )
{
    if ( m_widget->dirList->currentText() != txt ) {
        m_widget->dirList->blockSignals(true);
        m_widget->dirList->changeItem( txt, m_widget->dirList->currentItem() );
        m_widget->dirList->blockSignals(false);
        change();
    }
}


/***********************************************************************************/


void KPluginOptions::pluginInit()
{
}


void KPluginOptions::pluginLoad( TDEConfig* /*config*/ )
{
    kdDebug() << "-> KPluginOptions::fillPluginList" << endl;
    m_widget->pluginList->clear();
    TQRegExp version(";version=[^:]*:");

    // open the cache file
    TQFile cachef( locate("data", "nsplugins/cache") );
    if ( !cachef.exists() || !cachef.open(IO_ReadOnly) ) {
        kdDebug() << "Could not load plugin cache file!" << endl;
        return;
    }

    TQTextStream cache(&cachef);

    // root object
    TQListViewItem *root = new TQListViewItem( m_widget->pluginList, i18n("Netscape Plugins") );
    root->setOpen( true );
    root->setSelectable( false );
    root->setExpandable( true );
    root->setPixmap(0, SmallIcon("netscape"));

    // read in cache
    TQString line, plugin;
    TQListViewItem *next = 0;
    TQListViewItem *lastMIME = 0;
    while ( !cache.atEnd() ) {

        line = cache.readLine();
        //kdDebug() << line << endl;
        if (line.isEmpty() || (line.left(1) == "#"))
            continue;

        if (line.left(1) == "[") {

            plugin = line.mid(1,line.length()-2);
            //kdDebug() << "plugin=" << plugin << endl;

            // add plugin root item
            next = new TQListViewItem( root, i18n("Plugin"), plugin );
            next->setOpen( false );
            next->setSelectable( false );
            next->setExpandable( true );

            lastMIME = 0;

            continue;
        }

        TQStringList desc = TQStringList::split(':', line, TRUE);
        TQString mime = desc[0].stripWhiteSpace();
        TQString name = desc[2];
        TQString suffixes = desc[1];

        if (!mime.isEmpty()) {
            //kdDebug() << "mime=" << mime << " desc=" << name << " suffix=" << suffixes << endl;
            lastMIME = new TQListViewItem( next, lastMIME, i18n("MIME type"), mime );
            lastMIME->setOpen( false );
            lastMIME->setSelectable( false );
            lastMIME->setExpandable( true );

            TQListViewItem *last = new TQListViewItem( lastMIME, 0, i18n("Description"), name );
            last->setOpen( false );
            last->setSelectable( false );
            last->setExpandable( false );

            last = new TQListViewItem( lastMIME, last, i18n("Suffixes"), suffixes );
            last->setOpen( false );
            last->setSelectable( false );
            last->setExpandable( false );
        }
    }

    //kdDebug() << "<- KPluginOptions::fillPluginList" << endl;
}


void KPluginOptions::pluginSave( TDEConfig* /*config*/ )
{

}

// == class PluginDomainDialog =====

PluginDomainDialog::PluginDomainDialog(TQWidget *parent) :
	TQWidget(parent,"PluginDomainDialog") {
  setCaption(i18n("Domain-Specific Policies"));

  thisLayout = new TQVBoxLayout(this);
  thisLayout->addSpacing(6);
  TQFrame *hrule = new TQFrame(this);
  hrule->setFrameStyle(TQFrame::HLine | TQFrame::Sunken);
  thisLayout->addWidget(hrule);
  thisLayout->addSpacing(6);

  TQBoxLayout *hl = new TQHBoxLayout(this,0,6);
  hl->addStretch(10);

  TQPushButton *closePB = new KPushButton(KStdGuiItem::close(),this);
  connect(closePB,TQ_SIGNAL(clicked()),TQ_SLOT(slotClose()));
  hl->addWidget(closePB);
  thisLayout->addLayout(hl);
}

PluginDomainDialog::~PluginDomainDialog() {
}

void PluginDomainDialog::setMainWidget(TQWidget *widget) {
  thisLayout->insertWidget(0,widget);
}

void PluginDomainDialog::slotClose() {
  hide();
}

// == class PluginDomainListView =====

PluginDomainListView::PluginDomainListView(TDEConfig *config,const TQString &group,
	KPluginOptions *options,TQWidget *parent,const char *name)
	: DomainListView(config,i18n( "Doma&in-Specific" ), parent, name),
	group(group), options(options) {
}

PluginDomainListView::~PluginDomainListView() {
}

void PluginDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies *pol) {
  TQString caption;
  switch (trigger) {
    case AddButton:
      caption = i18n( "New Plugin Policy" );
      pol->setFeatureEnabled(!options->enablePluginsGloballyCB->isChecked());
      break;
    case ChangeButton: caption = i18n( "Change Plugin Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("&Plugin policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a plugin policy for "
                                    "the above host or domain."));
  pDlg.refresh();
}

PluginPolicies *PluginDomainListView::createPolicies() {
  return new PluginPolicies(config,group,false);
}

PluginPolicies *PluginDomainListView::copyPolicies(Policies *pol) {
  return new PluginPolicies(*static_cast<PluginPolicies *>(pol));
}

#include "pluginopts.moc"
