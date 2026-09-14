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

/*! \class Vt102Emulation

   \brief Actual Emulation for Konsole

   \sa TEWidget \sa Screen
*/
#include "config.h"

// this allows konsole to be compiled without XKB and XTEST extensions
// even though it might be available on a particular system.
#if defined(AVOID_XKB)
#undef HAVE_XKB
#endif

#include <tdelocale.h>
#include <tdemessagebox.h>

#include "Vt102Emulation.h"
#include "TEWidget.h"
#include "Screen.h"

#include <stdio.h>
#include <unistd.h>

#include <assert.h>
#include <kdebug.h>

#if defined(HAVE_XKB)
static void scrolllock_set_off();
static void scrolllock_set_on();
#endif

namespace Konsole
{

/* VT102 Terminal Emulation

   This class puts together the screens, the pty and the widget to a
   complete terminal emulation. Beside combining it's componentes, it
   handles the emulations's protocol.

   This module consists of the following sections:

   - Constructor/Destructor
   - Incoming Bytes Event pipeline
   - Outgoing Bytes
     - Mouse Events
     - Keyboard Events
   - Modes and Charset State
   - Diagnostics
*/

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                       Constructor / Destructor                            */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*
   Nothing really intesting happens here.
*/

/*!
*/

Vt102Emulation::Vt102Emulation(TEWidget* gui) : TEmulation(gui)
{
  //kdDebug(1211)<<"Vt102Emulation ctor() connecting"<<endl;
  TQObject::connect(gui,TQ_SIGNAL(mouseSignal(int,int,int)),
                   this,TQ_SLOT(sendMouseEvent(int,int,int)));
  TQObject::connect(gui, TQ_SIGNAL(sendStringToEmu(const char*)),
		   this, TQ_SLOT(sendString(const char*)));
  //kdDebug(1211)<<"Vt102Emulation ctor() initToken..."<<endl;
  initTokenizer();
  //kdDebug(1211)<<"Vt102Emulation ctor() reset()"<<endl;
  reset();
  //kdDebug(1211)<<"Vt102Emulation ctor() ctor done"<<endl;
}

/*!
*/

void Vt102Emulation::changeGUI(TEWidget* newgui)
{
  if (static_cast<TEWidget *>( gui )==newgui) return;

  if ( gui ) {
    TQObject::disconnect(gui,TQ_SIGNAL(mouseSignal(int,int,int)),
                        this,TQ_SLOT(sendMouseEvent(int,int,int)));
    TQObject::disconnect(gui, TQ_SIGNAL(sendStringToEmu(const char*)),
                        this, TQ_SLOT(sendString(const char*)));
  }
  TEmulation::changeGUI(newgui);
  TQObject::connect(gui,TQ_SIGNAL(mouseSignal(int,int,int)),
                   this,TQ_SLOT(sendMouseEvent(int,int,int)));
  TQObject::connect(gui, TQ_SIGNAL(sendStringToEmu(const char*)),
		   this, TQ_SLOT(sendString(const char*)));
}

/*!
*/

Vt102Emulation::~Vt102Emulation()
{
}

/*!
*/

void Vt102Emulation::clearEntireScreen()
{
  scr->clearEntireScreen();
}

void Vt102Emulation::reset()
{
  //kdDebug(1211)<<"Vt102Emulation::reset() resetTokenizer()"<<endl;
  resetTokenizer();
  //kdDebug(1211)<<"Vt102Emulation::reset() resetModes()"<<endl;
  resetModes();
  //kdDebug(1211)<<"Vt102Emulation::reset() resetCharSet()"<<endl;
  resetCharset(0);
  //kdDebug(1211)<<"Vt102Emulation::reset() reset screen0()"<<endl;
  screen[0]->reset();
  //kdDebug(1211)<<"Vt102Emulation::reset() resetCharSet()"<<endl;
  resetCharset(1);
  //kdDebug(1211)<<"Vt102Emulation::reset() reset screen 1"<<endl;
  screen[1]->reset();
  //kdDebug(1211)<<"Vt102Emulation::reset() setCodec()"<<endl;
  setCodec(0);
  //kdDebug(1211)<<"Vt102Emulation::reset() done"<<endl;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                     Processing the incoming byte stream                   */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/* Incoming Bytes Event pipeline

   This section deals with decoding the incoming character stream.
   Decoding means here, that the stream is first seperated into `tokens'
   which are then mapped to a `meaning' provided as operations by the
   `Screen' class or by the emulation class itself.

   The pipeline proceeds as follows:

   - Tokenizing the ESC codes (receiveChars)
   - VT100 code page translation of plain characters (applyCharset)
   - Interpretation of ESC codes (processToken)

   The escape codes and their meaning are described in the
   technical reference of this program.
*/

// Tokens ------------------------------------------------------------------ --

/*
   Since the tokens are the central notion in this section, we've put them
   in front. They provide the syntactical elements used to represent the
   terminals operations as byte sequences.

   They are encoded here into a single machine word, so that we can later
   switch over them easily. Depending on the token itself, additional
   argument variables are filled with parameter values.

   The tokens are defined below:

   - CHR        - Printable characters     (32..255 but DEL (=127))
   - CTL        - Control characters       (0..31 but ESC (= 27), DEL)
   - ESC        - Escape codes of the form <ESC><CHR but `[]()+*#'>
   - ESC_DE     - Escape codes of the form <ESC><any of `()+*#%'> C
   - CSI_PN     - Escape codes of the form <ESC>'['     {Pn} ';' {Pn} C
   - CSI_PS     - Escape codes of the form <ESC>'['     {Pn} ';' ...  C
   - CSI_PR     - Escape codes of the form <ESC>'[' '?' {Pn} ';' ...  C
   - CSI_PE     - Escape codes of the form <ESC>'[' '!' {Pn} ';' ...  C
   - VT52       - VT52 escape codes
                  - <ESC><Chr>
                  - <ESC>'Y'{Pc}{Pc}
   - OSC        - Escape codes of the form <ESC>`]' {Pn} `;' {Text} <BEL>
                  note that this is handled differently

