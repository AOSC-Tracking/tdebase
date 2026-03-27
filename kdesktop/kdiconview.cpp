/* This file is part of the TDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000, 2001 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kdiconview.h"
#include "krootwm.h"
#include "desktop.h"
#include "kdesktopsettings.h"

#include <tdeio/paste.h>
#include <tdeaccel.h>
#include <tdeapplication.h>
#include <kcolordrag.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirlister.h>
#include <tdeglobalsettings.h>
#include <kpropertiesdialog.h>
#include <tdelocale.h>
#include <konqbookmarkmanager.h>
#include <konq_defaults.h>
#include <konq_drag.h>
#include <konq_operations.h>
#include <konq_popupmenu.h>
#include <konq_settings.h>
#include <konq_undo.h>
#include <kivfreespaceoverlay.h>
#include <kprotocolinfo.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kurldrag.h>
#include <twin.h>
#include <twinmodule.h>

#include <fixx11h.h>

#include <tqclipboard.h>
#include <tqdir.h>
#include <tqevent.h>
#include <tqregexp.h>

#include <unistd.h>

#include "kshadowengine.h"
#include "kdesktopshadowsettings.h"
#include "tdefileividesktop.h"

// for multihead
extern int kdesktop_screen_number;

// -----------------------------------------------------------------------------

TQRect KDIconView::desktopRect()
{
    return ( kdesktop_screen_number == 0 )
            ? TQApplication::desktop()->geometry() // simple case, or xinerama
            : TQApplication::desktop()->screenGeometry( kdesktop_screen_number ); // multi-head
}

// -----------------------------------------------------------------------------

void KDIconView::saveIconPosition(KSimpleConfig *config, int x, int y)
{
  // save the icon position in absolute coordinates
  config->writeEntry("Xabs", x);
  config->writeEntry("Yabs", y);

  // save also mentioning desktop size
  TQRect desk = desktopRect();
  TQString sizeStr = TQString( "_%1x%2" ).arg(desk.width()).arg(desk.height());

  config->writeEntry("Xabs" + sizeStr, x);
  config->writeEntry("Yabs" + sizeStr, y);
}

// -----------------------------------------------------------------------------

void KDIconView::readIconPosition(KSimpleConfig *config, int &x, int &y)
{
  // check if we have the position for the current desktop size
  TQRect desk = desktopRect();
  TQString sizeStr = TQString( "_%1x%2" ).arg(desk.width()).arg(desk.height());

  x = config->readNumEntry("Xabs" + sizeStr, -99999);

  if ( x != -99999 )
    y = config->readNumEntry("Yabs" + sizeStr);
  else
  {
    // not found; use the resolution independent position
    x = config->readNumEntry("Xabs", -99999);

    if ( x != -99999 )
      y = config->readNumEntry("Yabs");
    else  // for compatibility, read the old iconArea-relative-position
    {
      // problem here: when reading a position before we know the correct
      // desktopIconsArea, the relative position do not make sense
      // workaround: use desktopRect() as the allowed size

      TQRect desk = desktopRect();
      TQString X_w = TQString("X %1").arg(desk.width() );
      TQString Y_h = TQString("Y %1").arg(desk.height());

      x = config->readNumEntry(X_w, -99999);
      if ( x != -99999 ) x = config->readNumEntry("X");
      if ( x < 0 ) x += desk.width();

      y = config->readNumEntry(Y_h, -99999);
      if ( y != -99999 ) y = config->readNumEntry("Y");
      if ( y < 0 ) y += desk.height();
    }
  }
}

// -----------------------------------------------------------------------------

KDIconView::KDIconView( TQWidget *parent, const char* name )
    : KonqIconViewWidget( parent, name, (WFlags)WResizeNoErase, true ),
      m_actionCollection( this, "KDIconView::m_actionCollection" ),
      m_accel( 0L ),
      m_bNeedRepaint( false ),
      m_bNeedSave( false ),
      m_autoAlign( false ),
      m_hasExistingPos( false ),
      m_bEditableDesktopIcons( kapp->authorize("editable_desktop_icons") ),
      m_bShowDot( false ),
      m_bVertAlign( true ),
      m_dirLister( 0L ),
      m_mergeDirs(),
      m_dotDirectory( 0L ),
      m_lastDeletedIconPos(),
      m_eSortCriterion( NameCaseInsensitive ),
      m_bSortDirectoriesFirst( true ),
      m_itemsAlwaysFirst(),
      m_enableMedia(false),
      m_gotIconsArea(false),
      m_needDesktopAlign(true),
      m_paOutstandingFreeSpaceOverlaysTimer( 0L )
{
    setResizeMode( Fixed );
    setIconArea( desktopRect() );  // the default is the whole desktop

    // Initialise the shadow data objects...
    m_shadowEngine = new KShadowEngine(new KDesktopShadowSettings(TDEGlobal::config()));

    // Initialize media handler
    mMediaListView = new TQListView();

    connect( TQApplication::clipboard(), TQ_SIGNAL(dataChanged()), this, TQ_SLOT(slotClipboardDataChanged()) );

    setURL( desktopURL() ); // sets m_url

    m_desktopDirs = TDEGlobal::dirs()->findDirs( "appdata", "Desktop" );
    initDotDirectories();

    connect( this, TQ_SIGNAL( executed( TQIconViewItem * ) ),
             TQ_SLOT( slotExecuted( TQIconViewItem * ) ) );
    connect( this, TQ_SIGNAL( returnPressed( TQIconViewItem * ) ),
             TQ_SLOT( slotReturnPressed( TQIconViewItem * ) ) );
    connect( this, TQ_SIGNAL( mouseButtonPressed(int, TQIconViewItem*, const TQPoint&)),
             TQ_SLOT( slotMouseButtonPressed(int, TQIconViewItem*, const TQPoint&)) );
    connect( this, TQ_SIGNAL( mouseButtonClicked(int, TQIconViewItem*, const TQPoint&)),
             TQ_SLOT( slotMouseButtonClickedKDesktop(int, TQIconViewItem*, const TQPoint&)) );
    connect( this, TQ_SIGNAL( contextMenuRequested(TQIconViewItem*, const TQPoint&)),
             TQ_SLOT( slotContextMenuRequested(TQIconViewItem*, const TQPoint&)) );

    connect( this, TQ_SIGNAL( enableAction( const char * , bool ) ),
             TQ_SLOT( slotEnableAction( const char * , bool ) ) );

    // Hack: KonqIconViewWidget::slotItemRenamed is not virtual :-(
    disconnect( this, TQ_SIGNAL(itemRenamed(TQIconViewItem *, const TQString &)),
             this, TQ_SLOT(slotItemRenamed(TQIconViewItem *, const TQString &)) );
    connect( this, TQ_SIGNAL(itemRenamed(TQIconViewItem *, const TQString &)),
             this, TQ_SLOT(slotItemRenamed(TQIconViewItem *, const TQString &)) );

    if (!m_bEditableDesktopIcons)
    {
      setIconsLocked(true);
    }
}

KDIconView::~KDIconView()
{
    if (m_dotDirectory && !m_bEditableDesktopIcons || m_iconsLocked) {
      m_dotDirectory->rollback(false); // Don't save positions
    }

    delete m_dotDirectory;
    delete m_dirLister;
    delete m_shadowEngine;
}

void KDIconView::initDotDirectories()
{
    TQStringList dirs = m_desktopDirs;
    KURL u = desktopURL();
    if (u.isLocalFile()) {
       dirs.prepend(u.path());
    }

    TQString prefix = iconPositionGroupPrefix();
    TQString dotFileName = locateLocal("appdata", "IconPositions");
    if (kdesktop_screen_number != 0) {
        dotFileName += "_Desktop" + TQString::number(kdesktop_screen_number);
    }

    if (m_dotDirectory && !m_bEditableDesktopIcons) {
        m_dotDirectory->rollback(false); // Don't save positions
    }

    delete m_dotDirectory;

    m_dotDirectory = new KSimpleConfig( dotFileName );
    // If we don't allow editable desktop icons, empty m_dotDirectory
    if (!m_bEditableDesktopIcons)
    {
        TQStringList groups = m_dotDirectory->groupList();
        TQStringList::ConstIterator gIt = groups.begin();
        TQStringList::ConstIterator gEnd = groups.end();
        for (; gIt != gEnd; ++gIt )
        {
            m_dotDirectory->deleteGroup(*gIt, true);
        }
    }
    TQRect desk = desktopRect();
    TQString X_w = TQString( "X %1" ).arg( desk.width() );
    TQString Y_h = TQString( "Y %1" ).arg( desk.height() );
    for ( TQStringList::ConstIterator it = dirs.begin() ; it != dirs.end() ; ++it )
    {
        kdDebug(1204) << "KDIconView::initDotDirectories found dir " << *it << endl;
        TQString localDotFileName = *it + "/.directory";

        if (TQFile::exists(localDotFileName))
        {
           KSimpleConfig dotDir(localDotFileName, true); // Read only

           TQStringList groups = dotDir.groupList();
           TQStringList::ConstIterator gIt = groups.begin();
           TQStringList::ConstIterator gEnd = groups.end();
           for (; gIt != gEnd; ++gIt )
           {
              if ( (*gIt).startsWith(prefix) )
              {
                 dotDir.setGroup( *gIt );
                 m_dotDirectory->setGroup( *gIt );

                 if (!m_dotDirectory->hasKey( X_w ))
                 {
                    int x,y;
                    readIconPosition(&dotDir, x, y);
                    m_dotDirectory->writeEntry( X_w, x );
                    m_dotDirectory->writeEntry( Y_h, y ); // Not persistent!
                 }
              }
           }
        }
    }
}

void KDIconView::initConfig( bool init )
{
    //kdDebug() << "initConfig " << init << endl;

    if ( !init ) {
        KonqFMSettings::reparseConfiguration();
        KDesktopSettings::self()->readConfig();
    }

    TDEConfig * config = TDEGlobal::config();

    if ( !init ) {
      KDesktopShadowSettings *shadowSettings = static_cast<KDesktopShadowSettings *>(m_shadowEngine->shadowSettings());
      shadowSettings->setConfig(config);
    }

    setMaySetWallpaper(!config->isImmutable() && !TDEGlobal::dirs()->isRestrictedResource("wallpaper"));
    m_bShowDot = KDesktopSettings::showHidden();
    m_bVertAlign = KDesktopSettings::vertAlign();
    TQStringList oldPreview = previewSettings();
    setPreviewSettings( KDesktopSettings::preview() );
    setSpacing( KDesktopSettings::iconSpacing() );

    // read arrange configuration
    m_eSortCriterion  = (SortCriterion)KDesktopSettings::sortCriterion();
    m_bSortDirectoriesFirst = KDesktopSettings::directoriesFirst();
    m_itemsAlwaysFirst = KDesktopSettings::alwaysFirstItems(); // Distributor plug-in

    if (KProtocolInfo::isKnownProtocol(TQString::fromLatin1("media"))) {
        m_enableMedia=KDesktopSettings::mediaEnabled();
    }
    else {
        m_enableMedia=false;
    }
    TQString tmpList=KDesktopSettings::exclude();
    kdDebug(1204)<<"m_excludeList"<<tmpList<<endl;
    m_excludedMedia=TQStringList::split(",",tmpList,false);
    kdDebug(1204)<<" m_excludeList / item count:" <<m_excludedMedia.count()<<endl;
    if ( m_dirLister ) // only when called while running - not on first startup
    {
        configureMedia();
        m_dirLister->setShowingDotFiles( m_bShowDot );
        m_dirLister->emitChanges();
    }
    slotFreeSpaceOverlaySettingChanged();

    setArrangement(m_bVertAlign ? TopToBottom : LeftToRight);

    if ( KonqIconViewWidget::initConfig( init ) )
        lineupIcons(); // called if the font changed.

    setAutoArrange( false );

    if ( previewSettings().count() )
    {
        for ( TQStringList::ConstIterator it = oldPreview.begin(); it != oldPreview.end(); ++it)
            if ( !previewSettings().contains( *it ) ){
                kdDebug(1204) << "Disabling preview for " << *it << endl;
                if ( *it == "audio/" )
                    disableSoundPreviews();
                else
                {
                    KService::Ptr serv = KService::serviceByDesktopName( *it );
                    Q_ASSERT( serv != 0L );
                    if ( serv )
                    {
                        setIcons( iconSize( ), serv->property("MimeTypes").toStringList() /* revert no-longer wanted previews to icons */ );
                    }
                }
            }
        startImagePreview( TQStringList(), true );
    }
    else
    {
        stopImagePreview();
        setIcons( iconSize(), "*" /* stopImagePreview */ );
    }

    if ( !init )
        updateContents();
}

