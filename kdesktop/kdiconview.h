/* This file is part of the KDE project
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

#ifndef kdiconview_h
#define kdiconview_h

#include <tqtimer.h>
#include <tqlistview.h>

#include <konq_iconviewwidget.h>
#include <tdeaction.h>
#include <tdefileitem.h>
#include <kdirnotify.h>
#include <tdemessagebox.h>

#include <dcopclient.h>

class KDirLister;
class KonqSettings;
class TDESimpleConfig;
class TDEAccel;
class KShadowEngine;
class KDesktopShadowSettings;

/**
 * This class is KDesktop's icon view.
 * The icon view is a child widget of the KDesktop widget.
 *
 * Added shadow capability by Laur Ivan (C) 2003
 * The shadow is supported by the new KFileIVIDesktop objects
 * which replace KFileIVI objects.
 */
class KDIconView : public KonqIconViewWidget, public KDirNotify
{
    TQ_OBJECT

public:
    KDIconView( TQWidget *parent, const char* name = 0L );
    ~KDIconView();

    virtual void initConfig( bool init );
    void configureMedia();
    /**
     * Start listing
     */
    void start();

    TDEActionCollection *actionCollection() { return &m_actionCollection; }

    enum SortCriterion {
        NameCaseSensitive = 0, NameCaseInsensitive, Size, Type, Date };

    void rearrangeIcons( SortCriterion sc, bool bSortDirectoriesFirst);

    /**
     * Re-arrange the desktop icons without confirmation.
     */
    void rearrangeIcons();

    void lineupIcons(TQIconView::Arrangement);

    void setIconsLocked( bool b );
    void setAutoAlign( bool b );

    TQStringList selectedURLs();

    void update( const TQString &url );

    /**
     * Save the icon positions
     */
    void saveIconPositions();

    /**
     * Check if the URL to the desktop has changed
     */
    void recheckDesktopURL();

    /**
     * Called when the desktop icons area has changed
     */
    void updateWorkArea( const TQRect &wr );

    /**
     * Reimplemented from KonqIconViewWidget (for image drops)
     */
    virtual void setWallpaper(const KURL &url) { emit newWallpaper( url ); }
    void setLastIconPosition( const TQPoint & );

    /**
     * Reimplemented from TQIconView to cleanup internal data (namely m_paOutstandingFreeSpaceOverlays)
     */
    virtual void takeItem( TQIconViewItem *item );
    virtual void clear();

    static KURL desktopURL();

    /// KDirNotify interface, for trash:/
    virtual void FilesAdded( const KURL & directory );
    virtual void FilesRemoved( const KURL::List & fileList );
    virtual void FilesChanged( const KURL::List & ) {}

    void startDirLister();

    TQPoint findPlaceForIconCol( int column, int dx, int dy, const TQRect &currentIconArea );
    TQPoint findPlaceForIconRow( int row, int dx, int dy, const TQRect &currentIconArea );
    TQPoint findPlaceForIcon( int column, int row, const TQRect &currentIconArea = TQRect() );

protected slots:

    // slots connected to the icon view
    void slotReturnPressed( TQIconViewItem *item );
    void slotExecuted( TQIconViewItem *item );
    void slotMouseButtonPressed(int _button, TQIconViewItem* _item, const TQPoint& _global);
    void slotMouseButtonClickedKDesktop(int _button, TQIconViewItem* _item, const TQPoint& _global);
    void slotContextMenuRequested(TQIconViewItem* _item, const TQPoint& _global);
    void slotEnableAction( const char * name, bool enabled );
public slots:
    void slotAboutToCreate(const TQPoint &pos, const TQValueList<TDEIO::CopyInfo> &files);
protected slots:
    void slotItemRenamed(TQIconViewItem*, const TQString &name);

    // slots connected to the directory lister
    void slotStarted( const KURL& url );
    void slotCompleted();
    void slotNewItems( const KFileItemList& );
    void slotDeleteItem( KFileItem * );
    void slotRefreshItems( const KFileItemList& );

    // slots connected to the popupmenu (actions)
    void slotCut();
    void slotCopy();
    void slotTrashActivated( TDEAction::ActivationReason reason, TQt::ButtonState state );
    void slotDelete();
    void slotPopupPasteTo();
    void slotProperties();

    void slotClipboardDataChanged();

    void slotNewMenuActivated();

