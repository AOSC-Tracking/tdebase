/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2004 Michael Brade <brade@kde.org>

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
#ifndef __konq_listviewwidget_h__
#define __konq_listviewwidget_h__

#include <tqvaluelist.h>
#include <tqvaluevector.h>

#include <kurl.h>
#include <kfileitem.h>
#include <klistview.h>
#include <kparts/browserextension.h>
#include <konq_propsview.h>
#include "konq_listviewitems.h"

namespace KIO { class Job; }

class TQCursor;
class TQRect;
class KDirLister;
class KonqFMSettings;
class ListViewPropertiesExtension;
class KToggleAction;
class KonqListView;
class KonqFileTip;
class ListViewBrowserExtension;
class TQTimer;
class TQFocusEvent;
class TQDragMoveEvent;
class TQDragEnterEvent;
class TQDragLeaveEvent;
class TQDropEvent;
class TQPaintEvent;
class TQResizeEvent;
class TQMouseEvent;

class ColumnInfo
{
public:
   ColumnInfo();
   void setData( const TQString& n, const TQString& desktopName, int kioUds,
                 KToggleAction *someAction, int theWith = -1 );
   void setData( const TQString& n, const TQString& desktopName, int kioUds /* UDS_EXTRA */,
                 TQVariant::Type type, KToggleAction *someAction, int theWith = -1 );
   int displayInColumn;
   TQString name;
   TQString desktopFileName;
   int udsId;
   TQVariant::Type type; // only used if udsId == UDS_EXTRA
   bool displayThisOne;
   KToggleAction *toggleThisOne;
   int width;
};

/**
 * The tree view widget (based on KListView).
 * Most of the functionality is here.
 */
class KonqBaseListViewWidget : public KListView
{
   friend class KonqBaseListViewItem;
   friend class KonqListView;
   friend class ListViewBrowserExtension;

   Q_OBJECT
   TQ_OBJECT
public:
   KonqBaseListViewWidget( KonqListView *parent, TQWidget *parentWidget );
   virtual ~KonqBaseListViewWidget();
   unsigned int NumberOfAtoms;

   virtual void stop();
   const KURL& url();

   struct iterator
   {
      KonqBaseListViewItem *m_p;

      iterator() : m_p( 0L ) { }
      iterator( KonqBaseListViewItem *_b ) : m_p( _b ) { }
      iterator( const iterator& _it ) : m_p( _it.m_p ) { }

      KonqBaseListViewItem& operator*() { return *m_p; }
      KonqBaseListViewItem *operator->() { return m_p; }
      bool operator==( const iterator& _it ) { return ( m_p == _it.m_p ); }
      bool operator!=( const iterator& _it ) { return ( m_p != _it.m_p ); }
      iterator& operator++();
      iterator operator++(int);
   };
   iterator begin() { iterator it( (KonqBaseListViewItem *)firstChild() ); return it; }
   iterator end() { iterator it; return it; }

   virtual bool openURL( const KURL &url );

   void selectedItems( TQPtrList<KonqBaseListViewItem> *_list );
   KFileItemList visibleFileItems();
   KFileItemList selectedFileItems();
   KURL::List selectedUrls( bool mostLocal = false );

   /** @return the KonqListViewDir which handles the directory _url */
   //virtual KonqListViewDir *findDir ( const TQString & _url );

   /**
    * @return the Properties instance for this view. Used by the items.
    */
   KonqPropsView *props() const;

   //TQPtrList<ColumnInfo> *columnConfigInfo() { return &confColumns; };
   TQValueVector<ColumnInfo>& columnConfigInfo() { return confColumns; };
   TQString sortedByColumn;

   virtual void setShowIcons( bool enable ) { m_showIcons = enable; }
   virtual bool showIcons() { return m_showIcons; }

   void setItemFont( const TQFont &f ) { m_itemFont = f; }
   TQFont itemFont() const { return m_itemFont; }
   void setItemColor( const TQColor &c ) { m_itemColor = c; }
   TQColor itemColor() const { return m_itemColor; }
   int iconSize() const { return props()->iconSize(); }

   void setAscending( bool b ) { m_bAscending = b; }
   bool ascending() const { return m_bAscending; }
   bool caseInsensitiveSort() const;

   virtual void paintEmptyArea( TQPainter *p, const TQRect &r );

   virtual void saveState( TQDataStream & );
   virtual void restoreState( TQDataStream & );

   virtual void disableIcons( const KURL::List& lst );

