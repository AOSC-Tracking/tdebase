/*  This file is part of the KDE project
    Copyright (C) 2000  David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <tqclipboard.h>
#include "konq_operations.h"

#include <kautomount.h>
#include <kinputdialog.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <knotifyclient.h>
#include <krun.h>
#include <kshell.h>
#include <tdeshortcut.h>

#include <kdirnotify_stub.h>

#include <dcopclient.h>
#include "konq_undo.h"
#include "konq_defaults.h"
#include "konqbookmarkmanager.h"

// For doDrop
#include <tqdir.h>//first
#include <assert.h>
#include <tdeapplication.h>
#include <kipc.h>
#include <kdebug.h>
#include <tdefileitem.h>
#include <kdesktopfile.h>
#include <kurldrag.h>
#include <tdeglobalsettings.h>
#include <kimageio.h>
#include <tdeio/job.h>
#include <tdeio/jobclasses.h>
#include <tdeio/paste.h>
#include <tdeio/netaccess.h>
#include <tdeio/renamedlg.h>
#include <konq_drag.h>
#include <konq_iconviewwidget.h>
#include <kprotocolinfo.h>
#include <kprocess.h>
#include <kstringhandler.h>
#include <tqpopupmenu.h>
#include <unistd.h>
#include <X11/Xlib.h>

KBookmarkManager * KonqBookmarkManager::s_bookmarkManager;

KonqOperations::KonqOperations( TQWidget *parent )
    : TQObject( parent, "KonqOperations" ),
      m_method( UNKNOWN ), m_info(0L), m_pasteInfo(0L)
{
}

KonqOperations::~KonqOperations()
{
    delete m_info;
    delete m_pasteInfo;
}

void KonqOperations::editMimeType( const TQString & mimeType )
{
  TQString keditfiletype = TQString::fromLatin1("keditfiletype");
  KRun::runCommand( keditfiletype + " " + TDEProcess::quote(mimeType),
                    keditfiletype, keditfiletype /*unused*/);
}

void KonqOperations::del( TQWidget * parent, int method, const KURL::List & selectedURLs )
{
  kdDebug(1203) << "KonqOperations::del " << parent->className() << endl;
  if ( selectedURLs.isEmpty() )
  {
    kdWarning(1203) << "Empty URL list !" << endl;
    return;
  }

  KonqOperations * op = new KonqOperations( parent );
    ConfirmationType confirmation = DEFAULT_CONFIRMATION;
  op->_del( method, selectedURLs, confirmation );
}

void KonqOperations::emptyTrash()
{
  KonqOperations *op = new KonqOperations( 0L );
  op->_del( EMPTYTRASH, KURL("trash:/"), SKIP_CONFIRMATION );
}

void KonqOperations::restoreTrashedItems( const KURL::List& urls )
{
  KonqOperations *op = new KonqOperations( 0L );
  op->_restoreTrashedItems( urls );
}

void KonqOperations::mkdir( TQWidget *parent, const KURL & url )
{
    TDEIO::Job * job = TDEIO::mkdir( url );
    KonqOperations * op = new KonqOperations( parent );
    op->setOperation( job, MKDIR, KURL::List(), url );
    (void) new KonqCommandRecorder( KonqCommand::MKDIR, KURL(), url, job ); // no support yet, apparently
}

void KonqOperations::doPaste( TQWidget * parent, const KURL & destURL )
{
   doPaste(parent, destURL, TQPoint());
}

void KonqOperations::doPaste( TQWidget * parent, const KURL & destURL, const TQPoint &pos )
{
    // move or not move ?
    bool move = false;
    TQMimeSource *data = TQApplication::clipboard()->data();
    if ( data->provides( "application/x-tde-cutselection" ) ) {
      move = KonqDrag::decodeIsCutSelection( data );
      kdDebug(1203) << "move (from clipboard data) = " << move << endl;
    }

    TDEIO::Job *job = TDEIO::pasteClipboard( destURL, move );
    if ( job )
    {
        KonqOperations * op = new KonqOperations( parent );
        TDEIO::CopyJob * copyJob = static_cast<TDEIO::CopyJob *>(job);
        KIOPasteInfo * pi = new KIOPasteInfo;
        pi->mousePos = pos;
        op->setPasteInfo( pi );
        op->setOperation( job, move ? MOVE : COPY, copyJob->srcURLs(), copyJob->destURL() );
        (void) new KonqCommandRecorder( move ? KonqCommand::MOVE : KonqCommand::COPY, KURL::List(), destURL, job );
    }
}

