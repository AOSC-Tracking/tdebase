/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000 - 2005 David Faure <faure@kde.org>

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
#include "konq_iconviewwidget.h"
#include "konq_operations.h"
#include "konq_undo.h"
#include "konq_sound.h"
#include "konq_filetip.h"

#include <tqclipboard.h>
#include <tqlayout.h>
#include <tqtimer.h>
#include <tqpainter.h>
#include <tqtooltip.h>
#include <tqlabel.h>
#include <tqmovie.h>
#include <tqregexp.h>
#include <tqcursor.h>

#include <tdeapplication.h>
#include <kdebug.h>
#include <tdeio/previewjob.h>
#include <tdefileivi.h>
#include <konq_settings.h>
#include <konq_drag.h>
#include <tdeglobalsettings.h>
#include <kpropertiesdialog.h>
#include <kipc.h>
#include <kicontheme.h>
#include <kiconeffect.h>
#include <kurldrag.h>
#include <tdestandarddirs.h>
#include <kprotocolinfo.h>
#include <ktrader.h>

#include <assert.h>
#include <unistd.h>
#include <tdelocale.h>


struct KonqIconViewWidgetPrivate
{
    KonqIconViewWidgetPrivate() {
        pActiveItem = 0;
        pRenamingItem = 0;
        bSoundPreviews = false;
        pSoundItem = 0;
        bSoundItemClicked = false;
        pSoundPlayer = 0;
        pSoundTimer = 0;
        pPreviewJob = 0;
        bAllowSetWallpaper = false;

        doAnimations = true;
        m_movie = 0L;
        m_movieBlocked = 0;
        pFileTip = 0;
        pActivateDoubleClick = 0L;
        bCaseInsensitive = true;
        pPreviewMimeTypes = 0L;
	bProgramsURLdrag = false;
    }
    ~KonqIconViewWidgetPrivate() {
        delete pSoundPlayer;
        delete pSoundTimer;
        delete m_movie;
        delete pFileTip;
        delete pActivateDoubleClick;
        delete pPreviewMimeTypes;
        //delete pPreviewJob; done by stopImagePreview
    }
    KFileIVI *pActiveItem;
    KFileIVI *pRenamingItem;
    // Sound preview
    KFileIVI *pSoundItem;
    KonqSoundPlayer *pSoundPlayer;
    TQTimer *pSoundTimer;
    bool bSoundPreviews;
    bool bSoundItemClicked;
    bool bAllowSetWallpaper;
    bool bCaseInsensitive;
    bool bBoostPreview;

    // Animated icons support
    bool doAnimations;
    TQMovie* m_movie;
    int m_movieBlocked;
    TQString movieFileName;

    TDEIO::PreviewJob *pPreviewJob;
    KonqFileTip* pFileTip;
    TQStringList previewSettings;
    bool renameItem;
    bool firstClick;
    bool releaseMouseEvent;
    TQPoint mousePos;
    int mouseState;
    TQTimer *pActivateDoubleClick;
    TQStringList* pPreviewMimeTypes;
    bool bProgramsURLdrag;
};

KonqIconViewWidget::KonqIconViewWidget( TQWidget * parent, const char * name, WFlags f, bool kdesktop )
    : TDEIconView( parent, name, f ),
      m_rootItem( 0L ), m_size( 0 ) /* default is DesktopIcon size */,
      m_bDesktop( kdesktop ),
      m_bSetGridX( !kdesktop ) /* No line breaking on the desktop */
{
    d = new KonqIconViewWidgetPrivate;
    connect( this, TQ_SIGNAL( dropped( TQDropEvent *, const TQValueList<TQIconDragItem> & ) ),
             this, TQ_SLOT( slotDropped( TQDropEvent*, const TQValueList<TQIconDragItem> & ) ) );

    connect( this, TQ_SIGNAL( selectionChanged() ),
             this, TQ_SLOT( slotSelectionChanged() ) );

    tdeApp->addKipcEventMask( KIPC::IconChanged );
    connect( tdeApp, TQ_SIGNAL(iconChanged(int)), TQ_SLOT(slotIconChanged(int)) );
    connect( this, TQ_SIGNAL(onItem(TQIconViewItem *)), TQ_SLOT(slotOnItem(TQIconViewItem *)) );
    connect( this, TQ_SIGNAL(onViewport()), TQ_SLOT(slotOnViewport()) );
    connect( this, TQ_SIGNAL(itemRenamed(TQIconViewItem *, const TQString &)), TQ_SLOT(slotItemRenamed(TQIconViewItem *, const TQString &)) );

    m_pSettings = KonqFMSettings::settings();  // already needed in setItemTextPos(), calculateGridX()
    d->bBoostPreview = boostPreview();

    // hardcoded settings
    setSelectionMode( TQIconView::Extended );
    setItemTextPos( TQIconView::Bottom );
    d->releaseMouseEvent = false;
    d->pFileTip = new KonqFileTip(this);
    d->firstClick = false;
    calculateGridX();
    setAutoArrange( true );
    setSorting( true, sortDirection() );
    readAnimatedIconsConfig();
    m_bSortDirsFirst = true;
    m_bMousePressed = false;
    m_LineupMode = LineupBoth;
    // emit our signals
    slotSelectionChanged();
    m_iconPositionGroupPrefix = TQString::fromLatin1( "IconPosition::" );
    KonqUndoManager::incRef();
}

KonqIconViewWidget::~KonqIconViewWidget()
{
    stopImagePreview();
    KonqUndoManager::decRef();
    delete d;
}

bool KonqIconViewWidget::maySetWallpaper()
{
    return d->bAllowSetWallpaper;
}

void KonqIconViewWidget::setMaySetWallpaper(bool b)
{
    d->bAllowSetWallpaper = b;
}

void KonqIconViewWidget::focusOutEvent( TQFocusEvent * ev )
{
    // We can't possibly have the mouse pressed and still lose focus.
    // Well, we can, but when we regain focus we should assume the mouse is
    // not down anymore or the slotOnItem code will break with highlighting!
    m_bMousePressed = false;

    // This will ensure that tooltips don't pop up and the mouseover icon
    // effect will go away if the mouse goes out of the view without
    // first moving into an empty portion of the view
    // Fixes part of #86968, and #85204
    // Matt Newell 2004-09-24
    slotOnViewport();

    TDEIconView::focusOutEvent( ev );
}

bool KonqIconViewWidget::isRenaming() const
{
    return d->pRenamingItem && d->pRenamingItem->m_renameBox;
}

const KFileIVI* KonqIconViewWidget::renamingItem()
{
    return d->pRenamingItem;
}

void KonqIconViewWidget::setRenamingItem(KFileIVI *theItem)
{
    d->pRenamingItem = theItem;
}

void KonqIconViewWidget::slotItemRenamed(TQIconViewItem *item, const TQString &name)
{
    kdDebug(1203) << "KonqIconViewWidget::slotItemRenamed" << endl;
    KFileIVI *viewItem = static_cast<KFileIVI *>(item);
    KFileItem *fileItem = viewItem->item();

    // The correct behavior is to show the old name until the rename has successfully
    // completed. Unfortunately, TDEIconView forces us to allow the text to be changed
    // before we try the rename, so set it back to the pre-rename state.
    viewItem->setText( fileItem->text() );
    kdDebug(1203)<<" fileItem->text() ;"<<fileItem->text()<<endl;
    // Don't do anything if the user renamed to a blank name.
    if( !name.isEmpty() )
    {
        // Actually attempt the rename. If it succeeds, KDirLister will update the name.
        KURL oldurl( fileItem->url() );
        KURL newurl( oldurl );
        newurl.setPath( newurl.directory(false) + TDEIO::encodeFileName( name ) );
        kdDebug(1203)<<" newurl :"<<newurl<<endl;
        // We use url()+name so that it also works if the name is a relative path (#51176)
        KonqOperations::rename( this, oldurl, newurl );
    }
}

void KonqIconViewWidget::slotIconChanged( int group )
{
    if (group != TDEIcon::Desktop)
        return;

    int size = m_size;
    if ( m_size == 0 )
      m_size = -1; // little trick to force grid change in setIcons
    setIcons( size ); // force re-determining all icons
    readAnimatedIconsConfig();
}

void KonqIconViewWidget::readAnimatedIconsConfig()
{
    TDEConfigGroup cfgGroup( TDEGlobal::config(), "DesktopIcons" );
    d->doAnimations = cfgGroup.readBoolEntry( "Animated", true /*default*/ );
}

