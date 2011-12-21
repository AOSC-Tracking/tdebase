/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

/*! \class TEWidget

    \brief Visible screen contents

   This class is responsible to map the `image' of a terminal emulation to the
   display. All the dependency of the emulation to a specific GUI or toolkit is
   localized here. Further, this widget has no knowledge about being part of an
   emulation, it simply work within the terminal emulation framework by exposing
   size and key events and by being ordered to show a new image.

   <ul>
   <li> The internal image has the size of the widget (evtl. rounded up)
   <li> The external image used in setImage can have any size.
   <li> (internally) the external image is simply copied to the internal
        when a setImage happens. During a resizeEvent no painting is done
        a paintEvent is expected to follow anyway.
   </ul>

   \sa TEScreen \sa Emulation
*/

/* FIXME:
   - 'image' may also be used uninitialized (it isn't in fact) in resizeEvent
   - 'font_a' not used in mouse events
   - add destructor
*/

/* TODO
   - evtl. be sensitive to `paletteChange' while using default colors.
   - set different 'rounding' styles? I.e. have a mode to show clipped
     chars?
*/

#include "config.h"
#include "TEWidget.h"
#include "konsole_wcwidth.h"

#include <tqapplication.h>
#include <tqpainter.h>
#include <tqclipboard.h>
#include <tqstyle.h>
#include <tqfile.h>
#include <tqdragobject.h>
#include <tqlayout.h>
#include <tqregexp.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

#include <assert.h>

#include <krun.h>
#include <kcursor.h>
#include <kdebug.h>
#include <klocale.h>
#include <knotifyclient.h>
#include <kglobalsettings.h>
#include <kshortcut.h>
#include <kurldrag.h>
#include <kio/netaccess.h>
#include <tqlabel.h>
#include <tqtimer.h>

#ifndef loc
#define loc(X,Y) ((Y)*columns+(X))
#endif

#define SCRWIDTH 16 // width of the scrollbar

#define yMouseScroll 1

#define REPCHAR   "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                  "abcdefgjijklmnopqrstuvwxyz" \
                  "0123456789./+@"

extern bool argb_visual; // declared in main.cpp and konsole_part.cpp

bool draw_translucent_background_colors = false;	// FIXME move to a GUI configuration setting

// scroll increment used when dragging selection at top/bottom of window.

// static
bool TEWidget::s_antialias = true;
bool TEWidget::s_standalone = false;

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                Colors                                     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

//FIXME: the default color table is in session.C now.
//       We need a way to get rid of this one, here.
static const ColorEntry base_color_table[TABLE_COLORS] =
// The following are almost IBM standard color codes, with some slight
// gamma correction for the dim colors to compensate for bright X screens.
// It contains the 8 ansiterm/xterm colors in 2 intensities.
{
  // Fixme: could add faint colors here, also.
  // normal
  ColorEntry(TQColor(0x00,0x00,0x00), 0, 0 ), ColorEntry( TQColor(0xB2,0xB2,0xB2), 1, 0 ), // Dfore, Dback
  ColorEntry(TQColor(0x00,0x00,0x00), 0, 0 ), ColorEntry( TQColor(0xB2,0x18,0x18), 0, 0 ), // Black, Red
  ColorEntry(TQColor(0x18,0xB2,0x18), 0, 0 ), ColorEntry( TQColor(0xB2,0x68,0x18), 0, 0 ), // Green, Yellow
  ColorEntry(TQColor(0x18,0x18,0xB2), 0, 0 ), ColorEntry( TQColor(0xB2,0x18,0xB2), 0, 0 ), // Blue,  Magenta
  ColorEntry(TQColor(0x18,0xB2,0xB2), 0, 0 ), ColorEntry( TQColor(0xB2,0xB2,0xB2), 0, 0 ), // Cyan,  White
  // intensiv
  ColorEntry(TQColor(0x00,0x00,0x00), 0, 1 ), ColorEntry( TQColor(0xFF,0xFF,0xFF), 1, 0 ),
  ColorEntry(TQColor(0x68,0x68,0x68), 0, 0 ), ColorEntry( TQColor(0xFF,0x54,0x54), 0, 0 ),
  ColorEntry(TQColor(0x54,0xFF,0x54), 0, 0 ), ColorEntry( TQColor(0xFF,0xFF,0x54), 0, 0 ),
  ColorEntry(TQColor(0x54,0x54,0xFF), 0, 0 ), ColorEntry( TQColor(0xFF,0x54,0xFF), 0, 0 ),
  ColorEntry(TQColor(0x54,0xFF,0xFF), 0, 0 ), ColorEntry( TQColor(0xFF,0xFF,0xFF), 0, 0 )
};

/* Note that we use ANSI color order (bgr), while IBMPC color order is (rgb)

   Code        0       1       2       3       4       5       6       7
   ----------- ------- ------- ------- ------- ------- ------- ------- -------
   ANSI  (bgr) Black   Red     Green   Yellow  Blue    Magenta Cyan    White
   IBMPC (rgb) Black   Blue    Green   Cyan    Red     Magenta Yellow  White
*/

void TEWidget::setDefaultBackColor(const TQColor& color)
{
  defaultBgColor = color;
  if (tqAlpha(blend_color) != 0xff && !backgroundPixmap())
    setBackgroundColor(getDefaultBackColor());
}

TQColor TEWidget::getDefaultBackColor()
{
  if (defaultBgColor.isValid())
    return defaultBgColor;
  return color_table[DEFAULT_BACK_COLOR].color;
}

const ColorEntry* TEWidget::getColorTable() const
{
  return color_table;
}

void TEWidget::setColorTable(const ColorEntry table[])
{
  for (int i = 0; i < TABLE_COLORS; i++) color_table[i] = table[i];
  const TQPixmap* pm = backgroundPixmap();
  if (!pm)
    if (!argb_visual || (tqAlpha(blend_color) == 0xff))
      setBackgroundColor(getDefaultBackColor());
    else {
      float alpha = tqAlpha(blend_color) / 255.;
      int pixel = tqAlpha(blend_color) << 24 |
                  int(tqRed(blend_color) * alpha) << 16 |
                  int(tqGreen(blend_color) * alpha) << 8  |
                  int(tqBlue(blend_color) * alpha);
      setBackgroundColor(TQColor(blend_color, pixel));
    }
  update();
}

//FIXME: add backgroundPixmapChanged.

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                   Font                                    */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*
   The VT100 has 32 special graphical characters. The usual vt100 extended
   xterm fonts have these at 0x00..0x1f.

   QT's iso mapping leaves 0x00..0x7f without any changes. But the graphicals
   come in here as proper unicode characters.

   We treat non-iso10646 fonts as VT100 extended and do the requiered mapping
   from unicode to 0x00..0x1f. The remaining translation is then left to the
   QCodec.
*/

static inline bool isLineChar(TQ_UINT16 c) { return ((c & 0xFF80) == 0x2500);}

// assert for i in [0..31] : vt100extended(vt100_graphics[i]) == i.

unsigned short vt100_graphics[32] =
{ // 0/8     1/9    2/10    3/11    4/12    5/13    6/14    7/15
  0x0020, 0x25C6, 0x2592, 0x2409, 0x240c, 0x240d, 0x240a, 0x00b0,
  0x00b1, 0x2424, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c,
  0xF800, 0xF801, 0x2500, 0xF803, 0xF804, 0x251c, 0x2524, 0x2534,
  0x252c, 0x2502, 0x2264, 0x2265, 0x03C0, 0x2260, 0x00A3, 0x00b7
};

/*
static TQChar vt100extended(TQChar c)
{
  switch (c.unicode())
  {
    case 0x25c6 : return  1;
    case 0x2592 : return  2;
    case 0x2409 : return  3;
    case 0x240c : return  4;
    case 0x240d : return  5;
    case 0x240a : return  6;
    case 0x00b0 : return  7;
    case 0x00b1 : return  8;
    case 0x2424 : return  9;
    case 0x240b : return 10;
    case 0x2518 : return 11;
    case 0x2510 : return 12;
    case 0x250c : return 13;
    case 0x2514 : return 14;
    case 0x253c : return 15;
    case 0xf800 : return 16;
    case 0xf801 : return 17;
    case 0x2500 : return 18;
    case 0xf803 : return 19;
    case 0xf804 : return 20;
    case 0x251c : return 21;
    case 0x2524 : return 22;
    case 0x2534 : return 23;
    case 0x252c : return 24;
    case 0x2502 : return 25;
    case 0x2264 : return 26;
    case 0x2265 : return 27;
    case 0x03c0 : return 28;
    case 0x2260 : return 29;
    case 0x00a3 : return 30;
    case 0x00b7 : return 31;
  }
  return c;
}

static TQChar identicalMap(TQChar c)
{
  return c;
}
*/

void TEWidget::fontChange(const TQFont &)
{
  TQFontMetrics fm(font());
  font_h = fm.height() + m_lineSpacing;

  // waba TEWidget 1.123:
  // "Base character width on widest ASCII character. This prevents too wide
  //  characters in the presence of double wide (e.g. Japanese) characters."
  // Get the width from representative normal width characters
  font_w = tqRound((double)fm.width(REPCHAR)/(double)strlen(REPCHAR));

  fixed_font = true;
  int fw = fm.width(REPCHAR[0]);
  for(unsigned int i=1; i< strlen(REPCHAR); i++){
    if (fw != fm.width(REPCHAR[i])){
      fixed_font = false;
      break;
  }
  }

  if (font_w>200) // don't trust unrealistic value, fallback to TQFontMetrics::maxWidth()
    font_w=fm.maxWidth();
  if (font_w<1)
    font_w=1;

  font_a = fm.ascent();
//printf("font: %s\n", font().toString().latin1());
//printf("fixed: %s\n", font().fixedPitch() ? "yes" : "no");
//printf("fixed_font: %d\n", fixed_font);
//printf("font_h: %d\n",font_h);
//printf("font_w: %d\n",font_w);
//printf("fw: %d\n",fw);
//printf("font_a: %d\n",font_a);
//printf("rawname: %s\n",font().rawName().ascii());

/*
#if defined(Q_CC_GNU)
#warning TODO: Review/fix vt100 extended font-mapping
#endif
*/

//  fontMap = identicalMap;
  emit changedFontMetricSignal( font_h, font_w );
  propagateSize();
  update();
}

