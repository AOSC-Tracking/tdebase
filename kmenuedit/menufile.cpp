/*
 *   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as 
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <tqfile.h>
#include <tqtextstream.h>
#include <tqregexp.h>

#include <kdebug.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdestandarddirs.h>

#include "menufile.h"


#define MF_MENU		"Menu"
#define MF_PUBLIC_ID	"-//freedesktop//DTD Menu 1.0//EN"
#define MF_SYSTEM_ID	"http://www.freedesktop.org/standards/menu-spec/1.0/menu.dtd"
#define MF_NAME		"Name"
#define MF_INCLUDE      "Include"
#define MF_EXCLUDE      "Exclude"
#define MF_FILENAME     "Filename"
#define MF_DELETED      "Deleted"
#define MF_NOTDELETED   "NotDeleted"
#define MF_MOVE         "Move"
#define MF_OLD          "Old"
#define MF_NEW          "New"
#define MF_DIRECTORY    "Directory"
#define MF_LAYOUT       "Layout"
#define MF_MENUNAME     "Menuname"
#define MF_SEPARATOR    "Separator"
#define MF_MERGE        "Merge"

MenuFile::MenuFile(const TQString &file)
 : m_fileName(file), m_bDirty(false)
{
   load();
}

MenuFile::~MenuFile()
{
}

bool MenuFile::load()
{
   if (m_fileName.isEmpty())
      return false;

   TQFile file( m_fileName );
   if (!file.open( IO_ReadOnly ))
   {
      kdWarning() << "Could not read " << m_fileName << endl;
      create();
      return false;
   }
   
   TQString errorMsg;
   int errorRow;
   int errorCol;
   if ( !m_doc.setContent( &file, &errorMsg, &errorRow, &errorCol ) ) {
      kdWarning() << "Parse error in " << m_fileName << ", line " << errorRow << ", col " << errorCol << ": " << errorMsg << endl;
      file.close();
      create();
      return false;
   }
   file.close();

   return true;
}

void MenuFile::create()
{
   TQDomImplementation impl;
   TQDomDocumentType docType = impl.createDocumentType( MF_MENU, MF_PUBLIC_ID, MF_SYSTEM_ID );
   m_doc = impl.createDocument(TQString::null, MF_MENU, docType);
}

bool MenuFile::save()
{
   TQFile file( m_fileName );
   
   if (!file.open( IO_WriteOnly ))
   {
      kdWarning() << "Could not write " << m_fileName << endl;
      m_error = i18n("Could not write to %1").arg(m_fileName);
      return false;
   }
   TQTextStream stream( &file );
   stream.setEncoding(TQTextStream::UnicodeUTF8);
   
   stream << m_doc.toString();

   file.close();
   
   if (file.status() != IO_Ok)
   {
      kdWarning() << "Could not close " << m_fileName << endl;
      m_error = i18n("Could not write to %1").arg(m_fileName);
      return false;
   }

   m_bDirty = false;
   
   return true;
}

TQDomElement MenuFile::findMenu(TQDomElement elem, const TQString &menuName, bool create)
{
   TQString menuNodeName;
   TQString subMenuName; 
   int i = menuName.find('/');
   if (i >= 0)
   {
      menuNodeName = menuName.left(i);
      subMenuName = menuName.mid(i+1);
   }
   else
   {
      menuNodeName = menuName;
   }
   if (i == 0)
      return findMenu(elem, subMenuName, create);

   if (menuNodeName.isEmpty())
      return elem;

   TQDomNode n = elem.firstChild();
   while( !n.isNull() )
   {
      TQDomElement e = n.toElement(); // try to convert the node to an element.
      if (e.tagName() == MF_MENU)
      {
         TQString name;

         TQDomNode n2 = e.firstChild();
         while ( !n2.isNull() )
         {
            TQDomElement e2 = n2.toElement();
            if (!e2.isNull() && e2.tagName() == MF_NAME)
            {
               name = e2.text();
               break;
            }
            n2 = n2.nextSibling();
         }
         
         if (name == menuNodeName)
         {
            if (subMenuName.isEmpty())
               return e;
            else
               return findMenu(e, subMenuName, create);
         }
      }
      n = n.nextSibling();
   }

   if (!create)
      return TQDomElement();
      
   // Create new node.
   TQDomElement newElem = m_doc.createElement(MF_MENU);
   TQDomElement newNameElem = m_doc.createElement(MF_NAME);
   newNameElem.appendChild(m_doc.createTextNode(menuNodeName));
   newElem.appendChild(newNameElem);
   elem.appendChild(newElem);

   if (subMenuName.isEmpty())
      return newElem;
   else
      return findMenu(newElem, subMenuName, create);
}
   
static TQString entryToDirId(const TQString &path)
{
   // See also TDEDesktopFile::locateLocal
   TQString local;
   if (path.startsWith("/"))
   {
      // XDG Desktop menu items come with absolute paths, we need to
      // extract their relative path and then build a local path.
      local = TDEGlobal::dirs()->relativeLocation("xdgdata-dirs", path);
   }
   
   if (local.isEmpty() || local.startsWith("/"))
   {
      // What now? Use filename only and hope for the best.
      local = path.mid(path.findRev('/')+1);
   }
   return local;                                                                                           
}

static void purgeIncludesExcludes(TQDomElement elem, const TQString &appId, TQDomElement &excludeNode, TQDomElement &includeNode)
{
   // Remove any previous includes/excludes of appId
   TQDomNode n = elem.firstChild();
   while( !n.isNull() )
   {
      TQDomElement e = n.toElement(); // try to convert the node to an element.
      bool bIncludeNode = (e.tagName() == MF_INCLUDE);
      bool bExcludeNode = (e.tagName() == MF_EXCLUDE);
      if (bIncludeNode)
         includeNode = e;
      if (bExcludeNode)
         excludeNode = e;
      if (bIncludeNode || bExcludeNode)
      {
         TQDomNode n2 = e.firstChild();
         while ( !n2.isNull() )
         {
            TQDomNode next = n2.nextSibling();
            TQDomElement e2 = n2.toElement();
            if (!e2.isNull() && e2.tagName() == MF_FILENAME)
            {
               if (e2.text() == appId)
               {
                  e.removeChild(e2);
                  break;
               }
            }
            n2 = next;
         }
      }
      n = n.nextSibling();
   }
}

static void purgeDeleted(TQDomElement elem)
{
   // Remove any previous includes/excludes of appId
   TQDomNode n = elem.firstChild();
   while( !n.isNull() )
   {
      TQDomNode next = n.nextSibling();
      TQDomElement e = n.toElement(); // try to convert the node to an element.
      if ((e.tagName() == MF_DELETED) || 
          (e.tagName() == MF_NOTDELETED))
      {
         elem.removeChild(e);
      }
      n = next;
   }
}

static void purgeLayout(TQDomElement elem)
{
   // Remove any previous includes/excludes of appId
   TQDomNode n = elem.firstChild();
   while( !n.isNull() )
   {
      TQDomNode next = n.nextSibling();
      TQDomElement e = n.toElement(); // try to convert the node to an element.
      if (e.tagName() == MF_LAYOUT)
      {
         elem.removeChild(e);
      }
      n = next;
   }
}

void MenuFile::addEntry(const TQString &menuName, const TQString &menuId)
{
   m_bDirty = true;   

   m_removedEntries.remove(menuId);

   TQDomElement elem = findMenu(m_doc.documentElement(), menuName, true);

   TQDomElement excludeNode;
   TQDomElement includeNode;

   purgeIncludesExcludes(elem, menuId, excludeNode, includeNode);

   if (includeNode.isNull())
   {
      includeNode = m_doc.createElement(MF_INCLUDE);
      elem.appendChild(includeNode);
   }
   
   TQDomElement fileNode = m_doc.createElement(MF_FILENAME);
   fileNode.appendChild(m_doc.createTextNode(menuId));
   includeNode.appendChild(fileNode);
}

void MenuFile::setLayout(const TQString &menuName, const TQStringList &layout)
{
   m_bDirty = true;   

   TQDomElement elem = findMenu(m_doc.documentElement(), menuName, true);

   purgeLayout(elem);

   TQDomElement layoutNode = m_doc.createElement(MF_LAYOUT);
   elem.appendChild(layoutNode);

   for(TQStringList::ConstIterator it = layout.begin();
       it != layout.end(); ++it)
   {
      TQString li = *it;
      if (li == ":S")
      {
         layoutNode.appendChild(m_doc.createElement(MF_SEPARATOR));
      }
      else if (li == ":M")
      {
         TQDomElement mergeNode = m_doc.createElement(MF_MERGE);
         mergeNode.setAttribute("type", "menus");
         layoutNode.appendChild(mergeNode);
      }
      else if (li == ":F")
      {
         TQDomElement mergeNode = m_doc.createElement(MF_MERGE);
         mergeNode.setAttribute("type", "files");
         layoutNode.appendChild(mergeNode);
      }
      else if (li == ":A")
      {
         TQDomElement mergeNode = m_doc.createElement(MF_MERGE);
         mergeNode.setAttribute("type", "all");
         layoutNode.appendChild(mergeNode);
      }
      else if (li.endsWith("/"))
      {
         li.truncate(li.length()-1);
         TQDomElement menuNode = m_doc.createElement(MF_MENUNAME);
         menuNode.appendChild(m_doc.createTextNode(li));
         layoutNode.appendChild(menuNode);
      }
      else
      {
         TQDomElement fileNode = m_doc.createElement(MF_FILENAME);
         fileNode.appendChild(m_doc.createTextNode(li));
         layoutNode.appendChild(fileNode);
      }
   }
}


void MenuFile::removeEntry(const TQString &menuName, const TQString &menuId)
{
   m_bDirty = true;
   
   m_removedEntries.append(menuId);
   
   TQDomElement elem = findMenu(m_doc.documentElement(), menuName, true);

   TQDomElement excludeNode;
   TQDomElement includeNode;

   purgeIncludesExcludes(elem, menuId, excludeNode, includeNode);

   if (excludeNode.isNull())
   {
      excludeNode = m_doc.createElement(MF_EXCLUDE);
      elem.appendChild(excludeNode);
   }
   
   TQDomElement fileNode = m_doc.createElement(MF_FILENAME);
   fileNode.appendChild(m_doc.createTextNode(menuId));
   excludeNode.appendChild(fileNode);
}

void MenuFile::addMenu(const TQString &menuName, const TQString &menuFile)
{
   m_bDirty = true;
   TQDomElement elem = findMenu(m_doc.documentElement(), menuName, true);

   TQDomElement dirElem = m_doc.createElement(MF_DIRECTORY);
   dirElem.appendChild(m_doc.createTextNode(entryToDirId(menuFile)));
   elem.appendChild(dirElem);
}

void MenuFile::moveMenu(const TQString &oldMenu, const TQString &newMenu)
{
   m_bDirty = true;

   // Undelete the new menu
   TQDomElement elem = findMenu(m_doc.documentElement(), newMenu, true);
   purgeDeleted(elem);
   elem.appendChild(m_doc.createElement(MF_NOTDELETED));

// TODO: GET RID OF COMMON PART, IT BREAKS STUFF
   // Find common part
   TQStringList oldMenuParts = TQStringList::split('/', oldMenu);
   TQStringList newMenuParts = TQStringList::split('/', newMenu);
   TQString commonMenuName;
   uint max = TQMIN(oldMenuParts.count(), newMenuParts.count());
   uint i = 0;
   for(; i < max; i++)
   {
      if (oldMenuParts[i] != newMenuParts[i])
         break;
      commonMenuName += '/' + oldMenuParts[i];
   }
   TQString oldMenuName;
   for(uint j = i; j < oldMenuParts.count(); j++)
   {
      if (i != j)
         oldMenuName += '/';
      oldMenuName += oldMenuParts[j];
   }
   TQString newMenuName;
   for(uint j = i; j < newMenuParts.count(); j++)
   {
      if (i != j)
         newMenuName += '/';
      newMenuName += newMenuParts[j];
   }

   if (oldMenuName == newMenuName) return; // Can happen

   elem = findMenu(m_doc.documentElement(), commonMenuName, true);

   // Add instructions for moving
   TQDomElement moveNode = m_doc.createElement(MF_MOVE);
   TQDomElement node = m_doc.createElement(MF_OLD);
   node.appendChild(m_doc.createTextNode(oldMenuName));
   moveNode.appendChild(node);
   node = m_doc.createElement(MF_NEW);
   node.appendChild(m_doc.createTextNode(newMenuName));
   moveNode.appendChild(node);
   elem.appendChild(moveNode);
}

void MenuFile::removeMenu(const TQString &menuName)
{
   m_bDirty = true;

   TQDomElement elem = findMenu(m_doc.documentElement(), menuName, true);

   purgeDeleted(elem);
   elem.appendChild(m_doc.createElement(MF_DELETED));
}

   /**
    * Returns a unique menu-name for a new menu under @p menuName 
    * inspired by @p newMenu
    */
