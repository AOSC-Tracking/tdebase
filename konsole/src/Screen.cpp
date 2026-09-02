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

/*! \class Screen

    \brief The image manipulated by the emulation.

    This class implements the operations of the terminal emulation framework.
    It is a complete passive device, driven by the emulation decoder
    (TEmuVT102). By this it forms in fact an ADT, that defines operations
    on a rectangular image.

    It does neither know how to display its image nor about escape sequences.
    It is further independent of the underlying toolkit. By this, one can even
    use this module for an ordinary text surface.

    Since the operations are called by a specific emulation decoder, one may
    collect their different operations here.

    The state manipulated by the operations is mainly kept in `image', though
    it is a little more complex beyond this. See the header file of the class.

    \sa TEWidget \sa VT102Emulation
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <kdebug.h>

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "konsole_wcwidth.h"
#include "TEHistory.h"
#include "Screen.h"

//FIXME: this is emulation specific. Use false for xterm, true for ANSI.
//FIXME: see if we can get this from terminfo.
#define BS_CLEARS false

#ifndef loc
#define loc(X,Y) ((Y)*m_columns+(X))
#endif

//#define REVERSE_WRAPPED_LINES  // for wrapped line debug

namespace Konsole
{

/*! creates a `Screen' of `lines' lines and `columns' columns.
*/

Screen::Screen(int l, int c)
  : m_lines(l), m_columns(c),
    m_image(new Character[(m_lines+1)*m_columns]),
    m_histCursor(0), m_hist(new HistoryScrollNone()),
    m_cursorX(0), m_cursorY(0),
    m_cursorFg(CharacterColor()), m_cursorBg(CharacterColor()), m_cursorRend(0),
    m_topMargin(0), m_bottomMargin(0),
    m_tabStops(0),
    m_selBegin(0), m_selTopLeft(0), m_selBottomRight(0), m_selBusy(false),
    m_blockSelectionMode(false),
    m_effectiveFg(CharacterColor()), m_effectiveBg(CharacterColor()), m_effectiveRend(0),
    m_savedCursorX(0), m_savedCursorY(0),
    m_savedCursorFg(CharacterColor()), m_savedCursorBg(CharacterColor()), m_savedCursorRend(0),
    m_lastPos(-1),
    m_lastDrawnChar(0)
{
  m_line_wrapped.resize(m_lines+1);
  initTabStops();
  clearSelection();
  reset();
}

/*! Destructor
*/

