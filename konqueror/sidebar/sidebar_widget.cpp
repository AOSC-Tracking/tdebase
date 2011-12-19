/***************************************************************************
                               sidebar_widget.cpp
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
#include <config.h>

#include <limits.h>

#include <tqdir.h>
#include <tqpopupmenu.h>
#include <tqhbox.h>
#include <tqpushbutton.h>
#include <tqwhatsthis.h>
#include <tqlayout.h>
#include <tqstringlist.h>
#include <tqucomextra_p.h>

#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kicondialog.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <konq_events.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <kpopupmenu.h>
#include <kprocess.h>
#include <kurlrequesterdlg.h>
#include <kinputdialog.h>
#include <kfiledialog.h>
#include <kdesktopfile.h>
#include "konqsidebar.h"

#include "sidebar_widget.h"
#include "sidebar_widget.moc"


addBackEnd::addBackEnd(TQWidget *parent, class TQPopupMenu *addmenu,
                       bool universal, const TQString &currentProfile,
                       const char *name)
 : TQObject(parent,name),
   m_parent(parent)
{
	m_universal=universal;
	m_currentProfile = currentProfile;
	menu = addmenu;
	connect(menu,TQT_SIGNAL(aboutToShow()),this,TQT_SLOT(aboutToShowAddMenu()));
	connect(menu,TQT_SIGNAL(activated(int)),this,TQT_SLOT(activatedAddMenu(int)));
}

void addBackEnd::aboutToShowAddMenu()
{
	if (!menu)
		return;
	KStandardDirs *dirs = KGlobal::dirs();
	TQStringList list = dirs->findAllResources("data","konqsidebartng/add/*.desktop",true,true);
	libNames.setAutoDelete(true);
	libNames.resize(0);
	libParam.setAutoDelete(true);
	libParam.resize(0);
	menu->clear();
	int i = 0;

	for (TQStringList::Iterator it = list.begin(); it != list.end(); ++it, i++ )
	{
		KDesktopFile *confFile;

		confFile = new KDesktopFile(*it, true);
		if (! confFile->tryExec()) {
			delete confFile;
			i--;
			continue;
		}
		if (m_universal) {
			if (confFile->readEntry("X-KDE-KonqSidebarUniversal").upper()!="TRUE") {
				delete confFile;
				i--;
				continue;
			}
		} else {
			if (confFile->readEntry("X-KDE-KonqSidebarBrowser").upper()=="FALSE") {
				delete confFile;
				i--;
				continue;
			}
		}
		TQString icon = confFile->readIcon();
		if (!icon.isEmpty())
		{
			menu->insertItem(SmallIcon(icon),
					 confFile->readEntry("Name"), i);
		} else {
			menu->insertItem(confFile->readEntry("Name"), i);
		}
		libNames.resize(libNames.size()+1);
		libNames.insert(libNames.count(), new TQString(confFile->readEntry("X-KDE-KonqSidebarAddModule")));
		libParam.resize(libParam.size()+1);
		libParam.insert(libParam.count(), new TQString(confFile->readEntry("X-KDE-KonqSidebarAddParam")));
		delete confFile;
	}
        menu->insertSeparator();
        menu->insertItem(i18n("Rollback to System Default"), i);
}


void addBackEnd::doRollBack()
{
	if (KMessageBox::warningContinueCancel(m_parent, i18n("<qt>This removes all your entries from the sidebar and adds the system default ones.<BR><B>This procedure is irreversible</B><BR>Do you want to proceed?</qt>"))==KMessageBox::Continue)
	{
		KStandardDirs *dirs = KGlobal::dirs();
		TQString loc=dirs->saveLocation("data","konqsidebartng/" + m_currentProfile + "/",true);
		TQDir dir(loc);
		TQStringList dirEntries = dir.entryList( TQDir::Dirs | TQDir::NoSymLinks );
		dirEntries.remove(".");
		dirEntries.remove("..");
		for ( TQStringList::Iterator it = dirEntries.begin(); it != dirEntries.end(); ++it ) {
			if ((*it)!="add")
				 KIO::NetAccess::del(KURL( loc+(*it) ), m_parent);
		}
		emit initialCopyNeeded();
	}
}


static TQString findFileName(const TQString* tmpl,bool universal, const TQString &profile) {
	TQString myFile, filename;
	KStandardDirs *dirs = KGlobal::dirs();
	TQString tmp = *tmpl;

	if (universal) {
		dirs->saveLocation("data", "konqsidebartng/kicker_entries/", true);
		tmp.prepend("/konqsidebartng/kicker_entries/");
	} else {
		dirs->saveLocation("data", "konqsidebartng/" + profile + "/entries/", true);
		tmp.prepend("/konqsidebartng/" + profile + "/entries/");
	}
	filename = tmp.arg("");
	myFile = locateLocal("data", filename);

	if (TQFile::exists(myFile)) {
		for (ulong l = 0; l < ULONG_MAX; l++) {
			filename = tmp.arg(l);
			myFile = locateLocal("data", filename);
			if (!TQFile::exists(myFile)) {
				break;
			} else {
				myFile = TQString::null;
			}
		}
	}

	return myFile;
}

void addBackEnd::activatedAddMenu(int id)
{
	kdDebug() << "activatedAddMenu: " << TQString("%1").arg(id) << endl;
	if (((uint)id) == libNames.size())
		doRollBack();
	if(((uint)id) >= libNames.size())
		return;

	KLibLoader *loader = KLibLoader::self();

        // try to load the library
	TQString libname = *libNames.at(id);
        KLibrary *lib = loader->library(TQFile::encodeName(libname));
        if (lib)
       	{
		// get the create_ function
		TQString factory("add_");
		factory = factory+(*libNames.at(id));
		void *add = lib->symbol(TQFile::encodeName(factory));

		if (add)
		{
			//call the add function
			bool (*func)(TQString*, TQString*, TQMap<TQString,TQString> *);
			TQMap<TQString,TQString> map;
			func = (bool (*)(TQString*, TQString*, TQMap<TQString,TQString> *)) add;
			TQString *tmp = new TQString("");
			if (func(tmp,libParam.at(id),&map))
			{
				TQString myFile = findFileName(tmp,m_universal,m_currentProfile);

				if (!myFile.isEmpty())
				{
					kdDebug() <<"trying to save to file: "<<myFile << endl;
					KSimpleConfig scf(myFile,false);
					scf.setGroup("Desktop Entry");
					for (TQMap<TQString,TQString>::ConstIterator it = map.begin(); it != map.end(); ++it) {
						kdDebug() <<"writing:"<<it.key()<<" / "<<it.data()<<endl;
						scf.writePathEntry(it.key(), it.data());
					}
					scf.sync();
					emit updateNeeded();

				} else {
					kdWarning() << "No unique filename found" << endl;
				}
			} else {
				kdWarning() << "No new entry (error?)" << endl;
			}
			delete tmp;
		}
	} else {
		kdWarning() << "libname:" << libNames.at(id)
			<< " doesn't specify a library!" << endl;
	}
}


/**************************************************************/
/*                      Sidebar_Widget                        */
/**************************************************************/

