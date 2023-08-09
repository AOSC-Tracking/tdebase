/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <tqdir.h>
#include <tqeventloop.h>

#include <tdelocale.h>
#include <tdeapplication.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <krun.h>
#include <kprotocolinfo.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <tdeglobalsettings.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <kxmlguibuilder.h>
#include <tdeparts/componentfactory.h>

#include <assert.h>

#include <tdefileshare.h>
#include <kprocess.h>

#include "kpropertiesdialog.h"
#include "knewmenu.h"
#include "konq_popupmenu.h"
#include "konq_operations.h"
#include "konq_xmlguiclient.h"
#include <dcopclient.h>

/*
 Test cases:
  iconview file: background
  iconview file: file (with and without servicemenus)
  iconview file: directory
  iconview remote protocol (e.g. ftp: or fish:)
  iconview trash:/
  sidebar directory tree
  sidebar Devices / Hard Disc
  tdehtml background
  tdehtml link
  tdehtml image (www.kde.org RMB on K logo)
  tdehtmlimage (same as above, then choose View image, then RMB)
  selected text in tdehtml
  embedded katepart
  kdesktop folder
  trash link on desktop
  trashed file or directory
  application .desktop file
 Then the same after uninstalling tdeaddons/konq-plugins (kuick and arkplugin in particular)
*/

class KonqPopupMenuGUIBuilder : public KXMLGUIBuilder
{
public:
  KonqPopupMenuGUIBuilder( TQPopupMenu *menu )
  : KXMLGUIBuilder( 0 )
  {
    m_menu = menu;
  }
  virtual ~KonqPopupMenuGUIBuilder()
  {
  }

  virtual TQWidget *createContainer( TQWidget *parent, int index,
          const TQDomElement &element,
          int &id )
  {
    if ( !parent && element.attribute( "name" ) == "popupmenu" )
      return m_menu;

    return KXMLGUIBuilder::createContainer( parent, index, element, id );
  }

  TQPopupMenu *m_menu;
};

class KonqPopupMenu::KonqPopupMenuPrivate
{
public:
  KonqPopupMenuPrivate() : m_parentWidget( 0 ),
                           m_itemFlags( KParts::BrowserExtension::DefaultPopupItems )
  {
  }
  TQString m_urlTitle;
  TQWidget *m_parentWidget;
  KParts::BrowserExtension::PopupFlags m_itemFlags;

  bool localURLSlotFired;
  KURL localURLResultURL;
  bool localURLResultIsLocal;
};

KonqPopupMenu::ProtocolInfo::ProtocolInfo()
{
  m_Reading = false;
  m_Writing = false;
  m_Deleting = false;
  m_Moving = false;
  m_TrashIncluded = false;
}

bool KonqPopupMenu::ProtocolInfo::supportsReading() const
{
  return m_Reading;
}

bool KonqPopupMenu::ProtocolInfo::supportsWriting() const
{
  return m_Writing;
}

bool KonqPopupMenu::ProtocolInfo::supportsDeleting() const
{
  return m_Deleting;
}

bool KonqPopupMenu::ProtocolInfo::supportsMoving() const
{
  return m_Moving;
}

bool KonqPopupMenu::ProtocolInfo::trashIncluded() const
{
  return m_TrashIncluded;
}

// This helper class stores the .desktop-file actions and the servicemenus
// in order to support X-TDE-Priority and X-TDE-Submenu.
class PopupServices
{
public:
    ServiceList* selectList( const TQString& priority, const TQString& submenuName );

    ServiceList builtin;
    ServiceList user, userToplevel, userPriority;
    TQMap<TQString, ServiceList> userSubmenus, userToplevelSubmenus, userPrioritySubmenus;
};

ServiceList* PopupServices::selectList( const TQString& priority, const TQString& submenuName )
{
    // we use the categories .desktop entry to define submenus
    // if none is defined, we just pop it in the main menu
    if (submenuName.isEmpty())
    {
        if (priority == "TopLevel")
        {
            return &userToplevel;
        }
        else if (priority == "Important")
        {
            return &userPriority;
        }
    }
    else if (priority == "TopLevel")
    {
        return &(userToplevelSubmenus[submenuName]);
    }
    else if (priority == "Important")
    {
        return &(userPrioritySubmenus[submenuName]);
    }
    else
    {
        return &(userSubmenus[submenuName]);
    }
    return &user;
}

//////////////////

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              TDEActionCollection & actions,
                              KNewMenu * newMenu,
                              bool showProperties )
    : TQPopupMenu( 0L, "konq_popupmenu" ),
      m_actions( actions ), m_ownActions( static_cast<TQWidget *>( 0 ), "KonqPopupMenu::m_ownActions" ),
		  m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
    KonqPopupFlags kpf = ( showProperties ? ShowProperties : IsLink ) | ShowNewWindow;
    init(0, kpf, KParts::BrowserExtension::DefaultPopupItems);
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              KURL viewURL,
                              TDEActionCollection & actions,
                              KNewMenu * newMenu,
                              TQWidget * parentWidget,
                              bool showProperties )
    : TQPopupMenu( parentWidget, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<TQWidget *>( 0 ), "KonqPopupMenu::m_ownActions" ), m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
    KonqPopupFlags kpf = ( showProperties ? ShowProperties : IsLink ) | ShowNewWindow;
    init(parentWidget, kpf, KParts::BrowserExtension::DefaultPopupItems);
}