void TEWidget::setVTFont(const TQFont& f)
{
  TQFont font = f;

  // the font must be small enough to allow at least one line and one character of text to fit 
  // on screen
  TQFontMetrics metrics(f);
  if ( metrics.height() < height() && metrics.maxWidth() < width() )
  {
    if (!s_antialias)
        font.setStyleStrategy( TQFont::NoAntialias );
    TQFrame::setFont(font);
    fontChange(font);
  }
}

void TEWidget::setFont(const TQFont &)
{
  // ignore font change request if not coming from konsole itself
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                         Constructor / Destructor                          */
/*                                                                           */
/* ------------------------------------------------------------------------- */

TEWidget::TEWidget(TQWidget *parent, const char *name)
:TQFrame(parent,name,TQt::WNoAutoErase)
,font_h(1)
,font_w(1)
,font_a(1)
,lines(1)
,columns(1)
,contentHeight(1)
,contentWidth(1)
,image(0)
,resizing(false)
,terminalSizeHint(false)
,terminalSizeStartup(true)
,bidiEnabled(false)
,actSel(0)
,word_selection_mode(false)
,line_selection_mode(false)
,preserve_line_breaks(true)
,column_selection_mode(false)
,scrollLoc(SCRNONE)
,word_characters(":@-./_~")
,m_bellMode(BELLSYSTEM)
,blinking(false)
,cursorBlinking(false)
,hasBlinkingCursor(false)
,ctrldrag(false)
,cuttobeginningofline(false)
,isBlinkEvent(false)
,isPrinting(false)
,printerFriendly(false)
,printerBold(false)
,isFixedSize(false)
,m_drop(0)
,possibleTripleClick(false)
,mResizeWidget(0)
,mResizeLabel(0)
,mResizeTimer(0)
,m_lineSpacing(0)
,colorsSwapped(false)
,rimX(1)
,rimY(1)
,m_imPreeditText(TQString::null)
,m_imPreeditLength(0)
,m_imStart(0)
,m_imStartLine(0)
,m_imEnd(0)
,m_imSelStart(0)
,m_imSelEnd(0)
,m_cursorLine(0)
,m_cursorCol(0)
,m_isIMEdit(false)
,m_isIMSel(false)
,blend_color(tqRgba(0,0,0,0xff))
{
  // The offsets are not yet calculated.
  // Do not calculate these too often to be more smoothly when resizing
  // konsole in opaque mode.
  bY = bX = 1;

  cb = TQApplication::clipboard();
  TQObject::connect( (TQObject*)cb, TQT_SIGNAL(selectionChanged()),
                    this, TQT_SLOT(onClearSelection()) );

  scrollbar = new TQScrollBar(this);
  scrollbar->setCursor( tqarrowCursor );
  connect(scrollbar, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(scrollChanged(int)));

  blinkT   = new TQTimer(this);
  connect(blinkT, TQT_SIGNAL(timeout()), this, TQT_SLOT(blinkEvent()));
  blinkCursorT   = new TQTimer(this);
  connect(blinkCursorT, TQT_SIGNAL(timeout()), this, TQT_SLOT(blinkCursorEvent()));

  setMouseMarks(true);
  setColorTable(base_color_table); // init color table

  tqApp->installEventFilter( this ); //FIXME: see below
  KCursor::setAutoHideCursor( this, true );

  // Init DnD ////////////////////////////////////////////////////////////////
  setAcceptDrops(true); // attempt
  dragInfo.state = diNone;

  setFocusPolicy( TQ_WheelFocus );

  // im
  setInputMethodEnabled(true);

  if (!argb_visual)
  {
    // Looks better at startup with KRootPixmap based pseudo-transparancy
    setBackgroundMode(NoBackground);
  }
}

//FIXME: make proper destructor
// Here's a start (David)
TEWidget::~TEWidget()
{
  tqApp->removeEventFilter( this );
  if (image) free(image);
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                             Display Operations                            */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/**
 A table for emulating the simple (single width) unicode drawing chars.
 It represents the 250x - 257x glyphs. If it's zero, we can't use it.
 if it's not, it's encoded as follows: imagine a 5x5 grid where the points are numbered
 0 to 24 left to top, top to bottom. Each point is represented by the corresponding bit.

 Then, the pixels basically have the following interpretation:
 _|||_
 -...-
 -...-
 -...-
 _|||_

where _ = none
      | = vertical line.
      - = horizontal line.
 */


enum LineEncode
{
    TopL  = (1<<1),
    TopC  = (1<<2),
    TopR  = (1<<3),

    LeftT = (1<<5),
    Int11 = (1<<6),
    Int12 = (1<<7),
    Int13 = (1<<8),
    RightT = (1<<9),

    LeftC = (1<<10),
    Int21 = (1<<11),
    Int22 = (1<<12),
    Int23 = (1<<13),
    RightC = (1<<14),

    LeftB = (1<<15),
    Int31 = (1<<16),
    Int32 = (1<<17),
    Int33 = (1<<18),
    RightB = (1<<19),

    BotL  = (1<<21),
    BotC  = (1<<22),
    BotR  = (1<<23)
};

#include "linefont.h"

static void drawLineChar(TQPainter& paint, int x, int y, int w, int h, uchar code)
{
    //Calculate cell midpoints, end points.
    int cx = x + w/2;
    int cy = y + h/2;
    int ex = x + w - 1;
    int ey = y + h - 1;

    TQ_UINT32 toDraw = LineChars[code];

    //Top lines:
    if (toDraw & TopL)
        paint.drawLine(cx-1, y, cx-1, cy-2);
    if (toDraw & TopC)
        paint.drawLine(cx, y, cx, cy-2);
    if (toDraw & TopR)
        paint.drawLine(cx+1, y, cx+1, cy-2);

    //Bot lines:
    if (toDraw & BotL)
        paint.drawLine(cx-1, cy+2, cx-1, ey);
    if (toDraw & BotC)
        paint.drawLine(cx, cy+2, cx, ey);
    if (toDraw & BotR)
        paint.drawLine(cx+1, cy+2, cx+1, ey);

    //Left lines:
    if (toDraw & LeftT)
        paint.drawLine(x, cy-1, cx-2, cy-1);
    if (toDraw & LeftC)
        paint.drawLine(x, cy, cx-2, cy);
    if (toDraw & LeftB)
        paint.drawLine(x, cy+1, cx-2, cy+1);

    //Right lines:
    if (toDraw & RightT)
        paint.drawLine(cx+2, cy-1, ex, cy-1);
    if (toDraw & RightC)
        paint.drawLine(cx+2, cy, ex, cy);
    if (toDraw & RightB)
        paint.drawLine(cx+2, cy+1, ex, cy+1);

    //Intersection points.
    if (toDraw & Int11)
        paint.drawPoint(cx-1, cy-1);
    if (toDraw & Int12)
        paint.drawPoint(cx, cy-1);
    if (toDraw & Int13)
        paint.drawPoint(cx+1, cy-1);

    if (toDraw & Int21)
        paint.drawPoint(cx-1, cy);
    if (toDraw & Int22)
        paint.drawPoint(cx, cy);
    if (toDraw & Int23)
        paint.drawPoint(cx+1, cy);

    if (toDraw & Int31)
        paint.drawPoint(cx-1, cy+1);
    if (toDraw & Int32)
        paint.drawPoint(cx, cy+1);
    if (toDraw & Int33)
        paint.drawPoint(cx+1, cy+1);

}

void TEWidget::drawTextFixed(TQPainter &paint, int x, int y,
                             TQString& str, const ca *attr)
{
  TQString drawstr;
  unsigned int nc=0;
  int w;
  for(unsigned int i=0;i<str.length();i++)
  {
    drawstr = str.at(i);
    // Add double of the width if next c is 0;
    if ((attr+nc+1)->c) // This may access image[image_size] See makeImage()
    {
      w = font_w;
      nc++;
    }
    else
    {
      w = font_w*2;
      nc+=2;
    }

    //Check for line-drawing char
    if (isLineChar(drawstr[0].unicode()))
    {
        uchar code = drawstr[0].cell();
        if (LineChars[code])
        {
            drawLineChar(paint, x, y, w, font_h, code);
            x += w;
            continue;
        }
    }

    paint.drawText(x,y, w, font_h, Qt::AlignHCenter | TQt::DontClip, drawstr, -1);
    x += w;
  }
}


/*!
    attributed string draw primitive
*/

void TEWidget::drawAttrStr(TQPainter &paint, TQRect rect,
                           TQString& str, const ca *attr, bool pm, bool clear)
{
  int a = font_a + m_lineSpacing / 2;
  TQColor fColor = printerFriendly ? Qt::black : attr->f.color(color_table);
  TQColor bColor = attr->b.color(color_table);
  TQString drawstr;

  if ((attr->r & RE_CURSOR) && !isPrinting)
    cursorRect = rect;

  // Paint background
  if (!printerFriendly)
  {
    if (attr->isTransparent(color_table))
    {
      if (pm)
        paint.setBackgroundMode( Qt::TransparentMode );
      if (clear || (blinking && (attr->r & RE_BLINK))) {
#ifdef USE_QT4
        paint.eraseRect(rect);
#else // USE_QT4
        erase(rect);
#endif // USE_QT4
      }
    }
    else
    {
      if (pm || clear || (blinking && (attr->r & RE_BLINK)) ||
          attr->b == cacol(CO_DFT, colorsSwapped ? DEFAULT_FORE_COLOR : DEFAULT_BACK_COLOR) )

        // draw background colors with 75% opacity
        if ( draw_translucent_background_colors && argb_visual && tqAlpha(blend_color) < 0xff ) {
          QRgb col = bColor.rgb();

          TQ_UINT8 salpha = 192;
          TQ_UINT8 dalpha = 255 - salpha;

          int a, r, g, b;
          a = QMIN( (tqAlpha (col) * salpha) / 255 + (tqAlpha (blend_color) * dalpha) / 255, 255 );
          r = QMIN( (tqRed   (col) * salpha) / 255 + (tqRed   (blend_color) * dalpha) / 255, 255 );
          g = QMIN( (tqGreen (col) * salpha) / 255 + (tqGreen (blend_color) * dalpha) / 255, 255 );
          b = QMIN( (tqBlue  (col) * salpha) / 255 + (tqBlue  (blend_color) * dalpha) / 255, 255 );

          col = a << 24 | r << 16 | g << 8 | b;
          int pixel = a << 24 | (r * a / 255) << 16 | (g * a / 255) << 8 | (b * a / 255);

          paint.fillRect(rect, TQColor(col, pixel));
        } else
          paint.fillRect(rect, bColor);
    }

    TQString tmpStr = str.simplifyWhiteSpace();
    if ( m_isIMEdit && !tmpStr.isEmpty() ) { // imput method edit area background color
      TQRect tmpRect = rect;
      if ( str != m_imPreeditText ) {  // ugly hack
        tmpRect.setLeft( tmpRect.left() + font_w );
        tmpRect.setWidth( tmpRect.width() + font_w );
      }

      paint.fillRect( tmpRect, Qt::darkCyan );  // currently use hard code color
    }

    if ( m_isIMSel && !tmpStr.isEmpty() ) { // imput method selection background color
      int x = rect.left() + ( font_w * (m_imSelStart - m_imStart) );
      int y = rect.top();
      int w = font_w * (m_imSelEnd - m_imSelStart);
      int h = font_h;

      TQRect tmpRect = TQRect( x, y, w, h );
      if ( str != m_imPreeditText ) {  // ugly hack
        tmpRect.setLeft( tmpRect.left() + font_w );
        tmpRect.setWidth( tmpRect.width() + font_w );
      }

      paint.fillRect( tmpRect, Qt::darkGray );   // currently use hard code color
    }
  }

  // Paint cursor
  if ((attr->r & RE_CURSOR) && !isPrinting) {
    paint.setBackgroundMode( Qt::TransparentMode );
    int h = font_h - m_lineSpacing;
    TQRect r(rect.x(),rect.y()+m_lineSpacing/2,rect.width(),h);
    if (hasFocus())
    {
       if (!cursorBlinking)
       {
          paint.fillRect(r, fColor);
          fColor = bColor;
       }
    }
    else
    {
       paint.setPen(fColor);
       paint.drawRect(r);
    }
  }

  if (!(blinking && (attr->r & RE_BLINK)))
  {
    // ### Disabled for now, since it causes problems with characters
    // that use the full width and/or height of the character cells.
    //bool shadow = ( !isPrinting && tqAlpha(blend_color) < 0xff
    //		    && tqGray( fColor.rgb() ) > 64 );
    bool shadow = false;
    paint.setPen(fColor);
    int x = rect.x();
    
    if (attr->isBold(color_table) && printerBold)
    {
      // When printing we use a bold font for bold
      paint.save();
      TQFont f = font();
      f.setBold(true);
      paint.setFont(f);
    }

    if(!fixed_font)
    {
      // The meaning of y differs between different versions of TQPainter::drawText!!
      int y = rect.y(); // top of rect

      if ( shadow ) {
        paint.setPen( Qt::black );
        drawTextFixed(paint, x+1, y+1, str, attr);
        paint.setPen(fColor);
      }

      drawTextFixed(paint, x, y, str, attr);
    }
    else
    {
      // The meaning of y differs between different versions of TQPainter::drawText!!
      int y = rect.y()+a; // baseline

      if ( shadow ) {
        paint.setPen( Qt::black );
        paint.drawText(x+1,y+1, str, -1, bidiEnabled ? TQPainter::Auto : TQPainter::LTR );
        paint.setPen(fColor);
      }

      paint.drawText(x,y, str, -1, bidiEnabled ? TQPainter::Auto : TQPainter::LTR );
    }

    if (attr->isBold(color_table) && isPrinting)
    {
      // When printing we use a bold font for bold
      paint.restore();
    }

    if ( attr->isBold(color_table) && !printerBold)
    {
      paint.setClipRect(rect);
      // On screen we use overstrike for bold
      paint.setBackgroundMode( Qt::TransparentMode );
      int x = rect.x()+1;
      if(!fixed_font)
      {
        // The meaning of y differs between different versions of TQPainter::drawText!!
        int y = rect.y(); // top of rect
        drawTextFixed(paint, x, y, str, attr);
      }
      else
      {
        // The meaning of y differs between different versions of TQPainter::drawText!!
        int y = rect.y()+a; // baseline
        if (bidiEnabled)
          paint.drawText(x,y, str, -1);
        else
          paint.drawText(x,y, str, -1, TQPainter::LTR);
      }
      paint.setClipping(false);
    }
    if (attr->r & RE_UNDERLINE)
      paint.drawLine(rect.left(), rect.y()+a+1,
                     rect.right(),rect.y()+a+1 );
  }
}

/*!
    Set XIM Position
*/
void TEWidget::setCursorPos(const int curx, const int cury)
{
    TQPoint tL  = contentsRect().topLeft();
    int    tLx = tL.x();
    int    tLy = tL.y();

    int xpos, ypos;
    ypos = bY + tLy + font_h*(cury-1) + font_a;
    xpos = bX + tLx + font_w*curx;
    /* The hasFocus() check is to avoid crashes in QXIMInputContext on some systems.
    See http://lists.kde.org/?l=kde-core-devel&m=115770546313922&w=2 . */
    if (hasFocus())
      setMicroFocusHint(xpos, ypos, 0, font_h);
    // fprintf(stderr, "x/y = %d/%d\txpos/ypos = %d/%d\n", curx, cury, xpos, ypos);
    m_cursorLine = cury;
    m_cursorCol = curx;
}

/*!
    The image can only be set completely.

    The size of the new image may or may not match the size of the widget.
*/

void TEWidget::setImage(const ca* const newimg, int lines, int columns)
{
  if (!image)
     updateImageSize(); // Create image

  int y,x,len;
  const TQPixmap* pm = backgroundPixmap();
  TQPainter paint;
  setUpdatesEnabled(false);
  paint.begin( this );

  TQPoint tL  = contentsRect().topLeft();
  int    tLx = tL.x();
  int    tLy = tL.y();
  hasBlinker = false;

  cacol cf;       // undefined
  cacol cb;       // undefined
  int   cr  = -1; // undefined

  int lins = TQMIN(this->lines,  TQMAX(0,lines  ));
  int cols = TQMIN(this->columns,TQMAX(0,columns));
  TQChar *disstrU = new TQChar[cols];
  char *dirtyMask = (char *) malloc(cols+2);

//{ static int cnt = 0; printf("setImage %d\n",cnt++); }
  for (y = 0; y < lins; y++)
  {
    const ca*       lcl = &image[y*this->columns];
    const ca* const ext = &newimg[y*columns];

    // The dirty mask indicates which characters need repainting. We also
    // mark surrounding neighbours dirty, in case the character exceeds
    // its cell boundaries
    memset(dirtyMask, 0, cols+2);
    // Two extra so that we don't have to have to care about start and end conditions
    for (x = 0; x < cols; x++)
    {
	if ( ( (m_imPreeditLength > 0) && ( ( m_imStartLine == y )
	      && ( ( m_imStart < m_imEnd ) && ( ( x > m_imStart ) ) && ( x < m_imEnd ) )
              || ( ( m_imSelStart < m_imSelEnd ) && ( ( x > m_imSelStart ) ) ) ) )
            || ext[x] != lcl[x])
      {
         dirtyMask[x] = dirtyMask[x+1] = dirtyMask[x+2] = 1;
      }
    }
    dirtyMask++; // Position correctly

    if (!resizing) // not while resizing, we're expecting a paintEvent
    for (x = 0; x < cols; x++)
    {
      hasBlinker |= (ext[x].r & RE_BLINK);
      // Start drawing if this character or the next one differs.
      // We also take the next one into account to handle the situation
      // where characters exceed their cell width.
      if (dirtyMask[x])
      {
        TQ_UINT16 c = ext[x+0].c;
        if ( !c )
            continue;
        int p = 0;
        disstrU[p++] = c; //fontMap(c);
        bool lineDraw = isLineChar(c);
        bool doubleWidth = (ext[x+1].c == 0);
        cr = ext[x].r;
        cb = ext[x].b;
        if (ext[x].f != cf) cf = ext[x].f;
        int lln = cols - x;
        for (len = 1; len < lln; len++)
        {
          c = ext[x+len].c;
          if (!c)
            continue; // Skip trailing part of multi-col chars.

          if (ext[x+len].f != cf || ext[x+len].b != cb || ext[x+len].r != cr ||
              !dirtyMask[x+len] || isLineChar(c) != lineDraw || (ext[x+len+1].c == 0) != doubleWidth)
            break;

          disstrU[p++] = c; //fontMap(c);
        }

        TQString unistr(disstrU, p);

        // for XIM on the spot input style
        m_isIMEdit = m_isIMSel = false;
        if ( m_imStartLine == y ) {
          if ( ( m_imStart < m_imEnd ) && ( x >= m_imStart-1 ) && ( x + int( unistr.length() ) <= m_imEnd ) )
            m_isIMEdit = true;

          if ( ( m_imSelStart < m_imSelEnd ) && ( x >= m_imStart-1 ) && ( x + int( unistr.length() ) <= m_imEnd ) )
            m_isIMSel = true;
	}
        else if ( m_imStartLine < y ) {  // for word worp
          if ( ( m_imStart < m_imEnd ) )
            m_isIMEdit = true;

          if ( ( m_imSelStart < m_imSelEnd ) )
            m_isIMSel = true;
	}

        bool save_fixed_font = fixed_font;
        if (lineDraw)
           fixed_font = false;
        if (doubleWidth)
           fixed_font = false;
        drawAttrStr(paint,
                    TQRect(bX+tLx+font_w*x,bY+tLy+font_h*y,font_w*len,font_h),
                    unistr, &ext[x], pm != NULL, true);
        fixed_font = save_fixed_font;
        x += len - 1;
      }
    }

    dirtyMask--; // Set back

    // finally, make `image' become `newimg'.
    memcpy((void*)lcl,(const void*)ext,cols*sizeof(ca));
  }
  drawFrame( &paint );
  paint.end();
  setUpdatesEnabled(true);
  if ( hasBlinker && !blinkT->isActive()) blinkT->start(1000); // 1000 ms
  if (!hasBlinker && blinkT->isActive()) { blinkT->stop(); blinking = false; }
  free(dirtyMask);
  delete [] disstrU;

  if (resizing && terminalSizeHint)
  {
     if (terminalSizeStartup) {
       terminalSizeStartup=false;
       return;
     }
     if (!mResizeWidget)
     {
        mResizeWidget = new TQFrame(this);
        TQFont f = KGlobalSettings::generalFont();
        int fs = f.pointSize();
        if (fs == -1)
           fs = TQFontInfo(f).pointSize();
        f.setPointSize((fs*3)/2);
        f.setBold(true);
        mResizeWidget->setFont(f);
        mResizeWidget->setFrameShape((TQFrame::Shape) (TQFrame::Box|TQFrame::Raised));
        mResizeWidget->setMidLineWidth(4);
        TQBoxLayout *l = new TQVBoxLayout( mResizeWidget, 10);
        mResizeLabel = new TQLabel(i18n("Size: XXX x XXX"), mResizeWidget);
        l->addWidget(mResizeLabel, 1, AlignCenter);
        mResizeWidget->setMinimumWidth(mResizeLabel->fontMetrics().width(i18n("Size: XXX x XXX"))+20);
        mResizeWidget->setMinimumHeight(mResizeLabel->sizeHint().height()+20);
        mResizeTimer = new TQTimer(this);
        connect(mResizeTimer, TQT_SIGNAL(timeout()), mResizeWidget, TQT_SLOT(hide()));
     }
     TQString sizeStr = i18n("Size: %1 x %2").arg(columns).arg(lines);
     mResizeLabel->setText(sizeStr);
     mResizeWidget->move((width()-mResizeWidget->width())/2,
                         (height()-mResizeWidget->height())/2+20);
     mResizeWidget->show();
     mResizeTimer->start(1000, true);
  }
}

void TEWidget::setBlinkingCursor(bool blink)
{
  hasBlinkingCursor=blink;
  if (blink && !blinkCursorT->isActive()) blinkCursorT->start(1000);
  if (!blink && blinkCursorT->isActive()) {
    blinkCursorT->stop();
    if (cursorBlinking)
      blinkCursorEvent();
    else
      cursorBlinking = false;
  }
}

// paint Event ////////////////////////////////////////////////////

/*!
    The difference of this routine vs. the `setImage' is,
    that the drawing does not include a difference analysis
    between the old and the new image. Instead, the internal
    image is used and the painting bound by the PaintEvent box.
*/

void TEWidget::paintEvent( TQPaintEvent* pe )
{
  const TQPixmap* pm = backgroundPixmap();
  TQPainter paint;
  setUpdatesEnabled(false);
  paint.begin( this );
  paint.setBackgroundMode( Qt::TransparentMode );

  // Note that the actual widget size can be slightly larger
  // that the image (the size is truncated towards the smaller
  // number of characters in `resizeEvent'. The paint rectangle
  // can thus be larger than the image, but less then the size
  // of one character.

  TQRect rect = pe->rect().intersect(contentsRect());

  paintContents(paint, rect, pm != 0);

  drawFrame( &paint );

  // Since we're using TQt::WNoAutoErase, we have to make sure that
  // every single pixel is painted by the paint event.
  // To do this, we must figure out which pixels are left in the
  // area between the terminal image and the frame border.

  // Calculate the contents rect excluding scroll bar.
  TQRect innerRect = contentsRect();
  if( scrollLoc != SCRNONE )
    innerRect.setWidth( innerRect.width() - scrollbar->width() );

  innerRect.setWidth( innerRect.width() + 3 );
  innerRect.setHeight( innerRect.height() );

  // Calculate the emulation rect (area needed for actual terminal contents)
  TQRect emurect( contentsRect().topLeft(), TQSize( columns * font_w + 2 * rimX, lines * font_h + 2 * rimY ));

  // Now erase() the remaining pixels on all sides of the emulation

  // Top
  TQRect er( innerRect );
  er.setBottom( emurect.top() );
  erase( er );

  // Bottom
  er.setBottom( innerRect.bottom() );
  er.setTop( emurect.bottom() );
  erase( er );

  // Left
  er.setTop( emurect.top() );
  er.setBottom( emurect.bottom() - 1 );
  er.setRight( emurect.left() );
  erase( er );

  // Right
  er.setRight( innerRect.right() );
  er.setTop( emurect.top() );
  er.setBottom( emurect.bottom() - 1 );
  er.setLeft( emurect.right() );
  erase( er );

  paint.end();
  setUpdatesEnabled(true);
}

void TEWidget::print(TQPainter &paint, bool friendly, bool exact)
{
   bool save_fixed_font = fixed_font;
   bool save_blinking = blinking;
   fixed_font = false;
   blinking = false;
   paint.setFont(font());

   isPrinting = true;
   printerFriendly = friendly;
   printerBold = !exact;

   if (exact)
   {
     TQPixmap pm(contentsRect().right(), contentsRect().bottom());
     pm.fill();

     TQPainter pm_paint;
     pm_paint.begin(&pm, TQT_TQWIDGET(this));
     paintContents(pm_paint, contentsRect(), true);
     pm_paint.end();
     paint.drawPixmap(0, 0, pm);
   }
   else
   {
     paintContents(paint, contentsRect(), true);
   }

   printerFriendly = false;
   isPrinting = false;
   printerBold = false;

   fixed_font = save_fixed_font;
   blinking = save_blinking;
}

void TEWidget::paintContents(TQPainter &paint, const TQRect &rect, bool pm)
{
  TQPoint tL  = contentsRect().topLeft();
  int    tLx = tL.x();
  int    tLy = tL.y();

  int lux = TQMIN(columns-1, TQMAX(0,(rect.left()   - tLx - bX ) / font_w));
  int luy = TQMIN(lines-1,   TQMAX(0,(rect.top()    - tLy - bY  ) / font_h));
  int rlx = TQMIN(columns-1, TQMAX(0,(rect.right()  - tLx - bX ) / font_w));
  int rly = TQMIN(lines-1,   TQMAX(0,(rect.bottom() - tLy - bY  ) / font_h));

  TQChar *disstrU = new TQChar[columns];
  for (int y = luy; y <= rly; y++)
  {
    TQ_UINT16 c = image[loc(lux,y)].c;
    int x = lux;
    if(!c && x)
      x--; // Search for start of multi-col char
    for (; x <= rlx; x++)
    {
      int len = 1;
      int p = 0;
      c = image[loc(x,y)].c;
      if (c)
         disstrU[p++] = c; //fontMap(c);
      bool lineDraw = isLineChar(c);
      bool doubleWidth = (image[loc(x,y)+1].c == 0);
      cacol cf = image[loc(x,y)].f;
      cacol cb = image[loc(x,y)].b;
      int   cr = image[loc(x,y)].r;
      while (x+len <= rlx &&
             image[loc(x+len,y)].f == cf &&
             image[loc(x+len,y)].b == cb &&
             image[loc(x+len,y)].r == cr &&
             (image[loc(x+len,y)+1].c == 0) == doubleWidth &&
             isLineChar( c = image[loc(x+len,y)].c) == lineDraw) // Assignment!
      {
        if (c)
          disstrU[p++] = c; //fontMap(c);
        if (doubleWidth) // assert((image[loc(x+len,y)+1].c == 0)), see above if condition
          len++; // Skip trailing part of multi-column char
        len++;
      }
      if ((x+len < columns) && (!image[loc(x+len,y)].c))
        len++; // Adjust for trailing part of multi-column char

      if (!isBlinkEvent || (cr & RE_BLINK))
      {
         bool save_fixed_font = fixed_font;
         if (lineDraw)
            fixed_font = false;
         if (doubleWidth)
            fixed_font = false;
         TQString unistr(disstrU,p);
         drawAttrStr(paint,
                TQRect(bX+tLx+font_w*x,bY+tLy+font_h*y,font_w*len,font_h),
                unistr, &image[loc(x,y)], pm, !(isBlinkEvent || isPrinting));
         fixed_font = save_fixed_font;
      }
      x += len - 1;
    }
  }
  delete [] disstrU;
}

void TEWidget::blinkEvent()
{
  blinking = !blinking;
  isBlinkEvent = true;
  repaint(false);
  isBlinkEvent = false;
}

void TEWidget::blinkCursorEvent()
{
  cursorBlinking = !cursorBlinking;
  repaint(cursorRect, true);
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                  Resizing                                 */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TEWidget::resizeEvent(TQResizeEvent*)
{
  updateImageSize();
}

void TEWidget::propagateSize()
{
  if (isFixedSize)
  {
     setSize(columns, lines);
     TQFrame::setFixedSize(sizeHint());
     parentWidget()->adjustSize();
     parentWidget()->setFixedSize(parentWidget()->sizeHint());
     return;
  }
  if (image)
     updateImageSize();
}

void TEWidget::updateImageSize()
{
  ca* oldimg = image;
  int oldlin = lines;
  int oldcol = columns;
  makeImage();
  // we copy the old image to reduce flicker
  int lins = QMIN(oldlin,lines);
  int cols = QMIN(oldcol,columns);
  if (oldimg)
  {
    for (int lin = 0; lin < lins; lin++)
      memcpy((void*)&image[columns*lin],
             (void*)&oldimg[oldcol*lin],cols*sizeof(ca));
    free(oldimg); //FIXME: try new,delete
  }

  //NOTE: control flows from the back through the chest right into the eye.
  //      `emu' will call back via `setImage'.

  resizing = (oldlin!=lines) || (oldcol!=columns);
  emit changedContentSizeSignal(contentHeight, contentWidth); // expose resizeEvent
  resizing = false;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                Scrollbar                                  */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TEWidget::scrollChanged(int)
{
  emit changedHistoryCursor(scrollbar->value()); //expose
}

void TEWidget::setScroll(int cursor, int slines)
{
  //kdDebug(1211)<<"TEWidget::setScroll() disconnect()"<<endl;
  disconnect(scrollbar, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(scrollChanged(int)));
  //kdDebug(1211)<<"TEWidget::setScroll() setRange()"<<endl;
  scrollbar->setRange(0,slines);
  //kdDebug(1211)<<"TEWidget::setScroll() setSteps()"<<endl;
  scrollbar->setSteps(1,lines);
  scrollbar->setValue(cursor);
  connect(scrollbar, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(scrollChanged(int)));
  //kdDebug(1211)<<"TEWidget::setScroll() done"<<endl;
}

void TEWidget::setScrollbarLocation(int loc)
{
  if (scrollLoc == loc) return; // quickly
  bY = bX = 1;
  scrollLoc = loc;
  calcGeometry();
  propagateSize();
  update();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                   Mouse                                   */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*!
    Three different operations can be performed using the mouse, and the
    routines in this section serve all of them:

    1) The press/release events are exposed to the application
    2) Marking (press and move left button) and Pasting (press middle button)
    3) The right mouse button is used from the configuration menu

    NOTE: During the marking process we attempt to keep the cursor within
    the bounds of the text as being displayed by setting the mouse position
    whenever the mouse has left the text area.

    Two reasons to do so:
    1) QT does not allow the `grabMouse' to confine-to the TEWidget.
       Thus a `XGrapPointer' would have to be used instead.
    2) Even if so, this would not help too much, since the text area
       of the TEWidget is normally not identical with it's bounds.

    The disadvantage of the current handling is, that the mouse can visibly
    leave the bounds of the widget and is then moved back. Because of the
    current construction, and the reasons mentioned above, we cannot do better
    without changing the overall construction.