void KonqIconViewWidget::slotOnItem( TQIconViewItem *_item )
{
    KFileIVI* item = static_cast<KFileIVI *>( _item );
    // Reset icon of previous item
    if( d->pActiveItem != 0L && d->pActiveItem != item )
    {
        if ( d->m_movie && d->pActiveItem->isAnimated() )
        {
            d->m_movie->pause(); // we'll see below what we do with it
            d->pActiveItem->setAnimated( false );
            d->pActiveItem->refreshIcon( true );
        }
        else {
            d->pActiveItem->setActive( false );
        }
        d->pActiveItem = 0L;
        d->pFileTip->setItem( 0L );
    }

    // Stop sound
    if (d->pSoundPlayer != 0 && item != d->pSoundItem)
    {
        d->pSoundPlayer->stop();

        d->pSoundItem = 0;
        if (d->pSoundTimer && d->pSoundTimer->isActive())
            d->pSoundTimer->stop();
    }

    if ( !m_bMousePressed )
    {
        if( item != d->pActiveItem )
        {
            d->pActiveItem = item;
            d->pFileTip->setItem( d->pActiveItem->item(),
                                  item->rect(),
                                  item->pixmap() );

            if ( d->doAnimations && d->pActiveItem && d->pActiveItem->hasAnimation() )
            {
                //kdDebug(1203) << "Playing animation for: " << d->pActiveItem->mouseOverAnimation() << endl;
                // Check if cached movie can be used
#if 0 // Qt-mng bug, reusing the movie doesn't work currently.
                if ( d->m_movie && d->movieFileName == d->pActiveItem->mouseOverAnimation() )
                {
                    d->pActiveItem->setAnimated( true );
                    if (d->m_movieBlocked) {
                        kdDebug(1203) << "onitem, but blocked" << endl;
                        d->m_movie->pause();
                    }
                    else {
                        kdDebug(1203) << "we go ahead.." << endl;
                        d->m_movieBlocked++;
                        TQTimer::singleShot(300, this, TQ_SLOT(slotReenableAnimation()));
                        d->m_movie->restart();
                        d->m_movie->unpause();
                    }
                }
                else
#endif
                {
                    TQMovie movie = TDEGlobal::iconLoader()->loadMovie( d->pActiveItem->mouseOverAnimation(), TDEIcon::Desktop, d->pActiveItem->iconSize() );
                    if ( !movie.isNull() )
                    {
                        delete d->m_movie;
                        d->m_movie = new TQMovie( movie ); // shallow copy, don't worry
                        // Fix alpha-channel - currently only if no background pixmap,
                        // the bg pixmap case requires to uncomment the code at qmovie.cpp:404
                        const TQPixmap* pm = backgroundPixmap();
                        bool hasPixmap = pm && !pm->isNull();
                        if ( !hasPixmap ) {
                            pm = viewport()->backgroundPixmap();
                            hasPixmap = pm && !pm->isNull();
                        }
                        if (!hasPixmap && backgroundMode() != NoBackground)
                           d->m_movie->setBackgroundColor( viewport()->backgroundColor() );
                        d->m_movie->connectUpdate( this, TQ_SLOT( slotMovieUpdate(const TQRect &) ) );
                        d->m_movie->connectStatus( this, TQ_SLOT( slotMovieStatus(int) ) );
                        d->movieFileName = d->pActiveItem->mouseOverAnimation();
                        d->pActiveItem->setAnimated( true );
                    }
                    else
                    {
                        d->pActiveItem->setAnimated( false );
                        if (d->m_movie)
                            d->m_movie->pause();
                        // No movie available, remember it
                        d->pActiveItem->setMouseOverAnimation( TQString::null );
                    }
                }
            } // animations
            // Only do the normal "mouseover" effect if no animation is in use
            if (d->pActiveItem && !d->pActiveItem->isAnimated())
            {
                d->pActiveItem->setActive( true );
            }
        }
        else // No change in current item
        {
            // No effect. If we want to underline on hover, we should
            // force the IVI to repaint here, though!
            d->pActiveItem = 0L;
            d->pFileTip->setItem( 0L );
        }
    } // bMousePressed
    else
    {
        // All features disabled during mouse clicking, e.g. rectangular
        // selection
        d->pActiveItem = 0L;
        d->pFileTip->setItem( 0L );
    }

    // ## shouldn't this be disabled during rectangular selection too ?
    if (d->bSoundPreviews && d->pSoundPlayer &&
        d->pSoundPlayer->mimeTypes().contains(
            item->item()->mimetype())
        && TDEGlobalSettings::showFilePreview(item->item()->url())
        && topLevelWidget() == tdeApp->activeWindow())
    {
        d->pSoundItem = item;
        d->bSoundItemClicked = false;
        if (!d->pSoundTimer)
        {
            d->pSoundTimer = new TQTimer(this);
            connect(d->pSoundTimer, TQ_SIGNAL(timeout()), TQ_SLOT(slotStartSoundPreview()));
        }
        if (d->pSoundTimer->isActive())
            d->pSoundTimer->stop();
        d->pSoundTimer->start(500, true);
    }
    else
    {
        if (d->pSoundPlayer)
            d->pSoundPlayer->stop();
        d->pSoundItem = 0;
        if (d->pSoundTimer && d->pSoundTimer->isActive())
            d->pSoundTimer->stop();
    }
}

void KonqIconViewWidget::slotOnViewport()
{
    d->pFileTip->setItem( 0L );

    if (d->pSoundPlayer)
      d->pSoundPlayer->stop();
    d->pSoundItem = 0;
    if (d->pSoundTimer && d->pSoundTimer->isActive())
      d->pSoundTimer->stop();

    if (d->pActiveItem == 0L)
        return;

    if ( d->doAnimations && d->m_movie && d->pActiveItem->isAnimated() )
    {
        d->pActiveItem->setAnimated( false );
#if 0
        // Aborting before the end of the animation ?
        if (d->m_movie->running()) {
            d->m_movie->pause();
            d->m_movieBlocked++;
            kdDebug(1203) << "on viewport, blocking" << endl;
            TQTimer::singleShot(300, this, TQ_SLOT(slotReenableAnimation()));
        }
#endif
        d->pActiveItem->refreshIcon( true );
        Q_ASSERT( d->pActiveItem->state() == TDEIcon::DefaultState );
        //delete d->m_movie;
        //d->m_movie = 0L;
        // TODO a timer to delete the movie after some time if unused?
    }
    else
    {
        d->pActiveItem->setActive( false );
    }
    d->pActiveItem = 0L;
}

void KonqIconViewWidget::slotStartSoundPreview()
{
  if (!d->pSoundItem || d->bSoundItemClicked)
    return;

  d->pSoundPlayer->play(d->pSoundItem->item()->url().url());
}


void KonqIconViewWidget::slotPreview(const KFileItem *item, const TQPixmap &pix)
{
    // ### slow. Idea: move KonqKfmIconView's m_itemDict into this class
    for (TQIconViewItem *it = firstItem(); it; it = it->nextItem())
    {
        KFileIVI* current = static_cast<KFileIVI *>(it);
        if (current->item() == item)
        {
            if (item->overlays() & TDEIcon::HiddenOverlay) {
                TQPixmap p(pix);

                TDEIconEffect::semiTransparent(p);
                current->setThumbnailPixmap(p);
            } else {
                current->setThumbnailPixmap(pix);
            }
            break;
        }
    }
}

void KonqIconViewWidget::slotPreviewResult()
{
    d->pPreviewJob = 0;
    emit imagePreviewFinished();
}

void KonqIconViewWidget::slotToolTipPreview(const KFileItem* , const TQPixmap &)
{
// unused - remove for KDE4
}

void KonqIconViewWidget::slotToolTipPreviewResult()
{
// unused - remove for KDE4
}

