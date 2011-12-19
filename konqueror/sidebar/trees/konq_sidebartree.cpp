/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                 2003 Waldo Bastian <bastian@kde.org>

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

#include "konq_sidebartreemodule.h"

#include <tqclipboard.h>
#include <tqcursor.h>
#include <tqdir.h>
#include <tqheader.h>
#include <tqpopupmenu.h>
#include <tqtimer.h>

#include <dcopclient.h>
#include <dcopref.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirnotify_stub.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <kmimetype.h>
#include <kprocess.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <kurldrag.h>

#include <stdlib.h>
#include <assert.h>


static const int autoOpenTimeout = 750;


getModule KonqSidebarTree::getPluginFactory(TQString name)
{
  if (!pluginFactories.contains(name))
  {
    KLibLoader *loader = KLibLoader::self();
    TQString libName    = pluginInfo[name];
    KLibrary *lib      = loader->library(TQFile::encodeName(libName));
    if (lib)
    {
      // get the create_ function
      TQString factory = "create_" + libName;
      void *create    = lib->symbol(TQFile::encodeName(factory));
      if (create)
      {
        getModule func = (getModule)create;
        pluginFactories.insert(name, func);
        kdDebug()<<"Added a module"<<endl;
      }
      else
      {
        kdWarning()<<"No create function found in"<<libName<<endl;
      }
    }
    else
      kdWarning() << "Module " << libName << " can't be loaded!" << endl;
  }

  return pluginFactories[name];
}

void KonqSidebarTree::loadModuleFactories()
{
  pluginFactories.clear();
  pluginInfo.clear();
  KStandardDirs *dirs=KGlobal::dirs();
  TQStringList list=dirs->findAllResources("data","konqsidebartng/dirtree/*.desktop",false,true);


  for (TQStringList::ConstIterator it=list.begin();it!=list.end();++it)
  {
    KSimpleConfig ksc(*it);
    ksc.setGroup("Desktop Entry");
    TQString name    = ksc.readEntry("X-KDE-TreeModule");
    TQString libName = ksc.readEntry("X-KDE-TreeModule-Lib");
    if ((name.isEmpty()) || (libName.isEmpty()))
        {kdWarning()<<"Bad Configuration file for a dirtree module "<<*it<<endl; continue;}

    //Register the library info.
    pluginInfo[name] = libName;
  }
}


class KonqSidebarTree_Internal
{
public:
    DropAcceptType m_dropMode;
    TQStringList m_dropFormats;
};


KonqSidebarTree::KonqSidebarTree( KonqSidebar_Tree *parent, TQWidget *parentWidget, int virt, const TQString& path )
    : KListView( parentWidget ),
      m_currentTopLevelItem( 0 ),
      m_toolTip( this ),
      m_scrollingLocked( false ),
      m_collection( 0 )
{
    d = new KonqSidebarTree_Internal;
    d->m_dropMode = SidebarTreeMode;

    loadModuleFactories();

    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );
    m_lstModules.setAutoDelete( true );

    setSelectionMode( TQListView::Single );
    setDragEnabled(true);

    m_part = parent;

    m_animationTimer = new TQTimer( this );
    connect( m_animationTimer, TQT_SIGNAL( timeout() ),
             this, TQT_SLOT( slotAnimation() ) );

    m_currentBeforeDropItem = 0;
    m_dropItem = 0;
    m_bOpeningFirstChild=false;

    addColumn( TQString::null );
    header()->hide();
    setTreeStepSize(15);

    m_autoOpenTimer = new TQTimer( this );
    connect( m_autoOpenTimer, TQT_SIGNAL( timeout() ),
             this, TQT_SLOT( slotAutoOpenFolder() ) );

    connect( this, TQT_SIGNAL( doubleClicked( TQListViewItem * ) ),
             this, TQT_SLOT( slotDoubleClicked( TQListViewItem * ) ) );
    connect( this, TQT_SIGNAL( mouseButtonPressed(int, TQListViewItem*, const TQPoint&, int)),
             this, TQT_SLOT( slotMouseButtonPressed(int, TQListViewItem*, const TQPoint&, int)) );
    connect( this, TQT_SIGNAL( mouseButtonClicked( int, TQListViewItem*, const TQPoint&, int ) ),
	     this, TQT_SLOT( slotMouseButtonClicked( int, TQListViewItem*, const TQPoint&, int ) ) );
    connect( this, TQT_SIGNAL( returnPressed( TQListViewItem * ) ),
             this, TQT_SLOT( slotDoubleClicked( TQListViewItem * ) ) );
    connect( this, TQT_SIGNAL( selectionChanged() ),
             this, TQT_SLOT( slotSelectionChanged() ) );

    connect( this, TQT_SIGNAL(itemRenamed(TQListViewItem*, const TQString &, int)),
             this, TQT_SLOT(slotItemRenamed(TQListViewItem*, const TQString &, int)));

