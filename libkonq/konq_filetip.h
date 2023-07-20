/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000, 2001, 2002 David Faure <david@mandrakesoft.com>
   Copyright (C) 2004 Martin Koller <m.koller@surfeu.at>

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

#ifndef KONQ_FILETIP_H
#define KONQ_FILETIP_H

#include <tqframe.h>
#include <tqpixmap.h>
#include <tdeio/previewjob.h>

#include <libkonq_export.h>

class KFileItem;
class TQLabel;
class TQScrollView;
class TQTimer;

//--------------------------------------------------------------------------------

class LIBKONQ_EXPORT KonqFileTip : public TQFrame
{
  TQ_OBJECT

  public:
    KonqFileTip( TQScrollView *parent );
    ~KonqFileTip();

    void setPreview(bool on);

    /**
      @param on show tooltip at all
      @param preview include file preview in tooltip
      @param num the number of tooltip texts to get from KFileItem
      */
    void setOptions( bool on, bool preview, int num );

    /** Set the item from which to get the tip information
      @param item the item from which to get the tip information
      @param rect the rectangle around which the tip will be shown
      @param pixmap the pixmap to be shown. If 0, no pixmap is shown
      */
    void setItem( KFileItem *item, const TQRect &rect = TQRect(),
                  const TQPixmap *pixmap = 0 );

    virtual bool eventFilter( TQObject *, TQEvent *e );

  protected:
    virtual void drawContents( TQPainter *p );
    virtual void resizeEvent( TQResizeEvent * );

  private slots:
    void gotPreview( const KFileItem*, const TQPixmap& );
    void gotPreviewResult();

    void startDelayed();
    void showTip();
    void hideTip();

  private:
    void setFilter( bool enable );

    void reposition();

    TQLabel*    m_iconLabel;
    TQLabel*    m_textLabel;
    bool       m_on : 1;
    bool       m_preview : 1;  // shall the preview icon be shown
    bool       m_filter : 1;
    TQPixmap    m_corners[4];
    int        m_corner;
    int        m_num;
    TQScrollView* m_view;
    KFileItem* m_item;
    TDEIO::PreviewJob* m_previewJob;
    TQRect      m_rect;
    TQTimer*    m_timer;
};

#endif
