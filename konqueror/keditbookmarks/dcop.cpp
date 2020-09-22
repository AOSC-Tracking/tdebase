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

#include "dcop.h"

#include "listview.h"
#include "toplevel.h"

#include <stdlib.h>

#include <tqclipboard.h>
#include <tqpopupmenu.h>
#include <tqpainter.h>

#include <tdelocale.h>
#include <kbookmarkmanager.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <tdeapplication.h>


KBookmarkEditorIface::KBookmarkEditorIface()
    : TQObject(), DCOPObject("KBookmarkEditor") {
    // connect(KBookmarkNotifier_stub, TQT_SIGNAL( updatedAccessMetadata(TQString,TQString) ), 
    //         this,                   TQT_SLOT( slotDcopUpdatedAccessMetadata(TQString,TQString) ));
    connectDCOPSignal(0, "KBookmarkNotifier", "updatedAccessMetadata(TQString,TQString)", "slotDcopUpdatedAccessMetadata(TQString,TQString)", false);
}

void KBookmarkEditorIface::slotDcopUpdatedAccessMetadata(TQString filename, TQString url) {
    // evil hack, konqi gets updates by way of historymgr,
    // therefore konqi does'nt want "save" notification,
    // unfortunately to stop konqi getting it is difficult
    // and probably not needed until more notifier events
    // are added. therefore, we always updateaccessmetadata
    // without caring about our modified state like normal
    // and thusly there is no need to the bkmgr to do a "save"

    // TODO - i'm not sure this is really true :)

    if (/*KEBApp::self()->modified() &&*/ filename == CurrentMgr::self()->path()) {
        kdDebug() << "slotDcopUpdatedAccessMetadata(" << url << ")" << endl;
        // no undo
        CurrentMgr::self()->mgr()->updateAccessMetadata(url, false);
        ListView::self()->updateStatus(url);
        KEBApp::self()->updateStatus(url);
        // notice - no save here! see! :)
    }
}
#include "dcop.moc"
