/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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
#include <konq_settings.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <assert.h>
#include <stdio.h>
#include <tqpainter.h>
#include <tqheader.h>
#include <kiconloader.h>

static TQString retrieveExtraEntry( KFileItem* fileitem, int numExtra )
{
    /// ######## SLOOOOW
    TDEIO::UDSEntry::ConstIterator it = fileitem->entry().begin();
    const TDEIO::UDSEntry::ConstIterator end = fileitem->entry().end();
    int n = 0;
    for( ; it != end; ++it )
    {
        if ((*it).m_uds == TDEIO::UDS_EXTRA)
        {
            ++n;
            if ( n == numExtra )
            {
                return (*it).m_str;
            }
        }
    }
    return TQString::null;
}


/**************************************************************
 *
 * KonqListViewItem
 *
 **************************************************************/
KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget,
                                    KonqListViewItem * _parent, KFileItem* _fileitem )
   : KonqBaseListViewItem( _listViewWidget, _parent, _fileitem ),
     m_pixmaps( listView()->columns() )
{
   updateContents();
}

KonqListViewItem::KonqListViewItem( KonqBaseListViewWidget *_listViewWidget, KFileItem* _fileitem )
   : KonqBaseListViewItem( _listViewWidget, _fileitem ),
     m_pixmaps( listView()->columns() )
{
   updateContents();
}

KonqListViewItem::~KonqListViewItem()
{
   for ( TQValueVector<TQPixmap*>::iterator
            it = m_pixmaps.begin(), itEnd = m_pixmaps.end();
         it != itEnd; ++it )
      delete *it;
}

void KonqListViewItem::updateContents()
{
   // Set the pixmap
   setDisabled( m_bDisabled );

   // Set the text of each column
   setText( 0, m_fileitem->text() );

   // The order is: .dir (0), dir (1), .file (2), file (3)
   sortChar = S_ISDIR( m_fileitem->mode() ) ? 1 : 3;
   if ( m_fileitem->text()[0] == '.' )
       --sortChar;

   //now we have the first column, so let's do the rest

   int numExtra = 1;
   for ( unsigned int i=0; i<m_pListViewWidget->NumberOfAtoms; i++ )
   {
      ColumnInfo *tmpColumn=&m_pListViewWidget->columnConfigInfo()[i];
      if (tmpColumn->displayThisOne)
      {
         switch (tmpColumn->udsId)
         {
         case TDEIO::UDS_USER:
            setText(tmpColumn->displayInColumn,m_fileitem->user());
            break;
         case TDEIO::UDS_GROUP:
            setText(tmpColumn->displayInColumn,m_fileitem->group());
            break;
         case TDEIO::UDS_FILE_TYPE:
            if (m_fileitem->isMimeTypeKnown()) {
               setText(tmpColumn->displayInColumn,m_fileitem->mimeComment());
            }
            break;
         case TDEIO::UDS_MIME_TYPE:
            if (m_fileitem->isMimeTypeKnown()) {
               setText(tmpColumn->displayInColumn,m_fileitem->mimetype());
            }
            break;
         case TDEIO::UDS_URL:
            setText(tmpColumn->displayInColumn,m_fileitem->url().prettyURL());
            break;
         case TDEIO::UDS_LINK_DEST:
            setText(tmpColumn->displayInColumn,m_fileitem->linkDest());
            break;
         case TDEIO::UDS_SIZE:
            if ( m_pListViewWidget->m_pSettings->fileSizeInBytes() )
                setText(tmpColumn->displayInColumn,TDEGlobal::locale()->formatNumber( m_fileitem->size(),0)+" ");
            else
                setText(tmpColumn->displayInColumn,TDEIO::convertSize(m_fileitem->size())+" ");
            break;
         case TDEIO::UDS_ACCESS:
            setText(tmpColumn->displayInColumn,m_fileitem->permissionsString());
            break;
         case TDEIO::UDS_MODIFICATION_TIME:
         case TDEIO::UDS_ACCESS_TIME:
         case TDEIO::UDS_CREATION_TIME:
            {
               TQDateTime dt;
               time_t _time = m_fileitem->time( tmpColumn->udsId );
               if ( _time != 0 )
               {
                   dt.setTime_t( _time );
                   setText(tmpColumn->displayInColumn, TDEGlobal::locale()->formatDateTime(dt));
               }
               else
               {
                   setText(tmpColumn->displayInColumn, TQString::null);
               }
            }
            break;
         case TDEIO::UDS_EXTRA:
         {
             const TQString entryStr = retrieveExtraEntry( m_fileitem, numExtra );
             if ( tmpColumn->type == TQVariant::DateTime )
             {
                 TQDateTime dt = TQT_TQDATETIME_OBJECT(TQDateTime::fromString( entryStr, Qt::ISODate ));
                 setText(tmpColumn->displayInColumn,
                         TDEGlobal::locale()->formatDateTime(dt));
             }
             else // if ( tmpColumn->type == TQVariant::String )
                 setText(tmpColumn->displayInColumn, entryStr);
             ++numExtra;
             break;
         }
         default:
            break;
         };
      };
   };
}

