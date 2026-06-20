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

/*
   The keyboard translation table allows to configure konsoles behavior
   on key strokes.

   FIXME: some bug crept in, disallowing '\0' to be emitted.
*/

#include "keytrans.h"
#include <tqbuffer.h>
#include <tqobject.h>
#include <tqintdict.h>
#include <tqfile.h>
#include <tdestandarddirs.h>
#include <tdelocale.h>

#include <stdio.h>
#include <stddef.h>

/* KeyEntry

   instances represent the individual assignments
*/

KeyTrans::KeyEntry::KeyEntry(int _ref, int _key, int _bits, int _mask, int _cmd, TQString _txt)
: ref(_ref), key(_key), bits(_bits), mask(_mask), cmd(_cmd), txt(_txt)
{
}

KeyTrans::KeyEntry::~KeyEntry()
{
}

bool KeyTrans::KeyEntry::matches(int _key, int _bits, int _mask)
{ int m = mask & _mask;
  return _key == key && (bits & m) == (_bits & m);
}

bool KeyTrans::KeyEntry::metaspecified(void)
{
  return ((mask & (1 << BITS_Alt))    && (bits & (1 << BITS_Alt))) ||
         ((mask & (1 << BITS_AnyMod)) && (bits & (1 << BITS_AnyMod)));
}

bool KeyTrans::KeyEntry::anymodspecified(void)
{
  return (mask & (1 << BITS_AnyMod)) && (bits & (1 << BITS_AnyMod));
}

TQString KeyTrans::KeyEntry::text()
{
  return txt;
}

/* KeyTrans

   combines the individual assignments to a proper map
   Takes part in a collection themself.
*/

KeyTrans::KeyTrans(const TQString& path)
:m_path(path)
,m_numb(0)
,m_fileRead(false)
{
  tableX.setAutoDelete(true);
  if (m_path=="[buildin]")
  {
     m_id = "default";
  }
  else
  {
     m_id = m_path;
     int i = m_id.findRev('/');
     if (i > -1)
        m_id = m_id.mid(i+1);
     i = m_id.findRev('.');
     if (i > -1)
        m_id = m_id.left(i);
  }
}

KeyTrans::KeyTrans()
{
/*  table.setAutoDelete(true);
  path = "";
  numb = 0;*/
}

KeyTrans::~KeyTrans()
{
}

KeyTrans::KeyEntry* KeyTrans::addEntry(int ref, int key, int bits, int mask, int cmd, TQString txt)
// returns conflicting entry
{
  for (TQPtrListIterator<KeyEntry> it(tableX); it.current(); ++it)
  {
    if (it.current()->matches(key,bits,mask))
    {
      return it.current();
    }
  }
  tableX.append(new KeyEntry(ref,key,bits,mask,cmd,txt));
  return (KeyEntry*)NULL;
}

