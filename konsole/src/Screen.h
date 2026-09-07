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
#include <tqstring.h>

#include "Character.h"
#include "TEHistory.h"

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

/**
    \brief An image of characters with associated attributes.

    The terminal emulation (TEmulation) receives a serial stream of
    characters from the program currently running in the terminal.
    From this stream it creates an image of characters which is ultimately
    rendered by the display widget (TEWidget). Some types of emulation
    may have more than one screen image.

    The number of lines of output history which are kept in addition to the current
    screen image depends on the history scroll being used to store the output.
    The scroll is specified using setScroll().

    The screen image has a selection associated with it, specified using
    setSelectionStart() and setSelectionEnd(). The selected text can be
    retrieved using selectedText(). When the visible image is retrieved,
    characters which are part of the selection have their colors inverted.
*/
class Screen
{
  public:
    /** Construct a new screen image of size @p lines by @p columns. */
    Screen(int lines, int columns);
    ~Screen();

    Screen(const Screen &) = delete;
    Screen &operator=(const Screen &) = delete;


    //// VT100/2 Operations

    ///
    /// Cursor Movement
    ///

    /** Move cursor up by @p n lines. The cursor will stop at the top margin. */
    void cursorUp(int n);

    /** Move cursor down by @p n lines. The cursor will stop at the bottom margin. */
    void cursorDown(int n);

    /** Move cursor to the left by @p n columns. The cursor will stop at the first column. */
    void cursorLeft(int n);

    /** Move cursor to the right by @p n columns. The cursor will stop at the right-most column. */
    void cursorRight(int n);

    /**
     * Moves cursor to beginning of the line by @p n lines down.
     * The cursor will stop at the beginning of the line.
     */
    void cursorNextLine(int n);

    /**
     * Moves cursor to beginning of the line by @p n lines up.
     * The cursor will stop at the beginning of the line.
     */
    void cursorPrevLine(int n);

    /** Position the cursor on line @p y. */
    void setCursorY(int y);

    /** Position the cursor at column @p x. */
    void setCursorX(int x);

    /** Position the cursor at line @p y, column @p x. */
    void setCursorYX(int y, int x);

    /**
     * Sets the margins for scrolling the screen.
     * @param top The top line of the new scrolling margin.
     * @param bot The bottom line of the new scrolling margin.
     */
    void setMargins(int top, int bot);

    /** Returns the top line of the scrolling region. */
    int topMargin() const { return m_topMargin; }

    /** Returns the bottom line of the scrolling region. */
    int bottomMargin() const { return m_bottomMargin; }

    /** Resets the scrolling margins back to the top and bottom lines of the screen. */
    void setDefaultMargins();

    /**
     * Moves the cursor down one line. If the MODE_NewLine flag is enabled,
     * then the cursor is returned to the leftmost column first.
     * Equivalent to nextLine() if the MODE_NewLine flag is set
     * or index() otherwise.
     */
    void newLine();

    /**
     * Moves the cursor down one line and positions it at the beginning
     * of the line. Equivalent to calling toStartOfLine() followed by index()
     */
    void nextLine();

    /**
     * Move the cursor down one line. If the cursor is on the bottom
     * line of the scrolling region (as returned by bottomMargin()) the
     * scrolling region is scrolled up by one line instead.
     */
    void index();

    /**
     * Move the cursor up one line. If the cursor is on the top line
     * of the scrolling region (as returned by topMargin()) the scrolling
     * region is scrolled down by one line instead.
     */
    void reverseIndex();

    /**
     * Scroll the scrolling region of the screen up by @p n lines.
     * The scrolling region is initially the whole screen, but can be changed using setMargins()
     */
    void scrollUp(int n);

    /**
     * Scroll the scrolling region of the screen down by @p n lines.
     * The scrolling region is initially the whole screen, but can be changed using setMargins()
     */
    void scrollDown(int n);

    /** Moves the cursor to the beginning of the current line. */
    void toStartOfLine() { m_cursorX = 0; }

    /**
     * Moves the cursor one column to the left.
     */
    void backSpace();

