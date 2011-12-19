/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <typeinfo>
#include <tqcursor.h>
#include <tqbitmap.h>
#include <tqpixmap.h>
#include <tqimage.h>
#include <tqmap.h>

#include <dcopclient.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kprocess.h>
#include <krun.h>
#include <kservicegroup.h>
#include <ksycoca.h>
#include <ksycocaentry.h>
#include <kservice.h>
#include <kurldrag.h>
#include <kio/job.h>

#include "global.h"
#include "kicker.h"
#include "kickerSettings.h"
#include "menumanager.h"
#include "popupmenutitle.h"
#include "panelbutton.h"
#include "recentapps.h"
#include "service_mnu.h"
#include "service_mnu.moc"

PanelServiceMenu::PanelServiceMenu(const TQString & label, const TQString & relPath, TQWidget * parent,
                                   const char * name, bool addmenumode, const TQString & insertInlineHeader)
    : KPanelMenu(label, parent, name),
      relPath_(relPath),
      insertInlineHeader_( insertInlineHeader ),
      clearOnClose_(false),
      addmenumode_(addmenumode),
      popupMenu_(0)
{
    excludeNoDisplay_=true;

    connect(KSycoca::self(), TQT_SIGNAL(databaseChanged()),
            TQT_SLOT(slotClearOnClose()));
    connect(this, TQT_SIGNAL(aboutToHide()), this, TQT_SLOT(slotClose()));
}

PanelServiceMenu::~PanelServiceMenu()
{
    clearSubmenus();
}


void PanelServiceMenu::setExcludeNoDisplay( bool flag )
{
  excludeNoDisplay_=flag;
}

void PanelServiceMenu::showMenu()
{
    activateParent(TQString::null);
}

// the initialization is split in initialize() and
// doInitialize() so that a subclass does not have to
// redo all the service parsing (see e.g. kicker/menuext/prefmenu)

void PanelServiceMenu::initialize()
{
    if (initialized()) return;

    setInitialized(true);
    entryMap_.clear();
    clear();

    clearSubmenus();
    searchSubMenuIDs.clear();
    searchMenuItems.clear();
    doInitialize();
}