void KDIconView::start()
{
    // We can only start once
    Q_ASSERT(!m_dirLister);
    if (m_dirLister) {
        return;
    }

    kdDebug(1204) << "KDIconView::start" << endl;

    // Create the directory lister
    m_dirLister = new KDirLister();

    m_bNeedSave = false;

    connect( m_dirLister, TQ_SIGNAL( clear() ),                               this, TQ_SLOT( slotClear() ) );
    connect( m_dirLister, TQ_SIGNAL( started(const KURL&) ),                  this, TQ_SLOT( slotStarted(const KURL&) ) );
    connect( m_dirLister, TQ_SIGNAL( completed() ),                           this, TQ_SLOT( slotCompleted() ) );
    connect( m_dirLister, TQ_SIGNAL( newItems( const KFileItemList & ) ),     this, TQ_SLOT( slotNewItems( const KFileItemList & ) ) );
    connect( m_dirLister, TQ_SIGNAL( deleteItem( KFileItem * ) ),             this, TQ_SLOT( slotDeleteItem( KFileItem * ) ) );
    connect( m_dirLister, TQ_SIGNAL( refreshItems( const KFileItemList & ) ), this, TQ_SLOT( slotRefreshItems( const KFileItemList & ) ) );

    // Start the directory lister !
    m_dirLister->setShowingDotFiles( m_bShowDot );
    kapp->allowURLAction("list", KURL(), url());
    startDirLister();
    createActions();
}

void KDIconView::configureMedia()
{
    kdDebug(1204) << "***********KDIconView::configureMedia() " <<endl;
    m_dirLister->setMimeExcludeFilter(m_excludedMedia);
    m_dirLister->emitChanges();
    updateContents();
    if (m_enableMedia) {
        for (KURL::List::Iterator it1=m_mergeDirs.begin();it1!=m_mergeDirs.end();++it1) {
            if ((*it1).url()=="media:/") {
                return;
            }
        }
        m_mergeDirs.append(KURL("media:/"));
        m_dirLister->openURL(KURL("media:/"), true);
    }
    else {
        for (KURL::List::Iterator it2=m_mergeDirs.begin();it2!=m_mergeDirs.end();++it2) {
            if ((*it2).url()=="media:/") {
                delete m_dirLister;
                m_dirLister=0;
                start();
                if (m_mergeDirs.contains(*it2)) {
                    m_mergeDirs.remove(*it2);
                    m_dirLister->stop("media");
                }
                return;
            }
        }
        return;
    }
}

void KDIconView::createActions()
{
    if (m_bEditableDesktopIcons)
    {
        TDEAction *undo = KStdAction::undo( KonqUndoManager::self(), TQ_SLOT( undo() ), &m_actionCollection, "undo" );
        connect( KonqUndoManager::self(), TQ_SIGNAL( undoAvailable( bool ) ),
             undo, TQ_SLOT( setEnabled( bool ) ) );
        connect( KonqUndoManager::self(), TQ_SIGNAL( undoTextChanged( const TQString & ) ),
             undo, TQ_SLOT( setText( const TQString & ) ) );
        undo->setEnabled( KonqUndoManager::self()->undoAvailable() );

        TDEAction* paCut = KStdAction::cut( this, TQ_SLOT( slotCut() ), &m_actionCollection, "cut" );
        TDEShortcut cutShortCut = paCut->shortcut();
        cutShortCut.remove( KKey( SHIFT + Key_Delete ) ); // used for deleting files
        paCut->setShortcut( cutShortCut );

        KStdAction::copy( this, TQ_SLOT( slotCopy() ), &m_actionCollection, "copy" );
        KStdAction::paste( this, TQ_SLOT( slotPaste() ), &m_actionCollection, "paste" );
        TDEAction *pasteTo = KStdAction::paste( this, TQ_SLOT( slotPopupPasteTo() ), &m_actionCollection, "pasteto" );
        pasteTo->setEnabled( false ); // only enabled during popupMenu()

        TDEShortcut reloadShortcut = TDEStdAccel::shortcut(TDEStdAccel::Reload);
        new TDEAction( i18n( "&Reload" ), "reload", reloadShortcut, this, TQ_SLOT( refreshIcons() ), &m_actionCollection, "reload" );

        (void) new TDEAction( i18n( "&Rename" ), /*"editrename",*/ Key_F2, this, TQ_SLOT( renameSelectedItem() ), &m_actionCollection, "rename" );
        (void) new TDEAction( i18n( "&Properties" ), ALT+Key_Return, this, TQ_SLOT( slotProperties() ), &m_actionCollection, "properties" );
        TDEAction* trash = new TDEAction( i18n( "&Move to Trash" ), "edittrash", Key_Delete, &m_actionCollection, "trash" );
        connect( trash, TQ_SIGNAL( activated( TDEAction::ActivationReason, TQt::ButtonState ) ),
                 this, TQ_SLOT( slotTrashActivated( TDEAction::ActivationReason, TQt::ButtonState ) ) );

        TDEConfig config("kdeglobals", true, false);
        config.setGroup( "KDE" );
        (void) new TDEAction( i18n( "&Delete" ), "edit-delete", SHIFT+Key_Delete, this, TQ_SLOT( slotDelete() ), &m_actionCollection, "del" );

        // Initial state of the actions (cut/copy/paste/...)
        slotSelectionChanged();
        //init paste action
        slotClipboardDataChanged();
    }
}

void KDIconView::rearrangeIcons( SortCriterion sc, bool bSortDirectoriesFirst )
{
    m_eSortCriterion = sc;
    m_bSortDirectoriesFirst = bSortDirectoriesFirst;
    rearrangeIcons();
}

