/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1996 by Matthias Ettrich <ettrich@kde.org>
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

/*! \class TEmulation

    \brief Mediator between TEWidget and Screen.

   This class is responsible to scan the escapes sequences of the terminal
   emulation and to map it to their corresponding semantic complements.
   Thus this module knows mainly about decoding escapes sequences and
   is a stateless device w.r.t. the semantics.

   It is also responsible to refresh the TEWidget by certain rules.

   \sa TEWidget \sa Screen

   \par A note on refreshing

   Although the modifications to the current screen image could immediately
   be propagated via `TEWidget' to the graphical surface, we have chosen
   another way here.

   The reason for doing so is twofold.

   First, experiments show that directly displaying the operation results
   in slowing down the overall performance of emulations. Displaying
   individual characters using X11 creates a lot of overhead.

   Second, by using the following refreshing method, the screen operations
   can be completely separated from the displaying. This greatly simplifies
   the programmer's task of coding and maintaining the screen operations,
   since one need not worry about differential modifications on the
   display affecting the operation of concern.

   We use a refreshing algorithm here that has been adoped from rxvt/kvt.

   By this, refreshing is driven by a timer, which is (re)started whenever
   a new bunch of data to be interpreted by the emulation arives at `onRcvBlock'.
   As soon as no more data arrive for `BULK_TIMEOUT' milliseconds, we trigger
   refresh. This rule suits both bulk display operation as done by curses as
   well as individual characters typed.

   We start also a second time which is never restarted. If repeatedly
   restarting of the first timer could delay continuous output indefinitly,
   the second timer guarantees that the output is refreshed with at least
   a fixed rate.
*/

/* FIXME
   - evtl. the bulk operations could be made more transparent.
*/

#include <kdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <tqregexp.h>
#include <tqclipboard.h>
#include <tqapplication.h>

#include <assert.h>

#include "TEmulation.h"
#include "TEHistory.h"
#include "Screen.h"
#include "TEWidget.h"