void PanelServiceMenu::fillMenu(KServiceGroup::Ptr& _root,
                                KServiceGroup::List& _list,
                                const TQString& /* _relPath */,
                                int& id)
{
    TQStringList suppressGenericNames = _root->suppressGenericNames();

    KServiceGroup::List::ConstIterator it = _list.begin();
    KSortableValueList<KSharedPtr<KSycocaEntry>,TQCString> slist;
    KSortableValueList<KSharedPtr<KSycocaEntry>,TQCString> glist;
    TQMap<TQString,TQString> specialTitle;
    TQMap<TQString,TQString> categoryIcon;

    bool separatorNeeded = false;
    for (; it != _list.end(); ++it)
    {
        KSycocaEntry * e = *it;

        if (e->isType(KST_KServiceGroup))
        {
            KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
            if ( KickerSettings::reduceMenuDepth() && g->SuSEshortMenu() ){
               KServiceGroup::List l = g->entries(true, excludeNoDisplay_ );
               if ( l.count() == 1 ) {
                  // the special case, we want to short the menu.
                  // TOFIX? : this works only for one level
                  KServiceGroup::List::ConstIterator _it=l.begin();
                  KSycocaEntry *_e = *_it;
                  if (_e->isType(KST_KService)) {
                     KService::Ptr s(static_cast<KService *>(_e));
		     TQString key;
                     if ( g->SuSEgeneralDescription() ) {
			// we use the application name
                        key = s->name();
                        if( !s->genericName().isEmpty()) {
                           if (KickerSettings::menuEntryFormat() == KickerSettings::NameAndDescription)
                               key = s->name() + " (" + s->genericName() + ")";
			   else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName)
                               key = s->genericName() + " (" + s->name() + ")";
			   else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly)
                             key = s->genericName();
                        }
                     }
		     else {
			// we use the normal menu description
			key = s->name();
                        if( !s->genericName().isEmpty()) {
                           if (KickerSettings::menuEntryFormat() == KickerSettings::NameAndDescription)
                               key = s->name() + " (" + g->caption() + ")";
			   else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName)
                               key = g->caption() + " (" + s->name() + ")";
			   else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly)
                             key = g->caption();
                        }
		     }
		     specialTitle.insert( _e->name(), key );
		     categoryIcon.insert( _e->name(), g->icon() );
                     slist.insert( key.local8Bit(), _e );
                     // and escape from here
                     continue;
                  }
               }
            }
            glist.insert( g->caption().local8Bit(), e );
        }else if( e->isType(KST_KService)) {
            KService::Ptr s(static_cast<KService *>(e));
            TQString name = s->name();
            if( !s->genericName().isEmpty()) {
               if (KickerSettings::menuEntryFormat() == KickerSettings::NameAndDescription)
                   name = s->name() + " (" + s->genericName() + ")";
	       else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName)
                   name = s->genericName() + " (" + s->name() + ")";
	       else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly)
                   name = s->genericName();
            }
            slist.insert( name.local8Bit(), e );
        } else
            slist.insert( e->name().local8Bit(), e );
    }

    _list = _root->SuSEsortEntries( slist, glist, excludeNoDisplay_, true );
    it = _list.begin();

    for (; it != _list.end(); ++it) {

        KSycocaEntry * e = *it;

        if (e->isType(KST_KServiceGroup)) {
            KServiceGroup::Ptr g(static_cast<KServiceGroup *>(e));
            if ( KickerSettings::reduceMenuDepth() && g->SuSEshortMenu() ){
               KServiceGroup::List l = g->entries(true, excludeNoDisplay_ );
               if ( l.count() == 1 ) {
 /*               // the special case, we want to short the menu.
                  // TOFIX? : this works only for one level
                  KServiceGroup::List::ConstIterator _it=l.begin();
                  KSycocaEntry *_e = *_it;
                  if (_e->isType(KST_KService)) {
                     KService::Ptr s(static_cast<KService *>(_e));
                     if ( g->SuSEgeneralDescription() )
                       // we use the application name
                       insertMenuItem(s, id++, -1, 0, TQString::null, TQString::null, g->icon() );
                    else
                       // we use the normal menu description
                       insertMenuItem(s, id++, -1, 0, TQString::null, g->caption(), g->icon() );
                     // and escape from here */
                     continue;
 //                 }   
               }      
            }         
            // standard sub menu
                      
            TQString groupCaption = g->caption();
                      
           // Avoid adding empty groups.
            KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(g->relPath());
                      
            int nbChildCount = subMenuRoot->childCount();
            if (nbChildCount == 0 && !g->showEmptyMenu())
            {         
                continue;
            }         
                      
            TQString inlineHeaderName = g->showInlineHeader() ? groupCaption : "";
            // Item names may contain ampersands. To avoid them being converted
            // to accelerators, replace them with two ampersands.
            groupCaption.replace("&", "&&");

            if ( nbChildCount == 1 && g->allowInline() && g->inlineAlias())
            {
                KServiceGroup::Ptr element = KServiceGroup::group(g->relPath());
                if ( element )
                {
                    //just one element
                    KServiceGroup::List listElement = element->entries(true, excludeNoDisplay_, true, KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName || KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly);
                    KSycocaEntry * e1 = *( listElement.begin() );
                    if ( e1->isType( KST_KService ) )
                    {
                        if (separatorNeeded)
                        {
                            insertSeparator();
                            separatorNeeded = false;
                        }

                        KService::Ptr s(static_cast<KService *>(e1));
//                        insertMenuItem(s, id++, -1, &suppressGenericNames);
                        insertMenuItem(s, id++, -1, &suppressGenericNames, TQString::null, specialTitle[s->name()], categoryIcon[s->name()] );
                        continue;
                    }
                }
            }

            if (g->allowInline() && ((nbChildCount <= g->inlineValue() ) ||   (g->inlineValue() == 0)))
            {
                //inline all entries
                KServiceGroup::Ptr rootElement = KServiceGroup::group(g->relPath());

                if (!rootElement || !rootElement->isValid())
                {
                    break;
                }

                KServiceGroup::List listElement = rootElement->entries(true, excludeNoDisplay_, true, KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName || KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly);

                if ( !g->inlineAlias() && !inlineHeaderName.isEmpty() )
                {
                    int mid = insertItem(new PopupMenuTitle(inlineHeaderName, font()), id + 1, id); 
                    id++;
                    setItemEnabled( mid, false );
                }

                fillMenu( rootElement, listElement,  g->relPath(), id );
                continue;
            }

            // Ignore dotfiles.
            if ((g->name().tqat(0) == '.'))
            {
                continue;
            }

            PanelServiceMenu * m =
                newSubMenu(g->name(), g->relPath(), this, g->name().utf8(), inlineHeaderName);
            m->setCaption(groupCaption);

            TQIconSet iconset = KickerLib::menuIconSet(g->icon());

            if (separatorNeeded)
            {
                insertSeparator();
                separatorNeeded = false;
            }

            int newId = insertItem(iconset, groupCaption, m, id++);
            entryMap_.insert(newId, static_cast<KSycocaEntry*>(g));
            // This submenu will be searched when applying a search string
            searchSubMenuIDs[m] = newId;
            // Also search the submenu name itself
            searchMenuItems.insert(newId);
            // We have to delete the sub menu our selves! (See Qt docs.)
            subMenus.append(m);
        }
        else if (e->isType(KST_KService))
        {
            if (separatorNeeded)
            {
                insertSeparator();
                separatorNeeded = false;
            }

            KService::Ptr s(static_cast<KService *>(e));
            searchMenuItems.insert(id);
//            insertMenuItem(s, id++, -1, &suppressGenericNames);
            insertMenuItem(s, id++, -1, &suppressGenericNames, TQString::null, specialTitle[s->name()], categoryIcon[s->name()] );
        }
        else if (e->isType(KST_KServiceSeparator))
        {
            separatorNeeded = true;
        }
    }