void KDIconView::rearrangeIcons()
{
    setupSortKeys();
    sort();  // calls arrangeItemsInGrid() which does not honor iconArea()

    if ( m_autoAlign )
        lineupIcons(  m_bVertAlign ? TQIconView::TopToBottom : TQIconView::LeftToRight );  // also saves position
    else
    {
        KonqIconViewWidget::lineupIcons(m_bVertAlign ? TQIconView::TopToBottom : TQIconView::LeftToRight);
        saveIconPositions();
    }
}

void KDIconView::lineupIcons()
{
    if ( !m_gotIconsArea ) return;
    KonqIconViewWidget::lineupIcons();
    saveIconPositions();
}

void KDIconView::incIconSpacing()
{
    if ( m_autoAlign && !KDesktopSettings::lockIcons() && KDesktopSettings::spacingCtrlScroll() )
    {
        setSpacing( ( spacing() + 1 ) );
        lineupIcons();

        KDesktopSettings::setIconSpacing( spacing() );
        KDesktopSettings::writeConfig();
    }
}

void KDIconView::decIconSpacing()
{
    if ( m_autoAlign && !KDesktopSettings::lockIcons() && KDesktopSettings::spacingCtrlScroll() && spacing() > 5 )
    {
        setSpacing( ( spacing() - 1 ) );
        lineupIcons();

        KDesktopSettings::setIconSpacing( spacing() );
        KDesktopSettings::writeConfig();
    }
}

void KDIconView::setAutoAlign( bool b )
{
    m_autoAlign = b;

    // Auto line-up icons
    if ( b ) {
        // set maxItemWidth to ensure sane initial icon layout before the auto align code is fully activated
        int sz = iconSize() ? iconSize() : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
        setMaxItemWidth( TQMAX( TQMAX( sz, previewIconSize( iconSize() ) ), KonqFMSettings::settings()->iconTextWidth() ) );
        setFont( font() );  // Force calcRect()

        if (!KRootWm::self()->startup) {
            lineupIcons();
        }
        else {
            KRootWm::self()->startup = false;
        }
        connect( this, TQ_SIGNAL( iconMoved() ),
                 this, TQ_SLOT( lineupIcons() ) );
    }
    else {
        // change maxItemWidth, because when grid-align was active, it changed this for the grid
        int sz = iconSize() ? iconSize() : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
        setMaxItemWidth( TQMAX( TQMAX( sz, previewIconSize( iconSize() ) ), KonqFMSettings::settings()->iconTextWidth() ) );
        setFont( font() );  // Force calcRect()

        disconnect( this, TQ_SIGNAL( iconMoved() ),
                    this, TQ_SLOT( lineupIcons() ) );
    }
}

void KDIconView::setIconsLocked( bool lock )
{
  m_iconsLocked = lock;

  setItemsMovable(!lock);
  setAcceptDrops(!lock);
  viewport()->setAcceptDrops(!lock);
}

void KDIconView::startDirLister()
{
    // if desktop is resized before start() is called (XRandr)
    if (!m_dirLister) return;

    m_dirLister->openURL( url() );

    // Gather the list of directories to merge into the desktop
    // (the main URL is desktopURL(), no need for it in the m_mergeDirs list)
    m_mergeDirs.clear();
    for ( TQStringList::ConstIterator it = m_desktopDirs.begin() ; it != m_desktopDirs.end() ; ++it )
    {
        kdDebug(1204) << "KDIconView::desktopResized found merge dir " << *it << endl;
        KURL u;
        u.setPath( *it );
        m_mergeDirs.append( u );
        // And start listing this dir right now
        kapp->allowURLAction("list", KURL(), u);
        m_dirLister->openURL( u, true );
    }
    configureMedia();
}

void KDIconView::lineupIcons(TQIconView::Arrangement align)
{
    m_bVertAlign = ( align == TQIconView::TopToBottom );
    setArrangement( m_bVertAlign ? TopToBottom : LeftToRight );

    if ( m_autoAlign )
    {
        KonqIconViewWidget::lineupIcons( align );
        saveIconPositions();
    }
    else
        rearrangeIcons();  // also saves the position

    KDesktopSettings::setVertAlign( m_bVertAlign );
    KDesktopSettings::writeConfig();
}

// Only used for DCOP
TQStringList KDIconView::selectedURLs()
{
    TQStringList seq;

    TQIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() )
        if ( it->isSelected() ) {
            KFileItem *fItem = ((KFileIVI *)it)->item();
            seq.append( fItem->url().url() ); // copy the URL
        }

    return seq;
}

void KDIconView::recheckDesktopURL()
{
    // Did someone change the path to the desktop ?
    kdDebug(1204) << desktopURL().url() << endl;
    kdDebug(1204) << url().url() << endl;
    if ( desktopURL() != url() )
    {
        kdDebug(1204) << "Desktop path changed from " << url().url() <<
            " to " << desktopURL().url() << endl;
        setURL( desktopURL() ); // sets m_url
        initDotDirectories();
        m_dirLister->openURL( url() );
    }
}

KURL KDIconView::desktopURL()
{
    // Support both paths and URLs
    TQString desktopPath = TDEGlobalSettings::desktopPath();
    if (kdesktop_screen_number != 0) {
        TQString dn = "Desktop";
        dn += TQString::number(kdesktop_screen_number);
        desktopPath.replace("Desktop", dn);
    }

    KURL desktopURL;
    if (desktopPath[0] == '/')
        desktopURL.setPath(desktopPath);
    else
        desktopURL = desktopPath;

    Q_ASSERT( desktopURL.isValid() );
    if ( !desktopURL.isValid() ) { // should never happen
        KURL u;
        u.setPath(  TQDir::homeDirPath() + "/" + "Desktop" + "/" );
        return u;
    }

    return desktopURL;
}

void KDIconView::contentsMousePressEvent( TQMouseEvent *e )
{
    if (!m_dirLister) return;
    //kdDebug(1204) << "KDIconView::contentsMousePressEvent" << endl;
    // TQIconView, as of Qt 2.2, doesn't emit mouseButtonPressed for LMB on background
    if ( e->button() == TQt::LeftButton && KRootWm::self()->hasLeftButtonMenu() )
    {
        TQIconViewItem *item = findItem( e->pos() );
        if ( !item )
        {
            // Left click menu
            KRootWm::self()->mousePressed( e->globalPos(), e->button() );
            return;
        }
    }
    KonqIconViewWidget::contentsMousePressEvent( e );
}

void KDIconView::mousePressEvent( TQMouseEvent *e )
{
    KRootWm::self()->mousePressed( e->globalPos(), e->button() );
}

void KDIconView::wheelEvent( TQWheelEvent* e )
{
    if (!m_dirLister) return;
    //kdDebug(1204) << "KDIconView::wheelEvent" << endl;

    TQIconViewItem *item = findItem( e->pos() );
    if ( !item )
    {
      TQWheelEvent *we = static_cast<TQWheelEvent*>(e);

      if ( we->state() == ControlButton )
      {
        if ( we->delta() >= 0 )
          incIconSpacing();
        else
          decIconSpacing();

        we->accept();
      }
      else
      {
        emit wheelRolled( e->delta() );
      }

      return;
    }

    KonqIconViewWidget::wheelEvent( e );
}

void KDIconView::slotProperties()
{
    KFileItemList selectedFiles = selectedFileItems();

    if( selectedFiles.isEmpty() )
      return;

    (void) new KPropertiesDialog( selectedFiles );
}

void KDIconView::slotContextMenuRequested(TQIconViewItem *_item, const TQPoint& _global)
{
    if (_item)
    {
      ((KFileIVI*)_item)->setSelected( true );
      popupMenu( _global, selectedFileItems() );
    }
}

void KDIconView::slotMouseButtonPressed(int _button, TQIconViewItem* _item, const TQPoint& _global)
{
    //kdDebug(1204) << "KDIconView::slotMouseButtonPressed" << endl;
    if (!m_dirLister) return;
    m_lastDeletedIconPos = TQPoint(); // user action -> not renaming an icon
    if(!_item) {
        KRootWm::self()->mousePressed( _global, _button );
    }
}

void KDIconView::slotMouseButtonClickedKDesktop(int _button, TQIconViewItem* _item, const TQPoint&)
{
    if (!m_dirLister) return;
    //kdDebug(1204) << "KDIconView::slotMouseButtonClickedKDesktop" << endl;
    if ( _item && _button == TQt::MidButton ) {
        slotExecuted(_item);
    }
}

// -----------------------------------------------------------------------------

void KDIconView::slotReturnPressed( TQIconViewItem *item )
{
    if (item && item->isSelected()) {
        slotExecuted(item);
    }
}

// -----------------------------------------------------------------------------

void KDIconView::slotExecuted( TQIconViewItem *item )
{
    kapp->propagateSessionManager();
    m_lastDeletedIconPos = TQPoint(); // user action -> not renaming an icon
    if (item) {
        visualActivate(item);
        ((KFileIVI*)item)->returnPressed();
    }
}

// -----------------------------------------------------------------------------

