/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#ifndef __konq_iconview_h__
#define __konq_iconview_h__

#include <tdeparts/browserextension.h>
#include <konq_iconviewwidget.h>
#include <konq_operations.h>
#include <konq_dirpart.h>
#include <kmimetyperesolver.h>
#include <tqptrdict.h>
#include <tqptrlist.h>
#include <tdefileivi.h>

class KonqPropsView;
class KFileItem;
class KDirLister;
class TDEAction;
class TDEToggleAction;
class TDEActionMenu;
class TQIconViewItem;
class IconViewBrowserExtension;

/**
 * The Icon View for konqueror.
 * The "Kfm" in the name stands for file management since it shows files :)
 */
class KonqKfmIconView : public KonqDirPart
{
  friend class IconViewBrowserExtension; // to access m_pProps
  Q_OBJECT
  TQ_PROPERTY( bool supportsUndo READ supportsUndo )
  TQ_PROPERTY( TQString viewMode READ viewMode WRITE setViewMode )
public:

  enum SortCriterion { NameCaseSensitive, NameCaseInsensitive, Size, Type, Date };

  KonqKfmIconView( TQWidget *parentWidget, TQObject *parent, const char *name, const TQString& mode );
  virtual ~KonqKfmIconView();

  virtual const KFileItem * currentItem();
  virtual KFileItemList selectedFileItems() {return m_pIconView->selectedFileItems();};


  KonqIconViewWidget *iconViewWidget() const { return m_pIconView; }

  bool supportsUndo() const { return true; }

  void setViewMode( const TQString &mode );
  TQString viewMode() const { return m_mode; }

  // "Cut" icons : disable those whose URL is in lst, enable the rest
  virtual void disableIcons( const KURL::List & lst );

  // See KMimeTypeResolver
  void mimeTypeDeterminationFinished();
  void determineIcon( KFileIVI * item );
  int iconSize() { return m_pIconView->iconSize(); }

public slots:
  void slotPreview( bool toggle );
  void slotShowDirectoryOverlays();
  void slotShowFreeSpaceOverlays();
  void slotShowDot();
  void slotSelect();
  void slotUnselect();
  void slotSelectAll();
  void slotUnselectAll();
  void slotInvertSelection();

  void slotSortByNameCaseSensitive( bool toggle );
  void slotSortByNameCaseInsensitive( bool toggle );
  void slotSortBySize( bool toggle );
  void slotSortByType( bool toggle );
  void slotSortByDate( bool toggle );
  void slotSortDescending();
  void slotSortDirsFirst();

protected slots:
  // slots connected to QIconView
  void slotReturnPressed( TQIconViewItem *item );
  void slotMouseButtonPressed(int, TQIconViewItem*, const TQPoint&);
  void slotMouseButtonClicked(int, TQIconViewItem*, const TQPoint&);
  void slotDoubleClicked(TQIconViewItem*);
  void slotContextMenuRequested(TQIconViewItem*, const TQPoint&);
  void slotOnItem( TQIconViewItem *item );
  void slotOnViewport();
  void slotSelectionChanged();

  // Slot used for spring loading folders
  void slotDragHeld( TQIconViewItem *item );
  void slotDragMove( bool accepted );
  void slotDragEntered( bool accepted );
  void slotDragLeft();
  void slotDragFinished();

  // slots connected to the directory lister - or to the kfind interface
  // They are reimplemented from KonqDirPart.
  virtual void slotStarted();
  virtual void slotCanceled();
  void slotCanceled( const KURL& url );
  virtual void slotCompleted();
  virtual void slotNewItems( const KFileItemList& );
  virtual void slotDeleteItem( KFileItem * );
  virtual void slotRefreshItems( const KFileItemList& );
  virtual void slotClear();
  virtual void slotRedirection( const KURL & );
  virtual void slotDirectoryOverlayStart();
  virtual void slotFreeSpaceOverlayStart();
  virtual void slotDirectoryOverlayFinished();
  virtual void slotFreeSpaceOverlayFinished();

  /**
   * This is the 'real' finished slot, where we emit the completed() signal
   * after everything was done.
   */
  void slotRenderingFinished();
  // (Re)Draws m_pIconView's contents. Connected to m_pTimeoutRefreshTimer.
  void slotRefreshViewport();

  // Connected to KonqDirPart
  void slotKFindOpened();
  void slotKFindClosed();

protected:
  virtual bool openFile() { return true; }
  virtual bool doOpenURL( const KURL& );
  virtual bool doCloseURL();

  virtual void newIconSize( int size );

  void setupSorting( SortCriterion criterion );

  /** */
  void setupSortKeys();

  TQString makeSizeKey( KFileIVI *item );

  /** The directory lister for this URL */
  KDirLister* m_dirLister;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit:1;

