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

#include <konq_filetip.h>

#include <kfileitem.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kapplication.h>

#include <tqlabel.h>
#include <tqtooltip.h>
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqscrollview.h>
#include <tqtimer.h>

#include <fixx11h.h>
//--------------------------------------------------------------------------------

KonqFileTip::KonqFileTip( TQScrollView* parent )
  : TQFrame( 0, 0, (WFlags)(WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WStyle_StaysOnTop | WX11BypassWM) ),
    m_on( false ),
    m_preview( false ),
    m_filter( false ),
    m_corner( 0 ),
    m_num( 0 ),
    m_view( parent ),
    m_item( 0 ),
    m_previewJob( 0 )
{
    m_iconLabel = new TQLabel(this);
    m_textLabel = new TQLabel(this);
    m_textLabel->setAlignment(TQt::AlignAuto | TQt::AlignTop);

    TQGridLayout* layout = new TQGridLayout(this, 1, 2, 8, 0);
    layout->addWidget(m_iconLabel, 0, 0);
    layout->addWidget(m_textLabel, 0, 1);
    layout->setResizeMode(TQLayout::Fixed);

    setPalette( TQToolTip::palette() );
    setMargin( 1 );
    setFrameStyle( TQFrame::Plain | TQFrame::Box );

    m_timer = new TQTimer(this);

    hide();
}

KonqFileTip::~KonqFileTip()
{
   if ( m_previewJob ) {
        m_previewJob->kill();
        m_previewJob = 0;
    }
}

void KonqFileTip::setPreview(bool on)
{
    m_preview = on;
    if(on)
        m_iconLabel->show();
    else
        m_iconLabel->hide();
}

void KonqFileTip::setOptions( bool on, bool preview, int num )
{
    setPreview(preview);
    m_on = on;
    m_num = num;
}

void KonqFileTip::setItem( KFileItem *item, const TQRect &rect, const TQPixmap *pixmap )
{
    hideTip();

    if (!m_on) return;

    if ( m_previewJob ) {
        m_previewJob->kill();
        m_previewJob = 0;
    }

    m_rect = rect;
    m_item = item;

    if ( m_item ) {
        if (m_preview) {
            if ( pixmap )
              m_iconLabel->setPixmap( *pixmap );
            else
              m_iconLabel->setPixmap( TQPixmap() );
        }

        // Don't start immediately, because the user could move the mouse over another item
        // This avoids a quick sequence of started preview-jobs
        m_timer->disconnect( this );
        connect(m_timer, TQT_SIGNAL(timeout()), this, TQT_SLOT(startDelayed()));
        m_timer->start( 300, true );
    }
}

void KonqFileTip::reposition()
{
    if ( m_rect.isEmpty() || !m_view || !m_view->viewport() ) return;

    TQRect rect = m_rect;
    TQPoint off = m_view->viewport()->mapToGlobal( m_view->contentsToViewport( rect.topRight() ) );
    rect.moveTopRight( off );

    TQPoint pos = rect.center();
    // m_corner:
    // 0: upperleft
    // 1: upperright
    // 2: lowerleft
    // 3: lowerright
    // 4+: none
    m_corner = 0;
    // should the tooltip be shown to the left or to the right of the ivi ?
    TQRect desk = KGlobalSettings::desktopGeometry(rect.center());
    if (rect.center().x() + width() > desk.right())
    {
        // to the left
        if (pos.x() - width() < 0) {
            pos.setX(0);
            m_corner = 4;
        } else {
            pos.setX( pos.x() - width() );
            m_corner = 1;
        }
    }
    // should the tooltip be shown above or below the ivi ?
    if (rect.bottom() + height() > desk.bottom())
    {
        // above
        pos.setY( rect.top() - height() );
        m_corner += 2;
    }
    else pos.setY( rect.bottom() + 1 );

    move( pos );
    update();
}

