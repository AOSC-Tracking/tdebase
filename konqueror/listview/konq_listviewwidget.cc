/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2001, 2002, 2004 Michael Brade <brade@kde.org>

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

#include "konq_listview.h"
#include "konq_listviewsettings.h"
#include "konq_listviewwidget.h"
#include <konq_filetip.h>
#include <konq_drag.h>
#include <konq_settings.h>

#include <kdebug.h>
#include <kdirlister.h>
#include <tdelocale.h>
#include <kprotocolinfo.h>
#include <tdeaction.h>
#include <kurldrag.h>
#include <tdemessagebox.h>
#include <kiconloader.h>
#include <kiconeffect.h>

#include <tqheader.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <tqtimer.h>
#include <tqevent.h>
#include <tqcursor.h>
#include <tqtooltip.h>

#include <stdlib.h>
#include <assert.h>

ColumnInfo::ColumnInfo()
   :displayInColumn(-1)
   ,name()
   ,desktopFileName()
   ,udsId(0)
   ,type(TQVariant::Invalid)
   ,displayThisOne(false)
   ,toggleThisOne(0)
{}


void ColumnInfo::setData(const TQString& n, const TQString& desktopName, int kioUds,
                         TDEToggleAction* someAction, int theWidth)
{
   displayInColumn=-1;
   name=n;
   desktopFileName=desktopName;
   udsId=kioUds;
   type=TQVariant::Invalid;
   displayThisOne=false;
   toggleThisOne=someAction;
   width=theWidth;
}

void ColumnInfo::setData(const TQString& n, const TQString& desktopName, int kioUds,
                         TQVariant::Type t, TDEToggleAction* someAction, int theWidth)
{
   displayInColumn=-1;
   name=n;
   desktopFileName=desktopName;
   udsId=kioUds;
   type=t;
   displayThisOne=false;
   toggleThisOne=someAction;
   width=theWidth;
}


KonqBaseListViewWidget::KonqBaseListViewWidget( KonqListView *parent, TQWidget *parentWidget)
   : TDEListView(parentWidget)
   ,sortedByColumn(0)
   ,m_pBrowserView(parent)
   ,m_dirLister(new KDirLister( true /*m_showIcons==false*/))
   ,m_dragOverItem(0)
   ,m_activeItem(0)
   ,m_selected(0)
   ,m_scrollTimer(0)
   ,m_rubber(0)
   ,m_showIcons(true)
   ,m_bCaseInsensitive(true)
   ,m_bUpdateContentsPosAfterListing(false)
   ,m_bAscending(true)
   ,m_itemFound(false)
   ,m_restored(false)
   ,m_filenameColumn(0)
   ,m_itemToGoTo("")
   ,m_backgroundTimer(0)
   ,m_fileTip(new KonqFileTip(this))
{
   kdDebug(1202) << "+KonqBaseListViewWidget" << endl;

   m_dirLister->setMainWindow(topLevelWidget());

   m_bTopLevelComplete  = true;

   //Adjust TDEListView behaviour
   setMultiSelection(true);
   setSelectionModeExt( FileManager );
   setDragEnabled(true);
   setItemsMovable(false);
   setUseSmallExecuteArea(true);

   initConfig();
#if 0
   connect( this, TQT_SIGNAL(rightButtonPressed(TQListViewItem*,const TQPoint&,int)),
            this, TQT_SLOT(slotRightButtonPressed(TQListViewItem*,const TQPoint&,int)));
#endif
   connect( this, TQT_SIGNAL(returnPressed( TQListViewItem * )),
            this, TQT_SLOT(slotReturnPressed( TQListViewItem * )) );
   connect( this, TQT_SIGNAL(mouseButtonClicked( int, TQListViewItem *, const TQPoint&, int )),
            this, TQT_SLOT(slotMouseButtonClicked2( int, TQListViewItem *, const TQPoint&, int )) );
   connect( this, TQT_SIGNAL(executed( TQListViewItem * )),
            this, TQT_SLOT(slotExecuted( TQListViewItem * )) );
   connect( this, TQT_SIGNAL(currentChanged( TQListViewItem * )),
            this, TQT_SLOT(slotCurrentChanged( TQListViewItem * )) );
   connect( this, TQT_SIGNAL(itemRenamed( TQListViewItem *, const TQString &, int )),
            this, TQT_SLOT(slotItemRenamed( TQListViewItem *, const TQString &, int )) );
   connect( this, TQT_SIGNAL(contextMenuRequested( TQListViewItem *, const TQPoint&, int )),
            this, TQT_SLOT(slotPopupMenu( TQListViewItem *, const TQPoint&, int )) );
   connect( this, TQT_SIGNAL(renameNext( TQListViewItem *, int )),
            this, TQT_SLOT(slotRenameNextItem( TQListViewItem*, int)) );
   connect( this, TQT_SIGNAL(renamePrev( TQListViewItem *, int )),
            this, TQT_SLOT(slotRenamePrevItem( TQListViewItem*, int)) );
   connect( this, TQT_SIGNAL(selectionChanged()), this, TQT_SLOT(slotSelectionChanged()) );

   connect( horizontalScrollBar(), TQT_SIGNAL(valueChanged( int )),
            this, TQT_SIGNAL(viewportAdjusted()) );
   connect( verticalScrollBar(), TQT_SIGNAL(valueChanged( int )),
            this, TQT_SIGNAL(viewportAdjusted()) );

   // Connect the directory lister
   connect( m_dirLister, TQT_SIGNAL(started( const KURL & )),
            this, TQT_SLOT(slotStarted()) );
   connect( m_dirLister, TQT_SIGNAL(completed()), this, TQT_SLOT(slotCompleted()) );
   connect( m_dirLister, TQT_SIGNAL(canceled()), this, TQT_SLOT(slotCanceled()) );
   connect( m_dirLister, TQT_SIGNAL(clear()), this, TQT_SLOT(slotClear()) );
   connect( m_dirLister, TQT_SIGNAL(newItems( const KFileItemList & ) ),
            this, TQT_SLOT(slotNewItems( const KFileItemList & )) );
   connect( m_dirLister, TQT_SIGNAL(deleteItem( KFileItem * )),
            this, TQT_SLOT(slotDeleteItem( KFileItem * )) );
   connect( m_dirLister, TQT_SIGNAL(refreshItems( const KFileItemList & )),
            this, TQT_SLOT( slotRefreshItems( const KFileItemList & )) );
   connect( m_dirLister, TQT_SIGNAL(redirection( const KURL & )),
            this, TQT_SLOT(slotRedirection( const KURL & )) );
   connect( m_dirLister, TQT_SIGNAL(itemsFilteredByMime( const KFileItemList & )),
            m_pBrowserView, TQT_SIGNAL(itemsFilteredByMime( const KFileItemList & )) );

   connect( m_dirLister, TQT_SIGNAL(infoMessage( const TQString& )),
            m_pBrowserView->extension(), TQT_SIGNAL(infoMessage( const TQString& )) );
   connect( m_dirLister, TQT_SIGNAL(percent( int )),
            m_pBrowserView->extension(), TQT_SIGNAL(loadingProgress( int )) );
   connect( m_dirLister, TQT_SIGNAL(speed( int )),
            m_pBrowserView->extension(), TQT_SIGNAL(speedProgress( int )) );

   connect( header(), TQT_SIGNAL(sizeChange( int, int, int )), TQT_SLOT(slotUpdateBackground()) );

   viewport()->setMouseTracking( true );
   viewport()->setFocusPolicy( TQ_WheelFocus );
   setFocusPolicy( TQ_WheelFocus );
   setAcceptDrops( true );

   //looks better with the statusbar
   setFrameStyle( TQFrame::StyledPanel | TQFrame::Sunken );
   setShowSortIndicator( true );
}