*/

/*!
*/

void TEWidget::mousePressEvent(TQMouseEvent* ev)
{
//printf("press [%d,%d] %d\n",ev->x()/font_w,ev->y()/font_h,ev->button());

  if ( possibleTripleClick && (ev->button()==Qt::LeftButton) ) {
    mouseTripleClickEvent(ev);
    return;
  }

  if ( !contentsRect().contains(ev->pos()) ) return;
  TQPoint tL  = contentsRect().topLeft();
  int    tLx = tL.x();
  int    tLy = tL.y();

  TQPoint pos = TQPoint((ev->x()-tLx-bX+(font_w/2))/font_w,(ev->y()-tLy-bY)/font_h);

//printf("press top left [%d,%d] by=%d\n",tLx,tLy, bY);
  if ( ev->button() == Qt::LeftButton)
  {
    line_selection_mode = false;
    word_selection_mode = false;

    emit isBusySelecting(true); // Keep it steady...
    // Drag only when the Control key is hold
    bool selected = false;
    // The receiver of the testIsSelected() signal will adjust
    // 'selected' accordingly.
    emit testIsSelected(pos.x(), pos.y(), selected);
    if ((!ctrldrag || ev->state() & ControlButton) && selected ) {
      // The user clicked inside selected text
      dragInfo.state = diPending;
      dragInfo.start = ev->pos();
    }
    else {
      // No reason to ever start a drag event
      dragInfo.state = diNone;

      preserve_line_breaks = !( ( ev->state() & ControlButton ) && !(ev->state() & AltButton) );
      column_selection_mode = (ev->state() & AltButton) && (ev->state() & ControlButton);

      if (mouse_marks || (ev->state() & ShiftButton))
      {
        emit clearSelectionSignal();
        pos.ry() += scrollbar->value();
        iPntSel = pntSel = pos;
        actSel = 1; // left mouse button pressed but nothing selected yet.
        grabMouse(   /*crossCursor*/  ); // handle with care!
      }
      else
      {
        emit mouseSignal( 0, (ev->x()-tLx-bX)/font_w +1, (ev->y()-tLy-bY)/font_h +1 +scrollbar->value() -scrollbar->maxValue() );
      }
    }
  }
  else if ( ev->button() == Qt::MidButton )
  {
    if ( mouse_marks || (!mouse_marks && (ev->state() & ShiftButton)) )
      emitSelection(true,ev->state() & ControlButton);
    else
      emit mouseSignal( 1, (ev->x()-tLx-bX)/font_w +1, (ev->y()-tLy-bY)/font_h +1 +scrollbar->value() -scrollbar->maxValue() );
  }
  else if ( ev->button() == Qt::RightButton )
  {
    if (mouse_marks || (ev->state() & ShiftButton)) {
      configureRequestPoint = TQPoint( ev->x(), ev->y() );
      emit configureRequest( this, ev->state()&(ShiftButton|ControlButton), ev->x(), ev->y() );
    }
    else
      emit mouseSignal( 2, (ev->x()-tLx-bX)/font_w +1, (ev->y()-tLy-bY)/font_h +1 +scrollbar->value() -scrollbar->maxValue() );
  }
}