void KonqListViewItem::setDisabled( bool disabled )
{
    KonqBaseListViewItem::setDisabled( disabled );
    int iconSize = m_pListViewWidget->iconSize();
    iconSize = iconSize ? iconSize : TDEGlobal::iconLoader()->currentSize( TDEIcon::Small ); // Default = small
    setPixmap( 0, m_fileitem->pixmap( iconSize, state() ) );
}

void KonqListViewItem::setActive( bool active )
{
    if ( m_bActive == active )
        return;

    //#### Optimize away repaint if possible, like the iconview does?
    KonqBaseListViewItem::setActive( active );
    int iconSize = m_pListViewWidget->iconSize();
    iconSize = iconSize ? iconSize : TDEGlobal::iconLoader()->currentSize( TDEIcon::Small ); // Default = small
    setPixmap( 0, m_fileitem->pixmap( iconSize, state() ) );
}

void KonqListViewItem::setPixmap( int column, const TQPixmap& pm )
{
   if ( column < 0 )
      return;

   const TQPixmap *current = pixmap( column );

   if ( ( pm.isNull() && !current ) ||
        ( current && pm.serialNumber() == current->serialNumber() ) )
      return;

   int oldWidth = current ? current->width() : 0;
   int oldHeight = current ? current->height() : 0;

   if ( (int)m_pixmaps.size() <= column )
      m_pixmaps.resize( column+1 );

   delete current;
   m_pixmaps[column] = pm.isNull() ? 0 : new TQPixmap( pm );

   int newWidth = pm.isNull() ? 0 : pm.width();
   int newHeight = pm.isNull() ? 0 : pm.height();

   // If the height or width have changed then we're going to have to repaint
   // this whole thing.  Fortunately since most of the calls are coming from
   // setActive() this is the uncommon case.

   if ( oldWidth != newWidth || oldHeight != newHeight )
   {
      setup();
      widthChanged( column );
      invalidateHeight();
      return;
   }

   // If we're just replacing the icon with another one its size -- i.e. a
   // "highlighted" icon, don't bother repainting the whole widget.

   TQListView *lv = m_pListViewWidget;

   int decorationWidth = lv->treeStepSize() * ( depth() + ( lv->rootIsDecorated() ? 1 : 0 ) );
   int x = lv->header()->sectionPos( column ) + decorationWidth + lv->itemMargin();
   int y = lv->itemPos( this );
   int w = newWidth;
   int h = height();
   lv->repaintContents( x, y, w, h );
}

const TQPixmap* KonqListViewItem::pixmap( int column ) const
{
   bool ok;
   if ((int)m_pixmaps.count() <= column)
      return 0;

   TQPixmap *pm = m_pixmaps.at( column, &ok );
   if( !ok )
      return 0;
   return pm;
}