KonqBaseListViewWidget::~KonqBaseListViewWidget()
{
   kdDebug(1202) << "-KonqBaseListViewWidget" << endl;

   delete m_selected; m_selected = 0;

   // TODO: this is a hack, better fix the connections of m_dirLister if possible!
   m_dirLister->disconnect( this );
   delete m_dirLister;

   delete m_fileTip;
}

void KonqBaseListViewWidget::readProtocolConfig( const KURL & url )
{
   const TQString protocol = url.protocol();
   KonqListViewSettings config( protocol );
   config.readConfig();
   sortedByColumn = config.sortBy();
   m_bAscending = config.sortOrder();

   m_filenameColumnWidth = config.fileNameColumnWidth();

   TQStringList lstColumns = config.columns();
   TQValueList<int> lstColumnWidths = config.columnWidths();
   if (lstColumns.isEmpty())
   {
      // Default column selection
      lstColumns.append( "Size" );
      lstColumns.append( "File Type" );
      lstColumns.append( "Modified" );
      lstColumns.append( "Permissions" );
      lstColumns.append( "Owner" );
      lstColumns.append( "Group" );
      lstColumns.append( "Link" );
   }

   // Default number of columns
   NumberOfAtoms = 11;
   int extraIndex = NumberOfAtoms;

   // Check for any extra data
   KProtocolInfo::ExtraFieldList extraFields = KProtocolInfo::extraFields(url);
   NumberOfAtoms += extraFields.count();
   confColumns.resize( NumberOfAtoms );

   KProtocolInfo::ExtraFieldList::Iterator extraFieldsIt = extraFields.begin();
   for ( int num = 1; extraFieldsIt != extraFields.end(); ++extraFieldsIt, ++num )
   {
      const TQString column = (*extraFieldsIt).name;
      if ( lstColumns.find(column) == lstColumns.end() )
         lstColumns << column;
      const TQString type = (*extraFieldsIt).type; // ## TODO use when sorting
      TQVariant::Type t = TQVariant::Invalid;
      if ( type.lower() == TQString(TQSTRING_OBJECT_NAME_STRING).lower() )
          t = TQVariant::String;
      else if ( type.lower() == TQString(TQDATETIME_OBJECT_NAME_STRING).lower() )
          t = TQVariant::DateTime;
      else
          kdWarning() << "Unsupported ExtraType '" << type << "'" << endl;
      confColumns[extraIndex++].setData( column, TQString("Extra%1").arg(num), TDEIO::UDS_EXTRA, t, 0);
   }

   //disable everything
   for ( unsigned int i = 0; i < NumberOfAtoms; i++ )
   {
      confColumns[i].displayThisOne = false;
      confColumns[i].displayInColumn = -1;
      if ( confColumns[i].toggleThisOne )
      {
          confColumns[i].toggleThisOne->setChecked( false );
          confColumns[i].toggleThisOne->setEnabled( true );
      }
   }
   int currentColumn = m_filenameColumn + 1;
   //check all columns in lstColumns
   for ( unsigned int i = 0; i < lstColumns.count(); i++ )
   {
      //search the column in confColumns
      for ( unsigned int j = 0; j < NumberOfAtoms; j++ )
      {
         if ( confColumns[j].name == *lstColumns.at(i) )
         {
            confColumns[j].displayThisOne = true;
            confColumns[j].displayInColumn = currentColumn;
            if ( confColumns[j].toggleThisOne )
               confColumns[j].toggleThisOne->setChecked( true );
            currentColumn++;

            if ( i < lstColumnWidths.count() )
               confColumns[j].width = *lstColumnWidths.at(i);
            else
            {
               // Default Column widths
               ColumnInfo *tmpColumn = &confColumns[j];
               TQString str;

               if ( tmpColumn->udsId == TDEIO::UDS_SIZE )
                  str = TDEGlobal::locale()->formatNumber( 888888888, 0 ) + "  ";
               else if ( tmpColumn->udsId == TDEIO::UDS_ACCESS )
                  str = "--Permissions--";
               else if ( tmpColumn->udsId == TDEIO::UDS_USER )
                  str = "a_long_username";
               else if ( tmpColumn->udsId == TDEIO::UDS_GROUP )
                  str = "a_groupname";
               else if ( tmpColumn->udsId == TDEIO::UDS_LINK_DEST )
                  str = "a_quite_long_filename_for_link_dest";
               else if ( tmpColumn->udsId == TDEIO::UDS_FILE_TYPE )
                  str = "a_long_comment_for_mimetype";
               else if ( tmpColumn->udsId == TDEIO::UDS_MIME_TYPE )
                  str = "_a_long_/_mimetype_";
               else if ( tmpColumn->udsId == TDEIO::UDS_URL )
                  str = "a_long_lonq_long_very_long_url";
               else if ( (tmpColumn->udsId & TDEIO::UDS_TIME)
                         || (tmpColumn->udsId == TDEIO::UDS_EXTRA &&
                             (tmpColumn->type & TQVariant::DateTime)) )
               {
                  TQDateTime dt( TQDate( 2000, 10, 10 ), TQTime( 20, 20, 20 ) );
                  str = TDEGlobal::locale()->formatDateTime( dt ) + "--";
               }
               else
                  str = "it_is_the_default_width";

               confColumns[j].width = fontMetrics().width(str);
            }
            break;
         }
      }
   }
   //check what the protocol provides
   TQStringList listingList = KProtocolInfo::listing( url );
   kdDebug(1202) << k_funcinfo << "protocol: " << protocol << endl;

   // Even if this is not given by the protocol, we can determine it.
   // Please don't remove this ;-). It makes it possible to show the file type
   // using the mimetype comment, which for most users is a nicer alternative
   // than the raw mimetype name.
   listingList.append( "MimeType" );
   for ( unsigned int i = 0; i < NumberOfAtoms; i++ )
   {
      if ( confColumns[i].udsId == TDEIO::UDS_URL ||
           confColumns[i].udsId == TDEIO::UDS_MIME_TYPE ||
           !confColumns[i].displayThisOne )
      {
         continue;
      }

      TQStringList::Iterator listIt = listingList.find( confColumns[i].desktopFileName );
      if ( listIt == listingList.end() ) // not found -> hide
      {
         //move all columns behind one to the front
         for ( unsigned int l = 0; l < NumberOfAtoms; l++ )
            if ( confColumns[l].displayInColumn > confColumns[i].displayInColumn )
               confColumns[l].displayInColumn--;

         //disable this column
         confColumns[i].displayThisOne = false;
         if ( confColumns[i].toggleThisOne )
         {
           confColumns[i].toggleThisOne->setEnabled( false );
           confColumns[i].toggleThisOne->setChecked( false );
         }
      }
   }
}

