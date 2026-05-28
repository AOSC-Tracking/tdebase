/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
    Copyright (C) 2000, 2001, 2002 David Faure <david@mandrakesoft.com>

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

#ifndef __konq_iconviewwidget_h__
#define __konq_iconviewwidget_h__

#include <kiconloader.h>
#include <kiconview.h>
#include <kurl.h>
#include <tqguardedptr.h>
#include <tdefileitem.h>
#include <tdeio/jobclasses.h>
#include <libkonq_export.h>

class KonqFMSettings;
class KFileIVI;
class KonqIconDrag;
namespace TDEIO { class Job; }

/**
 * A file-aware icon view, implementing drag'n'drop, KDE icon sizes,
 * user settings, animated icons...
 * Used by kdesktop and konq_iconview.
 *
 */
class LIBKONQ_EXPORT KonqIconViewWidget : public TDEIconView
{
    TQ_OBJECT
    TQ_PROPERTY( bool sortDirectoriesFirst READ sortDirectoriesFirst WRITE setSortDirectoriesFirst )
    TQ_PROPERTY( TQRect iconArea READ iconArea WRITE setIconArea )
    TQ_PROPERTY( int lineupMode READ lineupMode WRITE setLineupMode )
    TQ_PROPERTY( TQString url READ urlString WRITE setNewURL )

    friend class KFileIVI;

public:

    enum LineupMode { LineupHorizontal=1, LineupVertical, LineupBoth };

    /**
     * Constructor
     */
    KonqIconViewWidget( TQWidget *parent = 0L, const char *name = 0L, WFlags f = 0, bool kdesktop = false );
    virtual ~KonqIconViewWidget();

    /**
     * Read the configuration and apply it.
     * Call this in the inherited constructor with bInit=true,
     * and in some reparseConfiguration() slot with bInit=false.
     * Returns true if the font was changed (which means something has to
     * be done so that the icon's texts don't run into each other).
     * However Konq and KDesktop handle this differently.
     */
    bool initConfig( bool bInit );

    /**
     * Set the area that will be occupied by icons. It is still possible to
     * drag icons outside this area; this only applies to automatically placed
     * icons.
     */
    void setIconArea( const TQRect &rect );

    /**
     * Reimplemented to make the slotOnItem highlighting work.
     */
    virtual void focusOutEvent( TQFocusEvent * /* ev */ );

    /**
     * Returns the icon area.
     */
    TQRect iconArea() const;

    /**
     * Set the lineup mode. This determines in which direction(s) icons are
     * moved when lineing them up.
     */
    void setLineupMode(int mode);

    /**
     * Returns the lineup mode.
     */
    int lineupMode() const;

    /**
     * Line up the icons to a regular grid. The outline of the grid is
     * specified by iconArea. The two length parameters are
     * gridX and gridY.
     */
    void lineupIcons();

    /**
     * Line up the icons to a regular grid horizontally or vertically.
     *
     * @param arrangement the arrangement to use (TQIconView::LeftToRight
     *        for a horizontal arrangement and TQIconView::TopToBottom
     *        for vertical)
     */
    void lineupIcons( TQIconView::Arrangement arrangement );

    /**
     * Sets the icons of all items, and stores the @p size
     * This doesn't touch thumbnails, except if @p stopImagePreviewFor is set.
     * Takes care of the grid, when changing the size.
     *
     * @param size size to use for the icons
     * @param stopImagePreviewFor set to a list of mimetypes which should be made normal again.
     * For instance "text/plain,image/wmf".
     * Can be set to "*" for "all mimetypes" and to "image/"+"*" for "all images".
     */
    void setIcons( int size, const TQStringList& stopImagePreviewFor = TQStringList() );

    /**
     * Called on databaseChanged
     */
    void refreshMimeTypes();

    int iconSize() { return m_size; }

    void calculateGridX();
    /**
     * The horizontal distance between two icons
     * (whether or not a grid has been given to TQIconView)
     */
    int gridXValue() const;

    /**
     * Calculate the geometry of the fixed grid that is used to line up the
     * icons, for example when using the lineupIcons() method.
     *
     * @param x
     * @param y
     * @param dx Cell width
     * @param dy Cell height
     * @param nx Number of columns
     * @param ny Number of rows
     */
    void gridValues( int* x, int* y, int* dx, int* dy, int* nx, int* ny );

    /**
     * Start generating the previews.
     * @param ignored this parameter is probably ignored
     * @param force if true, all files are looked at.
     *    Otherwise, only those which are not a thumbnail already.
     *
     * @todo figure out the parameter meanings again
     */
    void startImagePreview( const TQStringList &ignored, bool force );
    void stopImagePreview();
    bool isPreviewRunning() const;
    // unused
    void setThumbnailPixmap( KFileIVI * item, const TQPixmap & pixmap );
    void disableSoundPreviews();

    void setURL ( const KURL & kurl );
    // ### KDE4: make const
    const KURL & url() { return m_url; }
    TQString urlString() const { return m_url.url(); }
    void setRootItem ( const KFileItem * item ) { m_rootItem = item; }

    /**
     * Get list of selected KFileItems
     */
    KFileItemList selectedFileItems();

    void setItemColor( const TQColor &c );
    TQColor itemColor() const;

    virtual void cutSelection();
    virtual void copySelection();
    virtual void pasteSelection();
    virtual KURL::List selectedUrls(); // KDE4: remove virtual + add const
    enum UrlFlags { UserVisibleUrls = 0, MostLocalUrls = 1 };
    KURL::List selectedUrls( UrlFlags flags ) const; // KDE4: merge with above, default is == UserVisibleUrls
    void paste( const KURL &url );

