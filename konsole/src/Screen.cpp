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

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <kdebug.h>

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "konsole_wcwidth.h"
#include "Screen.h"

// Macro to convert x,y position on screen to position within an image.
#ifndef loc
#define loc(X, Y) ((Y) * m_columns + (X))
#endif

static bool isSpace(uint16_t c);
static TQString makeString(const int *m, int d, bool stripTrailingSpaces);

#define LINE_END do \
  { \
    assert(d <= m_columns); \
    *stream << makeString(m, d, true) << (preserve_line_breaks ? "\n" : " "); \
    d = 0; \
  } while(false)

#define LINE_WRAP do \
  { \
    assert(d <= m_columns); \
    *stream << makeString(m, d, false); \
    d = 0; \
  } while(false)

#define LINE_FLUSH do \
  { \
    assert(d <= m_columns); \
    *stream << makeString(m, d, false); \
    d = 0; \
  } while(false)

namespace Konsole
{

Screen::Screen(int l, int c)
  : m_lines(l), m_columns(c),
    m_image(new Character[(m_lines + 1) * m_columns]),
    m_histCursor(0), m_hist(new HistoryScrollNone()),
    m_cursorX(0), m_cursorY(0),
    m_cursorFg(CharacterColor()), m_cursorBg(CharacterColor()), m_cursorRend(0),
    m_topMargin(0), m_bottomMargin(0), m_tabStops(nullptr),
    m_selBegin(0), m_selTopLeft(0), m_selBottomRight(0),
    m_selBusy(false), m_blockSelectionMode(false),
    m_effectiveFg(CharacterColor()), m_effectiveBg(CharacterColor()), m_effectiveRend(0),
    m_savedCursorX(0), m_savedCursorY(0),
    m_savedCursorFg(CharacterColor()), m_savedCursorBg(CharacterColor()), m_savedCursorRend(0),
    m_lastPos(-1), m_lastDrawnChar(0)
{
  m_line_wrapped.resize(m_lines + 1);
  initTabStops();
  clearSelection();
  reset();
}

Screen::~Screen()
{
  delete[] m_image;
  delete[] m_tabStops;
  delete m_hist;
}

void Screen::cursorUp(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  const int stop = m_cursorY < m_topMargin ? 0 : m_topMargin;
  m_cursorX = std::min(m_columns - 1, m_cursorX); // nowrap!
  m_cursorY = std::max(stop, m_cursorY - n);
}

void Screen::cursorDown(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  const int stop = m_cursorY > m_bottomMargin ? m_lines - 1 : m_bottomMargin;
  m_cursorX = std::min(m_columns - 1, m_cursorX); // nowrap!
  m_cursorY = std::min(stop, m_cursorY + n);
}

void Screen::cursorLeft(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  m_cursorX = std::min(m_columns - 1, m_cursorX); // nowrap!
  m_cursorX = std::max(0, m_cursorX - n);
}

void Screen::cursorRight(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  m_cursorX = std::min(m_columns - 1, m_cursorX + n);
}

void Screen::cursorNextLine(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  m_cursorX = 0;
  m_cursorY = std::min(m_lines - 1, m_cursorY + n);
}

void Screen::cursorPrevLine(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  m_cursorX = 0;
  m_cursorY = std::max(0, m_cursorY - n);
}

void Screen::setCursorY(int y)
{
  if (y < 1)
  {
    y = 1;
  }
  y -= 1; // Adjust to internal line numbering
  m_cursorY = std::clamp(y + (getMode(MODE_Origin) ? m_topMargin : 0) , 0, m_lines - 1);
}

void Screen::setCursorX(int x)
{
  x -= 1; // Adjust to internal column numbering
  m_cursorX = std::clamp(x, 0, m_columns - 1);
}

void Screen::setCursorYX(int y, int x)
{
  setCursorY(y);
  setCursorX(x);
}

void Screen::setMargins(int top, int bot)
{
  if (top < 1)
  {
    top = 1;
  }
  if (bot < 1 || bot > m_lines)
  {
    bot = m_lines;
  }
  if ( top >= bot )
  {
    // Default error action: ignore
    kdDebug() << " setRegion(" << top << "," << bot << ") : bad range." << endl;
    return;
  }
  top = top - 1; // Adjust to internal line numbering
  bot = bot - 1; // Adjust to internal line numbering
  m_topMargin = top;
  m_bottomMargin = bot;
  m_cursorX = 0;
  m_cursorY = getMode(MODE_Origin) ? top : 0;
}

void Screen::setDefaultMargins()
{
  m_topMargin = 0;
  m_bottomMargin = m_lines - 1;
}

void Screen::newLine()
{
  if (getMode(MODE_NewLine))
  {
    toStartOfLine();
  }
  index();
}

void Screen::nextLine()
{
  toStartOfLine();
  index();
}

void Screen::index()
{
  if (m_cursorY == m_bottomMargin)
  {
    scrollUp(1);
  }
  else if (m_cursorY < m_lines - 1)
  {
    m_cursorY += 1;
  }
}

void Screen::reverseIndex()
{
  if (m_cursorY == m_topMargin)
  {
    scrollDown(m_topMargin, 1);
  }
  else if (m_cursorY > 0)
  {
    m_cursorY -= 1;
  }
}

void Screen::scrollUp(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  for (int i = 0; i < n; i++)
  {
    if (m_topMargin == 0)
    {
      addHistLine();
    }
    scrollUp(m_topMargin, 1);
  }
}

void Screen::scrollDown(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  scrollDown(m_topMargin, n);
}

void Screen::backSpace()
{
  m_cursorX = std::max(0, m_cursorX - 1);
}

void Screen::tab(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  while (n > 0 && m_cursorX < m_columns - 1)
  {
    do
    {
      cursorRight(1);
    }
    while (m_cursorX < m_columns - 1 && !m_tabStops[m_cursorX]);
    n--;
  }
}

void Screen::backTab(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  while (n > 0 && m_cursorX > 0)
  {
    do
    {
      cursorLeft(1);
    }
    while (m_cursorX > 0 && !m_tabStops[m_cursorX]);
    n--;
  }
}

void Screen::eraseChars(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  int p = std::clamp(m_cursorX + n - 1, 0, m_columns - 1);
  clearImage(loc(m_cursorX, m_cursorY), loc(p, m_cursorY), ' ');
}

void Screen::deleteChars(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  if (n > m_columns)
  {
    n = m_columns;
  }
  int p = std::clamp(m_cursorX + n, 0, m_columns - 1);
  moveImage(loc(m_cursorX, m_cursorY), loc(p, m_cursorY), loc(m_columns - 1, m_cursorY));
  clearImage(loc(m_columns - n, m_cursorY), loc(m_columns - 1, m_cursorY), ' ');
}

void Screen::insertChars(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  int p = std::clamp(m_columns - 1 - n, 0, m_columns - 1);
  int q = std::clamp(m_cursorX + n, 0, m_columns - 1);
  moveImage(loc(q, m_cursorY), loc(m_cursorX, m_cursorY), loc(p, m_cursorY));
  clearImage(loc(m_cursorX, m_cursorY), loc(q - 1, m_cursorY), ' ');
}

void Screen::repeatChars(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  // From ECMA-48 version 5, section 8.3.103:
  // "If the character preceding REP is a control function or part of a
  // control function, the effect of REP is not defined by this Standard."
  //
  // So, a "normal" program should always use REP immediately after a visible
  // character (those other than escape sequences). So, m_lastDrawnChar can be
  // safely used.
  while (n > 0)
  {
    displayCharacter(m_lastDrawnChar);
    --n;
  }
}

void Screen::deleteLines(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  scrollUp(m_cursorY, n);
}

void Screen::insertLines(int n)
{
  if (n < 1)
  {
    n = 1;
  }
  scrollDown(m_cursorY, n);
}

void Screen::clearTabStops()
{
  for (int i = 0; i < m_columns; i++)
  {
    m_tabStops[i] = false;
  }
}

void Screen::changeTabStop(bool set)
{
  if (m_cursorX >= m_columns)
  {
    return;
  }
  m_tabStops[m_cursorX] = set;
}

void Screen::resetMode(int m)
{
  m_currParm.mode[m] = false;
  if (m == MODE_Origin)
  {
    m_cursorX = 0;
    m_cursorY = 0;
  }
}

void Screen::setMode(int m)
{
  m_currParm.mode[m] = true;
  if (m == MODE_Origin)
  {
    m_cursorX = 0;
    m_cursorY = m_topMargin;
  }
}

void Screen::saveCursor()
{
  m_savedCursorX = m_cursorX;
  m_savedCursorY = m_cursorY;
  m_savedCursorFg = m_cursorFg;
  m_savedCursorBg = m_cursorBg;
  m_savedCursorRend = m_cursorRend;
}

void Screen::restoreCursor()
{
  m_cursorX = std::min(m_savedCursorX, m_columns - 1);
  m_cursorY = std::min(m_savedCursorY, m_lines - 1);
  m_cursorFg = m_savedCursorFg;
  m_cursorBg = m_savedCursorBg;
  m_cursorRend = m_savedCursorRend;
  updateEffectiveRendition();
}

void Screen::clear()
{
  clearEntireScreen();
  setCursorYX(0, 0);
}

void Screen::clearEntireScreen()
{
  clearImage(loc(0, 0), loc(m_columns - 1, m_lines - 1), ' ');
}

void Screen::clearToEndOfScreen()
{
  clearImage(loc(m_cursorX, m_cursorY), loc(m_columns - 1, m_lines - 1), ' ');
}

void Screen::clearToBeginOfScreen()
{
  clearImage(loc(0, 0), loc(m_cursorX, m_cursorY), ' ');
}

void Screen::clearEntireLine()
{
  clearImage(loc(0, m_cursorY), loc(m_columns - 1, m_cursorY), ' ');
}

void Screen::clearToEndOfLine()
{
  clearImage(loc(m_cursorX, m_cursorY), loc(m_columns - 1, m_cursorY), ' ');
}

void Screen::clearToBeginOfLine()
{
  clearImage(loc(0, m_cursorY), loc(m_cursorX, m_cursorY), ' ');
}

void Screen::helpAlign()
{
  clearImage(loc(0, 0), loc(m_columns - 1, m_lines - 1), 'E');
}

void Screen::setRendition(int re)
{
  m_cursorRend |= re;
  updateEffectiveRendition();
}

void Screen::resetRendition(int re)
{
  m_cursorRend &= ~re;
  updateEffectiveRendition();
}

void Screen::setForeColor(int space, int color)
{
  m_cursorFg = CharacterColor(space, color);
  updateEffectiveRendition();
}

void Screen::setBackColor(int space, int color)
{
  m_cursorBg = CharacterColor(space, color);
  updateEffectiveRendition();
}

void Screen::setDefaultRendition()
{
  setForeColor(COLOR_SPACE_DEFAULT, DEFAULT_FORE_COLOR);
  setBackColor(COLOR_SPACE_DEFAULT, DEFAULT_BACK_COLOR);
  m_cursorRend = DEFAULT_RENDITION;
  updateEffectiveRendition();
}

void Screen::reset()
{
  // wrap at end of margin
  setMode(MODE_Wrap );
  saveMode(MODE_Wrap );

  // position reference to (0,0)
  resetMode(MODE_Origin);
  saveMode(MODE_Origin);

  // overstroke
  resetMode(MODE_Insert);
  saveMode(MODE_Insert);

  // cursor visible
  setMode(MODE_Cursor);

  // screen not inverse
  resetMode(MODE_Screen);

  resetMode(MODE_NewLine);

  m_topMargin = 0;
  m_bottomMargin = m_lines - 1;

  setDefaultRendition();
  saveCursor();

  clear();
}

void Screen::displayCharacter(unsigned short c)
{
  // Note that VT100 does wrapping before putting the character.
  // This has impact on the assumption of valid cursor positions.
  // We indicate the fact that a newline has to be triggered by
  // putting the cursor one right to the last column of the screen.

  int w = konsole_wcwidth(c);

  if (w <= 0)
  {
    return;
  }

  if (m_cursorX + w > m_columns)
  {
    if (getMode(MODE_Wrap))
    {
      m_line_wrapped[m_cursorY] = true;
      nextLine();
    }
    else
    {
      m_cursorX = m_columns - w;
    }
  }

  if (getMode(MODE_Insert))
  {
    insertChars(w);
  }

  int i = loc(m_cursorX, m_cursorY);
  checkSelection(i, i); // check if selection is still valid.

  m_image[i].m_character = c;
  m_image[i].m_fgColor = m_effectiveFg;
  m_image[i].m_bgColor = m_effectiveBg;
  m_image[i].m_rendition = m_effectiveRend;
  m_lastPos = i;
  m_lastDrawnChar = c;

  m_cursorX += w--;
  while (w)
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
  {
    return;
  }

  TQChar c(m_image[m_lastPos].m_character);
  compose.prepend(c);
  compose.compose();
  m_image[m_lastPos].m_character = compose[0].unicode();
}

void Screen::resizeImage(int new_lines, int new_columns)
{
  if (new_lines == m_lines && new_columns == m_columns)
  {
    return;
  }

  if (m_cursorY > new_lines - 1)
  {
    // attempt to preserve focus and lines
    m_bottomMargin = m_lines - 1; // FIXME: margin lost
    for (int i = 0; i < m_cursorY - (new_lines - 1); i++)
    {
      addHistLine();
      scrollUp(0, 1);
    }
  }

  // make new image
  Character *newimg = new Character[(new_lines + 1) * new_columns];
  TQBitArray newwrapped(new_lines + 1);
  clearSelection();

  // clear new image
  for (int y = 0; y < new_lines; y++)
  {
    for (int x = 0; x < new_columns; x++)
    {
      newimg[y * new_columns + x].m_character = ' ';
      newimg[y * new_columns + x].m_fgColor = CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_FORE_COLOR);
      newimg[y * new_columns + x].m_bgColor = CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_BACK_COLOR);
      newimg[y * new_columns + x].m_rendition = DEFAULT_RENDITION;
    }
    newwrapped[y] = false;
  }
  int cpy_lines = std::min(new_lines, m_lines);
  int cpy_columns = std::min(new_columns, m_columns);

  // copy to new image
  for (int y = 0; y < cpy_lines; y++)
  {
    for (int x = 0; x < cpy_columns; x++)
    {
      newimg[y * new_columns + x].m_character = m_image[loc(x, y)].m_character;
      newimg[y * new_columns + x].m_fgColor = m_image[loc(x, y)].m_fgColor;
      newimg[y * new_columns + x].m_bgColor = m_image[loc(x, y)].m_bgColor;
      newimg[y * new_columns + x].m_rendition = m_image[loc(x, y)].m_rendition;
    }
    newwrapped[y] = m_line_wrapped[y];
  }
  delete[] m_image;
  m_image = newimg;
  m_line_wrapped = newwrapped;
  m_lines = new_lines;
  m_columns = new_columns;
  m_cursorX = std::min(m_cursorX, m_columns - 1);
  m_cursorY = std::min(m_cursorY, m_lines - 1);

  // FIXME: try to keep values, evtl.
  m_topMargin = 0;
  m_bottomMargin = m_lines - 1;
  initTabStops();
  clearSelection();
}