namespace Konsole
{

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                               TEmulation                                  */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#define CNTL(c) ((c)-'@')

/*!
*/

TEmulation::TEmulation(TEWidget* w)
: gui(w),
  scr(0),
  connected(false),
  listenToKeyPress(false),
  metaKeyMode(false),
  metaIsPressed(false),
  m_codec(0),
  decoder(0),
  keytrans(0),
  m_findPos(-1)
{

  screen[0] = new Screen(gui->Lines(),gui->Columns());
  screen[1] = new Screen(gui->Lines(),gui->Columns());
  scr = screen[0];

  TQObject::connect(&bulk_timer1, TQ_SIGNAL(timeout()), this, TQ_SLOT(showBulk()) );
  TQObject::connect(&bulk_timer2, TQ_SIGNAL(timeout()), this, TQ_SLOT(showBulk()) );
  connectGUI();
  setKeymap(0); // Default keymap
}

/*!
*/

void TEmulation::connectGUI()
{
  TQObject::connect(gui,TQ_SIGNAL(changedHistoryCursor(int)),
                   this,TQ_SLOT(onHistoryCursorChange(int)));
  TQObject::connect(gui,TQ_SIGNAL(keyPressedSignal(TQKeyEvent*)),
                   this,TQ_SLOT(onKeyPress(TQKeyEvent*)));
  TQObject::connect(gui,TQ_SIGNAL(keyReleasedSignal(TQKeyEvent*)),
                   this,TQ_SLOT(onKeyReleased(TQKeyEvent*)));
  TQObject::connect(gui,TQ_SIGNAL(focusInSignal(TQFocusEvent*)),
                   this,TQ_SLOT(onFocusIn(TQFocusEvent*)));
  TQObject::connect(gui,TQ_SIGNAL(beginSelectionSignal(const int,const int,const bool)),
		   this,TQ_SLOT(onSelectionBegin(const int,const int,const bool)) );
  TQObject::connect(gui,TQ_SIGNAL(extendSelectionSignal(const int,const int)),
		   this,TQ_SLOT(onSelectionExtend(const int,const int)) );
  TQObject::connect(gui,TQ_SIGNAL(endSelectionSignal(const bool)),
		   this,TQ_SLOT(setSelection(const bool)) );
  TQObject::connect(gui,TQ_SIGNAL(copySelectionSignal()),
		   this,TQ_SLOT(copySelection()) );
  TQObject::connect(gui,TQ_SIGNAL(clearSelectionSignal()),
		   this,TQ_SLOT(clearSelection()) );
  TQObject::connect(gui,TQ_SIGNAL(isBusySelecting(bool)),
		   this,TQ_SLOT(isBusySelecting(bool)) );
  TQObject::connect(gui,TQ_SIGNAL(testIsSelected(const int, const int, bool &)),
		   this,TQ_SLOT(testIsSelected(const int, const int, bool &)) );
}

/*!
*/

void TEmulation::changeGUI(TEWidget* newgui)
{
  if (static_cast<TEWidget *>( gui )==newgui) return;

  if ( gui ) {
    TQObject::disconnect(gui,TQ_SIGNAL(changedHistoryCursor(int)),
                     this,TQ_SLOT(onHistoryCursorChange(int)));
    TQObject::disconnect(gui,TQ_SIGNAL(keyPressedSignal(TQKeyEvent*)),
                     this,TQ_SLOT(onKeyPress(TQKeyEvent*)));
    TQObject::disconnect(gui,TQ_SIGNAL(keyReleasedSignal(TQKeyEvent*)),
                     this,TQ_SLOT(onKeyReleased(TQKeyEvent*)));
    TQObject::disconnect(gui,TQ_SIGNAL(focusInSignal(TQFocusEvent*)),
                     this,TQ_SLOT(onFocusIn(TQFocusEvent*)));
    TQObject::disconnect(gui,TQ_SIGNAL(beginSelectionSignal(const int,const int,const bool)),
                     this,TQ_SLOT(onSelectionBegin(const int,const int,const bool)) );
    TQObject::disconnect(gui,TQ_SIGNAL(extendSelectionSignal(const int,const int)),
                     this,TQ_SLOT(onSelectionExtend(const int,const int)) );
    TQObject::disconnect(gui,TQ_SIGNAL(endSelectionSignal(const bool)),
                     this,TQ_SLOT(setSelection(const bool)) );
    TQObject::disconnect(gui,TQ_SIGNAL(copySelectionSignal()),
                     this,TQ_SLOT(copySelection()) );
    TQObject::disconnect(gui,TQ_SIGNAL(clearSelectionSignal()),
                     this,TQ_SLOT(clearSelection()) );
    TQObject::disconnect(gui,TQ_SIGNAL(isBusySelecting(bool)),
                     this,TQ_SLOT(isBusySelecting(bool)) );
    TQObject::disconnect(gui,TQ_SIGNAL(testIsSelected(const int, const int, bool &)),
                     this,TQ_SLOT(testIsSelected(const int, const int, bool &)) );
  }
  gui=newgui;
  connectGUI();
}

/*!
*/

TEmulation::~TEmulation()
{
  delete screen[0];
  delete screen[1];
  delete decoder;
}

/*! change between primary and alternate screen
*/

void TEmulation::setScreen(int n)
{
  Screen *old = scr;
  scr = screen[n&1];
  gui->setScreen(n&1, scr);
  if (scr != old)
     old->setBusySelecting(false);
}

void TEmulation::setHistory(const HistoryType& t)
{
  screen[0]->setScroll(t);

  if (!connected) return;
  showBulk();
}

const HistoryType& TEmulation::history()
{
  return screen[0]->getScroll();
}

void TEmulation::setCodec(const TQTextCodec * qtc)
{
  m_codec = qtc;
  delete decoder;
  decoder = m_codec->makeDecoder();
  emit useUtf8(utf8());
}

void TEmulation::setCodec(int c)
{
  setCodec(c ? TQTextCodec::codecForName("utf8")
           : TQTextCodec::codecForLocale());
}

void TEmulation::setKeymap(int no)
{
  keytrans = KeyTrans::find(no);
}

void TEmulation::setKeymap(const TQString &id)
{
  keytrans = KeyTrans::find(id);
}

TQString TEmulation::keymap()
{
  return keytrans->id();
}

int TEmulation::keymapNo()
{
  return keytrans->numb();
}

// Interpreting Codes ---------------------------------------------------------

/*
   This section deals with decoding the incoming character stream.
   Decoding means here, that the stream is first seperated into `tokens'
   which are then mapped to a `meaning' provided as operations by the
   `Screen' class.
*/

/*!
*/

void TEmulation::receiveChars(int c)
// process application unicode input to terminal
// this is a trivial scanner
{
  c &= 0xff;
  switch (c)
  {
    case '\b'      : scr->backSpace();                 break;
    case '\t'      : scr->tab();                       break;
    case '\n'      : scr->newLine();                   break;
    case '\r'      : scr->toStartOfLine();             break;
    case 0x07      : emit notifySessionState(NOTIFYBELL);
                     break;
    default        : scr->displayCharacter(c);         break;
  };
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                             Keyboard Handling                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*!
*/
void TEmulation::onKeyPress( TQKeyEvent* ev )
{
  if (!listenToKeyPress) return; // someone else gets the keys

  // HACK - workaround for what looks like a bug in Qt.
  // Specifically keep track of when the meta button is pressed or released.
  // Upon restarting TDE, restored windows do not received the correct KeyEvent state
  // when multiple keys are pressed: the MetaButton is missing. 
  // Instead on new created window, MetaButton information is correct.
  // Ex:
  // Meta is pressed --> the state is correct, both before and after 
  //   State: Before=0x0000/After=0x0800 Key: 0x1022 
  // Then h is presed --> the state does not contain the MetaButton anymore
  //   State: Before=0x0000/After=0x0000 Key: 0x0048
  if (ev->key() == TQt::Key_Meta)
    metaIsPressed = true;
  
  doKeyPress(ev);
}

void TEmulation::doKeyPress( TQKeyEvent* ev )
{
  emit notifySessionState(NOTIFYNORMAL);

  if (scr->getHistCursor() != scr->getHistLines() && !ev->text().isEmpty())
    scr->setHistCursor(scr->getHistLines());
  if (!ev->text().isEmpty())
  { // A block of text
    // Note that the text is proper unicode.
    // We should do a conversion here, but since this
    // routine will never be used, we simply emit plain ascii.
    emit sndBlock(TQString(ev->text()).ascii(),ev->text().length());
  }
  else if (ev->ascii()>0)
  { unsigned char c[1];
    c[0] = ev->ascii();
    emit sndBlock((char*)c,1);
  }
}

void TEmulation::onKeyReleased( TQKeyEvent* ev )
{
  if (!listenToKeyPress) return; // someone else gets the keys

  // HACK - workaround for what looks like a bug in Qt.
  // Specifically keep track of when the meta button is pressed or released.
  // Upon restarting TDE, restored windows do not received the correct KeyEvent state
  // when multiple keys are pressed: the MetaButton is missing. 
  // Instead on new created window, MetaButton information is correct.
  // Ex:
  // Meta is pressed --> the state is correct, both before and after 
  //   State: Before=0x0000/After=0x0800 Key: 0x1022 
  // Then h is presed --> the state does not contain the MetaButton anymore
  //   State: Before=0x0000/After=0x0000 Key: 0x0048
  if (ev->key() == TQt::Key_Meta || !(ev->stateAfter() & TQt::MetaButton))
    metaIsPressed = false;
  
  doKeyReleased(ev);
}

void TEmulation::doKeyReleased( TQKeyEvent* ke )
{
}

void TEmulation::onFocusIn( TQFocusEvent* fe )
{
  // HACK - workaround for what looks like a bug in Qt.
  // Always reset the status of 'metaIsPressed' when the emulation gets the focus,
  // to avoid pending cases. A pending case is a case where the emulation lost the 
  // focus while Meta was pressed but gets the focus when Meta is no longer pressed.
  metaIsPressed = false;
  doFocusIn(fe);
}

void TEmulation::doFocusIn( TQFocusEvent* fe )
{
}

// Unblocking, Byte to Unicode translation --------------------------------- --

/*
   We are doing code conversion from locale to unicode first.
*/

void TEmulation::onRcvBlock(const char *s, int len)
{
  emit notifySessionState(NOTIFYACTIVITY);

  bulkStart();

  TQString r;
  int i, l;

  for (i = 0; i < len; i++)
  {
    // If we get a control code halfway a multi-byte sequence
    // we flush the decoder and continue with the control code.
    if ((unsigned char) s[i] < 32)
    {
       if (!r.length()) {
         TQString tmp;
         // Flush decoder
         while(!tmp.length())
             tmp = decoder->toUnicode(" ",1);
       }

       receiveChars((unsigned char) s[i]);

       if (s[i] == '\030' && (len-i-1 > 3) && (strncmp(s+i+1, "B00", 3) == 0))
         emit zmodemDetected();

       continue;
    }

    // Otherwise, bulk decode until the next control code
    for(l = i; l < len; l++)
      if ((unsigned char) s[l+1] < 32)
         break;

    r = decoder->toUnicode(&s[i],l-i+1);
    int reslen = r.length();

    for (int j = 0; j < reslen; j++)
    {
      if (r[j].category() == TQChar::Mark_NonSpacing)
         scr->compose(r.mid(j,1));
      else
         receiveChars(r[j].unicode());
    }
    i = l;
  }
}

// Selection --------------------------------------------------------------- --

void TEmulation::onSelectionBegin(const int x, const int y, const bool columnmode) {
  if (!connected) return;
  scr->setSelectionStart(x,y,columnmode);
  showBulk();
}

void TEmulation::onSelectionExtend(const int x, const int y) {
  if (!connected) return;
  scr->setSelectionEnd(x,y);
  showBulk();
}

void TEmulation::setSelection(const bool preserve_line_breaks) {
  if (!connected) return;
  TQString t = scr->selectedText(preserve_line_breaks);
  if (!t.isNull()) gui->setSelection(t);
}

void TEmulation::isBusySelecting(bool busy)
{
  if (!connected) return;
  scr->setBusySelecting(busy);
}

void TEmulation::testIsSelected(const int x, const int y, bool &selected)
{
  if (!connected) return;
  selected=scr->isSelected(x,y);
}

void TEmulation::clearSelection() {
  if (!connected) return;
  scr->clearSelection();
  showBulk();
}

void TEmulation::selectAll(bool wholeHistory) {
  if (!connected) return;
  scr->selectAll(wholeHistory);
  showBulk();
  setSelection(true);
}

void TEmulation::copySelection() {
  if (!connected) return;
  TQString t = scr->selectedText(true);
  TQApplication::clipboard()->setText(t);
}

TQString TEmulation::getSelection() {
  if (connected) return scr->selectedText(true);
  return TQString::null;
}

void TEmulation::streamHistory(TQTextStream* stream) {
  scr->streamHistory(stream);
}

void TEmulation::findTextBegin()
{
  m_findPos = -1;
}

bool TEmulation::findTextNext( const TQString &str, bool forward, bool caseSensitive, bool regExp )
{
  int pos = -1;
  TQString string;

  if (forward) {
    for (int i = (m_findPos==-1?0:m_findPos+1); i<(scr->getHistLines()+scr->getLines()); i++) {
      string = scr->getHistoryLine(i);
      if (regExp)
        pos = string.find( TQRegExp(str,caseSensitive) );
      else
        pos = string.find(str, 0, caseSensitive);
      if(pos!=-1) {
        m_findPos=i;
        if(i>scr->getHistLines())
          scr->setHistCursor(scr->getHistLines());
        else
          scr->setHistCursor(i);
        showBulk();
	return true;
      }
    }
  }
  else { // searching backwards
    for(int i = (m_findPos==-1?(scr->getHistLines()+scr->getLines()):m_findPos-1); i>=0; i--) {
      string = scr->getHistoryLine(i);
      if (regExp)
        pos = string.find( TQRegExp(str,caseSensitive) );
      else
        pos = string.find(str, 0, caseSensitive);
      if(pos!=-1) {
        m_findPos=i;
        if(i>scr->getHistLines())
          scr->setHistCursor(scr->getHistLines());
        else
          scr->setHistCursor(i);
        showBulk();
	return true;
      }
    }
  }

  return false;
}

// Refreshing -------------------------------------------------------------- --

#define BULK_TIMEOUT1 10
#define BULK_TIMEOUT2 40

/*!
*/

void TEmulation::showBulk()
{
  bulk_timer1.stop();
  bulk_timer2.stop();

  if (connected)
  {
    Character* image = scr->getCookedImage();    // get the image
    gui->setImage(image,
                  scr->getLines(),
                  scr->getColumns());     // actual refresh
    gui->setCursorPos(scr->getCursorX(), scr->getCursorY());	// set XIM position
    free(image);
    //FIXME: check that we do not trigger other draw event here.
    gui->setLineWrapped( scr->getCookedLineWrapped() );
    //kdDebug(1211)<<"TEmulation::showBulk(): setScroll()"<<endl;
    gui->setScroll(scr->getHistCursor(),scr->getHistLines());
    //kdDebug(1211)<<"TEmulation::showBulk(): setScroll() done"<<endl;
  }
}

void TEmulation::bulkStart()
{
   bulk_timer1.start(BULK_TIMEOUT1,true);
   if (!bulk_timer2.isActive())
      bulk_timer2.start(BULK_TIMEOUT2, true);
}

void TEmulation::setConnect(bool c)
{
   //kdDebug(1211)<<"TEmulation::setConnect()"<<endl;
  connected = c;
  if ( connected)
  {
    showBulk();
  }
}

char TEmulation::eraseChar()
{
  return '\b';
}

void TEmulation::setListenToKeyPress(bool l)
{
  listenToKeyPress=l;
}

// ---------------------------------------------------------------------------

/*!  triggered by image size change of the TEWidget `gui'.

    This event is simply propagated to the attached screens
    and to the related serial line.
*/

void TEmulation::onImageSizeChange(int lines, int columns)
{
  assert( lines > 0 && columns > 0 );

   //kdDebug(1211)<<"TEmulation::onImageSizeChange()"<<endl;
  screen[0]->resizeImage(lines,columns);
  screen[1]->resizeImage(lines,columns);
    
  if (!connected) return;
   //kdDebug(1211)<<"TEmulation::onImageSizeChange() showBulk()"<<endl;
  showBulk();
   //kdDebug(1211)<<"TEmulation::onImageSizeChange() showBulk() done"<<endl;
  emit ImageSizeChanged(columns, lines);   // propagate event
   //kdDebug(1211)<<"TEmulation::onImageSizeChange() done"<<endl;
}

TQSize TEmulation::imageSize()
{
  return TQSize(scr->getColumns(), scr->getLines());
}

void TEmulation::onHistoryCursorChange(int cursor)
{
  if (!connected) return;
  scr->setHistCursor(cursor);

  bulkStart();
}

void TEmulation::setColumns(int columns)
{
  //FIXME: this goes strange ways.
  //       Can we put this straight or explain it at least?
  emit changeColumns(columns);
}

}

#include "TEmulation.moc"