Screen::~Screen()
{
  delete[] m_image;
  delete[] m_tabStops;
  delete   m_hist;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Normalized                    Screen Operations                           */
/*                                                                           */
/* ------------------------------------------------------------------------- */

// Cursor Setting --------------------------------------------------------------

/*! \section Cursor

    The `cursor' is a location within the screen that is implicitely used in
    many operations. The operations within this section allow to manipulate
    the cursor explicitly and to obtain it's value.

    The position of the cursor is guarantied to be between (including) 0 and
    `columns-1' and `lines-1'.
*/

/*!
    Move the cursor up.

    The cursor will not be moved beyond the top margin.
*/

void Screen::cursorUp(int n)
//=CUU
{
  if (n == 0) n = 1; // Default
  int stop = m_cursorY < m_topMargin ? 0 : m_topMargin;
  m_cursorX = TQMIN(m_columns-1,m_cursorX); // nowrap!
  m_cursorY = TQMAX(stop,m_cursorY-n);
}

/*!
    Move the cursor down.

    The cursor will not be moved beyond the bottom margin.
*/

void Screen::cursorDown(int n)
//=CUD
{
  if (n == 0) n = 1; // Default
  int stop = m_cursorY > m_bottomMargin ? m_lines-1 : m_bottomMargin;
  m_cursorX = TQMIN(m_columns-1,m_cursorX); // nowrap!
  m_cursorY = TQMIN(stop,m_cursorY+n);
}

/*!
    Move the cursor left.

    The cursor will not move beyond the first column.
*/

void Screen::cursorLeft(int n)
//=CUB
{
  if (n == 0) n = 1; // Default
  m_cursorX = TQMIN(m_columns-1,m_cursorX); // nowrap!
  m_cursorX = TQMAX(0,m_cursorX-n);
}

/*!
    Move the cursor right.

    The cursor will not move beyond the rightmost column.
*/

void Screen::cursorRight(int n)
//=CUF
{
  if (n == 0) n = 1; // Default
  m_cursorX = TQMIN(m_columns-1,m_cursorX+n);
}

/*!
    Move the cursor at most n lines next
*/

void Screen::cursorNextLine(int n)
//=CNL
{
	if (n == 0)
	{
		n = 1; // Default
	}
	m_cursorX = 0;
	while (n > 0)
	{
		if (m_cursorY < m_lines - 1)
		{
			m_cursorY += 1;
		}
		n--;
	}
}

/*!
    Move the cursor at most n lines previous
*/

void Screen::cursorPrevLine(int n)
//=CPL
{
	if (n == 0)
	{
		n = 1; // Default
	}
	m_cursorX = 0;
	while (n > 0)
	{
		if (m_cursorY  > 0)
		{
			m_cursorY -= 1;
		}
		n--;
	}
}

/*!
    Set top and bottom margin.
*/

void Screen::setMargins(int top, int bot)
//=STBM
{
  if (top == 0) top = 1;      // Default
  if (bot == 0) bot = m_lines;  // Default
  top = top - 1;              // Adjust to internal lineno
  bot = bot - 1;              // Adjust to internal lineno
  if ( !( 0 <= top && top < bot && bot < m_lines ) )
  { kdDebug()<<" setRegion("<<top<<","<<bot<<") : bad range."<<endl;
    return;                   // Default error action: ignore
  }
  m_topMargin = top;
  m_bottomMargin = bot;
  m_cursorX = 0;
  m_cursorY = getMode(MODE_Origin) ? top : 0;
}

/*!
    Move the cursor down one line.

    If cursor is on bottom margin, the region between the
    actual top and bottom margin is scrolled up instead.
*/

void Screen::index()
//=IND
{
  if (m_cursorY == m_bottomMargin)
  {
    scrollUp(1);
  }
  else if (m_cursorY < m_lines-1)
    m_cursorY += 1;
}

/*!
    Move the cursor up one line.

    If cursor is on the top margin, the region between the
    actual top and bottom margin is scrolled down instead.
*/

void Screen::reverseIndex()
//=RI
{
  if (m_cursorY == m_topMargin)
     scrollDown(m_topMargin,1);
  else if (m_cursorY > 0)
    m_cursorY -= 1;
}

/*!
    Move the cursor to the begin of the next line.

    If cursor is on bottom margin, the region between the
    actual top and bottom margin is scrolled up.
*/

void Screen::nextLine()
//=NEL
{
  toStartOfLine(); index();
}

// Line Editing ----------------------------------------------------------------

/*! \section inserting / deleting characters
*/

/*! erase `n' characters starting from (including) the cursor position.

    The line is filled in from the right with spaces.
*/

void Screen::eraseChars(int n)
{
  if (n == 0) n = 1; // Default
  int p = TQMAX(0,TQMIN(m_cursorX+n-1,m_columns-1));
  clearImage(loc(m_cursorX,m_cursorY),loc(p,m_cursorY),' ');
}

/*! delete `n' characters starting from (including) the cursor position.

    The line is filled in from the right with spaces.
*/

void Screen::deleteChars(int n)
{
  if (n == 0) n = 1; // Default
  if (n > m_columns) n = m_columns - 1;
  int p = TQMAX(0,TQMIN(m_cursorX+n,m_columns-1));
  moveImage(loc(m_cursorX,m_cursorY),loc(p,m_cursorY),loc(m_columns-1,m_cursorY));
  clearImage(loc(m_columns-n,m_cursorY),loc(m_columns-1,m_cursorY),' ');
}

/*! insert `n' spaces at the cursor position.

    The cursor is not moved by the operation.
*/

void Screen::insertChars(int n)
{
  if (n == 0) n = 1; // Default
  int p = TQMAX(0,TQMIN(m_columns-1-n,m_columns-1));
  int q = TQMAX(0,TQMIN(m_cursorX+n,m_columns-1));
  moveImage(loc(q,m_cursorY),loc(m_cursorX,m_cursorY),loc(p,m_cursorY));
  clearImage(loc(m_cursorX,m_cursorY),loc(q-1,m_cursorY),' ');
}

void Screen::repeatChars(int n)
{
    if (n == 0)
    {
        n = 1; // Default
    }

    // From ECMA-48 version 5, section 8.3.103:
    // "If the character preceding REP is a control function or part of a
    // control function, the effect of REP is not defined by this Standard."
    //
    // So, a "normal" program should always use REP immediately after a visible
    // character (those other than escape sequences). So, m_lastDrawnChar can be
    // safely used.
    for (int i = 0; i < n; i++)
    {
        displayCharacter(m_lastDrawnChar);
    }
}

/*! delete `n' lines starting from (including) the cursor position.

    The cursor is not moved by the operation.
*/

void Screen::deleteLines(int n)
{
  if (n == 0) n = 1; // Default
  scrollUp(m_cursorY,n);
}

/*! insert `n' lines at the cursor position.

    The cursor is not moved by the operation.
*/

void Screen::insertLines(int n)
{
  if (n == 0) n = 1; // Default
  scrollDown(m_cursorY,n);
}

// Mode Operations -----------------------------------------------------------

/*! Set a specific mode. */

void Screen::setMode(int m)
{
  m_currParm.mode[m] = true;
  switch(m)
  {
    case MODE_Origin : m_cursorX = 0; m_cursorY = m_topMargin; break; //FIXME: home
  }
}

/*! Reset a specific mode. */

void Screen::resetMode(int m)
{
  m_currParm.mode[m] = false;
  switch(m)
  {
    case MODE_Origin : m_cursorX = 0; m_cursorY = 0; break; //FIXME: home
  }
}

/*! Save a specific mode. */

void Screen::saveMode(int m)
{
  m_saveParm.mode[m] = m_currParm.mode[m];
}

/*! Restore a specific mode. */

void Screen::restoreMode(int m)
{
  m_currParm.mode[m] = m_saveParm.mode[m];
}

//NOTE: this is a helper function
/*! Return the setting  a specific mode. */
bool Screen::getMode(int m)
{
  return m_currParm.mode[m];
}

/*! Save the cursor position and the rendition attribute settings. */

void Screen::saveCursor()
{
  m_savedCursorX    = m_cursorX;
  m_savedCursorY    = m_cursorY;
  m_savedCursorFg   = m_cursorFg;
  m_savedCursorBg   = m_cursorBg;
  m_savedCursorRend = m_cursorRend;
}

/*! Restore the cursor position and the rendition attribute settings. */

void Screen::restoreCursor()
{
  m_cursorX    = TQMIN(m_savedCursorX,m_columns-1);
  m_cursorY    = TQMIN(m_savedCursorY,m_lines-1);
  m_cursorFg   = m_savedCursorFg;
  m_cursorBg   = m_savedCursorBg;
  m_cursorRend = m_savedCursorRend;
  updateEffectiveRendition();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                             Screen Operations                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*! Assing a new size to the screen.

    The topmost left position is maintained, while lower lines
    or right hand side columns might be removed or filled with
    spaces to fit the new size.

    The region setting is reset to the whole screen and the
    tab positions reinitialized.
*/

void Screen::resizeImage(int new_lines, int new_columns)
{
  if ((new_lines==m_lines) && (new_columns==m_columns)) return;

  if (m_cursorY > new_lines-1)
  { // attempt to preserve focus and lines
    m_bottomMargin = m_lines-1; //FIXME: margin lost
    for (int i = 0; i < m_cursorY-(new_lines-1); i++)
    {
      addHistLine(); scrollUp(0,1);
    }
  }

  // make new image

  Character* newimg = new Character[(new_lines+1)*new_columns];
  TQBitArray newwrapped(new_lines+1);
  clearSelection();

  // clear new image
  for (int y = 0; y < new_lines; y++) {
    for (int x = 0; x < new_columns; x++)
    {
      newimg[y*new_columns+x].m_character = ' ';
      newimg[y*new_columns+x].m_fgColor = CharacterColor(COLOR_SPACE_DEFAULT,DEFAULT_FORE_COLOR);
      newimg[y*new_columns+x].m_bgColor = CharacterColor(COLOR_SPACE_DEFAULT,DEFAULT_BACK_COLOR);
      newimg[y*new_columns+x].m_rendition = DEFAULT_RENDITION;
    }
    newwrapped[y]=false;
  }
  int cpy_lines   = TQMIN(new_lines,  m_lines);
  int cpy_columns = TQMIN(new_columns,m_columns);
  // copy to new image
  for (int y = 0; y < cpy_lines; y++) {
    for (int x = 0; x < cpy_columns; x++)
    {
      newimg[y*new_columns+x].m_character = m_image[loc(x,y)].m_character;
      newimg[y*new_columns+x].m_fgColor = m_image[loc(x,y)].m_fgColor;
      newimg[y*new_columns+x].m_bgColor = m_image[loc(x,y)].m_bgColor;
      newimg[y*new_columns+x].m_rendition = m_image[loc(x,y)].m_rendition;
    }
    newwrapped[y]=m_line_wrapped[y];
  }
  delete[] m_image;
  m_image = newimg;
  m_line_wrapped = newwrapped;
  m_lines = new_lines;
  m_columns = new_columns;
  m_cursorX = TQMIN(m_cursorX,m_columns-1);
  m_cursorY = TQMIN(m_cursorY,m_lines-1);

  // FIXME: try to keep values, evtl.
  m_topMargin=0;
  m_bottomMargin=m_lines-1;
  initTabStops();
  clearSelection();
}

/*
   Clarifying rendition here and in TEWidget.

   currently, TEWidget's color table is
     0       1       2 .. 9    10 .. 17
     dft_fg, dft_bg, dim 0..7, intensive 0..7

   m_cursorFg, m_cursorBg contain values 0..8;
   - 0    = default color
   - 1..8 = ansi specified color

   rendition attributes are

      attr           widget screen
      -------------- ------ ------
      RE_UNDERLINE     XX     XX    affects foreground only
      RE_BLINK         XX     XX    affects foreground only
      RE_BOLD          XX     XX    affects foreground only
      RE_REVERSE       --     XX
      RE_TRANSPARENT   XX     --    affects background only
      RE_INTENSIVE     XX     --    affects foreground only

   Note that RE_BOLD is used in both widget
   and screen rendition. Since xterm/vt102
   is to poor to distinguish between bold
   (which is a font attribute) and intensive
   (which is a color attribute), we translate
   this and RE_BOLD in falls eventually appart
   into RE_BOLD and RE_INTENSIVE.
*/

void Screen::reverseRendition(Character* p)
{ CharacterColor f = p->m_fgColor; CharacterColor b = p->m_bgColor;
  p->m_fgColor = b; p->m_bgColor = f; //p->r &= ~RE_TRANSPARENT;
}

void Screen::updateEffectiveRendition()
// calculate rendition
{
  m_effectiveRend = m_cursorRend & (RE_UNDERLINE | RE_BLINK);
  if (m_cursorRend & RE_REVERSE)
  {
    m_effectiveFg = m_cursorBg;
    m_effectiveBg = m_cursorFg;
  }
  else
  {
    m_effectiveFg = m_cursorFg;
    m_effectiveBg = m_cursorBg;
  }
  if (m_cursorRend & RE_BOLD)
    m_effectiveFg.setIntensive();
}

/*!
    returns the image.

    Get the size of the image by \sa getLines and \sa getColumns.

    NOTE that the image returned by this function must later be
    freed.

*/

Character* Screen::getCookedImage()
{
/*kdDebug() << "m_selBegin=" << m_selBegin << "(" << m_selBegin/m_columns << "," << m_selBegin%m_columns << ")"
  << "  m_selTopLeft=" << m_selTopLeft << "(" << m_selTopLeft/m_columns << "," << m_selTopLeft%m_columns << ")"
  << "  m_selBottomRight=" << m_selBottomRight << "(" << m_selBottomRight/m_columns << "," << m_selBottomRight%m_columns << ")"
  << "  m_histCursor=" << m_histCursor << endl;*/

  int x,y;
  Character* merged = (Character*)malloc((m_lines*m_columns+1)*sizeof(Character));
  Character dft(' ',CharacterColor(COLOR_SPACE_DEFAULT,DEFAULT_FORE_COLOR),CharacterColor(COLOR_SPACE_DEFAULT,DEFAULT_BACK_COLOR),DEFAULT_RENDITION);
  merged[m_lines*m_columns] = dft;

//  kdDebug(1211) << "InGetCookedImage" << endl;
  for (y = 0; (y < m_lines) && (y < (m_hist->getLines()-m_histCursor)); y++)
  {
    int len = TQMIN(m_columns,m_hist->getLineLen(y+m_histCursor));
    int yp  = y*m_columns;

//    kdDebug(1211) << "InGetCookedImage - In first For.  Y =" << y << "histCursor = " << m_histCursor << endl;
    m_hist->getCells(y+m_histCursor,0,len,merged+yp);
    for (x = len; x < m_columns; x++) merged[yp+x] = dft;
    if (m_selBegin !=-1)
    for (x = 0; x < m_columns; x++)
      {
#ifdef REVERSE_WRAPPED_LINES
        if (m_hist->isWrappedLine(y+m_histCursor))
          reverseRendition(&merged[p]);
#endif
        if (isSelected(x,y)) {
          int p=x + yp;
          reverseRendition(&merged[p]); // for selection
    }
  }
  }
  if (m_lines >= m_hist->getLines()-m_histCursor)
  {
    for (y = (m_hist->getLines()-m_histCursor); y < m_lines ; y++)
    {
       int yp  = y*m_columns;
       int yr =  (y-m_hist->getLines()+m_histCursor)*m_columns;
//       kdDebug(1211) << "InGetCookedImage - In second For.  Y =" << y << endl;
       for (x = 0; x < m_columns; x++)
       { int p = x + yp; int r = x + yr;
         merged[p] = m_image[r];
#ifdef REVERSE_WRAPPED_LINES
         if (m_line_wrapped[y- m_hist->getLines() +m_histCursor])
           reverseRendition(&merged[p]);
#endif
         if (m_selBegin != -1 && isSelected(x,y))
           reverseRendition(&merged[p]); // for selection
       }

    }
  }
  // evtl. inverse display
  if (getMode(MODE_Screen))
  {
    for (int i = 0; i < m_lines*m_columns; i++)
      reverseRendition(&merged[i]); // for reverse display
  }
//  if (getMode(MODE_Cursor) && (m_cursorY+(m_hist->getLines()-m_histCursor) < m_lines)) // cursor visible

  int loc_ = loc(m_cursorX, m_cursorY+m_hist->getLines()-m_histCursor);
  if(getMode(MODE_Cursor) && loc_ < m_columns*m_lines)
    merged[loc(m_cursorX,m_cursorY+(m_hist->getLines()-m_histCursor))].m_rendition|=RE_CURSOR;
  return merged;
}

TQBitArray Screen::getCookedLineWrapped()
{
  TQBitArray result(m_lines);

  for (int y = 0; (y < m_lines) && (y < (m_hist->getLines()-m_histCursor)); y++)
    result[y]=m_hist->isWrappedLine(y+m_histCursor);

  if (m_lines >= m_hist->getLines()-m_histCursor)
    for (int y = (m_hist->getLines()-m_histCursor); y < m_lines ; y++)
      result[y]=m_line_wrapped[y- m_hist->getLines() +m_histCursor];

  return result;
}

/*!
*/

void Screen::reset()
{
    setMode(MODE_Wrap  ); saveMode(MODE_Wrap  );  // wrap at end of margin
  resetMode(MODE_Origin); saveMode(MODE_Origin);  // position refere to [1,1]
  resetMode(MODE_Insert); saveMode(MODE_Insert);  // overstroke
    setMode(MODE_Cursor);                         // cursor visible
  resetMode(MODE_Screen);                         // screen not inverse
  resetMode(MODE_NewLine);

  m_topMargin=0;
  m_bottomMargin=m_lines-1;

  setDefaultRendition();
  saveCursor();

  clear();
}

/*! Clear the entire screen and home the cursor.
*/

void Screen::clear()
{
  clearEntireScreen();
  home();
}

/*! Moves the cursor left one column.
*/

void Screen::backSpace()
{
  m_cursorX = TQMAX(0,m_cursorX-1);
  if (BS_CLEARS) m_image[loc(m_cursorX,m_cursorY)].m_character = ' ';
}

/*!
*/

void Screen::tab(int n)
{
  // note that TAB is a format effector (does not write ' ');
  if (n == 0) n = 1;
  while((n > 0) && (m_cursorX < m_columns-1))
  {
    cursorRight(1); while((m_cursorX < m_columns-1) && !m_tabStops[m_cursorX]) cursorRight(1);
    n--;
  }
}

void Screen::backTab(int n)
{
  // note that TAB is a format effector (does not write ' ');
  if (n == 0) n = 1;
  while((n > 0) && (m_cursorX > 0))
  {
     cursorLeft(1); while((m_cursorX > 0) && !m_tabStops[m_cursorX]) cursorLeft(1);
     n--;
  }
}

void Screen::clearTabStops()
{
  for (int i = 0; i < m_columns; i++) m_tabStops[i] = false;
}

void Screen::changeTabStop(bool set)
{
  if (m_cursorX >= m_columns) return;
  m_tabStops[m_cursorX] = set;
}

void Screen::initTabStops()
{
  delete[] m_tabStops;
  m_tabStops = new bool[m_columns];

  // Arrg! The 1st tabstop has to be one longer than the other.
  // i.e. the kids start counting from 0 instead of 1.
  // Other programs might behave correctly. Be aware.
  for (int i = 0; i < m_columns; i++) m_tabStops[i] = (i%8 == 0 && i != 0);
}

/*!
   This behaves either as IND (Screen::index) or as NEL (Screen::nextLine)
   depending on the NewLine Mode (LNM). This mode also
   affects the key sequence returned for newline ([CR]LF).
*/

void Screen::newLine()
{
  if (getMode(MODE_NewLine)) toStartOfLine();
  index();
}

/*! put `c' literally onto the screen at the current cursor position.

    VT100 uses the convention to produce an automatic newline (am)
    with the *first* character that would fall onto the next line (xenl).
*/

void Screen::checkSelection(int from, int to)
{
  if (m_selBegin == -1) return;
  int scr_TL = loc(0, m_hist->getLines());
  //Clear entire selection if it overlaps region [from, to]
  if ( (m_selBottomRight > (from+scr_TL) )&&(m_selTopLeft < (to+scr_TL)) )
  {
    clearSelection();
  }
}

void Screen::displayCharacter(unsigned short c)
{
  // Note that VT100 does wrapping BEFORE putting the character.
  // This has impact on the assumption of valid cursor positions.
  // We indicate the fact that a newline has to be triggered by
  // putting the cursor one right to the last column of the screen.

  int w = konsole_wcwidth(c);

  if (w <= 0)
     return;

  if (m_cursorX+w > m_columns) {
    if (getMode(MODE_Wrap)) {
      m_line_wrapped[m_cursorY]=true;
      nextLine();
    }
    else
      m_cursorX = m_columns-w;
  }

  if (getMode(MODE_Insert)) insertChars(w);

  int i = loc(m_cursorX,m_cursorY);

  checkSelection(i, i); // check if selection is still valid.

  m_image[i].m_character = c;
  m_image[i].m_fgColor = m_effectiveFg;
  m_image[i].m_bgColor = m_effectiveBg;
  m_image[i].m_rendition = m_effectiveRend;
  
  m_lastPos = i;

  m_lastDrawnChar = c;

  m_cursorX += w--;

  while(w)
  {
     i++;
     m_image[i].m_character = 0;
     m_image[i].m_fgColor = m_effectiveFg;
     m_image[i].m_bgColor = m_effectiveBg;
     m_image[i].m_rendition = m_effectiveRend;
     w--;
  }
}

void Screen::compose(TQString compose)
{
  if (m_lastPos == -1)
     return;
     
  TQChar c(m_image[m_lastPos].m_character);
  compose.prepend(c);
  compose.compose();
  m_image[m_lastPos].m_character = compose[0].unicode();
}

// Region commands -------------------------------------------------------------

void Screen::scrollUp(int n)
{
   if (n == 0) n = 1; // Default
   if (m_topMargin == 0) addHistLine(); // hist.history
   scrollUp(m_topMargin, n);
}

/*! scroll up `n' lines within current region.
    The `n' new lines are cleared.
    \sa setRegion \sa scrollDown
*/

void Screen::scrollUp(int from, int n)
{
	if (n <= 0)
	{
		return;
	}
	if (from > m_bottomMargin)
	{
		return;
	}
	if ((from + n) > m_bottomMargin)
	{
		n = m_bottomMargin + 1 - from;
	}

	//FIXME: make sure `m_topMargin', `m_bottomMargin', `from', `n' is in bounds.
	moveImage(loc(0, from), loc(0, from+n), loc(m_columns, m_bottomMargin));
	clearImage(loc(0, m_bottomMargin-n+1), loc(m_columns-1, m_bottomMargin), ' ');
}

void Screen::scrollDown(int n)
{
   if (n == 0) n = 1; // Default
   scrollDown(m_topMargin, n);
}

/*! scroll down `n' lines within current region.
    The `n' new lines are cleared.
    \sa setRegion \sa scrollUp
*/

void Screen::scrollDown(int from, int n)
{
//FIXME: make sure `m_topMargin', `m_bottomMargin', `from', `n' is in bounds.
  if (n <= 0) return;
  if (from > m_bottomMargin) return;
  if (from + n > m_bottomMargin) n = m_bottomMargin - from;
  moveImage(loc(0,from+n),loc(0,from),loc(m_columns-1,m_bottomMargin-n));
  clearImage(loc(0,from),loc(m_columns-1,from+n-1),' ');
}

/*! position the cursor to a specific line and column. */
void Screen::setCursorYX(int y, int x)
{
  setCursorY(y); setCursorX(x);
}

/*! Set the cursor to x-th line. */

void Screen::setCursorX(int x)
{
  if (x == 0) x = 1; // Default
  x -= 1; // Adjust
  m_cursorX = TQMAX(0,TQMIN(m_columns-1, x));
}

/*! Set the cursor to y-th line. */

void Screen::setCursorY(int y)
{
  if (y == 0) y = 1; // Default
  y -= 1; // Adjust
  m_cursorY = TQMAX(0,TQMIN(m_lines  -1, y + (getMode(MODE_Origin) ? m_topMargin : 0) ));
}

/*! set cursor to the `left upper' corner of the screen (1,1).
*/

void Screen::home()
{
  m_cursorX = 0;
  m_cursorY = 0;
}

/*! set cursor to the begin of the current line.
*/

void Screen::toStartOfLine()
{
  m_cursorX = 0;
}

/*! returns the current cursor columns.
*/

int Screen::getCursorX()
{
  return m_cursorX;
}

/*! returns the current cursor line.
*/

int Screen::getCursorY()
{
  return m_cursorY;
}

// Erasing ---------------------------------------------------------------------

/*! \section Erasing

    This group of operations erase parts of the screen contents by filling
    it with spaces colored due to the current rendition settings.

    Althought the cursor position is involved in most of these operations,
    it is never modified by them.
*/

/*! fill screen between (including) `loca' and `loce' with spaces.

    This is an internal helper functions. The parameter types are internal
    addresses of within the screen image and make use of the way how the
    screen matrix is mapped to the image vector.
*/

void Screen::clearImage(int loca, int loce, char c)
{ int i;
  int scr_TL=loc(0,m_hist->getLines());
  //FIXME: check positions

  //Clear entire selection if it overlaps region to be moved...
  if ( (m_selBottomRight > (loca+scr_TL) )&&(m_selTopLeft < (loce+scr_TL)) )
  {
    clearSelection();
  }
  
  for (i = loca; i <= loce; i++)
  {
    // Use the current colors but the default rendition
    // Check with: echo -e '\033[41;33;07m\033[2Khello world\033[00m'
    m_image[i].m_character = c;
    m_image[i].m_fgColor = m_cursorFg;
    m_image[i].m_bgColor = m_cursorBg;
    m_image[i].m_rendition = DEFAULT_RENDITION;
  }

  for (i = loca/m_columns; i<=loce/m_columns; i++)
    m_line_wrapped[i]=false;
}

/*! move image between (including) `loca' and `loce' to 'dst'.

    This is an internal helper functions. The parameter types are internal
    addresses of within the screen image and make use of the way how the
    screen matrix is mapped to the image vector.
*/

void Screen::moveImage(int dst, int loca, int loce)
{
//FIXME: check positions
  if (loce < loca) {
    kdDebug(1211) << "WARNING!!! call to Screen:moveImage with loce < loca!" << endl;
    return;
  }
  //kdDebug(1211) << "Using memmove to scroll up" << endl;
  memmove(&m_image[dst],&m_image[loca],(loce-loca+1)*sizeof(Character));
  for (int i=0;i<=(loce-loca+1)/m_columns;i++)
    m_line_wrapped[(dst/m_columns)+i]=m_line_wrapped[(loca/m_columns)+i];
  if (m_lastPos != -1)
  {
     int diff = dst - loca; // Scroll by this amount
     m_lastPos += diff;
     if ((m_lastPos < 0) || (m_lastPos >= (m_lines*m_columns)))
        m_lastPos = -1;
  }
  if (m_selBegin != -1)
  {
     // Adjust selection to follow scroll.
     bool beginIsTL = (m_selBegin == m_selTopLeft);
     int diff = dst - loca; // Scroll by this amount
     int scr_TL=loc(0,m_hist->getLines());
     int srca = loca+scr_TL; // Translate index from screen to global
     int srce = loce+scr_TL; // Translate index from screen to global
     int desta = srca+diff;
     int deste = srce+diff;

     if ((m_selTopLeft >= srca) && (m_selTopLeft <= srce))
        m_selTopLeft += diff;
     else if ((m_selTopLeft >= desta) && (m_selTopLeft <= deste))
        m_selBottomRight = -1; // Clear selection (see below)

     if ((m_selBottomRight >= srca) && (m_selBottomRight <= srce))
        m_selBottomRight += diff;
     else if ((m_selBottomRight >= desta) && (m_selBottomRight <= deste))
        m_selBottomRight = -1; // Clear selection (see below)

     if (m_selBottomRight < 0)
     {
        clearSelection();
     }
     else
     {
        if (m_selTopLeft < 0)
           m_selTopLeft = 0;
     }

     if (beginIsTL)
        m_selBegin = m_selTopLeft;
     else
        m_selBegin = m_selBottomRight;
  }
}

/*! clear from (including) current cursor position to end of screen.
*/

void Screen::clearToEndOfScreen()
{
  clearImage(loc(m_cursorX,m_cursorY),loc(m_columns-1,m_lines-1),' ');
}

/*! clear from begin of screen to (including) current cursor position.
*/

void Screen::clearToBeginOfScreen()
{
  clearImage(loc(0,0),loc(m_cursorX,m_cursorY),' ');
}

/*! clear the entire screen.
*/

void Screen::clearEntireScreen()
{
  clearImage(loc(0,0),loc(m_columns-1,m_lines-1),' ');
}

/*! fill screen with 'E'
    This is to aid screen alignment
*/

void Screen::helpAlign()
{
  clearImage(loc(0,0),loc(m_columns-1,m_lines-1),'E');
}

/*! clear from (including) current cursor position to end of current cursor line.
*/

void Screen::clearToEndOfLine()
{
  clearImage(loc(m_cursorX,m_cursorY),loc(m_columns-1,m_cursorY),' ');
}

/*! clear from begin of current cursor line to (including) current cursor position.
*/

void Screen::clearToBeginOfLine()
{
  clearImage(loc(0,m_cursorY),loc(m_cursorX,m_cursorY),' ');
}

/*! clears entire current cursor line
*/

void Screen::clearEntireLine()
{
  clearImage(loc(0,m_cursorY),loc(m_columns-1,m_cursorY),' ');
}

// Rendition ------------------------------------------------------------------

/*!
    set rendition mode
*/

void Screen::setRendition(int re)
{
  m_cursorRend |= re;
  updateEffectiveRendition();
}

/*!
    reset rendition mode
*/

void Screen::resetRendition(int re)
{
  m_cursorRend &= ~re;
  updateEffectiveRendition();
}

/*!
*/

void Screen::setDefaultRendition()
{
  setForeColor(COLOR_SPACE_DEFAULT,DEFAULT_FORE_COLOR);
  setBackColor(COLOR_SPACE_DEFAULT,DEFAULT_BACK_COLOR);
  m_cursorRend   = DEFAULT_RENDITION;
  updateEffectiveRendition();
}

/*!
*/
void Screen::setForeColor(int space, int color)
{
  m_cursorFg = CharacterColor(space, color);
  updateEffectiveRendition();
}

/*!
*/
void Screen::setBackColor(int space, int color)
{
  m_cursorBg = CharacterColor(space, color);
  updateEffectiveRendition();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                            Marking & Selection                            */
/*                                                                           */
/* ------------------------------------------------------------------------- */

void Screen::clearSelection()
{
  m_selBottomRight = -1;
  m_selTopLeft = -1;
  m_selBegin = -1;
}

void Screen::selectAll()
{
  m_selBegin = 0;
  m_selTopLeft = 0;
  m_selBottomRight = loc(m_columns - 1, m_hist->getLines() + m_lines - 1);
  m_blockSelectionMode = false;
}

void Screen::setSelectionStart(const int x, const int y, const bool mode)
{
//  kdDebug(1211) << "setSelectionStart(" << x << "," << y << ")" << endl;
  m_selBegin = loc(x,y+m_histCursor) ;

  /* FIXME, HACK to correct for x too far to the right... */
  if (x == m_columns) m_selBegin--;

  m_selBottomRight = m_selBegin;
  m_selTopLeft = m_selBegin;
  m_blockSelectionMode = mode;
}

void Screen::setSelectionEnd(const int x, const int y)
{
//  kdDebug(1211) << "setSelectionEnd(" << x << "," << y << ")" << endl;
  if (m_selBegin == -1) return;
  int l =  loc(x,y + m_histCursor);

  if (l < m_selBegin)
  {
    m_selTopLeft = l;
    m_selBottomRight = m_selBegin;
  }
  else
  {
    /* FIXME, HACK to correct for x too far to the right... */
    if (x == m_columns) l--;

    m_selTopLeft = m_selBegin;
    m_selBottomRight = l;
  }
}

bool Screen::isSelected(const int x,const int y)
{
  if (m_blockSelectionMode) {
    int sel_Left,sel_Right;
    if ( m_selTopLeft % m_columns < m_selBottomRight % m_columns ) {
      sel_Left = m_selTopLeft; sel_Right = m_selBottomRight;
    } else {
      sel_Left = m_selBottomRight; sel_Right = m_selTopLeft;
    }
    return ( x >= sel_Left % m_columns ) && ( x <= sel_Right % m_columns ) &&
           ( y+m_histCursor >= m_selTopLeft / m_columns ) && ( y+m_histCursor <= m_selBottomRight / m_columns );
  }
  else {
  int pos = loc(x,y+m_histCursor);
  return ( pos >= m_selTopLeft && pos <= m_selBottomRight );
  }
}

static bool isSpace(uint16_t c)
{
  if ((c > 32) && (c < 127))
     return false;
  if ((c == 32) || (c == 0))
     return true;
  TQChar qc(c);
  return qc.isSpace();
}

TQString Screen::selectedText(bool preserve_line_breaks)
{
  TQString result;
  TQTextOStream stream(&result);
  selectedText(preserve_line_breaks, &stream);
  return result;
}


static TQString makeString(int *m, int d, bool stripTrailingSpaces)
{
  TQChar* qc = new TQChar[d];

  int last_space = -1;
  int j = 0;

  for (int i = 0; i < d; i++, j++)
    {
      if (m[i] == ' ')
        {
          if (last_space == -1)
            last_space = j;
        }
      else
        {
          last_space = -1;
        }
      qc[j] = m[i];
    }

  if ((last_space != -1) && stripTrailingSpaces)
    {
      // Strip trailing spaces
      j = last_space;
    }

  TQString res(qc, j);
  delete [] qc;
  return res;
}

void Screen::selectedText(bool preserve_line_breaks, TQTextStream *stream)
{
  if (m_selBegin == -1)
     return; // Selection got clear while selecting.

  int *m;			// buffer to fill.
  int s, d;			// source index, dest. index.
  int hist_BR = loc(0, m_hist->getLines());
  int hY = m_selTopLeft / m_columns;
  int hX = m_selTopLeft % m_columns;
  int eol;			// end of line

  s = m_selTopLeft;			// tracks copy in source.

				// allocate buffer for maximum
				// possible size...
  d = (m_selBottomRight - m_selTopLeft) / m_columns + 1;
  m = new int[m_columns + 3];
  d = 0;

#define LINE_END	do { \
                          assert(d <= m_columns); \
                          *stream << makeString(m, d, true) << (preserve_line_breaks ? "\n" : " "); \
                          d = 0; \
                        } while(false)
#define LINE_WRAP	do { \
                          assert(d <= m_columns); \
                          *stream << makeString(m, d, false); \
                          d = 0; \
                        } while(false)
#define LINE_FLUSH	do { \
                          assert(d <= m_columns); \
                          *stream << makeString(m, d, false); \
                          d = 0; \
                        } while(false)

  if (m_blockSelectionMode) {
    bool newlineneeded=false;
    preserve_line_breaks = true; // Just in case

    int sel_Left, sel_Right;
    if ( m_selTopLeft % m_columns < m_selBottomRight % m_columns ) {
      sel_Left = m_selTopLeft; sel_Right = m_selBottomRight;
    } else {
      sel_Left = m_selBottomRight; sel_Right = m_selTopLeft;
    }

    while (s <= m_selBottomRight) {
      if (s < hist_BR) {		// get lines from hist->history buffer.
          hX = sel_Left % m_columns;
	  eol = m_hist->getLineLen(hY);
          if (eol > m_columns)
              eol = m_columns;
	  if ((hY == (m_selBottomRight / m_columns)) &&
              (eol > (m_selBottomRight % m_columns)))
          {
              eol = m_selBottomRight % m_columns + 1;
          }

	  while (hX < eol && hX <= sel_Right % m_columns)
          {
            TQ_UINT16 c = m_hist->getCell(hY, hX++).m_character;
            if (c)
              m[d++] = c;
            s++;
          }
          LINE_END;

          hY++;
          s = hY * m_columns;
      }
      else {				// or from screen image.
        if (isSelected((s - hist_BR) % m_columns, (s - hist_BR) / m_columns)) {
          TQ_UINT16 c = m_image[s++ - hist_BR].m_character;
          if (c) {
            m[d++] = c;
            newlineneeded = true;
	  }
	  if (((s - hist_BR) % m_columns == 0) && newlineneeded)
	  {
            LINE_END;
	    newlineneeded = false;
	  }
	}
	else {
	  s++;
	  if (newlineneeded) {
            LINE_END;
	    newlineneeded = false;
	  }
	}
      }
    }
    if (newlineneeded)
      LINE_END;
  }
  else
  {
  while (s <= m_selBottomRight)
  {
      if (s < hist_BR)
      {				// get lines from hist->history buffer.
          eol = m_hist->getLineLen(hY);
          if (eol > m_columns)
              eol = m_columns;
          
          if ((hY == (m_selBottomRight / m_columns)) &&
              (eol > (m_selBottomRight % m_columns)))
          {
              eol = m_selBottomRight % m_columns + 1;
          }

          while (hX < eol)
          {
              TQ_UINT16 c = m_hist->getCell(hY, hX++).m_character;
              if (c)
                 m[d++] = c;
              s++;
          }

          if (s <= m_selBottomRight)
          {			// The line break handling
              bool wrap = false;
              if (eol % m_columns == 0)
              { 	        // That's either a full or empty line
                  if ((eol != 0) && m_hist->isWrappedLine(hY))
                     wrap = true;
              }
              else if ((eol + 1) % m_columns == 0)
              {
                  if (m_hist->isWrappedLine(hY))
                     wrap = true;
              }

              if (wrap)
              {
                  LINE_WRAP;
              }
              else
              {
                  LINE_END;
              }
                 
          }
          else
          {
              // Flush trailing stuff
              LINE_FLUSH;
          }

          hY++;
          hX = 0;
          s = hY * m_columns;
      }
      else
      {				// or from screen image.
        eol = (s / m_columns + 1) * m_columns - 1;

        bool addNewLine = false;

        if (eol < m_selBottomRight)
        {
            while ((eol > s) &&
                   (!m_image[eol - hist_BR].m_character || isSpace(m_image[eol - hist_BR].m_character)) &&
                   !m_line_wrapped[(eol-hist_BR)/m_columns])
            {
                eol--;
            }
        }
        else if (eol == m_selBottomRight)
        {
            if (!m_line_wrapped[(eol - hist_BR)/m_columns])
	        addNewLine = true;
        }
        else
        {
            eol = m_selBottomRight;
        }

        while (s <= eol)
        {
            TQ_UINT16 c = m_image[s++ - hist_BR].m_character;
            if (c)
                 m[d++] = c;
        }

        if (eol < m_selBottomRight)
        {			// eol processing
            bool wrap = false;
            if ((eol + 1) % m_columns == 0)
            {			// the whole line is filled
                if (m_line_wrapped[(eol - hist_BR)/m_columns])
                    wrap = true;
            }
            if (wrap)
            {
                LINE_WRAP;
            }
            else
            {
                LINE_END;
            }
        }
        else
        {
            // Flush trailing stuff
            if (addNewLine && preserve_line_breaks)
            {
               LINE_END;
            }
            else
            {
               LINE_FLUSH;
            }
        }

        s = (eol / m_columns + 1) * m_columns;
      }
    }
  }

  assert(d == 0);

  delete [] m;
}