/*    assert( m_part->getInterfaces()->getInstance()->dirs );
    TQString dirtreeDir = m_part->getInterfaces()->getInstance()->dirs()->saveLocation( "data", "konqueror/dirtree/" ); */

//    assert( KGlobal::dirs() );
//    TQString dirtreeDir = part->getInterfaces()->getInstance()->dirs()->saveLocation( "data", "konqueror/dirtree/" );

    if (virt==VIRT_Folder)
		{
		  m_dirtreeDir.dir.setPath(KGlobal::dirs()->saveLocation("data","konqsidebartng/virtual_folders/"+path+"/"));
		  m_dirtreeDir.relDir=path;
		}
	else
		m_dirtreeDir.dir.setPath( path );
    kdDebug(1201)<<m_dirtreeDir.dir.path()<<endl;
    m_dirtreeDir.type=virt;
    // Initial parsing
    rescanConfiguration();

    if (firstChild())
    {
      m_bOpeningFirstChild = true;
      firstChild()->setOpen(true);
      m_bOpeningFirstChild = false;
    }

    setFrameStyle( TQFrame::ToolBarPanel | TQFrame::Raised );
}

KonqSidebarTree::~KonqSidebarTree()
{
    clearTree();

    delete d;
}

void KonqSidebarTree::itemDestructed( KonqSidebarTreeItem *item )
{
    stopAnimation(item);

    if (item == m_currentBeforeDropItem)
    {
       m_currentBeforeDropItem = 0;
    }
}

void KonqSidebarTree::setDropFormats(const TQStringList &formats)
{
    d->m_dropFormats = formats;
}

void KonqSidebarTree::clearTree()
{
    m_lstModules.clear();
    m_topLevelItems.clear();
    m_mapCurrentOpeningFolders.clear();
    m_currentBeforeDropItem = 0;
    clear();

    if (m_dirtreeDir.type==VIRT_Folder)
    {
        setRootIsDecorated( true );
    }
    else
    {
        setRootIsDecorated( false );
    }
}

void KonqSidebarTree::followURL( const KURL &url )
{
    // Maybe we're there already ?
    KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
    if (selection && selection->externalURL().equals( url, true ))
    {
        ensureItemVisible( selection );
        return;
    }

    kdDebug(1201) << "KonqDirTree::followURL: " << url.url() << endl;
    TQPtrListIterator<KonqSidebarTreeTopLevelItem> topItem ( m_topLevelItems );
    for (; topItem.current(); ++topItem )
    {
        if ( topItem.current()->externalURL().isParentOf( url ) )
        {
            topItem.current()->module()->followURL( url );
            return; // done
        }
    }
    kdDebug(1201) << "KonqDirTree::followURL: Not found" << endl;
}

void KonqSidebarTree::contentsDragEnterEvent( TQDragEnterEvent *ev )
{
    m_dropItem = 0;
    m_currentBeforeDropItem = selectedItem();
    // Save the available formats
    m_lstDropFormats.clear();
    for( int i = 0; ev->format( i ); i++ )
      if ( *( ev->format( i ) ) )
         m_lstDropFormats.append( ev->format( i ) );
}

void KonqSidebarTree::contentsDragMoveEvent( TQDragMoveEvent *e )
{
    TQListViewItem *item = itemAt( contentsToViewport( e->pos() ) );

    // Accept drops on the background, if URLs
    if ( !item && m_lstDropFormats.contains("text/uri-list") )
    {
        m_dropItem = 0;
        e->acceptAction();
        if (selectedItem())
        setSelected( selectedItem(), false ); // no item selected
        return;
    }

    if (item && static_cast<KonqSidebarTreeItem*>(item)->acceptsDrops( m_lstDropFormats )) {
        d->m_dropMode = SidebarTreeMode;

        if ( !item->isSelectable() )
        {
            m_dropItem = 0;
            m_autoOpenTimer->stop();
            e->ignore();
            return;
        }

        e->acceptAction();

        setSelected( item, true );

        if ( item != m_dropItem )
        {
            m_autoOpenTimer->stop();
            m_dropItem = item;
            m_autoOpenTimer->start( autoOpenTimeout );
        }
    } else {
        d->m_dropMode = KListViewMode;
        KListView::contentsDragMoveEvent(e);
    }
}