void KonqIconViewWidget::slotMovieUpdate( const TQRect& rect )
{
    //kdDebug(1203) << "KonqIconViewWidget::slotMovieUpdate " << rect.x() << "," << rect.y() << " " << rect.width() << "x" << rect.height() << endl;
    Q_ASSERT( d );
    Q_ASSERT( d->m_movie );
    // seems stopAnimation triggers one last update
    if ( d->pActiveItem && d->m_movie && d->pActiveItem->isAnimated() ) {
        const TQPixmap &frame = d->m_movie->framePixmap();
        // This can happen if the icon was scaled to the desired size, so TDEIconLoader
        // will happily return a movie with different dimensions than the icon
        int iconSize=d->pActiveItem->iconSize();
        if (iconSize==0) iconSize = TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
        if ( frame.width() != iconSize || frame.height() != iconSize ) {
            d->pActiveItem->setAnimated( false );
            d->m_movie->pause();
            // No movie available, remember it
            d->pActiveItem->setMouseOverAnimation( TQString::null );
            d->pActiveItem->setActive( true );
            return;
        }
        d->pActiveItem->setPixmapDirect( frame, false, false /*no redraw*/ );
        TQRect pixRect = d->pActiveItem->pixmapRect(false);
        repaintContents( pixRect.x() + rect.x(), pixRect.y() + rect.y(), rect.width(), rect.height(), false );
    }
}

void KonqIconViewWidget::slotMovieStatus( int status )
{
    if ( status < 0 ) {
        // Error playing the MNG -> forget about it and do normal iconeffect
        if ( d->pActiveItem && d->pActiveItem->isAnimated() ) {
            d->pActiveItem->setAnimated( false );
            d->pActiveItem->setMouseOverAnimation( TQString::null );
            d->pActiveItem->setActive( true );
        }
    }
}

void KonqIconViewWidget::slotReenableAnimation()
{
    if (!--d->m_movieBlocked) {
        if ( d->pActiveItem && d->m_movie && d->m_movie->paused()) {
            kdDebug(1203) << "reenabled animation" << endl;
            d->m_movie->restart();
            d->m_movie->unpause();
        }
    }
}

void KonqIconViewWidget::clear()
{
    d->pFileTip->setItem( 0L );
    stopImagePreview(); // Just in case
    TDEIconView::clear();
    d->pActiveItem = 0L;
}

void KonqIconViewWidget::takeItem( TQIconViewItem *item )
{
    if ( d->pActiveItem == static_cast<KFileIVI *>(item) )
    {
        d->pFileTip->setItem( 0L );
        d->pActiveItem = 0L;
    }

    if ( d->pPreviewJob )
      d->pPreviewJob->removeItem( static_cast<KFileIVI *>(item)->item() );

    TDEIconView::takeItem( item );
}

// Currently unused - remove in KDE 4.0
void KonqIconViewWidget::setThumbnailPixmap( KFileIVI * item, const TQPixmap & pixmap )
{
    if ( item )
    {
        if ( d->pActiveItem == item )
        {
            d->pFileTip->setItem( 0L );
            d->pActiveItem = 0L;
        }
        item->setThumbnailPixmap( pixmap );
        if ( m_bSetGridX &&  item->width() > gridX() )
        {
          setGridX( item->width() );
          if (autoArrange())
            arrangeItemsInGrid();
        }
    }
}

bool KonqIconViewWidget::initConfig( bool bInit )
{
    bool fontChanged = false;

    // Color settings
    TQColor normalTextColor       = m_pSettings->normalTextColor();
    setItemColor( normalTextColor );

    if (m_bDesktop)
    {
      TQColor itemTextBg = m_pSettings->itemTextBackground();
      if ( itemTextBg.isValid() )
          setItemTextBackground( itemTextBg );
      else
          setItemTextBackground( TQt::NoBrush );
    }

    bool on = m_pSettings->showFileTips() && TQToolTip::isGloballyEnabled();
    d->pFileTip->setOptions(on,
                            m_pSettings->showPreviewsInFileTips(),
                            m_pSettings->numFileTips());

    // if the user wants our own tooltip, don't show the one from Qts ListView
    setShowToolTips(!on);

    // Font settings
    TQFont font( m_pSettings->standardFont() );
    if (!m_bDesktop)
        font.setUnderline( m_pSettings->underlineLink() );

    if ( font != KonqIconViewWidget::font() )
    {
        setFont( font );
        if (!bInit)
        {
            // TQIconView doesn't do it by default... but if the font is made much
            // bigger, we really need to give more space between the icons
            fontChanged = true;
        }
    }

    setIconTextHeight( m_pSettings->iconTextHeight() );

    if ( (itemTextPos() == TQIconView::Right) && (maxItemWidth() != gridXValue()) )
    {
        int size = m_size;
        m_size = -1; // little trick to force grid change in setIcons
        setIcons( size ); // force re-determining all icons
    }
    else if ( d->bBoostPreview != boostPreview() ) // Update icons if settings for preview icon size have changed
        setIcons(m_size);
    else if (!bInit)
        updateContents();
    return fontChanged;
}

bool KonqIconViewWidget::boostPreview() const
{
    if ( m_bDesktop ) return false;

    TDEConfigGroup group( TDEGlobal::config(), "PreviewSettings" );
    return group.readBoolEntry( "BoostSize", false );
}

void KonqIconViewWidget::disableSoundPreviews()
{
    d->bSoundPreviews = false;

    if (d->pSoundPlayer)
      d->pSoundPlayer->stop();
    d->pSoundItem = 0;
    if (d->pSoundTimer && d->pSoundTimer->isActive())
      d->pSoundTimer->stop();
}

void KonqIconViewWidget::setIcons( int size, const TQStringList& stopImagePreviewFor )
{
    // size has changed?
    bool sizeChanged = (m_size != size);
    int oldGridX = gridX();
    m_size = size;

    // boost preview option has changed?
    bool boost = boostPreview();
    bool previewSizeChanged = ( d->bBoostPreview != boost );
    d->bBoostPreview = boost;

    if ( sizeChanged || previewSizeChanged )
    {
        int realSize = size ? size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
        // choose spacing depending on font, but min 5 (due to KFileIVI  move limit)
        setSpacing( ( m_bDesktop || ( realSize > TDEIcon::SizeSmall ) ) ?
                    TQMAX( spacing(), TQFontMetrics(font()).width('n') ) : 0 );
    }

    if ( sizeChanged || previewSizeChanged || !stopImagePreviewFor.isEmpty() )
    {
        calculateGridX();
    }
    bool stopAll = !stopImagePreviewFor.isEmpty() && stopImagePreviewFor.first() == "*";

    // Disable repaints that can be triggered by ivi->setIcon(). Since icons are
    // resized in-place, if the icon size is increasing it can happens that the right
    // or bottom icons exceed the size of the viewport.. here we prevent the repaint
    // event that will be triggered in that case.
    bool prevUpdatesState = viewport()->isUpdatesEnabled();
    viewport()->setUpdatesEnabled( false );

    // Do this even if size didn't change, since this is used by refreshMimeTypes...
    for ( TQIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
        KFileIVI * ivi = static_cast<KFileIVI *>( it );
        // Set a normal icon for files that are not thumbnails, and for files
        // that are thumbnails but for which it should be stopped
        if ( !ivi->isThumbnail() ||
             sizeChanged ||
             previewSizeChanged ||
             stopAll ||
             mimeTypeMatch( ivi->item()->mimetype(), stopImagePreviewFor ) )
        {
            ivi->setIcon( size, ivi->state(), true, false );
        }
        else
            ivi->invalidateThumb( ivi->state(), true );
    }

    // Restore viewport update to previous state
    viewport()->setUpdatesEnabled( prevUpdatesState );

    if ( ( sizeChanged || previewSizeChanged || oldGridX != gridX() ||
         !stopImagePreviewFor.isEmpty() ) && autoArrange() )
        arrangeItemsInGrid( true ); // take new grid into account and repaint
    else
        viewport()->update(); //Repaint later..
}

bool KonqIconViewWidget::mimeTypeMatch( const TQString& mimeType, const TQStringList& mimeList ) const
{
    // Code duplication from TDEIO::PreviewJob
    KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
    for (TQStringList::ConstIterator mt = mimeList.begin(); mt != mimeList.end(); ++mt)
    {
        if ( mime->is( *mt ) )
            return true;
        // Support for *mt == "image/*"
        TQString tmp( mimeType );
        if ( (*mt).endsWith("*") && tmp.replace(TQRegExp("/.*"), "/*") == (*mt) )
            return true;
        if ( (*mt) == "text/plain" )
        {
            TQVariant textProperty = mime->property( "X-TDE-text" );
            if ( textProperty.type() == TQVariant::Bool && textProperty.toBool() )
                return true;
        }
    }
    return false;
}

