/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_dirpart.h"
#include "konq_bgnddlg.h"
#include "konq_propsview.h"
#include "konq_settings.h"

#include <tdeio/paste.h>
#include <tdeapplication.h>
#include <tdeaction.h>
#include <kdatastream.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <konq_drag.h>
#include <tdeparts/browserextension.h>
#include <kurldrag.h>
#include <kuserprofile.h>
#include <kurifilter.h>
#include <tdeglobalsettings.h>

#include <tqapplication.h>
#include <tqclipboard.h>
#include <tqfile.h>
#include <tqguardedptr.h>
#include <assert.h>
#include <tqvaluevector.h>

class KonqDirPart::KonqDirPartPrivate
{
public:
    KonqDirPartPrivate() : dirLister( 0 ) {}
    TQStringList mimeFilters;
    TDEToggleAction *aEnormousIcons;
    TDEToggleAction *aSmallMediumIcons;
    TQValueVector<int> iconSize;
            
    KDirLister* dirLister;
    bool dirSizeDirty;

    void findAvailableIconSizes(void);
    int findNearestIconSize(int size);
    int nearestIconSizeError(int size);
};

void KonqDirPart::KonqDirPartPrivate::findAvailableIconSizes(void)
{
    TDEIconTheme *root = TDEGlobal::instance()->iconLoader()->theme();
    iconSize.resize(1);
    if (root) {
	TQValueList<int> avSizes = root->querySizes(TDEIcon::Desktop);
        kdDebug(1203) << "The icon theme handles the sizes:" << avSizes << endl;
	qHeapSort(avSizes);
	int oldSize = -1;
	if (avSizes.count() < 10) {
	    // Fixed or threshold type icons
	    TQValueListConstIterator<int> i;
	    for (i = avSizes.begin(); i != avSizes.end(); i++) {
		// Skip duplicated values (sanity check)
		if (*i != oldSize) iconSize.append(*i);
		oldSize = *i;
	    }
	} else {
	    // Scalable icons.
	    const int progression[] = {16, 22, 32, 48, 64, 96, 128, 192, 256};

	    TQValueListConstIterator<int> j = avSizes.begin();
	    for (uint i = 0; i < 9; i++) {
		while (j++ != avSizes.end()) {
		    if (*j >= progression[i]) {
			iconSize.append(*j);
			kdDebug(1203) << "appending " << *j << " size." << endl;
			break;
		    }
		}
	    }
	}
    } else {
	iconSize.append(TDEIcon::SizeSmall); // 16
	iconSize.append(TDEIcon::SizeMedium); // 32
	iconSize.append(TDEIcon::SizeLarge); // 48
	iconSize.append(TDEIcon::SizeHuge); // 64
    }
    kdDebug(1203) << "Using " << iconSize.count() << " icon sizes." << endl;
}

int KonqDirPart::KonqDirPartPrivate::findNearestIconSize(int preferred)
{
    int s1 = iconSize[1];
    if (preferred == 0) return TDEGlobal::iconLoader()->currentSize(TDEIcon::Desktop);
    if (preferred <= s1) return s1;
    for (uint i = 2; i <= iconSize.count(); i++) {
        if (preferred <= iconSize[i]) {
	    if (preferred - s1 <  iconSize[i] - preferred) return s1;
	    else return iconSize[i];
	} else {
	    s1 = iconSize[i];
	}
    }
    return s1;
}

int KonqDirPart::KonqDirPartPrivate::nearestIconSizeError(int size)
{
    return QABS(size - findNearestIconSize(size));
}