#if 0
    // WABA: tear off handles don't work together with dynamically updated
    // menus. We can't update the menu while torn off, and we don't know
    // when it is torn off.
    if ( count() > 0  && !relPath_.isEmpty() )
      if (KGlobalSettings::insertTearOffHandle())
        insertTearOffHandle();
#endif
}

void PanelServiceMenu::clearSubmenus()
{
    for (PopupMenuList::const_iterator it = subMenus.constBegin();
         it != subMenus.constEnd();
         ++it)
    {
        delete *it;
    }
    subMenus.clear();
}

void PanelServiceMenu::doInitialize()
{

    // Set the startposition outside the panel, so there is no drag initiated
    // when we use drag and click to select items. A drag is only initiated when
    // you click to open the menu, and then press and drag an item.
    startPos_ = TQPoint(-1,-1);

    // We ask KSycoca to give us all services (sorted).
    KServiceGroup::Ptr root = KServiceGroup::group(relPath_);

    if (!root || !root->isValid())
        return;

    KServiceGroup::List list = root->entries(true, excludeNoDisplay_, true, KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName || KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly);

    if (list.isEmpty()) {
        setItemEnabled(insertItem(i18n("No Entries")), false);
        return;
    }

    int id = serviceMenuStartId();

    if (addmenumode_) {
        int mid = insertItem(KickerLib::menuIconSet("ok"), i18n("Add This Menu"), id++);
        entryMap_.insert(mid, static_cast<KSycocaEntry*>(root));

        if (relPath_ == "")
        {
            insertItem(KickerLib::menuIconSet("exec"), i18n("Add Non-KDE Application"),
                       this, TQT_SLOT(addNonKDEApp()));
        }

        if (list.count() > 0) {
            insertSeparator();
            id++;
        }
    }
    if ( !insertInlineHeader_.isEmpty() )
    {
        int mid = insertItem(new PopupMenuTitle(insertInlineHeader_, font()), -1, 0);
        setItemEnabled( mid, false );
    }
    fillMenu( root, list, relPath_, id );
}