Character* Screen::getCookedImage()
{
  int x, y;
  Character *merged = (Character*)malloc((m_lines * m_columns + 1) * sizeof(Character));
  Character defaultChar(' ', CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_FORE_COLOR),
          CharacterColor(COLOR_SPACE_DEFAULT, DEFAULT_BACK_COLOR), DEFAULT_RENDITION);
  merged[m_lines * m_columns] = defaultChar;

  for (y = 0; y < m_lines && y < (m_hist->getLines() - m_histCursor); y++)
  {
    int len = std::min(m_columns, m_hist->getLineLen(y + m_histCursor));
    int yp = y * m_columns;
    m_hist->getCells(y + m_histCursor, 0, len, merged + yp);
    for (x = len; x < m_columns; x++)
    {
      merged[yp + x] = defaultChar;
    }
    if (m_selBegin != -1)
    {
      for (x = 0; x < m_columns; x++)
      {
        if (isSelected(x, y))
        {
          int p = x + yp;
          reverseRendition(&merged[p]); // for selection
        }
      }
    }
  }

  if (m_lines >= m_hist->getLines() - m_histCursor)
  {
    for (y = m_hist->getLines() - m_histCursor; y < m_lines ; y++)
    {
      int yp = y * m_columns;
      int yr = (y - m_hist->getLines() + m_histCursor) * m_columns;
      for (x = 0; x < m_columns; x++)
      {
        int p = x + yp;
        int r = x + yr;
        merged[p] = m_image[r];
        if (m_selBegin != -1 && isSelected(x, y))
        {
          reverseRendition(&merged[p]); // for selection
        }
      }

    }
  }

  // inverse display
  if (getMode(MODE_Screen))
  {
    for (int i = 0; i < m_lines * m_columns; i++)
    {
      reverseRendition(&merged[i]); // for reverse display
    }
  }

  int loc_ = loc(m_cursorX, m_cursorY + m_hist->getLines() - m_histCursor);
  if (getMode(MODE_Cursor) && loc_ < m_columns * m_lines)
  {
    merged[loc(m_cursorX, m_cursorY + (m_hist->getLines() - m_histCursor))].m_rendition |= RE_CURSOR;
  }

  return merged;
}

