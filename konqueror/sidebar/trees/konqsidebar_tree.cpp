#include "konqsidebar_tree.h"
#include "konqsidebar_tree.moc"
#include "konq_sidebartree.h"
#include <kdebug.h>
#include <tdestandarddirs.h>
#include <tdesimpleconfig.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <tdelistviewsearchline.h>

#include <tqclipboard.h>
#include <tqdragobject.h>
#include <tqtoolbutton.h>
#include <tqvbox.h>
#include <tqapplication.h>

KonqSidebar_Tree::KonqSidebar_Tree(TDEInstance *instance,TQObject *parent,TQWidget *widgetParent, TQString &desktopName_, const char* name):
                   KonqSidebarPlugin(instance,parent,widgetParent,desktopName_,name)
	{
		TDESimpleConfig ksc(desktopName_);
		ksc.setGroup("Desktop Entry");
		int virt= ( (ksc.readEntry("X-TDE-TreeModule","")=="Virtual") ?VIRT_Folder:VIRT_Link);
		if (virt==1) desktopName_=ksc.readEntry("X-TDE-RelURL","");

		widget = new TQVBox(widgetParent);

		if (ksc.readBoolEntry("X-TDE-SearchableTreeModule",false)) {
			TQHBox* searchline = new TQHBox(widget);
			searchline->setSpacing(KDialog::spacingHint());
			tree=new KonqSidebarTree(this,widget,virt,desktopName_);
			TQToolButton *clearSearch = new TQToolButton(searchline);
			clearSearch->setTextLabel(i18n("Clear Search"), true);
			clearSearch->setIconSet(SmallIconSet(TQApplication::reverseLayout() ? "clear_left" : "locationbar_erase"));
			TQLabel* slbl = new TQLabel(i18n("Se&arch:"), searchline);
			TDEListViewSearchLine* listViewSearch = new TDEListViewSearchLine(searchline,tree);
			slbl->setBuddy(listViewSearch);
			connect(clearSearch, TQ_SIGNAL(pressed()), listViewSearch, TQ_SLOT(clear()));
		}
		else
			tree=new KonqSidebarTree(this,widget,virt,desktopName_);

    		connect(tree, TQ_SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs &)),
			this,TQ_SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs &)));

		connect(tree,TQ_SIGNAL(createNewWindow( const KURL &, const KParts::URLArgs &)),
			this,TQ_SIGNAL(createNewWindow( const KURL &, const KParts::URLArgs &)));

		connect(tree,TQ_SIGNAL(popupMenu( const TQPoint &, const KURL &, const TQString &, mode_t )),
			this,TQ_SIGNAL(popupMenu( const TQPoint &, const KURL &, const TQString &, mode_t )));

		connect(tree,TQ_SIGNAL(popupMenu( const TQPoint &, const KFileItemList & )),
			this,TQ_SIGNAL(popupMenu( const TQPoint &, const KFileItemList & )));

		connect(tree,TQ_SIGNAL(enableAction( const char *, bool )),
			this,TQ_SIGNAL(enableAction( const char *, bool)));

        }


KonqSidebar_Tree::~KonqSidebar_Tree(){;}

void* KonqSidebar_Tree::provides(const TQString &) {return 0;}

//void KonqSidebar_Tree::emitStatusBarText (const TQString &) {;}

TQWidget *KonqSidebar_Tree::getWidget(){return widget;}

void KonqSidebar_Tree::handleURL(const KURL &url)
    {
	emit started( 0 );
        tree->followURL( url );
        emit completed();
    }

void KonqSidebar_Tree::cut()
{
    TQDragObject * drag = static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->dragObject( 0L, true );
    if (drag)
        TQApplication::clipboard()->setData( drag );
}

void KonqSidebar_Tree::copy()
{
    TQDragObject * drag = static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->dragObject( 0L );
    if (drag)
        TQApplication::clipboard()->setData( drag );
}

void KonqSidebar_Tree::paste()
{
    if (tree->currentItem())
        tree->currentItem()->paste();
}

void KonqSidebar_Tree::trash()
{
    if (tree->currentItem())
        tree->currentItem()->trash();
}

void KonqSidebar_Tree::del()
{
    if (tree->currentItem())
        tree->currentItem()->del();
}

void KonqSidebar_Tree::shred()
{
    if (tree->currentItem())
        tree->currentItem()->shred();
}

void KonqSidebar_Tree::rename()
{
    Q_ASSERT( tree->currentItem() );
    if (tree->currentItem())
        tree->currentItem()->rename();
}






extern "C"
{
    TDE_EXPORT void*  create_konqsidebar_tree(TDEInstance *inst,TQObject *par,TQWidget *widp,TQString &desktopname,const char *name)
    {
        return new KonqSidebar_Tree(inst,par,widp,desktopname,name);
    }
}

extern "C"
{
   TDE_EXPORT bool add_konqsidebar_tree(TQString* fn, TQString*, TQMap<TQString,TQString> *map)
   {
	  TDEStandardDirs *dirs=TDEGlobal::dirs();
	  TQStringList list=dirs->findAllResources("data","konqsidebartng/dirtree/*.desktop",false,true);
	  TQStringList names;
	  for (TQStringList::ConstIterator it=list.begin();it!=list.end();++it)
	  {
		TDESimpleConfig sc(*it);
		sc.setGroup("Desktop Entry");
		names<<sc.readEntry("Name");
	  }

	TQString item = KInputDialog::getItem( i18n( "Select Type" ),
		i18n( "Select type:" ), names );
	if (!item.isEmpty())
		{
			int id=names.findIndex( item );
			if (id==-1) return false;
			TDESimpleConfig ksc2(*list.at(id));
			ksc2.setGroup("Desktop Entry");
		        map->insert("Type","Link");
			map->insert("Icon",ksc2.readEntry("Icon"));
			map->insert("Name",ksc2.readEntry("Name"));
		 	map->insert("Open","false");
			map->insert("URL",ksc2.readEntry("X-TDE-Default-URL"));
			map->insert("X-TDE-KonqSidebarModule","konqsidebar_tree");
			map->insert("X-TDE-TreeModule",ksc2.readEntry("X-TDE-TreeModule"));
			map->insert("X-TDE-TreeModule-ShowHidden",ksc2.readEntry("X-TDE-TreeModule-ShowHidden"));
			fn->setLatin1("dirtree%1.desktop");
			return true;
		}
	return false;
   }
}