void KonqIconViewWidget::setItemTextPos( ItemTextPos pos )
{
    // can't call gridXValue() because this already would need the new itemTextPos()
    int sz = m_size ? m_size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );

    if ( m_bSetGridX ) {
        if ( pos == TQIconView::Bottom ) {
            setGridX( TQMAX( sz + 50, previewIconSize( sz ) + 13 ) );
        }
        else
        {
            setMaxItemWidth( TQMAX( sz, previewIconSize( sz ) ) + m_pSettings->iconTextWidth() );
            setGridX( -1 );
        }
    }

    TDEIconView::setItemTextPos( pos );
}

void KonqIconViewWidget::gridValues( int* x, int* y, int* dx, int* dy,
                                     int* nx, int* ny )
{
    int previewSize = previewIconSize( m_size );
    int iconSize = m_size ? m_size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );

    // Grid size
    // as KFileIVI limits to move an icon to x >= 5, y >= 5, we define a grid cell as:
    // spacing() must be >= 5 (currently set to 5 in setIcons())
    // horizontal: left spacing() + <width>
    // vertical  : top spacing(), <height>, bottom spacing()
    // The doubled space in y-direction gives a better visual separation and makes it clearer
    // to which item the text belongs
    *dx = spacing() + TQMAX( TQMAX( iconSize, previewSize ), m_pSettings->iconTextWidth() );
    int textHeight = iconTextHeight() * fontMetrics().height();
    *dy = spacing() + TQMAX( iconSize, previewSize ) + 2 + textHeight + spacing();

    // Icon Area
    int w, h;
    if ( m_IconRect.isValid() ) {  // w and h must be != 0, otherwise we would get a div by zero
        *x = m_IconRect.left(); w = m_IconRect.width();
        *y = m_IconRect.top();  h = m_IconRect.height();
    }
    else {
        *x = 0; w = viewport()->width();
        *y = 0; h = viewport()->height();
    }

    // bug:110775 avoid div by zero (happens e.g. when iconTextHeight or iconTextWidth are very large)
    if ( *dx > w )
        *dx = w;

    if ( *dy > h )
        *dy = h;

    *nx = w / *dx;
    *ny = h / *dy;
    // TODO: Check that items->count() <= nx * ny

    // Let have exactly nx columns and ny rows
    if(*nx && *ny) {
      *dx = w / *nx;
      *dy = h / *ny;
    }
    kdDebug(1203) << "x=" << *x << " y=" << *y << " spacing=" << spacing() << " iconSize=" << iconSize
                  << " w=" << w << " h=" << h
                  << " nx=" << *nx << " ny=" << *ny
                  << " dx=" << *dx << " dy=" << *dy << endl;
}

void KonqIconViewWidget::calculateGridX()
{
    if ( m_bSetGridX ) {
        if ( itemTextPos() == TQIconView::Bottom ) {
            setGridX( gridXValue() );
        }
        else
        {
            setMaxItemWidth( gridXValue() );
            setGridX( -1 );
        }
    }
}

int KonqIconViewWidget::gridXValue() const
{
    // this method is only used in konqi as filemanager (not desktop)
    int sz = m_size ? m_size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
    int newGridX;

    if ( itemTextPos() == TQIconView::Bottom )
        newGridX = TQMAX( sz + 50, previewIconSize( sz ) + 13 );
    else
        newGridX = TQMAX( sz, previewIconSize( sz ) ) + m_pSettings->iconTextWidth();

    //kdDebug(1203) << "gridXValue: " << newGridX << " sz=" << sz << endl;
    return newGridX;
}

void KonqIconViewWidget::refreshMimeTypes()
{
    updatePreviewMimeTypes();
    for ( TQIconViewItem *it = firstItem(); it; it = it->nextItem() )
        (static_cast<KFileIVI *>( it ))->item()->refreshMimeType();
    setIcons( m_size );
}

void KonqIconViewWidget::setURL( const KURL &kurl )
{
    stopImagePreview();
    m_url = kurl;

    d->pFileTip->setPreview( TDEGlobalSettings::showFilePreview(m_url) );

    if ( m_url.isLocalFile() )
        m_dotDirectoryPath = m_url.path(1).append( ".directory" );
    else
        m_dotDirectoryPath = TQString::null;
}

void KonqIconViewWidget::startImagePreview( const TQStringList &, bool force )
{
    stopImagePreview(); // just in case

    // Check config
    if ( !TDEGlobalSettings::showFilePreview( url() ) ) {
        kdDebug(1203) << "Previews disabled for protocol " << url().protocol() << endl;
        emit imagePreviewFinished();
        return;
    }

    if ((d->bSoundPreviews = d->previewSettings.contains( "audio/" )) &&
        !d->pSoundPlayer)
    {
      KLibFactory *factory = KLibLoader::self()->factory("konq_sound");
      if (factory)
        d->pSoundPlayer = static_cast<KonqSoundPlayer *>(
          factory->create(this, 0, "KonqSoundPlayer"));
      d->bSoundPreviews = (d->pSoundPlayer != 0L);
    }

    KFileItemList items;
    for ( TQIconViewItem *it = firstItem(); it; it = it->nextItem() )
        if ( force || !static_cast<KFileIVI *>( it )->hasValidThumbnail() )
            items.append( static_cast<KFileIVI *>( it )->item() );

    bool onlyAudio = true;
    for ( TQStringList::ConstIterator it = d->previewSettings.begin(); it != d->previewSettings.end(); ++it ) {
        if ( (*it).startsWith( "audio/" ) )
            d->bSoundPreviews = true;
        else
            onlyAudio = false;
    }

    if ( items.isEmpty() || onlyAudio ) {
        emit imagePreviewFinished();
        return; // don't start the preview job if not really necessary
    }

    int iconSize = m_size ? m_size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );
    int size;

    d->bBoostPreview = boostPreview();
    size = previewIconSize( iconSize );

    if ( !d->bBoostPreview )
         iconSize /= 2;

    d->pPreviewJob = TDEIO::filePreview( items, size, size, iconSize,
        m_pSettings->textPreviewIconTransparency(), true /* scale */,
        true /* save */, &(d->previewSettings) );
    connect( d->pPreviewJob, TQ_SIGNAL( gotPreview( const KFileItem *, const TQPixmap & ) ),
             this, TQ_SLOT( slotPreview( const KFileItem *, const TQPixmap & ) ) );
    connect( d->pPreviewJob, TQ_SIGNAL( result( TDEIO::Job * ) ),
             this, TQ_SLOT( slotPreviewResult() ) );
}

void KonqIconViewWidget::stopImagePreview()
{
    if (d->pPreviewJob)
    {
        d->pPreviewJob->kill();
        d->pPreviewJob = 0;
        // Now that previews are updated in-place, calling
        // arrangeItemsInGrid() here is not needed anymore
    }
}

bool KonqIconViewWidget::isPreviewRunning() const
{
    return d->pPreviewJob;
}

KFileItemList KonqIconViewWidget::selectedFileItems()
{
    KFileItemList lstItems;

    TQIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() )
        if ( it->isVisible() && it->isSelected() ) {
            KFileItem *fItem = (static_cast<KFileIVI *>(it))->item();
            lstItems.append( fItem );
        }
    return lstItems;
}

void KonqIconViewWidget::slotDropped( TQDropEvent *ev, const TQValueList<TQIconDragItem> & )
{
    // Drop on background
    KURL dirURL = url();
    if ( m_rootItem ) {
        bool dummy;
        dirURL = m_rootItem->mostLocalURL(dummy);
    }
    KonqOperations::doDrop( m_rootItem /* may be 0L */, dirURL, ev, this );
}

void KonqIconViewWidget::slotAboutToCreate(const TQPoint &, const TQValueList<TDEIO::CopyInfo> &)
{
   // Do nothing :-)
}

void KonqIconViewWidget::drawBackground( TQPainter *p, const TQRect &r )
{
    drawBackground(p, r, r.topLeft());
}

