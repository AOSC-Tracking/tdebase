////////////////////////////////////////////////////////////////////////////////
//
// Class Name    : CKFileFontView
// Author        : Craig Drummond
// Project       : K Font Installer
// Creation Date : 31/05/2003
// Version       : $Revision$ $Date$
//
////////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
////////////////////////////////////////////////////////////////////////////////
// (C) Craig Drummond, 2003
////////////////////////////////////////////////////////////////////////////////

//
// NOTE: HEAVILY copied from tdefiledetailview.cpp...
//
//   Copyright (C) 1997 Stephan Kulow <coolo@kde.org>
//                 2000, 2001 Carsten Pfeiffer <pfeiffer@kde.org>
//

#include <tqevent.h>
#include <tqkeycode.h>
#include <tqheader.h>
#include <tqpainter.h>
#include <tqpixmap.h>
#include <tdeapplication.h>
#include <tdefileitem.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <kicontheme.h>
#include <tdelocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kurldrag.h>
#include "KFileFontView.h"

#define COL_NAME 0
#define COL_SIZE 1
#define COL_TYPE 2

class CKFileFontView::CKFileFontViewPrivate
{
    public:

    CKFileFontViewPrivate() : itsDropItem(0) {}

    CFontListViewItem *itsDropItem;
    TQTimer            itsAutoOpenTimer;
};

CKFileFontView::CKFileFontView(TQWidget *parent, const char *name)
              : TDEListView(parent, name),
                KFileView(),
                d(new CKFileFontViewPrivate())
{
    itsSortingCol = COL_NAME;
    itsBlockSortingSignal = false;
    setViewName(i18n("Detailed View"));

    addColumn(i18n("Name"));
    addColumn(i18n("Size"));
    addColumn(i18n("Type"));
    setShowSortIndicator(true);
    setAllColumnsShowFocus(true);
    setDragEnabled(false);

    connect(header(), TQ_SIGNAL(sectionClicked(int)), TQ_SLOT(slotSortingChanged(int)));
    connect(this, TQ_SIGNAL(returnPressed(TQListViewItem *)), TQ_SLOT(slotActivate(TQListViewItem *)));
    connect(this, TQ_SIGNAL(clicked(TQListViewItem *, const TQPoint&, int)), TQ_SLOT(selected( TQListViewItem *)));
    connect(this, TQ_SIGNAL(doubleClicked(TQListViewItem *, const TQPoint &, int)), TQ_SLOT(slotActivate(TQListViewItem *)));
    connect(this, TQ_SIGNAL(contextMenuRequested(TQListViewItem *, const TQPoint &, int)),
	    this, TQ_SLOT(slotActivateMenu(TQListViewItem *, const TQPoint &)));

    // DND
    connect(&(d->itsAutoOpenTimer), TQ_SIGNAL(timeout()), this, TQ_SLOT(slotAutoOpen()));
    setSelectionMode(KFileView::selectionMode());
    itsResolver = new KMimeTypeResolver<CFontListViewItem, CKFileFontView>(this);
}

CKFileFontView::~CKFileFontView()
{
    delete itsResolver;
    delete d;
}

void CKFileFontView::setSelected(const KFileItem *info, bool enable)
{
    if (info)
    {
        // we can only hope that this casts works
        CFontListViewItem *item = (CFontListViewItem*)info->extraData(this);

        if (item)
	    TDEListView::setSelected(item, enable);
    }
}

void CKFileFontView::setCurrentItem(const KFileItem *item)
{
    if (item)
    {
        CFontListViewItem *it = (CFontListViewItem*) item->extraData(this);

        if (it)
            TDEListView::setCurrentItem(it);
    }
}

KFileItem * CKFileFontView::currentFileItem() const
{
    CFontListViewItem *current = static_cast<CFontListViewItem*>(currentItem());

    return current ? current->fileInfo() : NULL;
}

void CKFileFontView::clearSelection()
{
    TDEListView::clearSelection();
}

void CKFileFontView::selectAll()
{
    if (KFile::NoSelection!=KFileView::selectionMode() && KFile::Single!=KFileView::selectionMode())
        TDEListView::selectAll(true);
}

void CKFileFontView::invertSelection()
{
    TDEListView::invertSelection();
}

void CKFileFontView::slotActivateMenu(TQListViewItem *item,const TQPoint& pos)
{
    if (!item)
        sig->activateMenu(0, pos);
    else
    {
        CFontListViewItem *i = (CFontListViewItem*) item;
        sig->activateMenu(i->fileInfo(), pos);
    }
}

void CKFileFontView::clearView()
{
    itsResolver->m_lstPendingMimeIconItems.clear();
    TDEListView::clear();
}