void KDIconView::slotCut()
{
    cutSelection();
}

// -----------------------------------------------------------------------------

void KDIconView::slotCopy()
{
    copySelection();
}

// -----------------------------------------------------------------------------

void KDIconView::slotPaste()
{
    KonqOperations::doPaste(this, url(), KRootWm::self()->desktopMenuPosition());
}

void KDIconView::slotPopupPasteTo()
{
    Q_ASSERT( !m_popupURL.isEmpty() );
    if ( !m_popupURL.isEmpty() )
        paste( m_popupURL );
}

// These two functions and the following class are all lifted from desktopbehavior_impl.cpp to handle the media icons
class DesktopBehaviorMediaItem : public TQCheckListItem
{
public:
    DesktopBehaviorMediaItem(TQListView *parent, const TQString name, const TQString mimetype, bool on)
        : TQCheckListItem(parent, name, CheckBox),
          m_mimeType(mimetype){setOn(on);}

    const TQString &mimeType() const { return m_mimeType; }

private:
    TQString m_mimeType;
};

void KDIconView::fillMediaListView()
{
    g_pConfig = new TDEConfig("kdesktoprc");
    mMediaListView->hide();
    mMediaListView->clear();
    KMimeType::List mimetypes = KMimeType::allMimeTypes();
    TQValueListIterator<KMimeType::Ptr> it2(mimetypes.begin());
    g_pConfig->setGroup( "Media" );
    TQString excludedMedia=g_pConfig->readEntry("exclude","media/hdd_mounted,media/hdd_unmounted,media/floppy_unmounted,media/cdrom_unmounted,media/floppy5_unmounted");
    for (; it2 != mimetypes.end(); ++it2) {
       if ( ((*it2)->name().startsWith("media/")) )
       {
           bool ok=excludedMedia.contains((*it2)->name())==0;
           new DesktopBehaviorMediaItem (mMediaListView, (*it2)->comment(), (*it2)->name(),ok);
       }
    }
    delete g_pConfig;
}

void KDIconView::saveMediaListView()
{
    g_pConfig = new TDEConfig("kdesktoprc");
    g_pConfig->setGroup( "Media" );
    TQStringList exclude;
    for (DesktopBehaviorMediaItem *it=static_cast<DesktopBehaviorMediaItem *>(mMediaListView->firstChild());
         it; it=static_cast<DesktopBehaviorMediaItem *>(it->nextSibling()))
    {
        if (!it->isOn()) exclude << it->mimeType();
    }
    g_pConfig->writeEntry("exclude",exclude);
    g_pConfig->sync();

    // Reload kdesktop configuration to apply changes
    TQByteArray data;
    int konq_screen_number = TDEApplication::desktop()->primaryScreen();
    TQCString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
    delete g_pConfig;
}

void KDIconView::removeBuiltinIcon(TQString iconName)
{
    DesktopBehaviorMediaItem *changeItem;
    fillMediaListView();
    changeItem = static_cast<DesktopBehaviorMediaItem *>(mMediaListView->findItem(iconName, 0));
    if (changeItem != 0) {
        changeItem->setOn(false);
    }
    saveMediaListView();
    KMessageBox::information(0, i18n("You have chosen to remove a system icon") + TQString(".\n") + i18n("You can restore this icon in the future through the") + TQString(" \"") + ("Device Icons") + TQString("\" ") + i18n("tab in the") + TQString(" \"") + i18n("Behavior") + TQString("\" ") + i18n("pane of the Desktop Settings control module."), "System Icon Removed", "sysiconremovedwarning");
}

/**
 * The files on the desktop come from a variety of sources.
 * If an attempt is made to delete a .desktop file that does
 * not originate from the users own Desktop directory then
 * a .desktop file with "Hidden=true" is written to the users
 * own Desktop directory to hide the file.
 *
 * Returns true if all selected items have been deleted.
 * Returns false if there are selected items remaining that
 * still need to be deleted in a regular fashion.
 */
bool KDIconView::deleteGlobalDesktopFiles()
{
    KURL desktop_URL = desktopURL();
    if (!desktop_URL.isLocalFile())
        return false; // Dunno how to do this.

    TQString desktopPath = desktop_URL.path();

    bool itemsLeft = false;
    TQIconViewItem *it = 0;
    TQIconViewItem *nextIt = firstItem();
    for (; (it = nextIt); )
    {
        nextIt = it->nextItem();
        if ( !it->isSelected() )
            continue;

        KFileItem *fItem = ((KFileIVI *)it)->item();
        if (fItem->url().path().startsWith(desktopPath))
        {
            itemsLeft = true;
            continue; // File is in users own Desktop directory
        }

        if (!isDesktopFile(fItem))
        {
            itemsLeft = true;
            continue; // Not a .desktop file
        }

        // Ignore these special files
        // Name                 URL                                     Type            OnlyShowIn
        // My Documents         XDG_DOCUMENTS_DIR       Application     TDE;
        // My Computer          media:/                                 Link            TDE;
        // My Network Places    remote:/                                Link            TDE;
        // Printers             [exec] kjobviewer --all --show %i %m    Application     TDE;
        // Trash                trash:/                                 Link            TDE;
        // Web Browser          kfmclient openBrowser %u                Application     TDE;

        if ( isDesktopFile(fItem) ) {
            KSimpleConfig cfg( fItem->url().path(), true );
            cfg.setDesktopGroup();
            if ( cfg.readEntry( "X-Trinity-BuiltIn" ) == "true" ) {
                removeBuiltinIcon(cfg.readEntry( "Name" ));
                continue;
            }
        }

        KDesktopFile df(desktopPath + fItem->url().fileName());
        df.writeEntry("Hidden", true);
        df.sync();

        delete it;
    }
    return !itemsLeft;
}

void KDIconView::slotTrashActivated( TDEAction::ActivationReason reason, TQt::ButtonState state )
{
    if (deleteGlobalDesktopFiles())
       return; // All items deleted

    if ( reason == TDEAction::PopupMenuActivation && ( state & TQt::ShiftButton ) )
       KonqOperations::del(this, KonqOperations::DEL, selectedUrls());
    else
       KonqOperations::del(this, KonqOperations::TRASH, selectedUrls());
}

void KDIconView::slotDelete()
{
    if (deleteGlobalDesktopFiles())
       return; // All items deleted
    KonqOperations::del(this, KonqOperations::DEL, selectedUrls());
}

// -----------------------------------------------------------------------------

// This method is called when right-clicking over one or more items
// Not to be confused with the global popup-menu, KRootWm, when doing RMB on the desktop
void KDIconView::popupMenu( const TQPoint &_global, const KFileItemList& _items )
{
    if (!kapp->authorize("action/kdesktop_rmb")) return;
    if (!m_dirLister) return;
    if ( _items.count() == 1 )
        m_popupURL = _items.getFirst()->url();

    TDEAction* pasteTo = m_actionCollection.action( "pasteto" );
    if (pasteTo)
        pasteTo->setEnabled( m_actionCollection.action( "paste" )->isEnabled() );

    bool hasMediaFiles = false;
    KFileItemListIterator it(_items);
    for (; it.current() && !hasMediaFiles; ++it) {
        hasMediaFiles = it.current()->url().protocol() == "media";
    }

    KParts::BrowserExtension::PopupFlags itemFlags = KParts::BrowserExtension::DefaultPopupItems;
    if ( hasMediaFiles )
        itemFlags |= KParts::BrowserExtension::NoDeletion;
    KonqPopupMenu * popupMenu = new KonqPopupMenu( KonqBookmarkManager::self(), _items,
                                                   url(),
                                                   m_actionCollection,
                                                   KRootWm::self()->newMenu(),
                                                   this,
                                                   KonqPopupMenu::ShowProperties | KonqPopupMenu::ShowNewWindow,
                                                   itemFlags );

    popupMenu->exec( _global );
    delete popupMenu;
    m_popupURL = KURL();
    if (pasteTo)
        pasteTo->setEnabled( false );
}

void KDIconView::slotNewMenuActivated()
{
    //kdDebug(1204) << "KDIconView::slotNewMenuActivated" << endl;
    // New / <template> was chosen, a new file is going to appear soon,
    // make it appear at the position of the popupmenu.
    m_nextItemPos = KRootWm::self()->desktopMenuPosition();
}

// -----------------------------------------------------------------------------

void KDIconView::slotEnableAction( const char * name, bool enabled )
{
  //kdDebug(1204) << "slotEnableAction " << name << " enabled=" << enabled << endl;
  TQCString sName( name );
  // No such actions here... konqpopupmenu provides them.
  if ( sName == "properties" || sName == "editMimeType" )
    return;

  TDEAction * act = m_actionCollection.action( sName.data() );
  if (act)
    act->setEnabled( enabled );
}

// -----------------------------------------------------------------------------