void KonqOperations::copy( TQWidget * parent, int method, const KURL::List & selectedURLs, const KURL& destUrl )
{
  kdDebug(1203) << "KonqOperations::copy() " << parent->className() << endl;
  if ((method!=COPY) && (method!=MOVE) && (method!=LINK))
  {
    kdWarning(1203) << "Illegal copy method !" << endl;
    return;
  }
  if ( selectedURLs.isEmpty() )
  {
    kdWarning(1203) << "Empty URL list !" << endl;
    return;
  }

  KonqOperations * op = new KonqOperations( parent );
  TDEIO::Job* job(0);
  if (method==LINK)
     job= TDEIO::link( selectedURLs, destUrl);
  else if (method==MOVE)
     job= TDEIO::move( selectedURLs, destUrl);
  else
     job= TDEIO::copy( selectedURLs, destUrl);

  op->setOperation( job, method, selectedURLs, destUrl );

  if (method==COPY)
     (void) new KonqCommandRecorder( KonqCommand::COPY, selectedURLs, destUrl, job );
  else
     (void) new KonqCommandRecorder( method==MOVE?KonqCommand::MOVE:KonqCommand::LINK, selectedURLs, destUrl, job );
}

void KonqOperations::_del( int method, const KURL::List & _selectedURLs, ConfirmationType confirmation )
{
    KURL::List selectedURLs;
    for (KURL::List::ConstIterator it = _selectedURLs.begin(); it != _selectedURLs.end(); ++it) {
        if (KProtocolInfo::supportsDeleting(*it)) {
            selectedURLs.append(*it);
        }
    }
    if (selectedURLs.isEmpty()) {
        delete this;
        return;
    }

    if ( askDeleteConfirmation( selectedURLs, method, confirmation, parentWidget() ) )
    {
        //m_srcURLs = selectedURLs;
        TDEIO::Job *job;
        m_method = method;
        switch( method )
        {
        case TRASH:
        {
            job = TDEIO::trash( selectedURLs );
            (void) new KonqCommandRecorder( KonqCommand::TRASH, selectedURLs, "trash:/", job );
            break;
        }
        case EMPTYTRASH:
        {
            // Same as in ktrash --empty
            TQByteArray packedArgs;
            TQDataStream stream( packedArgs, IO_WriteOnly );
            stream << (int)1;
            job = TDEIO::special( "trash:/", packedArgs );
            KNotifyClient::event(0, "Trash: emptied");
            break;
        }
        case DEL:
            job = TDEIO::del( selectedURLs );
            break;
        case SHRED:
            job = TDEIO::del( selectedURLs, true );
            break;
        default:
            kdWarning() << "Unknown operation: " << method << endl;
            delete this;
            return;
        }
        connect( job, TQT_SIGNAL( result( TDEIO::Job * ) ),
                 TQT_SLOT( slotResult( TDEIO::Job * ) ) );
    } else
        delete this;
}

void KonqOperations::_restoreTrashedItems( const KURL::List& urls )
{
    m_method = RESTORE;
    KonqMultiRestoreJob* job = new KonqMultiRestoreJob( urls, true );
    connect( job, TQT_SIGNAL( result( TDEIO::Job * ) ),
             TQT_SLOT( slotResult( TDEIO::Job * ) ) );
}