void TEWidget::mouseMoveEvent(TQMouseEvent* ev)
{
  // for auto-hiding the cursor, we need mouseTracking
  if (ev->state() == Qt::NoButton ) return;

  if (dragInfo.state == diPending) {
    // we had a mouse down, but haven't confirmed a drag yet
    // if the mouse has moved sufficiently, we will confirm

   int distance = KGlobalSettings::dndEventDelay();
   if ( ev->x() > dragInfo.start.x() + distance || ev->x() < dragInfo.start.x() - distance ||
        ev->y() > dragInfo.start.y() + distance || ev->y() < dragInfo.start.y() - distance) {
      // we've left the drag square, we can start a real drag operation now
      emit isBusySelecting(false); // Ok.. we can breath again.
      emit clearSelectionSignal();
      doDrag();
    }
    return;
  } else if (dragInfo.state == diDragging) {
    // this isn't technically needed because mouseMoveEvent is suppressed during
    // Qt drag operations, replaced by dragMoveEvent
    return;
  }

  if (actSel == 0) return;

 // don't extend selection while pasting
  if (ev->state() & Qt::MidButton) return;

  extendSelection( ev->pos() );
}

void TEWidget::setSelectionEnd()
{
  extendSelection( configureRequestPoint );
}

void TEWidget::extendSelection( TQPoint pos )
{
  //if ( !contentsRect().contains(ev->pos()) ) return;
  TQPoint tL  = contentsRect().topLeft();
  int    tLx = tL.x();
  int    tLy = tL.y();
  int    scroll = scrollbar->value();

  // we're in the process of moving the mouse with the left button pressed
  // the mouse cursor will kept caught within the bounds of the text in
  // this widget.

  // Adjust position within text area bounds. See FIXME above.
  TQPoint oldpos = pos;
  if ( pos.x() < tLx+bX )                  pos.setX( tLx+bX );
  if ( pos.x() > tLx+bX+columns*font_w-1 ) pos.setX( tLx+bX+columns*font_w );
  if ( pos.y() < tLy+bY )                   pos.setY( tLy+bY );
  if ( pos.y() > tLy+bY+lines*font_h-1 )    pos.setY( tLy+bY+lines*font_h-1 );

  // check if we produce a mouse move event by this
  if ( pos != oldpos ) cursor().setPos(mapToGlobal(pos));

  if ( pos.y() == tLy+bY+lines*font_h-1 )
  {
    scrollbar->setValue(scrollbar->value()+yMouseScroll); // scrollforward
  }
  if ( pos.y() == tLy+bY )
  {
    scrollbar->setValue(scrollbar->value()-yMouseScroll); // scrollback
  }

  TQPoint here = TQPoint((pos.x()-tLx-bX+(font_w/2))/font_w,(pos.y()-tLy-bY)/font_h);
  TQPoint ohere;
  TQPoint iPntSelCorr = iPntSel;
  iPntSelCorr.ry() -= scrollbar->value();
  TQPoint pntSelCorr = pntSel;
  pntSelCorr.ry() -= scrollbar->value();
  bool swapping = false;

  if ( word_selection_mode )
  {
    // Extend to word boundaries
    int i;
    int selClass;

    bool left_not_right = ( here.y() < iPntSelCorr.y() ||
	   here.y() == iPntSelCorr.y() && here.x() < iPntSelCorr.x() );
    bool old_left_not_right = ( pntSelCorr.y() < iPntSelCorr.y() ||
	   pntSelCorr.y() == iPntSelCorr.y() && pntSelCorr.x() < iPntSelCorr.x() );
    swapping = left_not_right != old_left_not_right;

    // Find left (left_not_right ? from here : from start)
    TQPoint left = left_not_right ? here : iPntSelCorr;
    i = loc(left.x(),left.y());
    if (i>=0 && i<=image_size) {
      selClass = charClass(image[i].c);
      while ( ((left.x()>0) || (left.y()>0 && m_line_wrapped[left.y()-1])) && charClass(image[i-1].c) == selClass )
      { i--; if (left.x()>0) left.rx()--; else {left.rx()=columns-1; left.ry()--;} }
    }

    // Find left (left_not_right ? from start : from here)
    TQPoint right = left_not_right ? iPntSelCorr : here;
    i = loc(right.x(),right.y());
    if (i>=0 && i<=image_size) {
      selClass = charClass(image[i].c);
      while( ((right.x()<columns-1) || (right.y()<lines-1 && m_line_wrapped[right.y()])) && charClass(image[i+1].c) == selClass )
      { i++; if (right.x()<columns-1) right.rx()++; else {right.rx()=0; right.ry()++; } }
    }

    // Pick which is start (ohere) and which is extension (here)
    if ( left_not_right )
    {
      here = left; ohere = right;
    }
    else
    {
      here = right; ohere = left;
    }
    ohere.rx()++;
  }

  if ( line_selection_mode )
  {
    // Extend to complete line
    bool above_not_below = ( here.y() < iPntSelCorr.y() );

    TQPoint above = above_not_below ? here : iPntSelCorr;
    TQPoint below = above_not_below ? iPntSelCorr : here;

    while (above.y()>0 && m_line_wrapped[above.y()-1])
      above.ry()--;
    while (below.y()<lines-1 && m_line_wrapped[below.y()])
      below.ry()++;

    above.setX(0);
    below.setX(columns-1);

    // Pick which is start (ohere) and which is extension (here)
    if ( above_not_below )
    {
      here = above; ohere = below;
    }
    else
    {
      here = below; ohere = above;
    }

    TQPoint newSelBegin = TQPoint( ohere.x(), ohere.y() );
    swapping = !(tripleSelBegin==newSelBegin);
    tripleSelBegin = newSelBegin;

    ohere.rx()++;
  }

  int offset = 0;
  if ( !word_selection_mode && !line_selection_mode )
  {
    int i;
    int selClass;

    bool left_not_right = ( here.y() < iPntSelCorr.y() ||
	   here.y() == iPntSelCorr.y() && here.x() < iPntSelCorr.x() );
    bool old_left_not_right = ( pntSelCorr.y() < iPntSelCorr.y() ||
	   pntSelCorr.y() == iPntSelCorr.y() && pntSelCorr.x() < iPntSelCorr.x() );
    swapping = left_not_right != old_left_not_right;

    // Find left (left_not_right ? from here : from start)
    TQPoint left = left_not_right ? here : iPntSelCorr;

    // Find left (left_not_right ? from start : from here)
    TQPoint right = left_not_right ? iPntSelCorr : here;
    if ( right.x() > 0 && !column_selection_mode )
    {
      i = loc(right.x(),right.y());
      if (i>=0 && i<=image_size) {
        selClass = charClass(image[i-1].c);
        if (selClass == ' ')
        {
          while ( right.x() < columns-1 && charClass(image[i+1].c) == selClass && (right.y()<lines-1) && !m_line_wrapped[right.y()])
          { i++; right.rx()++; }
          if (right.x() < columns-1)
            right = left_not_right ? iPntSelCorr : here;
          else
            right.rx()++;  // will be balanced later because of offset=-1;
        }
      }
    }

    // Pick which is start (ohere) and which is extension (here)
    if ( left_not_right )
    {
      here = left; ohere = right; offset = 0;
    }
    else
    {
      here = right; ohere = left; offset = -1;
    }
  }

  if ((here == pntSelCorr) && (scroll == scrollbar->value())) return; // not moved

  if (here == ohere) return; // It's not left, it's not right.

  if ( actSel < 2 || swapping )
    if ( column_selection_mode && !line_selection_mode && !word_selection_mode )
      emit beginSelectionSignal( ohere.x(), ohere.y(), true );
    else
      emit beginSelectionSignal( ohere.x()-1-offset, ohere.y(), false );

  actSel = 2; // within selection
  pntSel = here;
  pntSel.ry() += scrollbar->value();

  if ( column_selection_mode && !line_selection_mode && !word_selection_mode )
    emit extendSelectionSignal( here.x(), here.y() );
  else
    emit extendSelectionSignal( here.x()+offset, here.y() );
}