TQBitArray Screen::getCookedLineWrapped()
{
  TQBitArray result(m_lines);
  for (int y = 0; y < m_lines && y < (m_hist->getLines() - m_histCursor); y++)
  {
    result[y] = m_hist->isWrappedLine(y + m_histCursor);
  }
  if (m_lines >= m_hist->getLines() - m_histCursor)
  {
    for (int y = m_hist->getLines() - m_histCursor; y < m_lines ; y++)
    {
      result[y] = m_line_wrapped[y - m_hist->getLines() + m_histCursor];
    }
  }
  return result;
}

void Screen::setScroll(const HistoryType &t)
{
  clearSelection();
  m_hist = t.getScroll(m_hist);
  m_histCursor = m_hist->getLines();
}

void Screen::setSelectionStart(const int x, const int y, const bool blockSelectionMode)
{
  m_selBegin = loc(x, y + m_histCursor) ;

  /* FIXME, HACK to correct for x too far to the right... */
  if (x == m_columns)
  {
    m_selBegin--;
  }

  m_selBottomRight = m_selBegin;
  m_selTopLeft = m_selBegin;
  m_blockSelectionMode = blockSelectionMode;
}

void Screen::setSelectionEnd(const int x, const int y)
{
  if (m_selBegin == -1)
  {
    return;
  }
  int l = loc(x, y + m_histCursor);
  if (l < m_selBegin)
  {
    m_selTopLeft = l;
    m_selBottomRight = m_selBegin;
  }
  else
  {
    /* FIXME, HACK to correct for x too far to the right... */
    if (x == m_columns)
    {
      l--;
    }
    m_selTopLeft = m_selBegin;
    m_selBottomRight = l;
  }
}