void KonqBaseListViewWidget::createColumns()
{
   //this column is always required, so add it
   if ( columns() < 1 )
       addColumn( i18n("Name"), m_filenameColumnWidth );
   setSorting( 0, true );

   //remove all columns that will be re-added
   for ( int i=columns()-1; i>m_filenameColumn; i--)
        removeColumn(i);

   //now add the checked columns
   int currentColumn = m_filenameColumn + 1;
   for ( int i = 0; i < (int)NumberOfAtoms; i++ )
   {
      if ( confColumns[i].displayThisOne && (confColumns[i].displayInColumn == currentColumn) )
      {
         addColumn( i18n(confColumns[i].name.utf8()), confColumns[i].width );
         if ( sortedByColumn == confColumns[i].desktopFileName )
            setSorting( currentColumn, m_bAscending );
         if ( confColumns[i].udsId == TDEIO::UDS_SIZE )
             setColumnAlignment( currentColumn, AlignRight );
         i = -1;
         currentColumn++;
      }
   }
   if ( sortedByColumn == "FileName" )
      setSorting( 0, m_bAscending );
}

void KonqBaseListViewWidget::stop()
{
   m_dirLister->stop();
}

const KURL & KonqBaseListViewWidget::url()
{
   return m_url;
}

void KonqBaseListViewWidget::initConfig()
{
   m_pSettings = KonqFMSettings::settings();

   TQFont stdFont( m_pSettings->standardFont() );
   setFont( stdFont );
   //TODO: create config GUI
   TQFont itemFont( m_pSettings->standardFont() );
   itemFont.setUnderline( m_pSettings->underlineLink() );
   setItemFont( itemFont );
   setItemColor( m_pSettings->normalTextColor() );

   bool on = m_pSettings->showFileTips() && TQToolTip::isGloballyEnabled();
   m_fileTip->setOptions( on, m_pSettings->showPreviewsInFileTips(), m_pSettings->numFileTips() );

   updateListContents();
}

void KonqBaseListViewWidget::contentsMousePressEvent( TQMouseEvent *e ) {
	if ( m_rubber ) {
		TQRect r( m_rubber->normalize() );
		delete m_rubber;
		m_rubber = 0;
		repaintContents( r, FALSE );
	}

	delete m_selected;
	m_selected = new TQPtrList<KonqBaseListViewItem>;

	TQPoint vp = contentsToViewport( e->pos() );
	KonqBaseListViewItem* item = isExecuteArea( vp ) ?
		static_cast<KonqBaseListViewItem*>( itemAt( vp ) ) : 0L;

	if ( item ) {
		TDEListView::contentsMousePressEvent( e );
	}
	else {
		if ( e->button() == Qt::LeftButton ) {
			m_rubber = new TQRect( e->x(), e->y(), 0, 0 );
			clearSelection();
			emit selectionChanged();
			m_fileTip->setItem( 0 );
		}
		if ( e->button() != Qt::RightButton ) {
			TQListView::contentsMousePressEvent( e );
		}
	}

	// Store list of selected items at mouse-press time.
	// This is used when autoscrolling (why?)
	// and during dnd (the target item is temporarily selected)
	selectedItems( m_selected );
}

void KonqBaseListViewWidget::contentsMouseReleaseEvent( TQMouseEvent *e ) {
	if ( m_rubber ) {
		resetKeyboardSelectionOperation();
		TQRect r( m_rubber->normalize() );
		delete m_rubber;
		m_rubber = 0;
		repaintContents( r, FALSE );
	}

	if ( m_scrollTimer ) {
		disconnect( m_scrollTimer, TQT_SIGNAL( timeout() ), this, TQT_SLOT( slotAutoScroll() ) );
		m_scrollTimer->stop();
		delete m_scrollTimer;
		m_scrollTimer = 0;
	}

	delete m_selected; m_selected = 0;
	TDEListView::contentsMouseReleaseEvent( e );
}

void KonqBaseListViewWidget::contentsMouseMoveEvent( TQMouseEvent *e )
{
   if ( m_rubber )
   {
      slotAutoScroll();
      return;
   }

   TQPoint vp = contentsToViewport( e->pos() );
   KonqBaseListViewItem* item = isExecuteArea( vp ) ?
         static_cast<KonqBaseListViewItem *>( itemAt( vp ) ) : 0;

   if ( item != m_activeItem )
   {
      if ( m_activeItem != 0 )
         m_activeItem->setActive( false );

      m_activeItem = item;

      if ( item )
      {
         item->setActive( true );
         emit m_pBrowserView->setStatusBarText( item->item()->getStatusBarInfo() );
         m_pBrowserView->emitMouseOver( item->item() );

         vp.setY( itemRect( item ).y() );
         TQRect rect( viewportToContents( vp ), TQSize(20, item->height()) );
         m_fileTip->setItem( item->item(), rect, item->pixmap( 0 ) );
         m_fileTip->setPreview( TDEGlobalSettings::showFilePreview( item->item()->url() ) );
         setShowToolTips( !m_pSettings->showFileTips() );
      }
      else
      {
         reportItemCounts();
         m_pBrowserView->emitMouseOver( 0 );

         m_fileTip->setItem( 0 );
         setShowToolTips( true );
      }
   }

   TDEListView::contentsMouseMoveEvent( e );
}

void KonqBaseListViewWidget::contentsWheelEvent( TQWheelEvent *e )
{
   // when scrolling with mousewheel, stop possible pending filetip
   m_fileTip->setItem( 0 );

   if ( m_activeItem != 0 )
   {
      m_activeItem->setActive( false );
      m_activeItem = 0;
   }

   reportItemCounts();
   m_pBrowserView->emitMouseOver( 0 );
   TDEListView::contentsWheelEvent( e );
}