KonqDirPart::KonqDirPart( TQObject *parent, const char *name )
            :KParts::ReadOnlyPart( parent, name ),
    m_pProps( 0L ),
    m_findPart( 0L )
{
    d = new KonqDirPartPrivate;
    resetCount();
    //m_bMultipleItemsSelected = false;

    connect( TQApplication::clipboard(), TQT_SIGNAL(dataChanged()), this, TQT_SLOT(slotClipboardDataChanged()) );

    actionCollection()->setHighlightingEnabled( true );

    m_paIncIconSize = new TDEAction( i18n( "Enlarge Icons" ), "zoom-in", 0, this, TQT_SLOT( slotIncIconSize() ), actionCollection(), "incIconSize" );
    m_paDecIconSize = new TDEAction( i18n( "Shrink Icons" ), "zoom-out", 0, this, TQT_SLOT( slotDecIconSize() ), actionCollection(), "decIconSize" );

    m_paDefaultIcons = new TDERadioAction( i18n( "&Default Size" ), 0, actionCollection(), "modedefault" );
    d->aEnormousIcons = new TDERadioAction( i18n( "&Huge" ), 0,
	    actionCollection(), "modeenormous" );
    m_paHugeIcons = new TDERadioAction( i18n( "&Very Large" ), 0, actionCollection(), "modehuge" );
    m_paLargeIcons = new TDERadioAction( i18n( "&Large" ), 0, actionCollection(), "modelarge" );
    m_paMediumIcons = new TDERadioAction( i18n( "&Medium" ), 0, actionCollection(), "modemedium" );
    d->aSmallMediumIcons = new TDERadioAction( i18n( "&Small" ), 0,
	    actionCollection(), "modesmallmedium" );
    m_paSmallIcons = new TDERadioAction( i18n( "&Tiny" ), 0, actionCollection(), "modesmall" );

    m_paDefaultIcons->setExclusiveGroup( "ViewMode" );
    d->aEnormousIcons->setExclusiveGroup( "ViewMode" );
    m_paHugeIcons->setExclusiveGroup( "ViewMode" );
    m_paLargeIcons->setExclusiveGroup( "ViewMode" );
    m_paMediumIcons->setExclusiveGroup( "ViewMode" );
    d->aSmallMediumIcons->setExclusiveGroup( "ViewMode" );
    m_paSmallIcons->setExclusiveGroup( "ViewMode" );

    connect( m_paDefaultIcons, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotIconSizeToggled( bool ) ) );
    connect( d->aEnormousIcons, TQT_SIGNAL( toggled( bool ) ),
	    this, TQT_SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paHugeIcons, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paLargeIcons, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paMediumIcons, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotIconSizeToggled( bool ) ) );
    connect( d->aSmallMediumIcons, TQT_SIGNAL( toggled( bool ) ),
	    this, TQT_SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paSmallIcons, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( slotIconSizeToggled( bool ) ) );

    connect( kapp, TQT_SIGNAL(iconChanged(int)), TQT_SLOT(slotIconChanged(int)) );
#if 0
    // Extract 6 icon sizes from the icon theme.
    // Use 16,22,32,48,64,128 as default.
    // Use these also if the icon theme is scalable.
    int i;
    d->iconSize[0] = 0; // Default value
    d->iconSize[1] = TDEIcon::SizeSmall; // 16
    d->iconSize[2] = TDEIcon::SizeSmallMedium; // 22
    d->iconSize[3] = TDEIcon::SizeMedium; // 32
    d->iconSize[4] = TDEIcon::SizeLarge; // 48
    d->iconSize[5] = TDEIcon::SizeHuge; // 64
    d->iconSize[6] = TDEIcon::SizeEnormous; // 128
    d->iconSize[7] = 192;
    d->iconSize[8] = 256;
    TDEIconTheme *root = TDEGlobal::instance()->iconLoader()->theme();
    if (root)
    {
      TQValueList<int> avSizes = root->querySizes(TDEIcon::Desktop);
      kdDebug(1203) << "the icon theme handles the following sizes:" << avSizes << endl;
      if (avSizes.count() < 10) {
	// Use the icon sizes supplied by the theme.
	// If avSizes contains more than 10 entries, assume a scalable
	// icon theme.
	TQValueList<int>::Iterator it;
	for (i=1, it=avSizes.begin(); (it!=avSizes.end()) && (i<7); it++, i++)
	{
	  d->iconSize[i] = *it;
	  kdDebug(1203) << "m_iIconSize[" << i << "] = " << *it << endl;
	}
	// Generate missing sizes
	for (; i < 7; i++) {
	  d->iconSize[i] = d->iconSize[i - 1] + d->iconSize[i - 1] / 2 ;
	  kdDebug(1203) << "m_iIconSize[" << i << "] = " << d->iconSize[i] << endl;
	}
      }
    }
#else
    d->iconSize.reserve(10);
    d->iconSize.append(0); // Default value
    adjustIconSizes();
#endif

    // Remove in KDE4 ...
    // These are here in the event subclasses access them.
    m_iIconSize[1] = TDEIcon::SizeSmall;
    m_iIconSize[2] = TDEIcon::SizeMedium;
    m_iIconSize[3] = TDEIcon::SizeLarge;
    m_iIconSize[4] = TDEIcon::SizeHuge;
    // ... up to here

    TDEAction *a = new TDEAction( i18n( "Configure Background..." ), "background", 0, this, TQT_SLOT( slotBackgroundSettings() ),
                              actionCollection(), "bgsettings" );

    a->setToolTip( i18n( "Allows choosing of background settings for this view" ) );
}