void KonqSidebarTree::contentsDragLeaveEvent( TQDragLeaveEvent *ev )
{
    // Restore the current item to what it was before the dragging (#17070)
    if ( m_currentBeforeDropItem )
        setSelected( m_currentBeforeDropItem, true );
    else
        setSelected( m_dropItem, false ); // no item selected
    m_currentBeforeDropItem = 0;
    m_dropItem = 0;
    m_lstDropFormats.clear();

    if (d->m_dropMode == KListViewMode) {
        KListView::contentsDragLeaveEvent(ev);
    }
}

void KonqSidebarTree::contentsDropEvent( TQDropEvent *ev )
{
    if (d->m_dropMode == SidebarTreeMode) {
        m_autoOpenTimer->stop();

        if ( !selectedItem() )
        {
    //        KonqOperations::doDrop( 0L, m_dirtreeDir.dir, ev, this );
            KURL::List urls;
            if ( KURLDrag::decode( ev, urls ) )
            {
               for(KURL::List::ConstIterator it = urls.begin();
                   it != urls.end(); ++it)
               {
                  addURL(0, *it);
               }
            }
        }
        else
        {
            KonqSidebarTreeItem *selection = static_cast<KonqSidebarTreeItem *>( selectedItem() );
            selection->drop( ev );
        }
    } else {
        KListView::contentsDropEvent(ev);
    }
}

static TQString findUniqueFilename(const TQString &path, TQString filename)
{
    if (filename.endsWith(".desktop"))
       filename.truncate(filename.length()-8);

    TQString name = filename;
    int n = 2;
    while(TQFile::exists(path + filename + ".desktop"))
    {
       filename = TQString("%2_%1").arg(n++).arg(name);
    }
    return path+filename+".desktop";
}

void KonqSidebarTree::addURL(KonqSidebarTreeTopLevelItem* item, const KURL & url)
{
    TQString path;
    if (item)
       path = item->path();
    else
       path = m_dirtreeDir.dir.path();

    KURL destUrl;

    if (url.isLocalFile() && url.fileName().endsWith(".desktop"))
    {
       TQString filename = findUniqueFilename(path, url.fileName());
       destUrl.setPath(filename);
       KIO::NetAccess::copy(url, destUrl, this);
    }
    else
    {
       TQString name = url.host();
       if (name.isEmpty())
          name = url.fileName();
       TQString filename = findUniqueFilename(path, name);
       destUrl.setPath(filename);

       KDesktopFile cfg(filename);
       cfg.writeEntry("Encoding", "UTF-8");
       cfg.writeEntry("Type","Link");
       cfg.writeEntry("URL", url.url());
       TQString icon = "folder";
       if (!url.isLocalFile())
          icon = KMimeType::favIconForURL(url);
       if (icon.isEmpty())
          icon = KProtocolInfo::icon( url.protocol() );
       cfg.writeEntry("Icon", icon);
       cfg.writeEntry("Name", name);
       cfg.writeEntry("Open", false);
       cfg.sync();
    }

    KDirNotify_stub allDirNotify( "*", "KDirNotify*" );
    destUrl.setPath( destUrl.directory() );
    allDirNotify.FilesAdded( destUrl );

    if (item)
       item->setOpen(true);
}

bool KonqSidebarTree::acceptDrag(TQDropEvent* e) const
{
    // for KListViewMode...
    for( int i = 0; e->format( i ); i++ )
        if ( d->m_dropFormats.contains(e->format( i ) ) )
            return true;
    return false;
}

TQDragObject* KonqSidebarTree::dragObject()
{
    KonqSidebarTreeItem* item = static_cast<KonqSidebarTreeItem *>( selectedItem() );
    if ( !item )
        return 0;

    TQDragObject* drag = item->dragObject( viewport(), false );
    if ( !drag )
        return 0;

    const TQPixmap *pix = item->pixmap(0);
    if ( pix && drag->pixmap().isNull() )
        drag->setPixmap( *pix );

    return drag;
}