void KonqBaseListViewWidget::leaveEvent( TQEvent *e )
{
   if ( m_activeItem != 0 )
   {
      m_activeItem->setActive( false );
      m_activeItem = 0;
   }

   reportItemCounts();
   m_pBrowserView->emitMouseOver( 0 );

   m_fileTip->setItem( 0 );

   TDEListView::leaveEvent( e );
}

void KonqBaseListViewWidget::drawRubber( TQPainter *p )
{
   if ( !m_rubber )
      return;

   p->setRasterOp( NotROP );
   p->setPen( TQPen( color0, 1 ) );
   p->setBrush( NoBrush );

   TQPoint pt( m_rubber->x(), m_rubber->y() );
   pt = contentsToViewport( pt );
   style().tqdrawPrimitive( TQStyle::PE_RubberBand, p,
                          TQRect( pt.x(), pt.y(), m_rubber->width(), m_rubber->height() ),
                          colorGroup(), TQStyle::Style_Default, colorGroup().base() );
}

void KonqBaseListViewWidget::slotAutoScroll()
{
   if ( !m_rubber )
      return;

   // this code assumes that all items have the same height

   const TQPoint pos = viewport()->mapFromGlobal( TQCursor::pos() );
   const TQPoint vc = viewportToContents( pos );
   TQListViewItem *at = itemAt( vc );

   if ( vc == m_rubber->bottomRight() )
      return;

   TQRect oldRubber = *m_rubber;

   const int oldTop = m_rubber->normalize().top();
   const int oldBottom = m_rubber->normalize().bottom();


   m_rubber->setBottomRight( vc );

   TQListViewItem *cur = itemAt( TQPoint(0,0) );

   bool block = signalsBlocked();
   blockSignals( true );

   TQRect rr;
   TQRect nr = m_rubber->normalize();
   bool changed = FALSE;

   if ( cur )
   {
         TQRect rect;
      if ( allColumnsShowFocus() )
          rect = itemRect( cur );
      else {
          rect = itemRect( cur );
          rect.setWidth( executeArea( cur ) );
      }


      rect = TQRect( viewportToContents( rect.topLeft() ),
                    viewportToContents( rect.bottomRight() ) );

      if ( !allColumnsShowFocus() )
      {
         rect.setLeft( header()->sectionPos( 0 ) );
         rect.setWidth( rect.width() );
      }
      else
      {
         rect.setLeft( 0 );
         rect.setWidth( header()->headerWidth() );
      }

      TQRect r = rect;
      TQListViewItem *tmp = cur;

      while ( cur && rect.top() <= oldBottom )
      {
         if ( rect.intersects( nr ) )
         {
            if ( !cur->isSelected() && cur->isSelectable() )
	    {
               setSelected( cur, true );
               setActiveMultiSelectItem( at );
               changed = TRUE;
               rr = rr.unite( itemRect( cur ) );
	    }
         }
	 else
	 {
            if ( cur->isSelected() )
	    {
               changed = TRUE;
               rr = rr.unite( itemRect( cur ) );
	    }

	    if ( !m_selected || !m_selected->contains( (KonqBaseListViewItem*)cur ) )
	    {
               setSelected( cur, false );
               setActiveMultiSelectItem( at );
	    }
	 }


         cur = cur->itemBelow();
         if (cur && !allColumnsShowFocus())
            rect.setWidth( executeArea( cur ) );
         rect.moveBy( 0, rect.height() );
      }

      rect = r;
      rect.moveBy( 0, -rect.height() );
      cur = tmp->itemAbove();

      while ( cur && rect.bottom() >= oldTop )
      {
         if ( rect.intersects( nr ) )
         {
            if ( !cur->isSelected() && cur->isSelectable() )
	    {
               setSelected( cur, true );
               setActiveMultiSelectItem( at );
               changed = TRUE;
               rr = rr.unite( itemRect( cur ) );
	    }
	 }
	 else 
	 {
            if ( cur->isSelected() )
	    {
               changed = TRUE;
              rr = rr.unite( itemRect( cur ) );
	    }

            if ( !m_selected || !m_selected->contains( (KonqBaseListViewItem*)cur ) )
	    {
               setSelected( cur, false );
               setActiveMultiSelectItem( at );
	    }
	 }
	 

         cur = cur->itemAbove();
         if (cur && !allColumnsShowFocus())
            rect.setWidth( executeArea( cur ) );
         rect.moveBy( 0, -rect.height() );
      }
   }

   blockSignals( block );
   emit selectionChanged();

     TQRect allRect = oldRubber.normalize();
   if ( changed ) 
   {
       allRect |= rr.normalize();
   }
   allRect |= m_rubber->normalize();
   TQPoint point = contentsToViewport( allRect.topLeft() );
   allRect = TQRect( point.x(), point.y(), allRect.width(), allRect.height() );
   allRect &= viewport()->rect();
   allRect.addCoords( -2, -2, 2, 2 );

   TQPixmap backrubber( viewport()->rect().size() );
   backrubber.fill( viewport(), viewport()->rect().topLeft() );

   TQPainter p( &backrubber );
   p.save();
   drawContentsOffset( &p, 
      contentsX(), 
      contentsY(), 
      contentsX() + allRect.left(), contentsY() + allRect.top(), 
      allRect.width(), allRect.height() );
   p.restore();
   drawRubber( &p );
   p.end();
   bitBlt( viewport(), allRect.topLeft(), &backrubber, allRect );

   const int scroll_margin = 40;
   ensureVisible( vc.x(), vc.y(), scroll_margin, scroll_margin );

   if ( !TQRect( scroll_margin, scroll_margin,
                viewport()->width() - 2*scroll_margin,
                viewport()->height() - 2*scroll_margin ).contains( pos ) )
   {
      if ( !m_scrollTimer )
      {
         m_scrollTimer = new TQTimer( this );

         connect( m_scrollTimer, TQT_SIGNAL( timeout() ),
                  this, TQT_SLOT( slotAutoScroll() ) );
         m_scrollTimer->start( 100, false );
      }
   }
   else if ( m_scrollTimer )
   {
      disconnect( m_scrollTimer, TQT_SIGNAL( timeout() ),
                  this, TQT_SLOT( slotAutoScroll() ) );
      m_scrollTimer->stop();
      delete m_scrollTimer;
      m_scrollTimer = 0;
   }
}

void KonqBaseListViewWidget::viewportPaintEvent( TQPaintEvent *e )
{

   TDEListView::viewportPaintEvent( e );

   TQPainter p( viewport() );
   drawRubber( &p );
   p.end();
}

void KonqBaseListViewWidget::viewportResizeEvent(TQResizeEvent * e)
{
   TDEListView::viewportResizeEvent(e);
   emit viewportAdjusted();
}

