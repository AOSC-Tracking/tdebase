/* This file is part of the KDE project
   Copyright (C) 2003, George Staikos <staikos@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "web_module.h"
#include <tqfileinfo.h>
#include <tqhbox.h>
#include <tqspinbox.h>
#include <tqtimer.h>

#include <dom/html_inline.h>
#include <kdebug.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <konq_pixmapprovider.h>
#include <tdeparts/browserextension.h>
#include <tdestandarddirs.h>


KonqSideBarWebModule::KonqSideBarWebModule(TDEInstance *instance, TQObject *parent, TQWidget *widgetParent, TQString &desktopName, const char* name)
	: KonqSidebarPlugin(instance, parent, widgetParent, desktopName, name)
{
	_htmlPart = new TDEHTMLSideBar(universalMode());
	connect(_htmlPart, TQ_SIGNAL(reload()), this, TQ_SLOT(reload()));
	connect(_htmlPart, TQ_SIGNAL(completed()), this, TQ_SLOT(pageLoaded()));
	connect(_htmlPart,
		TQ_SIGNAL(setWindowCaption(const TQString&)),
		this,
		TQ_SLOT(setTitle(const TQString&)));
	connect(_htmlPart,
		TQ_SIGNAL(openURLRequest(const TQString&, KParts::URLArgs)),
		this,
		TQ_SLOT(urlClicked(const TQString&, KParts::URLArgs)));
	connect(_htmlPart->browserExtension(),
		TQ_SIGNAL(openURLRequest(const KURL&, const KParts::URLArgs&)),
		this,
		TQ_SLOT(formClicked(const KURL&, const KParts::URLArgs&)));
	connect(_htmlPart,
		TQ_SIGNAL(setAutoReload()), this, TQ_SLOT( setAutoReload() ));
	connect(_htmlPart,
		TQ_SIGNAL(openURLNewWindow(const TQString&, KParts::URLArgs)),
		this,
		TQ_SLOT(urlNewWindow(const TQString&, KParts::URLArgs)));
	connect(_htmlPart,
		TQ_SIGNAL(submitFormRequest(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&)),
		this,
		TQ_SIGNAL(submitFormRequest(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&)));

	_desktopName = desktopName;

	TDESimpleConfig ksc(_desktopName);
	ksc.setGroup("Desktop Entry");
        reloadTimeout = ksc.readNumEntry("Reload", 0);
	_url = ksc.readPathEntry("URL");
	_htmlPart->openURL(_url );
	// Must load this delayed
	TQTimer::singleShot(0, this, TQ_SLOT(loadFavicon()));
}


KonqSideBarWebModule::~KonqSideBarWebModule() {
	delete _htmlPart;
	_htmlPart = 0L;
}


TQWidget *KonqSideBarWebModule::getWidget() {
	return _htmlPart->widget();
}

void KonqSideBarWebModule::setAutoReload(){
	KDialogBase dlg(0, "", true, i18n("Set Refresh Timeout (0 disables)"),
			KDialogBase::Ok|KDialogBase::Cancel);
	TQHBox *hbox = dlg.makeHBoxMainWidget();
	
	TQSpinBox *mins = new TQSpinBox( 0, 120, 1, hbox );
	mins->setSuffix( i18n(" min") );
	TQSpinBox *secs = new TQSpinBox( 0, 59, 1, hbox );
	secs->setSuffix( i18n(" sec") );

	if( reloadTimeout > 0 )	{
		int seconds = reloadTimeout / 1000;
		secs->setValue( seconds % 60 );
		mins->setValue( ( seconds - secs->value() ) / 60 );
	}
	
	if( dlg.exec() == TQDialog::Accepted ) {
		int msec = ( mins->value() * 60 + secs->value() ) * 1000;
		reloadTimeout = msec;
		TDESimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		ksc.writeEntry("Reload", reloadTimeout);	
		reload();
	}
}

void *KonqSideBarWebModule::provides(const TQString &) {
	return 0L;
}


void KonqSideBarWebModule::handleURL(const KURL &) {
}


void KonqSideBarWebModule::urlNewWindow(const TQString& url, KParts::URLArgs args)
{
	emit createNewWindow(KURL(url), args);
}


void KonqSideBarWebModule::urlClicked(const TQString& url, KParts::URLArgs args) 
{
	emit openURLRequest(KURL(url), args);
}


void KonqSideBarWebModule::formClicked(const KURL& url, const KParts::URLArgs& args) 
{
	_htmlPart->browserExtension()->setURLArgs(args);
	_htmlPart->openURL(url);
}


void KonqSideBarWebModule::loadFavicon() {
	TQString icon = KonqPixmapProvider::iconForURL(_url.url());
	if (icon.isEmpty()) {
		KonqFavIconMgr::downloadHostIcon(_url);
		icon = KonqPixmapProvider::iconForURL(_url.url());
	}

	if (!icon.isEmpty()) {
		emit setIcon(icon);

		TDESimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		if (icon != ksc.readPathEntry("Icon")) {
			ksc.writePathEntry("Icon", icon);
		}
	}
}


void KonqSideBarWebModule::reload() {
	_htmlPart->openURL(_url);
}


void KonqSideBarWebModule::setTitle(const TQString& title) {
	if (!title.isEmpty()) {
		emit setCaption(title);

		TDESimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		if (title != ksc.readPathEntry("Name")) {
			ksc.writePathEntry("Name", title);
		}
	}
}


void KonqSideBarWebModule::pageLoaded() {
	if( reloadTimeout > 0 ) {
		TQTimer::singleShot( reloadTimeout, this, TQ_SLOT( reload() ) );
	}
}


extern "C" {
	TDE_EXPORT KonqSidebarPlugin* create_konqsidebar_web(TDEInstance *instance, TQObject *parent, TQWidget *widget, TQString &desktopName, const char *name) {
		return new KonqSideBarWebModule(instance, parent, widget, desktopName, name);
	}
}


extern "C" {
	TDE_EXPORT bool add_konqsidebar_web(TQString* fn, TQString* param, TQMap<TQString,TQString> *map) {
		Q_UNUSED(param);
		TDEGlobal::dirs()->addResourceType("websidebardata", TDEStandardDirs::kde_default("data") + "konqsidebartng/websidebar");
		KURL url;
		url.setProtocol("file");
		TQStringList paths = TDEGlobal::dirs()->resourceDirs("websidebardata");
		for (TQStringList::Iterator i = paths.begin(); i != paths.end(); ++i) {
			if (TQFileInfo(*i + "websidebar.html").exists()) {
				url.setPath(*i + "websidebar.html");
				break;
			}
		}

		if (url.path().isEmpty())
			return false;
		map->insert("Type", "Link");
		map->insert("URL", url.url());
		map->insert("Icon", "netscape");
		map->insert("Name", i18n("Web SideBar Plugin"));
		map->insert("Open", "true");
		map->insert("X-TDE-KonqSidebarModule","konqsidebar_web");
		fn->setLatin1("websidebarplugin%1.desktop");
		return true;
	}
}


#include "web_module.moc"