void KonqSidebarTree::leaveEvent( TQEvent *e )
{
    KListView::leaveEvent( e );
//    emitStatusBarText( TQString::null );
}


void KonqSidebarTree::slotDoubleClicked( TQListViewItem *item )
{
    //kdDebug(1201) << "KonqSidebarTree::slotDoubleClicked " << item << endl;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    slotExecuted( item );
    item->setOpen( !item->isOpen() );
}

void KonqSidebarTree::slotExecuted( TQListViewItem *item )
{
    kdDebug(1201) << "KonqSidebarTree::slotExecuted " << item << endl;
    if ( !item )
        return;

    if ( !static_cast<KonqSidebarTreeItem*>(item)->isClickable() )
        return;

    KonqSidebarTreeItem *dItem = static_cast<KonqSidebarTreeItem *>( item );

    KParts::URLArgs args;

    args.serviceType = dItem->externalMimeType();
    args.trustedSource = true;
    KURL externalURL = dItem->externalURL();
    if ( !externalURL.isEmpty() )
	openURLRequest( externalURL, args );
}

void KonqSidebarTree::slotMouseButtonPressed( int _button, TQListViewItem* _item, const TQPoint&, int col )
{
    KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem*>( _item );
    if (_button == Qt::RightButton)
    {
        if ( item && col < 2)
        {
            item->setSelected( true );
            item->rightButtonPressed();
        }
    }
}

void KonqSidebarTree::slotMouseButtonClicked(int _button, TQListViewItem* _item, const TQPoint&, int col)
{
    KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem*>(_item);
    if(_item && col < 2)
    {
        switch( _button ) {
        case Qt::LeftButton:
            slotExecuted( item );
            break;
        case Qt::MidButton:
            item->middleButtonClicked();
            break;
        }
    }
}

void KonqSidebarTree::slotAutoOpenFolder()
{
    m_autoOpenTimer->stop();

    if ( !m_dropItem || m_dropItem->isOpen() )
        return;

    m_dropItem->setOpen( true );
    m_dropItem->tqrepaint();
}

void KonqSidebarTree::rescanConfiguration()
{
    kdDebug(1201) << "KonqSidebarTree::rescanConfiguration()" << endl;
    m_autoOpenTimer->stop();
    clearTree();
    if (m_dirtreeDir.type==VIRT_Folder)
	{
	         kdDebug(1201)<<"KonqSidebarTree::rescanConfiguration()-->scanDir"<<endl;
		 scanDir( 0, m_dirtreeDir.dir.path(), true);

	}
	else
		{
			    kdDebug(1201)<<"KonqSidebarTree::rescanConfiguration()-->loadTopLevel"<<endl;
		            loadTopLevelItem( 0, m_dirtreeDir.dir.path() );
		}
}

void KonqSidebarTree::slotSelectionChanged()
{
    if ( !m_dropItem ) // don't do this while the dragmove thing
    {
        KonqSidebarTreeItem * item = static_cast<KonqSidebarTreeItem *>( selectedItem() );
        if ( item )
            item->itemSelected();
        /* else   -- doesn't seem to happen
           {} */
    }
}

void KonqSidebarTree::FilesAdded( const KURL & dir )
{
    kdDebug(1201) << "KonqSidebarTree::FilesAdded " << dir.url() << endl;
    if ( m_dirtreeDir.dir.isParentOf( dir ) )
        // We use a timer in case of DCOP re-entrance..
        TQTimer::singleShot( 0, this, TQT_SLOT( rescanConfiguration() ) );
}

void KonqSidebarTree::FilesRemoved( const KURL::List & urls )
{
    //kdDebug(1201) << "KonqSidebarTree::FilesRemoved " << urls.count() << endl;
    for ( KURL::List::ConstIterator it = urls.begin() ; it != urls.end() ; ++it )
    {
        //kdDebug(1201) <<  "KonqSidebarTree::FilesRemoved " << (*it).prettyURL() << endl;
        if ( m_dirtreeDir.dir.isParentOf( *it ) )
        {
            TQTimer::singleShot( 0, this, TQT_SLOT( rescanConfiguration() ) );
            kdDebug(1201) << "KonqSidebarTree::FilesRemoved done" << endl;
            return;
        }
    }
}

