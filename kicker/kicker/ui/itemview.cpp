/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <dmctl.h>

#include <tqapplication.h>
#include <tqimage.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <tqwidgetstack.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqvbox.h>
#include <tqheader.h>
#include <tqdrawutil.h>
#include <tqdragobject.h>
#include <tqcursor.h>
#include <tqpaintdevicemetrics.h>
#include <tqbuffer.h>
#include <tqtooltip.h>
#include <tqstylesheet.h>
#include <tqiconview.h>

#include <dcopclient.h>
#include <kapplication.h>
#include <kaboutkde.h>
#include <kpixmapeffect.h>
#include <kaction.h>
#include <kbookmarkmenu.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kcombobox.h>
#include <kwin.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <kmultipledrag.h>

#include "client_mnu.h"
#include "container_base.h"
#include "global.h"
#include "kbutton.h"
#include "kicker.h"
#include "kickerSettings.h"
#include "konqbookmarkmanager.h"
#include "menuinfo.h"
#include "menumanager.h"
#include "popupmenutitle.h"
#include "quickbrowser_mnu.h"
#include "recentapps.h"

#include "k_mnu.h"
#include "k_new_mnu.h"
#include "itemview.h"

// --------------------------------------------------------------------------

KMenuItem::~KMenuItem()
{
    ItemView *listview = dynamic_cast<ItemView*>( listView() );
    if ( listview && listview->m_lastOne == this) {
      listview->m_lastOne = 0;
      listview->m_old_contentY = -1;
    }
}

static double pointSize( double pixelSize, TQPaintDevice *w )
{
    return pixelSize * 72. / TQPaintDevice::x11AppDpiY( w->x11Screen () );
}

static int pixelSize( double pixelSize, TQPaintDevice *w )
{
    return tqRound( pixelSize * TQPaintDevice::x11AppDpiY( w->x11Screen () ) / 72. );
}

void KMenuItem::init()
{
    setMultiLinesEnabled(true);
    m_s = 0;
    m_path = TQString();
    m_icon = TQString();
    m_menuPath = TQString();
    setDragEnabled(true);
    m_has_children = false;
    m_old_width = -1;
    if ( TQApplication::reverseLayout() )
        right_triangle.load( locate( "data", "kicker/pics/left_triangle.png" ) );
    else
        right_triangle.load( locate( "data", "kicker/pics/right_triangle.png" ) );
}

void KMenuItem::setTitle(const TQString& txt)
{
    m_title = txt;
    setText( 0, txt );
    setup();
}

void KMenuItem::setToolTip(const TQString& txt)
{
    m_tooltip = txt;
}

void KMenuItem::setDescription(const TQString& txt)
{
    m_description = txt;
    setup();
}

void KMenuItem::setIcon(const TQString& icon, int size)
{
    m_icon = icon;
    TQListViewItem::setPixmap(0, KGlobal::iconLoader()->loadIcon(icon, KIcon::Panel, size ));
}

void KMenuItem::setHasChildren( bool flag )
{
    m_has_children = flag;
    repaint();
}

void KMenuItem::setup()
{
    // if someone configured a larger generalFont than 10pt, he might have a _real_ problem with 7pt
    // the 7pt could be read out of konquerorrc I guess
    float min_font_size = 7. * TQMAX(1., KGlobalSettings::generalFont().pointSizeFloat() / 10.);

    const int expected_height = 38;
    description_font_size = TQMAX( pointSize( expected_height * .3, TQT_TQPAINTDEVICE(listView()) ) + KickerSettings::kickoffFontPointSizeOffset(), min_font_size ) ;
    title_font_size = TQMAX( pointSize( expected_height * .25, TQT_TQPAINTDEVICE(listView()) ) + KickerSettings::kickoffFontPointSizeOffset(), min_font_size + 1 );

    //kdDebug() << description_font_size << " " << title_font_size << " " << pointSize( expected_height * .25, listView() ) << endl;
    TQListViewItem::setup();
    setHeight( (int)TQMAX( expected_height, pixelSize( title_font_size + description_font_size * 2.3, TQT_TQPAINTDEVICE(listView()))));
}

void KMenuItem::paintCell(TQPainter* p, const TQColorGroup & cg, int column, int width, int align)
{
    ItemView *listview = static_cast<ItemView*>( listView() );
    int bottom = listView()->itemRect( this ).bottom();
    int diff = bottom - listView()->viewport()->height();

    KPixmap pm;
    pm.resize( width, height() );
    TQPainter pp( &pm );
    paintCellInter( &pp, cg, column, width, align );
    pp.end();

    if ( diff > 0 && diff <= height() ) // cut off
    {
        pm.resize( width, height() - diff );
        KPixmapEffect::blend( pm, float( diff ) / height(),
                              cg.color( TQColorGroup::Background ),
                              KPixmapEffect::VerticalGradient );
        p->drawPixmap( 0, 0, pm );
        if ( listview->m_lastOne != this )
        {
            listview->m_lastOne = this;
            listview->m_old_contentY = -1;
        }
    }
    else
    {
        p->drawPixmap( 0, 0, pm );
        if ( this == listview->m_lastOne ) {
            if ( bottom < 0 )
                listview->m_lastOne = static_cast<KMenuItem*>( itemAbove() );
            else
                listview->m_lastOne = static_cast<KMenuItem*>( itemBelow() );
            listview->m_old_contentY = -1;
            repaint();
        }
    }
}