TQString MenuFile::uniqueMenuName(const TQString &menuName, const TQString &newMenu, const TQStringList & excludeList)
{
   TQDomElement elem = findMenu(m_doc.documentElement(), menuName, false);
      
   TQString result = newMenu;
   if (result.endsWith("/"))
       result.truncate(result.length()-1);
       
   TQRegExp r("(.*)(?=-\\d+)");
   result = (r.search(result) > -1) ? r.cap(1) : result;
     
   int trunc = result.length(); // Position of trailing '/'
   
   result.append("/");
   
   for(int n = 1; ++n; )
   {
      if (findMenu(elem, result, false).isNull() && !excludeList.contains(result))
         return result;
         
      result.truncate(trunc);
      result.append(TQString("-%1/").arg(n));
   }
   return TQString::null; // Never reached
}

void MenuFile::performAction(const ActionAtom *atom)
{
   switch(atom->action)
   {
     case ADD_ENTRY: 
        addEntry(atom->arg1, atom->arg2);
        return;
     case REMOVE_ENTRY:
        removeEntry(atom->arg1, atom->arg2);
        return;
     case ADD_MENU:
        addMenu(atom->arg1, atom->arg2);
        return;
     case REMOVE_MENU:
        removeMenu(atom->arg1);
        return;
     case MOVE_MENU:
        moveMenu(atom->arg1, atom->arg2);
        return;
   }
}