// Straight from kpropsdlg :)
bool KDIconView::isDesktopFile( KFileItem * _item ) const
{
  // only local files
  if ( !_item->isLocalFile() )
    return false;

  // only regular files
  if ( !S_ISREG( _item->mode() ) )
    return false;

  TQString t( _item->url().path() );

  // only if readable
  if ( access( TQFile::encodeName(t), R_OK ) != 0 )
    return false;

  // return true if desktop file
  return ( (_item->mimetype() == TQString::fromLatin1("application/x-desktop"))
       || (_item->mimetype() == TQString::fromLatin1("media/builtin-mydocuments"))
       || (_item->mimetype() == TQString::fromLatin1("media/builtin-mycomputer"))
       || (_item->mimetype() == TQString::fromLatin1("media/builtin-mynetworkplaces"))
       || (_item->mimetype() == TQString::fromLatin1("media/builtin-printers"))
       || (_item->mimetype() == TQString::fromLatin1("media/builtin-trash"))
       || (_item->mimetype() == TQString::fromLatin1("media/builtin-webbrowser")) );
}

TQString KDIconView::stripDesktopExtension( const TQString & text )
{
    if (text.right(7) == TQString::fromLatin1(".kdelnk"))
      return text.left(text.length() - 7);
    else if (text.right(8) == TQString::fromLatin1(".desktop"))
      return text.left(text.length() - 8);
    return text;
}

bool KDIconView::makeFriendlyText( KFileIVI *fileIVI )
{
    KFileItem *item = fileIVI->item();
    TQString desktopFile;
    if ( item->isDir() && item->isLocalFile() )
    {
        KURL u( item->url() );
        u.addPath( ".directory" );
        // using TDEStandardDirs as this one checks for path being
        // a file instead of a directory
        if ( TDEStandardDirs::exists( u.path() ) ) {
            desktopFile = u.path();
        }
    }
    else if ( isDesktopFile( item ) )
    {
        desktopFile = item->url().path();
    }

    if ( !desktopFile.isEmpty() )
    {
        KSimpleConfig cfg( desktopFile, true );
        cfg.setDesktopGroup();
        if (cfg.readBoolEntry("Hidden")) {
            return false;
        }

        if (cfg.readBoolEntry( "NoDisplay", false )) {
            return false;
        }

        TQStringList tmpList;
        if (cfg.hasKey("OnlyShowIn"))
        {
            if (!cfg.readListEntry("OnlyShowIn", ';').contains("TDE")) {
                return false;
            }
        }
        if (cfg.hasKey("NotShowIn"))
        {
            if (cfg.readListEntry("NotShowIn", ';').contains("TDE")) {
                return false;
            }
        }
        if (cfg.hasKey("TryExec"))
        {
            if (TDEStandardDirs::findExe( cfg.readEntry( "TryExec" ) ).isEmpty()) {
                return false;
            }
        }

        TQString name = cfg.readEntry("Name");
        if ( !name.isEmpty() ) {
            fileIVI->setText( name );
        }
        else {
            // For compatibility
            fileIVI->setText( stripDesktopExtension( fileIVI->text() ) );
        }
    }

    return true;
}

// -----------------------------------------------------------------------------

void KDIconView::slotClear()
{
    clear();
}

// -----------------------------------------------------------------------------

void KDIconView::slotNewItems( const KFileItemList & entries )
{
  bool firstRun = (count() == 0);  // no icons yet, this seems to be the initial loading

  // delay updates until all new items have been created
  setUpdatesEnabled( false );
  TQRect area = iconArea();
  setIconArea( TQRect(  0, 0, -1, -1 ) );

  TQString desktopPath;
  KURL desktop_URL = desktopURL();
  if (desktop_URL.isLocalFile()) {
    desktopPath = desktop_URL.path();
  }
  // We have new items, so we'll need to repaint in slotCompleted
  m_bNeedRepaint = true;
  kdDebug(1214) << "KDIconView::slotNewItems count=" << entries.count() << endl;
  KFileItemListIterator it(entries);
  KFileIVI* fileIVI = 0L;

  typedef TQValueList<KFileIVI*> KFileIVIList;
  KFileIVIList newItemsList;

  // Ensure that the saved positions had a chance to be loaded
  if (!m_dotDirectory) {
      initDotDirectories();
  }

  if (m_nextItemPos.isNull() && !m_dotDirectory)  {
      // Not found, we'll need to save the new pos
      kdDebug(1214)<<"Neither a  drop position stored nor m_dotDirectory set"<<endl;
      m_dotDirectory = new KSimpleConfig( dotDirectoryPath(), true );
      // recursion
      slotNewItems( entries );
      delete m_dotDirectory;
      m_dotDirectory = 0;
      return;
  }

  for (; it.current(); ++it)
  {
    KURL url = it.current()->url();
    if (!desktopPath.isEmpty() && url.isLocalFile() && !url.path().startsWith(desktopPath))
    {
      TQString fileName = url.fileName();
      if (TQFile::exists(desktopPath + fileName)) {
         continue; // Don't duplicate entry
      }

      TQString mostLocal = locate("appdata", "Desktop/"+fileName);
      if (!mostLocal.isEmpty() && (mostLocal != url.path())) {
         continue; // Don't duplicate entry
      }
    }

    // No delayed mimetype determination on the desktop
    it.current()->determineMimeType();
    fileIVI = new KFileIVIDesktop( this, it.current(), iconSize(), m_shadowEngine );
    if (!makeFriendlyText( fileIVI ))
    {
      delete fileIVI;
      continue;
    }

    kdDebug(1214) << " slotNewItems: " << url.url() << " text: " << fileIVI->text() << endl;
    fileIVI->setRenameEnabled( false );

    if ( !m_nextItemPos.isNull() ) // position remembered from e.g. RMB-popupmenu position, when doing New/...
    {
      kdDebug(1214) << "slotNewItems : using popupmenu position " << m_nextItemPos.x() << "," << m_nextItemPos.y() << endl;
      fileIVI->move( m_nextItemPos.x(), m_nextItemPos.y() );
      m_nextItemPos = TQPoint();
    }
    else
    {
      kdDebug(1214) << "slotNewItems : trying to read position from .directory file"<<endl;
      TQString group = iconPositionGroupPrefix();
      TQString filename = url.fileName();
      if ( filename.endsWith(".part") && !m_dotDirectory->hasGroup( group + filename ) )
          filename = filename.left( filename.length() - 5 );
      group.append( filename );
      kdDebug(1214) << "slotNewItems : looking for group " << group << endl;
      if ( m_dotDirectory->hasGroup( group ) )
      {
        m_dotDirectory->setGroup( group );
        m_hasExistingPos = true;
        int x,y;
        readIconPosition(m_dotDirectory, x, y);

        kdDebug(1214)<<"slotNewItems() x: "<<x<<" y: "<<y<<endl;

        TQRect oldPos = fileIVI->rect();
        fileIVI->move( x, y );
        if ( (!firstRun) && (!isFreePosition( fileIVI, area )) && (!m_needDesktopAlign) ) // if we can't put it there, then let TQIconView decide
        {
            if (!isFreePosition( fileIVI, area ))
            {
                // Find the offending icon and move it out of the way; saved positions have precedence!
                TQRect r = fileIVI->rect();
                TQIconViewItem *it = firstItem();
                for (; it; it = it->nextItem() )
                {
                    if ( !it->rect().isValid() || it == fileIVI )
                    {
                        continue;
                    }

                    if ( it->intersects( r ) )
                    {
                        moveToFreePosition(it, area);
                    }
                }
            }
            else {
                kdDebug(1214)<<"slotNewItems() pos was not free :-("<<endl;
                fileIVI->move( oldPos.x(), oldPos.y() );
                m_dotDirectory->deleteGroup( group );
                m_bNeedSave = true;
            }
        }
        else
        {
            if (!isFreePosition( fileIVI, area ))
            {
                kdDebug(1214)<<"slotNewItems() pos was not free :-("<<endl;
                // Find the offending icon and move it out of the way; saved positions have precedence!
                TQRect r = fileIVI->rect();
                TQIconViewItem *it = firstItem();
                for (; it; it = it->nextItem() )
                {
                    if ( !it->rect().isValid() || it == fileIVI )
                    {
                        continue;
                    }

                    if ( it->intersects( r ) )
                    {
                        moveToFreePosition(it, area);
                    }
                }
            }
            else
            {
                kdDebug(1214)<<"Using saved position"<<endl;
            }
        }
      }
      else
      {
            // Not found, we'll need to save the new pos
            kdDebug(1214)<<"slotNewItems(): New item without position information, try to find a sane location"<<endl;
            newItemsList.append(fileIVI);
      }
    }

    KFileItem* fileItem = fileIVI->item();
    if ( fileItem->mimetype().startsWith("media/") && KDesktopSettings::mediaFreeSpaceDisplayEnabled() ) {
        if (fileItem->mimetype().contains("_mounted")) {
            showFreeSpaceOverlay(fileIVI);
        }
        else {
            // If not mounted, hide free space overlay
            fileIVI->setShowFreeSpaceOverlay(false);
        }
    }
  }

  KFileIVIList::iterator newitemit;
  for ( newitemit = newItemsList.begin(); newitemit != newItemsList.end(); ++newitemit )
  {
      fileIVI = (*newitemit);
      moveToFreePosition(fileIVI, area);
      m_bNeedSave = true;
  }

  setIconArea( area );

  // align on grid
  if ( m_autoAlign )
  {
      lineupIcons();
  }

  setUpdatesEnabled( true );
}