void KMenuItem::makeGradient( KPixmap &off, const TQColor &c )
{
    KPixmap blend;
    blend.resize( off.width() / 3, off.height() );
    bitBlt( &blend, 0, 0, &off, off.width() - blend.width(), 0, blend.width(), blend.height() );
    KPixmapEffect::blend( blend, 0.2, c, KPixmapEffect::HorizontalGradient );
    TQPainter p( &off );
    p.drawPixmap( off.width() - blend.width(), 0, blend );
    p.end();
}

void KMenuItem::paintCellInter(TQPainter* p, const TQColorGroup & cg, int column, int width, int align)
{
    const bool reverseLayout = TQApplication::reverseLayout();

    const BackgroundMode bgmode = listView()->viewport()->backgroundMode();
    const TQColorGroup::ColorRole crole = TQPalette::backgroundRoleFromMode( bgmode );
    TQColor backg = cg.color( crole );

    if ( isSelected() )
        backg = cg.color( TQColorGroup::Highlight );
    p->fillRect( 0, 0, width, height(), backg );

    TQFontMetrics fm( p->fontMetrics() );

    int pixsize = 32;
    if ( height() < 36 )
        pixsize = 16;
    const int left_margin = 30;
    const int margin = 3;

//    p->drawText( 2, 2, left_margin - 2, height(), align, TQString::number( childCount () ) );

    const TQPixmap * pix = pixmap( column );

    if ( pix )
    {
        TQPixmap pix32 = *pix;

        if ( pix->width() > pixsize )
        {
            TQImage i = pix->convertToImage().smoothScale( pixsize, pixsize );
            pix32.convertFromImage( i );
        }
        if ( reverseLayout )
            p->drawPixmap( width - ( (pixsize - pix32.width()) / 2 + left_margin ) - pix32.width(),
                       ( height() - pix32.height() ) / 2, pix32 );
        else
            p->drawPixmap( (pixsize - pix32.width()) / 2 + left_margin,
                       ( height() - pix32.height() ) / 2, pix32 );
    }

    if ( m_title.isEmpty() )
        return;

    int r = left_margin + pixsize + margin * 2;

    TQFont f1 = p->font();
    f1.setPointSizeFloat( title_font_size );
    f1.setWeight( TQFont::Normal ); // TQFont::DemiBold == 63

    TQFont f2 = p->font();
    f2.setPointSizeFloat( description_font_size );
    f2.setWeight( TQFont::Light );

    int f1h = TQFontMetrics( f1 ).height();
    int f2h = TQFontMetrics( f2 ).height();

    const int text_margin = 2;
    int spacing = ( height() - f1h - f2h - text_margin ) / 2;
    if ( m_description.isEmpty() )
        spacing = ( height() - f1h ) / 2;

    int right_triangle_size = pixelSize( 7, TQT_TQPAINTDEVICE(listView()) );

    int right_margin = listView()->verticalScrollBar()->width();
    if ( m_has_children )
        right_margin += right_triangle_size * 2;

    KPixmap off;
    TQPainter pp;

    off.resize( width-text_margin-r-right_margin, height() );
    pp.begin( &off );
    pp.fillRect( 0, 0, off.width(), off.height(), backg );

    if (isSelected())
       pp.setPen( cg.color( TQColorGroup::HighlightedText ) );
    else
       pp.setPen( cg.color( TQColorGroup::Text ) );

    pp.setFont( f1 );
    pp.drawText( 0, 0, off.width(), off.height(), align, m_title );
    pp.end();
    if ( TQFontMetrics( f1 ).width( m_title ) > off.width() )
    {
        makeGradient( off, backg );
        if ( !m_description.isEmpty() )
            setToolTip( m_title + "<br><br>" + m_description );
        else
            setToolTip( m_title );
    }
    if ( reverseLayout )
        p->drawPixmap( width - off.width() - r, spacing, off );
    else
        p->drawPixmap( r, spacing, off );

    if ( !m_description.isEmpty() )
    {
        pp.begin( &off );
        pp.fillRect( 0, 0, off.width(), off.height(), backg );

        TQColor myColor = cg.color( TQColorGroup::Text ).light( 200 );
        if ( tqGray( myColor.rgb() ) == 0 )
            myColor = TQColor( 100, 100, 110 );
        pp.setPen( myColor );
        pp.setPen( isSelected() ? cg.color( TQColorGroup::Mid ) : myColor );
        pp.setFont( f2 );
        pp.drawText( 0, 0, off.width(), off.height(), align, m_description );
        pp.end();
        if ( TQFontMetrics( f2 ).width( m_description ) > off.width() )
        {
            makeGradient( off, backg );
            setToolTip( m_title + "<br><br>" + m_description );
        }
        if ( reverseLayout )
            p->drawPixmap( width - off.width() - r, spacing + text_margin + f1h, off );
        else
            p->drawPixmap( r, spacing + text_margin + f1h, off );
    }

    if ( m_has_children )
    {
        TQImage i = right_triangle.convertToImage().smoothScale( right_triangle_size,
                                                                right_triangle_size );
        TQPixmap tri;
        tri.convertFromImage( i );

        if ( reverseLayout )
            p->drawPixmap( right_margin - tri.width(), ( height() - f1h ) / 2, tri );
        else
            p->drawPixmap( listView()->width() -  right_margin, ( height() - f1h ) / 2, tri );
    }

    if ( m_old_width != width )
    {
        // the listview caches paint events
        m_old_width = width;
        repaint();
    }
}

