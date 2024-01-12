/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <tqdragobject.h>
#include <tqstring.h>
#include <tqstringlist.h>

#include <tdeglobal.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <tdelocale.h>
#include <kdesktopfile.h>
#include <tdeglobalsettings.h>
#include <tdeapplication.h>
#include <kurldrag.h>
#include <tderecentdocument.h>

#include "recentdocsmenu.h"

K_EXPORT_KICKER_MENUEXT(recentdocs, RecentDocsMenu)

RecentDocsMenu::RecentDocsMenu(TQWidget *parent, const char *name,
                               const TQStringList &/*args*/)
    : KPanelMenu(TDERecentDocument::recentDocumentDirectory(), parent, name)
{
}

RecentDocsMenu::~RecentDocsMenu()
{
}

void RecentDocsMenu::initialize() {
	if (initialized()) clear();

	insertItem(SmallIconSet("history_clear"), i18n("Clear History"), this, TQ_SLOT(slotClearHistory()));
	insertSeparator();

	_fileList = TDERecentDocument::recentDocuments();

	if (_fileList.isEmpty()) {
		insertItem(i18n("No Entries"), 0);
		setItemEnabled(0, false);
		return;
	}

	int id = 0;
	char alreadyPresentInMenu;
	TQStringList previousEntries;
	for (TQStringList::ConstIterator it = _fileList.begin(); it != _fileList.end(); ++it) {
		KDesktopFile f(*it, true /* read only */);

		// Make sure this entry is not already present in the menu
		alreadyPresentInMenu = 0;
		for ( TQStringList::Iterator previt = previousEntries.begin(); previt != previousEntries.end(); ++previt ) {
			if (TQString::localeAwareCompare(*previt, f.readName().replace('&', TQString::fromAscii("&&") )) == 0) {
				alreadyPresentInMenu = 1;
			}
		}

		if (alreadyPresentInMenu == 0) {
			// Add item to menu
			insertItem(SmallIconSet(f.readIcon()), f.readName().replace('&', TQString::fromAscii("&&") ), id++);

			// Append to duplicate checking list
			previousEntries.append(f.readName().replace('&', TQString::fromAscii("&&") ));
		}
	}

    setInitialized(true);
}

void RecentDocsMenu::slotClearHistory() {
    TDERecentDocument::clear();
    reinitialize();
}

void RecentDocsMenu::slotExec(int id) {
	if (id >= 0) {
		kapp->propagateSessionManager();
		KURL u;
		u.setPath(_fileList[id]);
		KDEDesktopMimeType::run(u, true);
	}
}

void RecentDocsMenu::mousePressEvent(TQMouseEvent* e) {
	_mouseDown = e->pos();
	TQPopupMenu::mousePressEvent(e);
}

void RecentDocsMenu::mouseMoveEvent(TQMouseEvent* e) {
	KPanelMenu::mouseMoveEvent(e);

	if (!(e->state() & TQt::LeftButton))
		return;

	if (!rect().contains(_mouseDown))
		return;

	int dragLength = (e->pos() - _mouseDown).manhattanLength();

	if (dragLength <= TDEGlobalSettings::dndEventDelay())
		return;  // ignore it

	int id = idAt(_mouseDown);

	// Don't drag 'manual' items.
	if (id < 0)
		return;

	KDesktopFile f(_fileList[id], true /* read only */);

	KURL url ( f.readURL() );

	if (url.isEmpty()) // What are we to do ?
		return;

	KURL::List lst;
	lst.append(url);

	KURLDrag* d = new KURLDrag(lst, this);
	d->setPixmap(SmallIcon(f.readIcon()));
	d->dragCopy();
	close();
}

void RecentDocsMenu::slotAboutToShow()
{
    reinitialize();
    KPanelMenu::slotAboutToShow();
}

#include "recentdocsmenu.moc"