void TEWidget::mouseReleaseEvent(TQMouseEvent* ev)
{
//printf("release [%d,%d] %d\n",ev->x()/font_w,ev->y()/font_h,ev->button());
  if ( ev->button() == Qt::LeftButton)
  {
    emit isBusySelecting(false); // Ok.. we can breath again.
    if(dragInfo.state == diPending)
    {
      // We had a drag event pending but never confirmed.  Kill selection
      emit clearSelectionSignal();
    }
    else
    {
      if ( actSel > 1 )
          emit endSelectionSignal(preserve_line_breaks);
      actSel = 0;

      //FIXME: emits a release event even if the mouse is
      //       outside the range. The procedure used in `mouseMoveEvent'
      //       applies here, too.

      TQPoint tL  = contentsRect().topLeft();
      int    tLx = tL.x();
      int    tLy = tL.y();

      if (!mouse_marks && !(ev->state() & ShiftButton))
        emit mouseSignal( 3, // release
                        (ev->x()-tLx-bX)/font_w + 1,
                        (ev->y()-tLy-bY)/font_h + 1 +scrollbar->value() -scrollbar->maxValue());
      releaseMouse();
    }
    dragInfo.state = diNone;
  }
  if ( !mouse_marks && ((ev->button() == Qt::RightButton && !(ev->state() & ShiftButton))
                        || ev->button() == Qt::MidButton) ) {
    TQPoint tL  = contentsRect().topLeft();
    int    tLx = tL.x();
    int    tLy = tL.y();

    emit mouseSignal( 3, (ev->x()-tLx-bX)/font_w +1, (ev->y()-tLy-bY)/font_h +1 +scrollbar->value() -scrollbar->maxValue() );
    releaseMouse();
  }
}