// --------------------------------------------------------------------------

KMenuItemSeparator::KMenuItemSeparator(int nId, TQListView* parent)
    : KMenuItem(nId, parent), lv(parent), cached_width( 0 )
{
    setEnabled(false);
    left_margin = 15;
}

void KMenuItemSeparator::setup()
{
    KMenuItem::setup();

    TQFont f = TQFont();
    TQFontMetrics fm(f);
    f.setPointSize( 8 + KickerSettings::kickoffFontPointSizeOffset() );
    if ( itemAbove() && !text( 0 ).isEmpty() )
        setHeight( (int)TQMAX( 34., fm.height() * 1.4) );
    else
        setHeight( (int)TQMAX( 26., fm.height() * 1.4 ) );
}

void KMenuItemSeparator::setLink( const TQString &text, const TQString &url )
{
    m_link_text = text;
    m_link_url = url;
    m_link_rect = TQRect();
}

bool KMenuItemSeparator::hitsLink( const TQPoint &pos )
{
    return m_link_rect.contains( pos );
}

void KMenuItemSeparator::preparePixmap( int width )
{
    if ( cached_width != width )
    {
        pixmap.load( locate("data", "kicker/pics/menu_separator.png" ) );
        TQImage i = pixmap.convertToImage().smoothScale( width - 15 - left_margin, pixmap.height() );
        pixmap.convertFromImage( i );
        cached_width = width;
    }
}

void KMenuItemSeparator::paintCell(TQPainter* p, const TQColorGroup & cg, int column, int width, int align)
{
    preparePixmap(width);

    const int h = height();

    if (text(0).isEmpty()) {
      KMenuItem::paintCell(p, cg, column, width, align);
      p->drawPixmap( 15 , h/2, pixmap );
    }
    else {
      const BackgroundMode bgmode = lv->viewport()->backgroundMode();
      const TQColorGroup::ColorRole crole = TQPalette::backgroundRoleFromMode( bgmode );
      p->fillRect( 0, 0, width, h, cg.brush( crole ) );

      int margin = 0;
      if ( itemAbove() ) {
          p->drawPixmap( 15 , h/4, pixmap );
          margin = h / 4;
      }
      TQFont f = listView()->font();
      f.setWeight( TQFont::Normal );
      f.setPointSize( 8 + KickerSettings::kickoffFontPointSizeOffset() );
      p->setFont( f );
      TQColor myColor = cg.color( TQColorGroup::Text ).light( 200 );
      if ( tqGray( myColor.rgb() ) == 0 )
          myColor = TQColor( 100, 100, 110 );
      p->setPen( myColor );
      int twidth = p->fontMetrics().width(text(0));
      int lwidth = 0;
      int swidth = 0;
      int fwidth = 0;

      if ( !m_link_text.isEmpty() )
      {
          swidth = p->fontMetrics().width( " (" );
          lwidth = p->fontMetrics().width(m_link_text );
          fwidth = p->fontMetrics().width( ")" );
      }
      int pos = int(lv->width() * 0.9 - twidth - swidth - lwidth - fwidth);
      p->drawText( pos, margin + 5,
                   width, h - ( margin +5 ), AlignTop, text(0) );
      if ( !m_link_text.isEmpty() )
      {
          pos += twidth;
          p->drawText( pos, margin + 5,
                       width, h - ( margin +5 ), AlignTop, " (" );
          pos += swidth;
          p->setPen( cg.color( TQColorGroup::Link ) );
          f.setUnderline( true );
          p->setFont( f );
          p->drawText( pos, margin + 5,
                       width, h - ( margin +5 ), AlignTop, m_link_text );
          m_link_rect = TQRect( pos, margin + 5, lwidth, p->fontMetrics().height() );
          pos += lwidth;
          f.setUnderline( false );
          p->setFont( f );
          p->drawText( pos, margin + 5,
                       width, h - ( margin +5 ), AlignTop, ")" );
      }
    }
}