Sidebar_Widget::Sidebar_Widget(TQWidget *parent, KParts::ReadOnlyPart *par, const char *name,bool universalMode, const TQString &currentProfile)
	:TQWidget(parent,name),m_universalMode(universalMode),m_partParent(par),m_currentProfile(currentProfile)
{
	m_somethingVisible = false;
	m_initial = true;
	m_noUpdate = false;
	m_layout = 0;
	m_currentButton = 0;
	m_activeModule = 0;
	m_userMovedSplitter = false;
        //kdDebug() << "**** Sidebar_Widget:SidebarWidget()"<<endl;
	if (universalMode)
	{
		m_relPath = "konqsidebartng/kicker_entries/";
	}
	else
	{
		m_relPath = "konqsidebartng/" + currentProfile + "/entries/";
	}
	m_path = KGlobal::dirs()->saveLocation("data", m_relPath, true);
	m_buttons.setAutoDelete(true);
	m_hasStoredUrl = false;
	m_latestViewed = -1;
	setSizePolicy(TQSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding));

	TQSplitter *splitterWidget = splitter();
	if (splitterWidget) {
		splitterWidget->setResizeMode(parent, TQSplitter::FollowSizeHint);
		splitterWidget->setOpaqueResize( false );
		connect(splitterWidget,TQT_SIGNAL(setRubberbandCalled()),TQT_SLOT(userMovedSplitter()));
	}
		
	m_area = new KDockArea(this);
	m_area->setSizePolicy(TQSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding));
	m_mainDockWidget = m_area->createDockWidget("free", 0);
	m_mainDockWidget->setWidget(new TQWidget(m_mainDockWidget));
	m_area->setMainDockWidget(m_mainDockWidget);
	m_area->setMinimumWidth(0);
	m_mainDockWidget->setDockSite(KDockWidget::DockTop);
	m_mainDockWidget->setEnableDocking(KDockWidget::DockNone);

	m_buttonBar = new KMultiTabBar(KMultiTabBar::Vertical,this);
	m_buttonBar->showActiveTabTexts(true);

	m_menu = new TQPopupMenu(this, "Sidebar_Widget::Menu");
	TQPopupMenu *addMenu = new TQPopupMenu(this, "Sidebar_Widget::addPopup");
	m_menu->insertItem(i18n("Add New"), addMenu, 0);
	m_menu->insertItem(i18n("Multiple Views"), 1);
	m_menu->insertItem(i18n("Show Tabs Left"), 2);
	m_menu->insertItem(i18n("Show Configuration Button"), 3);
	if (!m_universalMode) {
		m_menu->insertItem(SmallIconSet("remove"),
                                   i18n("Close Navigation Panel"),
				par, TQT_SLOT(deleteLater()));
	}
        connect(m_menu, TQT_SIGNAL(aboutToShow()),
		this, TQT_SLOT(aboutToShowConfigMenu()));
	connect(m_menu, TQT_SIGNAL(activated(int)),
		this, TQT_SLOT(activatedMenu(int)));

	m_buttonPopup = 0;
	addBackEnd *ab = new addBackEnd(this, addMenu, universalMode,
                                        currentProfile,
                                        "Sidebar_Widget-addBackEnd");

	connect(ab, TQT_SIGNAL(updateNeeded()),
		this, TQT_SLOT(updateButtons()));
	connect(ab, TQT_SIGNAL(initialCopyNeeded()),
		this, TQT_SLOT(finishRollBack()));

	initialCopy();

        if (universalMode)
        {
                m_config = new KConfig("konqsidebartng_kicker.rc");
        }
        else
        {
                m_config = new KConfig("konqsidebartng.rc");
                m_config->setGroup(currentProfile);
        }
        readConfig();

        // Disable stuff (useful for Kiosk mode)!
        m_menu->setItemVisible(1, !m_immutableSingleWidgetMode);
        m_menu->setItemVisible(2, !m_immutableShowTabsLeft);
        m_menu->setItemVisible(3, !m_immutableShowExtraButtons);

	connect(&m_configTimer, TQT_SIGNAL(timeout()),
		this, TQT_SLOT(saveConfig()));
	m_somethingVisible = !m_openViews.isEmpty();
	doLayout();
	TQTimer::singleShot(0,this,TQT_SLOT(createButtons()));
	connect(m_area, TQT_SIGNAL(dockWidgetHasUndocked(KDockWidget*)),
		this, TQT_SLOT(dockWidgetHasUndocked(KDockWidget*)));
}