MenuFile::ActionAtom *MenuFile::pushAction(MenuFile::ActionType action, const TQString &arg1, const TQString &arg2)
{
   ActionAtom *atom = new ActionAtom;
   atom->action = action;
   atom->arg1 = arg1;
   atom->arg2 = arg2;
   m_actionList.append(atom);
   return atom;
}

void MenuFile::popAction(ActionAtom *atom)
{
   if (m_actionList.getLast() != atom)
   {
      tqWarning("MenuFile::popAction Error, action not last in list.");
      return;
   }
   m_actionList.removeLast();
   delete atom;
}

bool MenuFile::performAllActions()
{
   for(ActionAtom *atom; (atom = m_actionList.getFirst()); m_actionList.removeFirst())
   {
      performAction(atom);
      delete atom;
   }
   
   // Entries that have been removed from the menu are added to .hidden
   // so that they don't re-appear in Lost & Found
   TQStringList removed = m_removedEntries;
   m_removedEntries.clear();
   for(TQStringList::ConstIterator it = removed.begin();
       it != removed.end(); ++it)
   {
      addEntry("/.hidden/", *it);
   }

   m_removedEntries.clear();
   
   if (!m_bDirty)
      return true;
   
   return save();
}

bool MenuFile::dirty()
{
   return (m_actionList.count() != 0) || m_bDirty;
}