    /** Moves the cursor @p n tab-stops to the right. */
    void tab(int n = 1);

    /** Moves the cursor @p n tab-stops to the left. */
    void backTab(int n = 1);

    ///
    /// Editing
    ///

    /**
     * Erase @p n characters beginning from the current cursor position.
     * This is equivalent to over-writing @p n characters starting with the current
     * cursor position with spaces.
     * If @p n is 0 then one character is erased.
     */
    void eraseChars(int n);

    /**
     * Delete @p n characters beginning from the current cursor position.
     * If @p n is less or equal to 0 then one character is deleted.
     */
    void deleteChars(int n);

    /**
     * Insert @p n blank characters beginning from the current cursor position.
     * The position of the cursor is not altered.
     * If @p n is less or equal to 0 then one character is inserted.
     */
    void insertChars(int n);

    /**
     * Repeat the preceding graphic character @p n times, including SPACE.
     * If @p n is less or equal to 0 then the character is repeated once.
     */
    void repeatChars(int n);

    /**
     * Removes @p n lines beginning from the current cursor position.
     * The position of the cursor is not altered.
     * If @p n is less or equal to 0 then one line is removed.
     */
    void deleteLines(int n);

    /**
     * Inserts @p lines beginning from the current cursor position.
     * The position of the cursor is not altered.
     * If @p n is 0 then one line is inserted.
     */
    void insertLines(int n);

    /** Clears all the tab stops. */
    void clearTabStops();

    /**  Sets or removes a tab stop at the cursor's current column. */
    void changeTabStop(bool set);

    /** Resets (clears) the specified screen @p m. */
    void resetMode(int m);

    /** Sets (enables) the specified screen @p m. */
    void setMode(int m);

    /** Saves the state of the specified screen @p m. It can be restored using restoreMode() */
    void saveMode(int m) { m_saveParm.mode[m] = m_currParm.mode[m]; }

    /** Restores the state of a screen @p m saved by calling saveMode() */
    void restoreMode(int m) { m_currParm.mode[m] = m_saveParm.mode[m]; }

    /** Returns whether the specified screen @p me is enabled or not .*/
    bool getMode(int m) const { return m_currParm.mode[m]; }

    /**
     * Saves the current position and appearance (text color and style) of the cursor.
     * It can be restored by calling restoreCursor()
     */
    void saveCursor();

    /** Restores the position and appearance of the cursor. See saveCursor() */
    void restoreCursor();

    /** Clears the entire screen and home the cursor. */
    void clear();

    /** Clear the whole screen. */
    void clearEntireScreen();

    /** Clear the area of the screen from the current cursor position to the end of the screen. */
    void clearToEndOfScreen();

    /** Clear the area of the screen from the current cursor position to the start of the screen. */
    void clearToBeginOfScreen();

    /** Clears the whole of the line on which the cursor is currently positioned. */
    void clearEntireLine();

    /** Clears from the current cursor position to the end of the line. */
    void clearToEndOfLine();

    /** Clears from the current cursor position to the beginning of the line. */
    void clearToBeginOfLine();

    /** Fills the entire screen with the letter 'E' */
    void helpAlign();

    /**
     * Enables the given @p rendition flags.
     * Rendition flags control the appearance of characters on the screen.
     */
    void setRendition(int rendition);

    /**
     * Disables the given @p rendition flags.
     * Rendition flags control the appearance of characters on the screen.
     */
    void resetRendition(int rendition);

    /**
     * Sets the cursor's foreground color.
     * @param space The color space used by the @p color argument
     * @param color The new foreground color, the meaning of which depends on
     * the color @p space used.
     */
    void setForeColor(int space, int color);

    /**
     * Sets the cursor's background color.
     * @param space The color space used by the @p color argument.
     * @param color The new foreground color, the meaning of which depends on
     * the color @p space used.
     */
    void setBackColor(int space, int color);

    /**
     * Resets the cursor to the default colors and sets the
     * character's rendition flags back to the default settings.
     */
    void setDefaultRendition();

    /** Returns the column which the cursor is positioned at. */
    int getCursorX() const { return m_cursorX; }