KonqDirPart::~KonqDirPart()
{
    // Close the find part with us
    delete m_findPart;
    delete d;
    d = 0;
}

void KonqDirPart::adjustIconSizes()
{
    d->findAvailableIconSizes();
    m_paSmallIcons->setEnabled(d->findNearestIconSize(16) < 20);
    d->aSmallMediumIcons->setEnabled(d->nearestIconSizeError(22) < 2);
    m_paMediumIcons->setEnabled(d->nearestIconSizeError(32) < 6);
    m_paLargeIcons->setEnabled(d->nearestIconSizeError(48) < 8);
    m_paHugeIcons->setEnabled(d->nearestIconSizeError(64) < 12);
    d->aEnormousIcons->setEnabled(d->findNearestIconSize(128) > 110);

    if (m_pProps) {
	int size = m_pProps->iconSize();
	int nearSize = d->findNearestIconSize(size);

	if (size != nearSize) {
	    m_pProps->setIconSize(nearSize);
	}
	newIconSize(nearSize);
    }
}

void KonqDirPart::setMimeFilter (const TQStringList& mime)
{
    TQString u = url().url();

    if ( u.isEmpty () )
        return;

    if ( mime.isEmpty() )
        d->mimeFilters.clear();
    else
        d->mimeFilters = mime;
}

TQStringList KonqDirPart::mimeFilter() const
{
    return d->mimeFilters;
}

TQScrollView * KonqDirPart::scrollWidget()
{
    return static_cast<TQScrollView *>(widget());
}

void KonqDirPart::slotBackgroundSettings()
{
    TQColor bgndColor = m_pProps->bgColor( widget() );
    TQColor defaultColor = TDEGlobalSettings::baseColor();
    // dlg must be created on the heap as widget() can get deleted while dlg.exec(),
    // trying to delete dlg as its child then (#124210) - Frank Osterfeld
    TQGuardedPtr<KonqBgndDialog> dlg = new KonqBgndDialog( widget(), 
                                              m_pProps->bgPixmapFile(),
                                              bgndColor,
                                              defaultColor );
    
    if ( dlg->exec() == KonqBgndDialog::Accepted )
    {
        if ( dlg->color().isValid() )
        {
            m_pProps->setBgColor( dlg->color() );
        m_pProps->setBgPixmapFile( "" );
    }
        else
    {
            m_pProps->setBgColor( defaultColor );
        m_pProps->setBgPixmapFile( dlg->pixmapFile() );
        }
        m_pProps->applyColors( scrollWidget()->viewport() );
        scrollWidget()->viewport()->repaint();
    }
    
    delete dlg;
}

void KonqDirPart::lmbClicked( KFileItem * fileItem )
{
    KURL url = fileItem->url();
    if ( !fileItem->isReadable() )
    {
        // No permissions or local file that doesn't exist - need to find out which
        if ( ( !fileItem->isLocalFile() ) || TQFile::exists( url.path() ) )
        {
            KMessageBox::error( widget(), i18n("<p>You do not have enough permissions to read <b>%1</b></p>").arg(url.prettyURL()) );
            return;
        }
        KMessageBox::error( widget(), i18n("<p><b>%1</b> does not seem to exist anymore</p>").arg(url.prettyURL()) );
        return;
    }

    KParts::URLArgs args;
    fileItem->determineMimeType();
    if ( fileItem->isMimeTypeKnown() )
        args.serviceType = fileItem->mimetype();
    args.trustedSource = true;

    if (KonqFMSettings::settings()->alwaysNewWin() && fileItem->isDir()) {
        //args.frameName = "_blank"; // open new window
        // We tried the other option, passing the path as framename so that
        // an existing window for that dir is reused (like MSWindows does when
        // the similar option is activated and the sidebar is hidden (!)).
        // But this requires some work, including changing the framename
        // when navigating, etc. Not very much requested yet, in addition.
        KParts::WindowArgs wargs;
        KParts::ReadOnlyPart* dummy;
        emit m_extension->createNewWindow( url, args, wargs, dummy );
    }
    else
    {
        kdDebug() << "emit m_extension->openURLRequest( " << url.url() << "," << args.serviceType << ")" << endl;
        emit m_extension->openURLRequest( url, args );
    }
}