void KonqSidebarTree::FilesChanged( const KURL::List & urls )
{
    //kdDebug(1201) << "KonqSidebarTree::FilesChanged" << endl;
    // not same signal, but same implementation
    FilesRemoved( urls );
}

void KonqSidebarTree::scanDir( KonqSidebarTreeItem *parent, const TQString &path, bool isRoot )
{
    TQDir dir( path );

    if ( !dir.isReadable() )
        return;

    kdDebug(1201) << "scanDir " << path << endl;

    TQStringList entries = dir.entryList( TQDir::Files );
    TQStringList dirEntries = dir.entryList( TQDir::Dirs | TQDir::NoSymLinks );
    dirEntries.remove( "." );
    dirEntries.remove( ".." );

    if ( isRoot )
    {
        bool copyConfig = ( entries.count() == 0 && dirEntries.count() == 0 );
        if (!copyConfig)
        {
            // Check version number
            // Version 1 was the dirtree of KDE 2.0.x (no versioning at that time, so default)
            // Version 2 includes the history
            // Version 3 includes the bookmarks
            // Version 4 includes lan.desktop and floppy.desktop, Alex
            // Version 5 includes the audiocd browser
            // Version 6 includes the printmanager and lan browser
            const int currentVersion = 6;
            TQString key = TQString::fromLatin1("X-KDE-DirTreeVersionNumber");
            KSimpleConfig versionCfg( path + "/.directory" );
            int versionNumber = versionCfg.readNumEntry( key, 1 );
            kdDebug(1201) << "KonqSidebarTree::scanDir found version " << versionNumber << endl;
            if ( versionNumber < currentVersion )
            {
                versionCfg.writeEntry( key, currentVersion );
                versionCfg.sync();
                copyConfig = true;
            }
        }
        if (copyConfig)
        {
            // We will copy over the configuration for the dirtree, from the global directory
            TQStringList dirtree_dirs = KGlobal::dirs()->findDirs("data","konqsidebartng/virtual_folders/"+m_dirtreeDir.relDir+"/");


//            TQString dirtree_dir = KGlobal::dirs()->findDirs("data","konqsidebartng/virtual_folders/"+m_dirtreeDir.relDir+"/").last();  // most global
//            kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir=" << dirtree_dir << endl;

            /*
            // debug code

            TQStringList blah = m_part->getInterfaces->getInstance()->dirs()->dirs()->findDirs( "data", "konqueror/dirtree" );
            TQStringList::ConstIterator eIt = blah.begin();
            TQStringList::ConstIterator eEnd = blah.end();
            for (; eIt != eEnd; ++eIt )
                kdDebug(1201) << "KonqSidebarTree::scanDir findDirs got me " << *eIt << endl;
            // end debug code
            */

	    for (TQStringList::const_iterator ddit=dirtree_dirs.begin();ddit!=dirtree_dirs.end();++ddit) {
		TQString dirtree_dir=*ddit;
		if (dirtree_dir==path) continue;
	        //    if ( !dirtree_dir.isEmpty() && dirtree_dir != path )
	            {
        	        TQDir globalDir( dirtree_dir );
                	Q_ASSERT( globalDir.isReadable() );
	                // Only copy the entries that don't exist yet in the local dir
        	        TQStringList globalDirEntries = globalDir.entryList();
                	TQStringList::ConstIterator eIt = globalDirEntries.begin();
	                TQStringList::ConstIterator eEnd = globalDirEntries.end();
        	        for (; eIt != eEnd; ++eIt )
                	{
	                    //kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir contains " << *eIt << endl;
        	            if ( *eIt != "." && *eIt != ".."
                	         && !entries.contains( *eIt ) && !dirEntries.contains( *eIt ) )
	                    { // we don't have that one yet -> copy it.
                	        TQString cp("cp -R -- ");
        	                cp += KProcess::quote(dirtree_dir + *eIt);
	                        cp += " ";
        	                cp += KProcess::quote(path);
                	        kdDebug(1201) << "KonqSidebarTree::scanDir executing " << cp << endl;
                        	::system( TQFile::encodeName(cp) );
	                    }
        	        }
		     }
                  }
	                // hack to make TQDir refresh the lists
	                dir.setPath(path);
        	        entries = dir.entryList( TQDir::Files );
	                dirEntries = dir.entryList( TQDir::Dirs );
        	        dirEntries.remove( "." );
	                dirEntries.remove( ".." );
             }
	}
    TQStringList::ConstIterator eIt = entries.begin();
    TQStringList::ConstIterator eEnd = entries.end();

    for (; eIt != eEnd; ++eIt )
    {
        TQString filePath = TQString( *eIt ).prepend( path );
        KURL u;
        u.setPath( filePath );
        TQString foundMimeName = KMimeType::findByURL( u, 0, true )->name();
        if ( (foundMimeName == "application/x-desktop")
          || (foundMimeName == "media/builtin-mydocuments")
          || (foundMimeName == "media/builtin-mycomputer")
          || (foundMimeName == "media/builtin-mynetworkplaces")
          || (foundMimeName == "media/builtin-printers")
          || (foundMimeName == "media/builtin-trash")
          || (foundMimeName == "media/builtin-webbrowser") )
            loadTopLevelItem( parent, filePath );
    }

    eIt = dirEntries.begin();
    eEnd = dirEntries.end();

    for (; eIt != eEnd; eIt++ )
    {
        TQString newPath = TQString( path ).append( *eIt ).append( '/' );

        if ( newPath == KGlobalSettings::autostartPath() )
            continue;

        loadTopLevelGroup( parent, newPath );
    }
}