bool KonqOperations::askDeleteConfirmation( const KURL::List & selectedURLs, int method, ConfirmationType confirmation, TQWidget* widget )
{
    if ( confirmation == SKIP_CONFIRMATION )
        return true;
    TQString keyName;
    bool ask = ( confirmation == FORCE_CONFIRMATION );
    if ( !ask )
    {
        TDEConfig config("konquerorrc", true, false);
        config.setGroup( "Trash" );
        keyName = ( method == DEL ? "ConfirmDelete" : method == SHRED ? "ConfirmShred" : "ConfirmTrash" );
        bool defaultValue = ( method == DEL ? DEFAULT_CONFIRMDELETE : method == SHRED ? DEFAULT_CONFIRMSHRED : DEFAULT_CONFIRMTRASH );
        ask = config.readBoolEntry( keyName, defaultValue );
    }
    if ( ask )
    {
      KURL::List::ConstIterator it = selectedURLs.begin();
      TQStringList prettyList;
      for ( ; it != selectedURLs.end(); ++it ) {
        if ( (*it).protocol() == "trash" ) {
          TQString path = (*it).path();
          // HACK (#98983): remove "0-foo". Note that it works better than
	  // displaying KFileItem::name(), for files under a subdir.
          prettyList.append( path.remove(TQRegExp("^/[0-9]*-")) );
        } else
          prettyList.append( (*it).pathOrURL() );
      }

      int result;
      switch(method)
      {
      case DEL:
          result = KMessageBox::warningContinueCancelList( widget,
             	i18n( "Do you really want to delete this item?", "Do you really want to delete these %n items?", prettyList.count()),
             	prettyList,
		i18n( "Delete Files" ),
		KStdGuiItem::del(),
		keyName, KMessageBox::Dangerous);
	 break;

      case SHRED:
          result = KMessageBox::warningContinueCancelList( widget,
                i18n( "Do you really want to shred this item?", "Do you really want to shred these %n items?", prettyList.count()),
                prettyList,
                i18n( "Shred Files" ),
		KGuiItem( i18n( "Shred" ), "editshred" ),
		keyName, KMessageBox::Dangerous);
        break;

      case MOVE:
      default:
          result = KMessageBox::warningContinueCancelList( widget,
                i18n( "Do you really want to move this item to the trash?", "Do you really want to move these %n items to the trash?", prettyList.count()),
                prettyList,
		i18n( "Move to Trash" ),
		KGuiItem( i18n( "Verb", "&Trash" ), "edittrash"),
		keyName, KMessageBox::Dangerous);
      }
      if (!keyName.isEmpty())
      {
         // Check kmessagebox setting... erase & copy to konquerorrc.
         TDEConfig *config = kapp->config();
         TDEConfigGroupSaver saver(config, "Notification Messages");
         if (!config->readBoolEntry(keyName, true))
         {
            config->writeEntry(keyName, true);
            config->sync();
            TDEConfig konq_config("konquerorrc", false);
            konq_config.setGroup( "Trash" );
            konq_config.writeEntry( keyName, false );
         }
      }
      return (result == KMessageBox::Continue);
    }
    return true;
}

