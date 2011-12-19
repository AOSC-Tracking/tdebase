// kate: space-indent on; indent-width 3; replace-tabs on;
/* This file is part of the KDE project
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

#ifndef __listview_h
#define __listview_h

#include <assert.h>

#include <tqlistview.h>
#include <tqmap.h>
#include <tqvaluevector.h>

#include <klocale.h>
#include <kbookmark.h>
#include <klistview.h>
#include <kiconloader.h>

#include "toplevel.h"

class TQSplitter;
class KListViewSearchLine;

class KEBListViewItem : public TQListViewItem
{
public:
   KEBListViewItem(TQListView *, const KBookmarkGroup &);
   KEBListViewItem(KEBListViewItem *, TQListViewItem *);
   KEBListViewItem(KEBListViewItem *, TQListViewItem *, const KBookmarkGroup &);
   KEBListViewItem(KEBListViewItem *, const KBookmark &);
   KEBListViewItem(KEBListViewItem *, TQListViewItem *, const KBookmark &);

   KEBListViewItem(TQListView *, const KBookmark &);
   KEBListViewItem(TQListView *, TQListViewItem *, const KBookmark &);

   void nsPut(const TQString &nm);

   void modUpdate();

   void setOldStatus(const TQString &);
   void setTmpStatus(const TQString &);
   void restoreStatus();

   void paintCell(TQPainter *p, const TQColorGroup &cg, int column, int width, int alignment);
   void setSelected ( bool s );

   virtual void setOpen(bool);

   bool isEmptyFolderPadder() const { return m_emptyFolderPadder; }
   const KBookmark bookmark() const { return m_bookmark; }

  typedef enum { GreyStyle, BoldStyle, GreyBoldStyle, DefaultStyle } PaintStyle;

   static bool parentSelected(TQListViewItem * item);

private:
   const TQString nsGet() const;
   void normalConstruct(const KBookmark &);

   KBookmark m_bookmark;
   PaintStyle m_paintStyle;
   bool m_emptyFolderPadder;
   TQString m_oldStatus;
   void greyStyle(TQColorGroup &);
   void boldStyle(TQPainter *);
};

class KEBListView : public KListView
{
   Q_OBJECT
public:
   enum { 
      NameColumn = 0,
      UrlColumn = 1,
      CommentColumn = 2,
      StatusColumn = 3,
      AddressColumn = 4
   };
   KEBListView(TQWidget *parent, bool folderList) 
      : KListView(parent), m_folderList(folderList) {}
   virtual ~KEBListView() {}

   void init();
   void makeConnections();
   void readonlyFlagInit(bool);

   void loadColumnSetting();
   void saveColumnSetting();

   void updateByURL(TQString url);

   bool isFolderList() const { return m_folderList; }

   KEBListViewItem* rootItem() const;

public slots:
   virtual void rename(TQListViewItem *item, int c);
   void slotMoved();
   void slotContextMenu(KListView *, TQListViewItem *, const TQPoint &);
   void slotItemRenamed(TQListViewItem *, const TQString &, int);
   void slotDoubleClicked(TQListViewItem *, const TQPoint &, int);
   void slotDropped(TQDropEvent*, TQListViewItem*, TQListViewItem*);
   void slotColumnSizeChanged(int, int, int);

protected:
   virtual bool acceptDrag(TQDropEvent *e) const;
   virtual TQDragObject* dragObject();

private:
   bool m_folderList;
   bool m_widthsDirty;
};

// DESIGN - make some stuff private if possible
class ListView : public TQObject
{
   Q_OBJECT
public:
   // init stuff
   void initListViews();
   void updateListViewSetup(bool readOnly);
   void connectSignals();
   void setSearchLine(KListViewSearchLine * searchline) { m_searchline = searchline; };

   // selected item stuff
   void selected(KEBListViewItem * item, bool s);
   
   void invalidate(const TQString & address);
   void invalidate(TQListViewItem * item);
   void fixUpCurrent(const TQString & address);

   KEBListViewItem * firstSelected() const;
   TQValueVector<KEBListViewItem *> selectedItemsMap() const;

   TQValueList<TQString> selectedAddresses();

   // bookmark helpers
   TQValueList<KBookmark> itemsToBookmarks(const TQValueVector<KEBListViewItem *> & items) const;

   // bookmark stuff
   TQValueList<KBookmark> allBookmarks() const;
   TQValueList<KBookmark> selectedBookmarksExpanded() const;

   // address stuff
   KEBListViewItem* getItemAtAddress(const TQString &address) const;
   TQString userAddress() const;

   // gui stuff - DESIGN - all of it???
   SelcAbilities getSelectionAbilities() const;

   void updateListView();
   void setOpen(bool open); // DESIGN -rename to setAllOpenFlag
   void setCurrent(KEBListViewItem *item, bool select);
   void renameNextCell(bool dir);

   TQWidget *widget() const { return m_listView; }
   void rename(int);
   void clearSelection();

   void updateStatus(TQString url);

   static ListView* self() { return s_self; }
   static void createListViews(TQSplitter *parent);

   void handleMoved(KEBListView *);
   void handleDropped(KEBListView *, TQDropEvent *, TQListViewItem *, TQListViewItem *);
   void handleContextMenu(KEBListView *, KListView *, TQListViewItem *, const TQPoint &);
   void handleDoubleClicked(KEBListView *, TQListViewItem *, const TQPoint &, int);
   void handleItemRenamed(KEBListView *, TQListViewItem *, const TQString &, int);

   static void startRename(int column, KEBListViewItem *item);

   static void deselectAllChildren(KEBListViewItem *item);

   ~ListView();

public slots:
   void slotBkInfoUpdateListViewItem();

private:
   void updateTree();
   void selectedBookmarksExpandedHelper(KEBListViewItem * item, TQValueList<KBookmark> & bookmarks) const;
   void fillWithGroup(KEBListView *, KBookmarkGroup, KEBListViewItem * = 0);

   ListView();

   KEBListView *m_listView;
   KListViewSearchLine * m_searchline;

//  Actually this is a std:set, the bool is ignored
   TQMap<KEBListViewItem *, bool> mSelectedItems;
   bool m_needToFixUp;

   // statics
   static ListView *s_self;
   static int s_myrenamecolumn;
   static KEBListViewItem *s_myrenameitem;
   static TQStringList s_selected_addresses;
   static TQString s_current_address;
};

#endif