void KonqSidebarTree::loadTopLevelGroup( KonqSidebarTreeItem *parent, const TQString &path )
{
    TQDir dir( path );
    TQString name = dir.dirName();
    TQString icon = "folder";
    bool    open = false;

    kdDebug(1201) << "Scanning " << path << endl;

    TQString dotDirectoryFile = TQString( path ).append( "/.directory" );

    if ( TQFile::exists( dotDirectoryFile ) )
    {
        kdDebug(1201) << "Reading the .directory" << endl;
        KSimpleConfig cfg( dotDirectoryFile, true );
        cfg.setDesktopGroup();
        name = cfg.readEntry( "Name", name );
        icon = cfg.readEntry( "Icon", icon );
        //stripIcon( icon );
        open = cfg.readBoolEntry( "Open", open );
    }

    KonqSidebarTreeTopLevelItem *item;
    if ( parent )
    {
        kdDebug(1201) << "KonqSidebarTree::loadTopLevelGroup Inserting new group under parent " << endl;
        item = new KonqSidebarTreeTopLevelItem( parent, 0 /* no module */, path );
    }
    else
        item = new KonqSidebarTreeTopLevelItem( this, 0 /* no module */, path );
    item->setText( 0, name );
    item->setPixmap( 0, SmallIcon( icon ) );
    item->setListable( false );
    item->setClickable( false );
    item->setTopLevelGroup( true );
    item->setOpen( open );

    m_topLevelItems.append( item );

    kdDebug(1201) << "Inserting group " << name << "   " << path << endl;

    scanDir( item, path );

    if ( item->childCount() == 0 )
        item->setExpandable( false );
}

void KonqSidebarTree::loadTopLevelItem( KonqSidebarTreeItem *parent,  const TQString &filename )
{
    KDesktopFile cfg( filename, true );
    cfg.setDollarExpansion(true);

    TQFileInfo inf( filename );

    TQString path = filename;
    TQString name = KIO::decodeFileName( inf.fileName() );
    if ( name.length() > 8 && name.right( 8 ) == ".desktop" )
        name.truncate( name.length() - 8 );
    if ( name.length() > 7 && name.right( 7 ) == ".kdelnk" )
        name.truncate( name.length() - 7 );

    name = cfg.readEntry( "Name", name );
    KonqSidebarTreeModule * module = 0L;

    // Here's where we need to create the right module...
    // ### TODO: make this KTrader/KLibrary based.
    TQString moduleName = cfg.readEntry( "X-KDE-TreeModule" );
    TQString showHidden=cfg.readEntry("X-KDE-TreeModule-ShowHidden");

    if (moduleName.isEmpty()) moduleName="Directory";
    kdDebug(1201) << "##### Loading module: " << moduleName << " file: " << filename << endl;

    getModule func;
    func = getPluginFactory(moduleName);
    if (func!=0)
	{
		kdDebug(1201)<<"showHidden: "<<showHidden<<endl;
		module=func(this,showHidden.upper()=="TRUE");
	}

    if (module==0) {kdDebug()<<"No Module loaded"<<endl; return;}

    KonqSidebarTreeTopLevelItem *item;
    if ( parent )
        item = new KonqSidebarTreeTopLevelItem( parent, module, path );
    else
        item = new KonqSidebarTreeTopLevelItem( this, module, path );

    item->setText( 0, name );
    item->setPixmap( 0, SmallIcon( cfg.readIcon() ));

    module->addTopLevelItem( item );

    m_topLevelItems.append( item );
    m_lstModules.append( module );

    bool open = cfg.readBoolEntry( "Open", false );
    if ( open && item->isExpandable() )
        item->setOpen( true );
}

