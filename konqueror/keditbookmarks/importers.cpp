// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
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

#include "importers.h"

#include "commands.h"
#include "toplevel.h"
#include "listview.h"

#include <tqregexp.h>
#include <kdebug.h>
#include <tdelocale.h>

#include <tdemessagebox.h>
#include <tdefiledialog.h>

#include <kbookmarkmanager.h>

#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
#include <kbookmarkimporter_crash.h>
#include <kbookmarkdombuilder.h>

TQString ImportCommand::name() const {
    return i18n("Import %1 Bookmarks").arg(visibleName());
}

TQString ImportCommand::folder() const {
    return m_folder ? i18n("%1 Bookmarks").arg(visibleName()) : TQString();
}

ImportCommand* ImportCommand::importerFactory(const TQCString &type) {
    if (type == "Galeon") return new GaleonImportCommand();
    else if (type == "IE") return new IEImportCommand();
    else if (type == "KDE2") return new KDE2ImportCommand();
    else if (type == "Opera") return new OperaImportCommand();
    else if (type == "Crashes") return new CrashesImportCommand();
    else if (type == "Moz") return new MozImportCommand();
    else if (type == "NS") return new NSImportCommand();
    else {
        kdError() << "ImportCommand::importerFactory() - invalid type (" << type << ")!" << endl;
        return 0;
    }
}

ImportCommand* ImportCommand::performImport(const TQCString &type, TQWidget *top) {
    ImportCommand *importer = ImportCommand::importerFactory(type);

    TQString mydirname = importer->requestFilename();
    if (mydirname.isEmpty()) {
        delete importer;
        return 0;
    }

    int answer =
        KMessageBox::questionYesNoCancel(
                top, i18n("Import as a new subfolder or replace all the current bookmarks?"),
                i18n("%1 Import").arg(importer->visibleName()),
                i18n("As New Folder"), i18n("Replace"));

    if (answer == KMessageBox::Cancel) {
        delete importer;
        return 0;
    }

    importer->import(mydirname, answer == KMessageBox::Yes);
    return importer;
}

void ImportCommand::doCreateHoldingFolder(KBookmarkGroup &bkGroup) {
    bkGroup = CurrentMgr::self()->mgr()
        ->root().createNewFolder(CurrentMgr::self()->mgr(), folder(), false);
    bkGroup.internalElement().setAttribute("icon", m_icon);
    m_group = bkGroup.address();
}

void ImportCommand::execute() {
    KBookmarkGroup bkGroup;

    if (!folder().isNull()) {
        doCreateHoldingFolder(bkGroup);

    } else {
        // import into the root, after cleaning it up
        bkGroup = CurrentMgr::self()->mgr()->root();
        delete m_cleanUpCmd;
        m_cleanUpCmd = DeleteCommand::deleteAll(bkGroup);

        KMacroCommand *mcmd = (KMacroCommand*) m_cleanUpCmd;
        mcmd->addCommand(new DeleteCommand(bkGroup.address(),
                    true /* contentOnly */));
        m_cleanUpCmd->execute();

        // import at the root
        m_group = "";
    }

    doExecute(bkGroup);
}

void ImportCommand::unexecute() {
    if ( !folder().isEmpty() ) {
        // we created a group -> just delete it
        DeleteCommand cmd(m_group);
        cmd.execute();

    } else {
        // we imported at the root -> delete everything
        KBookmarkGroup root = CurrentMgr::self()->mgr()->root();
        KCommand *cmd = DeleteCommand::deleteAll(root);

        cmd->execute();
        delete cmd;

        // and recreate what was there before
        m_cleanUpCmd->unexecute();
    }
}

TQString ImportCommand::affectedBookmarks() const
{
    TQString rootAdr = CurrentMgr::self()->mgr()->root().address();
    if(m_group == rootAdr)
        return m_group;
    else
        return KBookmark::parentAddress(m_group);
}

/* -------------------------------------- */

