/***************************************************************************
 *   Copyright (C) 2004,2005 by Jakub Stachowski                           *
 *   qbast@go2.pl                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include <sys/stat.h>
#include <config.h>

#include <tqlayout.h>
#include <tqfile.h>
#include <tqgroupbox.h>
#include <tqradiobutton.h>
#include <tqtimer.h>
#include <tqtabwidget.h>
#include <tqcheckbox.h>
#include <tqprocess.h>
#include <tqcursor.h>
#include <tqbuttongroup.h>

#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdeparts/genericfactory.h>
#include <kprocess.h>
#include <klineedit.h>
#include <kpassdlg.h>
#include <ksimpleconfig.h>
#include <tdeapplication.h>
#include <tdemessagebox.h>

#include "kcmdnssd.h"
#include <dnssd/settings.h>
#include <dnssd/domainbrowser.h>
#include <kipc.h>

#define MDNSD_CONF "/etc/mdnsd.conf"
#define MDNSD_PID "/var/run/mdnsd.pid"

typedef KGenericFactory<KCMDnssd, TQWidget> KCMDnssdFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_tdednssd, KCMDnssdFactory("kcmtdednssd"))

KCMDnssd::KCMDnssd(TQWidget *parent, const char *name, const TQStringList&)
		: ConfigDialog(parent, name), m_wdchanged(false)
{
	setAboutData(new TDEAboutData(I18N_NOOP("kcm_tdednssd"),
	                            I18N_NOOP("ZeroConf configuration"),0,0,TDEAboutData::License_GPL,
	                            I18N_NOOP("(C) 2004,2005 Jakub Stachowski")));
	setQuickHelp(i18n("Setup services browsing with ZeroConf"));
	if (geteuid()!=0) tabs->removePage(tab_2); // normal user cannot change wide-area settings
	// show only global things in 'administrator mode' to prevent confusion
		else if (getenv("TDESU_USER")!=0) tabs->removePage(tab); 
	addConfig(DNSSD::Configuration::self(),this);
	// it is host-wide setting so it has to be in global config file
	domain = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdednssdrc" ));
	domain->setGroup("publishing");
	load();
	connect(hostedit,TQT_SIGNAL(textChanged(const TQString&)),this,TQT_SLOT(wdchanged()));
	connect(secretedit,TQT_SIGNAL(textChanged(const TQString&)),this,TQT_SLOT(wdchanged()));
	connect(domainedit,TQT_SIGNAL(textChanged(const TQString&)),this,TQT_SLOT(wdchanged()));
	connect(enableZeroconf,TQT_SIGNAL(toggled(bool)),this,TQT_SLOT(enableZeroconfChanged(bool)));
	m_enableZeroconfChanged=false;
	if (DNSSD::Configuration::self()->publishDomain().isEmpty()) WANButton->setEnabled(false);
	kcfg_PublishType->hide(); //unused with Avahi
}

KCMDnssd::~KCMDnssd()
{
	delete domain;
}

void KCMDnssd::save()
{
	setCursor(TQCursor(Qt::BusyCursor));
	TDECModule::save();
	if (geteuid()==0 && m_wdchanged) saveMdnsd(); 
	domain->setFileWriteMode(0644); // this should be readable for everyone
	domain->writeEntry("PublishDomain",domainedit->text());
	domain->sync();
	KIPC::sendMessageAll((KIPC::Message)KIPCDomainsChanged);
	if (m_enableZeroconfChanged) {

	  TQString scaryMessage = i18n("Enabling local network browsing will open a network port (5353) on your computer.  If security problems are discovered in the zeroconf server, remote attackers could access your computer as the \"avahi\" user.");

	  TDEProcess *proc = new TDEProcess;

	  *proc << "tdesu";

	  if (enableZeroconf->isChecked()) {
	    if (KMessageBox::warningYesNo( this, scaryMessage, i18n("Enable Zeroconf Network Browsing"), KGuiItem(i18n("Enable Browsing")), KGuiItem(i18n("Don't Enable Browsing")) ) == KMessageBox::Yes) {	    

	      *proc << "/usr/share/avahi/enable_avahi 1";
	      proc->start(TDEProcess::Block);
	    } else {
	      enableZeroconf->setChecked(false);
	    }
	  } else {
	    *proc << "/usr/share/avahi/enable_avahi 0";
	    proc->start(TDEProcess::Block);
	  }
	}
	setCursor(TQCursor(Qt::ArrowCursor));
}

void KCMDnssd::load()
{
	if (geteuid()==0) loadMdnsd();
	enableZeroconf->setChecked(false);
	TQProcess avahiStatus(TQString("/usr/share/avahi/avahi_status"), TQT_TQOBJECT(this), "avahiStatus");
	avahiStatus.start();
	while (avahiStatus.isRunning()) {
	  kapp->processEvents();
	}
	int exitStatus = avahiStatus.exitStatus();
	if (exitStatus == 0) { // disabled
	  enableZeroconf->setChecked(false);
	} else if (exitStatus == 1) { // enabled 
	  enableZeroconf->setChecked(true);
	} else if (exitStatus == 2) { // custom setup
	  enableZeroconf->setEnabled(false);
	}
	TDECModule::load();
}

// hack to work around not working isModified() for KPasswordEdit
void KCMDnssd::wdchanged()
{
	WANButton->setEnabled(!domainedit->text().isEmpty() && !hostedit->text().isEmpty());
	changed();
	m_wdchanged=true;
}

void KCMDnssd::enableZeroconfChanged(bool)
{
	changed();
	m_enableZeroconfChanged=true;
}

void KCMDnssd::loadMdnsd()
{
	TQFile f(MDNSD_CONF);
	if (!f.open(IO_ReadWrite)) return;
	TQTextStream stream(&f);
	TQString line;
	while (!stream.atEnd()) {
		line = stream.readLine();
		mdnsdLines.insert(line.section(' ',0,0,TQString::SectionSkipEmpty),
			line.section(' ',1,-1,TQString::SectionSkipEmpty));
		}
	if (!mdnsdLines["zone"].isNull()) domainedit->setText(mdnsdLines["zone"]);
	if (!mdnsdLines["hostname"].isNull()) hostedit->setText(mdnsdLines["hostname"]);
	if (!mdnsdLines["secret-64"].isNull()) secretedit->setText(mdnsdLines["secret-64"]);
}		
	
bool KCMDnssd::saveMdnsd()
{
	mdnsdLines["zone"]=domainedit->text();
	mdnsdLines["hostname"]=hostedit->text();
	if (!secretedit->text().isEmpty()) mdnsdLines["secret-64"]=secretedit->password();
		else mdnsdLines.remove("secret-64");
	TQFile f(MDNSD_CONF);
	bool newfile=!f.exists();
	if (!f.open(IO_WriteOnly)) return false; 
	TQTextStream stream(&f);
	for (TQMap<TQString,TQString>::ConstIterator it=mdnsdLines.begin();it!=mdnsdLines.end();
		++it) stream << it.key() << " " << (*it) << "\n";
	f.close();
	// if it is new file, then make it only accessible for root as it can contain shared
	// secret for dns server. 
	if (newfile) chmod(MDNSD_CONF,0600); 
	f.setName(MDNSD_PID);
	if (!f.open(IO_ReadOnly)) return true; // it is not running so no need to signal
	TQString line;
	if (f.readLine(line,16)<1) return true;
	unsigned int pid = line.toUInt();
	if (pid==0) return true;           // not a pid
	kill(pid,SIGHUP);
	return true;
}

TQString KCMDnssd::handbookSection() const
{
	// FIXME
	// No context-sensitive help documentation currently exists for this module!
	int index = tabs->currentPageIndex();
	if (index == 0) {
		//return "";
		return TQString::null;
	}
	else {
		return TQString::null;
	}
}
	
#include "kcmdnssd.moc"