KMenuItemHeader::KMenuItemHeader(int nId, const TQString& relPath, TQListView* parent)
    : KMenuItemSeparator(nId, parent)
{
    setEnabled( false );
    TQString path;
    if (relPath.startsWith( "new/" /*"kicker:/new/"*/ )) {
       paths.append( "kicker:/goup/" );
       texts.append( i18n("New Applications") );
       icons.append( "clock" );
    }
    else if (relPath == "kicker:/restart/") {
       texts.append( i18n("Restart Computer") );
    }
    else if (relPath == "kicker:/switchuser/") {
       texts.append( i18n("Switch User") );
    }
    else {
      KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(relPath);
      TQStringList items = TQStringList::split( '/', relPath );
      for ( TQStringList::ConstIterator it = items.begin(); it != items.end(); ++it )
      {
        path += *it + "/";
        paths.append( "kicker:/goup/" + path );
        KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(path);
        TQString groupCaption = subMenuRoot->caption();
        texts.append( groupCaption );
        icons.append( subMenuRoot->icon() );
      }
    }

    setPath( "kicker:/goup/" + path ); // the last wins for now
    left_margin = 10;
}

void KMenuItemHeader::setup()
{
    KMenuItem::setup();

    TQFontMetrics fm( listView()->font() );
    setHeight( TQMAX( int( texts.count() * fm.height() + ( texts.count() + 1 ) * 2 + 10 ), height()) );
    // nada
}

void KMenuItemHeader::paintCell(TQPainter* p, const TQColorGroup & cg, int , int width, int align )
{
    preparePixmap(width);

    const BackgroundMode bgmode = listView()->viewport()->backgroundMode();
    const TQColorGroup::ColorRole crole = TQPalette::backgroundRoleFromMode( bgmode );

    TQBrush br = cg.brush( crole );
    if ( isSelected() ) {
        br = cg.brush( TQColorGroup::Highlight );
        p->fillRect( 0, 0, width, height() - 3, br );
    } else {
        p->fillRect( 0, 0, width, height(), br );
    }

    TQFontMetrics fm( p->fontMetrics() );
    const int left_margin = 10;

    const int margin = 3;

    int r = left_margin + margin * 2;

    const int min_font_size = 7;
    int title_font_pixelSize = tqRound( pixelSize( TQMAX( pointSize( 12, TQT_TQPAINTDEVICE(listView()) ) + KickerSettings::kickoffFontPointSizeOffset(), min_font_size + 1 ), TQT_TQPAINTDEVICE(listView()) ) );

    TQFont f1 = p->font();
    f1.setPixelSize( title_font_pixelSize );
    p->setFont( f1 );
    int f1h = TQFontMetrics( f1 ).height();

    p->setPen( cg.color( TQColorGroup::Text ) );

    const int text_margin = 2;
    int spacing = ( height() - texts.count() * f1h - TQMAX( texts.count() - 1, 0 ) * text_margin ) / 2;

    for ( uint i = 0; i < texts.count(); ++i )
    {
        if (i==texts.count()-1) {
            f1.setWeight( TQFont::DemiBold );
            p->setFont( f1 );
            f1h = TQFontMetrics( f1 ).height();
        }

        p->drawText( r, spacing, width-text_margin-r, height(), align, texts[i] );
        spacing += text_margin + f1h;
        r += title_font_pixelSize;
    }

    p->drawPixmap( left_margin , height() - 2, pixmap );
}

KMenuSpacer::KMenuSpacer(int nId, TQListView* parent)
    : KMenuItem(nId, parent)
{
    setEnabled(false);
}

void KMenuSpacer::setup()
{
    // nada
}

void KMenuSpacer::paintCell(TQPainter* p, const TQColorGroup & cg, int , int width, int )
{
    const BackgroundMode bgmode = listView()->viewport()->backgroundMode();
    const TQColorGroup::ColorRole crole = TQPalette::backgroundRoleFromMode( bgmode );
    TQBrush br = cg.brush( crole );

    p->fillRect( 0, 0, width, height(), br );
}

void KMenuSpacer::setHeight( int i )
{
    KMenuItem::setHeight( i );
}

class ItemViewTip : public TQToolTip
{
public:
    ItemViewTip( TQWidget *parent, TQListView *lv );

    void maybeTip( const TQPoint &pos );

private:
    TQListView *view;

};

ItemViewTip::ItemViewTip( TQWidget *parent, TQListView *lv )
    : TQToolTip( parent ), view( lv )
{
}

void ItemViewTip::maybeTip( const TQPoint &pos )
{
    KMenuItem *item = dynamic_cast<KMenuItem*>( view->itemAt( pos ) );
    TQPoint contentsPos = view->viewportToContents( pos );
    if ( !item )
        return;

    if ( item->toolTip().isNull() )
        return;

    TQRect r = view->itemRect( item );
    int headerPos = view->header()->sectionPos( 0 );
    r.setLeft( headerPos );
    r.setRight( headerPos + view->header()->sectionSize( 0 ) );
    tip( r, item->toolTip() );
}