void Sidebar_Widget::addWebSideBar(const KURL& url, const TQString& /*name*/) {
	//kdDebug() << "Web sidebar entry to be added: " << url.url()
	//	<< " [" << name << "]" << endl;

	// Look for existing ones with this URL
	KStandardDirs *dirs = KGlobal::dirs();
	TQString list;
	dirs->saveLocation("data", m_relPath, true);
	list = locateLocal("data", m_relPath);

	// Go through list to see which ones exist.  Check them for the URL
	TQStringList files = TQDir(list).entryList("websidebarplugin*.desktop");
	for (TQStringList::Iterator it = files.begin(); it != files.end(); ++it){
		KSimpleConfig scf(list + *it, false);
		scf.setGroup("Desktop Entry");
		if (scf.readPathEntry("URL", TQString::null) == url.url()) {
			// We already have this one!
			KMessageBox::information(this,
					i18n("This entry already exists."));
			return;
		}
	}

	TQString tmpl = "websidebarplugin%1.desktop";
	TQString myFile = findFileName(&tmpl,m_universalMode,m_currentProfile);

	if (!myFile.isEmpty()) {
		KSimpleConfig scf(myFile, false);
		scf.setGroup("Desktop Entry");
		scf.writeEntry("Type", "Link");
		scf.writePathEntry("URL", url.url());
		scf.writeEntry("Icon", "netscape");
		scf.writeEntry("Name", i18n("Web SideBar Plugin"));
		scf.writeEntry("Open", "true");
		scf.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_web");
		scf.sync();

		TQTimer::singleShot(0,this,TQT_SLOT(updateButtons()));
	}
}


void Sidebar_Widget::finishRollBack()
{
	m_path = KGlobal::dirs()->saveLocation("data",m_relPath,true);
        initialCopy();
        TQTimer::singleShot(0,this,TQT_SLOT(updateButtons()));
}


void Sidebar_Widget::saveConfig()
{
	m_config->writeEntry("SingleWidgetMode",m_singleWidgetMode);
	m_config->writeEntry("ShowExtraButtons",m_showExtraButtons);
	m_config->writeEntry("ShowTabsLeft", m_showTabsLeft);
	m_config->writeEntry("HideTabs", m_hideTabs);
	m_config->writeEntry("SavedWidth",m_savedWidth);
	m_config->sync();
}

void Sidebar_Widget::doLayout()
{
	if (m_layout)
		delete m_layout;

	m_layout = new TQHBoxLayout(this);
	if  (m_showTabsLeft)
	{
		m_layout->add(m_buttonBar);
		m_layout->add(m_area);
		m_buttonBar->setPosition(KMultiTabBar::Left);
	} else {
		m_layout->add(m_area);
		m_layout->add(m_buttonBar);
		m_buttonBar->setPosition(KMultiTabBar::Right);
	}
	m_layout->activate();
	if (m_hideTabs) m_buttonBar->hide(); 
		else m_buttonBar->show();
}


void Sidebar_Widget::aboutToShowConfigMenu()
{
	m_menu->setItemChecked(1, !m_singleWidgetMode);
	m_menu->setItemChecked(2, m_showTabsLeft);
	m_menu->setItemChecked(3, m_showExtraButtons);
}


void Sidebar_Widget::initialCopy()
{
	kdDebug()<<"Initial copy"<<endl;
	TQStringList dirtree_dirs;
	if (m_universalMode)
		dirtree_dirs = KGlobal::dirs()->findDirs("data","konqsidebartng/kicker_entries/");
	else
		dirtree_dirs = KGlobal::dirs()->findDirs("data","konqsidebartng/entries/");
	if (dirtree_dirs.last()==m_path)
		return; //oups;

	int nVersion=-1;
	KSimpleConfig lcfg(m_path+".version");
	int lVersion=lcfg.readNumEntry("Version",0);


	for (TQStringList::const_iterator ddit=dirtree_dirs.begin();ddit!=dirtree_dirs.end();++ddit) {
		TQString dirtree_dir=*ddit;
		if (dirtree_dir == m_path) continue;


		kdDebug()<<"************************************ retrieving directory info:"<<dirtree_dir<<endl;

	        if ( !dirtree_dir.isEmpty() && dirtree_dir != m_path )
        	{
			KSimpleConfig gcfg(dirtree_dir+".version");
			int gversion = gcfg.readNumEntry("Version", 1);
			nVersion=(nVersion>gversion)?nVersion:gversion;
			if (lVersion >= gversion)
				continue;

	 	        TQDir dir(m_path);
    		        TQStringList entries = dir.entryList( TQDir::Files );
                	TQStringList dirEntries = dir.entryList( TQDir::Dirs | TQDir::NoSymLinks );
	                dirEntries.remove( "." );
        	        dirEntries.remove( ".." );

	                TQDir globalDir( dirtree_dir );
        	        Q_ASSERT( globalDir.isReadable() );
	                // Only copy the entries that don't exist yet in the local dir
        	        TQStringList globalDirEntries = globalDir.entryList();
                	TQStringList::ConstIterator eIt = globalDirEntries.begin();
	                TQStringList::ConstIterator eEnd = globalDirEntries.end();
        	        for (; eIt != eEnd; ++eIt )
                	{
                		//kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir contains " << *eIt << endl;
	                	if ( *eIt != "." && *eIt != ".." &&
					!entries.contains( *eIt ) &&
					!dirEntries.contains( *eIt ) )
				{ // we don't have that one yet -> copy it.
					TQString cp("cp -R -- ");
					cp += KProcess::quote(dirtree_dir + *eIt);
					cp += " ";
					cp += KProcess::quote(m_path);
					kdDebug() << "SidebarWidget::intialCopy executing " << cp << endl;
					::system( TQFile::encodeName(cp) );
				}
			}
		}

			lcfg.writeEntry("Version",(nVersion>lVersion)?nVersion:lVersion);
			lcfg.sync();

	}
}