void TEWidget::mouseDoubleClickEvent(TQMouseEvent* ev)
{
  if ( ev->button() != Qt::LeftButton) return;

  TQPoint tL  = contentsRect().topLeft();
  int    tLx = tL.x();
  int    tLy = tL.y();
  TQPoint pos = TQPoint((ev->x()-tLx-bX)/font_w,(ev->y()-tLy-bY)/font_h);

  // pass on double click as two clicks.
  if (!mouse_marks && !(ev->state() & ShiftButton))
  {
    // Send just _ONE_ click event, since the first click of the double click
    // was already sent by the click handler!
    emit mouseSignal( 0, pos.x()+1, pos.y()+1 +scrollbar->value() -scrollbar->maxValue() ); // left button
    return;
  }


  emit clearSelectionSignal();
  TQPoint bgnSel = pos;
  TQPoint endSel = pos;
  int i = loc(bgnSel.x(),bgnSel.y());
  iPntSel = bgnSel;
  iPntSel.ry() += scrollbar->value();

  word_selection_mode = true;

  // find word boundaries...
  int selClass = charClass(image[i].c);
  {
    // set the start...
     int x = bgnSel.x();
     while ( ((x>0) || (bgnSel.y()>0 && m_line_wrapped[bgnSel.y()-1])) && charClass(image[i-1].c) == selClass )
     { i--; if (x>0) x--; else {x=columns-1; bgnSel.ry()--;} }
     bgnSel.setX(x);
     emit beginSelectionSignal( bgnSel.x(), bgnSel.y(), false );

     // set the end...
     i = loc( endSel.x(), endSel.y() );
     x = endSel.x();
     while( ((x<columns-1) || (endSel.y()<lines-1 && m_line_wrapped[endSel.y()])) && charClass(image[i+1].c) == selClass )
     { i++; if (x<columns-1) x++; else {x=0; endSel.ry()++; } }
     endSel.setX(x);

     // In word selection mode don't select @ (64) if at end of word.
     if ( ( TQChar( image[i].c ) == '@' ) && ( ( endSel.x() - bgnSel.x() ) > 0 ) )
       endSel.setX( x - 1 );

     actSel = 2; // within selection
     emit extendSelectionSignal( endSel.x(), endSel.y() );
     emit endSelectionSignal(preserve_line_breaks);
   }

  possibleTripleClick=true;
  TQTimer::singleShot(TQApplication::doubleClickInterval(),this,TQT_SLOT(tripleClickTimeout()));
}

void TEWidget::wheelEvent( TQWheelEvent* ev )
{
  if (ev->orientation() != Qt::Vertical)
    return;

  if ( mouse_marks )
    TQApplication::sendEvent(scrollbar, ev);
  else
  {
    TQPoint tL  = contentsRect().topLeft();
    int    tLx = tL.x();
    int    tLy = tL.y();
    TQPoint pos = TQPoint((ev->x()-tLx-bX)/font_w,(ev->y()-tLy-bY)/font_h);
    emit mouseSignal( ev->delta() > 0 ? 4 : 5, pos.x() + 1, pos.y() + 1 +scrollbar->value() -scrollbar->maxValue() );
  }
}