void PanelServiceMenu::configChanged()
{
    deinitialize();
}

void PanelServiceMenu::insertMenuItem(KService::Ptr & s, int nId,
                                      int nIndex/*= -1*/,
                                      const TQStringList *suppressGenericNames /* = 0 */,
                                      const TQString & aliasname, const TQString & label /*=TQString::NULL*/,
                                      const TQString & categoryIcon /*=TQString::null*/)
{
    TQString serviceName = (aliasname.isEmpty() ? s->name() : aliasname).simplifyWhiteSpace();
    TQString comment = s->genericName().simplifyWhiteSpace();

    if (!comment.isEmpty())
    {
        if (KickerSettings::menuEntryFormat() == KickerSettings::NameAndDescription)
        {
            if ((!suppressGenericNames ||
                 !suppressGenericNames->contains(s->untranslatedGenericName())) &&
                serviceName.find(comment, 0, true) == -1)
            {
                if (comment.find(serviceName, 0, true) == -1)
                {
                    serviceName = i18n("Entries in K-menu: %1 app name, %2 description", "%1 - %2").arg(serviceName, comment);
                }
                else
                {
                    serviceName = comment;
                }
            }
        }
        else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionAndName)
        {
            serviceName = i18n("Entries in K-menu: %1 description, %2 app name", "%1 (%2)").arg(comment, serviceName);
        }
        else if (KickerSettings::menuEntryFormat() == KickerSettings::DescriptionOnly)
        {
            serviceName = comment;
        }
    }

    // restrict menu entries to a sane length
    if ( serviceName.length() > 60 ) {
      serviceName.truncate( 57 );
      serviceName += "...";
    }

    // check for NoDisplay
    if (s->noDisplay())
        return;

    // ignore dotfiles.
    if ((serviceName.tqat(0) == '.'))
        return;

    // item names may contain ampersands. To avoid them being converted
    // to accelerators, replace them with two ampersands.
    serviceName.replace("&", "&&");

    TQString icon = s->icon();
    if (icon=="unknown")
        icon = categoryIcon;

    int newId;
    if ( label.isEmpty() )
       newId = insertItem(KickerLib::menuIconSet(s->icon()), serviceName, nId, nIndex);
    else
       newId = insertItem(KickerLib::menuIconSet(s->icon()), label, nId, nIndex);
    entryMap_.insert(newId, static_cast<KSycocaEntry*>(s));
}

void PanelServiceMenu::activateParent(const TQString &child)
{
    PanelServiceMenu *parentMenu = dynamic_cast<PanelServiceMenu*>(parent());
    if (parentMenu)
    {
        parentMenu->activateParent(relPath_);
    }
    else
    {
        PanelPopupButton *kButton = MenuManager::the()->findKButtonFor(this);
        if (kButton)
        {
            adjustSize();
            kButton->showMenu();
        }
        else
        {
            show();
        }
    }

    if (!child.isEmpty())
    {
        EntryMap::Iterator mapIt;
        for ( mapIt = entryMap_.begin(); mapIt != entryMap_.end(); ++mapIt )
        {
            KServiceGroup *g = dynamic_cast<KServiceGroup *>(static_cast<KSycocaEntry*>(mapIt.data()));

            // if the dynamic_cast fails, we are looking at a KService entry
            if (g && (g->relPath() == child))
            {
               activateItemAt(indexOf(mapIt.key()));
               return;
            }
        }
    }
}