// --------------------------------------------------------------------------

ItemView::ItemView(TQWidget* parent, const char* name)
    : KListView(parent, name), m_spacer( 0 ),
      m_mouseMoveSelects(true), m_iconSize(32)
{
    setHScrollBarMode( TQScrollView::AlwaysOff );
    setFrameStyle( TQFrame::NoFrame );
    setSelectionMode(TQListView::Single);
    addColumn("");
    header()->setStretchEnabled(1, 0);
    //setColumnWidthMode(0, TQListView::Maximum);
    header()->hide();
    setMouseTracking(true);
    setItemMargin(0);
    setSorting(-1);
    setTreeStepSize(38);
    setFocusPolicy(TQ_NoFocus);

    m_lastOne = 0;
    m_old_contentY = -1;

    connect(this, TQT_SIGNAL(mouseButtonClicked( int, TQListViewItem*, const TQPoint &, int )),
                  TQT_SLOT(slotItemClicked(int, TQListViewItem*, const TQPoint &, int)));

    connect(this, TQT_SIGNAL(returnPressed(TQListViewItem*)), TQT_SLOT(slotItemClicked(TQListViewItem*)));
    connect(this, TQT_SIGNAL(spacePressed(TQListViewItem*)), TQT_SLOT(slotItemClicked(TQListViewItem*)));

    new ItemViewTip( viewport(), this );
}

KMenuItemHeader *ItemView::insertHeader(int id, const TQString &relpath)
{
    KMenuItemHeader *newItem = new KMenuItemHeader(id, relpath, this );
    moveItemToIndex(newItem, 1);
    setBackPath( "kicker:/goup/" + relpath ); // the last wins for now

    return newItem;
}

KMenuItem* ItemView::findItem(int nId)
{
    for (TQListViewItemIterator it(this); it.current(); ++it)
    {
	if(static_cast<KMenuItem*>(it.current())->id() == nId)
	    return static_cast<KMenuItem*>(it.current());
    }

    return 0L;
}

bool ItemView::focusNextPrevChild(bool /*next*/)
{
    return false;
}

KMenuItem* ItemView::itemAtIndex(int nIndex)
{
    if(nIndex <= 0)
	return 0L;

    if(nIndex >= childCount())
      return static_cast<KMenuItem*>(lastItem());

    int i = 1;
    TQListViewItemIterator it(this);
    for (;it.current(); ++i, ++it) {
	if(i == nIndex)
	    return static_cast<KMenuItem*>(it.current());
    }

    return static_cast<KMenuItem*>(lastItem());
}

KMenuItem* ItemView::insertItem( const TQString& icon, const TQString& text, const TQString& description, const
                                 TQString& path, int nId, int nIndex, KMenuItem *parent)
{
    KMenuItem* newItem = findItem(nId);

    if(!newItem && parent)
        newItem = new KMenuItem(nId, parent );
    else if ( !newItem )
	newItem = new KMenuItem(nId, this );

    newItem->setIcon(icon, m_iconSize);
    newItem->setTitle(text);
    newItem->setDescription(description);
    newItem->setPath(path);

    if (nIndex==-1)
      nIndex=childCount();

    moveItemToIndex(newItem, nIndex);

    return newItem;
}

KMenuItem* ItemView::insertItem( const TQString& icon, const TQString& text, const TQString& description,
                                 int nId, int nIndex, KMenuItem *parent)
{
   return insertItem( icon, text, description, TQString(), nId, nIndex, parent);
}

int ItemView::setItemEnabled(int id, bool enabled)
{
    KMenuItem* item = findItem(id);

    if(item)
	item->setEnabled(enabled);

    return 0;
}

KMenuItemSeparator *ItemView::insertSeparator(int nId, const TQString& text, int nIndex)
{
    KMenuItemSeparator *newItem = new KMenuItemSeparator(nId, this);

    newItem->setText(0, text);

    if (nIndex==-1)
      nIndex=childCount();

    moveItemToIndex(newItem, nIndex);
    return newItem;
}

void ItemView::moveItemToIndex(KMenuItem* item, int nIndex)
{

    if (nIndex <= 0) {
          takeItem(item);
          KListView::insertItem(item);
    }
    else {
        item->moveItem(itemAtIndex(nIndex));
    }
}

void ItemView::slotMoveContent()
{
    if ( !m_spacer )
        return;

    int item_height = 0;
    TQListViewItemIterator it( this );
    while ( it.current() ) {
        if ( !dynamic_cast<KMenuSpacer*>( it.current() ) && !it.current()->parent() && it.current()->isVisible() )  {
            it.current()->invalidateHeight();
            item_height += it.current()->totalHeight();
        }
        ++it;
    }

    if ( height() > item_height )
        m_spacer->setHeight( height() - item_height );
    else
        m_spacer->setHeight( 0 );
}