KonqPopupMenu::KonqPopupMenu( KBookmarkManager *mgr, const KFileItemList &items,
                              const KURL& viewURL,
                              TDEActionCollection & actions,
                              KNewMenu * newMenu,
                              TQWidget * parentWidget,
                              KonqPopupFlags kpf,
                              KParts::BrowserExtension::PopupFlags flags)
  : TQPopupMenu( parentWidget, "konq_popupmenu" ), m_actions( actions ), m_ownActions( static_cast<TQWidget *>( 0 ), "KonqPopupMenu::m_ownActions" ), m_pMenuNew( newMenu ), m_sViewURL(viewURL), m_lstItems(items), m_pManager(mgr)
{
    init(parentWidget, kpf, flags);
}

void KonqPopupMenu::init (TQWidget * parentWidget, KonqPopupFlags kpf, KParts::BrowserExtension::PopupFlags flags)
{
    d = new KonqPopupMenuPrivate;
    d->m_parentWidget = parentWidget;
    d->m_itemFlags = flags;
    setup(kpf);
}

int KonqPopupMenu::insertServicesSubmenus(const TQMap<TQString, ServiceList>& submenus,
                                          TQDomElement& menu,
                                          bool isBuiltin)
{
    int count = 0;
    TQMap<TQString, ServiceList>::ConstIterator it;

    for (it = submenus.begin(); it != submenus.end(); ++it)
    {
        if (it.data().isEmpty())
        {
            //avoid empty sub-menus
            continue;
        }

        TQDomElement actionSubmenu = m_doc.createElement( "menu" );
        actionSubmenu.setAttribute( "name", "actions " + it.key() );
        menu.appendChild( actionSubmenu );
        TQDomElement subtext = m_doc.createElement( "text" );
        actionSubmenu.appendChild( subtext );
        subtext.appendChild( m_doc.createTextNode( it.key() ) );
        count += insertServices(it.data(), actionSubmenu, isBuiltin);
    }

    return count;
}

int KonqPopupMenu::insertServices(const ServiceList& list,
                                  TQDomElement& menu,
                                  bool isBuiltin)
{
    static int id = 1000;
    int count = 0;

    ServiceList::const_iterator it = list.begin();
    for( ; it != list.end(); ++it )
    {
        if ((*it).isEmpty())
        {
            if (!menu.firstChild().isNull() &&
                menu.lastChild().toElement().tagName().lower() != "separator")
            {
                TQDomElement separator = m_doc.createElement( "separator" );
                menu.appendChild(separator);
            }
            continue;
        }

        if (isBuiltin || (*it).m_display == true)
        {
            TQCString name;
            name.setNum( id );
            name.prepend( isBuiltin ? "builtinservice_" : "userservice_" );
            TDEAction * act = new TDEAction( TQString((*it).m_strName).replace('&',"&&"), 0,
                                         TQT_TQOBJECT(this), TQT_SLOT( slotRunService() ),
                                         &m_ownActions, name );

            if ( !(*it).m_strIcon.isEmpty() )
            {
                TQPixmap pix = SmallIcon( (*it).m_strIcon );
                act->setIconSet( pix );
            }

            addAction( act, menu ); // Add to toplevel menu

            m_mapPopupServices[ id++ ] = *it;
            ++count;
        }
    }

    return count;
}

bool KonqPopupMenu::KIOSKAuthorizedAction(TDEConfig& cfg)
{
    if ( !cfg.hasKey( "X-TDE-AuthorizeAction") )
    {
        return true;
    }

    TQStringList list = cfg.readListEntry("X-TDE-AuthorizeAction");
    if (kapp && !list.isEmpty())
    {
        for(TQStringList::ConstIterator it = list.begin();
            it != list.end();
            ++it)
        {
            if (!kapp->authorize((*it).stripWhiteSpace()))
            {
                return false;
            }
        }
    }

    return true;
}