void KonqIconViewWidget::drawBackground( TQPainter *p, const TQRect &r , const TQPoint &pt)
{
    const TQPixmap *pm  = backgroundPixmap();
    bool hasPixmap = pm && !pm->isNull();
    if ( !hasPixmap ) {
        pm = viewport()->backgroundPixmap();
        hasPixmap = pm && !pm->isNull();
    }

    TQRect rtgt(r);
    rtgt.moveTopLeft(pt);
    if (!hasPixmap && backgroundMode() != NoBackground) {
        p->fillRect(rtgt, viewport()->backgroundColor());
        return;
    }

    if (hasPixmap) {
        int ax = (r.x() + contentsX() + leftMargin()) % pm->width();
        int ay = (r.y() + contentsY() + topMargin()) % pm->height();
        p->drawTiledPixmap(rtgt, *pm, TQPoint(ax, ay));
    }
}

TQDragObject * KonqIconViewWidget::dragObject()
{
    if ( !currentItem() )
        return 0;

    return konqDragObject( viewport() );
}

KonqIconDrag * KonqIconViewWidget::konqDragObject( TQWidget * dragSource )
{
    //kdDebug(1203) << "KonqIconViewWidget::konqDragObject" << endl;

    KonqIconDrag2 * drag = new KonqIconDrag2( dragSource );
    TQIconViewItem *primaryItem = currentItem();
    // Append all items to the drag object
    for ( TQIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
        if ( it->isVisible() && it->isSelected() ) {
          if (!primaryItem)
             primaryItem = it;
          KFileItem* fileItem = (static_cast<KFileIVI *>(it))->item();
          KURL url = fileItem->url();
          bool dummy;
          KURL mostLocalURL = fileItem->mostLocalURL(dummy);
          TQString itemURL = KURLDrag::urlToString(url);
          kdDebug(1203) << "itemURL=" << itemURL << endl;
          TQIconDragItem id;
          id.setData( TQCString(itemURL.latin1()) );
          drag->append( id,
                        TQRect( it->pixmapRect(false).topLeft() - m_mousePos,
                               it->pixmapRect().size() ),
                        TQRect( it->textRect(false).topLeft() - m_mousePos,
                               it->textRect().size() ),
                        itemURL, mostLocalURL );
        }
    }

    if (primaryItem)
       drag->setPixmap( *primaryItem->pixmap(), m_mousePos - primaryItem->pixmapRect(false).topLeft() );

    return drag;
}

void KonqIconViewWidget::contentsDragEnterEvent( TQDragEnterEvent *e )
{
    if ( e->provides( "text/uri-list" ) )
    {
        TQByteArray payload = e->encodedData( "text/uri-list" );
        if ( !payload.size() )
            kdError() << "Empty data !" << endl;
        // Cache the URLs, since we need them every time we move over a file
        // (see KFileIVI)
        bool ok = KURLDrag::decode( e, m_lstDragURLs );
        if( !ok )
            kdError() << "Couldn't decode urls dragged !" << endl;
    }

    KURL::List uriList;
    if ( KURLDrag::decode(e, uriList) )
    {
        if ( uriList.first().protocol() == "programs" )
        {
            e->ignore();
            emit dragEntered( false );
            d->bProgramsURLdrag = true;
            return;
        }
    }

    TDEIconView::contentsDragEnterEvent( e );
    emit dragEntered( true /*accepted*/ );
}

void KonqIconViewWidget::contentsDragMoveEvent( TQDragMoveEvent *e )
{
    if ( d->bProgramsURLdrag ) {
        emit dragMove( false );
        e->ignore();
        cancelPendingHeldSignal();
        return;
    }

    TQIconViewItem *item = findItem( e->pos() );
    if ( e->source() != viewport() &&
         !item && m_rootItem && !m_rootItem->isWritable() ) {
        emit dragMove( false );
        e->ignore();
        cancelPendingHeldSignal();
        return;
    }
    emit dragMove( true );
    TDEIconView::contentsDragMoveEvent( e );
}

void KonqIconViewWidget::contentsDragLeaveEvent( TQDragLeaveEvent *e )
{
    d->bProgramsURLdrag = false;
    TDEIconView::contentsDragLeaveEvent(e);
    emit dragLeft();
}


void KonqIconViewWidget::setItemColor( const TQColor &c )
{
    iColor = c;
}

TQColor KonqIconViewWidget::itemColor() const
{
    return iColor;
}

void KonqIconViewWidget::disableIcons( const KURL::List & lst )
{
  for ( TQIconViewItem *kit = firstItem(); kit; kit = kit->nextItem() )
  {
      bool bFound = false;
      // Wow. This is ugly. Matching two lists together....
      // Some sorting to optimise this would be a good idea ?
      for (KURL::List::ConstIterator it = lst.begin(); !bFound && it != lst.end(); ++it)
      {
          if ( static_cast<KFileIVI *>( kit )->item()->url() == *it )
          {
              bFound = true;
              // maybe remove "it" from lst here ?
          }
      }
      static_cast<KFileIVI *>( kit )->setDisabled( bFound );
  }
}

void KonqIconViewWidget::slotSelectionChanged()
{
    // This code is very related to ListViewBrowserExtension::updateActions
    int canCopy = 0;
    int canDel = 0;
    int canTrash = 0;
    bool bInTrash = false;
    int iCount = 0;

    for ( TQIconViewItem *it = firstItem(); it; it = it->nextItem() )
    {
        if ( it->isVisible() && it->isSelected() )
        {
            iCount++;
            canCopy++;

            KFileItem *item = ( static_cast<KFileIVI *>( it ) )->item();
            KURL url = item->url();
            TQString local_path = item->localPath();

            if ( url.directory(false) == TDEGlobalSettings::trashPath() ) {
                bInTrash = true;
            }
            if ( KProtocolInfo::supportsDeleting( url ) ) {
                canDel++;
            }
            if ( !local_path.isEmpty() ) {
                canTrash++;
            }
        }
    }

    emit enableAction( "cut", canDel > 0 );
    emit enableAction( "copy", canCopy > 0 );
    emit enableAction( "trash", canDel > 0 && !bInTrash && canTrash==canDel );
    emit enableAction( "del", canDel > 0 );
    emit enableAction( "properties", iCount > 0 && KPropertiesDialog::canDisplay( selectedFileItems() ) );
    emit enableAction( "editMimeType", ( iCount == 1 ) );
    emit enableAction( "rename", ( iCount == 1) && !bInTrash );
}

void KonqIconViewWidget::renameCurrentItem()
{
    if (currentItem())
    {
        static_cast<KFileIVI *>(currentItem())->rename();
    }
}

void KonqIconViewWidget::renameSelectedItem()
{
    kdDebug(1203) << " -- KonqIconViewWidget::renameSelectedItem() -- " << endl;
    TQIconViewItem *item = 0L;
    TQIconViewItem *it = firstItem();
    for (; it; it = it->nextItem() )
        if ( it->isVisible() && it->isSelected() && !item )
        {
            item = it;
            break;
        }
    if (!item)
    {
        Q_ASSERT(item);
        return;
    }
    static_cast<KFileIVI *>(item)->rename();
}

void KonqIconViewWidget::cutSelection()
{
    kdDebug(1203) << " -- KonqIconViewWidget::cutSelection() -- " << endl;
    KonqIconDrag * obj = konqDragObject( /* no parent ! */ );
    obj->setMoveSelection( true );
    TQApplication::clipboard()->setData( obj );
}

void KonqIconViewWidget::copySelection()
{
    kdDebug(1203) << " -- KonqIconViewWidget::copySelection() -- " << endl;
    KonqIconDrag * obj = konqDragObject( /* no parent ! */ );
    TQApplication::clipboard()->setData( obj );
}

void KonqIconViewWidget::pasteSelection()
{
    paste( url() );
}

void KonqIconViewWidget::paste( const KURL &url )
{
    KonqOperations::doPaste( this, url );
}

KURL::List KonqIconViewWidget::selectedUrls()
{
    return selectedUrls( UserVisibleUrls );
}

KURL::List KonqIconViewWidget::selectedUrls( UrlFlags flags ) const
{
    KURL::List lstURLs;
    bool dummy;
    for ( TQIconViewItem *it = firstItem(); it; it = it->nextItem() )
        if ( it->isVisible() && it->isSelected() ) {
            KFileItem* item = (static_cast<KFileIVI *>( it ))->item();
            lstURLs.append( flags == MostLocalUrls ? item->mostLocalURL( dummy ) : item->url() );
        }
    return lstURLs;
}

TQRect KonqIconViewWidget::iconArea() const
{
    return m_IconRect;
}

void KonqIconViewWidget::setIconArea(const TQRect &rect)
{
    m_IconRect = rect;
}