void Sidebar_Widget::buttonPopupActivate(int id)
{
	switch (id)
	{
		case 1:
		{
			KIconDialog kicd(this);
//			kicd.setStrictIconSize(true);
			TQString iconname=kicd.selectIcon(KIcon::Small);
			kdDebug()<<"New Icon Name:"<<iconname<<endl;
			if (!iconname.isEmpty())
			{
				KSimpleConfig ksc(m_path+m_currentButton->file);
				ksc.setGroup("Desktop Entry");
				ksc.writeEntry("Icon",iconname);
				ksc.sync();
			        TQTimer::singleShot(0,this,TQT_SLOT(updateButtons()));
			}
			break;
		}
		case 2:
		{
                        KURLRequesterDlg * dlg = new KURLRequesterDlg( m_currentButton->URL, i18n("Enter a URL:"), this, "url_dlg" );
                        dlg->fileDialog()->setMode( KFile::Directory );
			if (dlg->exec())
			{
				KSimpleConfig ksc(m_path+m_currentButton->file);
				ksc.setGroup("Desktop Entry");
                                if ( !dlg->selectedURL().isValid())
                                {
                                    KMessageBox::error(this, i18n("<qt><b>%1</b> does not exist</qt>").arg(dlg->selectedURL().url()));
                                }
                                else
                                {
                                    TQString newurl= dlg->selectedURL().prettyURL();
                                    //If we are going to set the name by 'set name', we don't set it here.
                                    //ksc.writeEntry("Name",newurl);
                                    ksc.writePathEntry("URL",newurl);
                                    ksc.sync();
                                    TQTimer::singleShot(0,this,TQT_SLOT(updateButtons()));
                                }
			}
                        delete dlg;
			break;
		}
		case 3:
		{
			if (KMessageBox::warningContinueCancel(this,i18n("<qt>Do you really want to remove the <b>%1</b> tab?</qt>").arg(m_currentButton->displayName),
                            TQString::null,KStdGuiItem::del())==KMessageBox::Continue)
			{
				TQFile f(m_path+m_currentButton->file);
				if (!f.remove())
					qDebug("Error, file not deleted");
				TQTimer::singleShot(0,this,TQT_SLOT(updateButtons()));
			}
			break;
		}
		case 4: // Set a name for this sidebar tab
		{
			bool ok;
			
			// Pop up the dialog asking the user for name.
			const TQString name = KInputDialog::getText(i18n("Set Name"), i18n("Enter the name:"),
			        m_currentButton->displayName, &ok, this);
			
			if(ok)
			{
				// Write the name in the .desktop file of this side button.
				KSimpleConfig ksc(m_path+m_currentButton->file);
				ksc.setGroup("Desktop Entry");
				ksc.writeEntry("Name", name, true, false, true /*localized*/ );
				ksc.sync();
				
				// Update the buttons with a TQTimer (why?)
				TQTimer::singleShot(0,this,TQT_SLOT(updateButtons()));
			}
			break;
		}
	}
}

void Sidebar_Widget::activatedMenu(int id)
{
	switch (id)
	{
		case 1:
		{
			m_singleWidgetMode = !m_singleWidgetMode;
			if ((m_singleWidgetMode) && (m_visibleViews.count()>1))
			{
				int tmpViewID=m_latestViewed;
				for (uint i=0; i<m_buttons.count(); i++) {
					ButtonInfo *button = m_buttons.at(i);
					if ((int) i != tmpViewID)
					{
						if (button->dock && button->dock->isVisibleTo(this))
							showHidePage(i);
					} else {
						if (button->dock)
						{
							m_area->setMainDockWidget(button->dock);
							m_mainDockWidget->undock();
						}
					}
				}
				m_latestViewed=tmpViewID;
			} else {
				if (!m_singleWidgetMode)
				{
					int tmpLatestViewed=m_latestViewed;
					m_area->setMainDockWidget(m_mainDockWidget);
	        			m_mainDockWidget->setDockSite(KDockWidget::DockTop);
				        m_mainDockWidget->setEnableDocking(KDockWidget::DockNone);
					m_mainDockWidget->show();
					if ((tmpLatestViewed>=0) && (tmpLatestViewed < (int) m_buttons.count()))
					{
						ButtonInfo *button = m_buttons.at(tmpLatestViewed);
						if (button && button->dock)
						{
							m_noUpdate=true;
							button->dock->undock();
			                                button->dock->setEnableDocking(KDockWidget::DockTop|
                                				KDockWidget::DockBottom/*|KDockWidget::DockDesktop*/);
							kdDebug()<<"Reconfiguring multi view mode"<<endl;
							m_buttonBar->setTab(tmpLatestViewed,true);
							showHidePage(tmpLatestViewed);
						}
					}
				}
			}
			break;
		}
		case 2:
		{
			m_showTabsLeft = ! m_showTabsLeft;
			doLayout();
			break;
		}
		case 3:
		{
			m_showExtraButtons = ! m_showExtraButtons;
			if(m_showExtraButtons)
			{
				m_buttonBar->button(-1)->show();
			}
			else
			{
				m_buttonBar->button(-1)->hide();

				KMessageBox::information(this,
				i18n("You have hidden the navigation panel configuration button. To make it visible again, click the right mouse button on any of the navigation panel buttons and select \"Show Configuration Button\"."));

			}
			break;
		}
		default:
			return;
	}
	m_configTimer.start(400, true);
}

