/* This file is part of the KDE project
   Copyright (C) 2003 George Staikos <staikos@kde.org>

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

#ifndef web_module_h
#define web_module_h

#include <assert.h>
#include <tdehtml_part.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <konqsidebarplugin.h>
#include <tdepopupmenu.h>
#include <tqobject.h>


// A wrapper for TDEHTMLPart to make it behave the way we want it to.
class TDEHTMLSideBar : public TDEHTMLPart
{
	TQ_OBJECT
	public:
		TDEHTMLSideBar(bool universal) : TDEHTMLPart() {
			setStatusMessagesEnabled(false);
			setMetaRefreshEnabled(true);
			setJavaEnabled(false);
			setPluginsEnabled(false);

			setFormNotification(TDEHTMLPart::Only);
			connect(this,
				TQ_SIGNAL(formSubmitNotification(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&)),
				this,
				TQ_SLOT(formProxy(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&))
				);


			_linkMenu = new TDEPopupMenu(widget(),
					"link context menu");
			if (!universal) {
				_linkMenu->insertItem(i18n("&Open Link"),
						this, TQ_SLOT(loadPage()));
				_linkMenu->insertItem(i18n("Open in New &Window"),
						this, TQ_SLOT(loadNewWindow()));
			} else {
				_linkMenu->insertItem(i18n("Open in New &Window"),
						this, TQ_SLOT(loadPage()));
			}
			_menu = new TDEPopupMenu(widget(), "context menu");
			_menu->insertItem(SmallIcon("reload"), i18n("&Reload"),
					this, TQ_SIGNAL(reload()));
			_menu->insertItem(SmallIcon("reload"), i18n("Set &Automatic Reload"),                                                  this, TQ_SIGNAL(setAutoReload()));

			connect(this,
				TQ_SIGNAL(popupMenu(const TQString&,const TQPoint&)),
				this,
				TQ_SLOT(showMenu(const TQString&, const TQPoint&)));

		}
		virtual ~TDEHTMLSideBar() {}

	signals:
		void submitFormRequest(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&);
		void openURLRequest(const TQString& url, KParts::URLArgs args);
		void openURLNewWindow(const TQString& url, KParts::URLArgs args);
		void reload();
		void setAutoReload();

	protected:
		virtual void urlSelected( const TQString &url, int button,
				int state, const TQString &_target,
				KParts::URLArgs args = KParts::URLArgs()) {
			if (button == TQt::LeftButton ){
				if (_target.lower() == "_self") {
					openURL(url);
				} else if (_target.lower() == "_blank") {
					emit openURLNewWindow(completeURL(url).url(), args);
				} else { // isEmpty goes here too
					emit openURLRequest(completeURL(url).url(), args);
				}
				return;
			}
			if (button == TQt::MidButton) {
				emit openURLNewWindow(completeURL(url).url(),
						args);
				return;
			}
			// A refresh
			if (button == 0 && _target.lower() == "_self") {
				openURL(completeURL(url));
				return;
			}
			TDEHTMLPart::urlSelected(url,button,state,_target,args);
		}

	protected slots:
		void loadPage() {
			emit openURLRequest(completeURL(_lastUrl).url(),
						KParts::URLArgs());
		}

		void loadNewWindow() {
			emit openURLNewWindow(completeURL(_lastUrl).url(),
						KParts::URLArgs());
		}

		void showMenu(const TQString& url, const TQPoint& pos) {
			if (url.isEmpty()) {
				_menu->popup(pos);
			} else {
				_lastUrl = url;
				_linkMenu->popup(pos);
			}
		}

		void formProxy(const char *action,
				const TQString& url,
				const TQByteArray& formData,
				const TQString& target,
				const TQString& contentType,
				const TQString& boundary) {
			TQString t = target.lower();
			TQString u;

			if (TQCString(action).lower() != "post") {
				// GET
				KURL kurl = completeURL(url);
				kurl.setQuery(formData.data());
				u = kurl.url();
			} else {
				u = completeURL(url).url();
			}

			// Some sites seem to use empty targets to send to the
			// main frame.
			if (t == "_content") {
				emit submitFormRequest(action, u, formData,
						target, contentType, boundary);
			} else if (t.isEmpty() || t == "_self") {
				setFormNotification(TDEHTMLPart::NoNotification);
				submitFormProxy(action, u, formData, target,
						contentType, boundary);
				setFormNotification(TDEHTMLPart::Only);
			}
		}
	private:
		TDEPopupMenu *_menu, *_linkMenu;
		TQString _lastUrl;
};



class KonqSideBarWebModule : public KonqSidebarPlugin
{
	TQ_OBJECT
	public:
		KonqSideBarWebModule(TDEInstance *instance, TQObject *parent,
			       	TQWidget *widgetParent, TQString &desktopName,
			       	const char *name);
		virtual ~KonqSideBarWebModule();

		virtual TQWidget *getWidget();
		virtual void *provides(const TQString &);

	signals:
		void submitFormRequest(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&);
		void openURLRequest(const KURL &url, const KParts::URLArgs &args);
		void createNewWindow(const KURL &url, const KParts::URLArgs &args);
	protected:
		virtual void handleURL(const KURL &url);

	private slots:
		void urlClicked(const TQString& url, KParts::URLArgs args);
		void formClicked(const KURL& url, const KParts::URLArgs& args);
		void urlNewWindow(const TQString& url, KParts::URLArgs args);
		void pageLoaded();
		void loadFavicon();
		void setTitle(const TQString&);
		void setAutoReload();
		void reload();

	private:
		TDEHTMLSideBar *_htmlPart;
		KURL _url;
		int reloadTimeout;
		TQString _desktopName;
};

#endif

