// kcookiesmain.cpp - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#include <tqlayout.h>
#include <tqtabwidget.h>

#include <tdelocale.h>
#include <tdeapplication.h>
#include <tdemessagebox.h>
#include <kdebug.h>
#include <dcopref.h>

#include "kcookiesmain.h"
#include "kcookiespolicies.h"
#include "kcookiesmanagement.h"

KCookiesMain::KCookiesMain(TQWidget *parent)
  : TDECModule(parent, "kcmtdeio")
{
    int currentTabNumber = 0;
    policiesTabNumber = -1;
    managementTabNumber = -1;

    management = 0;
    bool managerOK = true;

    DCOPReply reply = DCOPRef( "kded", "kded" ).call( "loadModule", 
        TQCString( "kcookiejar" ) );

    if( !reply.isValid() )
    {
       managerOK = false;
       kdDebug(7103) << "kcm_tdeio: KDED could not load KCookiejar!" << endl;
       KMessageBox::sorry(0, i18n("Unable to start the cookie handler service.\n"
                             "You will not be able to manage the cookies that "
                             "are stored on your computer."));
    }

    TQVBoxLayout *layout = new TQVBoxLayout(this);
    tab = new TQTabWidget(this);
    layout->addWidget(tab);

    policies = new KCookiesPolicies(this);
    tab->addTab(policies, i18n("&Policy"));
    policiesTabNumber = currentTabNumber;
    currentTabNumber++;
    connect(policies, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));

    if( managerOK )
    {
        management = new KCookiesManagement(this);
        tab->addTab(management, i18n("&Management"));
        managementTabNumber = currentTabNumber;
        currentTabNumber++;
        connect(management, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));
    }
}

KCookiesMain::~KCookiesMain()
{
}

void KCookiesMain::load()
{
  policies->load();
  if( management )
      management->load();
}

void KCookiesMain::save()
{
  policies->save();
  if ( management )
      management->save();
}

void KCookiesMain::defaults()
{
  TDECModule* module = static_cast<TDECModule*>(tab->currentPage());
  
  if ( module == policies )
    policies->defaults();
  else if( management )
    management->defaults();
}

TQString KCookiesMain::quickHelp() const
{
  return i18n("<h1>Cookies</h1> Cookies contain information that Konqueror"
    " (or other TDE applications using the HTTP protocol) stores on your"
    " computer, initiated by a remote Internet server. This means that"
    " a web server can store information about you and your browsing activities"
    " on your machine for later use. You might consider this an invasion of"
    " privacy. <p> However, cookies are useful in certain situations. For example, they"
    " are often used by Internet shops, so you can 'put things into a shopping basket'."
    " Some sites require you have a browser that supports cookies. <p>"
    " Because most people want a compromise between privacy and the benefits cookies offer,"
    " TDE offers you the ability to customize the way it handles cookies. So you might want"
    " to set TDE's default policy to ask you whenever a server wants to set a cookie,"
    " allowing you to decide. For your favorite shopping web sites that you trust, you might"
    " want to set the policy to accept, then you can access the web sites without being prompted"
    " every time TDE receives a cookie." );
}

TQString KCookiesMain::handbookSection() const
{
	int index = tab->currentPageIndex();
	if (index == policiesTabNumber) {
		//return "cookie-policy";
		return TQString::null;
	}
	else if (index == managementTabNumber) {
		return "cookie-management";
	}
	else {
		return TQString::null;
	}
}

#include "kcookiesmain.moc"
