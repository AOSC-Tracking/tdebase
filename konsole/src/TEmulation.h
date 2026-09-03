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

#ifndef EMULATION_H
#define EMULATION_H

#include "TEWidget.h"
#include <tqtimer.h>
#include <stdio.h>
#include <tqtextcodec.h>
#include <tqguardedptr.h>
#include <keytrans.h>

namespace Konsole
{

class HistoryType;
class Screen;

enum { NOTIFYNORMAL=0, NOTIFYBELL=1, NOTIFYACTIVITY=2, NOTIFYSILENCE=3 };

class TEmulation : public TQObject
{ TQ_OBJECT

public:

  TEmulation(TEWidget* gui);
  virtual void changeGUI(TEWidget* newgui);
  ~TEmulation();

public:
  TQSize imageSize();
  virtual void setHistory(const HistoryType&);
  const TQTextCodec *codec() { return m_codec; }
  void setCodec(const TQTextCodec *);
  virtual const HistoryType& history();
  virtual void streamHistory(TQTextStream*);

  virtual void findTextBegin();
  virtual bool findTextNext( const TQString &str, bool forward, bool caseSensitive, bool regExp );

public slots: // signals incoming from TEWidget

  virtual void onImageSizeChange(int lines, int columns);
  virtual void onHistoryCursorChange(int cursor);

  // HACK - start
          void onKeyPress(TQKeyEvent*);    // This should not be overwritten by derived classes
  virtual void doKeyPress(TQKeyEvent*);    // Instead this function should be overwritten if necessary
          void onKeyReleased(TQKeyEvent*); // This should not be overwritten by derived classes
  virtual void doKeyReleased(TQKeyEvent*); // Instead this function should be overwritten if necessary
          void onFocusIn(TQFocusEvent*);   // This should not be overwritten by derived classes
  virtual void doFocusIn(TQFocusEvent*);   // Instead this function should be overwritten if necessary
  // HACK - end
 
  virtual void clearSelection();
  virtual void selectAll(bool wholeHistory = true);
  virtual void copySelection();
  virtual TQString getSelection();
  virtual void onSelectionBegin(const int x, const int y, const bool columnmode);
  virtual void onSelectionExtend(const int x, const int y);
  virtual void setSelection(const bool preserve_line_breaks);
  virtual void isBusySelecting(bool busy);
  virtual void testIsSelected(const int x, const int y, bool &selected);

public slots: // signals incoming from data source

  void onRcvBlock(const char* txt,int len);

signals:

  void lockPty(bool);
  void useUtf8(bool);
  void sndBlock(const char* txt,int len);
  void ImageSizeChanged(int lines, int columns);
  void changeColumns(int columns);
  void changeColLin(int columns, int lines);
  void notifySessionState(int state);
  void zmodemDetected();
  void changeTabTextColor(int color);

public:

  virtual void onRcvChar(int);

  virtual void setMode  (int) = 0;
  virtual void resetMode(int) = 0;
  void setMetaKeyMode(bool mode) { metaKeyMode = mode; }

  virtual void sendString(const char*) = 0;

  virtual void setConnect(bool r);
  bool isConnected() { return connected; }
  
  bool utf8() { return m_codec->mibEnum() == 106; }

  virtual char getErase();

  virtual void setListenToKeyPress(bool l);
  void setColumns(int columns);

  void setKeymap(int no);
  void setKeymap(const TQString &id);
  int keymapNo();
  TQString keymap();

  virtual void clearEntireScreen() =0;
  virtual void reset() =0;

protected:

  TQGuardedPtr<TEWidget> gui;
  Screen* scr;         // referes to one `screen'
  Screen* screen[2];   // 0 = primary, 1 = alternate
  void setScreen(int n); // set `scr' to `screen[n]'

  bool   connected;    // communicate with widget
  bool   listenToKeyPress;  // listen to input
  bool   metaKeyMode;       // true -> meta key is handled as Alt
  bool   metaIsPressed;     // true -> the meta key has been pressed down - HACK

  void setCodec(int c); // codec number, 0 = locale, 1=utf8

  const TQTextCodec* m_codec;
  TQTextDecoder* decoder;

  KeyTrans* keytrans;

// refreshing related material.
// this is localized in the class.
private slots: // triggered by timer

  void showBulk();

private:

  void connectGUI();

  void bulkStart();

private:

  TQTimer bulk_timer1;
  TQTimer bulk_timer2;
  
  int    m_findPos;
};

}

#endif // ifndef EMULATION_H