  /**
   * Set to true while the dirlister is running, _if_ we asked it
   * explicitly (openURL). If it is auto-updating, this is not set to true.
   */
  bool m_bLoading:1;

  /**
   * Set to true if we still need to emit completed() at some point
   * (after the loading is finished and after the visible icons have been
   * processed)
   */
  bool m_bNeedEmitCompleted:1;

  /**
   * Set to true if slotCompleted needs to realign the icons
   */
  bool m_bNeedAlign:1;

  bool m_bUpdateContentsPosAfterListing:1;

  bool m_bDirPropertiesChanged:1;
  
  /**
   * Set in doCloseURL and used in setViewMode to restart a preview
   * job interrupted when switching to IconView or MultiColumnView.
   * Hacks like this must be removed in KDE4!
   */
  bool m_bPreviewRunningBeforeCloseURL:1;

  bool m_bNeedSetCurrentItem:1;
  
  KFileIVI * m_pEnsureVisible;
  
  TQStringList m_itemsToSelect;
  
  SortCriterion m_eSortCriterion;

  TDEToggleAction *m_paDotFiles;
  TDEToggleAction *m_paDirectoryOverlays;
  TDEToggleAction *m_paFreeSpaceOverlays;
  TDEToggleAction *m_paEnablePreviews;
  TQPtrList<KFileIVI> m_paOutstandingOverlays;
  TQPtrList<KFileIVI> m_paOutstandingFreeSpaceOverlays;
  TQTimer *m_paOutstandingOverlaysTimer;
  TQTimer *m_paOutstandingFreeSpaceOverlaysTimer;
/*  TDEToggleAction *m_paImagePreview;
  TDEToggleAction *m_paTextPreview;
  TDEToggleAction *m_paHTMLPreview;*/
  TDEActionMenu *m_pamPreview;
  TQPtrList<TDEToggleAction> m_paPreviewPlugins;
  TDEActionMenu *m_pamSort;

  TDEAction *m_paSelect;
  TDEAction *m_paUnselect;
  TDEAction *m_paSelectAll;
  TDEAction *m_paUnselectAll;
  TDEAction *m_paInvertSelection;

  TDEToggleAction *m_paSortDirsFirst;

  KonqIconViewWidget *m_pIconView;

  TQTimer *m_pTimeoutRefreshTimer;

  TQPtrDict<KFileIVI> m_itemDict; // maps KFileItem * -> KFileIVI *

  KMimeTypeResolver<KFileIVI,KonqKfmIconView> * m_mimeTypeResolver;

  TQString m_mode;

  private:
  void showDirectoryOverlay(KFileIVI*  item);
  void showFreeSpaceOverlay(KFileIVI*  item);
};

class IconViewBrowserExtension : public KonqDirPartBrowserExtension
{
  Q_OBJECT
  friend class KonqKfmIconView; // so that it can emit our signals
public:
  IconViewBrowserExtension( KonqKfmIconView *iconView );

  virtual int xOffset();
  virtual int yOffset();

public slots:
  // Those slots are automatically connected by the shell
  void reparseConfiguration();
  void setSaveViewPropertiesLocally( bool value );
  void setNameFilter( const TQString &nameFilter );

  void refreshMimeTypes() { m_iconView->iconViewWidget()->refreshMimeTypes(); }

  void rename() { m_iconView->iconViewWidget()->renameSelectedItem(); }
  void cut() { m_iconView->iconViewWidget()->cutSelection(); }
  void copy() { m_iconView->iconViewWidget()->copySelection(); }
  void paste() { m_iconView->iconViewWidget()->pasteSelection(); }
  void pasteTo( const KURL &u ) { m_iconView->iconViewWidget()->paste( u ); }

  void trash();
  void del() { KonqOperations::del(m_iconView->iconViewWidget(),
                                   KonqOperations::DEL,
                                   m_iconView->iconViewWidget()->selectedUrls()); }
  void properties();
  void editMimeType();

  // void print();

private:
  KonqKfmIconView *m_iconView;
  bool m_bSaveViewPropertiesLocally;
};

class SpringLoadingManager : public QObject
{
    Q_OBJECT
private:
    SpringLoadingManager();
    static SpringLoadingManager *s_self;
public:
    static SpringLoadingManager &self();
    static bool exists();

    void springLoadTrigger(KonqKfmIconView *view, KFileItem *file,
                           TQIconViewItem *item);

    void dragLeft(KonqKfmIconView *view);
    void dragEntered(KonqKfmIconView *view);
    void dragFinished(KonqKfmIconView *view);

private slots:
    void finished();

private:
    KURL m_startURL;
    KParts::ReadOnlyPart *m_startPart;

    // Timer allowing to know the user wants to abort the spring loading
    // and go back to his start url (closing the opened window if needed)
    TQTimer m_endTimer;
};


#endif