void KonqOperations::doDrop( const KFileItem * destItem, const KURL & dest, TQDropEvent * ev, TQWidget * parent )
{
    kdDebug(1203) << "doDrop: dest : " << dest.url() << endl;
    KURL::List lst;
    TQMap<TQString, TQString> metaData;
    if ( KURLDrag::decode( ev, lst, metaData ) ) // Are they urls ?
    {
        if( lst.count() == 0 )
        {
            kdWarning(1203) << "Oooops, no data ...." << endl;
            ev->accept(false);
            return;
        }
        kdDebug(1203) << "KonqOperations::doDrop metaData: " << metaData.count() << " entries." << endl;
        TQMap<TQString,TQString>::ConstIterator mit;
        for( mit = metaData.begin(); mit != metaData.end(); ++mit )
        {
            kdDebug(1203) << "metaData: key=" << mit.key() << " value=" << mit.data() << endl;
        }
        // Check if we dropped something on itself
        KURL::List::Iterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
        {
            kdDebug(1203) << "URL : " << (*it).url() << endl;
            if ( dest.equals( *it, true /*ignore trailing slashes*/ ) )
            {
                // The event source may be the view or an item (icon)
                // Note: ev->source() can be 0L! (in case of kdesktop) (Simon)
                if ( !ev->source() || ev->source() != parent && ev->source()->parent() != parent )
                    KMessageBox::sorry( parent, i18n("You cannot drop a folder on to itself") );
                kdDebug(1203) << "Dropped on itself" << endl;
                ev->accept(false);
                return; // do nothing instead of displaying kfm's annoying error box
            }
        }

        // Check the state of the modifiers key at the time of the drop
        Window root;
        Window child;
        int root_x, root_y, win_x, win_y;
        uint keybstate;
        XQueryPointer( tqt_xdisplay(), tqt_xrootwin(), &root, &child,
                       &root_x, &root_y, &win_x, &win_y, &keybstate );

        TQDropEvent::Action action = ev->action();
        // Check for the drop of a bookmark -> we want a Link action
        if ( ev->provides("application/x-xbel") )
        {
            keybstate |= ControlMask | ShiftMask;
            action = TQDropEvent::Link;
            kdDebug(1203) << "KonqOperations::doDrop Bookmark -> emulating Link" << endl;
        }

        KonqOperations * op = new KonqOperations(parent);
        op->setDropInfo( new DropInfo( keybstate, lst, metaData, win_x, win_y, action ) );

        // Ok, now we need destItem.
        if ( destItem )
        {
            op->asyncDrop( destItem ); // we have it already
        }
        else
        {
            // we need to stat to get it.
            op->_statURL( dest, op, TQT_SLOT( asyncDrop( const KFileItem * ) ) );
        }
        // In both cases asyncDrop will delete op when done

        ev->acceptAction();
    }
    else
    {
        //kdDebug(1203) << "Pasting to " << dest.url() << endl;
        KonqOperations * op = new KonqOperations(parent);
        TDEIO::CopyJob* job = TDEIO::pasteMimeSource( ev, dest,
                                                  i18n( "File name for dropped contents:" ),
                                                  parent );
        if ( job ) // 0 if canceled by user
        {
            op->setOperation( job, COPY, KURL::List(), job->destURL() );
            (void) new KonqCommandRecorder( KonqCommand::COPY, KURL::List(), dest, job );
        }
        ev->acceptAction();
    }
}