int KonqBaseListViewItem::compare( TQListViewItem* item, int col, bool ascending ) const
{
   KonqListViewItem* k = static_cast<KonqListViewItem*>( item );
   if ( sortChar != k->sortChar )
      // Dirs are always first, even when sorting in descending order
      return !ascending ? k->sortChar - sortChar : sortChar - k->sortChar;

   int numExtra = 0;
   for ( unsigned int i=0; i<m_pListViewWidget->NumberOfAtoms; i++ )
   {
      ColumnInfo *cInfo = &m_pListViewWidget->columnConfigInfo()[i];
      if ( cInfo->udsId == TDEIO::UDS_EXTRA )
          ++numExtra;
      if ( col == cInfo->displayInColumn )
      {
         switch ( cInfo->udsId )
         {
            case TDEIO::UDS_MODIFICATION_TIME:
            case TDEIO::UDS_ACCESS_TIME:
            case TDEIO::UDS_CREATION_TIME:
            {
                time_t t1 = m_fileitem->time( cInfo->udsId );
                time_t t2 = k->m_fileitem->time( cInfo->udsId );
                return ( t1 > t2 ) ? 1 : ( t1 < t2 ) ? -1 : 0;
            }
            case TDEIO::UDS_SIZE:
            {
                TDEIO::filesize_t s1 = m_fileitem->size();
                TDEIO::filesize_t s2 = k->m_fileitem->size();
                return ( s1 > s2 ) ? 1 : ( s1 < s2 ) ? -1 : 0;
            }
            case TDEIO::UDS_EXTRA:
            {
                if ( cInfo->type & TQVariant::DateTime ) {
                    const TQString entryStr1 = retrieveExtraEntry( m_fileitem, numExtra );
                    TQDateTime dt1 = TQT_TQDATETIME_OBJECT(TQDateTime::fromString( entryStr1, Qt::ISODate ));
                    const TQString entryStr2 = retrieveExtraEntry( k->m_fileitem, numExtra );
                    TQDateTime dt2 = TQT_TQDATETIME_OBJECT(TQDateTime::fromString( entryStr2, Qt::ISODate ));
                    return ( dt1 > dt2 ) ? 1 : ( dt1 < dt2 ) ? -1 : 0;
                }
            }
            default:
                break;
         }
         break;
      }
   }
   if ( m_pListViewWidget->caseInsensitiveSort() )
       return text( col ).lower().localeAwareCompare( k->text( col ).lower() );
   else {
       return m_pListViewWidget->m_pSettings->caseSensitiveCompare( text( col ), k->text( col ) );
   }
}

void KonqListViewItem::paintCell( TQPainter *_painter, const TQColorGroup & _cg, int _column, int _width, int _alignment )
{
    TQColorGroup cg( _cg );

    if ( _column == 0 )
    {
        _painter->setFont( m_pListViewWidget->itemFont() );
    }

    cg.setColor( TQColorGroup::Text, m_pListViewWidget->itemColor() );

    TDEListView *lv = static_cast< TDEListView* >( listView() );
    const TQPixmap *pm = TQT_TQPIXMAP_CONST(lv->viewport()->paletteBackgroundPixmap());
    if ( _column == 0 && isSelected() && !lv->allColumnsShowFocus() )
    {
        int newWidth = width( lv->fontMetrics(), lv, _column );
        if ( newWidth > _width )
            newWidth = _width;
        if ( pm && !pm->isNull() )
        {
            cg.setBrush( TQColorGroup::Base, TQBrush( backgroundColor(_column), *pm ) );
            TQPoint o = _painter->brushOrigin();
            _painter->setBrushOrigin( o.x() - lv->contentsX(), o.y() - lv->contentsY() );
            const TQColorGroup::ColorRole crole =
                TQPalette::backgroundRoleFromMode( lv->viewport()->backgroundMode() );
            _painter->fillRect( newWidth, 0, _width - newWidth, height(), cg.brush( crole ) );
            _painter->setBrushOrigin( o );
        }
        else
        {
            _painter->fillRect( newWidth, 0, _width - newWidth, height(), backgroundColor(_column) );
        }

        _width = newWidth;
    }

    TDEListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
}

void KonqListViewItem::paintFocus( TQPainter * _painter, const TQColorGroup & cg, const TQRect & _r )
{
    TQRect r( _r );
    TQListView *lv = static_cast< TQListView * >( listView() );
    r.setWidth( width( lv->fontMetrics(), lv, 0 ) );
    if ( r.right() > lv->header()->sectionRect( 0 ).right() )
        r.setRight( lv->header()->sectionRect( 0 ).right() );
    TQListViewItem::paintFocus( _painter, cg, r );
}