void KonqSidebarTree::slotAnimation()
{
    MapCurrentOpeningFolders::Iterator it = m_mapCurrentOpeningFolders.begin();
    MapCurrentOpeningFolders::Iterator end = m_mapCurrentOpeningFolders.end();
    for (; it != end; ++it )
    {
        uint & iconNumber = it.data().iconNumber;
        TQString icon = TQString::fromLatin1( it.data().iconBaseName ).append( TQString::number( iconNumber ) );
        it.key()->setPixmap( 0, SmallIcon( icon));

        iconNumber++;
        if ( iconNumber > it.data().iconCount )
            iconNumber = 1;
    }
}


void KonqSidebarTree::startAnimation( KonqSidebarTreeItem * item, const char * iconBaseName, uint iconCount, const TQPixmap * originalPixmap )
{
    const TQPixmap *pix = originalPixmap ? originalPixmap : item->pixmap(0);
    if (pix)
    {
        m_mapCurrentOpeningFolders.insert( item, AnimationInfo( iconBaseName, iconCount, *pix ) );
        if ( !m_animationTimer->isActive() )
            m_animationTimer->start( 50 );
    }
}

void KonqSidebarTree::stopAnimation( KonqSidebarTreeItem * item )
{
    MapCurrentOpeningFolders::Iterator it = m_mapCurrentOpeningFolders.find(item);
    if ( it != m_mapCurrentOpeningFolders.end() )
    {
        item->setPixmap( 0, it.data().originalPixmap );
        m_mapCurrentOpeningFolders.remove( item );

        if (m_mapCurrentOpeningFolders.isEmpty())
            m_animationTimer->stop();
    }
}

KonqSidebarTreeItem * KonqSidebarTree::currentItem() const
{
    return static_cast<KonqSidebarTreeItem *>( selectedItem() );
}

void KonqSidebarTree::setContentsPos( int x, int y )
{
    if ( !m_scrollingLocked )
	KListView::setContentsPos( x, y );
}

void KonqSidebarTree::slotItemRenamed(TQListViewItem* item, const TQString &name, int col)
{
    Q_ASSERT(col==0);
    if (col != 0) return;
    assert(item);
    KonqSidebarTreeItem * treeItem = static_cast<KonqSidebarTreeItem *>(item);
    treeItem->rename( name );
}


void KonqSidebarTree::enableActions( bool copy, bool cut, bool paste,
                        bool trash, bool del, bool rename)
{
    enableAction( "copy", copy );
    enableAction( "cut", cut );
    enableAction( "paste", paste );
    enableAction( "trash", trash );
    enableAction( "del", del );
    enableAction( "rename", rename );
}

bool KonqSidebarTree::tabSupport()
{
    // see if the newTab() dcop function is available (i.e. the sidebar is embedded into konqueror)
   DCOPRef ref(kapp->dcopClient()->appId(), topLevelWidget()->name());
    DCOPReply reply = ref.call("functions()");
    if (reply.isValid()) {
        QCStringList funcs;
        reply.get(funcs, "QCStringList");
        for (QCStringList::ConstIterator it = funcs.begin(); it != funcs.end(); ++it) {
            if ((*it) == "void newTab(TQString url)") {
                return true;
                break;
            }
        }
    }
    return false;
}

