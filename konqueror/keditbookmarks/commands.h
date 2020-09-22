/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __commands_h
#define __commands_h

#include <kcommand.h>
#include <kbookmark.h>
#include <tqvaluevector.h>

// Interface adds the affectedBookmarks method
// Any class should on call add those bookmarks which are 
// affected by executing or unexecuting the command
// Or a common parent of the affected bookmarks
// see KBookmarkManager::notifyChange(KBookmarkGroup)
class IKEBCommand
{
public:
   IKEBCommand() {};
   virtual ~IKEBCommand() {};
   virtual TQString affectedBookmarks() const = 0;
   virtual TQString currentAddress() const { return TQString::null; }
};

class KEBMacroCommand : public KMacroCommand, public IKEBCommand
{
public:
   KEBMacroCommand(const TQString &name)
      : KMacroCommand(name) {};
   virtual ~KEBMacroCommand() {};
   virtual TQString affectedBookmarks() const;
};

class DeleteManyCommand : public KEBMacroCommand
{
public:
   DeleteManyCommand(const TQString &name, const TQValueList<TQString>  & addresses);
   virtual ~DeleteManyCommand() {};
   virtual TQString currentAddress() const;
private:
   TQString prevOrParentAddress(TQString addr);
   TQString preOrderNextAddress(TQString addr);
   bool isConsecutive(const TQValueList<TQString> & addresses);
   TQString m_currentAddress;
};

class CreateCommand : public KCommand, public IKEBCommand
{
public:
   // separator
   CreateCommand(const TQString &address)
      : KCommand(), m_to(address),
        m_group(false), m_separator(true), m_originalBookmark(TQDomElement())
   { ; }

   // bookmark
   CreateCommand(const TQString &address,
                 const TQString &text, const TQString &iconPath, 
                 const KURL &url)
      : KCommand(), m_to(address), m_text(text), m_iconPath(iconPath), m_url(url),
        m_group(false), m_separator(false), m_originalBookmark(TQDomElement())
   { ; }

   // folder
   CreateCommand(const TQString &address,
                 const TQString &text, const TQString &iconPath, 
                 bool open)
      : KCommand(), m_to(address), m_text(text), m_iconPath(iconPath),
        m_group(true), m_separator(false), m_open(open), m_originalBookmark(TQDomElement())
   { ; }

   // clone existing bookmark
   CreateCommand(const TQString &address,
                 const KBookmark &original, const TQString &name = TQString::null)
      : KCommand(), m_to(address), m_group(false), m_separator(false),
        m_open(false), m_originalBookmark(original), m_mytext(name)
   { ; }

   TQString finalAddress() const;

   virtual ~CreateCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual TQString name() const;
   virtual TQString affectedBookmarks() const;
   virtual TQString currentAddress() const;
private:
   TQString m_to;
   TQString m_text;
   TQString m_iconPath;
   KURL m_url;
   bool m_group:1;
   bool m_separator:1;
   bool m_open:1;
   KBookmark m_originalBookmark;
   TQString m_mytext;
};

class EditCommand : public KCommand, public IKEBCommand
{
public:

   struct Edition {
      Edition() { ; } // needed for QValueList
      Edition(const TQString &a, const TQString &v) : attr(a), value(v) {}
      TQString attr;
      TQString value;
   };

   // change one attribute
   EditCommand(const TQString &address, Edition edition, const TQString &name = TQString::null) 
      : KCommand(), m_address(address), m_mytext(name)
   {
      m_editions.append(edition);
   }

   // change multiple attributes
   EditCommand(const TQString &address,
               const TQValueList<Edition> &editions, 
               const TQString &name = TQString::null)
      : KCommand(), m_address(address), m_editions(editions), m_mytext(name)
   { ; }

   void modify(const TQString & a, const TQString & v);

   virtual ~EditCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual TQString name() const;
   virtual TQString affectedBookmarks() const;
private:
   TQString m_address;
   TQValueList<Edition> m_editions;
   TQValueList<Edition> m_reverseEditions;
   TQString m_mytext;
};

class NodeEditCommand : public KCommand, public IKEBCommand
{
public:
   NodeEditCommand(const TQString &address, 
                   const TQString &newText, 
                   const TQString &nodeName)
      : KCommand(), m_address(address), m_newText(newText), m_nodename(nodeName)
   { ; }

   void modify(const TQString & newText);

   virtual ~NodeEditCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual TQString affectedBookmarks() const;
   virtual TQString name() const;
   static TQString getNodeText(KBookmark bk, const TQStringList &nodehier);
   static TQString setNodeText(KBookmark bk, const TQStringList &nodehier, 
                              TQString newValue);
private:
   TQString m_address;
   TQString m_newText;
   TQString m_oldText;
   TQString m_nodename;
};

class DeleteCommand : public KCommand, public IKEBCommand
{
public:
   DeleteCommand(const TQString &from, bool contentOnly = false)
      : KCommand(), m_from(from), m_cmd(0L), m_subCmd(0L), m_contentOnly(contentOnly)
   { ; }
   virtual ~DeleteCommand() { delete m_cmd; delete m_subCmd;}
   virtual void execute();
   virtual void unexecute();
   virtual TQString name() const { 
      // NOTE - DeleteCommand needs no name, its always embedded in a macrocommand
      return ""; 
   };
   virtual TQString affectedBookmarks() const;
   static KEBMacroCommand* deleteAll(const KBookmarkGroup &parentGroup);
private:
   TQString m_from;
   KCommand *m_cmd;
   KEBMacroCommand *m_subCmd;
   bool m_contentOnly;
};

class MoveCommand : public KCommand, public IKEBCommand
{
public:
   // "Create it with itemsAlreadyMoved=true since 
   // "TDEListView moves the item before telling us about it."
   MoveCommand(const TQString &from, const TQString &to, const TQString &name = TQString::null)
      : KCommand(), m_from(from), m_to(to), m_mytext(name)
   { ; }
   TQString finalAddress() const;
   virtual ~MoveCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual TQString name() const;
   virtual TQString affectedBookmarks() const;
private:
   TQString m_from;
   TQString m_to;
   TQString m_mytext;
};

class SortItem;

class SortCommand : public KEBMacroCommand
{
public:
   SortCommand(const TQString &name, const TQString &groupAddress)
      : KEBMacroCommand(name), m_groupAddress(groupAddress) 
   { ; }
   virtual ~SortCommand() 
   { ; }
   virtual void execute();
   virtual void unexecute();
   virtual TQString affectedBookmarks() const;
   // internal
   void moveAfter(const SortItem &moveMe, const SortItem &afterMe);
private:
   TQString m_groupAddress;
};

class KEBListViewItem;

class CmdGen {
public:
   static KEBMacroCommand* setAsToolbar(const KBookmark &bk);
   static KEBMacroCommand* setShownInToolbar(const TQValueList<KBookmark> &bk, bool show);
   static bool shownInToolbar(const KBookmark &bk);
   static KEBMacroCommand* deleteItems(const TQString &commandName, const TQValueVector<KEBListViewItem *> & items);
   static KEBMacroCommand* insertMimeSource(const TQString &cmdName, TQMimeSource *data, const TQString &addr);
   static KEBMacroCommand* itemsMoved(const TQValueVector<KEBListViewItem *> & items, const TQString &newAddress, bool copy);
private:
   CmdGen() { ; }
};

#endif