bool PanelServiceMenu::highlightMenuItem( const TQString &menuItemId )
{
    initialize();

    // Check menu itself
    EntryMap::Iterator mapIt;
    for ( mapIt = entryMap_.begin(); mapIt != entryMap_.end(); ++mapIt )
    {
        // Skip recent files menu
        if (mapIt.key() >= serviceMenuEndId())
        {
            continue;
        }
        KService *s = dynamic_cast<KService *>(
            static_cast<KSycocaEntry*>(mapIt.data()));
        if (s && (s->menuId() == menuItemId))
        {
            activateParent(TQString::null);
            int index = indexOf(mapIt.key());
            setActiveItem(index);

            // Warp mouse pointer to location of active item
            TQRect r = itemGeometry(index);
            TQCursor::setPos(mapToGlobal(TQPoint(r.x()+ r.width() - 15,
                r.y() + r.height() - 5)));
            return true;
        }
    }

    for(PopupMenuList::iterator it = subMenus.begin();
        it != subMenus.end();
        ++it)
    {
        PanelServiceMenu *serviceMenu = dynamic_cast<PanelServiceMenu*>(*it);
        if (serviceMenu && serviceMenu->highlightMenuItem(menuItemId))
            return true;
    }
    return false;
}

void PanelServiceMenu::slotExec(int id)
{
    if (!entryMap_.contains(id))
    {
        return;
    }

    KSycocaEntry * e = entryMap_[id];

    kapp->propagateSessionManager();

    KService::Ptr service = static_cast<KService *>(e);
    KApplication::startServiceByDesktopPath(service->desktopEntryPath(),
      TQStringList(), 0, 0, 0, "", true);

    updateRecentlyUsedApps(service);
    startPos_ = TQPoint(-1,-1);
}

void PanelServiceMenu::mousePressEvent(TQMouseEvent * ev)
{
    startPos_ = ev->pos();
    KPanelMenu::mousePressEvent(ev);
}

void PanelServiceMenu::mouseReleaseEvent(TQMouseEvent * ev)
{
    if (ev->button() == Qt::RightButton && !Kicker::the()->isKioskImmutable())
    {
        int id = idAt( ev->pos() );

        if (id < serviceMenuStartId())
        {
            return;
        }

        if (!entryMap_.contains(id))
        {
            kdDebug(1210) << "Cannot find service with menu id " << id << endl;
            return;
        }

        contextKSycocaEntry_ = entryMap_[id];

        delete popupMenu_;
        popupMenu_ = new KPopupMenu(this);
        connect(popupMenu_, TQT_SIGNAL(activated(int)), TQT_SLOT(slotContextMenu(int)));
        bool hasEntries = false;

        switch (contextKSycocaEntry_->sycocaType())
        {
        case KST_KService:
            if (kapp->authorize("editable_desktop_icons"))
            {
                hasEntries = true;
                popupMenu_->insertItem(SmallIconSet("desktop"),
                    i18n("Add Item to Desktop"), AddItemToDesktop);
            }
            if (kapp->authorizeKAction("kicker_rmb") && !Kicker::the()->isImmutable())
            {
                hasEntries = true;
                popupMenu_->insertItem(SmallIconSet("kicker"),
                    i18n("Add Item to Main Panel"), AddItemToPanel);
            }
            if (kapp->authorizeKAction("menuedit"))
            {
                hasEntries = true;
                popupMenu_->insertItem(SmallIconSet("kmenuedit"),
                    i18n("Edit Item"), EditItem);
            }
            if (kapp->authorize("run_command"))
            {
                hasEntries = true;
                popupMenu_->insertItem(SmallIconSet("run"),
                    i18n("Put Into Run Dialog"), PutIntoRunDialog);
            }
            break;

        case KST_KServiceGroup:
            if (kapp->authorize("editable_desktop_icons"))
            {
                hasEntries = true;
                popupMenu_->insertItem(SmallIconSet("desktop"),
                    i18n("Add Menu to Desktop"), AddMenuToDesktop);
            }
            if (kapp->authorizeKAction("kicker_rmb") && !Kicker::the()->isImmutable())
            {
                hasEntries = true;
                popupMenu_->insertItem(SmallIconSet("kicker"),
                    i18n("Add Menu to Main Panel"), AddMenuToPanel);
            }
            if (kapp->authorizeKAction("menuedit"))
            {
                hasEntries = true;
                popupMenu_->insertItem(SmallIconSet("kmenuedit"),
                    i18n("Edit Menu"), EditMenu);
            }
            break;

        default:
            break;
        }

        if (hasEntries)
        {
            popupMenu_->popup(this->mapToGlobal(ev->pos()));
            return;
        }
    }

    delete popupMenu_;
    popupMenu_ = 0;

    KPanelMenu::mouseReleaseEvent(ev);
}