void Sidebar_Widget::readConfig()
{
        m_disableConfig = m_config->readBoolEntry("DisableConfig",false);
	m_singleWidgetMode = m_config->readBoolEntry("SingleWidgetMode",true);
        m_immutableSingleWidgetMode =
                m_config->entryIsImmutable("SingleWidgetMode");
	m_showExtraButtons = m_config->readBoolEntry("ShowExtraButtons",false);
        m_immutableShowExtraButtons =
                m_config->entryIsImmutable("ShowExtraButtons");
	m_showTabsLeft = m_config->readBoolEntry("ShowTabsLeft", true);
        m_immutableShowTabsLeft = m_config->entryIsImmutable("ShowTabsLeft");
	m_hideTabs = m_config->readBoolEntry("HideTabs", false);
        m_immutableHideTabs = m_config->entryIsImmutable("HideTabs");

	if (m_initial) {
		m_openViews = m_config->readListEntry("OpenViews");
		m_savedWidth = m_config->readNumEntry("SavedWidth",200);
		m_initial=false;
	}
}

void Sidebar_Widget::stdAction(const char *handlestd)
{
	ButtonInfo* mod = m_activeModule;

	if (!mod)
		return;
	if (!(mod->module))
		return;

	kdDebug() << "Try calling >active< module's (" << mod->module->className() << ") slot " << handlestd << endl;

	int id = mod->module->metaObject()->findSlot( handlestd );
  	if ( id == -1 )
		return;
	kdDebug() << "Action slot was found, it will be called now" << endl;
  	TQUObject o[ 1 ];
	mod->module->qt_invoke( id, o );
  	return;
}


void Sidebar_Widget::updateButtons()
{
	//PARSE ALL DESKTOP FILES
	m_openViews = m_visibleViews;

	if (m_buttons.count() > 0)
	{
		for (uint i = 0; i < m_buttons.count(); i++)
		{
			ButtonInfo *button = m_buttons.at(i);
			if (button->dock)
			{
				m_noUpdate = true;
				if (button->dock->isVisibleTo(this)) {
					showHidePage(i);
				}

				delete button->module;
				delete button->dock;
			}
			m_buttonBar->removeTab(i);

		}
	}
	m_buttons.clear();

	readConfig();
	doLayout();
	createButtons();
}

void Sidebar_Widget::createButtons()
{
	if (!m_path.isEmpty())
	{
		kdDebug()<<"m_path: "<<m_path<<endl;
		TQDir dir(m_path);
		TQStringList list=dir.entryList("*.desktop");
		for (TQStringList::Iterator it=list.begin(); it!=list.end(); ++it)
		{
			addButton(*it);
		}
	}

	if (!m_buttonBar->button(-1)) {
		m_buttonBar->appendButton(SmallIcon("configure"), -1, m_menu,
					i18n("Configure Sidebar"));
	}

	if (m_showExtraButtons && !m_disableConfig) {
		m_buttonBar->button(-1)->show();
	} else {
		m_buttonBar->button(-1)->hide();
	}

	for (uint i = 0; i < m_buttons.count(); i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (m_openViews.contains(button->file))
		{
			m_buttonBar->setTab(i,true);
			m_noUpdate = true;
			showHidePage(i);
			if (m_singleWidgetMode) {
				break;
			}
		}
	}

	collapseExpandSidebar();
        m_noUpdate=false;
}

bool Sidebar_Widget::openURL(const class KURL &url)
{
	if (url.protocol()=="sidebar")
	{
		for (unsigned int i=0;i<m_buttons.count();i++)
			if (m_buttons.at(i)->file==url.path())
			{
				KMultiTabBarTab *tab = m_buttonBar->tab(i);
				if (!tab->isOn())
					tab->animateClick();
				return true;
			}
		return false;
	}

	m_storedUrl=url;
	m_hasStoredUrl=true;
        bool ret = false;
	for (unsigned int i=0;i<m_buttons.count();i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (button->dock)
		{
			if ((button->dock->isVisibleTo(this)) && (button->module))
			{
				ret = true;
				button->module->openURL(url);
			}
		}
	}
	return ret;
}

bool Sidebar_Widget::addButton(const TQString &desktoppath,int pos)
{
	int lastbtn = m_buttons.count();
	m_buttons.resize(m_buttons.size()+1);

  	KSimpleConfig *confFile;

	kdDebug() << "addButton:" << (m_path+desktoppath) << endl;

	confFile = new KSimpleConfig(m_path+desktoppath,true);
	confFile->setGroup("Desktop Entry");

    	TQString icon = confFile->readEntry("Icon");
	TQString name = confFile->readEntry("Name");
	TQString comment = confFile->readEntry("Comment");
	TQString url = confFile->readPathEntry("URL",TQString::null);
	TQString lib = confFile->readEntry("X-KDE-KonqSidebarModule");

        delete confFile;

	if (pos == -1)
	{
	  	m_buttonBar->appendTab(SmallIcon(icon), lastbtn, name);
		ButtonInfo *bi = new ButtonInfo(desktoppath, ((KonqSidebar*)m_partParent),0, url, lib, name,
						icon, TQT_TQOBJECT(this));
		/*int id=*/m_buttons.insert(lastbtn, bi);
		KMultiTabBarTab *tab = m_buttonBar->tab(lastbtn);
		tab->installEventFilter(this);
		connect(tab,TQT_SIGNAL(clicked(int)),this,TQT_SLOT(showHidePage(int)));

		// Set Whats This help
		// This uses the comments in the .desktop files
		TQWhatsThis::add(tab, comment);
	}

	return true;
}



