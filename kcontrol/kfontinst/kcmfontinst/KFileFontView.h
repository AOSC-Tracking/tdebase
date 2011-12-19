#ifndef __KFILE_FONT_VIEW_H__
#define __KFILE_FONT_VIEW_H__

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
// NOTE: HEAVILY copied from kfiledetailview.cpp...
//
//   Copyright (C) 1997 Stephan Kulow <coolo@kde.org>
//                 2000, 2001 Carste

class KFileItem;
class TQWidget;
class TQKeyEvent;

#include <klistview.h>
#include <kmimetyperesolver.h>

#include "kfileview.h"

/**
 * An item for the listiew, that has a reference to its corresponding
 * @ref KFileItem.
 */
class CFontListViewItem : public KListViewItem
{
    public:

    CFontListViewItem(TQListView *parent, const TQString &text, const TQPixmap &icon, KFileItem *fi)
	: KListViewItem(parent, text),
          itsInf(fi)
    {
        setPixmap(0, icon);
        setText(0, text);
    }

    CFontListViewItem(TQListView *parent, KFileItem *fi)
        : KListViewItem(parent),
          itsInf(fi)
    {
        init();
    }

    CFontListViewItem(TQListView *parent, const TQString &text, const TQPixmap &icon, KFileItem *fi, TQListViewItem *after)
	: KListViewItem(parent, after),
          itsInf(fi)
    {
        setPixmap(0, icon);
        setText(0, text);
    }

    ~CFontListViewItem() { itsInf->removeExtraData(listView()); }

    /**
     * @returns the corresponding KFileItem
     */
    KFileItem *fileInfo() const { return itsInf; }

    virtual TQString key( int /*column*/, bool /*ascending*/ ) const { return itsKey; }

    void setKey( const TQString& key ) { itsKey = key; }

    TQRect rect() const
    {
        TQRect r = listView()->itemRect(this);

        return TQRect(listView()->viewportToContents(r.topLeft()), TQSize(r.width(), r.height()));
    }

    void init();

    private:

    KFileItem *itsInf;
    TQString   itsKey;

    class CFontListViewItemPrivate;

    CFontListViewItemPrivate *d;
};

/**
 * A list-view capable of showing @ref KFileItem'. Used in the filedialog
 * for example. Most of the documentation is in @ref KFileView class.
 *
 * @see KDirOperator
 * @see KCombiView
 * @see KFileIconView
 */
class CKFileFontView : public KListView, public KFileView
{
    Q_OBJECT

    public:

    CKFileFontView(TQWidget *parent, const char *name);
    virtual ~CKFileFontView();

    virtual TQWidget *   widget() { return this; }
    virtual void        clearView();
    virtual void        setAutoUpdate(bool) {} // ### unused. remove in KDE4
    virtual void        setSelectionMode( KFile::SelectionMode sm );
    virtual void        updateView(bool b);
    virtual void        updateView(const KFileItem *i);
    virtual void        removeItem(const KFileItem *i);
    virtual void        listingCompleted();
    virtual void        setSelected(const KFileItem *i, bool b);
    virtual bool        isSelected(const KFileItem *i) const;
    virtual void        clearSelection();
    virtual void        selectAll();
    virtual void        invertSelection();
    virtual void        setCurrentItem( const KFileItem *i);
    virtual KFileItem * currentFileItem() const;
    virtual KFileItem * firstFileItem() const;
    virtual KFileItem * nextItem(const KFileItem *i) const;
    virtual KFileItem * prevItem(const KFileItem *i) const;
    virtual void        insertItem( KFileItem *i);

    void                readConfig(KConfig *kc, const TQString &group);
    void                writeConfig(KConfig *kc, const TQString &group);

    // implemented to get noticed about sorting changes (for sortingIndicator)
    virtual void        setSorting(TQDir::SortSpec s);
    void                ensureItemVisible(const KFileItem *i);

    // for KMimeTypeResolver
    void                mimeTypeDeterminationFinished();
    void                determineIcon(CFontListViewItem *item);
    TQScrollView *       scrollWidget() const { return (TQScrollView*) this; }

    signals:
    // The user dropped something.
    // fileItem points to the item dropped on or can be 0 if the
    // user dropped on empty space.
    void                dropped(TQDropEvent *event, KFileItem *fileItem);
    // The user dropped the URLs urls.
    // url points to the item dropped on or can be empty if the
    // user dropped on empty space.
    void                dropped(TQDropEvent *event, const KURL::List &urls, const KURL &url);

    protected:

    virtual void        keyPressEvent(TQKeyEvent *e);
    // DND support
    TQDragObject *       dragObject();
    void                contentsDragEnterEvent(TQDragEnterEvent *e);
    void                contentsDragMoveEvent(TQDragMoveEvent *e);
    void                contentsDragLeaveEvent(TQDragLeaveEvent *e);
    void                contentsDropEvent(TQDropEvent *e);
    bool                acceptDrag(TQDropEvent *e) const;

    int itsSortingCol;

    protected slots:

    void                slotSelectionChanged();

    private slots:

    void                slotSortingChanged(int c);
    void                selected(TQListViewItem *item);
    void                slotActivate(TQListViewItem *item);
    void                highlighted(TQListViewItem *item);
    void                slotActivateMenu(TQListViewItem *item, const TQPoint& pos);
    void                slotAutoOpen();

    private:

    virtual void        insertItem(TQListViewItem *i)          { KListView::insertItem(i); }
    virtual void        setSorting(int i, bool b)             { KListView::setSorting(i, b); }
    virtual void        setSelected(TQListViewItem *i, bool b) { KListView::setSelected(i, b); }

    inline CFontListViewItem * viewItem( const KFileItem *item ) const
    {
        return item ? (CFontListViewItem *) item->extraData(this) : NULL;
    }

    void                setSortingKey( CFontListViewItem *item, const KFileItem *i);

    bool                                                itsBlockSortingSignal;
    KMimeTypeResolver<CFontListViewItem,CKFileFontView> *itsResolver;

    protected:

    virtual void virtual_hook(int id, void *data);

    private:

    class CKFileFontViewPrivate;
    CKFileFontViewPrivate *d;
};

#endif
