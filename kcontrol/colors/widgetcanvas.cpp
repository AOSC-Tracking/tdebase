//
// A special widget which draws a sample of KDE widgets
// It is used to preview color schemes
//
// Copyright (c)  Mark Donohoe 1998
//

#include <tqdrawutil.h>
#include <tqpainter.h>
#include <tqscrollbar.h>
#include <tqbitmap.h>
#include <tqtooltip.h>
#include <tqstyle.h>
#include <tqpopupmenu.h>

#include <kcolordrag.h>
#include <kpixmapeffect.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <klocale.h>
#include <kpixmap.h>

#include "widgetcanvas.h"
#include "widgetcanvas.moc"
#include "stdclient_bitmaps.h"

static TQPixmap* close_pix = 0;
static TQPixmap* maximize_pix = 0;
static TQPixmap* minimize_pix = 0;
static TQPixmap* normalize_pix = 0;
static TQPixmap* pinup_pix = 0;
static TQPixmap* pindown_pix = 0;
static TQPixmap* menu_pix = 0;

static TQPixmap* dis_close_pix = 0;
static TQPixmap* dis_maximize_pix = 0;
static TQPixmap* dis_minimize_pix = 0;
static TQPixmap* dis_normalize_pix = 0;
static TQPixmap* dis_pinup_pix = 0;
static TQPixmap* dis_pindown_pix = 0;
static TQPixmap* dis_menu_pix = 0;


WidgetCanvas::WidgetCanvas( TQWidget *parent, const char *name )
	: TQWidget( parent, name  ), shadeSortColumn( true )
{
    setMouseTracking( true );
    setBackgroundMode( NoBackground );
    setAcceptDrops( true);
    setMinimumSize(200, 100);
    currentHotspot = -1;
}

void WidgetCanvas::addToolTip( int area, const TQString &tip )
{
    tips.insert(area, tip);
}

void WidgetCanvas::paintEvent(TQPaintEvent *)
{
    bitBlt( this, 0, 0, &smplw );
}

void WidgetCanvas::mousePressEvent( TQMouseEvent *me )
{
    for ( int i = 0; i < MAX_HOTSPOTS; i++ )
	if ( hotspots[i].rect.contains( me->pos() ) ) {
	    emit widgetSelected( hotspots[i].number );
	    return;
	}
}

void WidgetCanvas::mouseMoveEvent( TQMouseEvent *me )
{
    for ( int i = 0; i < MAX_HOTSPOTS; i++ )
	if ( hotspots[i].rect.contains( me->pos() ) ) {
	    if ( i != currentHotspot ) {
		TQString tip = tips[hotspots[i].number];
		TQToolTip::remove( this );
		TQToolTip::add( this, tip );
		currentHotspot = i;
	    }
	    return;
	}

    TQToolTip::remove( this );
}

void WidgetCanvas::dropEvent( TQDropEvent *e)
{
    TQColor c;
    if (KColorDrag::decode( e, c)) {
	for ( int i = 0; i < MAX_HOTSPOTS; i++ )
	    if ( hotspots[i].rect.contains( e->pos() ) ) {
		emit colorDropped( hotspots[i].number, c);
		return;
	    }
    }
}


void WidgetCanvas::dragEnterEvent( TQDragEnterEvent *e)
{
        e->accept( KColorDrag::canDecode( e));
}

void WidgetCanvas::paletteChange(const TQPalette &)
{
	drawSampleWidgets();
}

void WidgetCanvas::resizeEvent(TQResizeEvent *)
{
    drawSampleWidgets();
}

/*
 * This is necessary because otherwise the scrollbar in drawSampleWidgets()
 * doesn't show the first time.
 */
void WidgetCanvas::showEvent(TQShowEvent *)
{
    drawSampleWidgets();
}