int KonqIconViewWidget::lineupMode() const
{
    return m_LineupMode;
}

void KonqIconViewWidget::setLineupMode(int mode)
{
    m_LineupMode = mode;
}

bool KonqIconViewWidget::sortDirectoriesFirst() const
{
  return m_bSortDirsFirst;
}

void KonqIconViewWidget::setSortDirectoriesFirst( bool b )
{
  m_bSortDirsFirst = b;
}

void KonqIconViewWidget::contentsMouseMoveEvent( TQMouseEvent *e )
{
    if ( (d->pSoundPlayer && d->pSoundPlayer->isPlaying()) || (d->pSoundTimer && d->pSoundTimer->isActive()))
    {
        // The following call is SO expensive (the ::widgetAt call eats up to 80%
        // of the mouse move cpucycles!), so it's mandatory to place that function
        // under strict checks, such as d->pSoundPlayer->isPlaying()
        if ( TQApplication::widgetAt( TQCursor::pos() ) != topLevelWidget() )
        {
            if (d->pSoundPlayer)
                d->pSoundPlayer->stop();
            d->pSoundItem = 0;
            if (d->pSoundTimer && d->pSoundTimer->isActive())
                d->pSoundTimer->stop();
        }
    }
    d->renameItem= false;
    TDEIconView::contentsMouseMoveEvent( e );
}

void KonqIconViewWidget::contentsDropEvent( TQDropEvent * ev )
{
  TQIconViewItem *i = findItem( ev->pos() );
  KURL::List uriList;

    if ( ev->source() != viewport() &&
         !i && m_rootItem && !m_rootItem->isWritable() ) {
        ev->accept( false );
        return;
    }

  // Short-circuit TQIconView if Ctrl is pressed, so that it's possible
  // to drop a file into its own parent widget to copy it.
  if ( !i && (ev->action() == TQDropEvent::Copy || ev->action() == TQDropEvent::Link)
          && ev->source() && ev->source() == viewport()
          && KURLDrag::decode(ev, uriList) && !uriList.isEmpty()
          && uriList.first().upURL().url(1) == url().url(1))
  {
    // First we need to call TQIconView though, to clear the drag shape
    bool bMovable = itemsMovable();
    setItemsMovable(false); // hack ? call it what you want :-)
    TDEIconView::contentsDropEvent( ev );
    setItemsMovable(bMovable);

    TQValueList<TQIconDragItem> lst;
    slotDropped(ev, lst);
  }
  else
  {
    TDEIconView::contentsDropEvent( ev );
    emit dropped(); // What is this for ? (David)      KDE4: remove
  }
  // Don't do this here, it's too early !
  // slotSaveIconPositions();
  // If we want to save after the new file gets listed, though,
  // we could reimplement contentsDropEvent in KDIconView and set m_bNeedSave. Bah.

  // This signal is sent last because we need to ensure it is
  // taken in account when all the slots triggered by the dropped() signal
  // are executed. This way we know that the Drag and Drop is truely finished
  emit dragFinished();
}

void KonqIconViewWidget::doubleClickTimeout()
{
    d->renameItem= true;
    mousePressChangeValue();
    if ( d->releaseMouseEvent )
    {
        TQMouseEvent e( TQEvent::MouseButtonPress,d->mousePos , 1, d->mouseState);
        TQIconViewItem* item = findItem( e.pos() );
        KURL url;
        if ( item )
        {
            url= ( static_cast<KFileIVI *>( item ) )->item()->url();
            bool brenameTrash =false;
            if ( url.isLocalFile() && (url.directory(false) == TDEGlobalSettings::trashPath() || url.path(1).startsWith(TDEGlobalSettings::trashPath())))
                brenameTrash = true;

            if ( url.isLocalFile() && !brenameTrash && d->renameItem && m_pSettings->renameIconDirectly() && e.button() == TQt::LeftButton && item->textRect( false ).contains(e.pos()))
            {
                if( d->pActivateDoubleClick->isActive () )
                    d->pActivateDoubleClick->stop();
                item->rename();
                m_bMousePressed = false;
            }
        }
    }
    else
    {
        TQMouseEvent e( TQEvent::MouseMove,d->mousePos , 1, d->mouseState);
        TDEIconView::contentsMousePressEvent( &e );
    }
    if( d->pActivateDoubleClick->isActive() )
        d->pActivateDoubleClick->stop();

    d->releaseMouseEvent = false;
    d->renameItem= false;
}

void KonqIconViewWidget::wheelEvent(TQWheelEvent* e)
{
    // when scrolling with mousewheel, stop possible pending filetip
    d->pFileTip->setItem( 0 );

    if (e->state() == ControlButton)
    {
        if (e->delta() >= 0)
        {
            emit incIconSize();
        }
        else
        {
            emit decIconSize();
        }
        e->accept();
        return;
    }

    TDEIconView::wheelEvent(e);
}

void KonqIconViewWidget::leaveEvent( TQEvent *e )
{
    // when leaving the widget, stop possible pending filetip and icon effect
    slotOnViewport();

    TDEIconView::leaveEvent(e);
}

void KonqIconViewWidget::mousePressChangeValue()
{
  //kdDebug(1203) << "KonqIconViewWidget::contentsMousePressEvent" << endl;
  m_bMousePressed = true;
  if (d->pSoundPlayer)
    d->pSoundPlayer->stop();
  d->bSoundItemClicked = true;
  d->firstClick = false;

  // Once we click on the item, we don't want a tooltip
  // Fixes part of #86968
  d->pFileTip->setItem( 0 );
}

void KonqIconViewWidget::contentsMousePressEvent( TQMouseEvent *e )
{
    if(d->pActivateDoubleClick && d->pActivateDoubleClick->isActive ())
        d->pActivateDoubleClick->stop();
     TQIconViewItem* item = findItem( e->pos() );
     m_mousePos = e->pos();
     KURL url;
     if ( item )
     {
         url = ( static_cast<KFileIVI *>( item ) )->item()->url();
         bool brenameTrash =false;
         if ( url.isLocalFile() && (url.directory(false) == TDEGlobalSettings::trashPath() || url.path(1).startsWith(TDEGlobalSettings::trashPath())))
             brenameTrash = true;
         if ( !brenameTrash && !TDEGlobalSettings::singleClick() && m_pSettings->renameIconDirectly() && e->button() == TQt::LeftButton && item->textRect( false ).contains(e->pos())&& !d->firstClick &&  url.isLocalFile() && (!url.protocol().find("device", 0, false)==0))
         {
             d->firstClick = true;
             d->mousePos = e->pos();
             d->mouseState = e->state();
             if (!d->pActivateDoubleClick)
             {
                 d->pActivateDoubleClick = new TQTimer(this);
                 connect(d->pActivateDoubleClick, TQ_SIGNAL(timeout()), this, TQ_SLOT(doubleClickTimeout()));
             }
             if( d->pActivateDoubleClick->isActive () )
                 d->pActivateDoubleClick->stop();
             else
                 d->pActivateDoubleClick->start(TQApplication::doubleClickInterval());
             d->releaseMouseEvent = false;
             return;
         }
         else
             d->renameItem= false;
     }
     else
         d->renameItem= false;
    mousePressChangeValue();
    if(d->pActivateDoubleClick && d->pActivateDoubleClick->isActive())
        d->pActivateDoubleClick->stop();
    TDEIconView::contentsMousePressEvent( e );

}

void KonqIconViewWidget::contentsMouseReleaseEvent( TQMouseEvent *e )
{
    TDEIconView::contentsMouseReleaseEvent( e );
    if(d->releaseMouseEvent && d->pActivateDoubleClick && d->pActivateDoubleClick->isActive ())
        d->pActivateDoubleClick->stop();
    slotSelectionChanged();
    d->releaseMouseEvent = true;
    m_bMousePressed = false;
}