void TEWidget::tripleClickTimeout()
{
  possibleTripleClick=false;
}

void TEWidget::mouseTripleClickEvent(TQMouseEvent* ev)
{
  TQPoint tL  = contentsRect().topLeft();
  int    tLx = tL.x();
  int    tLy = tL.y();
  iPntSel = TQPoint((ev->x()-tLx-bX)/font_w,(ev->y()-tLy-bY)/font_h);

  emit clearSelectionSignal();

  line_selection_mode = true;
  word_selection_mode = false;

  actSel = 2; // within selection
  emit isBusySelecting(true); // Keep it steady...

  while (iPntSel.y()>0 && m_line_wrapped[iPntSel.y()-1])
    iPntSel.ry()--;
  if (cuttobeginningofline) {
    // find word boundary start
    int i = loc(iPntSel.x(),iPntSel.y());
    int selClass = charClass(image[i].c);
    int x = iPntSel.x();
    while ( ((x>0) || (iPntSel.y()>0 && m_line_wrapped[iPntSel.y()-1])) && charClass(image[i-1].c) == selClass )
    { i--; if (x>0) x--; else {x=columns-1; iPntSel.ry()--;} }

    emit beginSelectionSignal( x, iPntSel.y(), false );
    tripleSelBegin = TQPoint( x, iPntSel.y() );
  }
  else {
    emit beginSelectionSignal( 0, iPntSel.y(), false );
    tripleSelBegin = TQPoint( 0, iPntSel.y() );
  }

  while (iPntSel.y()<lines-1 && m_line_wrapped[iPntSel.y()])
    iPntSel.ry()++;
  emit extendSelectionSignal( columns-1, iPntSel.y() );

  emit endSelectionSignal(preserve_line_breaks);

  iPntSel.ry() += scrollbar->value();
}

void TEWidget::focusInEvent( TQFocusEvent * )
{
  repaint(cursorRect, true);  // *do* erase area, to get rid of the
                              // hollow cursor rectangle.
}


void TEWidget::focusOutEvent( TQFocusEvent * )
{
  repaint(cursorRect, true);  // don't erase area
}

bool TEWidget::focusNextPrevChild( bool next )
{
  if (next)
    return false; // This disables changing the active part in konqueror
                  // when pressing Tab
  return TQFrame::focusNextPrevChild( next );
}


int TEWidget::charClass(UINT16 ch) const
{
    TQChar qch=TQChar(ch);
    if ( qch.isSpace() ) return ' ';

    if ( qch.isLetterOrNumber() || word_characters.contains(qch, false) )
    return 'a';

    // Everything else is weird
    return 1;
}

void TEWidget::setWordCharacters(TQString wc)
{
	word_characters = wc;
}