    /** Returns the line which the cursor is positioned on. */
    int getCursorY() const { return m_cursorY; }

    /** Moves the cursor to the home position */
    void home() { setCursorYX(0, 0); }

    /**
     * Resets the state of the screen. This resets the various screen modes
     * back to their default states. The cursor style and colors are reset
     * (as if setDefaultRendition() had been called)
     *
     * <ul>
     * <li>Line wrapping is enabled.</li>
     * <li>Origin mode is disabled.</li>
     * <li>Insert mode is disabled.</li>
     * <li>Cursor mode is enabled.</li>
     * <li>Screen mode is disabled.</li>
     * <li>New line mode is disabled.</li>
     * </ul>
     */
    void reset();

    /**
     * Displays a new character at the current cursor position.
     *
     * If the cursor is currently positioned at the right-edge of the screen and
     * line wrapping is enabled then the character is added at the start of a new
     * line below the current one.
     *
     * If the MODE_Insert screen mode is currently enabled then the character
     * is inserted at the current cursor position, otherwise it will replace the
     * character already at the current cursor position.
     */
    void displayCharacter(unsigned short c);

    // Do composition with last shown character
    void compose(TQString compose);

    /**
     * Resizes the image to a new fixed size of @p new_lines by @p new_columns.
     * The top and bottom margins are reset to the top and bottom of the new
     * screen size. Tab stops are also reset and the current selection is
     * cleared.
     */
    void resizeImage(int new_lines, int new_columns);

    /**
     *  Returns a copy of the screen image.
     *  Get the size of the image by getLines() and getColumns().
     *  NOTE that the image returned by this function must later be freed.
     */
    Character* getCookedImage();

    /** Returns information about whether lines are wrapped or not. */
    TQBitArray getCookedLineWrapped();

    /** Return the number of lines. */
    int getLines() const { return m_lines; }

    /** Return the number of columns. */
    int getColumns() const { return m_columns; }

    /** Set the position of the history cursor. */
    void setHistCursor(int cursor) { m_histCursor = cursor; }

    /** Return the position of the history cursor. */
    int getHistCursor() const { return m_histCursor; }

    /** Return the number of lines in the history buffer. */
    int getHistLines() const { return m_hist->getLines(); }

    /** Sets the type of storage used to keep lines in the history. */
    void setScroll(const HistoryType &t);

    /** Returns the type of storage used to keep lines in the history. */
    const HistoryType& getScroll() const { return m_hist->getType(); }

    /**
     * Returns true if this screen keeps lines that are scrolled off the screen
     * in a history buffer.
     */
    bool hasScroll() const { return m_hist->hasScroll(); }

    /**
     * Sets the start of the selection.
     * @param x The column index of the first character in the selection.
     * @param y The line index of the first character in the selection.
     * @param blockSelectionMode True if the selection is in column mode.
     */
    void setSelectionStart(const int x, const int y, const bool blockSelectionMode);

    /**
     * Sets the end of the current selection.
     * @param x The column index of the last character in the selection.
     * @param y The line index of the last character in the selection.
     */
    void setSelectionEnd(const int x, const int y);

    /**
     * Retrieves the start of the selection or the cursor position if there
     * is no selection.
     */
    void getSelectionStart(int &column, int &line) const;

    /**
     * Retrieves the end of the selection or the cursor position if there
     * is no selection.
     */
    void getSelectionEnd(int &column, int &line) const;

    /** Clears the current selection */
    void clearSelection();

    /**
     * Select all screen text. If @p wholeHistory is true, the whole history
     * will be selected, otherwise just the current contents of the screen.
     */
    void selectAll(bool wholeHistory);

    /** Sets the state of the selection. */
    void setBusySelecting(bool busy) { m_selBusy = busy; }

    /**
     *  Returns true if the character at row @p y and column @p x is part of the
     *  current selection.
     */
    bool isSelected(const int x, const int y) const;

    /**
     * Returns the currently selected text.
     * @param preserve_line_breaks If true, new line characters will be
     * inserted into the returned text at the end of each terminal line.
     */
    TQString selectedText(bool preserve_line_breaks);