void KonqPopupMenu::setup(KonqPopupFlags kpf)
{
    assert( m_lstItems.count() >= 1 );

    m_ownActions.setWidget( this );

    const bool bIsLink   = (kpf & IsLink);
    bool currentDir      = false;
    bool sReading        = true;
    bool sDeleting       = ( d->m_itemFlags & KParts::BrowserExtension::NoDeletion ) == 0;
    bool sMoving         = sDeleting;
    bool sWriting        = sDeleting && m_lstItems.first()->isWritable();
    m_sMimeType          = m_lstItems.first()->mimetype();
    TQString mimeGroup   = m_sMimeType.left(m_sMimeType.find('/'));
    mode_t mode          = m_lstItems.first()->mode();
    bool isDirectory     = S_ISDIR(mode);
    bool isMediaFile     = false;
    bool isEncryptedMediaFile = false;
    bool isReallyLocal   = m_lstItems.first()->isLocalFile();
    bool isLocal         = isReallyLocal || m_lstItems.first()->url().protocol()=="media" ||
                           m_lstItems.first()->url().protocol()=="system";
    bool isTrashIncluded = false;
    bool isTrashLink     = false;
    m_lstPopupURLs.clear();
    int id = 0;
    setFont(TDEGlobalSettings::menuFont());
    m_pluginList.setAutoDelete( true );
    m_ownActions.setHighlightingEnabled( true );

    attrName = TQString::fromLatin1( "name" );

    prepareXMLGUIStuff();
    m_builder = new KonqPopupMenuGUIBuilder( this );
    m_factory = new KXMLGUIFactory( m_builder );

    KURL url;
    KFileItemListIterator it ( m_lstItems );
    TQStringList mimeTypeList;
    // Check whether all URLs are correct
    for ( ; it.current(); ++it )
    {
        url = (*it)->url();

        // Build the list of URLs
        m_lstPopupURLs.append( url );

        // Determine if common mode among all URLs
        if ( mode != (*it)->mode() )
            mode = 0; // modes are different => reset to 0

        // Determine if common mimetype among all URLs
        if ( m_sMimeType != (*it)->mimetype() )
        {
            m_sMimeType = TQString::null; // mimetypes are different => null

            if ( mimeGroup != (*it)->mimetype().left((*it)->mimetype().find('/')))
                mimeGroup = TQString::null; // mimetype groups are different as well!
        }

        if ( mimeTypeList.findIndex( (*it)->mimetype() ) == -1 )
            mimeTypeList << (*it)->mimetype();

        if ( isReallyLocal && !url.isLocalFile() )
            isReallyLocal = false;
        if ( isLocal && !url.isLocalFile() && url.protocol() != "media" && url.protocol() != "system" )
            isLocal = false;

        if ( !isTrashIncluded && (
             ( url.protocol() == "trash" && url.path().length() <= 1 )
             || url.url() == "system:/trash" || url.url() == "system:/trash/" ) ) {
            isTrashIncluded = true;
            isLocal = false;
        }

        if ( sReading )
            sReading = KProtocolInfo::supportsReading( url );

        if ( sWriting )
            sWriting = KProtocolInfo::supportsWriting( url ) && (*it)->isWritable();

        if ( sDeleting )
            sDeleting = KProtocolInfo::supportsDeleting( url );

        if ( sMoving )
            sMoving = KProtocolInfo::supportsMoving( url );
        if ( (*it)->mimetype().startsWith("media/") ) {
            isMediaFile = true;
            if ( (*it)->mimetype().contains("encrypted")) {
                isEncryptedMediaFile = true;
            }
        }
    }

    // If a local path is available, monitor that instead of the given remote URL...
    KURL realURL = m_sViewURL;
    if (!realURL.isLocalFile()) {
        d->localURLSlotFired = false;
        TDEIO::LocalURLJob* localURLJob = TDEIO::localURL(m_sViewURL);
        if (localURLJob) {
            connect(localURLJob, TQT_SIGNAL(localURL(TDEIO::LocalURLJob*, const KURL&, bool)), this, TQT_SLOT(slotLocalURL(TDEIO::LocalURLJob*, const KURL&, bool)));
            connect(localURLJob, TQT_SIGNAL(destroyed()), this, TQT_SLOT(slotLocalURLKIODestroyed()));
            while (!d->localURLSlotFired) {
                kapp->eventLoop()->enterLoop();
            }
            if (d->localURLResultIsLocal) {
                realURL = d->localURLResultURL;
            }
        }
    }

    url = realURL;
    url.cleanPath();

    //check if url is current directory
    if ( m_lstItems.count() == 1 )
    {
        KURL firstPopupURL( m_lstItems.first()->url() );
        firstPopupURL.cleanPath();
        //kdDebug(1203) << "View path is " << url.url() << endl;
        //kdDebug(1203) << "First popup path is " << firstPopupURL.url() << endl;
        currentDir = firstPopupURL.equals( url, true /* ignore_trailing */, true /* ignore_internalReferenceURLS */ );
        if ( isLocal && ((m_sMimeType == "application/x-desktop")
                      || (m_sMimeType == "media/builtin-mydocuments")
                      || (m_sMimeType == "media/builtin-mycomputer")
                      || (m_sMimeType == "media/builtin-mynetworkplaces")
                      || (m_sMimeType == "media/builtin-printers")
                      || (m_sMimeType == "media/builtin-trash")
                      || (m_sMimeType == "media/builtin-webbrowser")) ) {
            KSimpleConfig cfg( firstPopupURL.path(), true );
            cfg.setDesktopGroup();
            isTrashLink = ( cfg.readEntry("Type") == "Link" && cfg.readEntry("URL") == "trash:/" );
        }

        if ( isTrashLink ) {
            sDeleting = false;
        }
    }

    m_info.m_Reading = sReading;
    m_info.m_Writing = sWriting;
    m_info.m_Deleting = sDeleting;
    m_info.m_Moving = sMoving;
    m_info.m_TrashIncluded = isTrashIncluded;

    // isCurrentTrash: popup on trash:/ itself, or on the trash.desktop link
    bool isCurrentTrash = ( m_lstItems.count() == 1 && isTrashIncluded ) || isTrashLink;
    bool isIntoTrash = ( url.protocol() == "trash" || url.url().startsWith( "system:/trash" ) ) && !isCurrentTrash; // trashed file, not trash:/ itself
    //kdDebug() << "isLocal=" << isLocal << " url=" << url << " isCurrentTrash=" << isCurrentTrash << " isIntoTrash=" << isIntoTrash << " isTrashIncluded=" << isTrashIncluded << endl;
    clear();

    //////////////////////////////////////////////////////////////////////////

    TDEAction * act;

    if (!isCurrentTrash)
        addMerge( "konqueror" );

    bool isKDesktop = TQCString( kapp->name() ) == "kdesktop";

    if (( kpf & ShowProperties ) && isKDesktop &&
        !kapp->authorize("editable_desktop_icons"))
    {
        kpf &= ~ShowProperties; // remove flag
    }

    // Either 'newview' is in the actions we're given (probably in the tabhandling group)
    // or we need to insert it ourselves (e.g. for kdesktop). In the first case, actNewWindow must remain 0.
    if ( ((kpf & ShowNewWindow) != 0) && sReading  && !isEncryptedMediaFile)
    {
        TQString openStr = isKDesktop ? i18n( "&Open" ) : i18n( "Open in New &Window" );
        TDEAction *actNewWindow = new TDEAction( openStr, "window-new", 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupNewView() ), &m_ownActions, "newview" );
        actNewWindow->setToolTip( i18n( "Open item in a new window" ) );
        addAction( actNewWindow );
        addSeparator();
    }

    if ( S_ISDIR(mode) && sWriting && !isCurrentTrash ) // A dir, and we can create things into it
    {
        if ( currentDir && m_pMenuNew ) // Current dir -> add the "new" menu
        {
            // As requested by KNewMenu :
            m_pMenuNew->slotCheckUpToDate();
            m_pMenuNew->setPopupFiles( m_lstPopupURLs );

            addAction( m_pMenuNew );

            addSeparator();
        }
        else
        {
            if (d->m_itemFlags & KParts::BrowserExtension::ShowCreateDirectory)
            {
                TDEAction *actNewDir = new TDEAction( i18n( "Create &Folder..." ), "folder-new", 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupNewDir() ), &m_ownActions, "newdir" );
                addAction( actNewDir );
                addSeparator();
            }
        }
    } else if ( isIntoTrash ) {
        // Trashed item, offer restoring
        act = new TDEAction( i18n( "&Restore" ), 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupRestoreTrashedItems() ), &m_ownActions, "restore" );
        addAction( act );
    }

    if (d->m_itemFlags & KParts::BrowserExtension::ShowNavigationItems)
    {
        if (d->m_itemFlags & KParts::BrowserExtension::ShowUp)
            addAction( "up" );
        addAction( "back" );
        addAction( "forward" );
        if (d->m_itemFlags & KParts::BrowserExtension::ShowReload)
            addAction( "reload" );
        addSeparator();
    }

    if (!isEncryptedMediaFile)
    {
        addGroup( "tabhandling" ); // includes a separator
    }

    if ( !bIsLink )
    {
        if ( !currentDir && sReading ) {
            if ( sDeleting ) {
                addAction( "cut" );
            }
            addAction( "copy" );
        }

        if ( S_ISDIR(mode) && sWriting ) {
            if ( currentDir )
                addAction( "paste" );
            else
                addAction( "pasteto" );
        }
        if ( !currentDir )
        {
            if ( m_lstItems.count() == 1 && sMoving )
                addAction( "rename" );

            bool addTrash = false;
            bool addDel = false;

            if ( sMoving && !isIntoTrash && !isTrashLink )
                addTrash = true;

            if ( sDeleting ) {
                if ( !isLocal )
                    addDel = true;
                else if (TDEApplication::keyboardMouseState() & TQt::ShiftButton) {
                    addTrash = false;
                    addDel = true;
                }
                else {
                    TDEConfigGroup configGroup( kapp->config(), "KDE" );
                    if ( configGroup.readBoolEntry( "ShowDeleteCommand", false ) )
                        addDel = true;
                }
            }

            if ( addTrash )
                addAction( "trash" );
            if ( addDel )
                addAction( "del" );
        }
    }
    if ( isCurrentTrash )
    {
        act = new TDEAction( i18n( "&Empty Trash Bin" ), "emptytrash", 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupEmptyTrashBin() ), &m_ownActions, "empytrash" );
        KSimpleConfig trashConfig( "trashrc", true );
        trashConfig.setGroup( "Status" );
        act->setEnabled( !trashConfig.readBoolEntry( "Empty", true ) );
        addAction( act );
    }
    addGroup( "editactions" );

    if (d->m_itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) {
      addMerge( 0 );
      m_factory->addClient( this );
      return;
    }

    if ( !isCurrentTrash && !isIntoTrash && (d->m_itemFlags & KParts::BrowserExtension::ShowBookmark))
    {
        addSeparator();
        TQString caption;
        if (currentDir)
        {
           bool httpPage = (m_sViewURL.protocol().find("http", 0, false) == 0);
           if (httpPage)
              caption = i18n("&Bookmark This Page");
           else
              caption = i18n("&Bookmark This Location");
        }
        else if (S_ISDIR(mode))
           caption = i18n("&Bookmark This Folder");
        else if (bIsLink)
           caption = i18n("&Bookmark This Link");
        else
           caption = i18n("&Bookmark This File");

        act = new TDEAction( caption, "bookmark_add", 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupAddToBookmark() ), &m_ownActions, "bookmark_add" );
        if (m_lstItems.count() > 1)
            act->setEnabled(false);
        if (kapp->authorizeTDEAction("bookmarks"))
            addAction( act );
        if (bIsLink)
            addGroup( "linkactions" );
    }

    //////////////////////////////////////////////////////

    const bool isSingleLocal = m_lstItems.count() == 1 && isLocal;
    PopupServices s;
    KURL urlForServiceMenu( m_lstItems.first()->url() );
    if (isLocal && !isReallyLocal) { // media or system
        bool dummy;
        urlForServiceMenu = m_lstItems.first()->mostLocalURL(dummy);
    }

    // 1 - Look for builtin and user-defined services
    if ( ((m_sMimeType == "application/x-desktop")
       || (m_sMimeType == "media/builtin-mydocuments")
       || (m_sMimeType == "media/builtin-mycomputer")
       || (m_sMimeType == "media/builtin-mynetworkplaces")
       || (m_sMimeType == "media/builtin-printers")
       || (m_sMimeType == "media/builtin-trash")
       || (m_sMimeType == "media/builtin-webbrowser")) && isSingleLocal ) // .desktop file
    {
        // get builtin services, like mount/unmount
        s.builtin = KDEDesktopMimeType::builtinServices( urlForServiceMenu );
        const TQString path = urlForServiceMenu.path();
        KSimpleConfig cfg( path, true );
        cfg.setDesktopGroup();
        const TQString priority = cfg.readEntry("X-TDE-Priority");
        const TQString submenuName = cfg.readEntry( "X-TDE-Submenu" );
        if ( cfg.readEntry("Type") == "Link" ) {
           urlForServiceMenu = cfg.readEntry("URL");
           // TODO: Do we want to make all the actions apply on the target
           // of the .desktop file instead of the .desktop file itself?
        }
        ServiceList* list = s.selectList( priority, submenuName );
        (*list) = KDEDesktopMimeType::userDefinedServices( path, cfg, urlForServiceMenu.isLocalFile() );
    }

    if ( sReading )
    {

        // 2 - Look for "servicesmenus" bindings (konqueror-specific user-defined services)

        // first check the .directory if this is a directory
        if (isDirectory && isSingleLocal)
        {
            TQString dotDirectoryFile = urlForServiceMenu.path(1).append(".directory");
            KSimpleConfig cfg( dotDirectoryFile, true );
            cfg.setDesktopGroup();

            if (KIOSKAuthorizedAction(cfg))
            {
                const TQString priority = cfg.readEntry("X-TDE-Priority");
                const TQString submenuName = cfg.readEntry( "X-TDE-Submenu" );
                ServiceList* list = s.selectList( priority, submenuName );
                (*list) += KDEDesktopMimeType::userDefinedServices( dotDirectoryFile, cfg, true );
            }
        }

        // findAllResources() also removes duplicates
        TQStringList entries = TDEGlobal::dirs()->findAllResources("data",
                "konqueror/servicemenus/*.desktop", false, true);
        entries.sort();  // sort to ensure consistent order in popup menu
        TQStringList::ConstIterator eIt = entries.begin();
        const TQStringList::ConstIterator eEnd = entries.end();
        for (; eIt != eEnd; ++eIt )
        {
            KSimpleConfig cfg( *eIt, true );
            cfg.setDesktopGroup();

            if (!KIOSKAuthorizedAction(cfg))
            {
                continue;
            }

            if ( cfg.hasKey( "X-TDE-ShowIfRunning" ) )
            {
                const TQString app = cfg.readEntry( "X-TDE-ShowIfRunning" );
                if ( !kapp->dcopClient()->isApplicationRegistered( app.utf8() ) )
                    continue;
            }
            if ( cfg.hasKey( "X-TDE-ShowIfDcopCall" ) )
            {
                TQString dcopcall = cfg.readEntry( "X-TDE-ShowIfDcopCall" );
                const TQCString app = TQString(dcopcall.section(' ', 0,0)).utf8();

                //if( !kapp->dcopClient()->isApplicationRegistered( app ))
                //	continue; //app does not exist so cannot send call

                TQByteArray dataToSend;
                TQDataStream dataStream(dataToSend, IO_WriteOnly);
                dataStream << m_lstPopupURLs;

                TQCString replyType;
                TQByteArray replyData;
                TQCString object = TQString(dcopcall.section(' ', 1,-2)).utf8();
                TQString function =  TQString(dcopcall.section(' ', -1));
                if(!function.endsWith("(KURL::List)")) {
                    kdWarning() << "Desktop file " << *eIt << " contains an invalid X-TDE-ShowIfDcopCall - the function must take the exact parameter (KURL::List) and must be specified." << endl;
                    continue; //Be safe.
                }

                if(!kapp->dcopClient()->call( app, object,
                                              function.utf8(),
                                              dataToSend, replyType, replyData, true, 1000))
                    continue;
                if(replyType != "bool" || !replyData[0])
                    continue;

            }
            if ( cfg.hasKey( "X-TDE-Protocol" ) )
            {
                const TQString protocol = cfg.readEntry( "X-TDE-Protocol" );
                if ( protocol != urlForServiceMenu.protocol() )
                    continue;
            }
            else if ( cfg.hasKey( "X-TDE-Protocols" ) )
            {
                TQStringList protocols = TQStringList::split( "," , cfg.readEntry( "X-TDE-Protocols" ) );
                if ( !protocols.contains( urlForServiceMenu.protocol() ) )
                    continue;
            }
            else if ( urlForServiceMenu.protocol() == "trash" || urlForServiceMenu.url().startsWith( "system:/trash" ) )
            {
                // Require servicemenus for the trash to ask for protocol=trash explicitely.
                // Trashed files aren't supposed to be available for actions.
                // One might want a servicemenu for trash.desktop itself though.
                continue;
            }

            if ( cfg.hasKey( "X-TDE-Require" ) )
            {
                const TQStringList capabilities = cfg.readListEntry( "X-TDE-Require" );
                if ( capabilities.contains( "Write" ) && !sWriting )
                    continue;
            }
            if ( (cfg.hasKey( "Actions" ) || cfg.hasKey( "X-TDE-GetActionMenu") ) && cfg.hasKey( "X-TDE-ServiceTypes" ) )
            {
                const TQStringList types = cfg.readListEntry( "X-TDE-ServiceTypes" );
                const TQStringList excludeTypes = cfg.readListEntry( "X-TDE-ExcludeServiceTypes" );
                bool ok = false;

                // check for exact matches or a typeglob'd mimetype if we have a mimetype
                for (TQStringList::ConstIterator it = types.begin();
                     it != types.end() && !ok;
                     ++it)
                {
                    // first check if we have an all mimetype
                    bool checkTheMimetypes = false;
                    if (*it == "all/all" ||
                        *it == "allfiles" /*compat with KDE up to 3.0.3*/)
                    {
                        checkTheMimetypes = true;
                    }

                    // next, do we match all files?
                    if (!ok &&
                        !isDirectory &&
                        *it == "all/allfiles")
                    {
                        checkTheMimetypes = true;
                    }

                    // if we have a mimetype, see if we have an exact or a type globbed match
                    if ((!ok &&
                        (!m_sMimeType.isEmpty() &&
                         *it == m_sMimeType)) ||
                        (!mimeGroup.isEmpty() &&
                         (((*it).right(1) == "*") &&
                          (*it).left((*it).find('/')) == mimeGroup)))
                    {
                        checkTheMimetypes = true;
                    }

                    if (checkTheMimetypes)
                    {
                        ok = true;
                        for (TQStringList::ConstIterator itex = excludeTypes.begin(); itex != excludeTypes.end(); ++itex)
                        {
                            if( ((*itex).right(1) == "*" && (*itex).left((*itex).find('/')) == mimeGroup) ||
                                ((*itex) == m_sMimeType) )
                            {
                                ok = false;
                                break;
                            }
                        }
                    }
                }

                if ( ok )
                {
                    const TQString priority = cfg.readEntry("X-TDE-Priority");
                    const TQString submenuName = cfg.readEntry( "X-TDE-Submenu" );

                    ServiceList *list = s.selectList( priority, submenuName );
                    (*list) += KDEDesktopMimeType::userDefinedServices( *eIt, cfg, url.isLocalFile(), m_lstPopupURLs );
                }
            }
        }

        TDETrader::OfferList offers;

        if (kapp->authorizeTDEAction("openwith"))
        {
            TQString constraint = "Type == 'Application' and DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'";
            TQString subConstraint = " and '%1' in ServiceTypes";

            TQStringList::ConstIterator it = mimeTypeList.begin();
            TQStringList::ConstIterator end = mimeTypeList.end();
            Q_ASSERT( it != end );
            TQString first = *it;
            ++it;
            while ( it != end ) {
                constraint += subConstraint.arg( *it );
                ++it;
            }

            offers = TDETrader::self()->query( first, constraint );
        }

        //// Ok, we have everything, now insert

        m_mapPopup.clear();
        m_mapPopupServices.clear();
        // "Open With..." for folders is really not very useful, especially for remote folders.
        // (media:/something, or trash:/, or ftp://...)
        if ( !isDirectory || isLocal )
        {
            if ( hasAction() )
                addSeparator();

            if ( !offers.isEmpty() )
            {
                // First block, app and preview offers
                id = 1;

                TQDomElement menu = m_menuElement;

                if ( offers.count() > 1 ) // submenu 'open with'
                {
                    menu = m_doc.createElement( "menu" );
                    menu.setAttribute( "name", "openwith submenu" );
                    m_menuElement.appendChild( menu );
                    TQDomElement text = m_doc.createElement( "text" );
                    menu.appendChild( text );
                    text.appendChild( m_doc.createTextNode( i18n("&Open With") ) );
                }

                TDETrader::OfferList::ConstIterator it = offers.begin();
                for( ; it != offers.end(); it++ )
                {
                    KService::Ptr service = (*it);

                    // Skip OnlyShowIn=Foo and NotShowIn=TDE entries,
                    // but still offer NoDisplay=true entries, that's the
                    // whole point of such desktop files. This is why we don't
                    // use service->noDisplay() here.
                    const TQString onlyShowIn = service->property("OnlyShowIn", TQVariant::String).toString();
                    if ( !onlyShowIn.isEmpty() ) {
                        const TQStringList aList = TQStringList::split(';', onlyShowIn);
                        if (!aList.contains("TDE"))
                            continue;
                    }
                    const TQString notShowIn = service->property("NotShowIn", TQVariant::String).toString();
                    if ( !notShowIn.isEmpty() ) {
                        const TQStringList aList = TQStringList::split(';', notShowIn);
                        if (aList.contains("TDE"))
                            continue;
                    }

                    TQCString nam;
                    nam.setNum( id );

                    TQString actionName( (*it)->name().replace("&", "&&") );
                    if ( menu == m_menuElement ) // no submenu -> prefix single offer
                        actionName = i18n( "Open with %1" ).arg( actionName );

                    act = new TDEAction( actionName, (*it)->pixmap( TDEIcon::Small ), 0,
                                       TQT_TQOBJECT(this), TQT_SLOT( slotRunService() ),
                                       &m_ownActions, nam.prepend( "appservice_" ) );
                    addAction( act, menu );

                    m_mapPopup[ id++ ] = *it;
                }

                TQString openWithActionName;
                if ( menu != m_menuElement ) // submenu
                {
                    addSeparator( menu );
                    openWithActionName = i18n( "&Other..." );
                }
                else
                {
                    openWithActionName = i18n( "&Open With..." );
                }
                TDEAction *openWithAct = new TDEAction( openWithActionName, 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
                addAction( openWithAct, menu );
            }
            else // no app offers -> Open With...
            {
                act = new TDEAction( i18n( "&Open With..." ), 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupOpenWith() ), &m_ownActions, "openwith" );
                addAction( act );
            }

        }
        addGroup( "preview" );
    }

    // Second block, builtin + user
    TQDomElement actionMenu = m_menuElement;
    int userItemCount = 0;
    if (s.user.count() + s.userSubmenus.count() +
        s.userPriority.count() + s.userPrioritySubmenus.count() > 1)
    {
        // we have more than one item, so let's make a submenu
        actionMenu = m_doc.createElement( "menu" );
        actionMenu.setAttribute( "name", "actions submenu" );
        m_menuElement.appendChild( actionMenu );
        TQDomElement text = m_doc.createElement( "text" );
        actionMenu.appendChild( text );
        text.appendChild( m_doc.createTextNode( i18n("Ac&tions") ) );
    }

    userItemCount += insertServicesSubmenus(s.userPrioritySubmenus, actionMenu, false);
    userItemCount += insertServices(s.userPriority, actionMenu, false);

    // see if we need to put a separator between our priority items and our regular items
    if (userItemCount > 0 &&
        (s.user.count() > 0 ||
         s.userSubmenus.count() > 0 ||
         s.builtin.count() > 0) &&
         actionMenu.lastChild().toElement().tagName().lower() != "separator")
    {
        TQDomElement separator = m_doc.createElement( "separator" );
        actionMenu.appendChild(separator);
    }

    userItemCount += insertServicesSubmenus(s.userSubmenus, actionMenu, false);
    userItemCount += insertServices(s.user, actionMenu, false);
    userItemCount += insertServices(s.builtin, m_menuElement, true);

    userItemCount += insertServicesSubmenus(s.userToplevelSubmenus, m_menuElement, false);
    userItemCount += insertServices(s.userToplevel, m_menuElement, false);

    if ( userItemCount > 0 )
    {
        addPendingSeparator();
    }

    if ( !isCurrentTrash && !isIntoTrash && !isMediaFile && sReading )
        addPlugins(); // now it's time to add plugins

    if ( KPropertiesDialog::canDisplay( m_lstItems ) && (kpf & ShowProperties) )
    {
        act = new TDEAction( i18n( "&Properties" ), "edit", 0, TQT_TQOBJECT(this), TQT_SLOT( slotPopupProperties() ),
                           &m_ownActions, "properties" );
        addAction( act );
    }

    while ( !m_menuElement.lastChild().isNull() &&
            m_menuElement.lastChild().toElement().tagName().lower() == "separator" )
        m_menuElement.removeChild( m_menuElement.lastChild() );

    if ( isDirectory && isLocal )
    {
        if ( KFileShare::authorization() == KFileShare::Authorized )
        {
            addSeparator();
            act = new TDEAction( i18n("Share"), 0, TQT_TQOBJECT(this), TQT_SLOT( slotOpenShareFileDialog() ),
                               &m_ownActions, "sharefile" );
            addAction( act );
        }
    }

    addMerge( 0 );
    //kdDebug() << k_funcinfo << domDocument().toString() << endl;

    m_factory->addClient( this );
}