void KonqOperations::asyncDrop( const KFileItem * destItem )
{
    assert(m_info); // setDropInfo should have been called before asyncDrop
    m_destURL = destItem->url();

    //kdDebug(1203) << "KonqOperations::asyncDrop destItem->mode=" << destItem->mode() << " url=" << m_destURL << endl;
    // Check what the destination is
    if ( destItem->isDir() )
    {
        doFileCopy();
        return;
    }
    if ( !m_destURL.isLocalFile() )
    {
        // We dropped onto a remote URL that is not a directory!
        // (e.g. an HTTP link in the sidebar).
        // Can't do that, but we can't prevent it before stating the dest....
        kdWarning(1203) << "Cannot drop onto " << m_destURL << endl;
        delete this;
        return;
    }
    if ( (destItem->mimetype() == "application/x-desktop")
      || (destItem->mimetype() == "media/builtin-mydocuments")
      || (destItem->mimetype() == "media/builtin-mycomputer")
      || (destItem->mimetype() == "media/builtin-mynetworkplaces")
      || (destItem->mimetype() == "media/builtin-printers")
      || (destItem->mimetype() == "media/builtin-trash")
      || (destItem->mimetype() == "media/builtin-webbrowser") )
    {
        // Local .desktop file. What type ?
        KDesktopFile desktopFile( m_destURL.path() );
        if ( desktopFile.hasApplicationType() )
        {
            TQString error;
            TQStringList stringList;
            KURL::List lst = m_info->lst;
            KURL::List::Iterator it = lst.begin();
            for ( ; it != lst.end() ; it++ )
            {
                stringList.append((*it).url());
            }
            if ( TDEApplication::startServiceByDesktopPath( m_destURL.path(), stringList, &error ) > 0 )
                KMessageBox::error( 0L, error );
        }
        else
        {
            // Device or Link -> adjust dest
            if ( desktopFile.hasDeviceType() && desktopFile.hasKey("MountPoint") ) {
                TQString point = desktopFile.readEntry( "MountPoint" );
                m_destURL.setPath( point );
                TQString dev = desktopFile.readDevice();
                TQString mp = TDEIO::findDeviceMountPoint( dev );
                // Is the device already mounted ?
                if ( !mp.isNull() )
                    doFileCopy();
                else
                {
                    bool ro = desktopFile.readBoolEntry( "ReadOnly", false );
                    TQString fstype = desktopFile.readEntry( "FSType" );
                    KAutoMount* am = new KAutoMount( ro, fstype, dev, point, m_destURL.path(), false );
                    connect( am, TQT_SIGNAL( finished() ), this, TQT_SLOT( doFileCopy() ) );
                }
                return;
            }
            else if ( desktopFile.hasLinkType() && desktopFile.hasKey("URL") ) {
                m_destURL = desktopFile.readPathEntry("URL");
                doFileCopy();
                return;
            }
            // else, well: mimetype, service, servicetype or .directory. Can't really drop anything on those.
        }
    }
    else
    {
        // Should be a local executable
        // (If this fails, there is a bug in KFileItem::acceptsDrops)
        kdDebug(1203) << "KonqOperations::doDrop " << m_destURL.path() << "should be an executable" << endl;
        Q_ASSERT ( access( TQFile::encodeName(m_destURL.path()), X_OK ) == 0 );
        TDEProcess proc;
        proc << m_destURL.path() ;
        // Launch executable for each of the files
        KURL::List lst = m_info->lst;
        KURL::List::Iterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
            proc << (*it).path(); // assume local files
        kdDebug(1203) << "starting " << m_destURL.path() << " with " << lst.count() << " arguments" << endl;
        proc.start( TDEProcess::DontCare );
    }
    delete this;
}

