/***************************************************************************
                               sidebar_widget.h
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _SIDEBAR_WIDGET_
#define _SIDEBAR_WIDGET_

#include <tqptrvector.h>
#include <tqtimer.h>
#include <tqstring.h>
#include <tqguardedptr.h>

#include <kdockwidget.h>
#include <kurl.h>
#include <ktoolbar.h>
#include <kparts/part.h>
#include <kmultitabbar.h>

#include "konqsidebarplugin.h"
#include "konqsidebariface_p.h"

class KDockWidget;
class TQHBoxLayout;
class TQSplitter;
class TQStringList;

class ButtonInfo: public TQObject, public KonqSidebarIface
{
	Q_OBJECT
public:
	ButtonInfo(const TQString& file_, class KonqSidebarIface *part, class KDockWidget *dock_,
			const TQString &url_,const TQString &lib,
			const TQString &dispName_, const TQString &iconName_,
			TQObject *parent)
		: TQObject(parent), file(file_), dock(dock_), URL(url_),
		libName(lib), displayName(dispName_), iconName(iconName_), m_part(part)
		{
		copy = cut = paste = trash = del = rename =false;
		}

	~ButtonInfo() {}

	TQString file;
	KDockWidget *dock;
	KonqSidebarPlugin *module;
	TQString URL;
	TQString libName;
	TQString displayName;
	TQString iconName;
	bool copy;
	bool cut;
	bool paste;
	bool trash;
	bool del;
        bool rename;
        KonqSidebarIface *m_part;
	virtual bool universalMode() {return m_part->universalMode();}
};


class addBackEnd: public TQObject
{
	Q_OBJECT
public:
	addBackEnd(TQWidget *parent,class TQPopupMenu *addmenu, bool universal,
                   const TQString &currentProfile, const char *name=0);
	~addBackEnd(){;}
protected slots:
	void aboutToShowAddMenu();
	void activatedAddMenu(int);
signals:
	void updateNeeded();
	void initialCopyNeeded();
private:
	TQGuardedPtr<class TQPopupMenu> menu;
	TQPtrVector<TQString> libNames;
	TQPtrVector<TQString> libParam;
	bool m_universal;
	TQString m_currentProfile;
	void doRollBack();
	TQWidget *m_parent;
};

class KDE_EXPORT Sidebar_Widget: public TQWidget
{
	Q_OBJECT
public:
	friend class ButtonInfo;
public:
	Sidebar_Widget(TQWidget *parent, KParts::ReadOnlyPart *par,
						const char * name,bool universalMode, 
						const TQString &currentProfile);
	~Sidebar_Widget();
	bool openURL(const class KURL &url);
	void stdAction(const char *handlestd);
	//virtual KParts::ReadOnlyPart *getPart();
	KParts::BrowserExtension *getExtension();
        virtual TQSize sizeHint() const;	

public slots:
	void addWebSideBar(const KURL& url, const TQString& name);

protected:
	void customEvent(TQCustomEvent* ev);
	void resizeEvent(TQResizeEvent* ev);
	virtual bool eventFilter(TQObject*,TQEvent*);
	virtual void mousePressEvent(TQMouseEvent*);

protected slots:
	void showHidePage(int value);
	void createButtons();
	void updateButtons();
	void finishRollBack();
	void activatedMenu(int id);
	void buttonPopupActivate(int);
  	void dockWidgetHasUndocked(KDockWidget*);
	void aboutToShowConfigMenu();
	void saveConfig();

signals:
	void started(KIO::Job *);
	void completed();
	void fileSelection(const KFileItemList& iems);
	void fileMouseOver(const KFileItem& item);

public:
	/* interface KonqSidebar_PluginInterface*/
	KInstance  *getInstance();
//        virtual void showError(TQString &);      for later extension
//        virtual void showMessage(TQString &);    for later extension
	/* end of interface implementation */


 /* The following public slots are wrappers for browserextension fields */
public slots:
	void openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
	/* @internal
	 * @since 3.2
	 * ### KDE4 remove me
	 */
	void submitFormRequest(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&);
  	void createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
	void createNewWindow( const KURL &url, const KParts::URLArgs &args,
             const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part );

	void popupMenu( const TQPoint &global, const KFileItemList &items );
  	void popupMenu( KXMLGUIClient *client, const TQPoint &global, const KFileItemList &items );
	void popupMenu( const TQPoint &global, const KURL &url,
		const TQString &mimeType, mode_t mode = (mode_t)-1 );
	void popupMenu( KXMLGUIClient *client,
		const TQPoint &global, const KURL &url,
		const TQString &mimeType, mode_t mode = (mode_t)-1 );
	void enableAction( const char * name, bool enabled );
	void userMovedSplitter();
	
private:
	TQSplitter *splitter() const;
	bool addButton(const TQString &desktoppath,int pos=-1);
	bool createView(ButtonInfo *data);
	KonqSidebarPlugin *loadModule(TQWidget *par,TQString &desktopName,TQString lib_name,ButtonInfo *bi);
	void readConfig();
	void initialCopy();
	void doLayout();
	void connectModule(TQObject *mod);
	void collapseExpandSidebar();
	bool doEnableActions();
	bool m_universalMode;
	bool m_userMovedSplitter;
private:
	KParts::ReadOnlyPart *m_partParent;
	KDockArea *m_area;
	KDockWidget *m_mainDockWidget;

	KMultiTabBar *m_buttonBar;
        TQPtrVector<ButtonInfo> m_buttons;
	TQHBoxLayout *m_layout;
	KPopupMenu *m_buttonPopup;
	TQPopupMenu *m_menu;
	TQGuardedPtr<ButtonInfo> m_activeModule;
	TQGuardedPtr<ButtonInfo> m_currentButton;
	
	KConfig *m_config;
	TQTimer m_configTimer;
	
	KURL m_storedUrl;
	int m_savedWidth;
	int m_latestViewed;

	bool m_hasStoredUrl;
	bool m_singleWidgetMode;
        bool m_immutableSingleWidgetMode;
	bool m_showTabsLeft;
        bool m_immutableShowTabsLeft;
	bool m_hideTabs;
        bool m_immutableHideTabs;
        bool m_disableConfig;
	bool m_showExtraButtons;
        bool m_immutableShowExtraButtons;
	bool m_somethingVisible;
	bool m_noUpdate;
	bool m_initial;

	TQString m_path;
	TQString m_relPath;
	TQString m_currentProfile;
	TQStringList m_visibleViews; // The views that are actually open
	TQStringList m_openViews; // The views that should be opened

signals:
	void panelHasBeenExpanded(bool);
};

#endif
