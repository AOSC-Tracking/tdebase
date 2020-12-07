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

#include "konq_textviewitem.h"
#include "konq_settings.h"

#include <assert.h>
#include <stdio.h>
#include <tdeglobal.h>

int KonqTextViewItem::compare( TQListViewItem *item, int col, bool ascending ) const
{
   if (col==1)
      return KonqBaseListViewItem::compare(item, 0, ascending);
   return KonqBaseListViewItem::compare(item, col, ascending);
}

/*TQString KonqTextViewItem::key( int _column, bool asc) const
{
   if (_column==1) return key(0,asc);
   TQString tmp = TQString::number( sortChar );
   //check if it is a time column
   if (_column>1)
   {
      KonqTextViewWidget* lv = static_cast<KonqTextViewWidget *>(listView());
      for (unsigned int i=0; i<lv->NumberOfAtoms; i++)
      {
         ColumnInfo *cInfo=&lv->columnConfigInfo()[i];
         if (_column==cInfo->displayInColumn)
         {
            if ((cInfo->udsId==TDEIO::UDS_MODIFICATION_TIME)
                || (cInfo->udsId==TDEIO::UDS_ACCESS_TIME)
                || (cInfo->udsId==TDEIO::UDS_CREATION_TIME))
            {
               tmp += TQString::number( m_fileitem->time(cInfo->udsId) ).rightJustify( 14, '0' );
               return tmp;
            }
            else if (cInfo->udsId==TDEIO::UDS_SIZE)
            {
               tmp += TDEIO::number( m_fileitem->size() ).rightJustify( 20, '0' );
               return tmp;
            }
            else break;

         };
      };
   };
   tmp+=text(_column);
   return tmp;
}*/

void KonqTextViewItem::updateContents()
{
   TQString tmp;
   TDEIO::filesize_t size=m_fileitem->size();
   mode_t m=m_fileitem->mode();

   // The order is: .dir (0), dir (1), .file (2), file (3)
   sortChar = S_ISDIR( m_fileitem->mode() ) ? 1 : 3;
   if ( m_fileitem->text()[0] == '.' )
       --sortChar;

   if (m_fileitem->isLink())
   {
      if (S_ISDIR(m))
      {
         type=KTVI_DIRLINK;
         tmp="~";
      }
      else if ((S_ISREG(m)) || (S_ISCHR(m)) || (S_ISBLK(m)) || (S_ISSOCK(m)) || (S_ISFIFO(m)))
      {
         tmp="@";
         type=KTVI_REGULARLINK;
      }
      else
      {
         tmp="!";
         type=KTVI_UNKNOWN;
         size=0;
      };
   }
   else if (S_ISREG(m))
   {
      if ((m_fileitem->permissions() & (S_IXUSR|S_IXGRP|S_IXOTH)) !=0 )
      {
         tmp="*";
         type=KTVI_EXEC;
      }
      else
      {
         tmp="";
         type=KTVI_REGULAR;
      };
   }
   else if (S_ISDIR(m))
   {
      type=KTVI_DIR;
      tmp="/";
   }
   else if (S_ISCHR(m))
   {
      type=KTVI_CHARDEV;
      tmp="-";
   }
   else if (S_ISBLK(m))
   {
      type=KTVI_BLOCKDEV;
      tmp="+";
   }
   else if (S_ISSOCK(m))
   {
      type=KTVI_SOCKET;
      tmp="=";
   }
   else if (S_ISFIFO(m))
   {
      type=KTVI_FIFO;
      tmp=">";
   }
   else
   {
      tmp="!";
      type=KTVI_UNKNOWN;
      size=0;
   };
   setText(1,tmp);
   setText(0,m_fileitem->text());
   //now we have the first two columns, so let's do the rest
   KonqTextViewWidget* lv = static_cast<KonqTextViewWidget *>(listView());

   for (unsigned int i=0; i<lv->NumberOfAtoms; i++)
   {
      ColumnInfo *tmpColumn=&lv->confColumns[i];
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
         case TDEIO::UDS_LINK_DEST:
            setText(tmpColumn->displayInColumn,m_fileitem->linkDest());
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
         case TDEIO::UDS_SIZE:
            if ( static_cast<KonqBaseListViewWidget *>(listView())->m_pSettings->fileSizeInBytes() )
                setText(tmpColumn->displayInColumn,TDEGlobal::locale()->formatNumber(size, 0)+" ");
            else
                setText(tmpColumn->displayInColumn,TDEIO::convertSize(size)+" ");
            break;
         case TDEIO::UDS_ACCESS:
            setText(tmpColumn->displayInColumn,m_fileitem->permissionsString());
            break;
         case TDEIO::UDS_MODIFICATION_TIME:
         case TDEIO::UDS_ACCESS_TIME:
         case TDEIO::UDS_CREATION_TIME:
            for( TDEIO::UDSEntry::ConstIterator it = m_fileitem->entry().begin(); it != m_fileitem->entry().end(); it++ )
            {
               if ((*it).m_uds==(unsigned int)tmpColumn->udsId)
               {
                  TQDateTime dt;
                  dt.setTime_t((time_t) (*it).m_long);
                  setText(tmpColumn->displayInColumn,TDEGlobal::locale()->formatDateTime(dt));
                  break;
               };

            };
            break;
         default:
            break;
         };
      };
   };
}

void KonqTextViewItem::paintCell( TQPainter *_painter, const TQColorGroup & _cg, int _column, int _width, int _alignment )
{
   TQColorGroup cg( _cg );
   cg.setColor(TQColorGroup::Text, static_cast<KonqTextViewWidget *>(listView())->colors[type]);
   // Don't do that! Keep things readable whatever the selection background color is
//   cg.setColor(TQColorGroup::HighlightedText, static_cast<KonqTextViewWidget *>(listView())->highlight[type]);
//   cg.setColor(TQColorGroup::Highlight, Qt::darkGray);

   TDEListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
}

/*void KonqTextViewItem::paintFocus( TQPainter *_p, const TQColorGroup &_cg, const TQRect &_r )
{
   listView()->style().drawFocusRect( _p, _r, _cg,
           isSelected() ? &_cg.highlight() : &_cg.base(), isSelected() );

   TQPixmap pix( _r.width(), _r.height() );
   bitBlt( &pix, 0, 0, _p->device(), _r.left(), _r.top(), _r.width(), _r.height() );
   TQImage im = pix.convertToImage();
   im = KImageEffect::fade( im, 0.25, Qt::black );
   _p->drawImage( _r.topLeft(), im );
}*/

void KonqTextViewItem::setup()
{
   widthChanged();
   int h(listView()->fontMetrics().height());
   if ( h % 2 > 0 ) h++;
   setHeight(h);
}

void KonqTextViewItem::mimetypeFound()
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