void KonqPopupMenu::slotOpenShareFileDialog()
{
    KPropertiesDialog* dlg = showPropertiesDialog();
    dlg->showFileSharingPage();
}

KonqPopupMenu::~KonqPopupMenu()
{
  m_pluginList.clear();
  delete m_factory;
  delete m_builder;
  delete d;
  //kdDebug(1203) << "~KonqPopupMenu leave" << endl;
}

void KonqPopupMenu::setURLTitle( const TQString& urlTitle )
{
    d->m_urlTitle = urlTitle;
}

void KonqPopupMenu::slotPopupNewView()
{
  KURL::List::ConstIterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); it++ )
    (void) new KRun(*it);
}

void KonqPopupMenu::slotPopupNewDir()
{
  if (m_lstPopupURLs.empty())
    return;

  KonqOperations::newDir(d->m_parentWidget, m_lstPopupURLs.first());
}

void KonqPopupMenu::slotPopupEmptyTrashBin()
{
  KonqOperations::emptyTrash();
}

void KonqPopupMenu::slotPopupRestoreTrashedItems()
{
  KonqOperations::restoreTrashedItems( m_lstPopupURLs );
}

void KonqPopupMenu::slotPopupOpenWith()
{
  KRun::displayOpenWithDialog( m_lstPopupURLs );
}