void KonqBaseListViewWidget::viewportDragMoveEvent( TQDragMoveEvent *_ev )
{
   KonqBaseListViewItem *item =
       isExecuteArea( _ev->pos() ) ? (KonqBaseListViewItem*)itemAt( _ev->pos() ) : 0L;

   // Unselect previous drag-over-item
   if ( m_dragOverItem && m_dragOverItem != item )
       if ( !m_selected || !m_selected->contains( m_dragOverItem ) )
           setSelected( m_dragOverItem, false );

   if ( !item )
   {
      _ev->acceptAction();
      m_dragOverItem = 0L;
      return;
   }

   if ( item->item()->acceptsDrops() )
   {
      _ev->acceptAction();
      if ( m_dragOverItem != item )
      {
         setSelected( item, true );
         m_dragOverItem = item;
      }
   }
   else
   {
      _ev->ignore();
      m_dragOverItem = 0L;
   }
}

void KonqBaseListViewWidget::viewportDragEnterEvent( TQDragEnterEvent *_ev )
{
   m_dragOverItem = 0L;

   // By default we accept any format
   _ev->acceptAction();
}

void KonqBaseListViewWidget::viewportDragLeaveEvent( TQDragLeaveEvent * )
{
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;
}

void KonqBaseListViewWidget::viewportDropEvent( TQDropEvent *ev  )
{
   if ( m_dirLister->url().isEmpty() )
      return;
   kdDebug() << "KonqBaseListViewWidget::viewportDropEvent" << endl;
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;

   ev->accept();

   // We dropped on an item only if we dropped on the Name column.
   KonqBaseListViewItem *item =
       isExecuteArea( ev->pos() ) ? (KonqBaseListViewItem*)itemAt( ev->pos() ) : 0;

   KFileItem * destItem = (item) ? item->item() : m_dirLister->rootItem();
   KURL u = destItem ? destItem->url() : url();
   if ( u.isEmpty() )
      return;
   KonqOperations::doDrop( destItem /*may be 0L*/, u, ev, this );
}

void KonqBaseListViewWidget::startDrag()
{
   m_fileTip->setItem( 0 );
   KURL::List urls = selectedUrls( false );

   TQListViewItem * m_pressedItem = currentItem();

   TQPixmap pixmap2;
   bool pixmap0Invalid = !m_pressedItem->pixmap(0) || m_pressedItem->pixmap(0)->isNull();

   // Multiple URLs ?
   if (( urls.count() > 1 ) || (pixmap0Invalid))
   {
      int iconSize = m_pBrowserView->m_pProps->iconSize();
      iconSize = iconSize ? iconSize : TDEGlobal::iconLoader()->currentSize( TDEIcon::Small ); // Default = small
      pixmap2 = DesktopIcon( "application-vnd.tde.tdemultiple", iconSize );
      if ( pixmap2.isNull() )
          kdWarning(1202) << "Could not find multiple pixmap" << endl;
   }

   //KURLDrag *d = new KURLDrag( urls, viewport() );
   KonqDrag *drag= new KonqDrag( urls, selectedUrls(true), false, viewport() );
   if ( !pixmap2.isNull() )
      drag->setPixmap( pixmap2 );
   else if ( !pixmap0Invalid )
      drag->setPixmap( *m_pressedItem->pixmap( 0 ) );

   drag->drag();
}

void KonqBaseListViewWidget::slotItemRenamed( TQListViewItem *item, const TQString &name, int col )
{
   Q_ASSERT( col == 0 );
   Q_ASSERT( item != 0 );

   // The correct behavior is to show the old name until the rename has successfully
   // completed. Unfortunately, TDEListView forces us to allow the text to be changed
   // before we try the rename, so set it back to the pre-rename state.
   KonqBaseListViewItem *renamedItem = static_cast<KonqBaseListViewItem*>(item);
   renamedItem->updateContents();

   // Don't do anything if the user renamed to a blank name.
   if( !name.isEmpty() )
   {
      // Actually attempt the rename. If it succeeds, KDirLister will update the name.
      KonqOperations::rename( this, renamedItem->item()->url(), TDEIO::encodeFileName( name ) );
   }

   // When the TDEListViewLineEdit loses focus, focus tends to go to the location bar...
   setFocus();
}

void KonqBaseListViewWidget::slotRenameNextItem(TQListViewItem *item, int)
{
  TQListViewItem *nextItem = item->itemBelow();
  if (!nextItem)
  {
    nextItem=this->firstChild();
    if (!nextItem)
      return;   
  }

  deactivateAutomaticSelection();
  setCurrentItem(nextItem);
  ListViewBrowserExtension *lvbe = dynamic_cast<ListViewBrowserExtension*>(m_pBrowserView->m_extension);
  if (lvbe)
    lvbe->rename();
}

void KonqBaseListViewWidget::slotRenamePrevItem(TQListViewItem *item, int)
{
  TQListViewItem *prevItem = item->itemAbove();
  if (!prevItem)
  {
    prevItem=this->lastItem();
    if (!prevItem)
      return;
  }

  deactivateAutomaticSelection();
  setCurrentItem(prevItem);
  ListViewBrowserExtension *lvbe = dynamic_cast<ListViewBrowserExtension*>(m_pBrowserView->m_extension);
  if (lvbe)
    lvbe->rename();
}

void KonqBaseListViewWidget::reportItemCounts()
{
   KFileItemList lst = selectedFileItems();
   if ( !lst.isEmpty() )
      m_pBrowserView->emitCounts( lst );
   else
   {
      lst = visibleFileItems();
      m_pBrowserView->emitCounts( lst );
   }
}

void KonqBaseListViewWidget::slotSelectionChanged()
{
   reportItemCounts();

   KFileItemList lst = selectedFileItems();
   emit m_pBrowserView->m_extension->selectionInfo( lst );
}

void KonqBaseListViewWidget::slotMouseButtonClicked2( int _button,
      TQListViewItem *_item, const TQPoint& pos, int )
{
   if ( _button == Qt::MidButton )
   {
      if ( _item && isExecuteArea( viewport()->mapFromGlobal(pos) ) )
         m_pBrowserView->mmbClicked( static_cast<KonqBaseListViewItem *>(_item)->item() );
      else // MMB on background
         m_pBrowserView->mmbClicked( 0 );
   }
}

void KonqBaseListViewWidget::slotExecuted( TQListViewItem *item )
{
   if ( !item )
      return;
   m_fileTip->setItem( 0 );
   // isExecuteArea() checks whether the mouse pointer is
   // over an area where an action should be triggered
   // (i.e. the Name column, including pixmap and "+")
   if ( isExecuteArea( viewport()->mapFromGlobal( TQCursor::pos() ) ) )
      slotReturnPressed( item );
}