// -----------------------------------------------------------------------------

// see also KonqKfmIconView::slotRefreshItems
void KDIconView::slotRefreshItems( const KFileItemList & entries )
{
    kdDebug(1204) << "KDIconView::slotRefreshItems" << endl;
    bool bNeedPreviewJob = false;
    KFileItemListIterator rit(entries);
    for (; rit.current(); ++rit)
    {
        bool found = false;
        TQIconViewItem *it = firstItem();
        for ( ; it ; it = it->nextItem() )
        {
            KFileIVI * fileIVI = static_cast<KFileIVI *>(it);
            if ( fileIVI->item() == rit.current() ) // compare the pointers
            {
                kdDebug(1204) << "KDIconView::slotRefreshItems refreshing icon " << fileIVI->item()->url().url() << endl;
                found = true;
                fileIVI->setText( rit.current()->text() );
                if (!makeFriendlyText( fileIVI ))
                {
                    delete fileIVI;
                    break;
                }
                if ( fileIVI->isThumbnail() ) {
                    bNeedPreviewJob = true;
                    fileIVI->invalidateThumbnail();
                }
                else
                    fileIVI->refreshIcon( true );
                if ( rit.current()->isMimeTypeKnown() ) {
                    fileIVI->setMouseOverAnimation( rit.current()->iconName() );
                }
                if ( rit.current()->mimetype().startsWith("media/") && KDesktopSettings::mediaFreeSpaceDisplayEnabled() ) {
                    if (rit.current()->mimetype().contains("_mounted")) {
                        showFreeSpaceOverlay(fileIVI);
                    }
                    else {
                        // If not mounted, hide free space overlay
                        fileIVI->setShowFreeSpaceOverlay(false);
                    }
                }
                break;
            }
        }
        if ( !found )
            kdDebug(1204) << "Item not found: " << rit.current()->url().url() << endl;
    }
    if ( bNeedPreviewJob && previewSettings().count() )
    {
        startImagePreview( TQStringList(), false );
    }
    else
    {
        // In case we replace a big icon with a small one, need to repaint.
        updateContents();
        // Can't do that with m_bNeedRepaint since slotCompleted isn't called
        m_bNeedRepaint = false;
    }
}


void KDIconView::refreshIcons()
{
    TQIconViewItem *it = firstItem();
    for ( ; it ; it = it->nextItem() )
    {
        KFileIVI * fileIVI = static_cast<KFileIVI *>(it);
        if (!(fileIVI->item()->mimetype().startsWith("media/"))) {
            fileIVI->item()->refresh();
        }
        fileIVI->refreshIcon( true );
        makeFriendlyText( fileIVI );
    }
    if (m_enableMedia) {
        m_dirLister->updateDirectory(KURL("media:/"));
    }
}


void KDIconView::FilesAdded( const KURL & directory )
{
    if ( directory.path().length() <= 1 && directory.protocol() == "trash" ) {
        refreshTrashIcon();
    }
}

void KDIconView::FilesRemoved( const KURL::List & fileList )
{
    if ( !fileList.isEmpty() ) {
        const KURL url = fileList.first();
        if ( url.protocol() == "trash" ) {
            refreshTrashIcon();
        }
    }
}

void KDIconView::refreshTrashIcon()
{
    TQIconViewItem *it = firstItem();
    for ( ; it ; it = it->nextItem() )
    {
        KFileIVI * fileIVI = static_cast<KFileIVI *>(it);
        KFileItem* item = fileIVI->item();
        if ( isDesktopFile( item ) ) {
            KSimpleConfig cfg( item->url().path(), true );
            cfg.setDesktopGroup();
            if ( cfg.readEntry( "Type" ) == "Link" &&
                 cfg.readEntry( "URL" ) == "trash:/" ) {
                fileIVI->refreshIcon( true );
            }
        }
    }
}

void KDIconView::slotFreeSpaceOverlaySettingChanged()
{
    bool show = KDesktopSettings::mediaFreeSpaceDisplayEnabled();

    for ( TQIconViewItem *item = firstItem(); item; item = item->nextItem() )
    {
        KFileIVI* kItem = static_cast<KFileIVI*>(item);
        if ( !kItem->item()->isDir() ) continue;

        if (show) {
            showFreeSpaceOverlay(kItem);
        } else {
            kItem -> setShowFreeSpaceOverlay(false);
        }
    }

    updateContents();
}

void KDIconView::showFreeSpaceOverlay(KFileIVI* item)
{
    KFileItem* fileItem = item->item();

    if ( TDEGlobalSettings::showFilePreview( fileItem->url() ) ) {
        m_paOutstandingFreeSpaceOverlays.append(item);
        if (m_paOutstandingFreeSpaceOverlays.count() == 1)
        {
           if (!m_paOutstandingFreeSpaceOverlaysTimer)
           {
              m_paOutstandingFreeSpaceOverlaysTimer = new TQTimer(this);
              connect(m_paOutstandingFreeSpaceOverlaysTimer, TQ_SIGNAL(timeout()), TQ_SLOT(slotFreeSpaceOverlayStart()));
           }
           m_paOutstandingFreeSpaceOverlaysTimer->start(20, true);
        }
    }
}

void KDIconView::slotFreeSpaceOverlayStart()
{
    do
    {
       KFileIVI* item = m_paOutstandingFreeSpaceOverlays.first();
       if (!item) {
          return; // Nothing to do
       }

       KIVFreeSpaceOverlay* overlay = item->setShowFreeSpaceOverlay( true );

       if (overlay)
       {
          connect( overlay, TQ_SIGNAL( finished() ), this, TQ_SLOT( slotFreeSpaceOverlayFinished() ) );
          overlay->start(); // Watch out, may emit finished() immediately!!
          return; // Let it run....
       }
       m_paOutstandingFreeSpaceOverlays.removeFirst();
    } while (true);
}

void KDIconView::slotFreeSpaceOverlayFinished()
{
    m_paOutstandingFreeSpaceOverlays.removeFirst();

    if (m_paOutstandingFreeSpaceOverlays.count() > 0) {
        m_paOutstandingFreeSpaceOverlaysTimer->start(0, true); // Don't call directly to prevent deep recursion.
    }
}

// -----------------------------------------------------------------------------

void KDIconView::slotDeleteItem( KFileItem * _fileitem )
{
    kdDebug(1204) << "KDIconView::slotDeleteItems" << endl;
    // we need to find out the KFileIVI containing the fileitem
    TQIconViewItem *it = firstItem();
    while ( it ) {
      KFileIVI * fileIVI = static_cast<KFileIVI *>(it);
      if ( fileIVI->item() == _fileitem ) { // compare the pointers
        // Delete this item.
        //kdDebug(1204) << fileIVI->text() << endl;

        TQString group = iconPositionGroupPrefix();
        group.append( fileIVI->item()->url().fileName() );
        if ( m_dotDirectory->hasGroup( group ) ) {
            m_dotDirectory->deleteGroup( group );
        }

        m_lastDeletedIconPos = fileIVI->pos();
        delete fileIVI;
        break;
      }
      it = it->nextItem();
    }
    m_bNeedRepaint = true;
}

// -----------------------------------------------------------------------------

void KDIconView::slotStarted( const KURL& _url )
{
    kdDebug(1204) << "KDIconView::slotStarted url: " << _url.url() << " url().url(): "<<url().url()<<endl;
}

void KDIconView::slotCompleted()
{
    // Root item ? Store in konqiconviewwidget (used for drops onto the background, for instance)
    if ( m_dirLister->rootItem() )
      setRootItem( m_dirLister->rootItem() );

    if ( previewSettings().count() )
        startImagePreview( TQStringList(), true );
    else
    {
        stopImagePreview();
        setIcons( iconSize(), "*" /* stopImagePreview */ );
    }

    // during first run need to rearrange all icons so default config settings will be used
    kdDebug(1204)<<"slotCompleted() m_hasExistingPos: "<<(m_hasExistingPos?(int)1:(int)0)<<endl;
    if (!m_hasExistingPos)
        rearrangeIcons();

//    kdDebug(1204) << "KDIconView::slotCompleted save:" << m_bNeedSave << " repaint:" << m_bNeedRepaint << endl;
    if ( m_bNeedSave )
    {
        // Done here because we want to align icons only once initially, and each time new icons appear.
        // This MUST precede the call to saveIconPositions().
        emit iconMoved();
        saveIconPositions();
        m_hasExistingPos = true; // if we didn't have positions, we have now.
        m_bNeedSave = false;
    }
    if ( m_bNeedRepaint )
    {
        viewport()->repaint();
        m_bNeedRepaint = false;
    }
}

void KDIconView::slotClipboardDataChanged()
{
    // This is very related to KonqDirPart::slotClipboardDataChanged

    KURL::List lst;
    TQMimeSource *data = TQApplication::clipboard()->data();
    if ( data->provides( "application/x-tde-cutselection" ) && data->provides( "text/uri-list" ) )
        if ( KonqDrag::decodeIsCutSelection( data ) )
            (void) KURLDrag::decode( data, lst );

    disableIcons( lst );

    TQString actionText = TDEIO::pasteActionText();
    bool paste = !actionText.isEmpty();
    if ( paste ) {
        TDEAction* pasteAction = m_actionCollection.action( "paste" );
        if ( pasteAction )
            pasteAction->setText( actionText );
    }
    slotEnableAction( "paste", paste );
}