KMenuItem *ItemView::insertMenuItem(KService::Ptr& s, int nId, int nIndex, KMenuItem* parentItem,
                                    const TQString& aliasname, const TQString & label, const TQString & categoryIcon )
{
    if (!s)
	return 0;

    TQString serviceName = aliasname.isEmpty() ? s->name() : aliasname;

    kdDebug() << "insertMenuItem " << nId << " " << nIndex << " " << s->name() << endl;
    KMenuItem* newItem = 0; //findItem(nId);
    if(!newItem)
	newItem = parentItem ? new KMenuItem(nId, parentItem) : new KMenuItem(nId, this);

    newItem->setIcon(s->icon()=="unknown" ? categoryIcon : s->icon(), m_iconSize);
    if ((KickerSettings::DescriptionAndName || KickerSettings::menuEntryFormat()
            == KickerSettings::DescriptionOnly) && !s->genericName().isEmpty()) {
      newItem->setTitle(s->genericName());
      newItem->setDescription(label.isEmpty() ? serviceName : label);
    }
    else {
      newItem->setTitle(label.isEmpty() ? serviceName : label);
      newItem->setDescription(s->genericName());
    }
    newItem->setService(s);

    if (nIndex==-2)
      return newItem;

    if (nIndex==-1)
      nIndex=childCount();

    moveItemToIndex(newItem, nIndex);

    return newItem;
}

KMenuItem* ItemView::insertDocumentItem(const TQString& s, int nId, int nIndex, const TQStringList* /*suppressGenericNames*/,
                                        const TQString& /*aliasname*/)
{
    KMenuItem* newItem = findItem(nId);

    if(!newItem)
	newItem = new KMenuItem(nId, this);

    KMimeType::Ptr mt = KMimeType::findByURL( s );
    newItem->setIcon(KMimeType::iconForURL( s ), m_iconSize);
    newItem->setTitle(s);
    newItem->setDescription(mt->comment());
    newItem->setPath(s);

    if (nIndex==-1)
      nIndex=childCount();

    moveItemToIndex(newItem, nIndex);

    return newItem;
}

KMenuItem* ItemView::insertRecentlyItem(const TQString& s, int nId, int nIndex)
{
    KDesktopFile f(s, true /* read only */);

    KMenuItem* newItem = findItem(nId);

    if(!newItem)
	newItem = new KMenuItem(nId, this);

    newItem->setIcon(f.readIcon(), m_iconSize);

    // work around upstream fixed bug
    TQString name=f.readName();
    if (name.isEmpty())
      name=f.readURL();

    newItem->setTitle(name);

    TQString comment = f.readComment();
    if (comment.isEmpty()) {
      KURL url(f.readURL());
      if (!url.host().isEmpty())
        comment = i18n("Host: %1").arg(url.host());
    }

    newItem->setDescription(comment);
    newItem->setPath(s);

    if (nIndex==-1)
      nIndex=childCount();

    moveItemToIndex(newItem, nIndex);

    return newItem;
}

int ItemView::insertItem(PopupMenuTitle*, int, int)
{
    return 0;
}

KMenuItem* ItemView::insertSubItem(const TQString& icon, const TQString& caption, const TQString& description, const TQString& path, KMenuItem* parentItem)
{
#warning FIXME
    KMenuItem* newItem = parentItem ? new KMenuItem(-1, parentItem) : new KMenuItem(-1, this);
    newItem->setTitle(caption);
    newItem->setDescription(description);
    newItem->setIcon(icon, m_iconSize);
    newItem->setPath(path);

    return newItem;
}



void ItemView::slotItemClicked(int button, TQListViewItem * item, const TQPoint & /*pos*/, int /*c*/ )
{
    if (button==1)
      slotItemClicked(item);
}

void ItemView::slotItemClicked(TQListViewItem* item)
{
    KMenuItem* kitem = dynamic_cast<KMenuItem*>(item);
    if ( !kitem )
        return;

    if(kitem->service()) {
        emit startService(kitem->service());
    }
    else if(!kitem->path().isEmpty()) {
        emit startURL(kitem->path());
    }
}

void ItemView::contentsMousePressEvent ( TQMouseEvent * e )
{
    KListView::contentsMousePressEvent( e );

    TQPoint vp = contentsToViewport(e->pos());
    KMenuItemSeparator *si = dynamic_cast<KMenuItemSeparator*>( itemAt( vp ) );
    if ( si )
    {
        if ( si->hitsLink( vp - itemRect(si).topLeft() ) )
            emit startURL( si->linkUrl() );
    }
}