extern int kicker_screen_number;

void PanelServiceMenu::slotContextMenu(int selected)
{
    KProcess *proc;
    KService::Ptr service;
    KServiceGroup::Ptr g;
    TQByteArray ba;
    TQDataStream ds(ba, IO_WriteOnly);

    KURL src,dest;
    KIO::CopyJob *job;
    KDesktopFile *df;

    switch (selected) {
        case AddItemToDesktop:
	    service = static_cast<KService *>(contextKSycocaEntry_);

	    src.setPath( KGlobal::dirs()->findResource( "apps", service->desktopEntryPath() ) );
	    dest.setPath( KGlobalSettings::desktopPath() );
	    dest.setFileName( src.fileName() );

            job = KIO::copyAs( src, dest );
            job->setDefaultPermissions( true );
	    break;

        case AddItemToPanel: {
            TQCString appname = "kicker";
            if ( kicker_screen_number )
                appname.sprintf("kicker-screen-%d", kicker_screen_number);
            service = static_cast<KService *>(contextKSycocaEntry_);
            kapp->dcopClient()->send(appname, "Panel", "addServiceButton(TQString)", service->desktopEntryPath());
            break;
        }

	case EditItem:
            proc = new KProcess(TQT_TQOBJECT(this));
            *proc << KStandardDirs::findExe(TQString::fromLatin1("kmenuedit"));
            *proc << "/"+relPath_ << static_cast<KService *>(contextKSycocaEntry_)->menuId();
            proc->start();
	    break;

        case PutIntoRunDialog: {
            close();
            TQCString appname = "kdesktop";
            if ( kicker_screen_number )
                appname.sprintf("kdesktop-screen-%d", kicker_screen_number);
            service = static_cast<KService *>(contextKSycocaEntry_);
            kapp->updateRemoteUserTimestamp( appname );
            kapp->dcopClient()->send(appname, "default", "popupExecuteCommand(TQString)", service->exec());
            break;
        }

	case AddMenuToDesktop:
	    g = static_cast<KServiceGroup *>(contextKSycocaEntry_);
            dest.setPath( KGlobalSettings::desktopPath() );
	    dest.setFileName( g->caption() );

	    df = new KDesktopFile( dest.path() );
            df->writeEntry( "Icon", g->icon() );
            df->writePathEntry( "URL", "programs:/"+g->name() );
	    df->writeEntry( "Name", g->caption() );
	    df->writeEntry( "Type", "Link" );
            df->sync();
	    delete df;

	    break;

        case AddMenuToPanel: {
            TQCString appname = "kicker";
            if ( kicker_screen_number )
                appname.sprintf("kicker-screen-%d", kicker_screen_number);

            g = static_cast<KServiceGroup *>(contextKSycocaEntry_);
            ds << "foo" << g->relPath();
            kapp->dcopClient()->send("kicker", "Panel", "addServiceMenuButton(TQString,TQString)", ba);
            break;
        }

        case EditMenu:
            proc = new KProcess(TQT_TQOBJECT(this));
            *proc << KStandardDirs::findExe(TQString::fromLatin1("kmenuedit"));
            *proc << "/"+static_cast<KServiceGroup *>(contextKSycocaEntry_)->relPath();
            proc->start();
	    break;

	default:
	    break;
	}
}