void KonqBaseListViewWidget::selectedItems( TQPtrList<KonqBaseListViewItem> *_list )
{
   iterator it = begin();
   for ( ; it != end(); it++ )
      if ( it->isSelected() )
         _list->append( &*it );
}

KFileItemList KonqBaseListViewWidget::visibleFileItems()
{
   KFileItemList list;
   KonqBaseListViewItem *item = static_cast<KonqBaseListViewItem *>(firstChild());
   while ( item )
   {
      list.append( item->item() );
      item = static_cast<KonqBaseListViewItem *>(item->itemBelow());
   }
   return list;
}

KFileItemList KonqBaseListViewWidget::selectedFileItems()
{
   KFileItemList list;
   iterator it = begin();
   for ( ; it != end(); it++ )
      if ( it->isSelected() )
         list.append( it->item() );
   return list;
}

KURL::List KonqBaseListViewWidget::selectedUrls( bool mostLocal )
{
   bool dummy;
   KURL::List list;
   iterator it = begin();
   for ( ; it != end(); it++ )
      if ( it->isSelected() )
         list.append( mostLocal ? it->item()->mostLocalURL( dummy ) : it->item()->url() );
   return list;
}

KonqPropsView * KonqBaseListViewWidget::props() const
{
   return m_pBrowserView->m_pProps;
}

void KonqBaseListViewWidget::slotReturnPressed( TQListViewItem *_item )
{
   if ( !_item )
      return;
   KFileItem *fileItem = static_cast<KonqBaseListViewItem *>(_item)->item();
   if ( !fileItem )
      return;

   KURL url = fileItem->url();
   url.cleanPath();
   bool isIntoTrash =  url.isLocalFile() && url.path(1).startsWith(TDEGlobalSettings::trashPath());
   if ( !isIntoTrash || (isIntoTrash && fileItem->isDir()) )
   {
         m_pBrowserView->lmbClicked( fileItem );

	 if (_item->pixmap(0) != 0)
	 {
	   // Rect of the TQListViewItem's pixmap area.
           TQRect rect = _item->listView()->itemRect(_item);

	   // calculate nesting depth
	   int nestingDepth = 0;
	   for (TQListViewItem *currentItem = _item->parent();
	        currentItem != 0;
	        currentItem = currentItem->parent())
	  	  nestingDepth++;
	
	   // no parent no indent
	   if (_item->parent() == 0)
		nestingDepth = 0;
	
	   // Root decoration means additional indent
	   if (_item->listView()->rootIsDecorated())
		nestingDepth++;
	
	   // set recalculated rect	
	   rect.setLeft(_item->listView()->itemMargin() + _item->listView()->treeStepSize() * nestingDepth);
	   rect.setWidth(_item->pixmap(0)->width());

	   // gather pixmap
	   TQPixmap *pix = new TQPixmap(*(_item->pixmap(0)));

	   // call the icon effect if enabled
	   if (TDEGlobalSettings::showKonqIconActivationEffect() == true) {
	       TDEIconEffect::visualActivate(viewport(), rect, pix);
	   }

	   // clean up
	   delete(pix);
	 }
   }
   else
      KMessageBox::information( 0, i18n("You must take the file out of the trash before being able to use it.") );
}

void KonqBaseListViewWidget::slotPopupMenu( TQListViewItem *i, const TQPoint &point, int c )
{
  kdDebug(1202) << "KonqBaseListViewWidget::slotPopupMenu" << endl;
  popupMenu( point, ( i != 0 && c == -1 ) ); // i != 0 && c == -1 when activated by keyboard
}

void KonqBaseListViewWidget::popupMenu( const TQPoint& _global, bool alwaysForSelectedFiles )
{
   m_fileTip->setItem( 0 );

   KFileItemList lstItems;
   KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems;

   // Only consider a right-click on the name column as something
   // related to the selection. On all the other columns, we want
   // a popup for the current dir instead.
   if ( alwaysForSelectedFiles || isExecuteArea( viewport()->mapFromGlobal( _global ) ) )
   {
       TQPtrList<KonqBaseListViewItem> items;
       selectedItems( &items );
       for ( KonqBaseListViewItem *item = items.first(); item; item = items.next() )
          lstItems.append( item->item() );
   }

   KFileItem *rootItem = 0L;
   bool deleteRootItem = false;
   if ( lstItems.count() == 0 ) // emit popup for background
   {
      clearSelection();

      if ( m_dirLister->url().isEmpty() )
         return;
      rootItem = m_dirLister->rootItem();
      if ( !rootItem )
      {
         if ( url().isEmpty() )
            return;
         // Maybe we want to do a stat to get full info about the root item
         // (when we use permissions). For now create a dummy one.
         rootItem = new KFileItem( S_IFDIR, (mode_t)-1, url() );
         deleteRootItem = true;
      }

      lstItems.append( rootItem );
      popupFlags = KParts::BrowserExtension::ShowNavigationItems | KParts::BrowserExtension::ShowUp;
   }
   emit m_pBrowserView->extension()->popupMenu( 0, _global, lstItems, KParts::URLArgs(), popupFlags );

   if ( deleteRootItem )
      delete rootItem; // we just created it
}

void KonqBaseListViewWidget::updateListContents()
{
   for ( KonqBaseListViewWidget::iterator it = begin(); it != end(); it++ )
      it->updateContents();
}