void CKFileFontView::insertItem(KFileItem *i)
{
    KFileView::insertItem(i);

    CFontListViewItem *item = new CFontListViewItem((TQListView*) this, i);

    setSortingKey(item, i);

    i->setExtraData(this, item);

    if (!i->isMimeTypeKnown())
        itsResolver->m_lstPendingMimeIconItems.append(item);
}

void CKFileFontView::slotActivate(TQListViewItem *item)
{
    if (item)
    {
        const KFileItem *fi = ((CFontListViewItem*)item)->fileInfo();

        if (fi)
            sig->activate(fi);
    }
}

void CKFileFontView::selected(TQListViewItem *item)
{
    if (item && !(TDEApplication::keyboardMouseState() & (ShiftButton|ControlButton)) &&
         TDEGlobalSettings::singleClick())
    {
        const KFileItem *fi = ((CFontListViewItem*)item)->fileInfo();

        if (fi && (fi->isDir() || !onlyDoubleClickSelectsFiles()))
            sig->activate(fi);
    }
}

void CKFileFontView::highlighted( TQListViewItem *item )
{
    if (item)
    {
        const KFileItem *fi = ((CFontListViewItem*)item)->fileInfo();

        if (fi)
            sig->highlightFile(fi);
    }
}

void CKFileFontView::setSelectionMode(KFile::SelectionMode sm)
{
    disconnect(TQ_SIGNAL(selectionChanged()), this);
    disconnect(TQ_SIGNAL(selectionChanged(TQListViewItem *)), this);

    switch (sm)
    {
        case KFile::Multi:
            TQListView::setSelectionMode(TQListView::Multi);
            break;
        case KFile::Extended:
            TQListView::setSelectionMode(TQListView::Extended);
            break;
        case KFile::NoSelection:
            TQListView::setSelectionMode(TQListView::NoSelection);
            break;
        default: // fall through
        case KFile::Single:
            TQListView::setSelectionMode(TQListView::Single);
            break;
    }

    // for highlighting
    if (KFile::Multi==sm || KFile::Extended==sm)
        connect(this, TQ_SIGNAL(selectionChanged()), TQ_SLOT(slotSelectionChanged()));
    else
        connect(this, TQ_SIGNAL(selectionChanged(TQListViewItem *)), TQ_SLOT(highlighted(TQListViewItem * )));
}

bool CKFileFontView::isSelected(const KFileItem *i) const
{
    if (!i)
        return false;
    else
    {
        CFontListViewItem *item = (CFontListViewItem*) i->extraData(this);

        return (item && item->isSelected());
    }
}

void CKFileFontView::updateView(bool b)
{
    if (b)
    {
        TQListViewItemIterator it((TQListView*)this);

        for (; it.current(); ++it)
        {
            CFontListViewItem *item=static_cast<CFontListViewItem *>(it.current());

            item->setPixmap(0, item->fileInfo()->pixmap(TDEIcon::SizeSmall));
        }
    }
}

void CKFileFontView::updateView(const KFileItem *i)
{
    if (i)
    {
        CFontListViewItem *item = (CFontListViewItem*) i->extraData(this);

        if (item)
        {
            item->init();
            setSortingKey(item, i);
        }
    }
}

void CKFileFontView::setSortingKey(CFontListViewItem *item, const KFileItem *i)
{
    TQDir::SortSpec spec = KFileView::sorting();

    if (spec&TQDir::Size)
        item->setKey(sortingKey(i->size(), i->isDir(), spec));
    else
        item->setKey(sortingKey(i->text(), i->isDir(), spec));
}

void CKFileFontView::removeItem(const KFileItem *i)
{
    if (i)
    {
        CFontListViewItem *item = (CFontListViewItem*) i->extraData(this);

        itsResolver->m_lstPendingMimeIconItems.remove(item);
        delete item;

        KFileView::removeItem(i);
    }
}