void WidgetCanvas::resetTitlebarPixmaps(const TQColor &actMed,
                                        const TQColor &disMed)
{
    if(close_pix) delete close_pix;
    if(maximize_pix) delete maximize_pix;
    if(minimize_pix) delete minimize_pix;
    if(normalize_pix) delete normalize_pix;
    if(pinup_pix) delete pinup_pix;
    if(pindown_pix) delete pindown_pix;
    if(menu_pix) delete menu_pix;

    if(dis_close_pix) delete dis_close_pix;
    if(dis_maximize_pix) delete dis_maximize_pix;
    if(dis_minimize_pix) delete dis_minimize_pix;
    if(dis_normalize_pix) delete dis_normalize_pix;
    if(dis_pinup_pix) delete dis_pinup_pix;
    if(dis_pindown_pix) delete dis_pindown_pix;
    if(dis_menu_pix) delete dis_menu_pix;
    
    TQPainter pact, pdis;
    TQBitmap bitmap;
    TQColor actHigh = actMed.light(150);
    TQColor actLow = actMed.dark(120);
    TQColor disHigh = disMed.light(150);
    TQColor disLow = disMed.dark(120);
    
    close_pix = new TQPixmap(16, 16);
    dis_close_pix = new TQPixmap(16, 16);
    pact.begin(close_pix); pdis.begin(dis_close_pix);
    bitmap = TQBitmap(16, 16, close_white_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actHigh); pdis.setPen(disHigh);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, close_dgray_bits, true);
    pact.setPen(actLow); pdis.setPen(disLow);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    pact.end(); pdis.end();
    bitmap = TQBitmap(16, 16, close_mask_bits, true);
    close_pix->setMask(bitmap); dis_close_pix->setMask(bitmap);

    minimize_pix = new TQPixmap(16, 16);
    dis_minimize_pix = new TQPixmap(16, 16);
    pact.begin(minimize_pix); pdis.begin(dis_minimize_pix);
    bitmap = TQBitmap(16, 16, iconify_white_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actHigh); pdis.setPen(disHigh);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, iconify_dgray_bits, true);
    pact.setPen(actLow); pdis.setPen(disLow);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    pact.end(); pdis.end();
    bitmap = TQBitmap(16, 16, iconify_mask_bits, true);
    minimize_pix->setMask(bitmap); dis_minimize_pix->setMask(bitmap);
    
    maximize_pix = new TQPixmap(16, 16);
    dis_maximize_pix = new TQPixmap(16, 16);
    pact.begin(maximize_pix); pdis.begin(dis_maximize_pix);
    bitmap = TQBitmap(16, 16, maximize_white_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actHigh); pdis.setPen(disHigh);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, maximize_dgray_bits, true);
    pact.setPen(actLow); pdis.setPen(disLow);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    pact.end(); pdis.end();
    bitmap = TQBitmap(16, 16, maximize_mask_bits, true);
    maximize_pix->setMask(bitmap); dis_maximize_pix->setMask(bitmap);

    normalize_pix = new TQPixmap(16, 16);
    dis_normalize_pix = new TQPixmap(16, 16);
    pact.begin(normalize_pix); pdis.begin(dis_normalize_pix);
    bitmap = TQBitmap(16, 16, maximizedown_white_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actHigh); pdis.setPen(disHigh);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, maximizedown_dgray_bits, true);
    pact.setPen(actLow); pdis.setPen(disLow);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    pact.end(); pdis.end();
    bitmap = TQBitmap(16, 16, maximizedown_mask_bits, true);
    normalize_pix->setMask(bitmap); dis_normalize_pix->setMask(bitmap);

    menu_pix = new TQPixmap(16, 16);
    dis_menu_pix = new TQPixmap(16, 16);
    pact.begin(menu_pix); pdis.begin(dis_menu_pix);
    bitmap = TQBitmap(16, 16, menu_white_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actHigh); pdis.setPen(disHigh);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, menu_dgray_bits, true);
    pact.setPen(actLow); pdis.setPen(disLow);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    pact.end(); pdis.end();
    bitmap = TQBitmap(16, 16, menu_mask_bits, true);
    menu_pix->setMask(bitmap); dis_menu_pix->setMask(bitmap);

    pinup_pix = new TQPixmap(16, 16);
    dis_pinup_pix = new TQPixmap(16, 16);
    pact.begin(pinup_pix); pdis.begin(dis_pinup_pix);
    bitmap = TQBitmap(16, 16, pinup_white_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actHigh); pdis.setPen(disHigh);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, pinup_gray_bits, true);
    pact.setPen(actMed); pdis.setPen(disMed);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, pinup_dgray_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actLow); pdis.setPen(disLow);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    pact.end(); pdis.end();
    bitmap = TQBitmap(16, 16, pinup_mask_bits, true);
    pinup_pix->setMask(bitmap); dis_pinup_pix->setMask(bitmap);
    
    pindown_pix = new TQPixmap(16, 16);
    dis_pindown_pix = new TQPixmap(16, 16);
    pact.begin(pindown_pix); pdis.begin(dis_pindown_pix);
    bitmap = TQBitmap(16, 16, pindown_white_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actHigh); pdis.setPen(disHigh);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, pindown_gray_bits, true);
    pact.setPen(actMed); pdis.setPen(disMed);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    bitmap = TQBitmap(16, 16, pindown_dgray_bits, true);
    bitmap.setMask(bitmap);
    pact.setPen(actLow); pdis.setPen(disLow);
    pact.drawPixmap(0, 0, bitmap);
    pdis.drawPixmap(0, 0, bitmap);
    pact.end(); pdis.end();
    bitmap = TQBitmap(16, 16, pindown_mask_bits, true);
    pindown_pix->setMask(bitmap); dis_pindown_pix->setMask(bitmap);
    
}