bool Sidebar_Widget::eventFilter(TQObject *obj, TQEvent *ev)
{

	if (ev->type()==TQEvent::MouseButtonPress && ((TQMouseEvent *)ev)->button()==Qt::RightButton)
	{
		KMultiTabBarTab *bt=tqt_dynamic_cast<KMultiTabBarTab*>(obj);
		if (bt)
		{
			kdDebug()<<"Request for popup"<<endl;
			m_currentButton = 0;
			for (uint i=0;i<m_buttons.count();i++)
			{
				if (bt==m_buttonBar->tab(i))
				{
					m_currentButton = m_buttons.at(i);
					break;
				}
			}

			if (m_currentButton)
			{
				if (!m_buttonPopup)
				{
					m_buttonPopup=new KPopupMenu(this, "Sidebar_Widget::ButtonPopup");
					m_buttonPopup->insertTitle(SmallIcon("unknown"), "", 50);
					m_buttonPopup->insertItem(SmallIconSet("text"), i18n("Set Name..."),4); // Item to open a dialog to change the name of the sidebar item (by Pupeno)
					m_buttonPopup->insertItem(SmallIconSet("www"), i18n("Set URL..."),2);
					m_buttonPopup->insertItem(SmallIconSet("icons"), i18n("Set Icon..."),1);
					m_buttonPopup->insertSeparator();
					m_buttonPopup->insertItem(SmallIconSet("editdelete"), i18n("Remove"),3);
					m_buttonPopup->insertSeparator();
					m_buttonPopup->insertItem(SmallIconSet("configure"), i18n("Configure Navigation Panel"), m_menu, 4);
					connect(m_buttonPopup, TQT_SIGNAL(activated(int)),
						this, TQT_SLOT(buttonPopupActivate(int)));
				}
				m_buttonPopup->setItemEnabled(2,!m_currentButton->URL.isEmpty());
			        m_buttonPopup->changeTitle(50,SmallIcon(m_currentButton->iconName),
						m_currentButton->displayName);
				if (!m_disableConfig)
                                { m_buttonPopup->exec(TQCursor::pos()); }
			}
			return true;

		}
	}
	return false;
}

void Sidebar_Widget::mousePressEvent(TQMouseEvent *ev)
{
	if (ev->type()==TQEvent::MouseButtonPress &&
            ((TQMouseEvent *)ev)->button()==Qt::RightButton &&
            !m_disableConfig)
        { m_menu->exec(TQCursor::pos()); }
}

KonqSidebarPlugin *Sidebar_Widget::loadModule(TQWidget *par,TQString &desktopName,TQString lib_name,ButtonInfo* bi)
{
	KLibLoader *loader = KLibLoader::self();

	// try to load the library
      	KLibrary *lib = loader->library(TQFile::encodeName(lib_name));
	if (lib)
	{
		// get the create_ function
		TQString factory("create_%1");
		void *create = lib->symbol(TQFile::encodeName(factory.arg(lib_name)));

		if (create)
		{
			// create the module

			KonqSidebarPlugin* (*func)(KInstance*,TQObject *, TQWidget*, TQString&, const char *);
			func = (KonqSidebarPlugin* (*)(KInstance*,TQObject *, TQWidget *, TQString&, const char *)) create;
			TQString fullPath(m_path+desktopName);
			return  (KonqSidebarPlugin*)func(getInstance(),bi,par,fullPath,0);
		}
	} else {
		kdWarning() << "Module " << lib_name << " doesn't specify a library!" << endl;
	}
	return 0;
}

KParts::BrowserExtension *Sidebar_Widget::getExtension()
{
	return KParts::BrowserExtension::childObject(m_partParent);
}

bool Sidebar_Widget::createView( ButtonInfo *data)
{
	bool ret = true;
	KSimpleConfig *confFile;
	confFile = new KSimpleConfig(data->file,true);
	confFile->setGroup("Desktop Entry");

	data->dock = m_area->createDockWidget(confFile->readEntry("Name",i18n("Unknown")),0);
	data->module = loadModule(data->dock,data->file,data->libName,data);

	if (data->module == 0)
	{
		delete data->dock;
		data->dock = 0;
		ret = false;
	} else {
		data->dock->setWidget(data->module->getWidget());
		data->dock->setEnableDocking(KDockWidget::DockTop|
		KDockWidget::DockBottom/*|KDockWidget::DockDesktop*/);
		data->dock->setDockSite(KDockWidget::DockTop|KDockWidget::DockBottom);
		connectModule(data->module);
		connect(this, TQT_SIGNAL(fileSelection(const KFileItemList&)),
			data->module, TQT_SLOT(openPreview(const KFileItemList&)));

		connect(this, TQT_SIGNAL(fileMouseOver(const KFileItem&)),
			data->module, TQT_SLOT(openPreviewOnMouseOver(const KFileItem&)));
	}

	delete confFile;
	return ret;
}