void Screen::getSelectionStart(int &column, int &line) const
{
  if (m_selTopLeft != -1)
  {
    column = m_selTopLeft % m_columns;
    line = m_selTopLeft / m_columns;
  }
  else
  {
    column = m_cursorX + getHistLines();
    line = m_cursorY + getHistLines();
  }
}

void Screen::getSelectionEnd(int &column, int &line) const
{
  if (m_selBottomRight != -1)
  {
    column = m_selBottomRight % m_columns;
    line = m_selBottomRight / m_columns;
  }
  else
  {
    column = m_cursorX + getHistLines();
    line = m_cursorY + getHistLines();
  }
}

void Screen::clearSelection()
{
  m_selBottomRight = -1;
  m_selTopLeft = -1;
  m_selBegin = -1;
}

void Screen::selectAll(bool wholeHistory)
{
  int firstLine = wholeHistory ? 0 : m_histCursor;
  int lastLine = m_lines - 1 + (wholeHistory ? m_hist->getLines() : m_histCursor);

  m_selBegin = loc(0, firstLine);
  m_selTopLeft = m_selBegin;
  m_selBottomRight = loc(m_columns - 1, lastLine);
  m_blockSelectionMode = false;
}

bool Screen::isSelected(const int x, const int y) const
{
  if (m_blockSelectionMode)
  {
    int sel_Left, sel_Right;
    if ( m_selTopLeft % m_columns < m_selBottomRight % m_columns )
    {
      sel_Left = m_selTopLeft; sel_Right = m_selBottomRight;
    }
    else
    {
      sel_Left = m_selBottomRight; sel_Right = m_selTopLeft;
    }
    return ( x >= sel_Left % m_columns ) && ( x <= sel_Right % m_columns ) &&
           ( y + m_histCursor >= m_selTopLeft / m_columns ) &&
           ( y + m_histCursor <= m_selBottomRight / m_columns );
  }
  else
  {
    int pos = loc(x, y + m_histCursor);
    return ( pos >= m_selTopLeft && pos <= m_selBottomRight );
  }
}