bool KonqBaseListViewWidget::openURL( const KURL &url )
{
   kdDebug(1202) << k_funcinfo << "protocol: " << url.protocol()
                               << " url: " << url.path() << endl;

   // The first time or new protocol? So create the columns first.
   if ( columns() < 1 || url.protocol() != m_url.protocol() )
   {
      readProtocolConfig( url );
      createColumns();
   }

   m_bTopLevelComplete = false;
   m_itemFound = false;

   if ( m_itemToGoTo.isEmpty() && url.equals( m_url.upURL(), true ) )
      m_itemToGoTo = m_url.fileName( true );

   // Check for new properties in the new dir
   // newProps returns true the first time, and any time something might
   // have changed.
   bool newProps = m_pBrowserView->m_pProps->enterDir( url );

   m_dirLister->setNameFilter( m_pBrowserView->nameFilter() );
   m_dirLister->setMimeFilter( m_pBrowserView->mimeFilter() );
   m_dirLister->setShowingDotFiles( m_pBrowserView->m_pProps->isShowingDotFiles() );

   KParts::URLArgs args = m_pBrowserView->extension()->urlArgs();
   if ( args.reload )
   {
      args.xOffset = contentsX();
      args.yOffset = contentsY();
      m_pBrowserView->extension()->setURLArgs( args );

      if ( currentItem() && itemRect( currentItem() ).isValid() )
         m_itemToGoTo = currentItem()->text(0);

      m_pBrowserView->m_filesToSelect.clear();
      iterator it = begin();
      for( ; it != end(); it++ )
         if ( it->isSelected() )
            m_pBrowserView->m_filesToSelect += it->text(0);
   }

   m_itemsToSelect = m_pBrowserView->m_filesToSelect;
   if ( !m_itemsToSelect.isEmpty() && m_itemToGoTo.isEmpty() )
      m_itemToGoTo = m_itemsToSelect[0];

   if ( columnWidthMode(0) == Maximum )
      setColumnWidth(0,50);

   m_url = url;
   m_bUpdateContentsPosAfterListing = true;

   // Start the directory lister !
   m_dirLister->openURL( url, false /* new url */, args.reload );

   // Apply properties and reflect them on the actions
   // do it after starting the dir lister to avoid changing the properties
   // of the old view
   if ( newProps )
   {
      m_pBrowserView->newIconSize( m_pBrowserView->m_pProps->iconSize() );
      m_pBrowserView->m_paShowDot->setChecked( m_pBrowserView->m_pProps->isShowingDotFiles() );
      if ( m_pBrowserView->m_paCaseInsensitive->isChecked() != m_pBrowserView->m_pProps->isCaseInsensitiveSort() ) {
          m_pBrowserView->m_paCaseInsensitive->setChecked( m_pBrowserView->m_pProps->isCaseInsensitiveSort() );
          // This is in case openURL returned all items synchronously.
          sort();
      }

      // It has to be "viewport()" - this is what KonqDirPart's slots act upon,
      // and otherwise we get a color/pixmap in the square between the scrollbars.
      m_pBrowserView->m_pProps->applyColors( viewport() );
   }

   return true;
}

void KonqBaseListViewWidget::setComplete()
{
   kdDebug(1202) << k_funcinfo << "Update Contents Pos: "
                 << m_bUpdateContentsPosAfterListing << endl;

   m_bTopLevelComplete = true;

   // Alex: this flag is set when we are just finishing a voluntary listing,
   // so do the go-to-item thing only under here. When we update the
   // current directory automatically (e.g. after a file has been deleted),
   // we don't want to go to the first item ! (David)
   if ( m_bUpdateContentsPosAfterListing )
   {
      m_bUpdateContentsPosAfterListing = false;

      if ( !m_itemFound )
         setCurrentItem( firstChild() );

      if ( !m_restored && !m_pBrowserView->extension()->urlArgs().reload )
         ensureItemVisible( currentItem() );
      else
         setContentsPos( m_pBrowserView->extension()->urlArgs().xOffset,
                         m_pBrowserView->extension()->urlArgs().yOffset );

      emit selectionChanged();
   }

   m_itemToGoTo = "";
   m_restored = false;

   // Show totals
   reportItemCounts();

   m_pBrowserView->emitMouseOver( 0 );

   if ( !isUpdatesEnabled() || !viewport()->isUpdatesEnabled() )
   {
      viewport()->setUpdatesEnabled( true );
      setUpdatesEnabled( true );
      triggerUpdate();
   }

   // Show "cut" icons as such
   m_pBrowserView->slotClipboardDataChanged();
}

void KonqBaseListViewWidget::slotStarted()
{
   //kdDebug(1202) << k_funcinfo << endl;

   if (!m_bTopLevelComplete)
      emit m_pBrowserView->started( 0 );
}

void KonqBaseListViewWidget::slotCompleted()
{
   //kdDebug(1202) << k_funcinfo << endl;

   setComplete();
   if ( m_bTopLevelComplete )
       emit m_pBrowserView->completed();
   m_pBrowserView->listingComplete();
}

void KonqBaseListViewWidget::slotCanceled()
{
   //kdDebug(1202) << k_funcinfo << endl;

   setComplete();
   emit m_pBrowserView->canceled( TQString::null );
}

void KonqBaseListViewWidget::slotClear()
{
   //kdDebug(1202) << k_funcinfo << endl;

   m_activeItem = 0;
   m_fileTip->setItem( 0 );
   delete m_selected; m_selected = 0;
   m_pBrowserView->resetCount();
   m_pBrowserView->lstPendingMimeIconItems().clear();

   viewport()->setUpdatesEnabled( false );
   setUpdatesEnabled( false );
   clear();
}

void KonqBaseListViewWidget::slotNewItems( const KFileItemList & entries )
{
   //kdDebug(1202) << k_funcinfo << entries.count() << endl;

   for ( TQPtrListIterator<KFileItem> kit ( entries ); kit.current(); ++kit )
   {
      KonqListViewItem * tmp = new KonqListViewItem( this, *kit );
      if ( !m_itemFound && tmp->text(0) == m_itemToGoTo )
      {
         setCurrentItem( tmp );
         m_itemFound = true;
      }
      if ( !m_itemsToSelect.isEmpty() ) {
         TQStringList::Iterator tsit = m_itemsToSelect.find( (*kit)->name() );
         if ( tsit != m_itemsToSelect.end() ) {
            m_itemsToSelect.remove( tsit );
            setSelected( tmp, true );
         }
      }
      if ( !(*kit)->isMimeTypeKnown() )
          m_pBrowserView->lstPendingMimeIconItems().append( tmp );
   }
   m_pBrowserView->newItems( entries );

   if ( !viewport()->isUpdatesEnabled() )
   {
      viewport()->setUpdatesEnabled( true );
      setUpdatesEnabled( true );
      triggerUpdate();
   }
   slotUpdateBackground();
}

void KonqBaseListViewWidget::slotDeleteItem( KFileItem * _fileitem )
{
 // new in 3.5.5
#ifdef TDEPARTS_BROWSEREXTENSION_HAS_ITEMS_REMOVED
  KFileItemList list;
  list.append( _fileitem );
  emit m_pBrowserView->extension()->itemsRemoved( list );
#else
#error "Your tdelibs doesn't have KParts::BrowserExtension::itemsRemoved, please update it to at least 3.5.5"
#endif

  iterator it = begin();
  for( ; it != end(); ++it )
    if ( (*it).item() == _fileitem )
    {
      kdDebug(1202) << k_funcinfo << "removing " << _fileitem->url().url() << " from tree!" << endl;

      m_pBrowserView->deleteItem( _fileitem );
      m_pBrowserView->lstPendingMimeIconItems().remove( &(*it) );

      if ( m_activeItem == &(*it) ) {
          m_fileTip->setItem( 0 );
          m_activeItem = 0;
      }

      delete &(*it);
      // HACK HACK HACK: TQListViewItem/KonqBaseListViewItem should
      // take care and the source looks like it does; till the
      // real bug is found, this fixes some crashes (malte)
      emit selectionChanged();
      return;
    }

  // This is needed for the case the root of the current view is deleted.
  // I supposed slotUpdateBackground has to be called as well after an item
  // was removed from a listview and was just forgotten previously (Brade).
  // OK, but this code also gets activated when deleting a hidden file... (dfaure)
  if ( !viewport()->isUpdatesEnabled() )
  {
    viewport()->setUpdatesEnabled( true );
    setUpdatesEnabled( true );
    triggerUpdate();
  }
  slotUpdateBackground();
}