void KDIconView::renameDesktopFile(const TQString &path, const TQString &name)
{
    KDesktopFile cfg( path, false );

    // if we don't have the desktop entry group, then we assume that
    // it's not a config file (and we don't nuke it!)
    if ( !cfg.hasGroup( "Desktop Entry" ) )
      return;

    if ( cfg.readName() == name )
      return;

    cfg.writeEntry( "Name", name, true, false, false );
    cfg.writeEntry( "Name", name, true, false, true );
    cfg.sync();
}

void KDIconView::slotItemRenamed(TQIconViewItem* _item, const TQString &name)
{
    kdDebug(1204) << "KDIconView::slotItemRenamed(item, \"" << name << "\" )" << endl;
    TQString newName(name);
    if ( _item)
    {
       KFileIVI *fileItem = static_cast< KFileIVI* >( _item );
       //save position of item renamed
       m_lastDeletedIconPos = fileItem->pos();
       if ( fileItem->item() && !fileItem->item()->isLink() )
       {
          TQString desktopFile( fileItem->item()->url().path() );
          if (!desktopFile.isEmpty())
          {
             // first and foremost, we make sure that this is a .desktop file
             // before we write anything to it
             KMimeType::Ptr type = KMimeType::findByURL( fileItem->item()->url() );
             bool bDesktopFile = false;

             if ( (type->name() == "application/x-desktop")
               || (type->name() == "media/builtin-mydocuments")
               || (type->name() == "media/builtin-mycomputer")
               || (type->name() == "media/builtin-mynetworkplaces")
               || (type->name() == "media/builtin-printers")
               || (type->name() == "media/builtin-trash")
               || (type->name() == "media/builtin-webbrowser") )
             {
                bDesktopFile = true;
                if (!newName.endsWith(".desktop")) {
                   newName += ".desktop";
                }
             }
             else if(type->name() == "inode/directory") {
                desktopFile += "/.directory";
                bDesktopFile = true;
             }

             if (TQFile(desktopFile).exists() && bDesktopFile)
             {
                renameDesktopFile(desktopFile, name);
                return;
             }
          }
       }
   }
   KonqIconViewWidget::slotItemRenamed(_item, newName);
}

void KDIconView::slotAboutToCreate(const TQPoint &pos, const TQValueList<TDEIO::CopyInfo> &files)
{
   if (pos.isNull())
      return;

    if (m_dropPos != pos)
    {
       m_dropPos = pos;
       m_lastDropPos = pos;
    }

    TQString dir = url().path(-1); // Strip trailing /

    TQValueList<TDEIO::CopyInfo>::ConstIterator it = files.begin();
    int gridX = gridXValue();
    int gridY = 120; // 120 pixels should be enough for everyone (tm)

    for ( ; it!= files.end() ; ++it )
    {
        kdDebug(1214) << "KDIconView::saveFuturePosition x=" << m_lastDropPos.x() << " y=" << m_lastDropPos.y() << " filename=" << (*it).uDest.prettyURL() << endl;
        if ((*it).uDest.isLocalFile() && ((*it).uDest.directory() == dir))
        {
           m_dotDirectory->setGroup( iconPositionGroupPrefix() + (*it).uDest.fileName() );
           saveIconPosition(m_dotDirectory, m_lastDropPos.x(), m_lastDropPos.y());
           int dX = m_lastDropPos.x() - m_dropPos.x();
           int dY = m_lastDropPos.y() - m_dropPos.y();
           if ((TQABS(dX) > TQABS(dY)) || (m_lastDropPos.x() + 2*gridX > width()))
              m_lastDropPos = TQPoint(m_dropPos.x(), m_lastDropPos.y() + gridY);
           else
              m_lastDropPos = TQPoint(m_lastDropPos.x() + gridX, m_lastDropPos.y());
        }
    }
    m_dotDirectory->sync();
}

// -----------------------------------------------------------------------------

void KDIconView::showEvent( TQShowEvent *e )
{
    //HACK to avoid TQIconView calling arrangeItemsInGrid (Simon)
    //EVEN MORE HACK: unfortunately, TQScrollView has no concept of
    //TopToBottom, therefore, it always adds LeftToRight.  So, if any of
    //the icons have a setting, we'll use TQScrollView.. but otherwise,
    //we use the iconview
   //kdDebug(1204)<<"showEvent() m_hasExistingPos: "<<(m_hasExistingPos?(int)1:(int)0)<<endl;
    if (m_hasExistingPos)
        TQScrollView::showEvent( e );
    else
        TDEIconView::showEvent( e );
}

void KDIconView::contentsDropEvent( TQDropEvent * e )
{
    kdDebug(1204)<<"void KDIconView::contentsDropEvent( TQDropEvent * e )\n";
    // mind: if it's a filedrag which itself is an image, libkonq is called. There's a popup for drops as well
    // that contains the same line "Set as Wallpaper" in void KonqOperations::asyncDrop
    bool isColorDrag = KColorDrag::canDecode(e);
    bool isImageDrag = TQImageDrag::canDecode(e);
    bool isUrlDrag = KURLDrag::canDecode(e);

    bool isImmutable = TDEGlobal::config()->isImmutable();

    if ( (isColorDrag || isImageDrag) && !isUrlDrag ) {
        // Hack to clear the drag shape
        bool bMovable = itemsMovable();
        bool bSignals = signalsBlocked();
        setItemsMovable(false);
        blockSignals(true);
        TDEIconView::contentsDropEvent( e );
        blockSignals(bSignals);
        setItemsMovable(bMovable);
        // End hack

        if ( !isImmutable ) // just ignore event in kiosk-mode
        {
            if ( isColorDrag)
               emit colorDropEvent( e );
            else if (isImageDrag)
               emit imageDropEvent( e );
        }
    } else {
        setLastIconPosition( e->pos() );
        KonqIconViewWidget::contentsDropEvent( e );
    }

    // Check if any items have been moved outside the desktop area.
    // If we find any, move them right back in there. (#40418)
    TQRect desk = desktopRect();
    bool adjustedAnyItems = false;
    for( TQIconViewItem *item = firstItem(); item; item = item->nextItem() )
    {
        if( !desk.contains( item->rect(), true ))
        {
            TQRect r = item->rect();

            if( r.top() < 0 )
                r.moveTop( 0 );
            if( r.bottom() > rect().bottom() )
                r.moveBottom( rect().bottom() );
            if( r.left() < 0 )
                r.moveLeft( 0 );
            if( r.right() > rect().right() )
                r.moveRight( rect().right() );

            item->move( r.x(), r.y() );
            adjustedAnyItems = true;
        }
    }
    if( adjustedAnyItems )
    {
        // Make sure the viewport isn't unnecessarily resized by now,
        // then schedule a repaint to remove any garbage pixels.
        resizeContents( width(), height() );
        viewport()->update();
    }

    if (TQIconDrag::canDecode(e)) {
      emit iconMoved();
      if ( !m_autoAlign )    // if autoAlign, positions were saved in lineupIcons
        saveIconPositions();
    }
}

// don't scroll when someone uses his nifty mouse wheel
void KDIconView::viewportWheelEvent( TQWheelEvent * e )
{
    e->accept();
}

void KDIconView::updateWorkArea( const TQRect &wr )
{
    m_gotIconsArea = true;  // now we have it!

    if (( iconArea() == wr ) && (m_needDesktopAlign == false)) {
        // nothing changed; avoid repaint/saveIconPosition ...
        return;
    }

    TQRect oldArea = iconArea();
    setIconArea( wr );

    kdDebug(1204) << "KDIconView::updateWorkArea wr: " << wr.x() << "," << wr.y() << " " << wr.width() << "x" << wr.height() << endl;
    kdDebug(1204) << "  oldArea:                     " << oldArea.x() << "," << oldArea.y() << " " << oldArea.width() << "x" << oldArea.height() << endl;

    bool needRepaint = false;
    TQIconViewItem* item;
    int dx, dy;

    dx = wr.left() - oldArea.left();
    dy = wr.top() - oldArea.top();

    if ( dx != 0 || dy != 0 ) {
        if ( (dx > 0) || (dy > 0) ) {
            // the iconArea was shifted right/down; less space now
            for ( item = firstItem(); item; item = item->nextItem() ) {
                // check if there is any item inside the now unavailable area
                // If so, we have to move _all_ items
                // If not, we don't have to move any item (avoids bug:117868)
                if ( (item->x() < wr.x()) || (item->y() < wr.y()) ) {
                    needRepaint = true;
                    break;
                }
            }
        }
        else {
            // the iconArea was shifted left/up; more space now - use it
            needRepaint = true;
        }

        if ( needRepaint ) {
            for ( item = firstItem(); item; item = item->nextItem() ) {
                item->moveBy( dx, dy );
            }
        }
    }

    for ( item = firstItem(); item; item = item->nextItem() ) {
        TQRect r( item->rect() );
        int dx = 0, dy = 0;
        if ( r.bottom() > wr.bottom() ) {
            dy = wr.bottom() - r.bottom() - 1;
        }
        if ( r.right() > wr.right() ) {
            dx = wr.right() - r.right() - 1;
        }
        if ( dx != 0 || dy != 0 ) {
            needRepaint = true;
            item->moveBy( dx, dy );
        }
    }
    if ( needRepaint ) {
        viewport()->repaint( false );
        repaint( false );
        saveIconPositions();
    }

    m_needDesktopAlign = false;
    lineupIcons();
}