void ItemView::contentsMouseMoveEvent(TQMouseEvent *e)
{
    TQPoint vp = contentsToViewport(e->pos());
    TQListViewItem * i = itemAt( vp );

    bool link_cursor = false;
    KMenuItemSeparator *si = dynamic_cast<KMenuItemSeparator*>( i );
    if ( si )
        link_cursor = si->hitsLink( vp - itemRect(si).topLeft() );

    if (i && !i->isSelectable() && !link_cursor) {
      unsetCursor();
      viewport()->unsetCursor();
      return;
    }

    KListView::contentsMouseMoveEvent(e);

    if (m_mouseMoveSelects) {
      if(i && i->isEnabled() && !i->isSelected() &&
         // FIXME: This is wrong if you drag over the items.
         (e->state() & (Qt::LeftButton|Qt::MidButton|Qt::RightButton)) == 0)
          KListView::setSelected(i, true);
      else if (!i && selectedItem())
          KListView::setSelected(selectedItem(), false);
    }

    if ( link_cursor )
        setCursor( Qt::PointingHandCursor );
    else
        unsetCursor();

}

void ItemView::leaveEvent(TQEvent* e)
{
    KListView::leaveEvent(e);

    clearSelection();
}

void ItemView::resizeEvent ( TQResizeEvent * e )
{
    KListView::resizeEvent( e );
//    if ( m_lastOne )
//        int diff = itemRect( m_lastOne ).bottom() - viewport()->height();
}

void ItemView::viewportPaintEvent ( TQPaintEvent * pe )
{
    //kdDebug() << "viewportPaintEvent " << pe->rect() << " " << contentsY () << " " << m_old_contentY << endl;
    KListView::viewportPaintEvent( pe );

    if ( m_lastOne && m_old_contentY != contentsY() ) {
        m_old_contentY = contentsY();
        m_lastOne->repaint();
    }
}

void ItemView::clear()
{
    KListView::clear();
    m_lastOne = 0;
    m_old_contentY = -1;
    m_back_url = TQString();
}

void ItemView::contentsWheelEvent(TQWheelEvent *e)
{
    KListView::contentsWheelEvent(e);

    TQPoint vp = contentsToViewport(e->pos());
    TQListViewItem * i = itemAt( vp );

    if(i && i->isEnabled() && !i->isSelected() &&
       // FIXME: This is wrong if you drag over the items.
       (e->state() & (Qt::LeftButton|Qt::MidButton|Qt::RightButton)) == 0)
        KListView::setSelected(i, true);
    else if (!i && selectedItem())
        KListView::setSelected(selectedItem(), false);
}

TQDragObject * ItemView::dragObject()
{
    KMultipleDrag* o = 0;
    TQListViewItem *item = itemAt( viewport()->mapFromGlobal(TQCursor::pos()) );
    if ( item ) {
      KMenuItem* kitem = static_cast<KMenuItem*>(item);

      if (dynamic_cast<KMenuItemHeader*>(item))
        return 0;

      o = new KMultipleDrag(viewport());
      TQPixmap pix = KGlobal::iconLoader()->loadIcon( kitem->icon(), KIcon::Panel, m_iconSize);
      TQPixmap add = KGlobal::iconLoader()->loadIcon( "add", KIcon::Small );

      TQPainter p( &pix );
      p.drawPixmap(pix.height()-add.height(), pix.width()-add.width(), add);
      p.end();

      TQBitmap mask;

      if (pix.mask())
          mask = *pix.mask();
      else {
	  mask.resize(pix.size());
	  mask.fill(Qt::color1);
      }

      bitBlt( &mask, pix.width()-add.width(), pix.height()-add.height(), add.mask(), 0, 0, add.width(), add.height(), OrROP );
      pix.setMask( mask );
      o->setPixmap(pix);

      if(kitem->service()) {
        // If the path to the desktop file is relative, try to get the full
        // path from KStdDirs.
        TQString path = kitem->service()->desktopEntryPath();
        path = locate("apps", path);
        o->addDragObject(new KURLDrag(KURL::List(KURL(path)), 0));
      }
      else if (kitem->path().startsWith("kicker:/new") || kitem->path().startsWith("system:/")
        || kitem->path().startsWith("kicker:/switchuser_") || kitem->path().startsWith("kicker:/restart_")) {
        delete o;
        return 0;
      }
      else if (kitem->hasChildren()) {
         o->addDragObject(new KURLDrag(KURL::List(KURL("programs:/"+kitem->menuPath())), 0));
         return o;
      }
      else if(!kitem->path().isEmpty() && !kitem->path().startsWith("kicker:/") && !kitem->path().startsWith("kaddressbook:/")) {
         TQString uri = kitem->path();

         if (uri.startsWith(locateLocal("data", TQString::fromLatin1("RecentDocuments/")))) {
             KDesktopFile df(uri,true);
             uri=df.readURL();
         }

         o->addDragObject(new KURLDrag(KURL::List(KURL(uri)), 0));
      }

      o->addDragObject(new KMenuItemDrag(*kitem,this));
    }
    return o;
}

int ItemView::goodHeight()
{
    int item_height = 0;
    TQListViewItemIterator it( this );
    while ( it.current() ) {
        if ( !dynamic_cast<KMenuSpacer*>( it.current() ) && !it.current()->parent() && it.current()->isVisible() )  {
            item_height += it.current()->height();
        }
        ++it;
    }

    return item_height;
}