void KonqSidebarTree::showToplevelContextMenu()
{
    KonqSidebarTreeTopLevelItem *item = 0;
    KonqSidebarTreeItem *treeItem = currentItem();
    if (treeItem && treeItem->isTopLevelItem())
        item = static_cast<KonqSidebarTreeTopLevelItem *>(treeItem);

    if (!m_collection)
    {
        m_collection = new KActionCollection( this, "bookmark actions" );
        (void) new KAction( i18n("&Create New Folder..."), "folder_new", 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotCreateFolder() ), m_collection, "create_folder");
        (void) new KAction( i18n("Delete Folder"), "editdelete", 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotDelete() ), m_collection, "delete_folder");
        (void) new KAction( i18n("Rename"), 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotRename() ), m_collection, "rename");
        (void) new KAction( i18n("Delete Link"), "editdelete", 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotDelete() ), m_collection, "delete_link");
        (void) new KAction( i18n("Properties"), "edit", 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotProperties() ), m_collection, "item_properties");
        (void) new KAction( i18n("Open in New Window"), "window_new", 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotOpenNewWindow() ), m_collection, "open_window");
        (void) new KAction( i18n("Open in New Tab"), "tab_new", 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotOpenTab() ), m_collection, "open_tab");
        (void) new KAction( i18n("Copy Link Address"), "editcopy", 0, TQT_TQOBJECT(this),
                            TQT_SLOT( slotCopyLocation() ), m_collection, "copy_location");
    }

    TQPopupMenu *menu = new TQPopupMenu;

    if (item) {
        if (item->isTopLevelGroup()) {
            m_collection->action("rename")->plug(menu);
            m_collection->action("delete_folder")->plug(menu);
            menu->insertSeparator();
            m_collection->action("create_folder")->plug(menu);
        } else {
            if (tabSupport())
                m_collection->action("open_tab")->plug(menu);
            m_collection->action("open_window")->plug(menu);
            m_collection->action("copy_location")->plug(menu);
            menu->insertSeparator();
            m_collection->action("rename")->plug(menu);
            m_collection->action("delete_link")->plug(menu);
        }
        menu->insertSeparator();
        m_collection->action("item_properties")->plug(menu);
    } else {
        m_collection->action("create_folder")->plug(menu);
    }

    m_currentTopLevelItem = item;

    menu->exec( TQCursor::pos() );
    delete menu;

    m_currentTopLevelItem = 0;
}

void KonqSidebarTree::slotCreateFolder()
{
    TQString path;
    TQString name = i18n("New Folder");

    while(true)
    {
        name = KInputDialog::getText(i18n("Create New Folder"),
    			i18n("Enter folder name:"), name);
        if (name.isEmpty())
            return;

        if (m_currentTopLevelItem)
            path = m_currentTopLevelItem->path();
        else
            path = m_dirtreeDir.dir.path();

        if (!path.endsWith("/"))
            path += "/";

        path = path + name;

        if (!TQFile::exists(path))
            break;

        name = name + "-2";
   }

   KGlobal::dirs()->makeDir(path);

   loadTopLevelGroup(m_currentTopLevelItem, path);
}

void KonqSidebarTree::slotDelete()
{
    if (!m_currentTopLevelItem) return;
    m_currentTopLevelItem->del();
}

void KonqSidebarTree::slotRename()
{
    if (!m_currentTopLevelItem) return;
    m_currentTopLevelItem->rename();
}

void KonqSidebarTree::slotProperties()
{
    if (!m_currentTopLevelItem) return;

    KURL url;
    url.setPath(m_currentTopLevelItem->path());

    KPropertiesDialog *dlg = new KPropertiesDialog( url );
    dlg->setFileNameReadOnly(true);
    dlg->exec();
    delete dlg;
}

void KonqSidebarTree::slotOpenNewWindow()
{
    if (!m_currentTopLevelItem) return;
    emit createNewWindow( m_currentTopLevelItem->externalURL() );
}

void KonqSidebarTree::slotOpenTab()
{
    if (!m_currentTopLevelItem) return;
    DCOPRef ref(kapp->dcopClient()->appId(), topLevelWidget()->name());
    ref.call( "newTab(TQString)", m_currentTopLevelItem->externalURL().url() );
}

void KonqSidebarTree::slotCopyLocation()
{
    if (!m_currentTopLevelItem) return;
    KURL url = m_currentTopLevelItem->externalURL();
    kapp->tqclipboard()->setData( new KURLDrag(url, 0), TQClipboard::Selection );
    kapp->tqclipboard()->setData( new KURLDrag(url, 0), TQClipboard::Clipboard );
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


void KonqSidebarTreeToolTip::maybeTip( const TQPoint &point )
{
    TQListViewItem *item = m_view->itemAt( point );
    if ( item ) {
	TQString text = static_cast<KonqSidebarTreeItem*>( item )->toolTipText();
	if ( !text.isEmpty() )
	    tip ( m_view->itemRect( item ), text );
    }
}




#include "konq_sidebartree.moc"