TQString MozImportCommand::requestFilename() const {
    static KMozillaBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

TQString NSImportCommand::requestFilename() const {
    static KNSBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

TQString OperaImportCommand::requestFilename() const {
    static KOperaBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

TQString CrashesImportCommand::requestFilename() const {
    static TDECrashBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

TQString IEImportCommand::requestFilename() const {
    static KIEBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

// following two are really just xbel

TQString GaleonImportCommand::requestFilename() const {
    return KFileDialog::getOpenFileName(
            TQDir::homeDirPath() + "/.galeon",
            i18n("*.xbel|Galeon Bookmark Files (*.xbel)"));
}

#include "kstandarddirs.h"

TQString KDE2ImportCommand::requestFilename() const {
    return KFileDialog::getOpenFileName(
            locateLocal("data", "konqueror"),
            i18n("*.xml|TDE Bookmark Files (*.xml)"));
}

/* -------------------------------------- */

static void parseInto(const KBookmarkGroup &bkGroup, KBookmarkImporterBase *importer) {
    KBookmarkDomBuilder builder(bkGroup, CurrentMgr::self()->mgr());
    builder.connectImporter(importer);
    importer->parse();
}

void OperaImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    KOperaBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void CrashesImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    TDECrashBookmarkImporterImpl importer;
    importer.setShouldDelete(true);
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void IEImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    KIEBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void HTMLImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    KNSBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    importer.setUtf8(m_utf8);
    parseInto(bkGroup, &importer);
}

/* -------------------------------------- */

void XBELImportCommand::doCreateHoldingFolder(KBookmarkGroup &) {
    // rather than reuse the old group node we transform the
    // root xbel node into the group when doing an xbel import
}

void XBELImportCommand::doExecute(const KBookmarkGroup &/*bkGroup*/) {
    // check if already open first???
    KBookmarkManager *pManager = KBookmarkManager::managerForFile(m_fileName, false);

    TQDomDocument doc = CurrentMgr::self()->mgr()->internalDocument();

    // get the xbel
    TQDomNode subDoc = pManager->internalDocument().namedItem("xbel").cloneNode();
    if (subDoc.isProcessingInstruction())
        subDoc = subDoc.nextSibling();
    if (subDoc.isDocumentType())
        subDoc = subDoc.nextSibling();
    if (subDoc.nodeName() != "xbel")
        return;

    if (!folder().isEmpty()) {
        // transform into folder
        subDoc.toElement().setTagName("folder");

        // clear attributes
        TQStringList tags;
        for (uint i = 0; i < subDoc.attributes().count(); i++)
            tags << subDoc.attributes().item(i).toAttr().name();
        for (TQStringList::Iterator it = tags.begin(); it != tags.end(); ++it)
            subDoc.attributes().removeNamedItem((*it));

        subDoc.toElement().setAttribute("icon", m_icon);

        // give the folder a name
        TQDomElement textElem = doc.createElement("title");
        subDoc.insertBefore(textElem, subDoc.firstChild());
        textElem.appendChild(doc.createTextNode(folder()));
    }

    // import and add it
    TQDomNode node = doc.importNode(subDoc, true);

    if (!folder().isEmpty()) {
        CurrentMgr::self()->mgr()->root().internalElement().appendChild(node);
        m_group = KBookmarkGroup(node.toElement()).address();

    } else {
        TQDomElement root = CurrentMgr::self()->mgr()->root().internalElement();

        TQValueList<TQDomElement> childList;

        TQDomNode n = subDoc.firstChild().toElement();
        while (!n.isNull()) {
            TQDomElement e = n.toElement();
            if (!e.isNull())
                childList.append(e);
            n = n.nextSibling();
        }

        TQValueList<TQDomElement>::Iterator it = childList.begin();
        TQValueList<TQDomElement>::Iterator end = childList.end();
        for (; it!= end ; ++it)
            root.appendChild((*it));
    }
}

#include "importers.moc"