void KonqPopupMenu::slotPopupAddToBookmark()
{
  KBookmarkGroup root;
  if ( m_lstPopupURLs.count() == 1 ) {
    KURL url = m_lstPopupURLs.first();
    TQString title = d->m_urlTitle.isEmpty() ? url.prettyURL() : d->m_urlTitle;
    root = m_pManager->addBookmarkDialog( url.prettyURL(), title );
  }
  else
  {
    root = m_pManager->root();
    KURL::List::ConstIterator it = m_lstPopupURLs.begin();
    for ( ; it != m_lstPopupURLs.end(); it++ )
      root.addBookmark( m_pManager, (*it).prettyURL(), (*it) );
  }
  m_pManager->emitChanged( root );
}

void KonqPopupMenu::slotRunService()
{
  TQCString senderName = sender()->name();
  int id = senderName.mid( senderName.find( '_' ) + 1 ).toInt();

  // Is it a usual service (application)
  TQMap<int,KService::Ptr>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( **it, m_lstPopupURLs );
    return;
  }

  // Is it a service specific to desktop entry files ?
  TQMap<int,KDEDesktopMimeType::Service>::Iterator it2 = m_mapPopupServices.find( id );
  if ( it2 != m_mapPopupServices.end() )
  {
      KDEDesktopMimeType::executeService( m_lstPopupURLs, it2.data() );
  }

  return;
}