   KonqListView *m_pBrowserView;
   KonqFMSettings *m_pSettings;

signals:
   void viewportAdjusted();

public slots:
   //virtual void slotOnItem( KonqBaseListViewItem* _item );
   // The '2' was added to differentiate it from KListView::slotMouseButtonClicked()
   void slotMouseButtonClicked2( int _button, TQListViewItem *_item, const TQPoint& pos, int );
   virtual void slotExecuted( TQListViewItem *_item );
   void slotItemRenamed( TQListViewItem *, const TQString &, int );

protected slots:
   void slotAutoScroll();

   // from TQListView
   virtual void slotReturnPressed( TQListViewItem *_item );
   virtual void slotCurrentChanged( TQListViewItem *_item ) { slotOnItem( _item ); }

   // slots connected to the directory lister
   virtual void slotStarted();
   virtual void slotCompleted();
   virtual void slotCanceled();
   virtual void slotClear();
   virtual void slotNewItems( const KFileItemList & );
   virtual void slotDeleteItem( KFileItem * );
   virtual void slotRefreshItems( const KFileItemList & );
   virtual void slotRedirection( const KURL & );
   void slotPopupMenu( TQListViewItem *, const TQPoint&, int );

   // forces a repaint on column size changes / branch expansion
   // when there is a background pixmap
   void slotUpdateBackground();

   //Notifies the browser view of the currently selected items
   void slotSelectionChanged();
   virtual void reportItemCounts();

protected:
   //creates the listview columns according to confColumns
   virtual void createColumns();
   //reads the configuration for the columns of the current
   //protocol, it is called when the protocol changes
   //it checks/unchecks the menu items and sets confColumns
   void readProtocolConfig( const KURL& url );
   //calls updateContents of every ListViewItem, called after
   //the columns changed
   void updateListContents();

   //this is called in the constructor, so virtual would be nonsense
   void initConfig();

   virtual void startDrag();
   virtual void viewportDragMoveEvent( TQDragMoveEvent *_ev );
   virtual void viewportDragEnterEvent( TQDragEnterEvent *_ev );
   virtual void viewportDragLeaveEvent( TQDragLeaveEvent *_ev );
   virtual void viewportDropEvent( TQDropEvent *_ev );
   virtual void viewportPaintEvent( TQPaintEvent *e );
   virtual void viewportResizeEvent( TQResizeEvent *e );

   virtual void drawRubber( TQPainter * );
   virtual void contentsMousePressEvent( TQMouseEvent *e );
   virtual void contentsMouseReleaseEvent( TQMouseEvent *e );
   virtual void contentsMouseMoveEvent( TQMouseEvent *e );
   virtual void contentsWheelEvent( TQWheelEvent * e );

   virtual void leaveEvent( TQEvent *e );

   /** Common method for slotCompleted and slotCanceled */
   virtual void setComplete();

   //the second parameter is set to true when the menu shortcut is pressed,
   //so the position of the mouse pointer doesn't matter when using keyboard, aleXXX
   virtual void popupMenu( const TQPoint& _global, bool alwaysForSelectedFiles = false );

   //this one is called only by KListView, and this is friend anyways (Alex)
   //KDirLister *dirLister() const { return m_dirLister; }

protected:
   int executeArea( TQListViewItem *_item );

   /** The directory lister for this URL */
   KDirLister *m_dirLister;

   //TQPtrList<ColumnInfo> confColumns;
   // IMO there is really no need for an advanced data structure
   //we have a fixed number of members,
   //it consumes less memory and access should be faster (Alex)
   // This might not be the case for ever... we should introduce custom fields in kio (David)
   TQValueVector<ColumnInfo> confColumns;

   KonqBaseListViewItem *m_dragOverItem;
   KonqBaseListViewItem *m_activeItem;
   TQPtrList<KonqBaseListViewItem> *m_selected;
   TQTimer *m_scrollTimer;

   TQFont m_itemFont;
   TQColor m_itemColor;

   TQRect *m_rubber;
   TQPixmap *m_backrubber;
   
   bool m_bTopLevelComplete:1;
   bool m_showIcons:1;
   bool m_bCaseInsensitive:1;
   bool m_bUpdateContentsPosAfterListing:1;
   bool m_bAscending:1;
   bool m_itemFound:1;
   bool m_restored:1;

   int m_filenameColumn;
   int m_filenameColumnWidth;

   KURL m_url;

   TQString m_itemToGoTo;
   TQStringList m_itemsToSelect;
   TQTimer *m_backgroundTimer;

   KonqFileTip *m_fileTip;
};

#endif