void KonqOperations::doFileCopy()
{
    assert(m_info); // setDropInfo - and asyncDrop - should have been called before asyncDrop
    KURL::List lst = m_info->lst;
    TQDropEvent::Action action = m_info->action;
    bool isDesktopFile = false;
    bool itemIsOnDesktop = false;
    bool allItemsAreFromTrash = true;
    KURL::List mlst; // list of items that can be moved
    for (KURL::List::ConstIterator it = lst.begin(); it != lst.end(); ++it)
    {
        bool local = (*it).isLocalFile();
        if ( KProtocolInfo::supportsDeleting( *it ) && (!local || TQFileInfo((*it).directory()).isWritable() ))
            mlst.append(*it);
        if ( local && KDesktopFile::isDesktopFile((*it).path()))
            isDesktopFile = true;
        if ( local && (*it).path().startsWith(TDEGlobalSettings::desktopPath()))
            itemIsOnDesktop = true;
        if ( local || (*it).protocol() != "trash" )
            allItemsAreFromTrash = false;
    }

    bool linkOnly = false;
    if (isDesktopFile && !kapp->authorize("run_desktop_files") &&
        (m_destURL.path(1) == TDEGlobalSettings::desktopPath()) )
    {
       linkOnly = true;
    }

    if ( !mlst.isEmpty() && m_destURL.protocol() == "trash" )
    {
        if ( itemIsOnDesktop && !kapp->authorize("editable_desktop_icons") )
        {
            delete this;
            return;
        }

        m_method = TRASH;
        if ( askDeleteConfirmation( mlst, TRASH, DEFAULT_CONFIRMATION, parentWidget() ) )
            action = TQDropEvent::Move;
        else
        {
            delete this;
            return;
        }
    }
    else if ( allItemsAreFromTrash || m_destURL.protocol() == "trash" ) {
        // No point in asking copy/move/link when using dnd from or to the trash.
        action = TQDropEvent::Move;
    }
    else if ( (((m_info->keybstate & ControlMask) == 0) && ((m_info->keybstate & ShiftMask) == 0)) ||
              linkOnly )
    {
        // Neither control nor shift are pressed => show popup menu
        KonqIconViewWidget *iconView = tqt_dynamic_cast<KonqIconViewWidget*>(parent());
        bool bSetWallpaper = false;
        if ( iconView && iconView->maySetWallpaper() && lst.count() == 1 )
	{
            KURL url = lst.first();
            KMimeType::Ptr mime = KMimeType::findByURL( url );
            if ( ( !KImageIO::type(url.path()).isEmpty() ) ||
                 ( KImageIO::isSupported(mime->name(), KImageIO::Reading) ) ||
                 mime->is( "image/svg+xml" ) )
            {
                bSetWallpaper = true;
            }
        }

        // Check what the source can do
        KURL url = lst.first(); // we'll assume it's the same for all URLs (hack)
        bool sReading = KProtocolInfo::supportsReading( url );
        bool sDeleting = KProtocolInfo::supportsDeleting( url );
        bool sMoving = KProtocolInfo::supportsMoving( url );
        // Check what the destination can do
        bool dWriting = KProtocolInfo::supportsWriting( m_destURL );
        if ( !dWriting )
        {
            delete this;
            return;
        }

        TQPopupMenu popup;
        if (!mlst.isEmpty() && (sMoving || (sReading && sDeleting)) && !linkOnly )
            popup.insertItem(SmallIconSet("goto"), i18n( "&Move Here" ) + "\t" + KKey::modFlagLabel( KKey::SHIFT ), 2 );
        if ( sReading && !linkOnly)
            popup.insertItem(SmallIconSet("edit-copy"), i18n( "&Copy Here" ) + "\t" + KKey::modFlagLabel( KKey::CTRL ), 1 );
        popup.insertItem(SmallIconSet("www"), i18n( "&Link Here" ) + "\t" + KKey::modFlagLabel( (KKey::ModFlag)( KKey::CTRL|KKey::SHIFT ) ), 3 );
        if (bSetWallpaper)
            popup.insertItem(SmallIconSet("background"), i18n( "Set as &Wallpaper" ), 4 );
        popup.insertSeparator();
        popup.insertItem(SmallIconSet("cancel"), i18n( "C&ancel" ) + "\t" + KKey( Qt::Key_Escape ).toString(), 5);

        int result = popup.exec( m_info->mousePos );

        switch (result) {
        case 1 : action = TQDropEvent::Copy; break;
        case 2 : action = TQDropEvent::Move; break;
        case 3 : action = TQDropEvent::Link; break;
        case 4 :
        {
            kdDebug(1203) << "setWallpaper iconView=" << iconView << " url=" << lst.first().url() << endl;
            if (iconView && iconView->isDesktop() ) iconView->setWallpaper(lst.first());
            delete this;
            return;
        }
        case 5 :
        default : delete this; return;
        }
    }

    TDEIO::Job * job = 0;
    switch ( action ) {
    case TQDropEvent::Move :
        job = TDEIO::move( lst, m_destURL );
        job->setMetaData( m_info->metaData );
        setOperation( job, m_method == TRASH ? TRASH : MOVE, lst, m_destURL );
        (void) new KonqCommandRecorder(
            m_method == TRASH ? KonqCommand::TRASH : KonqCommand::MOVE,
            lst, m_destURL, job );
        return; // we still have stuff to do -> don't delete ourselves
    case TQDropEvent::Copy :
        job = TDEIO::copy( lst, m_destURL );
        job->setMetaData( m_info->metaData );
        setOperation( job, COPY, lst, m_destURL );
        (void) new KonqCommandRecorder( KonqCommand::COPY, lst, m_destURL, job );
        return;
    case TQDropEvent::Link :
        kdDebug(1203) << "KonqOperations::asyncDrop lst.count=" << lst.count() << endl;
        job = TDEIO::link( lst, m_destURL );
        job->setMetaData( m_info->metaData );
        setOperation( job, LINK, lst, m_destURL );
        (void) new KonqCommandRecorder( KonqCommand::LINK, lst, m_destURL, job );
        return;
    default : kdError(1203) << "Unknown action " << (int)action << endl;
    }
    delete this;
}