void Sidebar_Widget::showHidePage(int page)
{
	ButtonInfo *info = m_buttons.at(page);
	if (!info->dock)
	{
		if (m_buttonBar->isTabRaised(page))
		{
			//SingleWidgetMode
			if (m_singleWidgetMode)
			{
				if (m_latestViewed != -1)
				{
					m_noUpdate = true;
					showHidePage(m_latestViewed);
				}
			}

			if (!createView(info))
			{
				m_buttonBar->setTab(page,false);
				return;
			}

			m_buttonBar->setTab(page,true);

			connect(info->module,
				TQT_SIGNAL(setIcon(const TQString&)),
				m_buttonBar->tab(page),
				TQT_SLOT(setIcon(const TQString&)));

			connect(info->module,
				TQT_SIGNAL(setCaption(const TQString&)),
				m_buttonBar->tab(page),
				TQT_SLOT(setText(const TQString&)));

			if (m_singleWidgetMode)
			{
				m_area->setMainDockWidget(info->dock);
				m_mainDockWidget->undock();
			} else {
				info->dock->manualDock(m_mainDockWidget,KDockWidget::DockTop,100);
			}

			info->dock->show();

			if (m_hasStoredUrl)
				info->module->openURL(m_storedUrl);
			m_visibleViews<<info->file;
			m_latestViewed=page;
		}
	} else {
		if ((!info->dock->isVisibleTo(this)) && (m_buttonBar->isTabRaised(page))) {
			//SingleWidgetMode
			if (m_singleWidgetMode) {
				if (m_latestViewed != -1) {
					m_noUpdate = true;
					showHidePage(m_latestViewed);
				}
			}

			if (m_singleWidgetMode) {
				m_area->setMainDockWidget(info->dock);
				m_mainDockWidget->undock();
			} else {
				info->dock->manualDock(m_mainDockWidget,KDockWidget::DockTop,100);
			}

			info->dock->show();
			m_latestViewed = page;
			if (m_hasStoredUrl)
				info->module->openURL(m_storedUrl);
			m_visibleViews << info->file;
			m_buttonBar->setTab(page,true);
		} else {
			m_buttonBar->setTab(page,false);
			if (m_singleWidgetMode) {
				m_area->setMainDockWidget(m_mainDockWidget);
				m_mainDockWidget->show();
			}
			info->dock->undock();
			m_latestViewed = -1;
			m_visibleViews.remove(info->file);
		}
	}

	if (!m_noUpdate)
		collapseExpandSidebar();
	m_noUpdate = false;
}

void Sidebar_Widget::collapseExpandSidebar()
{
	if (!parentWidget())
		return; // Can happen during destruction
		
	if (m_visibleViews.count()==0)
	{
		m_somethingVisible = false;
		parentWidget()->setMaximumWidth(minimumSizeHint().width());
		updateGeometry();
		emit panelHasBeenExpanded(false);
	} else {
		m_somethingVisible = true;
		parentWidget()->setMaximumWidth(32767);
		updateGeometry();
		emit panelHasBeenExpanded(true);
	}
}

TQSize Sidebar_Widget::sizeHint() const
{
        if (m_somethingVisible)
           return TQSize(m_savedWidth,200);
        return minimumSizeHint();
}

void Sidebar_Widget::dockWidgetHasUndocked(KDockWidget* wid)
{
	kdDebug()<<" Sidebar_Widget::dockWidgetHasUndocked(KDockWidget*)"<<endl;
	for (unsigned int i=0;i<m_buttons.count();i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (button->dock==wid)
		{
			if (m_buttonBar->isTabRaised(i))
			{
				m_buttonBar->setTab(i,false);
				showHidePage(i);
			}
		}
	}
}

KInstance  *Sidebar_Widget::getInstance()
{
	return ((KonqSidebar*)m_partParent)->getInstance();
}

void Sidebar_Widget::submitFormRequest(const char *action,
					const TQString& url,
					const TQByteArray& formData,
					const TQString& /*target*/,
					const TQString& contentType,
					const TQString& /*boundary*/ )
{
KParts::URLArgs args;

	args.setContentType("Content-Type: " + contentType);
	args.postData = formData;
	args.setDoPost(TQCString(action).lower() == "post");
	// boundary?
	emit getExtension()->openURLRequest(KURL( url ), args);
}

void Sidebar_Widget::openURLRequest( const KURL &url, const KParts::URLArgs &args)
{
	getExtension()->openURLRequest(url,args);
}

void Sidebar_Widget::createNewWindow( const KURL &url, const KParts::URLArgs &args)
{
	getExtension()->createNewWindow(url,args);
}

void Sidebar_Widget::createNewWindow( const KURL &url, const KParts::URLArgs &args,
	const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part )
{
	getExtension()->createNewWindow(url,args,windowArgs,part);
}

void Sidebar_Widget::enableAction( const char * name, bool enabled )
{
 	if (TQT_TQOBJECT_CONST(sender())->parent()->isA("ButtonInfo"))
	{
		ButtonInfo *btninfo = static_cast<ButtonInfo*>(sender()->parent());
		if (btninfo)
		{
			TQString n(name);
			if (n == "copy")
				btninfo->copy = enabled;
			else if (n == "cut")
				btninfo->cut = enabled;
			else if (n == "paste")
				btninfo->paste = enabled;
			else if (n == "trash")
				btninfo->trash = enabled;
			else if (n == "del")
				btninfo->del = enabled;
			else if (n == "rename")
				btninfo->rename = enabled;
		}
	}
}


bool  Sidebar_Widget::doEnableActions()
{
 	if (!(TQT_TQOBJECT_CONST(sender())->parent()->isA("ButtonInfo")))
	{
		kdDebug()<<"Couldn't set active module, aborting"<<endl;
		return false;
	} else {
		m_activeModule=static_cast<ButtonInfo*>(sender()->parent());
		getExtension()->enableAction( "copy", m_activeModule->copy );
		getExtension()->enableAction( "cut", m_activeModule->cut );
		getExtension()->enableAction( "paste", m_activeModule->paste );
		getExtension()->enableAction( "trash", m_activeModule->trash );
		getExtension()->enableAction( "del", m_activeModule->del );
		getExtension()->enableAction( "rename", m_activeModule->rename );
		return true;
	}

}

void Sidebar_Widget::popupMenu( const TQPoint &global, const KFileItemList &items )
{
	if (doEnableActions())
		getExtension()->popupMenu(global,items);
}


void Sidebar_Widget::popupMenu( KXMLGUIClient *client, const TQPoint &global, const KFileItemList &items )
{
	if (doEnableActions())
		getExtension()->popupMenu(client,global,items);
}

void Sidebar_Widget::popupMenu( const TQPoint &global, const KURL &url,
	const TQString &mimeType, mode_t mode)
{
	if (doEnableActions())
		getExtension()->popupMenu(global,url,mimeType,mode);
}