TQString Screen::selectedText(bool preserve_line_breaks)
{
  TQString result;
  TQTextOStream stream(&result);
  selectedText(preserve_line_breaks, &stream);
  return result;
}

void Screen::selectedText(bool preserve_line_breaks, TQTextStream *stream)
{
  if (m_selBegin == -1)
  {
    return; // Selection got clear while selecting.
  }

  int *m; // buffer to fill.
  int s, d; // source index, dest. index.
  int hist_BR = loc(0, m_hist->getLines());
  int hY = m_selTopLeft / m_columns;
  int hX = m_selTopLeft % m_columns;
  int eol; // end of line

  // allocate buffer for maximum possible size...
  s = m_selTopLeft; // tracks copy in source.
  d = (m_selBottomRight - m_selTopLeft) / m_columns + 1;
  m = new int[m_columns + 3];
  d = 0;

  if (m_blockSelectionMode)
  {
    bool newlineneeded = false;
    preserve_line_breaks = true; // Just in case

    int sel_Left, sel_Right;
    if ( m_selTopLeft % m_columns < m_selBottomRight % m_columns )
    {
      sel_Left = m_selTopLeft; sel_Right = m_selBottomRight;
    }
    else
    {
      sel_Left = m_selBottomRight; sel_Right = m_selTopLeft;
    }

    while (s <= m_selBottomRight)
    {
      if (s < hist_BR)
      {
        // get lines from hist->history buffer.
        hX = sel_Left % m_columns;
        eol = m_hist->getLineLen(hY);
        if (eol > m_columns)
        {
          eol = m_columns;
        }
        if ((hY == (m_selBottomRight / m_columns)) && (eol > (m_selBottomRight % m_columns)))
        {
          eol = m_selBottomRight % m_columns + 1;
        }

        while (hX < eol && hX <= sel_Right % m_columns)
        {
          uint16_t c = m_hist->getCell(hY, hX++).m_character;
          if (c)
          {
            m[d++] = c;
          }
          s++;
        }
        LINE_END;

        hY++;
        s = hY * m_columns;
      }
      else
      {
        // or from screen image.
        if (isSelected((s - hist_BR) % m_columns, (s - hist_BR) / m_columns))
        {
          uint16_t c = m_image[s++ - hist_BR].m_character;
          if (c)
          {
            m[d++] = c;
            newlineneeded = true;
          }
          if (((s - hist_BR) % m_columns == 0) && newlineneeded)
          {
            LINE_END;
            newlineneeded = false;
          }
        }
        else
        {
          s++;
          if (newlineneeded)
          {
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
      {
        // get lines from hist->history buffer.
        eol = m_hist->getLineLen(hY);
        if (eol > m_columns)
        {
          eol = m_columns;
        }

        if ((hY == (m_selBottomRight / m_columns)) && (eol > (m_selBottomRight % m_columns)))
        {
          eol = m_selBottomRight % m_columns + 1;
        }

        while (hX < eol)
        {
          uint16_t c = m_hist->getCell(hY, hX++).m_character;
          if (c)
          {
            m[d++] = c;
          }
          s++;
        }

        if (s <= m_selBottomRight)
        {
          // The line break handling
          bool wrap = false;
          if (eol % m_columns == 0)
          {
            // That's either a full or empty line
            if ((eol != 0) && m_hist->isWrappedLine(hY))
            {
              wrap = true;
            }
          }
          else if ((eol + 1) % m_columns == 0)
          {
            if (m_hist->isWrappedLine(hY))
            {
              wrap = true;
            }
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
      {
        // or from screen image.
        eol = (s / m_columns + 1) * m_columns - 1;
        bool addNewLine = false;
        if (eol < m_selBottomRight)
        {
          while ((eol > s) &&
              (!m_image[eol - hist_BR].m_character || isSpace(m_image[eol - hist_BR].m_character)) &&
              !m_line_wrapped[(eol - hist_BR) / m_columns])
          {
            eol--;
          }
        }
        else if (eol == m_selBottomRight)
        {
          if (!m_line_wrapped[(eol - hist_BR) / m_columns])
          {
            addNewLine = true;
          }
        }
        else
        {
          eol = m_selBottomRight;
        }

        while (s <= eol)
        {
          uint16_t c = m_image[s++ - hist_BR].m_character;
          if (c)
          {
            m[d++] = c;
          }
        }

        if (eol < m_selBottomRight)
        {
          // eol processing
          bool wrap = false;
          if ((eol + 1) % m_columns == 0)
          {
            // the whole line is filled
            if (m_line_wrapped[(eol - hist_BR) / m_columns])
            {
              wrap = true;
            }
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

void Screen::streamHistory(TQTextStream *stream)
{
  m_selBegin = 0;
  m_selBottomRight = m_selBegin;
  m_selTopLeft = m_selBegin;
  setSelectionEnd(m_columns - 1, m_lines - 1 + m_hist->getLines() - m_histCursor);
  selectedText(true, stream);
  clearSelection();
}

TQString Screen::getHistoryLine(int no)
{
  m_selBegin = loc(0, no);
  m_selTopLeft = m_selBegin;
  m_selBottomRight = loc(m_columns - 1, no);
  return selectedText(false);
}

void Screen::checkSelection(int from, int to)
{
  if (m_selBegin == -1)
	{
		return;
	}
  int scr_TL = loc(0, m_hist->getLines());
  //Clear entire selection if it overlaps region [from, to]
  if ( m_selBottomRight > (from + scr_TL) && m_selTopLeft < (to + scr_TL) )
  {
    clearSelection();
  }
}

void Screen::clearImage(int startLoc, int endLoc, char c)
{
	int i;
  int scr_TL = loc(0, m_hist->getLines());
  // FIXME: check positions

  // Clear entire selection if it overlaps region to be moved...
  if ( m_selBottomRight > (startLoc + scr_TL) && m_selTopLeft < (endLoc + scr_TL) )
  {
    clearSelection();
  }

  for (i = startLoc; i <= endLoc; i++)
  {
    // Use the current colors but the default rendition
    // Check with: echo -e '\033[41;33;07m\033[2Khello world\033[00m'
    m_image[i].m_character = c;
    m_image[i].m_fgColor = m_cursorFg;
    m_image[i].m_bgColor = m_cursorBg;
    m_image[i].m_rendition = DEFAULT_RENDITION;
  }

  for (i = startLoc / m_columns; i <= endLoc / m_columns; i++)
	{
		m_line_wrapped[i] = false;
	}
}

void Screen::moveImage(int dest, int sourceStartLoc, int sourceEndLoc)
{
	// FIXME: check positions
	if (sourceEndLoc < sourceStartLoc)
	{
		kdDebug(1211) << "WARNING!!! call to Screen:moveImage with sourceEndLoc < sourceStartLoc!" << endl;
		return;
	}
	memmove(&m_image[dest], &m_image[sourceStartLoc], (sourceEndLoc - sourceStartLoc + 1) * sizeof(Character));
	for (int i = 0;i <= (sourceEndLoc - sourceStartLoc + 1) / m_columns;i++)
	{
		m_line_wrapped[(dest / m_columns) + i] = m_line_wrapped[(sourceStartLoc / m_columns) + i];
	}
	if (m_lastPos != -1)
	{
		int diff = dest - sourceStartLoc; // Scroll by this amount
		m_lastPos += diff;
		if ((m_lastPos < 0) || (m_lastPos >= (m_lines * m_columns)))
		{
			m_lastPos = -1;
		}
	}
	if (m_selBegin != -1)
	{
		// Adjust selection to follow scroll.
		bool beginIsTL = (m_selBegin == m_selTopLeft);
		int diff = dest - sourceStartLoc; // Scroll by this amount
		int scr_TL = loc(0, m_hist->getLines());
		int srca = sourceStartLoc + scr_TL; // Translate index from screen to global
		int srce = sourceEndLoc + scr_TL; // Translate index from screen to global
		int desta = srca + diff;
		int deste = srce + diff;

		if ((m_selTopLeft >= srca) && (m_selTopLeft <= srce))
		{
			m_selTopLeft += diff;
		}
		else if ((m_selTopLeft >= desta) && (m_selTopLeft <= deste))
		{
			m_selBottomRight = -1; // Clear selection (see below)
		}

		if ((m_selBottomRight >= srca) && (m_selBottomRight <= srce))
		{
			m_selBottomRight += diff;
		}
		else if ((m_selBottomRight >= desta) && (m_selBottomRight <= deste))
		{
			m_selBottomRight = -1; // Clear selection (see below)
		}

		if (m_selBottomRight < 0)
		{
			clearSelection();
		}
		else
		{
			if (m_selTopLeft < 0)
			{
				m_selTopLeft = 0;
			}
		}

		if (beginIsTL)
		{
			m_selBegin = m_selTopLeft;
		}
		else
		{
			m_selBegin = m_selBottomRight;
		}
	}
}

void Screen::scrollUp(int from, int n)
{
  if (n < 1)
  {
    n = 1;
  }
  if (from > m_bottomMargin)
  {
    return;
  }
  if ((from + n) > m_bottomMargin)
  {
    n = m_bottomMargin + 1 - from;
  }

  // FIXME: make sure `m_topMargin', `m_bottomMargin', `from', `n' is in bounds.
  moveImage(loc(0, from), loc(0, from + n), loc(m_columns - 1, m_bottomMargin));
  clearImage(loc(0, m_bottomMargin - n + 1), loc(m_columns - 1, m_bottomMargin), ' ');
}

void Screen::scrollDown(int from, int n)
{
  if (n < 1)
  {
    n = 1;
  }
  if (from > m_bottomMargin)
  {
    return;
  }
  if ((from + n) > m_bottomMargin)
  {
    n = m_bottomMargin - from;
  }

  // FIXME: make sure `m_topMargin', `m_bottomMargin', `from', `n' is in bounds.
  moveImage(loc(0, from + n), loc(0, from), loc(m_columns - 1, m_bottomMargin - n));
  clearImage(loc(0, from), loc(m_columns - 1, from + n - 1), ' ');
}

void Screen::addHistLine()
{
  assert(hasScroll() || m_histCursor == 0);

  // add to hist buffer
  // we have to take care about scrolling, too...

  if (hasScroll())
  {
		Character defaultChar;

    int end = m_columns - 1;
    while (end >= 0 && m_image[end] == defaultChar && !m_line_wrapped[0])
		{
			end -= 1;
		}

    int oldHistLines = m_hist->getLines();

    m_hist->addCells(m_image, end + 1);
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
    if ((m_histCursor > 0) && // We can scroll up and...
        ((m_histCursor != newHistLines) || // User is looking at history...
          m_selBusy)) // or user is selecting text.
    {
       m_histCursor--;
    }

    if (m_selBegin != -1)
    {
       // Scroll selection in history up
       int top_BR = loc(0, 1 + newHistLines);

       if (m_selTopLeft < top_BR)
			 {
				 m_selTopLeft -= m_columns;
			 }

       if (m_selBottomRight < top_BR)
			 {
				 m_selBottomRight -= m_columns;
			 }

       if (m_selBottomRight < 0)
       {
          clearSelection();
       }
       else
       {
          if (m_selTopLeft < 0)
					{
						m_selTopLeft = 0;
					}
       }

       if (beginIsTL)
			 {
				 m_selBegin = m_selTopLeft;
			 }
       else
			 {
				 m_selBegin = m_selBottomRight;
			 }
    }
  }

  if (!hasScroll())
	{
		m_histCursor = 0; // FIXME: a poor workaround
	}
}

void Screen::initTabStops()
{
  delete[] m_tabStops;
  m_tabStops = new bool[m_columns];

  // Arrg! The 1st tabstop has to be one longer than the other.
  // i.e. the kids start counting from 0 instead of 1.
  // Other programs might behave correctly. Be aware.
  for (int i = 0; i < m_columns; i++)
	{
		m_tabStops[i] = (i % 8 == 0 && i != 0);
	}
}

/*
   The rendition attributes are

      attr           widget screen
      -------------- ------ ------
      RE_UNDERLINE     XX     XX    affects foreground only
      RE_BLINK         XX     XX    affects foreground only
      RE_BOLD          XX     XX    affects foreground only
      RE_REVERSE       --     XX
      RE_TRANSPARENT   XX     --    affects background only
      RE_INTENSIVE     XX     --    affects foreground only

   Depending on settings, bold may be rendered as a heavier font
   in addition to a different color.
*/

void Screen::updateEffectiveRendition()
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
  {
    m_effectiveFg.setIntensive();
  }
}

void Screen::reverseRendition(Character *p)
{
  CharacterColor f = p->m_fgColor;
  CharacterColor b = p->m_bgColor;
  p->m_fgColor = b;
  p->m_bgColor = f;
  // p->r &= ~RE_TRANSPARENT;
}

} // namespace Konsole

static bool isSpace(uint16_t c)
{
  if (c > 32 && c < 127)
  {
    return false;
  }
  if (c == 32 || c == 0)
  {
    return true;
  }
  TQChar qc(c);
  return qc.isSpace();
}

static TQString makeString(const int *m, int d, bool stripTrailingSpaces)
{
  // Compute effective length, stripping trailing spaces if requested.
  if (stripTrailingSpaces)
  {
    while (d > 0 && m[d - 1] == ' ')
    {
      d--;
    }
  }

  TQString res;
  res.reserve(d);
  for (int i = 0; i < d; i++)
  {
    res += TQChar(m[i]);
  }
  return res;
}