void KDIconView::setupSortKeys()
{
    // can't use sorting in KFileIVI::setKey()
    setProperty("sortDirectoriesFirst", TQVariant(false));

    for (TQIconViewItem *it = firstItem(); it; it = it->nextItem())
    {
        TQString strKey;

        if (!m_itemsAlwaysFirst.isEmpty())
        {
            TQString strFileName = static_cast<KFileIVI *>( it )->item()->url().fileName();
            int nFind = m_itemsAlwaysFirst.findIndex(strFileName);
            if (nFind >= 0)
                strKey = "0" + TQString::number(nFind);
        }

        if (strKey.isEmpty())
        {
            switch (m_eSortCriterion)
            {
            case NameCaseSensitive:
                strKey = it->text();
                break;
            case NameCaseInsensitive:
                strKey = it->text().lower();
                break;
            case Size:
                strKey = TDEIO::number(static_cast<KFileIVI *>( it )->item()->size()).rightJustify(20, '0');
                break;
            case Type:
                // Sort by Type + Name (#17014)
                strKey = static_cast<KFileIVI *>( it )->item()->mimetype() + '~' + it->text().lower();
                break;
            case Date:
                TQDateTime dayt;
                dayt.setTime_t( static_cast<KFileIVI *>( it )->
                    item()->time( TDEIO::UDS_MODIFICATION_TIME ) );
                strKey = dayt.toString( "yyyyMMddhhmmss" );
                break;
            }

            if (m_bSortDirectoriesFirst)
            {
                if (S_ISDIR(static_cast<KFileIVI *>( it )->item()->mode()))
                    strKey.prepend(sortDirection() ? '1' : '2');
                else
                    strKey.prepend(sortDirection() ? '2' : '1' );
            }
            else
                strKey.prepend('1');
        }

        it->setKey(strKey);
    }
}

bool KDIconView::isFreePosition( const TQIconViewItem *item, const TQRect &currentIconArea ) const
{
    TQRect r = item->rect();
    TQRect area = currentIconArea;
    if (area.isNull()) area = iconArea();

    // If the proposed item rect is not contained by the desktop, by definition the item position is not free!
    if (!area.contains(r, false)) {
        return false;
    }

    TQIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() ) {
        if ( !it->rect().isValid() || it == item ) {
            continue;
        }

        if ( it->intersects( r ) ) {
            return false;
        }
    }

    return true;
}

bool KDIconView::isFreePosition( const TQIconViewItem *item, const TQRect& rect, const TQRect& currentIconArea) const
{
    TQRect area = currentIconArea;
    if (area.isNull()) area = iconArea();

    // If the proposed item rect is not contained by the desktop, by definition the item position is not free!
    if (!area.contains(rect, false)) {
        return false;
    }

    TQIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() ) {
        if ( !rect.isValid() || it == item ) {
            continue;
        }

        if ( it->intersects( rect ) ) {
            return false;
        }
    }

    return true;
}

void KDIconView::setLastIconPosition( const TQPoint &_pos )
{
    m_lastDeletedIconPos = _pos;
}

void KDIconView::moveToFreePosition(TQIconViewItem *item, const TQRect &currentIconArea)
{
    bool success = false;
    // It may be that a file has been renamed. In this case,
    // m_lastDeletedIconPos is the position to use for this "apparently new" item.
    // (We rely on deleteItem being now emitted before newItems).
    if ( !m_lastDeletedIconPos.isNull() ) {
        // Problem is: I'd like to compare those two file's attributes
        // (size, creation time, modification time... etc.) but since renaming
        // is done by kpropsdlg, all of those can have changed (and creation time
        // is different since the new file is a copy!)
        kdDebug(1214) << "Moving " << item->text() << " to position of last deleted icon." << endl;
        item->move( m_lastDeletedIconPos );
        m_lastDeletedIconPos = TQPoint();
        return;
    }

    //try to find a free place to put the item, honouring the m_bVertAlign property
    TQRect rect=item->rect();
    if (m_bVertAlign) {
        kdDebug(1214)<<"moveToFreePosition for vertical alignment"<<endl;

        rect.moveTopLeft(TQPoint(currentIconArea.x()+spacing(),currentIconArea.y()+spacing()));
        do {
            success=false;
            while (rect.bottom()<(currentIconArea.y()+currentIconArea.height())) {
                if (!isFreePosition(item,rect,currentIconArea)) {
                    rect.moveBy(0,rect.height()+spacing());
                }
                else {
                    success=true;
                    break;
                }
            }

            if (!success) {
                rect.moveTopLeft(TQPoint(rect.right()+spacing(),spacing()));
            }
            else {
                break;
            }
        } while (item->rect().right()<(currentIconArea.x()+currentIconArea.width()));

        if (success) {
            item->move(rect.x(),rect.y());
        }
        else {
            item->move(width()-spacing()-item->rect().width(),height()-spacing()-item->rect().height());
        }
    }
}


TQPoint KDIconView::findPlaceForIconCol( int column, int dx, int dy, const TQRect &currentIconArea )
{
    if (column < 0)
        return TQPoint();

    TQRect rect;
    rect.moveTopLeft( TQPoint(column * dx, 0) );
    rect.setWidth(dx);
    rect.setHeight(dy);

    if (rect.right() > viewport()->width())
        return TQPoint();

    while ( rect.bottom() < viewport()->height() - spacing() )
    {
        if ( !isFreePosition(0,rect,currentIconArea) ) {
            rect.moveBy(0, rect.height());
        }
        else {
            return rect.topLeft();
        }
    }

    return TQPoint();
}

TQPoint KDIconView::findPlaceForIconRow( int row, int dx, int dy, const TQRect &currentIconArea )
{
    if (row < 0)
        return TQPoint();

    TQRect rect;
    rect.moveTopLeft(TQPoint(0, row * dy));
    rect.setWidth(dx);
    rect.setHeight(dy);

    if (rect.bottom() > viewport()->height())
        return TQPoint();

    while (rect.right() < viewport()->width() - spacing())
    {
        if (!isFreePosition(0,rect,currentIconArea))
            rect.moveBy(rect.width()+spacing(), 0);
        else
            return rect.topLeft();
    }

    return TQPoint();
}

TQPoint KDIconView::findPlaceForIcon( int column, int row, const TQRect &currentIconArea )
{
    int dx = gridXValue(), dy = 0;
    TQIconViewItem *item = firstItem();
    for ( ; item; item = item->nextItem() ) {
        dx = TQMAX( dx, item->width() );
        dy = TQMAX( dy, item->height() );
    }

    dx += spacing();
    dy += spacing();

    if (row == -1) {
        int max_cols = viewport()->width() / dx;
        int delta = 0;
        TQPoint res;
        do {
            delta++;
            res = findPlaceForIconCol(column + (delta / 2) * (-2 * (delta % 2) + 1),
                                      dx, dy, currentIconArea);
            if (delta / 2 > TQMAX(max_cols - column, column))
                return res;
        } while (res.isNull());
        return res;
    }

    if (column == -1) {
        int max_rows = viewport()->height() / dy;
        int delta = 0;
        TQPoint res;
        do {
            delta++;
            res = findPlaceForIconRow(row + (delta / 2) * (-2 * (delta % 2) + 1),
                                      dx, dy, currentIconArea);
            if (delta / 2 > TQMAX(max_rows - row, row))
                return res;
        } while (res.isNull());
        return res;
    }

    // very unlikely - if I may add that
    return TQPoint(0, 0);
}

void KDIconView::saveIconPositions()
{
  kdDebug(1214) << "KDIconView::saveIconPositions" << endl;

  if (!m_bEditableDesktopIcons) {
    return; // Don't save position
  }

  TQString prefix = iconPositionGroupPrefix();
  TQIconViewItem *it = firstItem();
  if ( !it ) {
    return; // No more icons. Maybe we're closing and they've been removed already
  }

  while ( it )
  {
    KFileIVI *ivi = static_cast<KFileIVI *>( it );
    KFileItem *item = ivi->item();

    m_dotDirectory->setGroup( prefix + item->url().fileName() );
    kdDebug(1214) << "KDIconView::saveIconPositions " << item->url().fileName() << " " << it->x() << " " << it->y() << endl;
    saveIconPosition(m_dotDirectory, it->x(), it->y());

    it = it->nextItem();
  }

  m_dotDirectory->sync();
}

void KDIconView::update( const TQString &_url )
{
    if (m_dirLister) {
        m_dirLister->updateDirectory( _url );
    }
}


#include "kdiconview.moc"