void PanelServiceMenu::mouseMoveEvent(TQMouseEvent * ev)
{
    KPanelMenu::mouseMoveEvent(ev);

    if (Kicker::the()->isKioskImmutable())
        return;

    if ( (ev->state() & Qt::LeftButton ) != Qt::LeftButton )
        return;

    TQPoint p = ev->pos() - startPos_;
    if (p.manhattanLength() <= TQApplication::startDragDistance() )
        return;

    int id = idAt(startPos_);

    // Don't drag items we didn't create.
    if (id < serviceMenuStartId())
        return;

    if (!entryMap_.contains(id)) {
        kdDebug(1210) << "Cannot find service with menu id " << id << endl;
        return;
    }

    KSycocaEntry * e = entryMap_[id];

    TQPixmap icon;
    KURL url;

    switch (e->sycocaType()) {

        case KST_KService:
        {
            icon = static_cast<KService *>(e)->pixmap(KIcon::Small);
            TQString filePath = static_cast<KService *>(e)->desktopEntryPath();
            if (filePath[0] != '/')
            {
                filePath = locate("apps", filePath);
            }
            url.setPath(filePath);
            break;
        }

        case KST_KServiceGroup:
        {
            icon = KGlobal::iconLoader()
                   ->loadIcon(static_cast<KServiceGroup *>(e)->icon(), KIcon::Small);
            url = "programs:/" + static_cast<KServiceGroup *>(e)->relPath();
            break;
        }

        default:
        {
            return;
            break;
        }
    }

    // If the path to the desktop file is relative, try to get the full
    // path from KStdDirs.

    KURLDrag *d = new KURLDrag(KURL::List(url), this);
    connect(d, TQT_SIGNAL(destroyed()), this, TQT_SLOT(slotDragObjectDestroyed()));
    d->setPixmap(icon);
    d->dragCopy();

    // Set the startposition outside the panel, so there is no drag initiated
    // when we use drag and click to select items. A drag is only initiated when
    // you click to open the menu, and then press and drag an item.
    startPos_ = TQPoint(-1,-1);
}

void PanelServiceMenu::dragEnterEvent(TQDragEnterEvent *event)
{
    // Set the DragObject's target to this widget. This is needed because the
    // widget doesn't accept drops, but we want to determine if the drag object
    // is dropped on it. This avoids closing on accidental drags. If this
    // widget accepts drops in the future, these lines can be removed.
    if (event->source() == this)
    {
        KURLDrag::setTarget(this);
    }
    event->ignore();
}

void PanelServiceMenu::dragLeaveEvent(TQDragLeaveEvent *)
{
    // see PanelServiceMenu::dragEnterEvent why this is nescessary
    if (!TQT_TQRECT_OBJECT(frameGeometry()).contains(TQCursor::pos()))
    {
        KURLDrag::setTarget(0);
    }
}

void PanelServiceMenu::slotDragObjectDestroyed()
{
    if (KURLDrag::target() != this)
    {
        // we need to re-enter the event loop before calling close() here
        // this gets called _before_ the drag object is destroyed, so we are
        // still in its event loop. closing the menu before that event loop is
        // exited may result in getting hung up in it which in turn prevents
        // the execution of any code after the original exec() statement
        // though the panels themselves continue on otherwise normally
        // (we just have some sort of nested event loop)
        TQTimer::singleShot(0, this, TQT_SLOT(close()));
    }
}

PanelServiceMenu *PanelServiceMenu::newSubMenu(const TQString & label, const TQString & relPath,
                                               TQWidget * parent, const char * name, const TQString& _inlineHeader)
{
    return new PanelServiceMenu(label, relPath, parent, name, false,_inlineHeader);
}