void KonqIconViewWidget::slotSaveIconPositions()
{
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  // This code is currently not used but left in for compatibility reasons.
  // It can be removed in KDE 4.0
  // Saving of desktop icon positions is now done in KDIconView::saveIconPositions()
  // in tdebase/kdesktop/kdiconview.cpp
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

  printf("WARNING: Strongly deprecated method KonqIconViewWidget::slotSaveIconPositions() called!\n"); fflush(stdout);

  if ( m_dotDirectoryPath.isEmpty() )
    return;
  if ( !m_bDesktop )
    return; // Currently not available in Konqueror
  kdDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions" << endl;
  TDESimpleConfig dotDirectory( m_dotDirectoryPath );
  TQIconViewItem *it = firstItem();
  if ( !it )
    return; // No more icons. Maybe we're closing and they've been removed already
  while ( it )
  {
    KFileIVI *ivi = static_cast<KFileIVI *>( it );
    KFileItem *item = ivi->item();

    dotDirectory.setGroup( TQString( m_iconPositionGroupPrefix ).append( item->url().fileName() ) );
    kdDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions " << item->url().fileName() << " " << it->x() << " " << it->y() << endl;
    dotDirectory.writeEntry( TQString( "X %1" ).arg( width() ), it->x() );
    dotDirectory.writeEntry( TQString( "Y %1" ).arg( height() ), it->y() );
    dotDirectory.writeEntry( "Exists", true );

    it = it->nextItem();
  }

  TQStringList groups = dotDirectory.groupList();
  TQStringList::ConstIterator gIt = groups.begin();
  TQStringList::ConstIterator gEnd = groups.end();
  for (; gIt != gEnd; ++gIt )
    if ( (*gIt).left( m_iconPositionGroupPrefix.length() ) == m_iconPositionGroupPrefix )
    {
      dotDirectory.setGroup( *gIt );
      if ( dotDirectory.hasKey( "Exists" ) )
        dotDirectory.deleteEntry( "Exists", false );
      else
      {
        kdDebug(1214) << "KonqIconViewWidget::slotSaveIconPositions deleting group " << *gIt << endl;
        dotDirectory.deleteGroup( *gIt );
      }
    }

  dotDirectory.sync();

  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  // This code is currently not used but left in for compatibility reasons.
  // It can be removed in KDE 4.0
  // Saving of desktop icon positions is now done in KDIconView::saveIconPositions()
  // in tdebase/kdesktop/kdiconview.cpp
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
}

// Adapted version of TQIconView::insertInGrid, that works relative to
// m_IconRect, instead of the entire viewport.

void KonqIconViewWidget::insertInGrid(TQIconViewItem *item)
{
    if (0L == item)
        return;

    if (!m_IconRect.isValid())
    {
        TDEIconView::insertInGrid(item);
        return;
    }

    TQRegion r(m_IconRect);
    TQIconViewItem *i = firstItem();
    int y = -1;
    for (; i; i = i->nextItem() )
    {
        r = r.subtract(i->rect());
        y = TQMAX(y, i->y() + i->height());
    }

    TQMemArray<TQRect> rects = r.rects();
    TQMemArray<TQRect>::Iterator it = rects.begin();
    bool foundPlace = false;
    for (; it != rects.end(); ++it)
    {
        TQRect rect = *it;
        if (rect.width() >= item->width() && rect.height() >= item->height())
        {
            int sx = 0, sy = 0;
            if (rect.width() >= item->width() + spacing())
                sx = spacing();
            if (rect.height() >= item->height() + spacing())
                sy = spacing();
            item->move(rect.x() + sx, rect.y() + sy);
            foundPlace = true;
            break;
        }
    }

    if (!foundPlace)
        item->move(m_IconRect.topLeft());

    //item->dirty = false;
    return;
}


/*
 * The algorithm used for lineing up the icons could be called
 * "beating flat the icon field". Imagine the icon field to be some height
 * field on a regular grid, with the height being the number of icons in
 * each grid element. Now imagine slamming on the field with a shovel or
 * some other flat surface. The high peaks will be flattened and spread out
 * over their adjacent areas. This is basically what the algorithm tries to
 * simulate.
 *
 * First, the icons are binned to a grid of the desired size. If all bins
 * are containing at most one icon, we're done, of course. We just have to
 * move all icons to the center of each grid element.
 * For each bin which has more than one icon in it, we calculate 4
 * "friction coefficients", one for each cardinal direction. The friction
 * coefficient of a direction is the number of icons adjacent in that
 * direction. The idea is that this number is somewhat a measure in which
 * direction the icons should flow: icons flow in the direction of lowest
 * friction coefficient. We move a maximum of one icon per bin and loop over
 * all bins. This procedure is repeated some maximum number of times or until
 * no icons are moved anymore.
 *
 * I don't know if this algorithm is good or bad, I don't even know if it will
 * work all the time. It seems a correct thing to do, however, and it seems to
 * work particularly well. In any case, the number of runs is limited so there
 * can be no races.
 */

void KonqIconViewWidget::lineupIcons()
{
    // even if there are no items yet, calculate the maxItemWidth to have the correct
    // item rect when we insert new items

    // Create a grid of (ny x nx) bins.
    int x0, y0, dx, dy, nx, ny;
    gridValues( &x0, &y0, &dx, &dy, &nx, &ny );

    int itemWidth = dx - spacing();
    bool newItemWidth = false;
    if ( maxItemWidth() != itemWidth ) {
        newItemWidth = true;
        setMaxItemWidth( itemWidth );
        setFont( font() );  // Force calcRect()
    }

    if ( !firstItem() ) {
        kdDebug(1203) << "No icons at all ?\n";
        return;
    }

    int iconSize = m_size ? m_size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );

    typedef TQValueList<TQIconViewItem*> Bin;
    Bin*** bins = new Bin**[nx];
    int i;
    int j;
    for ( i = 0; i < nx ; i++ ) {
        bins[i] = new Bin*[ny];
        for ( j = 0; j < ny; j++ )
            bins[i][j] = 0L;
    }

    // Insert items into grid
    int textHeight = iconTextHeight() * fontMetrics().height();

    for ( TQIconViewItem* item = firstItem(); item; item = item->nextItem() ) {
        int x = item->x() + item->width() / 2 - x0;
        int y = item->pixmapRect( false ).bottom() - iconSize / 2
                - ( dy - ( iconSize + textHeight ) ) / 2 - y0;
        int posX = TQMIN( nx-1, TQMAX( 0, x / dx ) );
        int posY = TQMIN( ny-1, TQMAX( 0, y / dy ) );

        if ( !bins[posX][posY] )
            bins[posX][posY] = new Bin;
        bins[posX][posY]->prepend( item );
    }

    // The shuffle code
    int n, k;
    const int infinity = 10000;
    int nmoves = 1;
    for ( n = 0; n < 30 && nmoves > 0; n++ ) {
        nmoves = 0;
        for ( i = 0; i < nx; i++ ) {
            for ( j = 0; j < ny; j++ ) {
                if ( !bins[i][j] || ( bins[i][j]->count() <= 1 ) )
                    continue;

                // Calculate the 4 "friction coefficients".
                int tf = 0, bf = 0, lf = 0, rf = 0;
                for ( k = j-1; k >= 0 && bins[i][k] && bins[i][k]->count(); k-- )
                    tf += bins[i][k]->count();
                if ( k == -1 )
                    tf += infinity;

                for ( k = j+1; k < ny && bins[i][k] && bins[i][k]->count(); k++ )
                    bf += bins[i][k]->count();
                if ( k == ny )
                    bf += infinity;

                for ( k = i-1; k >= 0 && bins[k][j] && bins[k][j]->count(); k-- )
                    lf += bins[k][j]->count();
                if ( k == -1 )
                    lf += infinity;

                for ( k = i+1; k < nx && bins[k][j] && bins[k][j]->count(); k++ )
                    rf += bins[k][j]->count();
                if ( k == nx )
                    rf += infinity;

                // If we are stuck between walls, continue
                if ( tf >= infinity && bf >= infinity &&
                     lf >= infinity && rf >= infinity )
                    continue;

                // Is there a preferred lineup direction?
                if ( m_LineupMode == LineupHorizontal ) {
                    tf += infinity;
                    bf += infinity;
                }
                else if ( m_LineupMode == LineupVertical ) {
                    lf += infinity;
                    rf += infinity;
                }

                // Move one item in the direction of the least friction
                TQIconViewItem* movedItem;
                Bin* items = bins[i][j];

                int mini = TQMIN( TQMIN( tf, bf ), TQMIN( lf, rf ) );
                if ( tf == mini ) {
                    // move top item in (i,j) to (i,j-1)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->y() < movedItem->y() )
                            movedItem = *it;
                    }
                    items->remove( movedItem );
                    if ( !bins[i][j-1] )
                        bins[i][j-1] = new Bin;
                    bins[i][j-1]->prepend( movedItem );
                }
                else if ( bf ==mini ) {
                    // move bottom item in (i,j) to (i,j+1)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->y() > movedItem->y() )
                            movedItem = *it;
                    }
                    items->remove( movedItem );
                    if ( !bins[i][j+1] )
                        bins[i][j+1] = new Bin;
                    bins[i][j+1]->prepend( movedItem );
                }
                else if ( lf == mini )
                {
                    // move left item in (i,j) to (i-1,j)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->x() < movedItem->x() )
                            movedItem = *it;
                    }
                    items->remove( movedItem );
                    if ( !bins[i-1][j] )
                        bins[i-1][j] = new Bin;
                    bins[i-1][j]->prepend( movedItem );
                }
                else {
                    // move right item in (i,j) to (i+1,j)
                    Bin::iterator it = items->begin();
                    movedItem = *it;
                    for ( ++it; it != items->end(); ++it ) {
                        if ( (*it)->x() > movedItem->x() )
                            movedItem = *it;
                    }
                    items->remove( movedItem );
                    if ( !bins[i+1][j] )
                        bins[i+1][j] = new Bin;
                    bins[i+1][j]->prepend( movedItem );
                }
                nmoves++;
            }
        }
    }

    // Perform the actual moving
    TQRegion repaintRegion;
    TQValueList<TQIconViewItem*> movedItems;

    for ( i = 0; i < nx; i++ ) {
        for ( j = 0; j < ny; j++ ) {
            Bin* bin = bins[i][j];
            if ( !bin )
                continue;
            if ( !bin->isEmpty() ) {
                TQIconViewItem* item = bin->first();
                int newX = x0 + i*dx + spacing() +
                           TQMAX(0, ( (dx-spacing()) - item->width() ) / 2);  // pixmap can be larger as iconsize
                // align all icons vertically to their text
                int newY = y0 + j*dy + dy - spacing() - ( item->pixmapRect().bottom() + 2 + textHeight );
                if ( item->x() != newX || item->y() != newY ) {
                    TQRect oldRect = item->rect();
                    movedItems.prepend( item );
                    item->move( newX, newY );
                    if ( item->rect() != oldRect )
                        repaintRegion = repaintRegion.unite( oldRect );
                }
            }
            delete bin;
            bins[i][j] = 0L;
        }
    }

    // repaint
    if ( newItemWidth )
        updateContents();
    else {
        // Repaint only repaintRegion...
        TQMemArray<TQRect> rects = repaintRegion.rects();
        for ( uint l = 0; l < rects.count(); l++ ) {
            kdDebug( 1203 ) << "Repainting (" << rects[l].x() << ","
                            << rects[l].y() << ")\n";
            repaintContents( rects[l], false );
        }
        // Repaint icons that were moved
        while ( !movedItems.isEmpty() ) {
            repaintItem( movedItems.first() );
            movedItems.remove( movedItems.first() );
        }
    }

    for ( i = 0; i < nx ; i++ ) {
            delete [] bins[i];
    }
    delete [] bins;
}

