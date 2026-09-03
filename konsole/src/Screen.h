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

#ifndef SCREEN_H
#define SCREEN_H

#include <tqbitarray.h>

#include "Character.h"

#define MODE_Origin    0
#define MODE_Wrap      1
#define MODE_Insert    2
#define MODE_Screen    3
#define MODE_Cursor    4
#define MODE_NewLine   5
#define MODES_SCREEN   6

namespace Konsole
{

class HistoryScroll;
class HistoryType;

/*!
*/
struct ScreenParm
{
  int mode[MODES_SCREEN];
};


class Screen
{
public:
    Screen(int lines, int columns);
    ~Screen();

public: // these are all `Screen' operations
    //
    // VT100/2 Operations ------------------
    //
    // Cursor Movement
    //
    void cursorUp      (int n);
    void cursorDown    (int n);
    void cursorLeft    (int n);
    void cursorRight   (int n);
    void cursorNextLine(int n);
    void cursorPrevLine(int n);
    void setCursorY    (int y);
    void setCursorX    (int x);
    void setCursorYX   (int y, int x);
    void setMargins    (int t, int b);
    //
    // Cursor Movement with Scrolling
    //
    void newLine     ();
    void nextLine    ();
    void index       ();
    void reverseIndex();
    //
    // Scrolling
    //
    void scrollUp(int n);
    void scrollDown(int n);
    //
    void toStartOfLine();
    void backSpace    ();
    void tab          (int n = 1);
    void backTab      (int n);
    //
    // Editing
    //
    void eraseChars  (int n);
    void deleteChars (int n);
    void insertChars (int n);
    void repeatChars (int n);
    void deleteLines (int n);
    void insertLines (int n);
    //
    // -------------------------------------
    //
    void clearTabStops();
    void changeTabStop(bool set);
    //
    void resetMode   (int n);
    void setMode     (int n);
    void saveMode    (int n);
    void restoreMode (int n);
    //
    void saveCursor  ();
    void restoreCursor();
    //
    // -------------------------------------
    //
    void clearEntireScreen();
    void clearToEndOfScreen();
    void clearToBeginOfScreen();
    //
    void clearEntireLine();
    void clearToEndOfLine();
    void clearToBeginOfLine();
    //
    void helpAlign   ();
    //
    // -------------------------------------
    //
    void setRendition  (int rendition);
    void resetRendition(int rendition);
    //
    void setForeColor  (int space, int color);
    void setBackColor  (int space, int color);
    //
    void setDefaultRendition();
    //
    // -------------------------------------
    //
    bool getMode     (int n);
    //
    // only for report cursor position
    //
    int  getCursorX();
    int  getCursorY();
    //
    // -------------------------------------
    //
    void clear();
    void home();
    void reset();
    // Show character
    void displayCharacter(unsigned short c);
    
    // Do composition with last shown character
    void compose(TQString compose);
    
    //
    void resizeImage(int new_lines, int new_columns);
    //
    Character*  getCookedImage();
    TQBitArray getCookedLineWrapped();

    /*! return the number of lines. */
    int  getLines()   { return m_lines; }
    /*! return the number of columns. */
    int  getColumns() { return m_columns; }

    /*! set the position of the history cursor. */
    void setHistCursor(int cursor);
    /*! return the position of the history cursor. */

    int  getHistCursor();
    int  getHistLines ();
    void setScroll(const HistoryType&);
    const HistoryType& getScroll();
    bool hasScroll();

    //
    // Selection
    //
    void setSelectionStart(const int x, const int y, const bool columnmode);
    void setSelectionEnd(const int x, const int y);
    void clearSelection();
    void selectAll(bool wholeHistory);
    void setBusySelecting(bool busy) { m_selBusy = busy; }
    bool isSelected(const int x,const int y);

    TQString selectedText(bool preserve_line_breaks);
    void selectedText(bool preserve_line_breaks, TQTextStream* stream);
    void streamHistory(TQTextStream* stream);
    TQString getHistoryLine(int no);

    void checkSelection(int from, int to);

private: // helper

    void clearImage(int loca, int loce, char c);
    void moveImage(int dst, int loca, int loce);
    
    void scrollUp(int from, int i);
    void scrollDown(int from, int i);

    void addHistLine();

    void initTabStops();

    void updateEffectiveRendition();
    void reverseRendition(Character* p);

    /*
       The state of the screen is more complex as one would
       expect first. The screem does really do part of the
       emulation providing state informations in form of modes,
       margins, tabulators, cursor etc.

       Even more unexpected are variables to save and restore
       parts of the state.
    */

    // screen image ----------------

    int m_lines;
    int m_columns;
    Character *m_image; // [lines][columns]
    TQBitArray m_line_wrapped; // [lines]

    // history buffer ---------------

    int m_histCursor;   // display position relative to start of the history buffer
    HistoryScroll *m_hist;
    
    // cursor location

    int m_cursorX;
    int m_cursorY;

    // cursor color and rendition info

    CharacterColor m_cursorFg;      // foreground
    CharacterColor m_cursorBg;      // background
    uint8_t        m_cursorRend;    // rendition

    // margins ----------------

    int m_topMargin;      // top margin
    int m_bottomMargin;   // bottom margin

    // states ----------------

    ScreenParm m_currParm;

    // ----------------------------

    bool *m_tabStops;

    // selection -------------------

    int  m_selBegin;           // The first location selected.
    int  m_selTopLeft;         // TopLeft Location.
    int  m_selBottomRight;     // Bottom Right Location.
    bool m_selBusy;            // Busy making a selection.
    bool m_blockSelectionMode; // Column selection mode

    // effective colors and rendition ------------

    CharacterColor m_effectiveFg;   // These are derived from
    CharacterColor m_effectiveBg;   // the m_cursor* variables above
    uint8_t        m_effectiveRend; // to speed up operation

    //
    // save cursor, rendition & states ------------
    // 

    // cursor location

    int m_savedCursorX;
    int m_savedCursorY;

    // rendition info

    CharacterColor m_savedCursorFg;
    CharacterColor m_savedCursorBg;
    uint8_t        m_savedCursorRend;
    
    // last position where we added a character
    int m_lastPos;

    // used in REP (repeating char)
    unsigned short m_lastDrawnChar;

    // modes
    ScreenParm m_saveParm;
};

}

#endif