void PanelServiceMenu::slotClearOnClose()
{
    if (!initialized()) return;

    if (!isVisible()){
        clearOnClose_ = false;
        slotClear();
    } else {
        clearOnClose_ = true;
    }
}

void PanelServiceMenu::slotClose()
{
    if (clearOnClose_)
    {
        clearOnClose_ = false;
        slotClear();
    }

    delete popupMenu_;
    popupMenu_ = 0;
}

void PanelServiceMenu::slotClear()
{
    if (isVisible())
    {
        // QPopupMenu's aboutToHide() is emitted before the popup is really hidden,
        // and also before a click in the menu is handled, so do the clearing
        // only after that has been handled
        TQTimer::singleShot(100, this, TQT_SLOT(slotClear()));
        return;
    }

    entryMap_.clear();
    KPanelMenu::slotClear();

    for (PopupMenuList::iterator it = subMenus.begin();
         it != subMenus.end();
         ++it)
    {
        delete *it;
    }
    subMenus.clear();
    searchSubMenuIDs.clear();
    searchMenuItems.clear();
}

void PanelServiceMenu::selectFirstItem()
{
    setActiveItem(indexOf(serviceMenuStartId()));
}

void PanelServiceMenu::setSearchString(const TQString &searchString)
{
    // We must initialize the menu, because it might have not been opened before
    initialize();

    bool foundSomething = false;
    std::set<int> nonemptyMenus;
    std::set<int>::const_iterator menuItemIt(searchMenuItems.begin());
    // Apply the filter on this menu
    for (; menuItemIt != searchMenuItems.end(); ++menuItemIt) {
        int id = *menuItemIt;
        KService* s = dynamic_cast< KService* >( static_cast< KSycocaEntry* >( entryMap_[ id ]));
        TQString menuText = text(id);
        if (menuText.contains(searchString, false) > 0
            || ( s != NULL && ( s->name().contains(searchString, false) > 0
                               || s->exec().contains(searchString, false) > 0
                               || s->comment().contains(searchString, false) > 0
                               || s->genericName().contains(searchString, false) > 0
                               || s->exec().contains(searchString, false) > 0 )
                )) {
            setItemEnabled(id, true);
            foundSomething = true;
            nonemptyMenus.insert(id);
        }
        else {
            setItemEnabled(id, false);
        }
    }
    // Apply the filter on this menu
    /*for (int i=count()-1; i>=0; --i) {
        int id = idAt(i);
        TQString menuText = text(id);
        if (menuText.contains(searchString, false) > 0) {
            setItemEnabled(id, true);
            foundSomething = true;
            nonemptyMenus.insert(id);
        }
        else {
            setItemEnabled(id, false);
        }
    }*/

    PanelServiceMenuMap::iterator it(searchSubMenuIDs.begin());
    // Apply the search filter on submenus
    for (; it != searchSubMenuIDs.end(); ++it) {
        it.key()->setSearchString(searchString);
        if (nonemptyMenus.find(it.data()) != nonemptyMenus.end()) {
            // if the current menu is a match already, we don't
            // block access to the contained items
            setItemEnabled(it.data(), true);
            it.key()->setSearchString(TQString());
            foundSomething = true;
        }
        else if (it.key()->hasSearchResults()) {
            setItemEnabled(it.data(), true);
            foundSomething = true;
        }
        else {
            setItemEnabled(it.data(), false);
        }
    }

    hasSearchResults_ = foundSomething;
}

bool PanelServiceMenu::hasSearchResults()
{
    return hasSearchResults_;
}

// updates "recent" section of KMenu
void PanelServiceMenu::updateRecentlyUsedApps(KService::Ptr &service)
{
    TQString strItem(service->desktopEntryPath());

    // don't add an item from root kmenu level
    if (!strItem.contains('/'))
    {
        return;
    }

    // add it into recent apps list
    RecentlyLaunchedApps::the().appLaunched(strItem);
    RecentlyLaunchedApps::the().save();
    RecentlyLaunchedApps::the().m_bNeedToUpdate = true;
}