    /**
     * Stream the currently selected text into a stream.
     * @param preserve_line_breaks If true, new line characters will be
     * inserted into the returned text at the end of each terminal line.
     * @param stream The stream where to print the selected text.
     */
    void selectedText(bool preserve_line_breaks, TQTextStream *stream);

    /**
     * Write the history into a stream.
     * @param stream The stream where to print the selected text.
     */
    void streamHistory(TQTextStream *stream);

    /**
     * Returns the @p no line from the history.
     * NOTE: the current selection gets modified.
     */
    TQString getHistoryLine(int no);

    /**
     * Checks if the text between @p from and @p to is inside the current
     * selection. If this is the case, the selection is cleared. The
     * @p from and @p to are coordinates in the current viewable window.
     * The loc(x,y) macro can be used to generate these values from a
     * column,line pair.
     *
     * @param from The start of the area to check.
     * @param to The end of the area to check
     */
    void checkSelection(int from, int to);

  private:

    // Fills a section of the screen image with the character @p c.
    // The parameters are specified as offsets from the start of the screen image.
    // The loc(x,y) macro can be used to generate these values from a column,line pair.
    // @param startLoc The starting location.
    // @param endLoc The ending location.
    void clearImage(int startLoc, int endLoc, char c);

    // Moves the screen image between 'sourceStartLoc' and 'sourceEndLoc' to 'dest'.
    // The parameters are specified as offsets from the start of the screen image.
    // The loc(x,y) macro can be used to generate these values from a column,line pair.
    // @param sourceStartLoc The starting location.
    // @param sourceEndLoc The ending location.
    // NOTE: can only move whole lines.
    void moveImage(int dest, int sourceStartLoc, int sourceEndLoc);

    // Scroll up 'n' lines in current region, clearing the bottom 'n' lines
    void scrollUp(int from, int n);

    // Scroll down 'n' lines in current region, clearing the top 'n' lines
    void scrollDown(int from, int n);

    // Add a line to the history buffer
    void addHistLine();

    // Initialize the tab stop locations
    void initTabStops();

    // Update the effective rendition
    void updateEffectiveRendition();

    // Invert foreground and background colors for the character @p p
    void reverseRendition(Character *p);

    /*
       The Screen class does more than one would expect, because it does part of the
       emulation providing state informations in form of modes, margins, tabulators,
       cursor, etc... including variables to save and restore parts of the state.
    */

    // screen image
    int m_lines;
    int m_columns;
    Character *m_image; // [lines][columns]
    TQBitArray m_line_wrapped; // [lines]

    // history buffer
    int m_histCursor;   // display position relative to start of the history buffer
    HistoryScroll *m_hist;

    // cursor location
    int m_cursorX;
    int m_cursorY;

    // cursor color and rendition info
    CharacterColor m_cursorFg;   // foreground
    CharacterColor m_cursorBg;   // background
    uint8_t        m_cursorRend; // rendition

    // margins
    int m_topMargin;    // top margin
    int m_bottomMargin; // bottom margin

    // stats
    ScreenParm m_currParm;
    bool *m_tabStops;

    // selection
    int  m_selBegin;           // The first location selected.
    int  m_selTopLeft;         // TopLeft Location.
    int  m_selBottomRight;     // Bottom Right Location.
    bool m_selBusy;            // Busy making a selection.
    bool m_blockSelectionMode; // Column selection mode

    // effective colors and rendition
    CharacterColor m_effectiveFg;
    CharacterColor m_effectiveBg;
    uint8_t        m_effectiveRend;

    // ------ saved information ------

    // saved cursor location
    int m_savedCursorX;
    int m_savedCursorY;

    // saved rendition info
    CharacterColor m_savedCursorFg;
    CharacterColor m_savedCursorBg;
    uint8_t        m_savedCursorRend;

    // last position where we added a character
    int m_lastPos;

    // used in REP (repeating char)
    unsigned short m_lastDrawnChar;

    // saved state
    ScreenParm m_saveParm;
};

}

#endif