void WidgetCanvas::drawSampleWidgets()
{
    int textLen, tmp;
    int highlightVal, lowlightVal;

    KConfig * c = new KConfig("kcmfonts");

    // Keep in sync with kglobalsettings.

    TQFont windowFontGuess(KGlobalSettings::generalFont().family(), 12, TQFont::SansSerif, true);
    windowFontGuess.setPixelSize(12);

    c->setGroup("WM");
    TQFont windowFont = c->readFontEntry("activeFont", &windowFontGuess);

    c->setGroup("General");
    TQFont defaultMenuFont = KGlobalSettings::menuFont();
    TQFont menuFont = c->readFontEntry("menuFont", &defaultMenuFont);

    delete c;
    c = 0;

    // Calculate the highlight and lowloght from contrast value and create
    // color group from color scheme.

    highlightVal=100+(2*contrast+4)*16/10;
    lowlightVal=100+(2*contrast+4)*10;

    TQColorGroup cg( txt, back,
                    back.light(highlightVal),
                    back.dark(lowlightVal),
                    back.dark(120),
                    txt, window );

    // We will need this brush.

    TQBrush brush(SolidPattern);
    brush.setColor( back );

    // Create a scrollbar and redirect drawing into a temp. pixmap to save a
    // lot of fiddly drawing later.

    TQScrollBar *vertScrollBar = new TQScrollBar( Qt::Vertical, this );
    // TODO: vertScrollBar->setStyle( new TQMotifStyle() );
    vertScrollBar->setGeometry( 400, 400, SCROLLBAR_SIZE, height());
    vertScrollBar->setRange( 0,  0 );
    vertScrollBar->setPalette( TQPalette(cg,cg,cg));
    vertScrollBar->show();

    TQPixmap pm( vertScrollBar->width(), vertScrollBar->height() );
    pm.fill( back );
#ifndef __osf__
    TQPainter::redirect( vertScrollBar, &pm );
#endif
    vertScrollBar->repaint();
    TQPainter::redirect( vertScrollBar, 0 );
    vertScrollBar->hide();

    // Reset the titlebar pixmaps
    resetTitlebarPixmaps(aTitleBtn, iTitleBtn);
	
    // Initialize the pixmap which we draw sample widgets into.

    smplw.resize(width(), height());
    //smplw.fill( parentWidget()->back() );
    smplw.fill( parentWidget()->colorGroup().mid() );

    // Actually start painting in

    TQPainter paint( &smplw );

    // Inactive window

    qDrawWinPanel ( &paint, 15, 5, width()-48, height(), cg, FALSE,
                    &brush);

    paint.setBrush( iaTitle );
    paint.setPen( iaTitle );
    //paint.drawRect( 20, 10, width()-60, 20 );

    KPixmap pmTitle;
    pmTitle.resize( width()-160, 20 );

    // Switched to vertical gradient because those kwin styles that
    // use the gradient have it vertical.
    KPixmapEffect::gradient(pmTitle, iaTitle, iaBlend,
                            KPixmapEffect::HorizontalGradient);
    paint.drawPixmap( 60, 10, pmTitle );


    paint.setFont( windowFont );
    paint.setPen( iaTxt );
    paint.drawText( 65, 25, i18n("Inactive window") );
    textLen = paint.fontMetrics().width(  i18n("Inactive window") );

    tmp = width()-100;
    paint.drawPixmap(22, 12, *dis_menu_pix);
    paint.drawPixmap(42, 12, *dis_pinup_pix);
    paint.drawPixmap(tmp+2, 12, *dis_minimize_pix);
    paint.drawPixmap(tmp+22, 12, *dis_maximize_pix);
    paint.drawPixmap(tmp+42, 12, *dis_close_pix);
    
    int spot = 0;
    hotspots[ spot++ ] =
        HotSpot( TQRect( 65, 25-14, textLen, 14 ), CSM_Inactive_title_text );

    hotspots[ spot++ ] =
        HotSpot( TQRect( 60, 10, (width()-160)/2, 20 ), CSM_Inactive_title_bar );

    hotspots[ spot++ ] =
        HotSpot( TQRect( 60+(width()-160)/2, 10,
                        (width()-160)/2, 20 ), CSM_Inactive_title_blend );

    hotspots[spot++] =
        HotSpot(TQRect(20, 12, 40, 20), CSM_Inactive_title_button); 
    hotspots[spot++] =
        HotSpot(TQRect(tmp, 12, 60, 20), CSM_Inactive_title_button);
    

    // Active window

    qDrawWinPanel ( &paint, 20, 25+5, width()-40, height(), cg, FALSE,
                    &brush);

    paint.setBrush( aTitle );paint.setPen( aTitle );
    paint.drawRect( 65, 30+5, width()-152, 20 );

    // Switched to vertical gradient because those kwin styles that
    // use the gradient have it vertical.
    pmTitle.resize( width()-152, 20 );
    KPixmapEffect::gradient(pmTitle, aTitle, aBlend,
                            KPixmapEffect::HorizontalGradient);
    paint.drawPixmap( 65, 35, pmTitle );

    paint.setFont( windowFont );
    paint.setPen( aTxt );
    paint.drawText( 75, 50,  i18n("Active window") );
    textLen = paint.fontMetrics().width(  i18n("Active window" ));

    tmp = width()-152+65;
    paint.drawPixmap(27, 35, *menu_pix);
    paint.drawPixmap(47, 35, *pinup_pix);
    paint.drawPixmap(tmp+2, 35, *minimize_pix);
    paint.drawPixmap(tmp+22, 35, *maximize_pix);
    paint.drawPixmap(tmp+42, 35, *close_pix);

    hotspots[ spot++ ] =
        HotSpot( TQRect( 75, 50-14, textLen, 14 ), CSM_Active_title_text);
    hotspots[ spot ++] =
        HotSpot( TQRect( 65, 35, (width()-152)/2, 20 ), CSM_Active_title_bar );
    hotspots[ spot ++] =
        HotSpot( TQRect( 65+(width()-152)/2, 35,
                        (width()-152)/2, 20 ), CSM_Active_title_blend );

    hotspots[spot++] =
        HotSpot(TQRect(25, 35, 40, 20), CSM_Active_title_button);
    hotspots[spot++] =
        HotSpot(TQRect(tmp, 35, 60, 20), CSM_Active_title_button);
    
    // Menu bar

    //qDrawShadePanel ( &paint, 25, 55, width()-52, 28, cg, FALSE, 2, &brush);
    kapp->tqstyle().tqdrawPrimitive(TQStyle::PE_PanelMenuBar, &paint, 
			TQRect(TQPoint(25, 55), TQSize(width()-52, 28)), cg);

    paint.setFont( menuFont );
    paint.setPen(txt );
	TQString file = i18n("File");
    textLen = paint.fontMetrics().width( file );
    //qDrawShadePanel ( &paint, 30, 59, textLen + 10, 21, cg, FALSE, 2, &brush);
	kapp->tqstyle().tqdrawPrimitive(TQStyle::PE_Panel, &paint,
			TQRect(30, 59, textLen + 10, 21), cg);
    paint.drawText( 35, 74, file );

    hotspots[ spot++ ] =
        HotSpot( TQRect( 35, 62, textLen, 14 ), CSM_Text );
    hotspots[ spot++ ] =
        HotSpot( TQRect( 27, 57, 33, 21 ), CSM_Background );

    paint.setFont( menuFont );
    paint.setPen( txt );
    paint.drawText( 35 + textLen + 20, 74, i18n("Edit") );
    textLen = paint.fontMetrics().width( i18n("Edit") );

    hotspots[ spot++ ] = HotSpot( TQRect( 35 + textLen + 20, 62, textLen, 14 ), CSM_Text );

    // Button Rects need to go before window

    // Frame and window contents

    brush.setColor( window );
    qDrawShadePanel ( &paint, 25, 80+5-4, width()-7-45-2,
                      height(), cg, TRUE, 2, &brush);

    // Standard text
    TQFont fnt = KGlobalSettings::generalFont();
    paint.setFont( fnt );
    paint.setPen( windowTxt );
    paint.drawText( 140, 127-20, i18n( "Standard text") );
    textLen = paint.fontMetrics().width( i18n("Standard text") );
    int column2 = 120 + textLen + 40 + 16;
 
    hotspots[ spot++ ] =
        HotSpot( TQRect( 140, 113-20, textLen, 14 ), CSM_Standard_text );

    // Selected text
    textLen = paint.fontMetrics().width( i18n("Selected text") );
    if (120 + textLen + 40 + 16 > column2)
       column2 = 120 + textLen + 40 + 16;

    paint.setBrush( select );paint.setPen( select );
    paint.drawRect ( 120, 115, textLen+40, 32);

    paint.setFont( fnt );
    paint.setPen( selectTxt );
    paint.drawText( 140, 135, i18n( "Selected text") );

    hotspots[ spot++ ] =
        HotSpot( TQRect( 140, 121, textLen, 14 ), CSM_Select_text );
    hotspots[ spot++ ] =
        HotSpot( TQRect( 120, 115, textLen+40, 32), CSM_Select_background ); // select bg

    // Link
    paint.setPen( link );
    paint.drawText( column2+18, 127-20, i18n( "link") );
    textLen = paint.fontMetrics().width( i18n("link") );
    paint.drawLine( column2+18, 109, column2+18+textLen, 109);

    hotspots[ spot++ ] =
        HotSpot( TQRect( column2+18, 113-20, textLen, 17 ), CSM_Link );

    int column3 = column2 + 25 + textLen;
    // Followed Link
    paint.setPen( visitedLink );
    paint.drawText( column3, 127-20, i18n( "followed link") );
    textLen = paint.fontMetrics().width( i18n("followed link") );
    paint.drawLine( column3, 109, column3+textLen, 109);

    hotspots[ spot++ ] =
        HotSpot( TQRect( column3, 113-20, textLen, 17 ), CSM_Followed_Link );

    // Button
    int xpos = column2;
    int ypos = 115 + 2;
    textLen = paint.fontMetrics().width(i18n("Push Button"));
    hotspots[ spot++ ] =
        HotSpot( TQRect(xpos+16, ypos+((28-paint.fontMetrics().height())/2),
                       textLen, paint.fontMetrics().height()), CSM_Button_text );
    hotspots[ spot++ ] =
        HotSpot( TQRect(xpos, ypos, textLen+32, 28), CSM_Button_background );
    //brush.setColor( button );
    TQColorGroup cg2(cg);
    cg2.setColor(TQColorGroup::Button, button);
    cg2.setColor(TQColorGroup::Background, window);
    //qDrawWinButton(&paint, xpos, ypos, textLen+32, 28, cg, false, &brush);
	kapp->tqstyle().tqdrawPrimitive(TQStyle::PE_ButtonCommand, &paint,
			TQRect(xpos, ypos, textLen+32, 28), cg2, TQStyle::Style_Enabled | TQStyle::Style_Raised);
    paint.setPen(buttonTxt);
    paint.drawText(xpos, ypos, textLen+32, 28, AlignCenter,
                   i18n("Push Button"));

    // Scrollbar
    paint.drawPixmap(width()-55+27-16-2,80+5-2,pm);

    // Menu
	
    brush.setColor( back );
	
	int textLenNew, textLenOpen, textLenSave;
	

    textLenNew = paint.fontMetrics().width( i18n("New") );

    hotspots[ spot++ ] =
        HotSpot( TQRect( 56, 83, textLenNew, 14 ), CSM_Text );

    paint.setFont( menuFont );
    textLenOpen = paint.fontMetrics().width( i18n("Menu item", "Open") );

    hotspots[ spot++ ] =
        HotSpot( TQRect( 56, 105, textLenOpen, 14 ), CSM_Text );

    paint.setFont( menuFont );
    textLenSave = paint.fontMetrics().width( i18n("Menu item", "Save") );

	TQPopupMenu *popup = new TQPopupMenu( this );
	popup->setFont( menuFont );
	popup->setPalette( TQPalette(cg,cg,cg));
	popup->insertItem(i18n("New"));
	popup->insertItem(i18n("Menu item", "Open"));
	int id = popup->insertItem(i18n("Menu item", "Save"));
	popup->setItemEnabled( id, false );

	// HACK: Force Layouting
	//Sad Eagle: sizeHint() forces layouting too, and it's a lot less visible
	//popup->sizeHint(); // Breaks with Qt 3.3
	popup->resize(popup->sizeHint());

	pm = TQPixmap::grabWidget( popup );
	delete popup;
	bitBlt(&smplw, 30, 80, &pm, 0, 0, pm.width(), pm.height());

    hotspots[ spot++ ] =
        HotSpot( TQRect( 28, 78, 88, 77 ), CSM_Background );

    hotspots[ spot++ ] =
        HotSpot( TQRect(25, 80+5-4, width()-7-45-2-16, height()), CSM_Standard_background );


    // Valance

    qDrawWinPanel ( &paint, 0, 0, width(), height(),
                    parentWidget()->colorGroup(), TRUE, 0);

    // Stop the painting

    hotspots[ spot++ ] =
        HotSpot( TQRect( 0, 0, width(), height() ), CSM_Background ); // ?

    repaint( FALSE );
}