KMenuItemDrag::KMenuItemDrag(KMenuItem& item, TQWidget *dragSource)
    : TQDragObject(dragSource, 0)
{
    TQBuffer buff(a);
    buff.open(IO_WriteOnly);
    TQDataStream s(&buff);

    s << item.id() << (item.service() ? item.service()->storageId() : TQString())
      << item.title() << item.description() << item.icon() << item.path();
}

KMenuItemDrag::~KMenuItemDrag()
{
}

const char * KMenuItemDrag::format(int i) const
{
    if (i == 0)
        return "application/kmenuitem";

    return 0;
}

TQByteArray KMenuItemDrag::encodedData(const char* mimeType) const
{
    if (TQString("application/kmenuitem") == mimeType)
        return a;

    return TQByteArray();
}

bool KMenuItemDrag::canDecode(const TQMimeSource * e)
{
    if (e->provides( "application/kmenuitem" ) )
        return true;

    return false;
}

bool ItemView::acceptDrag (TQDropEvent* event) const
{
    if ( !acceptDrops() )
        return false;

    if (KMenuItemDrag::canDecode(event))
        return true;

    if (TQTextDrag::canDecode(event)) {
        TQString text;
        TQTextDrag::decode(event,text);
        return !text.startsWith("programs:/");
    }

    return itemsMovable();
}

bool KMenuItemDrag::decode(const TQMimeSource* e, KMenuItemInfo& item)
{
    TQByteArray a = e->encodedData("application/kmenuitem");

    if (a.isEmpty()) {
        TQStringList l;
        bool ret = TQUriDrag::decodeToUnicodeUris( e, l );
        if ( ret )
        {
            for ( TQStringList::ConstIterator it = l.begin(); it != l.end(); ++it )
            {
                TQString url = *it;
                kdDebug () << "Url " << url << endl;
                item.m_path = KURL( url ).path();
                if ( KDesktopFile::isDesktopFile( item.m_path ) )
                {
                    KDesktopFile df( item.m_path, true );
                    item.m_description = df.readGenericName();
                    item.m_icon = df.readIcon();
                    item.m_title = df.readName();
                }
                else
                {
                    item.m_title = item.m_path;
                    item.m_icon = KMimeType::iconForURL( url );
                    item.m_title = item.m_path.section( '/', -1, -1 );
                    int last_slash = url.findRev ('/', -1);
                    if (last_slash == 0)
                        item.m_description = i18n("Directory: /)");
                    else
                        item.m_description = i18n("Directory: ") + url.section ('/', -2, -2);
                }

                return true;
            }
        }
        return false;
    }

    TQBuffer buff(a);
    buff.open(IO_ReadOnly);
    TQDataStream s(&buff);

    KMenuItemInfo i;
    TQString storageId;
    s >> i.m_id >> storageId >> i.m_title >> i.m_description >> i.m_icon >> i.m_path;

    i.m_s = storageId.isEmpty() ? 0 : KService::serviceByStorageId(storageId);
    item = i;

    return true;
}

FavoritesItemView::FavoritesItemView(TQWidget* parent, const char* name)
    : ItemView(parent, name)
{
}

bool FavoritesItemView::acceptDrag (TQDropEvent* event) const
{
    if (event->source()==this->viewport())
        return true;

    if (KMenuItemDrag::canDecode(event)) {
        KMenuItemInfo item;
        KMenuItemDrag::decode(event,item);
        TQStringList favs = KickerSettings::favorites();

        if (item.m_s)
            return favs.find(item.m_s->storageId())==favs.end();
        else {
            TQStringList::Iterator it;

            TQString uri = item.m_path;

            if (uri.startsWith(locateLocal("data", TQString::fromLatin1("RecentDocuments/")))) {
               KDesktopFile df(uri,true);
               uri=df.readURL();
            }

            for (it = favs.begin(); it != favs.end(); ++it) {
                if ((*it)[0]=='/') {
                    KDesktopFile df((*it),true);
                    if (df.readURL().replace("file://",TQString())==uri)
                        break;
                }
            }
            return it==favs.end();
        }
    }

    if (TQTextDrag::canDecode(event)) {
        TQString text;
        TQTextDrag::decode(event,text);
        TQStringList favs = KickerSettings::favorites();

        if (text.endsWith(".desktop")) {
            KService::Ptr p = KService::serviceByDesktopPath(text.replace("file://",TQString()));
            return (p && favs.find(p->storageId())==favs.end());
        }
        else {
            TQStringList::Iterator it;
            for (it = favs.begin(); it != favs.end(); ++it) {
                if ((*it)[0]=='/') {
                    KDesktopFile df((*it),true);
                    if (df.readURL().replace("file://",TQString())==text)
                        break;
                }
            }
            return it==favs.end();
        }
    }

    return itemsMovable();
}

#include "itemview.moc"

// vim:cindent:sw=4:
