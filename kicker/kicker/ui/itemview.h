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

#ifndef __itemview_h__
#define __itemview_h__

#include <dcopobject.h>
#include <tqintdict.h>
#include <tqpixmap.h>
#include <tqframe.h>
#include <tqtoolbutton.h>
#include <klistview.h>
#include <tqdragobject.h>

#include "kmenubase.h"
#include "kmenuitembase.h"
#include "service_mnu.h"

class KickerClientMenu;
class KBookmarkMenu;
class KActionCollection;
class KBookmarkOwner;
class Panel;
class TQWidgetStack;
class KHistoryCombo;
class TQScrollView;
class PopupMenuTitle;
class TQWidget;
class TQVBoxLayout;
class TQTimer;
class KPixmap;

class KMenuItem : public TQListViewItem
{
public:
    KMenuItem(int nId, TQListView* parent) : TQListViewItem(parent), m_id(nId) { init(); }
    KMenuItem(int nId, TQListViewItem* parent) : TQListViewItem(parent), m_id(nId) { init(); }
    ~KMenuItem();

    void setIcon(const TQString& icon, int size);
    TQString icon() const { return m_icon; }
    void setTitle( const TQString& text );
    TQString title() const { return m_title; }
    void setToolTip( const TQString& text );
    TQString toolTip() const { return m_tooltip; }
    void setDescription(const TQString& text);
    TQString description() const { return m_description; }
    void setService(KService::Ptr& s) { m_s = s; }
    KService::Ptr service() { return m_s; }
    void setPath(const TQString& u) { m_path = u; }
    TQString path() const { return m_path; }
    void setMenuPath(const TQString& u) { m_menuPath = u; }
    TQString menuPath() const { return m_menuPath; }
    int id() const { return m_id; }
    void setHasChildren(bool flag);
    bool hasChildren() const { return m_has_children; }
    void makeGradient(KPixmap &off, const TQColor& c);

protected:
    virtual void paintCell(TQPainter* p, const TQColorGroup & cg, int column, int width, int align);
    virtual void paintCellInter(TQPainter* p, const TQColorGroup & cg, int column, int width, int align);
    virtual void setup();

private:
    void init();

    int m_id;
    KService::Ptr m_s;
    TQString m_title;
    TQString m_description;
    TQString m_path;
    TQString m_icon;
    TQString m_tooltip;
    TQString m_menuPath;
    float title_font_size;
    float description_font_size;
    bool m_has_children;
    int m_old_width;
    TQPixmap right_triangle;
};

class KMenuItemSeparator : public KMenuItem
{
public:
    KMenuItemSeparator(int nId, TQListView* parent);
    virtual void setup();

    virtual void paintCell(TQPainter* p, const TQColorGroup & cg, int column, int width, int align);
    void setLink(const TQString &text, const TQString &link = TQString::null );

    TQString linkUrl() const { return m_link_url; }

    /// returns true if the cursor has to change
    bool hitsLink(const TQPoint &pos);

protected:
    void preparePixmap(int width);
    TQPixmap pixmap;
    int left_margin;

private:
    TQListView* lv;
    int cached_width;
    TQString m_link_text, m_link_url;
    TQRect m_link_rect;

};

class KMenuItemHeader : public KMenuItemSeparator
{
public:
    KMenuItemHeader( int nId, const TQString &relpath, TQListView* parent);
    virtual void setup();

    virtual void paintCell(TQPainter* p, const TQColorGroup & cg, int column, int width, int align);

private:
    TQListView* lv;
    TQStringList paths;
    TQStringList texts;
    TQStringList icons;
    TQPixmap left_triangle;
};

class KMenuSpacer : public KMenuItem
{
public:
    KMenuSpacer(int nId, TQListView* parent);
    virtual void paintCell(TQPainter* p, const TQColorGroup & cg, int column, int width, int align);
    virtual void setup();

    void setHeight(int);
};

class ItemView : public KListView
{
    friend class KMenuItem;

    Q_OBJECT
public:
    ItemView(TQWidget* parent, const char* name = 0);

    KMenuItem* insertItem( const TQString& icon, const TQString& text, const TQString& description, int nId, int nIndex, KMenuItem* parentItem = 0 );
    KMenuItem* insertItem( const TQString& icon, const TQString& text, const TQString& description, const TQString& path, int nId, int nIndex, KMenuItem* parentItem = 0 );
    int insertItem( PopupMenuTitle*, int, int);
    int setItemEnabled(int id, bool enabled);
    KMenuItemSeparator *insertSeparator(int id, const TQString& text, int nIndex);
    KMenuItemHeader *insertHeader(int id, const TQString &relpath);
    KMenuItem* insertMenuItem(KService::Ptr & s, int nId, int nIndex = -1, KMenuItem* parentItem = 0,
                        const TQString &aliasname = TQString::null, const TQString &label = TQString::null,
                        const TQString &categoryIcon = TQString::null);
    KMenuItem* insertRecentlyItem(const TQString& s, int nId, int nIndex = -1);
    KMenuItem* insertDocumentItem(const TQString& s, int nId, int nIndex = -1 , const TQStringList* suppressGenericNames = 0,
                        const TQString& aliasname = TQString::null);
    KMenuItem* insertSubItem(const TQString& icon, const TQString& caption, const TQString& description, const TQString& path, KMenuItem* parentItem);
    KMenuItem* findItem(int nId);

    void setIconSize(int size) { m_iconSize = size; }
    void setMouseMoveSelects(bool select) { m_mouseMoveSelects = select; }
    void clear();
    int goodHeight();
    TQString path;
    void setBackPath( const TQString &str ) { m_back_url = str; }
    TQString backPath() const { return m_back_url; }

public slots:
    void slotItemClicked(TQListViewItem*);
    void slotMoveContent();

signals:
    void startService(KService::Ptr kservice);
    void startURL(const TQString& u);

protected:
    void contentsMouseMoveEvent(TQMouseEvent *e);
    void contentsMousePressEvent ( TQMouseEvent * e );
    void contentsWheelEvent(TQWheelEvent *e);
    void leaveEvent(TQEvent *e);
    virtual void resizeEvent ( TQResizeEvent * e );
    virtual void viewportPaintEvent ( TQPaintEvent * pe );
    virtual TQDragObject* dragObject ();
    virtual bool acceptDrag (TQDropEvent* event) const;
    virtual bool focusNextPrevChild(bool next);

private slots:
    void slotItemClicked(int button, TQListViewItem * item, const TQPoint & pos, int c );

private:
    KMenuItem* itemAtIndex(int nIndex);
    void moveItemToIndex(KMenuItem*, int);

    TQWidget* m_itemBox;
    TQVBoxLayout* m_itemLayout;
    KMenuItem *m_lastOne;
    KMenuSpacer *m_spacer;

    TQString m_back_url;

    bool m_mouseMoveSelects;
    int m_iconSize;
    int m_old_contentY;
};

class FavoritesItemView : public ItemView
{
public:
    FavoritesItemView(TQWidget* parent, const char* name = 0);

protected:
    virtual bool acceptDrag (TQDropEvent* event) const;
};

class KMenuItemInfo
{
public:
    int m_id;
    KService::Ptr m_s;
    TQString m_title;
    TQString m_description;
    TQString m_path;
    TQString m_icon;
};

class KMenuItemDrag : public TQDragObject
{
    public:
        KMenuItemDrag(KMenuItem& item, TQWidget *dragSource);
        ~KMenuItemDrag();

        virtual const char * format(int i = 0) const;
        virtual TQByteArray encodedData(const char *) const;

        static bool canDecode(const TQMimeSource * e);
        static bool decode(const TQMimeSource* e, KMenuItemInfo& item);

    private:
        TQByteArray a;
};

#endif