void KonqPopupMenu::slotPopupMimeType()
{
    KonqOperations::editMimeType( m_sMimeType );
}

void KonqPopupMenu::slotPopupProperties()
{
    (void)showPropertiesDialog();
}

KPropertiesDialog* KonqPopupMenu::showPropertiesDialog()
{
    // It may be that the tdefileitem was created by hand
    // (see KonqKfmIconView::slotMouseButtonPressed)
    // In that case, we can get more precise info in the properties
    // (like permissions) if we stat the URL.
    if ( m_lstItems.count() == 1 )
    {
        KFileItem * item = m_lstItems.first();
        if (item->entry().count() == 0) // this item wasn't listed by a slave
        {
            // KPropertiesDialog will use stat to get more info on the file
            return new KPropertiesDialog( item->url(), d->m_parentWidget );
        }
    }
    return new KPropertiesDialog( m_lstItems, d->m_parentWidget );
}

TDEAction *KonqPopupMenu::action( const TQDomElement &element ) const
{
  TQCString name = element.attribute( attrName ).ascii();
  TDEAction *res = m_ownActions.action( static_cast<const char *>(name) );

  if ( !res )
    res = m_actions.action( static_cast<const char *>(name) );

  if ( !res && m_pMenuNew && strcmp( name, m_pMenuNew->name() ) == 0 )
    return m_pMenuNew;

  return res;
}