    // For communication with KDesktop
signals:
    void colorDropEvent( TQDropEvent *e );
    void imageDropEvent( TQDropEvent *e );
    void newWallpaper( const KURL & );
    void iconMoved();
    void wheelRolled( int delta );

public slots:
    /**
     * Lineup the desktop icons.
     */
    void lineupIcons();
    void slotPaste(); // for krootwm
    void slotClear();
    void refreshIcons();

    void incIconSpacing();
    void decIconSpacing();

protected slots:
    void slotFreeSpaceOverlayStart();
    void slotFreeSpaceOverlayFinished();
    void slotFreeSpaceOverlaySettingChanged();

protected:
    void createActions();
    void setupSortKeys();
    void initDotDirectories();

    bool makeFriendlyText( KFileIVI *fileIVI );
    static TQString stripDesktopExtension( const TQString & text );
    bool isDesktopFile( KFileItem * _item ) const;
    bool isFreePosition( const TQIconViewItem *item, const TQRect &currentIconArea ) const;
    bool isFreePosition( const TQIconViewItem *item, const TQRect& rect, const TQRect &currentIconArea ) const;
    void moveToFreePosition(TQIconViewItem *item, const TQRect &currentIconArea );

    bool deleteGlobalDesktopFiles();
    void removeBuiltinIcon(TQString iconName);
    void fillMediaListView();
    void saveMediaListView();

    static void renameDesktopFile(const TQString &path, const TQString &name);

    void popupMenu( const TQPoint &_global, const KFileItemList& _items );
    virtual void showEvent( TQShowEvent *e );
    virtual void contentsDropEvent( TQDropEvent *e );
    virtual void viewportWheelEvent( TQWheelEvent * );
    virtual void contentsMousePressEvent( TQMouseEvent *e );
    virtual void mousePressEvent( TQMouseEvent *e );
    virtual void wheelEvent( TQWheelEvent* e );

private:
    void refreshTrashIcon();

    static TQRect desktopRect();
    static void saveIconPosition(TDESimpleConfig *config, int x, int y);
    static void readIconPosition(TDESimpleConfig *config, int &x, int &y);

    void showFreeSpaceOverlay(KFileIVI*  item);

    /** Our action collection, parent of all our actions */
    TDEActionCollection m_actionCollection;

    /** TDEAccel object, to make the actions shortcuts work */
    TDEAccel *m_accel;

    bool m_bNeedRepaint;
    bool m_bNeedSave;
    bool m_iconsLocked;
    bool m_autoAlign;

    /** true if even one icon has an icon-position entry in the .directory */
    bool m_hasExistingPos;

    /** whether the user may move/edit/remove desktop icons */
    bool m_bEditableDesktopIcons;

    /** Show dot files ? */
    bool m_bShowDot;

    /** Vertical or Horizontal align of icons on desktop */
    bool m_bVertAlign;

    /** The directory lister - created only in start() */
    KDirLister* m_dirLister;

    /** The list of urls to be merged into the desktop, in addition to desktopURL */
    KURL::List m_mergeDirs;

    /** The list of dirs to be merged into the desktop, in addition to desktopURL **/
    TQStringList m_desktopDirs;

    /** The desktop's .directory, used for storing icon positions */
    TDESimpleConfig *m_dotDirectory;

    /** Position of last deleted icon - used when renaming a file */
    TQPoint m_lastDeletedIconPos;

    /** Sorting */
    SortCriterion m_eSortCriterion;
    bool m_bSortDirectoriesFirst;
    TQStringList m_itemsAlwaysFirst;

    /**
     * The shadow object
     */
    KShadowEngine *m_shadowEngine;

    /** Position where to move the next item.
     * It is set to the KRootWm position when "new file" is chosen. */
    TQPoint m_nextItemPos;

    /** Position where the last drop occurred */
    TQPoint m_dropPos;

    /** Position for the last dropped item */
    TQPoint m_lastDropPos;

    /** URL of the items which is being RMB'ed - when only one */
    KURL m_popupURL;

    /** media list management */
    bool m_enableMedia;
    TQStringList m_excludedMedia;

    // did we already get the correct desktopIconsArea (from kicker)
    // needed when we want to line up icons on a grid
    bool m_gotIconsArea;

    bool m_needDesktopAlign;

    TQListView *mMediaListView;

    TQPtrList<KFileIVI> m_paOutstandingFreeSpaceOverlays;
    TQTimer *m_paOutstandingFreeSpaceOverlaysTimer;

    TDEConfig *g_pConfig;
};

#endif