void KonqDirPart::mmbClicked( KFileItem * fileItem )
{
    if ( fileItem )
    {
        // Optimisation to avoid KRun to call kfmclient that then tells us
        // to open a window :-)
        KService::Ptr offer = KServiceTypeProfile::preferredService(fileItem->mimetype(), "Application");
        //if (offer) kdDebug(1203) << "KonqDirPart::mmbClicked: got service " << offer->desktopEntryName() << endl;
        if ( offer && offer->desktopEntryName().startsWith("kfmclient") )
        {
            KParts::URLArgs args;
            args.serviceType = fileItem->mimetype();
            emit m_extension->createNewWindow( fileItem->url(), args );
        }
        else
            fileItem->run();
    }
    else
    {
        m_extension->pasteRequest();
    }
}

void KonqDirPart::saveState( TQDataStream& stream )
{
    stream << m_nameFilter;
}

void KonqDirPart::restoreState( TQDataStream& stream )
{
    stream >> m_nameFilter;
}

void KonqDirPart::saveFindState( TQDataStream& stream )
{
    // assert only doable in KDE4.
    //assert( m_findPart ); // test done by caller.
    if ( !m_findPart )
        return;

    // When we have a find part, our own URL wasn't saved (see KonqDirPartBrowserExtension)
    // So let's do it here
    stream << m_url;

    KParts::BrowserExtension* ext = KParts::BrowserExtension::childObject( m_findPart );
    if( !ext )
        return;

    ext->saveState( stream );
}

void KonqDirPart::restoreFindState( TQDataStream& stream )
{
    // Restore our own URL
    stream >> m_url;

    emit findOpen( this );

    KParts::BrowserExtension* ext = KParts::BrowserExtension::childObject( m_findPart );
    slotClear();

    if( !ext )
        return;

    ext->restoreState( stream );
}

void KonqDirPart::slotClipboardDataChanged()
{
    // This is very related to KDIconView::slotClipboardDataChanged

    KURL::List lst;
    TQMimeSource *data = TQApplication::clipboard()->data();
    if ( data->provides( "application/x-tde-cutselection" ) && data->provides( "text/uri-list" ) ) {
        if ( KonqDrag::decodeIsCutSelection( data ) ) {
            (void) KURLDrag::decode( data, lst );
        }
    }

    disableIcons( lst );

    updatePasteAction();
}

void KonqDirPart::updatePasteAction() // KDE4: merge into method above
{
    TQString actionText = TDEIO::pasteActionText();
    bool paste = !actionText.isEmpty();
    if ( paste ) {
      emit m_extension->setActionText( "paste", actionText );
    }
    emit m_extension->enableAction( "paste", paste );
}

void KonqDirPart::newItems(const KFileItemList &entries)
{
    d->dirSizeDirty = true;
    if ( m_findPart ) {
        emitTotalCount();
    }

    emit itemsAdded(entries);
}

void KonqDirPart::deleteItem(KFileItem * fileItem)
{
    d->dirSizeDirty = true;
    emit itemRemoved(fileItem);
}

void KonqDirPart::refreshItems(const KFileItemList &entries)
{
    emit itemsRefresh(entries);
}

void KonqDirPart::emitTotalCount()
{
    if ( !d->dirLister || d->dirLister->url().isEmpty() ) {
        return;
    }
    if ( d->dirSizeDirty ) {
        m_lDirSize = 0;
        m_lFileCount = 0;
        m_lDirCount = 0;
        KFileItemList entries = d->dirLister->items();
        for (KFileItemListIterator it(entries); it.current(); ++it)
        {
            if ( !it.current()->isDir() )
            {
                if (!it.current()->isLink()) { // symlinks don't contribute to the size
                    m_lDirSize += it.current()->size();
                }
                m_lFileCount++;
            }
            else {
                m_lDirCount++;
            }
        }
        d->dirSizeDirty = false;
    }

    TQString summary =
        TDEIO::itemsSummaryString(m_lFileCount + m_lDirCount,
                                m_lFileCount,
                                m_lDirCount,
                                m_lDirSize,
                                true);
    bool bShowsResult = false;
    if (m_findPart)
    {
        TQVariant prop = m_findPart->property( "showsResult" );
        bShowsResult = prop.isValid() && prop.toBool();
    }
    //kdDebug(1203) << "KonqDirPart::emitTotalCount bShowsResult=" << bShowsResult << endl;
    emit setStatusBarText( bShowsResult ? i18n("Search result: %1").arg(summary) : summary );
}