void CKFileFontView::slotSortingChanged(int col)
{
    TQDir::SortSpec sort = sorting();
    int sortSpec = -1;
    bool reversed = col == itsSortingCol && (sort & TQDir::Reversed) == 0;
    itsSortingCol = col;

    switch(col)
    {
        case COL_NAME:
            sortSpec = (sort & ~TQDir::SortByMask | TQDir::Name);
            break;
        case COL_SIZE:
            sortSpec = (sort & ~TQDir::SortByMask | TQDir::Size);
            break;
        // the following columns have no equivalent in TQDir, so we set it
        // to TQDir::Unsorted and remember the column (itsSortingCol)
        case COL_TYPE:
            sortSpec = (sort & ~TQDir::SortByMask | TQDir::Time);
            break;
        default:
            break;
    }

    if (reversed)
        sortSpec|=TQDir::Reversed;
    else
        sortSpec&=~TQDir::Reversed;

    if (sort & TQDir::IgnoreCase)
        sortSpec|=TQDir::IgnoreCase;
    else
        sortSpec&=~TQDir::IgnoreCase;

    KFileView::setSorting(static_cast<TQDir::SortSpec>(sortSpec));

    KFileItem             *item;
    KFileItemListIterator it(*items());

    if ( sortSpec & TQDir::Size )
    {
        for (; (item = it.current()); ++it )
        {
            CFontListViewItem *i = viewItem(item);
            i->setKey(sortingKey(item->size(), item->isDir(), sortSpec));
        }
    }
    else
        for (; (item = it.current()); ++it )
        {
            CFontListViewItem *i = viewItem(item);

            i->setKey(sortingKey(i->text(itsSortingCol), item->isDir(), sortSpec));
        }

    TDEListView::setSorting(itsSortingCol, !reversed);
    TDEListView::sort();

    if (!itsBlockSortingSignal)
        sig->changeSorting( static_cast<TQDir::SortSpec>( sortSpec ) );
}

void CKFileFontView::setSorting(TQDir::SortSpec spec)
{
    if (spec & TQDir::Size)
        itsSortingCol=COL_SIZE;
    else
        itsSortingCol=COL_NAME;

    // inversed, because slotSortingChanged will reverse it
    if (spec & TQDir::Reversed)
        spec = (TQDir::SortSpec) (spec & ~TQDir::Reversed);
    else
        spec = (TQDir::SortSpec) (spec | TQDir::Reversed);

    KFileView::setSorting((TQDir::SortSpec) spec);

    // don't emit sortingChanged() when called via setSorting()
    itsBlockSortingSignal = true; // can't use blockSignals()
    slotSortingChanged(itsSortingCol);
    itsBlockSortingSignal = false;
}

void CKFileFontView::ensureItemVisible(const KFileItem *i)
{
    if (i)
    {
        CFontListViewItem *item = (CFontListViewItem*) i->extraData(this);
        
        if ( item )
            TDEListView::ensureItemVisible(item);
    }
}

// we're in multiselection mode
void CKFileFontView::slotSelectionChanged()
{
    sig->highlightFile(NULL);
}

KFileItem * CKFileFontView::firstFileItem() const
{
    CFontListViewItem *item = static_cast<CFontListViewItem*>(firstChild());

    return item ? item->fileInfo() : NULL;
}

KFileItem * CKFileFontView::nextItem(const KFileItem *fileItem) const
{
    if (fileItem)
    {
        CFontListViewItem *item = viewItem(fileItem);

        return item && item->itemBelow() ? ((CFontListViewItem*) item->itemBelow())->fileInfo() : NULL;
    }

    return firstFileItem();
}

KFileItem * CKFileFontView::prevItem(const KFileItem *fileItem) const
{
    if (fileItem)
    {
        CFontListViewItem *item = viewItem(fileItem);

        return item && item->itemAbove() ? ((CFontListViewItem*) item->itemAbove())->fileInfo() : NULL;
    }

    return firstFileItem();
}

void CKFileFontView::keyPressEvent(TQKeyEvent *e)
{
    TDEListView::keyPressEvent(e);

    if (Key_Return==e->key() || Key_Enter==e->key())
        if (e->state() & ControlButton)
            e->ignore();
        else
            e->accept();
}

//
// mimetype determination on demand
//
void CKFileFontView::mimeTypeDeterminationFinished()
{
    // anything to do?
}

void CKFileFontView::determineIcon(CFontListViewItem *item)
{
    item->fileInfo()->determineMimeType();
    updateView(item->fileInfo());
}

void CKFileFontView::listingCompleted()
{
    itsResolver->start();
}

TQDragObject *CKFileFontView::dragObject()
{
    // create a list of the URL:s that we want to drag
    KURL::List            urls;
    KFileItemListIterator it(* KFileView::selectedItems());
    TQPixmap               pixmap;
    TQPoint                hotspot;

    for ( ; it.current(); ++it )
        urls.append( (*it)->url() );

    if(urls.count()> 1)
        pixmap = DesktopIcon("application-vnd.tde.tdemultiple", TDEIcon::SizeSmall);
    if(pixmap.isNull())
        pixmap = currentFileItem()->pixmap(TDEIcon::SizeSmall);

    hotspot.setX(pixmap.width() / 2);
    hotspot.setY(pixmap.height() / 2);

    TQDragObject *dragObject=new KURLDrag(urls, widget());

    if(dragObject)
        dragObject->setPixmap(pixmap, hotspot);

    return dragObject;
}