bool KeyTrans::findEntry(int key, int bits, int* cmd, const char** txt, int* len,
               bool* metaspecified)
{
  if (!m_fileRead) readConfig();
  
  if (bits & ((1<<BITS_Shift)|(1<<BITS_Alt)|(1<<BITS_Control)))
    bits |= (1<<BITS_AnyMod);
  
  for (TQPtrListIterator<KeyEntry> it(tableX); it.current(); ++it)
    if (it.current()->matches(key,bits,0xffff))
    {
      *cmd = it.current()->cmd;
      *len = it.current()->txt.length();
      if ((*cmd==CMD_send) && it.current()->anymodspecified() && (*len < 16))
      {
        static char buf[16];
        char *c;
	char mask = '1' + BITS(0, bits&(1<<BITS_Shift)) + BITS(1, bits&(1<<BITS_Alt)) + BITS(2, bits&(1<<BITS_Control));
        strcpy(buf, it.current()->txt.ascii());
        c = strchr(buf, '*');
        if (c) *c = mask;
        *txt = buf;
      }
      else
        *txt = it.current()->txt.ascii();
      *metaspecified = it.current()->metaspecified();
      return true;
    }
  return false;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Scanner for keyboard configuration                                        */
/*                                                                           */
/* ------------------------------------------------------------------------- */

// regular tokenizer
/* Tokens
   - Spaces
   - Name    (A-Za-z0-9)+
   - String
   - Opr     on of +-:
*/

#define SYMName    0
#define SYMString  1
#define SYMEol     2
#define SYMEof     3
#define SYMOpr     4
#define SYMError   5

#define inRange(L,X,H) ((L <= X) && (X <= H))
#define isNibble(X) (inRange('A',X,'F')||inRange('a',X,'f')||inRange('0',X,'9'))
#define convNibble(X) (inRange('0',X,'9')?X-'0':X+10-(inRange('A',X,'F')?'A':'a'))

class KeytabReader
{
public:
  KeytabReader(TQString p, TQIODevice &d);
public:
  void getCc();
  void getSymbol();
  void parseTo(KeyTrans* kt);
  void ReportError(const char* msg);
  void ReportToken(); // diagnostic
private:
  int     sym;
  TQString res;
  int     len;
  int     slinno;
  int     scolno;
private:
  int     cc;
  int     linno;
  int     colno;
  TQIODevice* buf;
  TQString path;
};


KeytabReader::KeytabReader(TQString p, TQIODevice &d)
{
  path = p;
  buf = &d;
  cc = 0;
  colno = 0;
}

void KeytabReader::getCc()
{
  if (cc == '\n') { linno += 1; colno = 0; }
  if (cc < 0) return;
  cc = buf->getch();
  colno += 1;
}

void KeytabReader::getSymbol()
{
  res = ""; len = 0; sym = SYMError;
  while (cc == ' ') getCc(); // skip spaces
  if (cc == '#')      // skip comment
  {
    while (cc != '\n' && cc > 0) getCc();
  }
  slinno = linno;
  scolno = colno;
  if (cc <= 0)
  {
    sym = SYMEof; // eos
  }
  else if (cc == '\n')
  {
    getCc();
    sym = SYMEol; // eol
  }
  else if (inRange('A',cc,'Z')||inRange('a',cc,'z')||inRange('0',cc,'9')||(cc=='_'))
  {
    while (inRange('A',cc,'Z') || inRange('a',cc,'z') || inRange('0',cc,'9') || (cc=='_'))
    {
      res = res + (char)cc;
      getCc();
    }
    sym = SYMName;
  }
  else if (strchr("+-:",cc))
  {
    res = "";
    res = res + (char)cc;
    getCc();
    sym = SYMOpr;
  }
  else if (cc == '"')
  {
    getCc();
    while (cc >= ' ' && cc != '"')
    { int sc;
      if (cc == '\\') // handle quotation
      {
        getCc();
        switch (cc)
        {
          case 'E'  : sc = 27; getCc(); break;
          case 'b'  : sc =  8; getCc(); break;
          case 'f'  : sc = 12; getCc(); break;
          case 't'  : sc =  9; getCc(); break;
          case 'r'  : sc = 13; getCc(); break;
          case 'n'  : sc = 10; getCc(); break;
          case '\\' : // fall thru
          case '"'  : sc = cc; getCc(); break;
          case 'x'  : getCc();
                      sc = 0;
                      if (!isNibble(cc)) return; sc = 16*sc + convNibble(cc); getCc();
                      if (!isNibble(cc)) return; sc = 16*sc + convNibble(cc); getCc();
                      break;
          default   : return;
        }
      }
      else
      {
        // regular char
        sc = cc; getCc();
      }
      res = res + (char)sc;
      len = len + 1;
    }
    if (cc != '"') return;
    getCc();
    sym = SYMString;
  }
  else
  {
    // cc was an invalid char. Get next cc or we will loop forever.
    getCc();
  }
}

void KeytabReader::ReportToken() // diagnostic
{
  printf("sym(%d): ",slinno);
  switch(sym)
  {
    case SYMEol    : printf("End of line"); break;
    case SYMEof    : printf("End of file"); break;
    case SYMName   : printf("Name: %s",res.latin1()); break;
    case SYMOpr    : printf("Opr : %s",res.latin1()); break;
    case SYMString : printf("String len %d,%d ",res.length(),len);
                     for (unsigned i = 0; i < res.length(); i++)
                       printf(" %02x(%c)",res.latin1()[i],res.latin1()[i]>=' '?res.latin1()[i]:'?');
                     break;
  }
  printf("\n");
}

void KeytabReader::ReportError(const char* msg) // diagnostic
{
  fprintf(stderr,"%s(%d,%d):error: %s.\n",path.ascii(),slinno,scolno,msg);
}

// local symbol tables ---------------------------------------------------------------------

class KeyTransSymbols
{
public:
  KeyTransSymbols();
protected:
  void defOprSyms();
  void defModSyms();
  void defKeySyms();
  void defKeySym(const char* key, int val);
  void defOprSym(const char* key, int val);
  void defModSym(const char* key, int val);
public:
  TQDict<TQObject> keysyms;
  TQDict<TQObject> modsyms;
  TQDict<TQObject> oprsyms;
};

static KeyTransSymbols * syms = 0L;

// parser ----------------------------------------------------------------------------------
/* Syntax
   - Line :: [KeyName { ("+" | "-") ModeName } ":" (String|CommandName)] "\n"
   - Comment :: '#' (any but \n)*
*/

void KeyTrans::readConfig()
{
   if (m_fileRead) return;
   m_fileRead=true;
   TQIODevice* buf(0);
   if (m_path=="[buildin]")
   {
      TQCString txt =
#include "default.keytab.h"
;
      TQBuffer* newbuf;
      newbuf = new TQBuffer();
      newbuf->setBuffer(txt);
      buf=newbuf;
   }
   else
   {
      buf=new TQFile(m_path);
   };
   KeytabReader ktr(m_path,*buf);
   ktr.parseTo(this);
   delete buf;
}

#define assertSyntax(Cond,Message) if (!(Cond)) { ReportError(Message); goto ERROR; }

void KeytabReader::parseTo(KeyTrans* kt)
{
  // Opening sequence

  buf->open(IO_ReadOnly);
  getCc();
  linno = 1;
  colno  = 1;
  getSymbol();

Loop:
  // syntax: ["key" KeyName { ("+" | "-") ModeName } ":" String/CommandName] ["#" Comment]
  if (sym == SYMName && !strcmp(res.latin1(),"keyboard"))
  {
    getSymbol(); assertSyntax(sym == SYMString, "Header expected")
    kt->m_hdr = i18n(res.latin1());
    getSymbol(); assertSyntax(sym == SYMEol, "Text unexpected")
    getSymbol();                   // eoln
    goto Loop;
  }
  if (sym == SYMName && !strcmp(res.latin1(),"key"))
  {
//printf("line %3d: ",startofsym);
    getSymbol(); assertSyntax(sym == SYMName, "Name expected")
    assertSyntax(syms->keysyms[res], "Unknown key name")
    ptrdiff_t key = (ptrdiff_t)(syms->keysyms[res]) - 1;
//printf(" key %s (%04x)",res.latin1(),(int)syms->keysyms[res]-1);
    getSymbol(); // + - :
    int mode = 0;
    int mask = 0;
    while (sym == SYMOpr && (!strcmp(res.latin1(),"+") || !strcmp(res.latin1(),"-")))
    {
      bool on = !strcmp(res.latin1(),"+");
      getSymbol();
      // mode name
      assertSyntax(sym == SYMName, "Name expected")
      assertSyntax(syms->modsyms[res], "Unknown mode name")
      ptrdiff_t bits = (ptrdiff_t)(syms->modsyms[res]) - 1;
      if (mask & (1 << bits))
      {
        fprintf(stderr,"%s(%d,%d): mode name used multiple times.\n",path.ascii(),slinno,scolno);
      }
      else
      {
        mode |= (on << bits);
        mask |= (1 << bits);
      }
//printf(", mode %s(%d) %s",res.latin1(),(int)syms->modsyms[res]-1,on?"on":"off");
      getSymbol();
    }
    assertSyntax(sym == SYMOpr && !strcmp(res.latin1(),":"), "':' expected")
    getSymbol();
    // string or command
    assertSyntax(sym == SYMName || sym == SYMString,"Command or string expected")
    ptrdiff_t cmd = 0;
    if (sym == SYMName)
    {
      assertSyntax(syms->oprsyms[res], "Unknown operator name")
      cmd = (ptrdiff_t)(syms->oprsyms[res]) - 1;
//printf(": do %s(%d)",res.latin1(),(int)syms->oprsyms[res]-1);
    }
    if (sym == SYMString)
    {
      cmd = CMD_send;
//printf(": send");
//for (unsigned i = 0; i < res.length(); i++)
//printf(" %02x(%c)",res.latin1()[i],res.latin1()[i]>=' '?res.latin1()[i]:'?');
    }
//printf(". summary %04x,%02x,%02x,%d\n",key,mode,mask,cmd);
    KeyTrans::KeyEntry* ke = kt->addEntry(slinno,key,mode,mask,cmd,res);
    if (ke)
    {
      fprintf(stderr,"%s(%d): keystroke already assigned in line %d.\n",path.ascii(),slinno,ke->ref);
    }
    getSymbol();
    assertSyntax(sym == SYMEol, "Unexpected text")
    goto Loop;
  }
  if (sym == SYMEol)
  {
    getSymbol();
    goto Loop;
  }

  assertSyntax(sym == SYMEof, "Undecodable Line")

  buf->close();
  return;

ERROR:
  while (sym != SYMEol && sym != SYMEof) getSymbol(); // eoln
  goto Loop;
}


// local symbol tables ---------------------------------------------------------------------
// material needed for parsing the config file.
// This is incomplete work.

void KeyTransSymbols::defKeySym(const char* key, int val)
{
  keysyms.insert(key,(TQObject*)(val+1));
}

void KeyTransSymbols::defOprSym(const char* key, int val)
{
  oprsyms.insert(key,(TQObject*)(val+1));
}

void KeyTransSymbols::defModSym(const char* key, int val)
{
  modsyms.insert(key,(TQObject*)(val+1));
}

void KeyTransSymbols::defOprSyms()
{
  // Modifier
  defOprSym("scrollLineUp",  CMD_scrollLineUp  );
  defOprSym("scrollLineDown",CMD_scrollLineDown);
  defOprSym("scrollPageUp",  CMD_scrollPageUp  );
  defOprSym("scrollPageDown",CMD_scrollPageDown);
  defOprSym("scrollLock",    CMD_scrollLock);
}

void KeyTransSymbols::defModSyms()
{
  // Modifier
  defModSym("Shift",      BITS_Shift        );
  defModSym("Control",    BITS_Control      );
  defModSym("Alt",        BITS_Alt          );
  // Modes
  defModSym("BsHack",     BITS_BsHack       ); // deprecated
  defModSym("Ansi",       BITS_Ansi         );
  defModSym("NewLine",    BITS_NewLine      );
  defModSym("AppCuKeys",  BITS_AppCuKeys    );
  defModSym("AppScreen",  BITS_AppScreen    );
  // Special (Any Modifier)
  defModSym("AnyMod",     BITS_AnyMod       );
}

void KeyTransSymbols::defKeySyms()
{
  // Grey keys
  defKeySym("Escape",       TQt::Key_Escape      );
  defKeySym("Tab",          TQt::Key_Tab         );
  defKeySym("Backtab",      TQt::Key_Backtab     );
  defKeySym("Backspace",    TQt::Key_Backspace   );
  defKeySym("Return",       TQt::Key_Return      );
  defKeySym("Enter",        TQt::Key_Enter       );
  defKeySym("Insert",       TQt::Key_Insert      );
  defKeySym("Delete",       TQt::Key_Delete      );
  defKeySym("Pause",        TQt::Key_Pause       );
  defKeySym("Print",        TQt::Key_Print       );
  defKeySym("SysReq",       TQt::Key_SysReq      );
  defKeySym("Home",         TQt::Key_Home        );
  defKeySym("End",          TQt::Key_End         );
  defKeySym("Left",         TQt::Key_Left        );
  defKeySym("Up",           TQt::Key_Up          );
  defKeySym("Right",        TQt::Key_Right       );
  defKeySym("Down",         TQt::Key_Down        );
  defKeySym("Prior",        TQt::Key_Prior      );
  defKeySym("Next",         TQt::Key_Next       );
  defKeySym("Shift",        TQt::Key_Shift       );
  defKeySym("Control",      TQt::Key_Control     );
  defKeySym("Meta",         TQt::Key_Meta        );
  defKeySym("Alt",          TQt::Key_Alt         );
  defKeySym("CapsLock",     TQt::Key_CapsLock    );
  defKeySym("NumLock",      TQt::Key_NumLock     );
  defKeySym("ScrollLock",   TQt::Key_ScrollLock  );
  defKeySym("F1",           TQt::Key_F1          );
  defKeySym("F2",           TQt::Key_F2          );
  defKeySym("F3",           TQt::Key_F3          );
  defKeySym("F4",           TQt::Key_F4          );
  defKeySym("F5",           TQt::Key_F5          );
  defKeySym("F6",           TQt::Key_F6          );
  defKeySym("F7",           TQt::Key_F7          );
  defKeySym("F8",           TQt::Key_F8          );
  defKeySym("F9",           TQt::Key_F9          );
  defKeySym("F10",          TQt::Key_F10         );
  defKeySym("F11",          TQt::Key_F11         );
  defKeySym("F12",          TQt::Key_F12         );
  defKeySym("F13",          TQt::Key_F13         );
  defKeySym("F14",          TQt::Key_F14         );
  defKeySym("F15",          TQt::Key_F15         );
  defKeySym("F16",          TQt::Key_F16         );
  defKeySym("F17",          TQt::Key_F17         );
  defKeySym("F18",          TQt::Key_F18         );
  defKeySym("F19",          TQt::Key_F19         );
  defKeySym("F20",          TQt::Key_F20         );
  defKeySym("F21",          TQt::Key_F21         );
  defKeySym("F22",          TQt::Key_F22         );
  defKeySym("F23",          TQt::Key_F23         );
  defKeySym("F24",          TQt::Key_F24         );
  defKeySym("F25",          TQt::Key_F25         );
  defKeySym("F26",          TQt::Key_F26         );
  defKeySym("F27",          TQt::Key_F27         );
  defKeySym("F28",          TQt::Key_F28         );
  defKeySym("F29",          TQt::Key_F29         );
  defKeySym("F30",          TQt::Key_F30         );
  defKeySym("F31",          TQt::Key_F31         );
  defKeySym("F32",          TQt::Key_F32         );
  defKeySym("F33",          TQt::Key_F33         );
  defKeySym("F34",          TQt::Key_F34         );
  defKeySym("F35",          TQt::Key_F35         );
  defKeySym("Super_L",      TQt::Key_Super_L     );
  defKeySym("Super_R",      TQt::Key_Super_R     );
  defKeySym("Menu",         TQt::Key_Menu        );
  defKeySym("Hyper_L",      TQt::Key_Hyper_L     );
  defKeySym("Hyper_R",      TQt::Key_Hyper_R     );

  // Regular keys
  defKeySym("Space",        TQt::Key_Space       );
  defKeySym("Exclam",       TQt::Key_Exclam      );
  defKeySym("QuoteDbl",     TQt::Key_QuoteDbl    );
  defKeySym("NumberSign",   TQt::Key_NumberSign  );
  defKeySym("Dollar",       TQt::Key_Dollar      );
  defKeySym("Percent",      TQt::Key_Percent     );
  defKeySym("Ampersand",    TQt::Key_Ampersand   );
  defKeySym("Apostrophe",   TQt::Key_Apostrophe  );
  defKeySym("ParenLeft",    TQt::Key_ParenLeft   );
  defKeySym("ParenRight",   TQt::Key_ParenRight  );
  defKeySym("Asterisk",     TQt::Key_Asterisk    );
  defKeySym("Plus",         TQt::Key_Plus        );
  defKeySym("Comma",        TQt::Key_Comma       );
  defKeySym("Minus",        TQt::Key_Minus       );
  defKeySym("Period",       TQt::Key_Period      );
  defKeySym("Slash",        TQt::Key_Slash       );
  defKeySym("0",            TQt::Key_0           );
  defKeySym("1",            TQt::Key_1           );
  defKeySym("2",            TQt::Key_2           );
  defKeySym("3",            TQt::Key_3           );
  defKeySym("4",            TQt::Key_4           );
  defKeySym("5",            TQt::Key_5           );
  defKeySym("6",            TQt::Key_6           );
  defKeySym("7",            TQt::Key_7           );
  defKeySym("8",            TQt::Key_8           );
  defKeySym("9",            TQt::Key_9           );
  defKeySym("Colon",        TQt::Key_Colon       );
  defKeySym("Semicolon",    TQt::Key_Semicolon   );
  defKeySym("Less",         TQt::Key_Less        );
  defKeySym("Equal",        TQt::Key_Equal       );
  defKeySym("Greater",      TQt::Key_Greater     );
  defKeySym("Question",     TQt::Key_Question    );
  defKeySym("At",           TQt::Key_At          );
  defKeySym("A",            TQt::Key_A           );
  defKeySym("B",            TQt::Key_B           );
  defKeySym("C",            TQt::Key_C           );
  defKeySym("D",            TQt::Key_D           );
  defKeySym("E",            TQt::Key_E           );
  defKeySym("F",            TQt::Key_F           );
  defKeySym("G",            TQt::Key_G           );
  defKeySym("H",            TQt::Key_H           );
  defKeySym("I",            TQt::Key_I           );
  defKeySym("J",            TQt::Key_J           );
  defKeySym("K",            TQt::Key_K           );
  defKeySym("L",            TQt::Key_L           );
  defKeySym("M",            TQt::Key_M           );
  defKeySym("N",            TQt::Key_N           );
  defKeySym("O",            TQt::Key_O           );
  defKeySym("P",            TQt::Key_P           );
  defKeySym("Q",            TQt::Key_Q           );
  defKeySym("R",            TQt::Key_R           );
  defKeySym("S",            TQt::Key_S           );
  defKeySym("T",            TQt::Key_T           );
  defKeySym("U",            TQt::Key_U           );
  defKeySym("V",            TQt::Key_V           );
  defKeySym("W",            TQt::Key_W           );
  defKeySym("X",            TQt::Key_X           );
  defKeySym("Y",            TQt::Key_Y           );
  defKeySym("Z",            TQt::Key_Z           );
  defKeySym("BracketLeft",  TQt::Key_BracketLeft );
  defKeySym("Backslash",    TQt::Key_Backslash   );
  defKeySym("BracketRight", TQt::Key_BracketRight);
  defKeySym("AsciiCircum",  TQt::Key_AsciiCircum );
  defKeySym("Underscore",   TQt::Key_Underscore  );
  defKeySym("QuoteLeft",    TQt::Key_QuoteLeft   );
  defKeySym("BraceLeft",    TQt::Key_BraceLeft   );
  defKeySym("Bar",          TQt::Key_Bar         );
  defKeySym("BraceRight",   TQt::Key_BraceRight  );
  defKeySym("AsciiTilde",   TQt::Key_AsciiTilde  );
}

KeyTransSymbols::KeyTransSymbols()
{
  defModSyms();
  defOprSyms();
  defKeySyms();
}

// Global material -----------------------------------------------------------

static int keytab_serial = 0; //FIXME: remove,localize

static TQIntDict<KeyTrans> * numb2keymap = 0L;

KeyTrans* KeyTrans::find(int numb)
{
  KeyTrans* res = numb2keymap->find(numb);
  return res ? res : numb2keymap->find(0);
}

KeyTrans* KeyTrans::find(const TQString &id)
{
  TQIntDictIterator<KeyTrans> it(*numb2keymap);
  while(it.current())
  {
    if (it.current()->id() == id)
       return it.current();
    ++it;
  }
  return numb2keymap->find(0);
}

int KeyTrans::count()
{
  return numb2keymap->count();
}

void KeyTrans::addKeyTrans()
{
  m_numb = keytab_serial ++;
  numb2keymap->insert(m_numb,this);
}

void KeyTrans::loadAll()
{
  if (!numb2keymap)
    numb2keymap = new TQIntDict<KeyTrans>;
  else  {  // Needed for konsole_part.
    numb2keymap->clear();
    keytab_serial = 0;
  }

  if (!syms)
    syms = new KeyTransSymbols;

  //defaultKeyTrans()->addKeyTrans();
  KeyTrans* sc = new KeyTrans("[buildin]");
  sc->addKeyTrans();

  TQStringList lst = TDEGlobal::dirs()->findAllResources("data", "konsole/*.keytab");

  for(TQStringList::Iterator it = lst.begin(); it != lst.end(); ++it )
  {
    //TQFile file(TQFile::encodeName(*it));
    sc = new KeyTrans(TQFile::encodeName(*it));
    //KeyTrans* sc = KeyTrans::fromDevice(TQFile::encodeName(*it),file);
    if (sc) sc->addKeyTrans();
  }
}

// Debugging material -----------------------------------------------------------
/*
void TestTokenizer(TQBuffer &buf)
{
  // opening sequence

  buf.open(IO_ReadOnly);
  cc = buf.getch();
  lineno = 1;

  // Test tokenizer

  while (getSymbol(buf)) ReportToken();

  buf.close();
}

void test()
{
  // Opening sequence

  TQCString txt =
#include "default.keytab.h"
  ;
  TQBuffer buf(txt);
  if (0) TestTokenizer(buf);
  if (1) { KeyTrans kt; kt.scanTable(buf); }
}
*/