void KonqDirPart::emitCounts( const KFileItemList & lst )
{
    if ( lst.count() == 1 )
        emit setStatusBarText( ((KFileItemList)lst).first()->getStatusBarInfo() );
    else
    {
        long long fileSizeSum = 0;
        uint fileCount = 0;
        uint dirCount = 0;

        for ( KFileItemListIterator it( lst ); it.current(); ++it )
        {
            if ( it.current()->isDir() )
                dirCount++;
            else
            {
                if ( !it.current()->isLink() ) // ignore symlinks
                    fileSizeSum += it.current()->size();
                fileCount++;
            }
        }

        emit setStatusBarText( TDEIO::itemsSummaryString( fileCount + dirCount,
                                                        fileCount, dirCount,
                                                        fileSizeSum, true ) );
    }
}

void KonqDirPart::emitCounts( const KFileItemList & lst, bool selectionChanged )
{
    if ( lst.count() == 0 ) {
        emitTotalCount();
    }
    else {
        emitCounts( lst );
    }

    // Yes, the caller could do that too :)
    // But this bool could also be used to cache the TQString for the last
    // selection, as long as selectionChanged is false.
    // Not sure it's worth it though.
    // MiB: no, I don't think it's worth it. Especially regarding the
    //      loss of readability of the code. Thus, this will be removed in
    //      KDE 4.0.
    if ( selectionChanged ) {
        emit m_extension->selectionInfo( lst );
    }
}

void KonqDirPart::emitMouseOver( const KFileItem* item )
{
    emit m_extension->mouseOverInfo( item );
}

void KonqDirPart::slotIconSizeToggled( bool toggleOn )
{
    //kdDebug(1203) << "KonqDirPart::slotIconSizeToggled" << endl;

    // This slot is called when an iconsize action is checked or by calling
    // action->setChecked(false) (previously true). So we must filter out
    // the 'untoggled' case to prevent odd results here (repaints/loops!)
    if ( !toggleOn )
        return;

    if ( m_paDefaultIcons->isChecked() )
        setIconSize(0);
    else if ( d->aEnormousIcons->isChecked() )
        setIconSize(d->findNearestIconSize(TDEIcon::SizeEnormous));
    else if ( m_paHugeIcons->isChecked() )
        setIconSize(d->findNearestIconSize(TDEIcon::SizeHuge));
    else if ( m_paLargeIcons->isChecked() )
        setIconSize(d->findNearestIconSize(TDEIcon::SizeLarge));
    else if ( m_paMediumIcons->isChecked() )
        setIconSize(d->findNearestIconSize(TDEIcon::SizeMedium));
    else if ( d->aSmallMediumIcons->isChecked() )
        setIconSize(d->findNearestIconSize(TDEIcon::SizeSmallMedium));
    else if ( m_paSmallIcons->isChecked() )
        setIconSize(d->findNearestIconSize(TDEIcon::SizeSmall));
}

void KonqDirPart::slotIncIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
    uint sizeIndex = 0;
    for ( uint idx = 1; idx < d->iconSize.count() ; ++idx )
        if (s == d->iconSize[idx]) {
            sizeIndex = idx;
	    break;
	}
    if ( sizeIndex > 0 && sizeIndex < d->iconSize.count() - 1 )
    {
        setIconSize( d->iconSize[sizeIndex + 1] );
    }
}

void KonqDirPart::slotDecIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
    uint sizeIndex = 0;
    for ( uint idx = 1; idx < d->iconSize.count() ; ++idx )
        if (s == d->iconSize[idx]) {
            sizeIndex = idx;
	    break;
	}
    if ( sizeIndex > 1 )
    {
        setIconSize( d->iconSize[sizeIndex - 1] );
    }
}

// Only updates Actions, a GUI update is done in the views by reimplementing this
void KonqDirPart::newIconSize( int size /*0=default, or 16,32,48....*/ )
{
    int realSize = (size==0) ? TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop ) : size;
    m_paDecIconSize->setEnabled(realSize > d->iconSize[1]);
    m_paIncIconSize->setEnabled(realSize < d->iconSize.back());

    m_paDefaultIcons->setChecked(size == 0);
    d->aEnormousIcons->setChecked(size == d->findNearestIconSize(TDEIcon::SizeEnormous));
    m_paHugeIcons->setChecked(size == d->findNearestIconSize(TDEIcon::SizeHuge));
    m_paLargeIcons->setChecked(size == d->findNearestIconSize(TDEIcon::SizeLarge));
    m_paMediumIcons->setChecked(size == d->findNearestIconSize(TDEIcon::SizeMedium));
    d->aSmallMediumIcons->setChecked(size == d->findNearestIconSize(TDEIcon::SizeSmallMedium));
    m_paSmallIcons->setChecked(size == d->findNearestIconSize(TDEIcon::SizeSmall));
}