void Sidebar_Widget::popupMenu( KXMLGUIClient *client,
	const TQPoint &global, const KURL &url,
	const TQString &mimeType, mode_t mode )
{
	if (doEnableActions())
		getExtension()->popupMenu(client,global,url,mimeType,mode);
}

void Sidebar_Widget::connectModule(TQObject *mod)
{
	if (mod->metaObject()->findSignal("started(KIO::Job*)") != -1) {
		connect(mod,TQT_SIGNAL(started(KIO::Job *)),this, TQT_SIGNAL(started(KIO::Job*)));
	}

	if (mod->metaObject()->findSignal("completed()") != -1) {
		connect(mod,TQT_SIGNAL(completed()),this,TQT_SIGNAL(completed()));
	}

	if (mod->metaObject()->findSignal("popupMenu(const " TQPOINT_OBJECT_NAME_STRING "&,const KURL&,const " TQSTRING_OBJECT_NAME_STRING "&,mode_t)") != -1) {
		connect(mod,TQT_SIGNAL(popupMenu( const TQPoint &, const KURL &,
			const TQString &, mode_t)),this,TQT_SLOT(popupMenu( const
			TQPoint &, const KURL&, const TQString &, mode_t)));
	}

	if (mod->metaObject()->findSignal("popupMenu(KXMLGUIClient*,const " TQPOINT_OBJECT_NAME_STRING " &,const KURL&,const " TQSTRING_OBJECT_NAME_STRING "&,mode_t)") != -1) {
		connect(mod,TQT_SIGNAL(popupMenu( KXMLGUIClient *, const TQPoint &,
			const KURL &,const TQString &, mode_t)),this,
			TQT_SLOT(popupMenu( KXMLGUIClient *, const TQPoint &,
			const KURL &,const TQString &, mode_t)));
	}

	if (mod->metaObject()->findSignal("popupMenu(const " TQPOINT_OBJECT_NAME_STRING "&,const KFileItemList&)") != -1) {
		connect(mod,TQT_SIGNAL(popupMenu( const TQPoint &, const KFileItemList & )),
			this,TQT_SLOT(popupMenu( const TQPoint &, const KFileItemList & )));
	}

	if (mod->metaObject()->findSignal("openURLRequest(const KURL&,const KParts::URLArgs&)") != -1) {
		connect(mod,TQT_SIGNAL(openURLRequest( const KURL &, const KParts::URLArgs &)),
			this,TQT_SLOT(openURLRequest( const KURL &, const KParts::URLArgs &)));
	}

	if (mod->metaObject()->findSignal("submitFormRequest(const char*,const " TQSTRING_OBJECT_NAME_STRING "&,const " TQBYTEARRAY_OBJECT_NAME_STRING "&,const " TQSTRING_OBJECT_NAME_STRING "&,const " TQSTRING_OBJECT_NAME_STRING "&,const " TQSTRING_OBJECT_NAME_STRING "&)") != -1) {
		connect(mod,
			TQT_SIGNAL(submitFormRequest(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&)),
			this,
			TQT_SLOT(submitFormRequest(const char*,const TQString&,const TQByteArray&,const TQString&,const TQString&,const TQString&)));
	}

	if (mod->metaObject()->findSignal("enableAction(const char*,bool)") != -1) {
		connect(mod,TQT_SIGNAL(enableAction( const char *, bool)),
			this,TQT_SLOT(enableAction(const char *, bool)));
	}

	if (mod->metaObject()->findSignal("createNewWindow(const KURL&,const KParts::URLArgs&)") != -1) {
		connect(mod,TQT_SIGNAL(createNewWindow( const KURL &, const KParts::URLArgs &)),
			this,TQT_SLOT(createNewWindow( const KURL &, const KParts::URLArgs &)));
	}
}



Sidebar_Widget::~Sidebar_Widget()
{
        m_config->writeEntry("OpenViews", m_visibleViews);
	if (m_configTimer.isActive())
		saveConfig();
	delete m_config;
	m_noUpdate = true;
	for (uint i=0;i<m_buttons.count();i++)
	{
		ButtonInfo *button = m_buttons.at(i);
		if (button->dock)
			button->dock->undock();
	}
}

void Sidebar_Widget::customEvent(TQCustomEvent* ev)
{
	if (KonqFileSelectionEvent::test(ev))
	{
		emit fileSelection(static_cast<KonqFileSelectionEvent*>(ev)->selection());
	} else if (KonqFileMouseOverEvent::test(ev)) {
		if (!(static_cast<KonqFileMouseOverEvent*>(ev)->item())) {
			emit fileMouseOver(KFileItem(KURL(),TQString::null,KFileItem::Unknown));
		} else {
			emit fileMouseOver(*static_cast<KonqFileMouseOverEvent*>(ev)->item());
		}
	} 
}

void Sidebar_Widget::resizeEvent(TQResizeEvent* ev)
{
	if (m_somethingVisible && m_userMovedSplitter)
	{
		int newWidth = width();
                TQSplitter *split = splitter();
		if (split && (m_savedWidth != newWidth))
		{
			TQValueList<int> sizes = split->sizes();
			if ((sizes.count() >= 2) && (sizes[1]))
			{
				m_savedWidth = newWidth;
				updateGeometry();
				m_configTimer.start(400, true);
			}
		}
	}
	m_userMovedSplitter = false;
	TQWidget::resizeEvent(ev);
}

TQSplitter *Sidebar_Widget::splitter() const
{
	if (m_universalMode) return 0;
	TQObject *p = parent();
	if (!p) return 0;
	p = p->parent();
	return static_cast<TQSplitter*>(TQT_TQWIDGET(p));
}

void Sidebar_Widget::userMovedSplitter()
{
	m_userMovedSplitter = true;
}