    bool sortDirectoriesFirst() const;
    void setSortDirectoriesFirst( bool b );

    void setCaseInsensitiveSort( bool b );
    bool caseInsensitiveSort() const;

    /**
     * Cache of the dragged URLs over the icon view, used by KFileIVI
     */
    const KURL::List & dragURLs() { return m_lstDragURLs; }

    /**
     * Reimplemented from QIconView
     */
    virtual void clear();

    /**
     * Reimplemented from QIconView
     */
    virtual void takeItem( TQIconViewItem *item );

    /**
     * Reimplemented from TQIconView to take into account iconArea.
     */
    virtual void insertInGrid( TQIconViewItem *item );

    /**
     * Reimplemented from TQIconView to update the gridX
     */
    virtual void setItemTextPos( ItemTextPos pos );

    /**
     * Give feedback when item is activated.
     */
    virtual void visualActivate(TQIconViewItem *);

    bool isDesktop() const { return m_bDesktop; }

    /**
     * Provided for KDesktop.
     */
    virtual void setWallpaper(const KURL&) { }

    bool maySetWallpaper();
    void setMaySetWallpaper(bool b);

    void disableIcons( const KURL::List & lst );

    TQString iconPositionGroupPrefix() const { return m_iconPositionGroupPrefix; }
    TQString dotDirectoryPath() const { return m_dotDirectoryPath; }

    void setPreviewSettings(const TQStringList& mimeTypes);
    const TQStringList& previewSettings();
    void setNewURL( const TQString& url );

    bool isRenaming() const; // reimplemented from TQIconView

public slots:
    /**
     * Checks the new selection and emits enableAction() signals
     */
    virtual void slotSelectionChanged();

    void slotSaveIconPositions();

    void renameSelectedItem();
    void renameCurrentItem();

    void slotToolTipPreview( const KFileItem *, const TQPixmap & );  // ### unused - remove for KDE4
    void slotToolTipPreviewResult() ;  // ### unused - remove for KDE4

signals:
    /**
     * For cut/copy/paste/move/delete (see tdeparts/browserextension.h)
     */
    void enableAction( const char * name, bool enabled );

    void dropped();
    void imagePreviewFinished();

    void incIconSize();
    void decIconSize();

    /**
     * We need to track drag in icon views for the spring loading folders
     */
    void dragEntered( bool accepted );
    void dragLeft();

    void dragMove( bool accepted );
    /**
     * Emited after the dropped() event. This way we know when the
     * drag'n'drop is really finished.
     */
    void dragFinished();

protected slots:
    virtual void slotDropped( TQDropEvent *e, const TQValueList<TQIconDragItem> & );

    void slotItemRenamed(TQIconViewItem *item, const TQString &name);

    void slotIconChanged(int);
    void slotOnItem(TQIconViewItem *);
    void slotOnViewport();
    void slotStartSoundPreview();
    void slotPreview(const KFileItem *, const TQPixmap &);
    void slotPreviewResult();

    void slotMovieUpdate( const TQRect& rect );
    void slotMovieStatus( int status );
    void slotReenableAnimation();

    void slotAboutToCreate(const TQPoint &pos, const TQValueList<TDEIO::CopyInfo> &files);
    void doubleClickTimeout();

protected:
    virtual TQDragObject *dragObject();
    KonqIconDrag *konqDragObject( TQWidget * dragSource = 0L );
    bool mimeTypeMatch( const TQString& mimeType, const TQStringList& mimeList ) const;

    virtual void drawBackground( TQPainter *p, const TQRect &r );
    /**
     * r is the rectangle which you want to paint from the background.
     * pt is the upper left point in the painter device where you want to paint
     * the rectangle r.
     */
    virtual void drawBackground( TQPainter *p, const TQRect &r,
		 			const TQPoint &pt );
    virtual void contentsDragEnterEvent( TQDragEnterEvent *e );
    virtual void contentsDragLeaveEvent( TQDragLeaveEvent *e );
    virtual void contentsDragMoveEvent( TQDragMoveEvent *e );
    virtual void contentsDropEvent( TQDropEvent *e );
    virtual void contentsMousePressEvent( TQMouseEvent *e );
    virtual void contentsMouseReleaseEvent ( TQMouseEvent * e );
    virtual void contentsMouseMoveEvent( TQMouseEvent *e );
    virtual void backgroundPixmapChange( const TQPixmap & );
    virtual void wheelEvent( TQWheelEvent* );
    virtual void leaveEvent( TQEvent *e );

    void readAnimatedIconsConfig();
    void mousePressChangeValue();

    bool boostPreview() const;
    int previewIconSize( int size ) const;
    int largestPreviewIconSize( int size ) const;
    bool canPreview( KFileItem* item );
    void updatePreviewMimeTypes();

    const KFileIVI* renamingItem();
    void setRenamingItem(KFileIVI *theItem);

private:
    KURL m_url;
    const KFileItem * m_rootItem;

    KURL::List m_lstDragURLs;

    int m_size;

    /** Konqueror settings */
    KonqFMSettings * m_pSettings;

    bool m_bMousePressed;
    TQPoint m_mousePos;

    TQColor iColor;

    bool m_bSortDirsFirst;

    TQString m_iconPositionGroupPrefix;
    TQString m_dotDirectoryPath;

    int m_LineupMode;
    TQRect m_IconRect;

    bool m_bDesktop;
    bool m_bSetGridX;

private:
    struct KonqIconViewWidgetPrivate *d;

};

#endif