void Screen::streamHistory(TQTextStream* stream) {
  m_selBegin = 0;
  m_selBottomRight = m_selBegin;
  m_selTopLeft = m_selBegin;
  setSelectionEnd(m_columns-1,m_lines-1+m_hist->getLines()-m_histCursor);
  selectedText(true, stream);
  clearSelection();
}

TQString Screen::getHistoryLine(int no)
{
  m_selBegin = loc(0,no);
  m_selTopLeft = m_selBegin;
  m_selBottomRight = loc(m_columns-1,no);
  return selectedText(false);
}

void Screen::addHistLine()
{
  assert(hasScroll() || m_histCursor == 0);

  // add to hist buffer
  // we have to take care about scrolling, too...

  if (hasScroll())
  { Character dft;

    int end = m_columns-1;
    while (end >= 0 && m_image[end] == dft && !m_line_wrapped[0])
      end -= 1;

    int oldHistLines = m_hist->getLines();

    m_hist->addCells(m_image,end+1);
    m_hist->addLine(m_line_wrapped[0]);

    int newHistLines = m_hist->getLines();

    bool beginIsTL = (m_selBegin == m_selTopLeft);

    // adjust history cursor
    if (newHistLines > oldHistLines)
    {
       m_histCursor++;
       // Adjust selection for the new point of reference
       if (m_selBegin != -1)
       {
          m_selTopLeft += m_columns;
          m_selBottomRight += m_columns;
       }
    }

    // Scroll up if user is looking at the history and we can scroll up
    if ((m_histCursor > 0) &&  // We can scroll up and...
        ((m_histCursor != newHistLines) || // User is looking at history...
          m_selBusy)) // or user is selecting text.
    {
       m_histCursor--;
    }

    if (m_selBegin != -1)
    {
       // Scroll selection in history up
       int top_BR = loc(0, 1+newHistLines);

       if (m_selTopLeft < top_BR)
          m_selTopLeft -= m_columns;

       if (m_selBottomRight < top_BR)
          m_selBottomRight -= m_columns;

       if (m_selBottomRight < 0)
       {
          clearSelection();
       }
       else
       {
          if (m_selTopLeft < 0)
             m_selTopLeft = 0;
       }

       if (beginIsTL)
          m_selBegin = m_selTopLeft;
       else
          m_selBegin = m_selBottomRight;
    }
  }

  if (!hasScroll()) m_histCursor = 0; //FIXME: a poor workaround
}

void Screen::setHistCursor(int cursor)
{
  m_histCursor = cursor; //FIXME:rangecheck
}

int Screen::getHistCursor()
{
  return m_histCursor;
}

int Screen::getHistLines()
{
  return m_hist->getLines();
}

void Screen::setScroll(const HistoryType& t)
{
  clearSelection();
  m_hist = t.getScroll(m_hist);
  m_histCursor = m_hist->getLines();
}

bool Screen::hasScroll()
{
  return m_hist->hasScroll();
}

const HistoryType& Screen::getScroll()
{
  return m_hist->getType();
}

}