void TEWidget::setMouseMarks(bool on)
{
  mouse_marks = on;
  setCursor( mouse_marks ? tqibeamCursor : tqarrowCursor );
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                               Clipboard                                   */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#undef KeyPress

void TEWidget::emitText(TQString text)
{
  if (!text.isEmpty()) {
    TQKeyEvent e(TQEvent::KeyPress, 0,-1,0, text);
    emit keyPressedSignal(&e); // expose as a big fat keypress event
  }
}

void TEWidget::emitSelection(bool useXselection,bool appendReturn)
// Paste Clipboard by simulating keypress events
{
  TQApplication::clipboard()->setSelectionMode( useXselection );
  TQString text = TQApplication::clipboard()->text();
  if(appendReturn)
    text.append("\r");
  if ( ! text.isEmpty() )
  {
    text.replace("\n", "\r");
    TQKeyEvent e(TQEvent::KeyPress, 0,-1,0, text);
    emit keyPressedSignal(&e); // expose as a big fat keypress event
    emit clearSelectionSignal();
  }
  TQApplication::clipboard()->setSelectionMode( false );
}

void TEWidget::setSelection(const TQString& t)
{
  // Disconnect signal while WE set the clipboard
  TQClipboard *cb = TQApplication::clipboard();
  TQObject::disconnect( cb, TQT_SIGNAL(selectionChanged()),
                     this, TQT_SLOT(onClearSelection()) );

  cb->setSelectionMode( true );
  cb->setText(t);
  cb->setSelectionMode( false );

  TQObject::connect( cb, TQT_SIGNAL(selectionChanged()),
                     this, TQT_SLOT(onClearSelection()) );
}

void TEWidget::copyClipboard()
{
  emit copySelectionSignal();
}

void TEWidget::pasteClipboard()
{
  emitSelection(false,false);
}

void TEWidget::pasteSelection()
{
  emitSelection(true,false);
}

void TEWidget::onClearSelection()
{
  emit clearSelectionSignal();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                Keyboard                                   */
/*                                                                           */
/* ------------------------------------------------------------------------- */

//FIXME: an `eventFilter' has been installed instead of a `keyPressEvent'
//       due to a bug in `QT' or the ignorance of the author to prevent
//       repaint events being emitted to the screen whenever one leaves
//       or reenters the screen to/from another application.
//
//   Troll says one needs to change focusInEvent() and focusOutEvent(),
//   which would also let you have an in-focus cursor and an out-focus
//   cursor like xterm does.

// for the auto-hide cursor feature, I added empty focusInEvent() and
// focusOutEvent() so that update() isn't called.
// For auto-hide, we need to get keypress-events, but we only get them when
// we have focus.

void TEWidget::doScroll(int lines)
{
  scrollbar->setValue(scrollbar->value()+lines);
}

bool TEWidget::eventFilter( TQObject *obj, TQEvent *e )
{
  if ( (e->type() == TQEvent::Accel ||
       e->type() == TQEvent::AccelAvailable ) && tqApp->focusWidget() == this )
  {
      TQT_TQKEYEVENT( e )->ignore();
      return false;
  }
  if ( TQT_BASE_OBJECT(obj) != TQT_BASE_OBJECT(this) /* when embedded */ && TQT_BASE_OBJECT(obj) != TQT_BASE_OBJECT(parent()) /* when standalone */ )
      return false; // not us
  if ( e->type() == TQEvent::KeyPress )
  {
    TQKeyEvent* ke = (TQKeyEvent*)e;

    actSel=0; // Key stroke implies a screen update, so TEWidget won't
              // know where the current selection is.

    if (hasBlinkingCursor) {
      blinkCursorT->start(1000);
      if (cursorBlinking)
        blinkCursorEvent();
      else
        cursorBlinking = false;
    }

    emit keyPressedSignal(ke); // expose

    // in Qt2 when key events were propagated up the tree
    // (unhandled? -> parent widget) they passed the event filter only once at
    // the beginning. in qt3 this has changed, that is, the event filter is
    // called each time the event is sent (see loop in TQApplication::notify,
    // when internalNotify() is called for KeyPress, whereas internalNotify
    // activates also the global event filter) . That's why we stop propagation
    // here.
    return true;
  }
  if ( e->type() == TQEvent::Enter )
  {
    TQObject::disconnect( (TQObject*)cb, TQT_SIGNAL(dataChanged()),
      this, TQT_SLOT(onClearSelection()) );
  }
  if ( e->type() == TQEvent::Leave )
  {
    TQObject::connect( (TQObject*)cb, TQT_SIGNAL(dataChanged()),
      this, TQT_SLOT(onClearSelection()) );
  }
  return TQFrame::eventFilter( obj, e );
}

void TEWidget::imStartEvent( TQIMEvent */*e*/ )
{
  m_imStart = m_cursorCol;
  m_imStartLine = m_cursorLine;
  m_imPreeditLength = 0;

  m_imEnd = m_imSelStart = m_imSelEnd = 0;
  m_isIMEdit = m_isIMSel = false;
}

void TEWidget::imComposeEvent( TQIMEvent *e )
{
  TQString text = TQString::null;
  if ( m_imPreeditLength > 0 ) {
    text.fill( '\010', m_imPreeditLength );
  }

  m_imEnd = m_imStart + string_width( e->text() );

  TQString tmpStr = e->text().left( e->cursorPos() );
  m_imSelStart = m_imStart + string_width( tmpStr );

  tmpStr = e->text().mid( e->cursorPos(), e->selectionLength() );
  m_imSelEnd = m_imSelStart + string_width( tmpStr );
  m_imPreeditLength = e->text().length();
  m_imPreeditText = e->text();
  text += e->text();

  if ( text.length() > 0 ) {
    TQKeyEvent ke( TQEvent::KeyPress, 0, -1, 0, text );
    emit keyPressedSignal( &ke );
  }
}

void TEWidget::imEndEvent( TQIMEvent *e )
{
  TQString text = TQString::null;
  if ( m_imPreeditLength > 0 ) {
      text.fill( '\010', m_imPreeditLength );
  }

  m_imEnd = m_imSelStart = m_imSelEnd = 0;
  text += e->text();
  if ( text.length() > 0 ) {
    TQKeyEvent ke( TQEvent::KeyPress, 0, -1, 0, text );
    emit keyPressedSignal( &ke );
  }

  TQPoint tL  = contentsRect().topLeft();
  int tLx = tL.x();
  int tLy = tL.y();

  TQRect repaintRect = TQRect( bX+tLx, bY+tLy+font_h*m_imStartLine,
                             contentsRect().width(), contentsRect().height() );
  m_imStart = 0;
  m_imPreeditLength = 0;

  m_isIMEdit = m_isIMSel = false;
  repaint( repaintRect, true );
}

// Override any Ctrl+<key> accelerator when pressed with the keyboard
// focus in TEWidget, so that the key will be passed to the terminal instead.
bool TEWidget::event( TQEvent *e )
{
  if ( e->type() == TQEvent::AccelOverride )
  {
    TQKeyEvent *ke = TQT_TQKEYEVENT( e );
    KKey key( ke );
    int keyCodeQt = key.keyCodeQt();

    if ( !standalone() && (ke->state() == ControlButton) )
    {
      ke->accept();
      return true;
    }

    // Override any of the following accelerators:
    switch ( keyCodeQt )
    {
      case Key_Tab:
      case Key_Delete:
        ke->accept();
        return true;
    }
  }
  return TQFrame::event( e );
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                  Frame                                    */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TEWidget::frameChanged()
{
  propagateSize();
  update();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                   Sound                                   */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TEWidget::setBellMode(int mode)
{
  m_bellMode=mode;
}

void TEWidget::Bell(bool visibleSession, TQString message)
{
  if (bellTimer.isActive())
          return;

  //minimum delay in milliseconds between each bell event
  //for the 3 different types of bells.
  const int BELLSYSTEM_DELAY = 100;
  const int BELLNOTIFY_DELAY = 500; //longer to avoid horrible noise with several audible system
                                    //notifications in close succession
  const int BELLVISUAL_DELAY = 500; //longer to avoid ugly flickering with several flashes in close
                                    //succession

  if (m_bellMode==BELLNONE) return;

  if (m_bellMode==BELLSYSTEM) {
    bellTimer.start(BELLSYSTEM_DELAY,true);
    KNotifyClient::beep();
  } else if (m_bellMode==BELLNOTIFY) {
    bellTimer.start(BELLNOTIFY_DELAY,true);

    if (visibleSession)
      KNotifyClient::event(winId(), "BellVisible", message);
    else
      KNotifyClient::event(winId(), "BellInvisible", message);
  } else if (m_bellMode==BELLVISUAL) {
    bellTimer.start(BELLVISUAL_DELAY,true);
    
    swapColorTable();
    TQTimer::singleShot(200,this,TQT_SLOT(swapColorTable()));
  }
}

void TEWidget::swapColorTable()
{
  ColorEntry color = color_table[1];
  color_table[1]=color_table[0];
  color_table[0]= color;
  colorsSwapped = !colorsSwapped;
  update();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                 Auxiluary                                 */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void TEWidget::clearImage()
// initialize the image
// for internal use only
{
  // We initialize image[image_size] too. See makeImage()
  for (int i = 0; i <= image_size; i++)
  {
    image[i].c = ' ';
    image[i].f = cacol(CO_DFT,DEFAULT_FORE_COLOR);
    image[i].b = cacol(CO_DFT,DEFAULT_BACK_COLOR);
    image[i].r = DEFAULT_RENDITION;
  }
}

// Create Image ///////////////////////////////////////////////////////

void TEWidget::calcGeometry()
{
  scrollbar->resize(TQApplication::style().pixelMetric(TQStyle::PM_ScrollBarExtent),
                    contentsRect().height());
  switch(scrollLoc)
  {
    case SCRNONE :
     bX = rimX;
     contentWidth = contentsRect().width() - 2 * rimX;
     scrollbar->hide();
     break;
    case SCRLEFT :
     bX = rimX+scrollbar->width();
     contentWidth = contentsRect().width() - 2 * rimX - scrollbar->width();
     scrollbar->move(contentsRect().topLeft());
     scrollbar->show();
     break;
    case SCRRIGHT:
     bX = rimX;
     contentWidth = contentsRect().width()  - 2 * rimX - scrollbar->width();
     scrollbar->move(contentsRect().topRight() - TQPoint(scrollbar->width()-1,0));
     scrollbar->show();
     break;
  }

  //FIXME: support 'rounding' styles
  bY = rimY;
  contentHeight = contentsRect().height() - 2 * rimY + /* mysterious */ 1;

  if (!isFixedSize)
  {
     columns = contentWidth / font_w;

     if (columns<1) {
       kdDebug(1211) << "TEWidget::calcGeometry: columns=" << columns << endl;
       columns=1;
     }
     lines = contentHeight / font_h;
  }
}

void TEWidget::makeImage()
{
  calcGeometry();
  image_size=lines*columns;
  // We over-commit 1 character so that we can be more relaxed in dealing with
  // certain boundary conditions: image[image_size] is a valid but unused position
  image = (ca*) malloc((image_size+1)*sizeof(ca));
  clearImage();
}

// calculate the needed size
void TEWidget::setSize(int cols, int lins)
{
  int frw = width() - contentsRect().width();
  int frh = height() - contentsRect().height();
  int scw = (scrollLoc==SCRNONE?0:scrollbar->width());
  m_size = TQSize(font_w*cols + 2*rimX + frw + scw, font_h*lins + 2*rimY + frh + /* mysterious */ 1);
  updateGeometry();
}

void TEWidget::setFixedSize(int cols, int lins)
{
  isFixedSize = true;
  columns = cols;
  lines = lins;
  if (image)
  {
     free(image);
     makeImage();
  }
  setSize(cols, lins);
  TQFrame::setFixedSize(m_size);
}

TQSize TEWidget::sizeHint() const
{
  return m_size;
}

void TEWidget::styleChange(TQStyle &)
{
    propagateSize();
}


/* --------------------------------------------------------------------- */
/*                                                                       */
/* Drag & Drop                                                           */
/*                                                                       */
/* --------------------------------------------------------------------- */

void TEWidget::dragEnterEvent(TQDragEnterEvent* e)
{
  e->accept(TQTextDrag::canDecode(e) ||
      KURLDrag::canDecode(e));
}

enum dropPopupOptions { paste, cd, cp, ln, mv };

void TEWidget::dropEvent(TQDropEvent* event)
{
   if (m_drop==0)
   {
      m_drop = new KPopupMenu(this);
      m_drop->insertItem( i18n("Paste"), paste );
      m_drop->insertSeparator();
      m_drop->insertItem( "cd", cd );
      m_drop->insertItem( "cp", cp );
      m_drop->insertItem( "ln", ln );
      m_drop->insertItem( "mv", mv );
      connect(m_drop, TQT_SIGNAL(activated(int)), TQT_SLOT(drop_menu_activated(int)));
   };
    // The current behaviour when url(s) are dropped is
    // * if there is only ONE url and if it's a LOCAL one, ask for paste or cd/cp/ln/mv
    // * if there are only LOCAL urls, ask for paste or cp/ln/mv
    // * in all other cases, just paste
    //   (for non-local ones, or for a list of URLs, 'cd' is nonsense)
  KURL::List urllist;
  m_dnd_file_count = 0;
  dropText = "";
  bool justPaste = true;

  if(KURLDrag::decode(event, urllist)) {
    justPaste =false;
    if (!urllist.isEmpty()) {
      KURL::List::Iterator it;

      m_drop->setItemEnabled( cd, true );
      m_drop->setItemEnabled( ln, true );

      for ( it = urllist.begin(); it != urllist.end(); ++it ) {
        if(m_dnd_file_count++ > 0) {
          dropText += " ";
	  m_drop->setItemEnabled(cd,false);
        }
        KURL url = KIO::NetAccess::mostLocalURL( *it, 0 );
        TQString tmp;
        if (url.isLocalFile()) {
          tmp = url.path(); // local URL : remove protocol. This helps "ln" & "cd" and doesn't harm the others
        } else if ( url.protocol() == TQString::fromLatin1( "mailto" ) ) {
	  justPaste = true;
	  break;
	} else {
          tmp = url.url();
	  m_drop->setItemEnabled( cd, false );
	  m_drop->setItemEnabled( ln, false );
        }
        if (urllist.count()>1)
          KRun::shellQuote(tmp);
        dropText += tmp;
      }

      if (!justPaste) m_drop->popup(mapToGlobal(event->pos()));
    }
  }
  if(justPaste && TQTextDrag::decode(event, dropText)) {
    kdDebug(1211) << "Drop:" << dropText.local8Bit() << "\n";
    emit sendStringToEmu(dropText.local8Bit());
    // Paste it
  }
}

void TEWidget::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new TQTextDrag(TQApplication::clipboard()->text(TQClipboard::Selection), this);
  dragInfo.dragObject->dragCopy();
  // Don't delete the TQTextDrag object.  Qt will delete it when it's done with it.
}

void TEWidget::drop_menu_activated(int item)
{
   switch (item)
  {
   case paste:
      if (m_dnd_file_count==1)
        KRun::shellQuote(dropText);
      emit sendStringToEmu(dropText.local8Bit());
      setActiveWindow();
      break;
   case cd:
     emit sendStringToEmu("cd ");
      struct stat statbuf;
      if ( ::stat( TQFile::encodeName( dropText ), &statbuf ) == 0 )
      {
         if ( !S_ISDIR(statbuf.st_mode) )
         {
            KURL url;
            url.setPath( dropText );
            dropText = url.directory( true, false ); // remove filename
         }
      }
      KRun::shellQuote(dropText);
      emit sendStringToEmu(dropText.local8Bit());
      emit sendStringToEmu("\n");
      setActiveWindow();
      break;
   case cp:
     emit sendStringToEmu("kfmclient copy " );
     break;
   case ln:
     emit sendStringToEmu("ln -s ");
     break;
   case mv:
     emit sendStringToEmu("kfmclient move " );
     break;
   }
   if (item>cd && item<=mv) {
      if (m_dnd_file_count==1)
        KRun::shellQuote(dropText);
      emit sendStringToEmu(dropText.local8Bit());
      emit sendStringToEmu(" .\n");
      setActiveWindow();
   }
}

uint TEWidget::lineSpacing() const
{
  return m_lineSpacing;
}

void TEWidget::setLineSpacing(uint i)
{
  m_lineSpacing = i;
  setVTFont(font()); // Trigger an update.
}

#include "TEWidget.moc"
