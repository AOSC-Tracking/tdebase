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

#ifndef TE_WIDGET_H
#define TE_WIDGET_H

#include <tqbitarray.h>
#include <tqwidget.h>
#include <tqcolor.h>
#include <tqkeycode.h>
#include <tqtimer.h>
#include <tqscrollbar.h>
#include <tqdragobject.h>

#include <kpopupmenu.h>

#include "TECommon.h"


extern unsigned short vt100_graphics[32];

class Konsole;
class TQLabel;
class TQTimer;

class TEWidget : public TQFrame
// a widget representing attributed text
{
   Q_OBJECT
   TQ_OBJECT

  friend class Konsole;
public:

    TEWidget(TQWidget *parent=0, const char *name=0);
    virtual ~TEWidget();

    void setBlendColor(const QRgb color) { blend_color = color; }

    void setDefaultBackColor(const TQColor& color);
    TQColor getDefaultBackColor();

    const ColorEntry* getColorTable() const;
    void              setColorTable(const ColorEntry table[]);

    void setScrollbarLocation(int loc);
    int  getScrollbarLocation() { return scrollLoc; }
    enum { SCRNONE=0, SCRLEFT=1, SCRRIGHT=2 };

    void setScroll(int cursor, int lines);
    void doScroll(int lines);

    bool blinkingCursor() { return hasBlinkingCursor; }
    void setBlinkingCursor(bool blink);

    void setCtrlDrag(bool enable) { ctrldrag=enable; }
    bool ctrlDrag() { return ctrldrag; }

    void setCutToBeginningOfLine(bool enable) { cuttobeginningofline=enable; }
    bool cutToBeginningOfLine() { return cuttobeginningofline; }

    void setLineSpacing(uint);
    uint lineSpacing() const;

    void emitSelection(bool useXselection,bool appendReturn);
    void emitText(TQString text);

    void setImage(const ca* const newimg, int lines, int columns);
    void setLineWrapped(TQBitArray line_wrapped) { m_line_wrapped=line_wrapped; }

    void setCursorPos(const int curx, const int cury);

    int  Lines()   { return lines;   }
    int  Columns() { return columns; }

    int  fontHeight()   { return font_h;   }
    int  fontWidth()    { return font_w; }
    
    void calcGeometry();
    void propagateSize();
    void updateImageSize();
    void setSize(int cols, int lins);
    void setFixedSize(int cols, int lins);
    TQSize sizeHint() const;

    void setWordCharacters(TQString wc);
    TQString wordCharacters() { return word_characters; }

    void setBellMode(int mode);
    int bellMode() { return m_bellMode; }
    enum { BELLSYSTEM=0, BELLNOTIFY=1, BELLVISUAL=2, BELLNONE=3 };
    void Bell(bool visibleSession, TQString message);

    void setSelection(const TQString &t);

    /** 
     * Reimplemented.  Has no effect.  Use setVTFont() to change the font
     * used to draw characters in the display.
     */
    virtual void setFont(const TQFont &);

    /** Returns the font used to draw characters in the display */
    TQFont getVTFont() { return font(); }

    /** 
     * Sets the font used to draw the display.  Has no effect if @p font
     * is larger than the size of the display itself.    
     */
    void setVTFont(const TQFont& font);

    void setMouseMarks(bool on);
    static void setAntialias( bool enable ) { s_antialias = enable; }
    static bool antialias()                 { return s_antialias;   }
    static void setStandalone( bool standalone ) { s_standalone = standalone; }
    static bool standalone()                { return s_standalone;   }

    void setTerminalSizeHint(bool on) { terminalSizeHint=on; }
    bool isTerminalSizeHint() { return terminalSizeHint; }
    void setTerminalSizeStartup(bool on) { terminalSizeStartup=on; }

    void setBidiEnabled(bool set) { bidiEnabled=set; }
    bool isBidiEnabled() { return bidiEnabled; }
    
    void print(TQPainter &paint, bool friendly, bool exact);

    void setRim(int rim) { rimX=rim; rimY=rim; }

public slots:

    void setSelectionEnd();
    void copyClipboard();
    void pasteClipboard();
    void pasteSelection();
    void onClearSelection();

signals:

    void keyPressedSignal(TQKeyEvent *e);
    void mouseSignal(int cb, int cx, int cy);
    void changedFontMetricSignal(int height, int width);
    void changedContentSizeSignal(int height, int width);
    void changedHistoryCursor(int value);
    void configureRequest( TEWidget*, int state, int x, int y );

    void copySelectionSignal();
    void clearSelectionSignal();
    void beginSelectionSignal( const int x, const int y, const bool columnmode );
    void extendSelectionSignal( const int x, const int y );
    void endSelectionSignal(const bool preserve_line_breaks);
    void isBusySelecting(bool);
    void testIsSelected(const int x, const int y, bool &selected /* result */);
    void sendStringToEmu(const char*);

protected:

    virtual void styleChange( TQStyle& );

    bool eventFilter( TQObject *, TQEvent * );
    bool event( TQEvent * );

    void drawTextFixed(TQPainter &paint, int x, int y,
                       TQString& str, const ca *attr);

    void drawAttrStr(TQPainter &paint, TQRect rect,
                     TQString& str, const ca *attr, bool pm, bool clear);
    void paintEvent( TQPaintEvent * );

    void paintContents(TQPainter &paint, const TQRect &rect, bool pm=false);

    void resizeEvent(TQResizeEvent*);

    void fontChange(const TQFont &font);
    void frameChanged();

    void mouseDoubleClickEvent(TQMouseEvent* ev);
    void mousePressEvent( TQMouseEvent* );
    void mouseReleaseEvent( TQMouseEvent* );
    void mouseMoveEvent( TQMouseEvent* );
    void extendSelection( TQPoint pos );
    void wheelEvent( TQWheelEvent* );

    void focusInEvent( TQFocusEvent * );
    void focusOutEvent( TQFocusEvent * );
    bool focusNextPrevChild( bool next );
    // Dnd
    void dragEnterEvent(TQDragEnterEvent* event);
    void dropEvent(TQDropEvent* event);
    void doDrag();
    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState       state;
      TQPoint          start;
      TQTextDrag       *dragObject;
    } dragInfo;

    virtual int charClass(UINT16) const;

    void clearImage();

    void mouseTripleClickEvent(TQMouseEvent* ev);

    void imStartEvent( TQIMEvent *e );
    void imComposeEvent( TQIMEvent *e );
    void imEndEvent( TQIMEvent *e );