void KonqOperations::rename( TQWidget * parent, const KURL & oldurl, const KURL& newurl )
{
    kdDebug(1203) << "KonqOperations::rename oldurl=" << oldurl << " newurl=" << newurl << endl;
    if ( oldurl == newurl )
        return;

    KURL::List lst;
    lst.append(oldurl);
    TDEIO::Job * job = TDEIO::moveAs( oldurl, newurl, !oldurl.isLocalFile() );
    KonqOperations * op = new KonqOperations( parent );
    op->setOperation( job, MOVE, lst, newurl );
    (void) new KonqCommandRecorder( KonqCommand::MOVE, lst, newurl, job );
    // if moving the desktop then update config file and emit
    if ( oldurl.isLocalFile() && oldurl.path(1) == TDEGlobalSettings::desktopPath() )
    {
        kdDebug(1203) << "That rename was the Desktop path, updating config files" << endl;
        TDEConfig *globalConfig = TDEGlobal::config();
        TDEConfigGroupSaver cgs( globalConfig, "Paths" );
        globalConfig->writePathEntry("Desktop" , newurl.path(), true, true );
        globalConfig->sync();
        KIPC::sendMessageAll(KIPC::SettingsChanged, TDEApplication::SETTINGS_PATHS);
    }
}

void KonqOperations::setOperation( TDEIO::Job * job, int method, const KURL::List & /*src*/, const KURL & dest )
{
    m_method = method;
    //m_srcURLs = src;
    m_destURL = dest;
    if ( job )
    {
        connect( job, TQT_SIGNAL( result( TDEIO::Job * ) ),
                 TQT_SLOT( slotResult( TDEIO::Job * ) ) );
        TDEIO::CopyJob *copyJob = tqt_dynamic_cast<TDEIO::CopyJob*>(job);
        KonqIconViewWidget *iconView = tqt_dynamic_cast<KonqIconViewWidget*>(parent());
        if (copyJob && iconView)
        {
            connect(copyJob, TQT_SIGNAL(aboutToCreate(TDEIO::Job *,const TQValueList<TDEIO::CopyInfo> &)),
                 this, TQT_SLOT(slotAboutToCreate(TDEIO::Job *,const TQValueList<TDEIO::CopyInfo> &)));
            connect(this, TQT_SIGNAL(aboutToCreate(const TQPoint &, const TQValueList<TDEIO::CopyInfo> &)),
                 iconView, TQT_SLOT(slotAboutToCreate(const TQPoint &, const TQValueList<TDEIO::CopyInfo> &)));
        }
    }
    else // for link
        slotResult( 0L );
}

void KonqOperations::slotAboutToCreate(TDEIO::Job *, const TQValueList<TDEIO::CopyInfo> &files)
{
    emit aboutToCreate( m_info ? m_info->mousePos : m_pasteInfo ? m_pasteInfo->mousePos : TQPoint(), files);
}

void KonqOperations::statURL( const KURL & url, const TQObject *receiver, const char *member )
{
    KonqOperations * op = new KonqOperations( 0L );
    op->_statURL( url, receiver, member );
    op->m_method = STAT;
}

void KonqOperations::_statURL( const KURL & url, const TQObject *receiver, const char *member )
{
    connect( this, TQT_SIGNAL( statFinished( const KFileItem * ) ), receiver, member );
    TDEIO::StatJob * job = TDEIO::stat( url /*, false?*/ );
    connect( job, TQT_SIGNAL( result( TDEIO::Job * ) ),
             TQT_SLOT( slotStatResult( TDEIO::Job * ) ) );
}

void KonqOperations::slotStatResult( TDEIO::Job * job )
{
    if ( job->error())
        job->showErrorDialog( (TQWidget*)parent() );
    else
    {
        TDEIO::StatJob * statJob = static_cast<TDEIO::StatJob*>(job);
        KFileItem * item = new KFileItem( statJob->statResult(), statJob->url() );
        emit statFinished( item );
        delete item;
    }
    // If we're only here for a stat, we're done. But not if we used _statURL internally
    if ( m_method == STAT )
        delete this;
}