TDEActionCollection *KonqPopupMenu::actionCollection() const
{
    return const_cast<TDEActionCollection *>( &m_ownActions );
}

TQString KonqPopupMenu::mimeType() const
{
    return m_sMimeType;
}

KonqPopupMenu::ProtocolInfo KonqPopupMenu::protocolInfo() const
{
    return m_info;
}

void KonqPopupMenu::addPlugins()
{
    // search for Konq_PopupMenuPlugins inspired by simons kpropsdlg
    //search for a plugin with the right protocol
    TDETrader::OfferList plugin_offers;
    unsigned int pluginCount = 0;
    plugin_offers = TDETrader::self()->query( m_sMimeType.isNull() ? TQString::fromLatin1( "all/all" ) : m_sMimeType, "'KonqPopupMenu/Plugin' in ServiceTypes");
    if ( plugin_offers.isEmpty() )
        return; // no plugins installed do not bother about it

    TDETrader::OfferList::ConstIterator iterator = plugin_offers.begin();
    TDETrader::OfferList::ConstIterator end = plugin_offers.end();

    addGroup( "plugins" );
    // travers the offerlist
    for(; iterator != end; ++iterator, ++pluginCount ) {
        //kdDebug() << (*iterator)->library() << endl;
        KonqPopupMenuPlugin *plugin =
            KParts::ComponentFactory::
            createInstanceFromLibrary<KonqPopupMenuPlugin>( TQFile::encodeName( (*iterator)->library() ),
                                                            TQT_TQOBJECT(this),
                                                            (*iterator)->name().latin1() );
        if ( !plugin )
            continue;
        // This make the kuick plugin insert its stuff above "Properties"
        TQString pluginClientName = TQString::fromLatin1( "Plugin%1" ).arg( pluginCount );
        addMerge( pluginClientName );
        plugin->domDocument().documentElement().setAttribute( "name", pluginClientName );
        m_pluginList.append( plugin );
        insertChildClient( plugin );
    }

    // ## Where is this used?
    addMerge( "plugins" );
}

KURL KonqPopupMenu::url() const // ### should be viewURL()
{
  return m_sViewURL;
}

KFileItemList KonqPopupMenu::fileItemList() const
{
  return m_lstItems;
}

KURL::List KonqPopupMenu::popupURLList() const
{
  return m_lstPopupURLs;
}

void KonqPopupMenu::slotLocalURL(TDEIO::LocalURLJob *job, const KURL& url, bool isLocal)
{
  d->localURLSlotFired = true;
  d->localURLResultURL = url;
  d->localURLResultIsLocal = isLocal;
  kapp->eventLoop()->exitLoop();
}

void KonqPopupMenu::slotLocalURLKIODestroyed()
{
  if (!d->localURLSlotFired) {
    d->localURLSlotFired = true;
    d->localURLResultURL = KURL();
    d->localURLResultIsLocal = false;
    kapp->eventLoop()->exitLoop();
  }
}

/**
        Plugin
*/

KonqPopupMenuPlugin::KonqPopupMenuPlugin( KonqPopupMenu *parent, const char *name )
    : TQObject( parent, name )
{
}

KonqPopupMenuPlugin::~KonqPopupMenuPlugin()
{
}

#include "konq_popupmenu.moc"