   The last two forms allow list of arguments. Since the elements of
   the lists are treated individually the same way, they are passed
   as individual tokens to the interpretation. Further, because the
   meaning of the parameters are names (althought represented as numbers),
   they are includes within the token ('N').

*/

#define token_construct(T,A,N) ( ((((int)N) & 0xffff) << 16) | ((((int)A) & 0xff) << 8) | (((int)T) & 0xff) )

#define token_chr(   )     token_construct(0,0,0)
#define token_ctl(A  )     token_construct(1,A,0)
#define token_esc(A  )     token_construct(2,A,0)
#define token_esc_cs(A,B)  token_construct(3,A,B)
#define token_esc_de(A  )  token_construct(4,A,0)
#define token_csi_ps(A,N)  token_construct(5,A,N)
#define token_csi_pn(A  )  token_construct(6,A,0)
#define token_csi_pr(A,N)  token_construct(7,A,N)

#define token_vt52(A  )    token_construct(8,A,0)

#define token_csi_pg(A  )  token_construct(9,A,0)

#define token_csi_pe(A  )  token_construct(10,A,0)

// Tokenizer --------------------------------------------------------------- --

/* The tokenizers state

   The state is represented by the buffer (m_tokenBuffer, m_tokenBufferPos),
   and accompanied by decoded arguments kept in params.
   Note that they are kept internal in the tokenizer.
*/

void Vt102Emulation::resetTokenizer()
{
  m_tokenBufferPos = 0;
  params.count = 0;
  params.value[0] = 0;
  params.value[1] = 0;
  params.sub[0].value[0] = 0;
  params.sub[0].count = 0;
}

void Vt102Emulation::addDigit(int dig)
{
  if (params.sub[params.count].count == 0) {
    params.value[params.count] = 10*params.value[params.count] + dig;
  } else {
    SubParam &sub = params.sub[params.count];
    sub.value[sub.count] = 10*sub.value[sub.count] + dig;
  }
}

void Vt102Emulation::addArgument()
{
  params.count = TQMIN(params.count+1,MAXARGS-1);
  params.value[params.count] = 0;
  params.sub[params.count].value[0] = 0;
  params.sub[params.count].count = 0;
}

void Vt102Emulation::addSubParam()
{
  SubParam &sub = params.sub[params.count];
  sub.count = TQMIN(sub.count+1,MAXARGS-1);
  sub.value[sub.count] = 0;
}

void Vt102Emulation::addToCurrentToken(int cc)
{
  m_tokenBuffer[m_tokenBufferPos] = cc;
  m_tokenBufferPos = TQMIN(m_tokenBufferPos+1,MAXPBUF-1);
}

// Character Classes used while decoding

#define CTL  1
#define CHR  2
#define CPN  4
#define DIG  8
#define SCS 16
#define GRP 32
#define CPS 64

void Vt102Emulation::initTokenizer()
{ int i; uint8_t* s;
  for(i =  0;                      i < 256; i++) m_charClass[ i]  = 0;
  for(i =  0;                      i <  32; i++) m_charClass[ i] |= CTL;
  for(i = 32;                      i < 256; i++) m_charClass[ i] |= CHR;
  for(s = (uint8_t*)"@ABCDEFGHILMPSTXZbcdfry"; *s; s++) m_charClass[*s] |= CPN;
// resize = \e[8;<row>;<col>t
  for(s = (uint8_t*)"t"; *s; s++) m_charClass[*s] |= CPS;
  for(s = (uint8_t*)"0123456789"        ; *s; s++) m_charClass[*s] |= DIG;
  for(s = (uint8_t*)"()+*%"             ; *s; s++) m_charClass[*s] |= SCS;
  for(s = (uint8_t*)"()+*#[]%"          ; *s; s++) m_charClass[*s] |= GRP;
  resetTokenizer();
}

/* Ok, here comes the nasty part of the decoder.

   Instead of keeping an explicit state, we deduce it from the
   token scanned so far. It is then immediately combined with
   the current character to form a scanning decision.

   This is done by the following defines.

   - P is the length of the token scanned so far.
   - L (often P-1) is the position on which contents we base a decision.
   - C is a character or a group of characters (taken from 'm_charClass').

   Note that they need to applied in proper order.
*/

#define lec(P,L,C) (p == (P) &&                     s[(L)]         == (C))
#define lun(     ) (p ==  1  &&                       cc           >= 32 )
#define les(P,L,C) (p == (P) && s[L] < 256  && (m_charClass[s[(L)]] & (C)) == (C))
#define eec(C)     (p >=  3  &&        cc                          == (C))
#define ees(C)     (p >=  3  && cc < 256 &&    (m_charClass[  cc  ] & (C)) == (C))
#define eps(C)     (p >=  3  && s[2] != '?' && s[2] != '!' && s[2] != '>' && cc < 256 && (m_charClass[  cc  ] & (C)) == (C))
#define epp( )     (p >=  3  && s[2] == '?'                              )
#define epe( )     (p >=  3  && s[2] == '!'                              )
#define egt(     ) (p >=  3  && s[2] == '>'                              )
#define Xpe        (m_tokenBufferPos>=2  && m_tokenBuffer[1] == ']'                           )
#define OSC        (Xpe                        &&     cc           ==  7 )
#define ces(C)     (            cc < 256 &&    (m_charClass[  cc  ] & (C)) == (C) && !OSC)

#define ESC 27
#define CNTL(c) ((c)-'@')

// process an incoming unicode character

void Vt102Emulation::receiveChars(int cc)
{ int i;
  if (cc == 127) return; //VT100: ignore.

  if (ces(    CTL))
  { // DEC HACK ALERT! Control Characters are allowed *within* esc sequences in VT100
    // This means, they do neither a resetTokenizer nor a addToCurrentToken. Some of them, do
    // of course. Guess this originates from a weakly layered handling of the X-on
    // X-off protocol, which comes really below this level.
    if (cc == CNTL('X') || cc == CNTL('Z') || cc == ESC) resetTokenizer(); //VT100: CAN or SUB
    if (cc != ESC)    { processToken( token_ctl(cc+'@' ),    0,   0); return; }
  }

  addToCurrentToken(cc); // advance the state

  int* s = m_tokenBuffer;
  int  p = m_tokenBufferPos;

  if (getMode(MODE_Ansi)) // decide on proper action
  {
    if (lec(1,0,ESC)) {                                                       return; }
    if (lec(1,0,ESC+128)) { s[0] = ESC; receiveChars('[');                       return; }
    if (les(2,1,GRP)) {                                                       return; }
    if (OSC         ) { OSC_sequence_handler();                 resetTokenizer(); return; }
    if (Xpe         ) {                                                       return; }
    if (lec(3,2,'?')) {                                                       return; }
    if (lec(3,2,'>')) {                                                       return; }
    if (lec(3,2,'!')) {                                                       return; }
    if (lun(       )) { processToken( token_chr(), applyCharset(cc), 0);    resetTokenizer(); return; }
    if (lec(2,0,ESC)) { processToken( token_esc(s[1]),    0,   0);          resetTokenizer(); return; }
    if (les(3,1,SCS)) { processToken( token_esc_cs(s[1],s[2]),    0,   0);  resetTokenizer(); return; }
    if (lec(3,1,'#')) { processToken( token_esc_de(s[2]),    0,   0);       resetTokenizer(); return; }
    if (eps(    CPN)) { processToken( token_csi_pn(cc), params.value[0],params.value[1]); resetTokenizer(); return; }

// resize = \e[8;<row>;<col>t
    if (eps(    CPS)) { processToken( token_csi_ps(cc, params.value[0]), params.value[1], params.value[2]);   resetTokenizer(); return; }

    if (epe(       )) { processToken( token_csi_pe(cc),      0,   0);       resetTokenizer(); return; }
    if (ees(    DIG)) { addDigit(cc-'0');                                     return; }
    if (eec(';'))     { addArgument();                                        return; }
    if (eec(':'))     { addSubParam();                                        return; }
    for (i=0;i<=params.count;i++)
    {
      if ( epp(     ))  { processToken( token_csi_pr(cc,params.value[i]),    0,   0); }
      else if(egt(    ))   { processToken( token_csi_pg(cc     ),    0,   0); } // spec. case for ESC]>0c or ESC]>c
      else if (cc == 'm' && (params.value[i] == 38 || params.value[i] == 48))
      {
        // CSI SGR 38/48
        const SubParam &sub = params.sub[i];
        if (params.count - i >= 4 && params.value[i+1] == 2)
        {
          // ESC[ ... {38/48};2;<red>;<green>;<blue> ... m
          int color = (params.value[i+2] << 16) | (params.value[i+3] << 8) | params.value[i+4];
          processToken(token_csi_ps(cc, params.value[i]), COLOR_SPACE_RGB, color);
          i += 4;
        }
        else if (sub.count >= 5 && sub.value[1] == 2)
        {
          // ESC[ ... {38/48}:2:<id>:<red>:<green>:<blue> ... m
          int color = (sub.value[3] << 16) | (sub.value[4] << 8) | sub.value[5];
          processToken(token_csi_ps(cc, params.value[i]), COLOR_SPACE_RGB, color);
        }
        else if (sub.count == 4 && sub.value[1] == 2)
        {
          // ESC[ ... {38/48}:2:<red>:<green>:<blue> ... m
          int color = (sub.value[2] << 16) | (sub.value[3] << 8) | sub.value[4];
          processToken(token_csi_ps(cc, params.value[i]), COLOR_SPACE_RGB, color);
        }
        else if (params.count - i >= 2 && params.value[i+1] == 5)
        {
          // ESC[ ... {38/48};5;<index> ... m
          processToken(token_csi_ps(cc, params.value[i]), COLOR_SPACE_256, params.value[i+2]);
          i += 2;
        }
        else if (sub.count >= 2 && sub.value[1] == 5)
        {
          // ESC[ ... {38/48}:5;<index> ... m
          processToken(token_csi_ps(cc, params.value[i]), COLOR_SPACE_256, sub.value[2]);
        }
      }
      else { processToken( token_csi_ps(cc,params.value[i]),    0,   0); }
    }
    resetTokenizer();
  }
  else // mode VT52
  {
    if (lec(1,0,ESC))                                                      return;
    if (les(1,0,CHR)) { processToken( token_chr(       ), s[0],   0); resetTokenizer(); return; }
    if (lec(2,1,'Y'))                                                      return;
    if (lec(3,1,'Y'))                                                      return;
    if (p < 4)        { processToken( token_vt52(s[1]   ),    0,   0); resetTokenizer(); return; }
                        processToken( token_vt52(s[1]   ), s[2],s[3]); resetTokenizer(); return;
  }
}

void Vt102Emulation::OSC_sequence_handler()
{
  int i,arg = 0;
  for (i = 2; i < m_tokenBufferPos && '0' <= m_tokenBuffer[i] && m_tokenBuffer[i] <= '9' ; i++)
  {
    arg = 10*arg + (m_tokenBuffer[i]-'0');
  }
  if (m_tokenBuffer[i++] != ';') { reportDecodingError(); return; }
  TQChar *str = new TQChar[m_tokenBufferPos-i-1];
  for (int j = 0; j < m_tokenBufferPos-i-1; j++) str[j] = m_tokenBuffer[i+j];
  TQString unistr(str, m_tokenBufferPos-i-1);
  // arg == 1 doesn't change the title. In XTerm it only changes the icon name
  // (btw: arg=0 changes title and icon, arg=1 only icon, arg=2 only title
  emit changeTitle(arg,unistr);
  delete [] str;
}

// Interpreting Codes ---------------------------------------------------------

/*
   Now that the incoming character stream is properly tokenized,
   meaning is assigned to them. These are either operations of
   the current screen, or of the emulation class itself.

   The token to be interpreteted comes in as a machine word
   possibly accompanied by two parameters.

   Likewise, the operations assigned to, come with up to two
   arguments. One could consider to make up a proper table
   from the function below.

   The technical reference manual provides more informations
   about this mapping.
*/

void Vt102Emulation::processToken( int token, int p, int q )
{
#if 0
int N = (token>>0)&0xff;
int A = (token>>8)&0xff;
switch( N )
{
   case 0: printf("%c", (p < 128) ? p : '?');
           break;
   case 1: if (A == 'J') printf("\r");
           else if (A == 'M') printf("\n");
           else printf("CTL-%c ", (token>>8)&0xff);
           break;
   case 2: printf("ESC-%c ", (token>>8)&0xff);
           break;
   case 3: printf("ESC_CS-%c-%c ", (token>>8)&0xff, (token>>16)&0xff);
           break;
   case 4: printf("ESC_DE-%c ", (token>>8)&0xff);
           break;
   case 5: printf("CSI-PS-%c-%d", (token>>8)&0xff, (token>>16)&0xff );
           break;
   case 6: printf("CSI-PN-%c [%d]", (token>>8)&0xff, p);
           break;
   case 7: printf("CSI-PR-%c-%d", (token>>8)&0xff, (token>>16)&0xff );
           break;
   case 8: printf("VT52-%c", (token>>8)&0xff);
           break;
   case 9: printf("CSI-PG-%c", (token>>8)&0xff);
           break;
   case 10: printf("CSI-PE-%c", (token>>8)&0xff);
           break;
}
#endif

  switch (token)
  {

    case token_chr(         ) : scr->displayCharacter     (p         ); break; //UTF16

    //             127 DEL    : ignored on input

    case token_ctl('@'      ) : /* NUL: ignored                      */ break;
    case token_ctl('A'      ) : /* SOH: ignored                      */ break;
    case token_ctl('B'      ) : /* STX: ignored                      */ break;
    case token_ctl('C'      ) : /* ETX: ignored                      */ break;
    case token_ctl('D'      ) : /* EOT: ignored                      */ break;
    case token_ctl('E'      ) :      reportAnswerBack     (          ); break; //VT100
    case token_ctl('F'      ) : /* ACK: ignored                      */ break;
    case token_ctl('G'      ) : emit notifySessionState(NOTIFYBELL);
                                break; //VT100
    case token_ctl('H'      ) : scr->backSpace            (          ); break; //VT100
    case token_ctl('I'      ) : scr->tab                  (          ); break; //VT100
    case token_ctl('J'      ) : scr->newLine              (          ); break; //VT100
    case token_ctl('K'      ) : scr->newLine              (          ); break; //VT100
    case token_ctl('L'      ) : scr->newLine              (          ); break; //VT100
    case token_ctl('M'      ) : scr->toStartOfLine        (          ); break; //VT100

    case token_ctl('N'      ) :      useCharset           (         1); break; //VT100
    case token_ctl('O'      ) :      useCharset           (         0); break; //VT100

    case token_ctl('P'      ) : /* DLE: ignored                      */ break;
    case token_ctl('Q'      ) : /* DC1: XON continue                 */ break; //VT100
    case token_ctl('R'      ) : /* DC2: ignored                      */ break;
    case token_ctl('S'      ) : /* DC3: XOFF halt                    */ break; //VT100
    case token_ctl('T'      ) : /* DC4: ignored                      */ break;
    case token_ctl('U'      ) : /* NAK: ignored                      */ break;
    case token_ctl('V'      ) : /* SYN: ignored                      */ break;
    case token_ctl('W'      ) : /* ETB: ignored                      */ break;
    case token_ctl('X'      ) : scr->displayCharacter     (    0x2592); break; //VT100
    case token_ctl('Y'      ) : /* EM : ignored                      */ break;
    case token_ctl('Z'      ) : scr->displayCharacter     (    0x2592); break; //VT100
    case token_ctl('['      ) : /* ESC: cannot be seen here.         */ break;
    case token_ctl('\\'     ) : /* FS : ignored                      */ break;
    case token_ctl(']'      ) : /* GS : ignored                      */ break;
    case token_ctl('^'      ) : /* RS : ignored                      */ break;
    case token_ctl('_'      ) : /* US : ignored                      */ break;

    case token_esc('D'      ) : scr->index                (          ); break; //VT100
    case token_esc('E'      ) : scr->nextLine             (          ); break; //VT100
    case token_esc('H'      ) : scr->changeTabStop        (true      ); break; //VT100
    case token_esc('M'      ) : scr->reverseIndex         (          ); break; //VT100
    case token_esc('Z'      ) :      reportTerminalType   (          ); break;
    case token_esc('c'      ) :      reset                (          ); break;

    case token_esc('n'      ) :      useCharset           (         2); break;
    case token_esc('o'      ) :      useCharset           (         3); break;
    case token_esc('7'      ) :      saveCursor           (          ); break;
    case token_esc('8'      ) :      restoreCursor        (          ); break;

    case token_esc('='      ) :          setMode      (MODE_AppKeyPad); break;
    case token_esc('>'      ) :        resetMode      (MODE_AppKeyPad); break;
    case token_esc('<'      ) :          setMode      (MODE_Ansi     ); break; //VT100

    case token_esc_cs('(',  '0') :      setCharset           (0,     '0'); break; //VT100
    case token_esc_cs('(',  'A') :      setCharset           (0,     'A'); break; //VT100
    case token_esc_cs('(',  'B') :      setCharset           (0,     'B'); break; //VT100

    case token_esc_cs(')',  '0') :      setCharset           (1,     '0'); break; //VT100
    case token_esc_cs(')',  'A') :      setCharset           (1,     'A'); break; //VT100
    case token_esc_cs(')',  'B') :      setCharset           (1,     'B'); break; //VT100

    case token_esc_cs('*',  '0') :      setCharset           (2,     '0'); break; //VT100
    case token_esc_cs('*',  'A') :      setCharset           (2,     'A'); break; //VT100
    case token_esc_cs('*',  'B') :      setCharset           (2,     'B'); break; //VT100

    case token_esc_cs('+',  '0') :      setCharset           (3,     '0'); break; //VT100
    case token_esc_cs('+',  'A') :      setCharset           (3,     'A'); break; //VT100
    case token_esc_cs('+',  'B') :      setCharset           (3,     'B'); break; //VT100

    case token_esc_cs('%',  'G') :      setCodec             (1         ); break; //LINUX
    case token_esc_cs('%',  '@') :      setCodec             (0         ); break; //LINUX

    case token_esc_de('3'      ) : /* IGNORED: double high, top half    */ break;
    case token_esc_de('4'      ) : /* IGNORED: double high, bottom half */ break;
    case token_esc_de('5'      ) : /* IGNORED: single width, single high*/ break;
    case token_esc_de('6'      ) : /* IGNORED: double width, single high*/ break;
    case token_esc_de('8'      ) : scr->helpAlign            (          ); break;

// resize = \e[8;<row>;<col>t
    case token_csi_ps('t',    8) : changeColLin( q /* col */, p /* lin */ ); break;

// change tab text color : \e[28;<color>t  color: 0-16,777,215
    case token_csi_ps('t',    28) : emit changeTabTextColor   ( p        ); break;

    case token_csi_ps('K',    0) : scr->clearToEndOfLine     (          ); break;
    case token_csi_ps('K',    1) : scr->clearToBeginOfLine   (          ); break;
    case token_csi_ps('K',    2) : scr->clearEntireLine      (          ); break;
    case token_csi_ps('J',    0) : scr->clearToEndOfScreen   (          ); break;
    case token_csi_ps('J',    1) : scr->clearToBeginOfScreen (          ); break;
    case token_csi_ps('J',    2) : scr->clearEntireScreen    (          ); break;
    case token_csi_ps('g',    0) : scr->changeTabStop        (false     ); break; //VT100
    case token_csi_ps('g',    3) : scr->clearTabStops        (          ); break; //VT100
    case token_csi_ps('h',    4) : scr->    setMode      (MODE_Insert   ); break;
    case token_csi_ps('h',   20) :          setMode      (MODE_NewLine  ); break;
    case token_csi_ps('i',    0) : /* IGNORE: attached printer          */ break; //VT100
    case token_csi_ps('l',    4) : scr->  resetMode      (MODE_Insert   ); break;
    case token_csi_ps('l',   20) :        resetMode      (MODE_NewLine  ); break;
    case token_csi_ps('s',    0) :      saveCursor           (          ); break;
    case token_csi_ps('u',    0) :      restoreCursor        (          ); break;

    case token_csi_ps('m',    0) : scr->setDefaultRendition  (          ); break;
    case token_csi_ps('m',    1) : scr->  setRendition     (RE_BOLD     ); break; //VT100
    case token_csi_ps('m',    4) : scr->  setRendition     (RE_UNDERLINE); break; //VT100
    case token_csi_ps('m',    5) : scr->  setRendition     (RE_BLINK    ); break; //VT100
    case token_csi_ps('m',    7) : scr->  setRendition     (RE_REVERSE  ); break;
    case token_csi_ps('m',   10) : /* IGNORED: mapping related          */ break; //LINUX
    case token_csi_ps('m',   11) : /* IGNORED: mapping related          */ break; //LINUX
    case token_csi_ps('m',   12) : /* IGNORED: mapping related          */ break; //LINUX
    case token_csi_ps('m',   22) : scr->resetRendition     (RE_BOLD     ); break;
    case token_csi_ps('m',   24) : scr->resetRendition     (RE_UNDERLINE); break;
    case token_csi_ps('m',   25) : scr->resetRendition     (RE_BLINK    ); break;
    case token_csi_ps('m',   27) : scr->resetRendition     (RE_REVERSE  ); break;

    case token_csi_ps('m',   30) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  0); break;
    case token_csi_ps('m',   31) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  1); break;
    case token_csi_ps('m',   32) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  2); break;
    case token_csi_ps('m',   33) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  3); break;
    case token_csi_ps('m',   34) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  4); break;
    case token_csi_ps('m',   35) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  5); break;
    case token_csi_ps('m',   36) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  6); break;
    case token_csi_ps('m',   37) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  7); break;

    case token_csi_ps('m',   38) : scr->setForeColor         (p,       q); break;

    case token_csi_ps('m',   39) : scr->setForeColor         (COLOR_SPACE_DEFAULT,  0); break;

    case token_csi_ps('m',   40) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  0); break;
    case token_csi_ps('m',   41) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  1); break;
    case token_csi_ps('m',   42) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  2); break;
    case token_csi_ps('m',   43) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  3); break;
    case token_csi_ps('m',   44) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  4); break;
    case token_csi_ps('m',   45) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  5); break;
    case token_csi_ps('m',   46) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  6); break;
    case token_csi_ps('m',   47) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  7); break;

    case token_csi_ps('m',   48) : scr->setBackColor         (p,       q); break;

    case token_csi_ps('m',   49) : scr->setBackColor         (COLOR_SPACE_DEFAULT,  1); break;

    case token_csi_ps('m',   90) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  8); break;
    case token_csi_ps('m',   91) : scr->setForeColor         (COLOR_SPACE_SYSTEM,  9); break;
    case token_csi_ps('m',   92) : scr->setForeColor         (COLOR_SPACE_SYSTEM, 10); break;
    case token_csi_ps('m',   93) : scr->setForeColor         (COLOR_SPACE_SYSTEM, 11); break;
    case token_csi_ps('m',   94) : scr->setForeColor         (COLOR_SPACE_SYSTEM, 12); break;
    case token_csi_ps('m',   95) : scr->setForeColor         (COLOR_SPACE_SYSTEM, 13); break;
    case token_csi_ps('m',   96) : scr->setForeColor         (COLOR_SPACE_SYSTEM, 14); break;
    case token_csi_ps('m',   97) : scr->setForeColor         (COLOR_SPACE_SYSTEM, 15); break;

    case token_csi_ps('m',  100) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  8); break;
    case token_csi_ps('m',  101) : scr->setBackColor         (COLOR_SPACE_SYSTEM,  9); break;
    case token_csi_ps('m',  102) : scr->setBackColor         (COLOR_SPACE_SYSTEM, 10); break;
    case token_csi_ps('m',  103) : scr->setBackColor         (COLOR_SPACE_SYSTEM, 11); break;
    case token_csi_ps('m',  104) : scr->setBackColor         (COLOR_SPACE_SYSTEM, 12); break;
    case token_csi_ps('m',  105) : scr->setBackColor         (COLOR_SPACE_SYSTEM, 13); break;
    case token_csi_ps('m',  106) : scr->setBackColor         (COLOR_SPACE_SYSTEM, 14); break;
    case token_csi_ps('m',  107) : scr->setBackColor         (COLOR_SPACE_SYSTEM, 15); break;

    case token_csi_ps('n',    5) :      reportStatus         (          ); break;
    case token_csi_ps('n',    6) :      reportCursorPosition (          ); break;
    case token_csi_ps('q',    0) : /* IGNORED: LEDs off                 */ break; //VT100
    case token_csi_ps('q',    1) : /* IGNORED: LED1 on                  */ break; //VT100
    case token_csi_ps('q',    2) : /* IGNORED: LED2 on                  */ break; //VT100
    case token_csi_ps('q',    3) : /* IGNORED: LED3 on                  */ break; //VT100
    case token_csi_ps('q',    4) : /* IGNORED: LED4 on                  */ break; //VT100
    case token_csi_ps('x',    0) :      reportTerminalParms  (         2); break; //VT100
    case token_csi_ps('x',    1) :      reportTerminalParms  (         3); break; //VT100

    case token_csi_pn('@'      ) : scr->insertChars          (p         ); break;
    case token_csi_pn('A'      ) : scr->cursorUp             (p         ); break; //VT100
    case token_csi_pn('B'      ) : scr->cursorDown           (p         ); break; //VT100
    case token_csi_pn('C'      ) : scr->cursorRight          (p         ); break; //VT100
    case token_csi_pn('D'      ) : scr->cursorLeft           (p         ); break; //VT100
    case token_csi_pn('E'      ) : scr->cursorNextLine       (p         ); break; //VT100
    case token_csi_pn('F'      ) : scr->cursorPrevLine       (p         ); break; //VT100
    case token_csi_pn('G'      ) : scr->setCursorX           (p         ); break; //LINUX
    case token_csi_pn('H'      ) : scr->setCursorYX          (p,       q); break; //VT100
    case token_csi_pn('I'      ) : scr->tab                  (p         ); break;
    case token_csi_pn('L'      ) : scr->insertLines          (p         ); break;
    case token_csi_pn('M'      ) : scr->deleteLines          (p         ); break;
    case token_csi_pn('P'      ) : scr->deleteChars          (p         ); break;
    case token_csi_pn('S'      ) : scr->scrollUp             (p         ); break;
    case token_csi_pn('T'      ) : scr->scrollDown           (p         ); break;
    case token_csi_pn('X'      ) : scr->eraseChars           (p         ); break;
    case token_csi_pn('Z'      ) : scr->backTab              (p         ); break;
    case token_csi_pn('b'      ) : scr->repeatChars          (p         ); break;
    case token_csi_pn('c'      ) :      reportTerminalType   (          ); break; //VT100
    case token_csi_pn('d'      ) : scr->setCursorY           (p         ); break; //LINUX
    case token_csi_pn('f'      ) : scr->setCursorYX          (p,       q); break; //VT100
    case token_csi_pn('r'      ) :      setMargins           (p,       q); break; //VT100
    case token_csi_pn('y'      ) : /* IGNORED: Confidence test          */ break; //VT100

    case token_csi_pr('h',    1) :          setMode      (MODE_AppCuKeys); break; //VT100
    case token_csi_pr('l',    1) :        resetMode      (MODE_AppCuKeys); break; //VT100
    case token_csi_pr('s',    1) :         saveMode      (MODE_AppCuKeys); break; //FIXME
    case token_csi_pr('r',    1) :      restoreMode      (MODE_AppCuKeys); break; //FIXME

    case token_csi_pr('l',    2) :        resetMode      (MODE_Ansi     ); break; //VT100

    case token_csi_pr('h',    3) :                setColumns (       132); break; //VT100
    case token_csi_pr('l',    3) :                setColumns (        80); break; //VT100

    case token_csi_pr('h',    4) : /* IGNORED: soft scrolling           */ break; //VT100
    case token_csi_pr('l',    4) : /* IGNORED: soft scrolling           */ break; //VT100

    case token_csi_pr('h',    5) : scr->    setMode      (MODE_Screen   ); break; //VT100
    case token_csi_pr('l',    5) : scr->  resetMode      (MODE_Screen   ); break; //VT100

    case token_csi_pr('h',    6) : scr->    setMode      (MODE_Origin   ); break; //VT100
    case token_csi_pr('l',    6) : scr->  resetMode      (MODE_Origin   ); break; //VT100
    case token_csi_pr('s',    6) : scr->   saveMode      (MODE_Origin   ); break; //FIXME
    case token_csi_pr('r',    6) : scr->restoreMode      (MODE_Origin   ); break; //FIXME

    case token_csi_pr('h',    7) : scr->    setMode      (MODE_Wrap     ); break; //VT100
    case token_csi_pr('l',    7) : scr->  resetMode      (MODE_Wrap     ); break; //VT100
    case token_csi_pr('s',    7) : scr->   saveMode      (MODE_Wrap     ); break; //FIXME
    case token_csi_pr('r',    7) : scr->restoreMode      (MODE_Wrap     ); break; //FIXME

    case token_csi_pr('h',    8) : /* IGNORED: autorepeat on            */ break; //VT100
    case token_csi_pr('l',    8) : /* IGNORED: autorepeat off           */ break; //VT100
    case token_csi_pr('s',    8) : /* IGNORED: autorepeat on            */ break; //VT100
    case token_csi_pr('r',    8) : /* IGNORED: autorepeat off           */ break; //VT100

    case token_csi_pr('h',    9) : /* IGNORED: interlace                */ break; //VT100
    case token_csi_pr('l',    9) : /* IGNORED: interlace                */ break; //VT100
    case token_csi_pr('s',    9) : /* IGNORED: interlace                */ break; //VT100
    case token_csi_pr('r',    9) : /* IGNORED: interlace                */ break; //VT100

    case token_csi_pr('h',   12) : /* IGNORED: Cursor blink             */ break; //att610
    case token_csi_pr('l',   12) : /* IGNORED: Cursor blink             */ break; //att610
    case token_csi_pr('s',   12) : /* IGNORED: Cursor blink             */ break; //att610
    case token_csi_pr('r',   12) : /* IGNORED: Cursor blink             */ break; //att610

    case token_csi_pr('h',   25) :          setMode      (MODE_Cursor   ); break; //VT100
    case token_csi_pr('l',   25) :        resetMode      (MODE_Cursor   ); break; //VT100
    case token_csi_pr('s',   25) :         saveMode      (MODE_Cursor   ); break; //VT100
    case token_csi_pr('r',   25) :      restoreMode      (MODE_Cursor   ); break; //VT100

    case token_csi_pr('h',   41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM
    case token_csi_pr('l',   41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM
    case token_csi_pr('s',   41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM
    case token_csi_pr('r',   41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM

    case token_csi_pr('h',   47) :          setMode      (MODE_AppScreen); break; //VT100
    case token_csi_pr('l',   47) :        resetMode      (MODE_AppScreen); break; //VT100
    case token_csi_pr('s',   47) :         saveMode      (MODE_AppScreen); break; //XTERM
    case token_csi_pr('r',   47) :      restoreMode      (MODE_AppScreen); break; //XTERM

    case token_csi_pr('h',   67) : /* IGNORED: DECBKM                   */ break; //XTERM
    case token_csi_pr('l',   67) : /* IGNORED: DECBKM                   */ break; //XTERM
    case token_csi_pr('s',   67) : /* IGNORED: DECBKM                   */ break; //XTERM
    case token_csi_pr('r',   67) : /* IGNORED: DECBKM                   */ break; //XTERM

    // XTerm defines the following modes:
    // SET_VT200_MOUSE             1000
    // SET_VT200_HIGHLIGHT_MOUSE   1001
    // SET_BTN_EVENT_MOUSE         1002
    // SET_ANY_EVENT_MOUSE         1003
    //
    // FIXME: Modes 1000,1002 and 1003 have subtle differences which we don't
    // support yet, we treat them all the same.

    case token_csi_pr('h', 1000) :          setMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('l', 1000) :        resetMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('s', 1000) :         saveMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('r', 1000) :      restoreMode      (MODE_Mouse1000); break; //XTERM

    case token_csi_pr('h', 1001) : /* IGNORED: hilite mouse tracking    */ break; //XTERM
    case token_csi_pr('l', 1001) :        resetMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('s', 1001) : /* IGNORED: hilite mouse tracking    */ break; //XTERM
    case token_csi_pr('r', 1001) : /* IGNORED: hilite mouse tracking    */ break; //XTERM

    case token_csi_pr('h', 1002) :          setMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('l', 1002) :        resetMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('s', 1002) :         saveMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('r', 1002) :      restoreMode      (MODE_Mouse1000); break; //XTERM

    case token_csi_pr('h', 1003) :          setMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('l', 1003) :        resetMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('s', 1003) :         saveMode      (MODE_Mouse1000); break; //XTERM
    case token_csi_pr('r', 1003) :      restoreMode      (MODE_Mouse1000); break; //XTERM

    case token_csi_pr('h', 1047) :          setMode      (MODE_AppScreen); break; //XTERM
    case token_csi_pr('l', 1047) : screen[1]->clearEntireScreen(); resetMode(MODE_AppScreen); break; //XTERM
    case token_csi_pr('s', 1047) :         saveMode      (MODE_AppScreen); break; //XTERM
    case token_csi_pr('r', 1047) :      restoreMode      (MODE_AppScreen); break; //XTERM

    //FIXME: Unitoken: save translations
    case token_csi_pr('h', 1048) :      saveCursor           (          ); break; //XTERM
    case token_csi_pr('l', 1048) :      restoreCursor        (          ); break; //XTERM
    case token_csi_pr('s', 1048) :      saveCursor           (          ); break; //XTERM
    case token_csi_pr('r', 1048) :      restoreCursor        (          ); break; //XTERM

    //FIXME: every once new sequences like this pop up in xterm.
    //       Here's a guess of what they could mean.
    case token_csi_pr('h', 1049) : saveCursor(); screen[1]->clearEntireScreen(); setMode(MODE_AppScreen); break; //XTERM
    case token_csi_pr('l', 1049) : resetMode(MODE_AppScreen); restoreCursor(); break; //XTERM

    //FIXME: weird DEC reset sequence
    case token_csi_pe('p'      ) : /* IGNORED: reset         (        ) */ break;

    //FIXME: when changing between vt52 and ansi mode evtl do some resetting.
    case token_vt52('A'      ) : scr->cursorUp             (         1); break; //VT52
    case token_vt52('B'      ) : scr->cursorDown           (         1); break; //VT52
    case token_vt52('C'      ) : scr->cursorRight          (         1); break; //VT52
    case token_vt52('D'      ) : scr->cursorLeft           (         1); break; //VT52

    case token_vt52('F'      ) :      setAndUseCharset     (0,     '0'); break; //VT52
    case token_vt52('G'      ) :      setAndUseCharset     (0,     'B'); break; //VT52

    case token_vt52('H'      ) : scr->setCursorYX          (1,1       ); break; //VT52
    case token_vt52('I'      ) : scr->reverseIndex         (          ); break; //VT52
    case token_vt52('J'      ) : scr->clearToEndOfScreen   (          ); break; //VT52
    case token_vt52('K'      ) : scr->clearToEndOfLine     (          ); break; //VT52
    case token_vt52('Y'      ) : scr->setCursorYX          (p-31,q-31 ); break; //VT52
    case token_vt52('Z'      ) :      reportTerminalType   (           ); break; //VT52
    case token_vt52('<'      ) :          setMode      (MODE_Ansi     ); break; //VT52
    case token_vt52('='      ) :          setMode      (MODE_AppKeyPad); break; //VT52
    case token_vt52('>'      ) :        resetMode      (MODE_AppKeyPad); break; //VT52

    case token_csi_pg('c'      ) :  reportSecondaryAttributes(          ); break; //VT100

    default : reportDecodingError();    break;
  };
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                          Terminal to Host protocol                        */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/* 
   Outgoing bytes originate from several sources:

   - Replies to Enquieries.
   - Mouse Events
   - Keyboard Events
*/

/*!
*/

void Vt102Emulation::sendString(const char* s)
{
  emit sndBlock(s,strlen(s));
}

// Replies ----------------------------------------------------------------- --

// This section copes with replies send as response to an enquiery control code.

/*!
*/

void Vt102Emulation::reportCursorPosition()
{ char tmp[20];
  sprintf(tmp,"\033[%d;%dR",scr->getCursorY()+1,scr->getCursorX()+1);
  sendString(tmp);
}

/*
   What follows here is rather obsolete and faked stuff.
   The correspondent enquieries are neverthenless issued.
*/

/*!
*/

void Vt102Emulation::reportTerminalType()
{
  // Primary device attribute response (Request was: ^[[0c or ^[[c (from TT321 Users Guide))
  //   VT220:  ^[[?63;1;2;3;6;7;8c   (list deps on emul. capabilities)
  //   VT100:  ^[[?1;2c
  //   VT101:  ^[[?1;0c
  //   VT102:  ^[[?6v
  if (getMode(MODE_Ansi))
    sendString("\033[?1;2c");     // I'm a VT100
  else
    sendString("\033/Z");         // I'm a VT52
}

void Vt102Emulation::reportSecondaryAttributes()
{
  // Seconday device attribute response (Request was: ^[[>0c or ^[[>c)
  if (getMode(MODE_Ansi))
    sendString("\033[>0;115;0c"); // Why 115?  ;)
  else
    sendString("\033/Z");         // FIXME I don't think VT52 knows about it but kept for
                                  // konsoles backward compatibility.
}

void Vt102Emulation::reportTerminalParms(int p)
// DECREPTPARM
{ char tmp[100];
  sprintf(tmp,"\033[%d;1;1;112;112;1;0x",p); // not really true.
  sendString(tmp);
}

/*!
*/

void Vt102Emulation::reportStatus()
{
  sendString("\033[0n"); //VT100. Device status report. 0 = Ready.
}

/*!
*/

#define ANSWER_BACK "" // This is really obsolete VT100 stuff.

void Vt102Emulation::reportAnswerBack()
{
  sendString(ANSWER_BACK);
}

// Mouse Handling ---------------------------------------------------------- --

/*!
    Mouse clicks are possibly reported to the client
    application if it has issued interest in them.
    They are normally consumed by the widget for copy
    and paste, but may be propagated from the widget
    when gui->setMouseMarks is set via setMode(MODE_Mouse1000).

    `x',`y' are 1-based.
    `ev' (event) indicates the button pressed (0-2)
                 or a general mouse release (3).
*/

void Vt102Emulation::sendMouseEvent( int cb, int cx, int cy )
{ char tmp[20];
  if (!connected || cx<1 || cy<1) return;
  // normal buttons are passed as 0x20 + button,
  // mouse wheel (buttons 4,5) as 0x5c + button
  if (cb >= 4) cb += 0x3c;
  sprintf(tmp,"\033[M%c%c%c",cb+0x20,cx+0x20,cy+0x20);
  sendString(tmp);
}

// Keyboard Handling ------------------------------------------------------- --

void Vt102Emulation::scrollLock(const bool lock)
{
  if (lock)
  {
    m_holdScreen = true;
    emit lockPty(true);
  }
  else
  {
    m_holdScreen = false;
    emit lockPty(false);
  }
#if defined(HAVE_XKB)
  if (m_holdScreen)
    scrolllock_set_on();
  else
    scrolllock_set_off();
#endif
}

void Vt102Emulation::onScrollLock()
{
  bool switchlock = !m_holdScreen;
  scrollLock(switchlock);
}

#define encodeMode(M,B) BITS(B,getMode(M))
#define encodeStat(M,B) BITS(B,((ev->state() & (M)) == (M)))

/*
   Keyboard event handling has been simplified somewhat by pushing
   the complications towards a configuration file [see KeyTrans class].
*/

void Vt102Emulation::doKeyPress( TQKeyEvent* ev )
{
  emit notifySessionState(NOTIFYNORMAL);

  //printf("State/Key: 0x%04x 0x%04x (%d,%d)\n", ev->state(),ev->key(),
  //       ev->text().length(),ev->text().length()?ev->text().ascii()[0]:0);

  // lookup in keyboard translation table ...
  int cmd = CMD_none; 
  const char *txt; 
  int len;
  bool metaspecified;
  int bits = encodeMode(MODE_NewLine		  , BITS_NewLine   ) + // OLD,
             encodeMode(MODE_Ansi		      , BITS_Ansi      ) + // OBSOLETE,
             encodeMode(MODE_AppCuKeys		, BITS_AppCuKeys ) + // VT100 stuff
             encodeMode(MODE_AppScreen		, BITS_AppScreen ) + // VT100 stuff
             encodeStat(TQt::ControlButton, BITS_Control   ) +
             encodeStat(TQt::ShiftButton	, BITS_Shift     ) +
             encodeStat(TQt::AltButton		, BITS_Alt       );
  if (metaKeyMode)
    bits += encodeStat(TQt::MetaButton , BITS_Alt);
  keytrans->findEntry(ev->key(), bits, &cmd, &txt, &len, &metaspecified);
  if (connected)
  {
    switch(cmd) // ... and execute if found.
    {
      case CMD_scrollPageUp   : gui->doScroll(-gui->Lines()/2); return;
      case CMD_scrollPageDown : gui->doScroll(+gui->Lines()/2); return;
      case CMD_scrollLineUp   : gui->doScroll(-1             ); return;
      case CMD_scrollLineDown : gui->doScroll(+1             ); return;
      case CMD_scrollLock     : onScrollLock(                ); return;
    }
  }
  if (m_holdScreen)
  {
    switch(ev->key())
    {
    case TQt::Key_Down : gui->doScroll(+1); return;
    case TQt::Key_Up : gui->doScroll(-1); return;
    case TQt::Key_PageUp : gui->doScroll(-gui->Lines()/2); return;
    case TQt::Key_PageDown : gui->doScroll(gui->Lines()/2); return;
    }
  }
  
  // revert to non-history when typing
  if (scr->getHistCursor() != scr->getHistLines() && (!ev->text().isEmpty()
    || ev->key()==TQt::Key_Down || ev->key()==TQt::Key_Up || ev->key()==TQt::Key_Left || ev->key()==TQt::Key_Right
    || ev->key()==TQt::Key_PageUp || ev->key()==TQt::Key_PageDown))
    scr->setHistCursor(scr->getHistLines());

  if (cmd==CMD_send)
  {
    if ((ev->state() & TQt::AltButton) || 
        (metaKeyMode && ((ev->state() & TQt::MetaButton) || metaIsPressed) && !metaspecified))
      sendString("\033");
    emit sndBlock(txt,len);
    return;
  }

  // fall back handling
  if (!ev->text().isEmpty())
  {
    if ((ev->state() & TQt::AltButton) || 
        (metaKeyMode && ((ev->state() & TQt::MetaButton) || metaIsPressed)))
      sendString("\033"); // ESC, this is the ALT prefix
    TQCString s = m_codec->fromUnicode(ev->text());     // encode for application
    // FIXME: In Qt 2, TQKeyEvent::text() would return "\003" for Ctrl-C etc.
    //        while in Qt 3 it returns the actual key ("c" or "C") which caused
    //        the ControlButton to be ignored. This hack seems to work for
    //        latin1 locales at least. Please anyone find a clean solution (malte)
    if (ev->state() & TQt::ControlButton)
      s.fill(ev->ascii(), 1);
    emit sndBlock(s.data(),s.length());              // we may well have s.length() > 1 
    return;
  }
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                VT100 Charsets                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

// Character Set Conversion ------------------------------------------------ --

/* 
   The processing contains a VT100 specific code translation layer.
   It's still in use and mainly responsible for the line drawing graphics.

   These and some other glyphs are assigned to codes (0x5f-0xfe)
   normally occupied by the latin letters. Since this codes also
   appear within control sequences, the extra code conversion
   does not permute with the tokenizer and is placed behind it
   in the pipeline. It only applies to tokens, which represent
   plain characters.

   This conversion it eventually continued in TEWidget.C, since 
   it might involve VT100 enhanced fonts, which have these
   particular glyphs allocated in (0x00-0x1f) in their code page.
*/

#define CHARSET m_charset[scr==screen[1]]

// Apply current character map.

unsigned short Vt102Emulation::applyCharset(unsigned short c)
{
  if (CHARSET.graphic && 0x5f <= c && c <= 0x7e) return vt100_graphics[c-0x5f];
  if (CHARSET.pound                && c == '#' ) return 0xa3; //This mode is obsolete
  return c;
}

/*
   "Charset" related part of the emulation state.
   This configures the VT100 charset filter.

   While most operation work on the current screen,
   the following two are different.
*/

void Vt102Emulation::resetCharset(int scrno)
{
  m_charset[scrno].cu_cs   = 0;
  strncpy(m_charset[scrno].charset,"BBBB",4);
  m_charset[scrno].sa_graphic = false;
  m_charset[scrno].sa_pound   = false;
  m_charset[scrno].graphic = false;
  m_charset[scrno].pound   = false;
}

/*!
*/

void Vt102Emulation::setCharset(int n, int cs) // on both screens.
{
  m_charset[0].charset[n&3] = cs; useCharset(m_charset[0].cu_cs);
  m_charset[1].charset[n&3] = cs; useCharset(m_charset[1].cu_cs);
}

/*!
*/

void Vt102Emulation::setAndUseCharset(int n, int cs)
{
  CHARSET.charset[n&3] = cs;
  useCharset(n&3);
}

/*!
*/

void Vt102Emulation::useCharset(int n)
{
  CHARSET.cu_cs   = n&3;
  CHARSET.graphic = (CHARSET.charset[n&3] == '0');
  CHARSET.pound   = (CHARSET.charset[n&3] == 'A'); //This mode is obsolete
}

void Vt102Emulation::setMargins(int t, int b)
{
  screen[0]->setMargins(t, b);
  screen[1]->setMargins(t, b);
}

/*! Save the cursor position and the rendition attribute settings. */

void Vt102Emulation::saveCursor()
{
  CHARSET.sa_graphic = CHARSET.graphic;
  CHARSET.sa_pound   = CHARSET.pound; //This mode is obsolete
  // we are not clear about these
  //sa_charset = charsets[cScreen->charset];
  //sa_charset_num = cScreen->charset;
  scr->saveCursor();
}

/*! Restore the cursor position and the rendition attribute settings. */

void Vt102Emulation::restoreCursor()
{
  CHARSET.graphic = CHARSET.sa_graphic;
  CHARSET.pound   = CHARSET.sa_pound; //This mode is obsolete
  scr->restoreCursor();
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                                Mode Operations                            */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*
   Some of the emulations state is either added to the state of the screens.

   This causes some scoping problems, since different emulations choose to
   located the mode either to the current screen or to both.

   For strange reasons, the extend of the rendition attributes ranges over
   all screens and not over the actual screen.

   We decided on the precise precise extend, somehow.
*/

// "Mode" related part of the state. These are all booleans.

void Vt102Emulation::resetModes()
{
  resetMode(MODE_Mouse1000); saveMode(MODE_Mouse1000);
  resetMode(MODE_AppScreen); saveMode(MODE_AppScreen);
  // here come obsolete modes
  resetMode(MODE_AppCuKeys); saveMode(MODE_AppCuKeys);
  resetMode(MODE_NewLine  );
    setMode(MODE_Ansi     );
  m_holdScreen = false;
}

void Vt102Emulation::setMode(int m)
{
  m_currentModes.mode[m] = true;
  switch (m)
  {
    case MODE_Mouse1000 : if (connected) gui->setMouseMarks(false);
    break;

    case MODE_AppScreen : screen[1]->clearSelection();
                          setScreen(1);
    break;
  }
  if (m < MODES_SCREEN || m == MODE_NewLine)
  {
    screen[0]->setMode(m);
    screen[1]->setMode(m);
  }
}

void Vt102Emulation::resetMode(int m)
{
  m_currentModes.mode[m] = false;
  switch (m)
  {
    case MODE_Mouse1000 : if (connected) gui->setMouseMarks(true);
    break;

    case MODE_AppScreen : screen[0]->clearSelection();
                          setScreen(0);
    break;
  }
  if (m < MODES_SCREEN || m == MODE_NewLine)
  {
    screen[0]->resetMode(m);
    screen[1]->resetMode(m);
  }
}

void Vt102Emulation::saveMode(int m)
{
  m_savedModes.mode[m] = m_currentModes.mode[m];
}

void Vt102Emulation::restoreMode(int m)
{
  if(m_savedModes.mode[m]) setMode(m); else resetMode(m);
}

bool Vt102Emulation::getMode(int m)
{
  return m_currentModes.mode[m];
}

void Vt102Emulation::setConnect(bool c)
{
  TEmulation::setConnect(c);
  if (gui)
  {
    TQObject::disconnect(gui, TQ_SIGNAL(sendStringToEmu(const char*)),
                        this, TQ_SLOT(sendString(const char*)));
  }
  if (c)
  { // refresh mouse mode
    if (getMode(MODE_Mouse1000))
      setMode(MODE_Mouse1000);
    else
      resetMode(MODE_Mouse1000);
#if defined(HAVE_XKB)
    if (m_holdScreen)
      scrolllock_set_on();
    else
      scrolllock_set_off();
#endif
    TQObject::connect(gui, TQ_SIGNAL(sendStringToEmu(const char*)),
                     this, TQ_SLOT(sendString(const char*)));
  }
}

char Vt102Emulation::eraseChar()
{
  int cmd = CMD_none; 
  const char* txt; 
  int len;
  bool metaspecified;
  
  if (keytrans->findEntry(TQt::Key_Backspace, 0, &cmd, &txt, &len,
      &metaspecified) && (cmd==CMD_send) && (len == 1))
    return txt[0];
    
  return '\b';
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/*                               Diagnostic                                  */
/*                                                                           */
/* ------------------------------------------------------------------------- */

/*! shows the contents of the scan buffer.

    This functions is used for diagnostics. It is called by \e reportDecodingError
    to inform about strings that cannot be decoded or handled by the emulation.

    \sa reportDecodingError
*/

/*!
*/

static void hexdump(int* s, int len)
{ int i;
  for (i = 0; i < len; i++)
  {
    if (s[i] == '\\')
      printf("\\\\");
    else
    if ((s[i]) > 32 && s[i] < 127)
      printf("%c",s[i]);
    else
      printf("\\%04x(hex)",s[i]);
  }
}

void Vt102Emulation::scan_buffer_report()
{
  if (m_tokenBufferPos == 0 || (m_tokenBufferPos == 1 && (m_tokenBuffer[0] & 0xff) >= 32)) return;
  printf("token: "); hexdump(m_tokenBuffer,m_tokenBufferPos); printf("\n");
}

/*!
*/

void Vt102Emulation::reportDecodingError()
{
#ifndef NDEBUG
  printf("[konsole Vt102Emulation] Undecodable/unrecognized "); scan_buffer_report();
#endif
}

}

/*
 Originally comes from NumLockX http://dforce.sh.cvut.cz/~seli/en/numlockx

 NumLockX
 
 Copyright (C) 2000-2001 Lubos Lunak        <l.lunak@kde.org>
 Copyright (C) 2001      Oswald Buddenhagen <ossi@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/

#if defined(HAVE_XKB)

#include <X11/Xlib.h>

#define explicit myexplicit
#include <X11/XKBlib.h>
#undef explicit

#include <X11/keysym.h>

/* the XKB stuff is based on code created by Oswald Buddenhagen <ossi@kde.org> */
static int xkb_init()
{
    int xkb_opcode, xkb_event, xkb_error;
    int xkb_lmaj = XkbMajorVersion;
    int xkb_lmin = XkbMinorVersion;
    return XkbLibraryVersion( &xkb_lmaj, &xkb_lmin )
        && XkbQueryExtension( tqt_xdisplay(), &xkb_opcode, &xkb_event, &xkb_error,
			       &xkb_lmaj, &xkb_lmin );
}
    
#if 0
// This method doesn't work in all cases. The atom "ScrollLock" doesn't seem
// to exist on all XFree versions (at least it's not here with my 3.3.6) - DF
static unsigned int xkb_mask_modifier( XkbDescPtr xkb, const char *name )
{
    int i;
    if( !xkb || !xkb->names )
	return 0;

    Atom atom = XInternAtom( xkb->dpy, name, true );
    if (atom == None)
        return 0;

    for( i = 0;
         i < XkbNumVirtualMods;
	 i++ )
    {
	if (atom == xkb->names->vmods[i] )
	{
	    unsigned int mask;
	    XkbVirtualModsToReal( xkb, 1 << i, &mask );
	    return mask;
	}
    }
    return 0;
}

static unsigned int xkb_scrolllock_mask()
{
    XkbDescPtr xkb;
    if(( xkb = XkbGetKeyboard( tqt_xdisplay(), XkbAllComponentsMask, XkbUseCoreKbd )) != NULL )
    {
        unsigned int mask = xkb_mask_modifier( xkb, "ScrollLock" );
        XkbFreeKeyboard( xkb, 0, True );
        return mask;
    }
    return 0;
}

#else
static unsigned int xkb_scrolllock_mask()
{
    int scrolllock_mask = 0;
    XModifierKeymap* map = XGetModifierMapping( tqt_xdisplay() );
    KeyCode scrolllock_keycode = XKeysymToKeycode( tqt_xdisplay(), XK_Scroll_Lock );
    if( scrolllock_keycode == NoSymbol ) {
        XFreeModifiermap(map);
        return 0;
    }
    for( int i = 0;
         i < 8;
         ++i )
        {
       if( map->modifiermap[ map->max_keypermod * i ] == scrolllock_keycode )
               scrolllock_mask += 1 << i;
       }

    XFreeModifiermap(map);
    return scrolllock_mask;
}
#endif


static unsigned int scrolllock_mask = 0;
        
static int xkb_set_on()
{
    if (!scrolllock_mask)
    {
       if( !xkb_init())
          return 0;
       scrolllock_mask = xkb_scrolllock_mask();
       if( scrolllock_mask == 0 )
          return 0;
    }
    XkbLockModifiers ( tqt_xdisplay(), XkbUseCoreKbd, scrolllock_mask, scrolllock_mask);
    return 1;
}
    
static int xkb_set_off()
{
    if (!scrolllock_mask)
    {
       if( !xkb_init())
          return 0;
       scrolllock_mask = xkb_scrolllock_mask();
       if( scrolllock_mask == 0 )
          return 0;
    }
    XkbLockModifiers ( tqt_xdisplay(), XkbUseCoreKbd, scrolllock_mask, 0);
    return 1;
}

static void scrolllock_set_on()
{
    xkb_set_on();
}

static void scrolllock_set_off()
{
    xkb_set_off();
}
#endif // defined(HAVE_XKB)

#include "Vt102Emulation.moc"