void KonqOperations::slotResult( TDEIO::Job * job )
{
    if (job && job->error())
        job->showErrorDialog( (TQWidget*)parent() );
    if ( m_method == EMPTYTRASH ) {
        // Update konq windows opened on trash:/
        KDirNotify_stub allDirNotify("*", "KDirNotify*");
        allDirNotify.FilesAdded( "trash:/" ); // yeah, files were removed, but we don't know which ones...
    }
    delete this;
}

void KonqOperations::rename( TQWidget * parent, const KURL & oldurl, const TQString & name )
{
    KURL newurl( oldurl );
    newurl.setPath( oldurl.directory(false, true) + name );
    kdDebug(1203) << "KonqOperations::rename("<<name<<") called. newurl=" << newurl << endl;
    rename( parent, oldurl, newurl );
}

void KonqOperations::newDir( TQWidget * parent, const KURL & baseURL )
{
    bool ok;
    TQString name = i18n( "New Folder" );
    if ( baseURL.isLocalFile() && TQFileInfo( baseURL.path(+1) + name ).exists() )
        name = TDEIO::RenameDlg::suggestName( baseURL, i18n( "New Folder" ) );

    name = KInputDialog::getText ( i18n( "New Folder" ),
        i18n( "Enter folder name:" ), name, &ok, parent );
    if ( ok && !name.isEmpty() )
    {
        KURL url;
        if ((name[0] == '/') || (name[0] == '~'))
        {
           url.setPath(KShell::tildeExpand(name));
        }
        else
        {
           name = TDEIO::encodeFileName( name );
           url = baseURL;
           url.addPath( name );
        }
        KonqOperations::mkdir( 0L, url );
    }
}

////

KonqMultiRestoreJob::KonqMultiRestoreJob( const KURL::List& urls, bool showProgressInfo )
    : TDEIO::Job( showProgressInfo ),
      m_urls( urls ), m_urlsIterator( m_urls.begin() ),
      m_progress( 0 )
{
  TQTimer::singleShot(0, this, TQT_SLOT(slotStart()));
}

void KonqMultiRestoreJob::slotStart()
{
    // Well, it's not a total in bytes, so this would look weird
    //if ( m_urlsIterator == m_urls.begin() ) // first time: emit total
    //    emit totalSize( m_urls.count() );

    if ( m_urlsIterator != m_urls.end() )
    {
        const KURL& url = *m_urlsIterator;

        KURL new_url = url;
        if ( new_url.protocol()=="system"
          && new_url.path().startsWith("/trash") )
        {
            TQString path = new_url.path();
	    path.remove(0, 6);
	    new_url.setProtocol("trash");
	    new_url.setPath(path);
        }

        Q_ASSERT( new_url.protocol() == "trash" );
        TQByteArray packedArgs;
        TQDataStream stream( packedArgs, IO_WriteOnly );
        stream << (int)3 << new_url;
        TDEIO::Job* job = TDEIO::special( new_url, packedArgs );
        addSubjob( job );
    }
    else // done!
    {
        KDirNotify_stub allDirNotify("*", "KDirNotify*");
        allDirNotify.FilesRemoved( m_urls );
        emitResult();
    }
}

void KonqMultiRestoreJob::slotResult( TDEIO::Job *job )
{
    if ( job->error() )
    {
        TDEIO::Job::slotResult( job ); // will set the error and emit result(this)
        return;
    }
    subjobs.remove( job );
    // Move on to next one
    ++m_urlsIterator;
    ++m_progress;
    //emit processedSize( this, m_progress );
    emitPercent( m_progress, m_urls.count() );
    slotStart();
}

TQWidget* KonqOperations::parentWidget() const
{
    return static_cast<TQWidget *>( parent() );
}

#include "konq_operations.moc"
