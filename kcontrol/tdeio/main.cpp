// (c) Torben Weis 1998
// (c) David Faure 1998
/*
 * main.cpp for lisa,reslisa,tdeio_lan and tdeio_rlan kcm module
 *
 *  Copyright (C) 2000,2001 Alexander Neundorf <neundorf@kde.org>
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

#include <tqfile.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqtabwidget.h>

#include <tdecmoduleloader.h>
#include <tdelocale.h>

#include "kcookiesmain.h"
#include "netpref.h"
#include "smbrodlg.h"
#include "useragentdlg.h"
#include "kproxydlg.h"
#include "cache.h"

#include "main.h"

extern "C"
{

  KDE_EXPORT TDECModule *create_cookie(TQWidget *parent, const char /**name*/)
  {
    return new KCookiesMain(parent);
  }

  KDE_EXPORT TDECModule *create_smb(TQWidget *parent, const char /**name*/)
  {
    return new SMBRoOptions(parent);
  }

  KDE_EXPORT TDECModule *create_useragent(TQWidget *parent, const char /**name*/)
  {
    return new UserAgentDlg(parent);
  }

  KDE_EXPORT TDECModule *create_proxy(TQWidget *parent, const char /**name*/)
  {
    return new KProxyOptions(parent);
  }

  KDE_EXPORT TDECModule *create_cache(TQWidget *parent, const char /**name*/)
  {
    return new KCacheConfigDialog( parent );
  }

  KDE_EXPORT TDECModule *create_netpref(TQWidget *parent, const char /**name*/)
  {
    return new KIOPreferences(parent);
  }

  KDE_EXPORT TDECModule *create_lanbrowser(TQWidget *parent, const char *)
  {
    return new LanBrowser(parent);
  }

}

LanBrowser::LanBrowser(TQWidget *parent)
:TDECModule(parent,"kcmtdeio")
,layout(this)
,tabs(this)
,smbPageTabNumber(-1)
,lisaPageTabNumber(-1)
,tdeioLanPageTabNumber(-1)
{
   int currentTabNumber = 0;

   setQuickHelp( i18n("<h1>Local Network Browsing</h1>Here you setup your "
		"<b>\"Network Neighborhood\"</b>. You "
		"can use either the LISa daemon and the lan:/ ioslave, or the "
		"ResLISa daemon and the rlan:/ ioslave.<br><br>"
		"About the <b>LAN ioslave</b> configuration:<br> If you select it, the "
		"ioslave, <i>if available</i>, will check whether the host "
		"supports this service when you open this host. Please note "
		"that paranoid people might consider even this to be an attack.<br>"
		"<i>Always</i> means that you will always see the links for the "
		"services, regardless of whether they are actually offered by the host. "
		"<i>Never</i> means that you will never have the links to the services. "
		"In both cases you will not contact the host, so nobody will ever regard "
		"you as an attacker.<br><br>More information about <b>LISa</b> "
		"can be found at <a href=\"http://lisa-home.sourceforge.net\">"
		"the LISa Homepage</a> or contact Alexander Neundorf "
		"&lt;<a href=\"mailto:neundorf@kde.org\">neundorf@kde.org</a>&gt;."));

   layout.addWidget(&tabs);

   smbPage = create_smb(&tabs, 0);
   tabs.addTab(smbPage, i18n("&Windows Shares"));
   smbPageTabNumber = currentTabNumber;
   currentTabNumber++;
   connect(smbPage,TQ_SIGNAL(changed(bool)), TQ_SLOT( changed() ));

   lisaPage = TDECModuleLoader::loadModule("kcmlisa", TDECModuleLoader::None, &tabs);
   if (lisaPage)
   {
     tabs.addTab(lisaPage,i18n("&LISa Daemon"));
     lisaPageTabNumber = currentTabNumber;
     currentTabNumber++;
     connect(lisaPage,TQ_SIGNAL(changed()), TQ_SLOT( changed() ));
   }

//   resLisaPage = TDECModuleLoader::loadModule("kcmreslisa", &tabs);
//   if (resLisaPage)
//   {
//     tabs.addTab(resLisaPage,i18n("R&esLISa Daemon"));
//     connect(resLisaPage,TQ_SIGNAL(changed()), TQ_SLOT( changed() ));
//   }

   tdeioLanPage = TDECModuleLoader::loadModule("kcmtdeiolan",  TDECModuleLoader::None, &tabs);
   if (tdeioLanPage)
   {
     tabs.addTab(tdeioLanPage,i18n("lan:/ Iosla&ve"));
     tdeioLanPageTabNumber = currentTabNumber;
     currentTabNumber++;
     connect(tdeioLanPage,TQ_SIGNAL(changed()), TQ_SLOT( changed() ));
   }

   setButtons(Apply|Help);
   load();
}

void LanBrowser::load()
{
   smbPage->load();
   if (lisaPage)
     lisaPage->load();
//   if (resLisaPage)
//     resLisaPage->load();
   if (tdeioLanPage)
     tdeioLanPage->load();
   emit changed(false);
}

void LanBrowser::save()
{
   smbPage->save();
//   if (resLisaPage)
//     resLisaPage->save();
   if (tdeioLanPage)
     tdeioLanPage->save();
   if (lisaPage)
     lisaPage->save();
   emit changed(false);
}

TQString LanBrowser::handbookDocPath() const
{
	int index = tabs.currentPageIndex();
	if (index == smbPageTabNumber)
		return TQString::null;
	else if (index == lisaPageTabNumber)
		return "kcontrol/lanbrowser/index.html";
	else if (index == tdeioLanPageTabNumber)
		return "kcontrol/lanbrowser/index.html";
	else
		return TQString::null;
}

TQString LanBrowser::handbookSection() const
{
	int index = tabs.currentPageIndex();
	if (index == smbPageTabNumber)
		return "windows-shares";
	else
		return TQString::null;
}

#include "main.moc"