protected slots:

    void scrollChanged(int value);
    void blinkEvent();
    void blinkCursorEvent();

private:

//    TQChar (*fontMap)(TQChar); // possible vt100 font extension

    bool fixed_font; // has fixed pitch
    int  font_h;     // height
    int  font_w;     // width
    int  font_a;     // ascend

    int bX;    // offset
    int bY;    // offset

    int lines;
    int columns;
    int contentHeight;
    int contentWidth;
    ca *image; // [lines][columns]
    int image_size;
    TQBitArray m_line_wrapped;

    ColorEntry color_table[TABLE_COLORS];
    TQColor defaultBgColor;

    bool resizing;
    bool terminalSizeHint,terminalSizeStartup;
    bool bidiEnabled;
    bool mouse_marks;

    void makeImage();

    TQPoint iPntSel; // initial selection point
    TQPoint pntSel; // current selection point
    TQPoint tripleSelBegin; // help avoid flicker
    int    actSel; // selection state
    bool    word_selection_mode;
    bool    line_selection_mode;
    bool    preserve_line_breaks;
    bool    column_selection_mode;

    TQClipboard*    cb;
    TQScrollBar* scrollbar;
    int         scrollLoc;
    TQString     word_characters;
    TQTimer      bellTimer; //used to rate-limit bell events.  started when a bell event occurs,
                           //and prevents further bell events until it stops
    int         m_bellMode;

    bool blinking;   // hide text in paintEvent
    bool hasBlinker; // has characters to blink
    bool cursorBlinking;     // hide cursor in paintEvent
    bool hasBlinkingCursor;  // has blinking cursor enabled
    bool ctrldrag;           // require Ctrl key for drag
    bool cuttobeginningofline; // triple click only selects forward
    bool isBlinkEvent; // paintEvent due to blinking.
    bool isPrinting; // Paint job is intended for printer
    bool printerFriendly; // paint printer friendly, save ink
    bool printerBold; // Use a bold font instead of overstrike for bold
    bool isFixedSize; //Columns / lines are locked.
    TQTimer* blinkT;  // active when hasBlinker
    TQTimer* blinkCursorT;  // active when hasBlinkingCursor

    KPopupMenu* m_drop;
    TQString dropText;
    int m_dnd_file_count;

    bool possibleTripleClick;  // is set in mouseDoubleClickEvent and deleted
                               // after TQApplication::doubleClickInterval() delay

    static bool s_antialias;   // do we antialias or not
    static bool s_standalone;  // are we part of a standalone konsole?

    TQFrame *mResizeWidget;
    TQLabel *mResizeLabel;
    TQTimer *mResizeTimer;

    uint m_lineSpacing;

    TQRect       cursorRect; //for quick changing of cursor

    TQPoint configureRequestPoint;  // remember right mouse button click position
    bool colorsSwapped; // true during visual bell

    // the rim should normally be 1, 0 only when running in full screen mode.
    int rimX;      // left/right rim width
    int rimY;      // top/bottom rim high
    TQSize m_size;

    TQString m_imPreeditText;
    int m_imPreeditLength;
    int m_imStart;
    int m_imStartLine;
    int m_imEnd;
    int m_imSelStart;
    int m_imSelEnd;
    int m_cursorLine;
    int m_cursorCol;
    bool m_isIMEdit;
    bool m_isIMSel;

    QRgb blend_color;
 
private slots:
    void drop_menu_activated(int item);
    void swapColorTable();
    void tripleClickTimeout();  // resets possibleTripleClick
};

#endif // TE_WIDGET_H
