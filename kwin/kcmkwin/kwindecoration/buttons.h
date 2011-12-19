/*
	This is the new kwindecoration kcontrol module

	Copyright (c) 2004, Sandro Giessl <sandro@giessl.com>
	Copyright (c) 2001
		Karol Szwed <gallium@kde.org>
		http://gallium.n3.net/

	Supports new kwin configuration plugins, and titlebar button position
	modification via dnd interface.

	Based on original "kwintheme" (Window Borders) 
	Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef __BUTTONS_H_
#define __BUTTONS_H_

#include <tqbitmap.h>
#include <tqevent.h>
#include <tqdragobject.h>
#include <tqlistbox.h>

#include <klistview.h>

class KDecorationFactory;

/**
 * This class holds the button data.
 */
class Button
{
	public:
		Button();
		Button(const TQString& name, const TQBitmap& icon, TQChar type, bool duplicate, bool supported);
		virtual ~Button();

		TQString name;
		TQBitmap icon;
		TQChar type;
		bool duplicate;
		bool supported;
};

class ButtonDrag : public TQStoredDrag
{
	public:
		ButtonDrag( Button btn, TQWidget* parent, const char* name=0 );
		~ButtonDrag() {};

		static bool canDecode( TQDropEvent* e );
		static bool decode( TQDropEvent* e, Button& btn );
};

/**
 * This is plugged into ButtonDropSite
 */
class ButtonDropSiteItem
{
	public:
		ButtonDropSiteItem(const Button& btn);
		~ButtonDropSiteItem();
	
		Button button();

		TQRect rect;
		int width();
		int height();

		void draw(TQPainter *p, const TQColorGroup& cg, TQRect rect);

	private:
		Button m_button;
};

/**
 * This is plugged into ButtonSource
 */
class ButtonSourceItem : public TQListViewItem
{
	public:
		ButtonSourceItem(TQListView * parent, const Button& btn);
		virtual ~ButtonSourceItem();

		void paintCell(TQPainter *p, const TQColorGroup &cg, int column, int width, int align);

		void setButton(const Button& btn);
		Button button() const;
	private:
		Button m_button;
		bool m_dirty;
};

/**
 * Implements the button drag source list view
 */
class ButtonSource : public KListView
{
	Q_OBJECT

	public:
		ButtonSource(TQWidget *parent = 0, const char* name = 0);
		virtual ~ButtonSource();

		TQSize sizeHint() const;

		void hideAllButtons();
		void showAllButtons();

	public slots:
		void hideButton(TQChar btn);
		void showButton(TQChar btn);

	protected:
		bool acceptDrag(TQDropEvent* e) const;
		virtual TQDragObject *dragObject();
};

typedef TQValueList<ButtonDropSiteItem*> ButtonList;

/**
 * This class renders and handles the demo titlebar dropsite
 */
class ButtonDropSite: public TQFrame
{
	Q_OBJECT

	public:
		ButtonDropSite( TQWidget* parent=0, const char* name=0 );
		~ButtonDropSite();

		// Allow external classes access our buttons - ensure buttons are
		// not duplicated however.
		ButtonList buttonsLeft;
		ButtonList buttonsRight;
		void clearLeft();
		void clearRight();

	signals:
		void buttonAdded(TQChar btn);
		void buttonRemoved(TQChar btn);
		void changed();

	public slots:
		bool removeSelectedButton(); ///< This slot is called after we drop on the item listbox...
		void recalcItemGeometry(); ///< Call this whenever the item list changes... updates the items' rect property

	protected:
		void resizeEvent(TQResizeEvent*);
		void dragEnterEvent( TQDragEnterEvent* e );
		void dragMoveEvent( TQDragMoveEvent* e );
		void dragLeaveEvent( TQDragLeaveEvent* e );
		void dropEvent( TQDropEvent* e );
		void mousePressEvent( TQMouseEvent* e ); ///< Starts dragging a button...

		void drawContents( TQPainter* p );
		ButtonDropSiteItem *buttonAt(TQPoint p);
		bool removeButton(ButtonDropSiteItem *item);
		int calcButtonListWidth(const ButtonList& buttons); ///< Computes the total space the buttons will take in the titlebar
		void drawButtonList(TQPainter *p, const ButtonList& buttons, int offset);

		TQRect leftDropArea();
		TQRect rightDropArea();

	private:
		/**
		 * Try to find the item. If found, set its list and iterator and return true, else return false
		 */
		bool getItemIterator(ButtonDropSiteItem *item, ButtonList* &list, ButtonList::iterator &iterator);

		void cleanDropVisualizer();
		TQRect m_oldDropVisualizer;

		ButtonDropSiteItem *m_selected;
};

class ButtonPositionWidget : public TQWidget
{
	Q_OBJECT

	public:
		ButtonPositionWidget(TQWidget *parent = 0, const char* name = 0);
		~ButtonPositionWidget();

		/**
		 * set the factory, so the class e.g. knows which buttons are supported by the client
		 */
		void setDecorationFactory(KDecorationFactory *factory);

		TQString buttonsLeft() const;
		TQString buttonsRight() const;
		void setButtonsLeft(const TQString &buttons);
		void setButtonsRight(const TQString &buttons);

	signals:
		void changed();

	private:
		void clearButtonList(const ButtonList& btns);
		Button getButton(TQChar type, bool& success);

		ButtonDropSite* m_dropSite;
		ButtonSource *m_buttonSource;

		KDecorationFactory *m_factory;
		TQString m_supportedButtons;
};


#endif
// vim: ts=4
// kate: space-indent off; tab-width 4;