void KonqFileTip::gotPreview( const KFileItem* item, const TQPixmap& pixmap )
{
    m_previewJob = 0;
    if (item != m_item) return;

    m_iconLabel -> setPixmap(pixmap);
}

void KonqFileTip::gotPreviewResult()
{
    m_previewJob = 0;
}

void KonqFileTip::drawContents( TQPainter *p )
{
    static const char * const names[] = {
        "arrow_topleft",
        "arrow_topright",
        "arrow_bottomleft",
        "arrow_bottomright"
    };

    if (m_corner >= 4) {  // 4 is empty, so don't draw anything
        TQFrame::drawContents( p );
        return;
    }

    if ( m_corners[m_corner].isNull())
        m_corners[m_corner].load( locate( "data", TQString::fromLatin1( "konqueror/pics/%1.png" ).arg( names[m_corner] ) ) );

    TQPixmap &pix = m_corners[m_corner];

    switch ( m_corner )
    {
        case 0:
            p->drawPixmap( 3, 3, pix );
            break;
        case 1:
            p->drawPixmap( width() - pix.width() - 3, 3, pix );
            break;
        case 2:
            p->drawPixmap( 3, height() - pix.height() - 3, pix );
            break;
        case 3:
            p->drawPixmap( width() - pix.width() - 3, height() - pix.height() - 3, pix );
            break;
    }

    TQFrame::drawContents( p );
}

void KonqFileTip::setFilter( bool enable )
{
    if ( enable == m_filter ) return;

    if ( enable ) {
        kapp->installEventFilter( this );
        TQApplication::setGlobalMouseTracking( true );
    }
    else {
        TQApplication::setGlobalMouseTracking( false );
        kapp->removeEventFilter( this );
    }
    m_filter = enable;
}

void KonqFileTip::showTip()
{
    TQString text = m_item->getToolTipText(m_num);

    if ( text.isEmpty() ) return;

    m_timer->disconnect( this );
    connect(m_timer, TQT_SIGNAL(timeout()), this, TQT_SLOT(hideTip()));
    m_timer->start( 15000, true );

    m_textLabel->setText( text );

    setFilter( true );

    reposition();
    show();
}

void KonqFileTip::hideTip()
{
    m_timer->stop();
    setFilter( false );
    if ( isShown() && m_view && m_view->viewport() &&
         (m_view->horizontalScrollBar()->isShown() || m_view->verticalScrollBar()->isShown()) )
      m_view->viewport()->update();
    hide();
}
void KonqFileTip::startDelayed()
{
    if ( m_preview ) {
        KFileItemList oneItem;
        oneItem.append( m_item );

        m_previewJob = KIO::filePreview( oneItem, 256, 256, 64, 70, true, true, 0);
        connect( m_previewJob, TQT_SIGNAL( gotPreview( const KFileItem *, const TQPixmap & ) ),
                 this, TQT_SLOT( gotPreview( const KFileItem *, const TQPixmap & ) ) );
        connect( m_previewJob, TQT_SIGNAL( result( KIO::Job * ) ),
                 this, TQT_SLOT( gotPreviewResult() ) );
    }

    m_timer->disconnect( this );
    connect(m_timer, TQT_SIGNAL(timeout()), this, TQT_SLOT(showTip()));
    m_timer->start( 400, true );
}

void KonqFileTip::resizeEvent( TQResizeEvent* event )
{
    TQFrame::resizeEvent(event);
    reposition();
}

bool KonqFileTip::eventFilter( TQObject *, TQEvent *e )
{
    switch ( e->type() )
    {
        case TQEvent::Leave:
        case TQEvent::MouseButtonPress:
        case TQEvent::MouseButtonRelease:
        case TQEvent::KeyPress:
        case TQEvent::KeyRelease:
        case TQEvent::FocusIn:
        case TQEvent::FocusOut:
        case TQEvent::Wheel:
            hideTip();
        default: break;
    }

    return false;
}

#include "konq_filetip.moc"