void KonqIconViewWidget::lineupIcons( TQIconView::Arrangement arrangement )
{
    int x0, y0, dx, dy, nxmax, nymax;
    gridValues( &x0, &y0, &dx, &dy, &nxmax, &nymax );
    int textHeight = iconTextHeight() * fontMetrics().height();

    TQRegion repaintRegion;
    TQValueList<TQIconViewItem*> movedItems;
    int nx = 0, ny = 0;

    TQIconViewItem* item;
    for ( item = firstItem(); item; item = item->nextItem() ) {
        int newX = x0 + nx*dx + spacing() +
                   TQMAX(0, ( (dx-spacing()) - item->width() ) / 2);  // icon can be larger as defined
        // align all icons vertically to their text
        int newY = y0 + ny*dy + dy - spacing() - ( item->pixmapRect().bottom() + 2 + textHeight );
        if ( item->x() != newX || item->y() != newY ) {
            TQRect oldRect = item->rect();
            movedItems.prepend( item );
            item->move( newX, newY );
            if ( item->rect() != oldRect )
                repaintRegion = repaintRegion.unite( oldRect );
        }
        if ( arrangement == TQIconView::LeftToRight ) {
            nx++;
            if ( nx >= nxmax ) {
                ny++;
                nx = 0;
            }
        }
        else {
            ny++;
            if ( ny >= nymax ) {
                nx++;
                ny = 0;
            }
        }
    }

    // Repaint only repaintRegion...
    TQMemArray<TQRect> rects = repaintRegion.rects();
    for ( uint l = 0; l < rects.count(); l++ ) {
        kdDebug( 1203 ) << "Repainting (" << rects[l].x() << ","
                        << rects[l].y() << ")\n";
        repaintContents( rects[l], false );
    }
    // Repaint icons that were moved
    while ( !movedItems.isEmpty() ) {
        repaintItem( movedItems.first() );
        movedItems.remove( movedItems.first() );
    }
}

int KonqIconViewWidget::largestPreviewIconSize( int size ) const
{
    int iconSize = size ? size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );

    if (iconSize < 28)
        return 48;
    if (iconSize < 40)
        return 64;
    if (iconSize < 60)
        return 96;
    if (iconSize < 120)
        return 128;

    return 192;
}

int KonqIconViewWidget::previewIconSize( int size ) const
{
    int iconSize = size ? size : TDEGlobal::iconLoader()->currentSize( TDEIcon::Desktop );

    if (!d->bBoostPreview)
        return iconSize;

    return largestPreviewIconSize( iconSize );
}

void KonqIconViewWidget::visualActivate(TQIconViewItem * item)
{
    // Rect of the TQIconViewItem.
    TQRect irect = item->rect();

    // Rect of the QIconViewItem's pixmap area.
    TQRect rect = item->pixmapRect();

    // Adjust to correct position. If this isn't done, the fact that the
    // text may be wider than the pixmap puts us off-centre.
    rect.moveBy(irect.x(), irect.y());

    // Adjust for scrolling (David)
    rect.moveBy( -contentsX(), -contentsY() );

    if (TDEGlobalSettings::showKonqIconActivationEffect() == true) {
        TDEIconEffect::visualActivate(viewport(), rect, item->pixmap());
    }
}

void KonqIconViewWidget::backgroundPixmapChange( const TQPixmap & )
{
    viewport()->update();
}

void KonqIconViewWidget::setPreviewSettings( const TQStringList& settings )
{
    d->previewSettings = settings;
    updatePreviewMimeTypes();

    int size = m_size;
    m_size = -1; // little trick to force grid change in setIcons
    setIcons( size ); // force re-determining all icons
}

const TQStringList& KonqIconViewWidget::previewSettings()
{
    return d->previewSettings;
}

void KonqIconViewWidget::setNewURL( const TQString& url )
{
    KURL u;
    if ( url.startsWith( "/" ) )
        u.setPath( url );
    else
        u = url;
    setURL( u );
}

void KonqIconViewWidget::setCaseInsensitiveSort( bool b )
{
    d->bCaseInsensitive = b;
}

bool KonqIconViewWidget::caseInsensitiveSort() const
{
    return d->bCaseInsensitive;
}

bool KonqIconViewWidget::canPreview( KFileItem* item )
{
    if ( !TDEGlobalSettings::showFilePreview( url() ) )
        return false;

    if ( d->pPreviewMimeTypes == 0L )
        updatePreviewMimeTypes();

    return mimeTypeMatch( item->mimetype(), *( d->pPreviewMimeTypes ) );
}

void KonqIconViewWidget::updatePreviewMimeTypes()
{
    if ( d->pPreviewMimeTypes == 0L )
        d->pPreviewMimeTypes = new TQStringList;
    else
        d->pPreviewMimeTypes->clear();

    // Load the list of plugins to determine which mimetypes are supported
    TDETrader::OfferList plugins = TDETrader::self()->query("ThumbCreator");
    TDETrader::OfferList::ConstIterator it;

    for ( it = plugins.begin(); it != plugins.end(); ++it ) {
        if ( d->previewSettings.contains((*it)->desktopEntryName()) ) {
            TQStringList mimeTypes = (*it)->property("MimeTypes").toStringList();
            for (TQStringList::ConstIterator mt = mimeTypes.begin(); mt != mimeTypes.end(); ++mt)
                d->pPreviewMimeTypes->append(*mt);
        }
    }
}

#include "konq_iconviewwidget.moc"