const char* KonqBaseListViewItem::makeAccessString( const mode_t mode)
{
   static char buffer[ 12 ];

   char uxbit,gxbit,oxbit;

   if ( (mode & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) )
      uxbit = 's';
   else if ( (mode & (S_IXUSR|S_ISUID)) == S_ISUID )
      uxbit = 'S';
   else if ( (mode & (S_IXUSR|S_ISUID)) == S_IXUSR )
      uxbit = 'x';
   else
      uxbit = '-';

   if ( (mode & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) )
      gxbit = 's';
   else if ( (mode & (S_IXGRP|S_ISGID)) == S_ISGID )
      gxbit = 'S';
   else if ( (mode & (S_IXGRP|S_ISGID)) == S_IXGRP )
      gxbit = 'x';
   else
      gxbit = '-';

   if ( (mode & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) )
      oxbit = 't';
   else if ( (mode & (S_IXOTH|S_ISVTX)) == S_ISVTX )
      oxbit = 'T';
   else if ( (mode & (S_IXOTH|S_ISVTX)) == S_IXOTH )
      oxbit = 'x';
   else
      oxbit = '-';

   buffer[0] = ((( mode & S_IRUSR ) == S_IRUSR ) ? 'r' : '-' );
   buffer[1] = ((( mode & S_IWUSR ) == S_IWUSR ) ? 'w' : '-' );
   buffer[2] = uxbit;
   buffer[3] = ((( mode & S_IRGRP ) == S_IRGRP ) ? 'r' : '-' );
   buffer[4] = ((( mode & S_IWGRP ) == S_IWGRP ) ? 'w' : '-' );
   buffer[5] = gxbit;
   buffer[6] = ((( mode & S_IROTH ) == S_IROTH ) ? 'r' : '-' );
   buffer[7] = ((( mode & S_IWOTH ) == S_IWOTH ) ? 'w' : '-' );
   buffer[8] = oxbit;
   buffer[9] = 0;

   return buffer;
}

KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget, KFileItem* _fileitem)
:TDEListViewItem(_listViewWidget)
,sortChar(0)
,m_bDisabled(false)
,m_bActive(false)
,m_fileitem(_fileitem)
,m_fileitemURL(_fileitem->url())
,m_pListViewWidget(_listViewWidget)
{}

KonqBaseListViewItem::KonqBaseListViewItem(KonqBaseListViewWidget *_listViewWidget, KonqBaseListViewItem *_parent, KFileItem* _fileitem)
:TDEListViewItem(_parent)
,sortChar(0)
,m_bDisabled(false)
,m_bActive(false)
,m_fileitem(_fileitem)
,m_fileitemURL(_fileitem->url())
,m_pListViewWidget(_listViewWidget)
{}

KonqBaseListViewItem::~KonqBaseListViewItem()
{
   if (m_pListViewWidget->m_activeItem == this)
      m_pListViewWidget->m_activeItem = 0;
   if (m_pListViewWidget->m_dragOverItem == this)
      m_pListViewWidget->m_dragOverItem = 0;

   if (m_pListViewWidget->m_selected)
      m_pListViewWidget->m_selected->removeRef(this);
}

TQRect KonqBaseListViewItem::rect() const
{
    TQRect r = m_pListViewWidget->itemRect(this);
    return TQRect( m_pListViewWidget->viewportToContents( r.topLeft() ), TQSize( r.width(), r.height() ) );
}

void KonqBaseListViewItem::mimetypeFound()
{
    // Update icon
    setDisabled( m_bDisabled );
    uint done = 0;
    KonqBaseListViewWidget * lv = m_pListViewWidget;
    for (unsigned int i=0; i<m_pListViewWidget->NumberOfAtoms && done < 2; i++)
    {
        ColumnInfo *tmpColumn=&lv->columnConfigInfo()[i];
        if (lv->columnConfigInfo()[i].udsId==TDEIO::UDS_FILE_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimeComment());
            done++;
        }
        if (lv->columnConfigInfo()[i].udsId==TDEIO::UDS_MIME_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimetype());
            done++;
        }
    }
}