void CKFileFontView::slotAutoOpen()
{
    d->itsAutoOpenTimer.stop();

    if(d->itsDropItem)
    {
        KFileItem *fileItem = d->itsDropItem->fileInfo();

        if (fileItem && !fileItem->isFile() && (fileItem->isDir() || fileItem->isLink()))
            sig->activate(fileItem);
    }
}

bool CKFileFontView::acceptDrag(TQDropEvent *e) const
{
#if 0   // Following doesn't seem to work, why???
    bool       ok=false;
    KURL::List urls;


    if((e->source()!=const_cast<CKFileFontView *>(this)) &&
       (TQDropEvent::Copy==e->action() || TQDropEvent::Move==e->action()) &&
       KURLDrag::decode(e, urls) && !urls.isEmpty())
    {
        KURL::List::Iterator it;

        ok=true;
        for(it=urls.begin(); ok && it!=urls.end(); ++it)
            if(!CFontEngine::isAFontOrAfm(TQFile::encodeName((*it).path())))
                ok=false;
    }

    return ok;
#endif

    return KURLDrag::canDecode(e) && (e->source()!= const_cast<CKFileFontView*>(this)) &&
           (TQDropEvent::Copy==e->action() || TQDropEvent::Move==e->action());
}

void CKFileFontView::contentsDragEnterEvent(TQDragEnterEvent *e)
{
    if (!acceptDrag(e)) // can we decode this ?
        e->ignore();            // No
    else
    {
        e->acceptAction();     // Yes

        if((dropOptions() & AutoOpenDirs))
        {
            CFontListViewItem *item = dynamic_cast<CFontListViewItem*>(itemAt(contentsToViewport(e->pos())));
            if (item)  // are we over an item ?
            {
                d->itsDropItem = item;
                d->itsAutoOpenTimer.start(autoOpenDelay()); // restart timer
            }
            else
            {
                d->itsDropItem = 0;
                d->itsAutoOpenTimer.stop();
            }
        }
    }
}

void CKFileFontView::contentsDragMoveEvent(TQDragMoveEvent *e)
{
    if (!acceptDrag(e)) // can we decode this ?
        e->ignore();            // No
    else
    {
        e->acceptAction();     // Yes

        if ((dropOptions() & AutoOpenDirs))
        {
            CFontListViewItem *item = dynamic_cast<CFontListViewItem*>(itemAt(contentsToViewport(e->pos())));

            if (item)  // are we over an item ?
            {
                if (d->itsDropItem != item)
                {
                    d->itsDropItem = item;
                    d->itsAutoOpenTimer.start(autoOpenDelay()); // restart timer
                }
            }
            else
            {
                d->itsDropItem = 0;
                d->itsAutoOpenTimer.stop();
            }
        }
    }
}

void CKFileFontView::contentsDragLeaveEvent(TQDragLeaveEvent *)
{
    d->itsDropItem = 0;
    d->itsAutoOpenTimer.stop();
}

void CKFileFontView::contentsDropEvent(TQDropEvent *e)
{
    d->itsDropItem = 0;
    d->itsAutoOpenTimer.stop();

    if (!acceptDrag(e)) // can we decode this ?
        e->ignore();            // No
    else
    {
        e->acceptAction();     // Yes

        CFontListViewItem *item = dynamic_cast<CFontListViewItem*>(itemAt(contentsToViewport(e->pos())));
        KFileItem         *fileItem = item ? item->fileInfo() : 0;
        KURL::List        urls;

        emit dropped(e, fileItem);

        if(KURLDrag::decode(e, urls) && !urls.isEmpty())
        {
            emit dropped(e, urls, fileItem ? fileItem->url() : KURL());
            sig->dropURLs(fileItem, e, urls);
        }
    }
}

void CKFileFontView::readConfig(TDEConfig *kc, const TQString &group)
{
    restoreLayout(kc, group.isEmpty() ? TQString("CFileFontView") : group);
    slotSortingChanged(sortColumn());
}

void CKFileFontView::writeConfig(TDEConfig *kc, const TQString &group)
{
    saveLayout(kc, group.isEmpty() ? TQString("CFileFontView") : group);
}

/////////////////////////////////////////////////////////////////

void CFontListViewItem::init()
{
    CFontListViewItem::setPixmap(COL_NAME, itsInf->pixmap(TDEIcon::SizeSmall));

    setText(COL_NAME, itsInf->text());
    setText(COL_SIZE, itsInf->isDir() ? "" : TDEGlobal::locale()->formatNumber(itsInf->size(), 0));
    setText(COL_TYPE, itsInf->mimeComment());
}

void CKFileFontView::virtual_hook(int id, void *data)
{
    TDEListView::virtual_hook(id, data);
    KFileView::virtual_hook(id, data);
}

#include "KFileFontView.moc"