void KonqBaseListViewWidget::slotRefreshItems( const KFileItemList & entries )
{
   //kdDebug(1202) << k_funcinfo << endl;

   TQPtrListIterator<KFileItem> kit ( entries );
   for ( ; kit.current(); ++kit )
   {
      iterator it = begin();
      for ( ; it != end(); ++it )
         if ( (*it).item() == kit.current() )
         {
            it->updateContents();
            break;
         }
   }

   m_pBrowserView->refreshItems(entries);  // Pass refresh command to KonqDirPart too
   reportItemCounts();
}

void KonqBaseListViewWidget::slotRedirection( const KURL & url )
{
   kdDebug(1202) << k_funcinfo << url << endl;

   if ( (columns() < 1) || (url.protocol() != m_url.protocol()) )
   {
      readProtocolConfig( url );
      createColumns();
   }
   const TQString prettyURL = url.pathOrURL();
   emit m_pBrowserView->extension()->setLocationBarURL( prettyURL );
   emit m_pBrowserView->setWindowCaption( prettyURL );
   m_pBrowserView->m_url = url;
   m_url = url;
}

KonqBaseListViewWidget::iterator& KonqBaseListViewWidget::iterator::operator++()
{
   if ( !m_p ) return *this;
   KonqBaseListViewItem *i = (KonqBaseListViewItem *)m_p->firstChild();
   if ( i )
   {
      m_p = i;
      return *this;
   }
   i = (KonqBaseListViewItem *)m_p->nextSibling();
   if ( i )
   {
      m_p = i;
      return *this;
   }
   m_p = (KonqBaseListViewItem *)m_p->parent();

   while ( m_p )
   {
      if ( m_p->nextSibling() )
         break;
      m_p = (KonqBaseListViewItem *)m_p->parent();
   }

   if ( m_p )
      m_p = (KonqBaseListViewItem *)m_p->nextSibling();

   return *this;
}

KonqBaseListViewWidget::iterator KonqBaseListViewWidget::iterator::operator++(int)
{
   KonqBaseListViewWidget::iterator it = *this;
   if ( !m_p ) return it;
   KonqBaseListViewItem *i = (KonqBaseListViewItem *)m_p->firstChild();
   if ( i )
   {
      m_p = i;
      return it;
   }
   i = (KonqBaseListViewItem *)m_p->nextSibling();
   if ( i )
   {
      m_p = i;
      return it;
   }
   m_p = (KonqBaseListViewItem *)m_p->parent();

   while ( m_p )
   {
      if ( m_p->nextSibling() )
         break;
      m_p = (KonqBaseListViewItem *)m_p->parent();
   }

   if ( m_p )
      m_p = (KonqBaseListViewItem *)m_p->nextSibling();
   return it;
}

void KonqBaseListViewWidget::paintEmptyArea( TQPainter *p, const TQRect &r )
{
   const TQPixmap *pm = TQT_TQPIXMAP_CONST(viewport()->paletteBackgroundPixmap());

   if (!pm || pm->isNull())
      p->fillRect(r, viewport()->backgroundColor());
   else
   {
       TQRect devRect = p->xForm( r );
       int ax = (devRect.x() + contentsX());
       int ay = (devRect.y() + contentsY());
       /* kdDebug() << "KonqBaseListViewWidget::paintEmptyArea "
                  << r.x() << "," << r.y() << " " << r.width() << "x" << r.height()
                  << " drawing pixmap with offset " << ax << "," << ay
                  << endl;*/
       p->drawTiledPixmap(r, *pm, TQPoint(ax, ay));
   }
}

void KonqBaseListViewWidget::disableIcons( const KURL::List & lst )
{
   iterator kit = begin();
   for( ; kit != end(); ++kit )
   {
      bool bFound = false;
      // Wow. This is ugly. Matching two lists together....
      // Some sorting to optimise this would be a good idea ?
      for (KURL::List::ConstIterator it = lst.begin(); !bFound && it != lst.end(); ++it)
      {
         if ( (*kit).item()->url() == *it ) // *it is encoded already
         {
            bFound = true;
            // maybe remove "it" from lst here ?
         }
      }
      (*kit).setDisabled( bFound );
   }
}

void KonqBaseListViewWidget::saveState( TQDataStream & ds )
{
   TQString str;
   if ( currentItem() )
      str = static_cast<KonqBaseListViewItem*>(currentItem())->item()->url().fileName(true);
   ds << str << m_url;
}

void KonqBaseListViewWidget::restoreState( TQDataStream & ds )
{
   m_restored = true;

   TQString str;
   KURL url;
   ds >> str >> url;
   if ( !str.isEmpty() )
      m_itemToGoTo = str;

   if ( columns() < 1 || url.protocol() != m_url.protocol() )
   {
      readProtocolConfig( url );
      createColumns();
   }
   m_url = url;

   m_bTopLevelComplete = false;
   m_itemFound = false;
}

void KonqBaseListViewWidget::slotUpdateBackground()
{
   if ( viewport()->paletteBackgroundPixmap() && !viewport()->paletteBackgroundPixmap()->isNull() )
   {
      if ( !m_backgroundTimer )
      {
         m_backgroundTimer = new TQTimer( this );
         connect( m_backgroundTimer, TQT_SIGNAL( timeout() ), viewport(), TQT_SLOT( update() ) );
      }
      else
         m_backgroundTimer->stop();

      m_backgroundTimer->start( 50, true );
   }
}

bool KonqBaseListViewWidget::caseInsensitiveSort() const
{
    return m_pBrowserView->m_pProps->isCaseInsensitiveSort();
}

// based on isExecuteArea from tdelistview.cpp
int KonqBaseListViewWidget::executeArea( TQListViewItem *_item )
{
   if ( !_item )
      return 0;

   int width = treeStepSize() * ( _item->depth() + ( rootIsDecorated() ? 1 : 0 ) );
   width += itemMargin();
   int ca = AlignHorizontal_Mask & columnAlignment( 0 );
   if ( ca == AlignLeft || ca == AlignAuto )
   {
      width += _item->width( fontMetrics(), this, 0 );
      if ( width > columnWidth( 0 ) )
         width = columnWidth( 0 );
   }
   return width;
}

#include "konq_listviewwidget.moc"