// Stores the new icon size and updates the GUI
void KonqDirPart::setIconSize( int size )
{
    //kdDebug(1203) << "KonqDirPart::setIconSize " << size << " -> updating props and GUI" << endl;
    m_pProps->setIconSize( size );
    newIconSize( size );
}

bool KonqDirPart::closeURL()
{
    // Tell all the childern objects to clean themselves up for dinner :)
    return doCloseURL();
}

bool KonqDirPart::openURL(const KURL& url)
{
    if ( m_findPart )
    {
        kdDebug(1203) << "KonqDirPart::openURL -> emit findClosed " << this << endl;
        delete m_findPart;
        m_findPart = 0L;
        emit findClosed( this );
    }

    m_url = url;
    emit aboutToOpenURL ();

    return doOpenURL(url);
}

void KonqDirPart::setFindPart( KParts::ReadOnlyPart * part )
{
    assert(part);
    m_findPart = part;
    connect( m_findPart, TQT_SIGNAL( started() ),
             this, TQT_SLOT( slotStarted() ) );
    connect( m_findPart, TQT_SIGNAL( started() ),
             this, TQT_SLOT( slotStartAnimationSearching() ) );
    connect( m_findPart, TQT_SIGNAL( clear() ),
             this, TQT_SLOT( slotClear() ) );
    connect( m_findPart, TQT_SIGNAL( newItems( const KFileItemList & ) ),
             this, TQT_SLOT( slotNewItems( const KFileItemList & ) ) );
    connect( m_findPart, TQT_SIGNAL( finished() ), // can't name it completed, it conflicts with a KROP signal
             this, TQT_SLOT( slotCompleted() ) );
    connect( m_findPart, TQT_SIGNAL( finished() ),
             this, TQT_SLOT( slotStopAnimationSearching() ) );
    connect( m_findPart, TQT_SIGNAL( canceled() ),
             this, TQT_SLOT( slotCanceled() ) );
    connect( m_findPart, TQT_SIGNAL( canceled() ),
             this, TQT_SLOT( slotStopAnimationSearching() ) );

    connect( m_findPart, TQT_SIGNAL( findClosed() ),
             this, TQT_SLOT( slotFindClosed() ) );

    emit findOpened( this );

    // set the initial URL in the find part
    m_findPart->openURL( url() );
}

void KonqDirPart::slotFindClosed()
{
    kdDebug(1203) << "KonqDirPart::slotFindClosed -> emit findClosed " << this << endl;
    delete m_findPart;
    m_findPart = 0L;
    emit findClosed( this );
    // reload where we were before
    openURL( url() );
}

void KonqDirPart::slotIconChanged( int group )
{
    if (group != TDEIcon::Desktop) return;
    adjustIconSizes();
}

void KonqDirPart::slotStartAnimationSearching()
{
  started(0);
}

void KonqDirPart::slotStopAnimationSearching()
{
  completed();
}

void KonqDirPartBrowserExtension::saveState( TQDataStream &stream )
{
    m_dirPart->saveState( stream );
    bool hasFindPart = m_dirPart->findPart();
    stream << hasFindPart;
    assert( ! ( hasFindPart && !strcmp(m_dirPart->className(), "KFindPart") ) );
    if ( !hasFindPart )
        KParts::BrowserExtension::saveState( stream );
    else {
        m_dirPart->saveFindState( stream );
    }
}

void KonqDirPartBrowserExtension::restoreState( TQDataStream &stream )
{
    m_dirPart->restoreState( stream );
    bool hasFindPart;
    stream >> hasFindPart;
    assert( ! ( hasFindPart && !strcmp(m_dirPart->className(), "KFindPart") ) );
    if ( !hasFindPart )
        // This calls openURL, that's why we don't want to call it in case of a find part
        KParts::BrowserExtension::restoreState( stream );
    else {
        m_dirPart->restoreFindState( stream );
    }
}


void KonqDirPart::resetCount()
{
    m_lDirSize = 0;
    m_lFileCount = 0;
    m_lDirCount = 0;
    d->dirSizeDirty = true;
}

void KonqDirPart::setDirLister( KDirLister* lister )
{
    d->dirLister = lister;
}

#include "konq_dirpart.moc"
