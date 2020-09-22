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

#ifndef __importers_h
#define __importers_h

#include "commands.h"
#include <tdelocale.h>
#include <tdeio/job.h>

#include <kcommand.h>
#include <kbookmark.h>

#include <tqptrstack.h>
#include <tqobject.h>

// part pure
class ImportCommand : public TQObject, public KCommand, public IKEBCommand
{
   Q_OBJECT
public:
   ImportCommand()
      : KCommand(), m_utf8(false), m_folder(false), m_cleanUpCmd(0L)
   { ; }

   virtual void import(const TQString &fileName, bool folder) = 0;

   virtual TQString name() const;
   virtual TQString visibleName() const { return m_visibleName; }
   virtual TQString requestFilename() const = 0;

   static ImportCommand* performImport(const TQCString &, TQWidget *);
   static ImportCommand* importerFactory(const TQCString &);

   virtual ~ImportCommand()
   { ; }

   virtual void execute();
   virtual void unexecute();
   virtual TQString affectedBookmarks() const;

   TQString groupAddress() const { return m_group; }
   TQString folder() const;

protected:
   /**
    * @param fileName HTML file to import
    * @param folder name of the folder to create. Empty for no creation (root()).
    * @param icon icon for the new folder, if @p folder isn't empty
    * @param utf8 true if the HTML is in utf-8 encoding
    */
   void init(const TQString &fileName, bool folder, const TQString &icon, bool utf8)
   {
      m_fileName = fileName;
      m_folder = folder;
      m_icon = icon;
      m_utf8 = utf8;
   }

   virtual void doCreateHoldingFolder(KBookmarkGroup &bkGroup);
   virtual void doExecute(const KBookmarkGroup &) = 0;

protected:
   TQString m_visibleName;
   TQString m_fileName;
   TQString m_icon;
   TQString m_group;
   bool m_utf8;

private:
   bool m_folder;
   KMacroCommand *m_cleanUpCmd;
};

// part pure
class XBELImportCommand : public ImportCommand
{
public:
   XBELImportCommand() : ImportCommand() { ; }
   virtual void import(const TQString &fileName, bool folder) = 0;
   virtual TQString requestFilename() const = 0;
private:
   virtual void doCreateHoldingFolder(KBookmarkGroup &bkGroup);
   virtual void doExecute(const KBookmarkGroup &);
};

class GaleonImportCommand : public XBELImportCommand
{
public:
   GaleonImportCommand() : XBELImportCommand() { m_visibleName = i18n("Galeon"); }
   virtual void import(const TQString &fileName, bool folder) {
      init(fileName, folder, "", false);
   }
   virtual TQString requestFilename() const;
};

class KDE2ImportCommand : public XBELImportCommand
{
public:
   KDE2ImportCommand() : XBELImportCommand() { m_visibleName = i18n("KDE"); }
   virtual void import(const TQString &fileName, bool folder) {
      init(fileName, folder, "", false);
   }
   virtual TQString requestFilename() const;
};

// part pure
class HTMLImportCommand : public ImportCommand
{
public:
   HTMLImportCommand() : ImportCommand() { ; }
   virtual void import(const TQString &fileName, bool folder) = 0;
   virtual TQString requestFilename() const = 0;
private:
   virtual void doExecute(const KBookmarkGroup &);
};

class NSImportCommand : public HTMLImportCommand
{
public:
   NSImportCommand() : HTMLImportCommand() { m_visibleName = i18n("Netscape"); }
   virtual void import(const TQString &fileName, bool folder) {
      init(fileName, folder, "netscape", false);
   }
   virtual TQString requestFilename() const;
};

class MozImportCommand : public HTMLImportCommand
{
public:
   MozImportCommand() : HTMLImportCommand() { m_visibleName = i18n("Mozilla"); }
   virtual void import(const TQString &fileName, bool folder) {
      init(fileName, folder, "mozilla", true);
   }
   virtual TQString requestFilename() const;
};

class IEImportCommand : public ImportCommand
{
public:
   IEImportCommand() : ImportCommand() { m_visibleName = i18n("IE"); }
   virtual void import(const TQString &fileName, bool folder) {
      init(fileName, folder, "", false);
   }
   virtual TQString requestFilename() const;
private:
   virtual void doExecute(const KBookmarkGroup &);
};

class OperaImportCommand : public ImportCommand
{
public:
   OperaImportCommand() : ImportCommand() { m_visibleName = i18n("Opera"); }
   virtual void import(const TQString &fileName, bool folder) {
      init(fileName, folder, "opera", false);
   }
   virtual TQString requestFilename() const;
private:
   virtual void doExecute(const KBookmarkGroup &);
};

class CrashesImportCommand : public ImportCommand
{
public:
   CrashesImportCommand() : ImportCommand() { m_visibleName = i18n("Crashes"); }
   virtual void import(const TQString &fileName, bool folder) {
      init(fileName, folder, "core", false);
   }
   virtual TQString requestFilename() const;
private:
   virtual void doExecute(const KBookmarkGroup &);
};

#endif
