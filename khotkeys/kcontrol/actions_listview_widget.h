/****************************************************************************

 KHotKeys

 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.

****************************************************************************/

#ifndef _ACTIONS_LISTVIEW_WIDGET_H_
#define _ACTIONS_LISTVIEW_WIDGET_H_

#include <khlistview.h>

#include <action_data.h>

#include <actions_listview_widget_ui.h>

class TQDragObject;

namespace KHotKeys
{

class Action_listview_item;
class Action_data_base;

class Actions_listview_widget
    : public Actions_listview_widget_ui
    {
    TQ_OBJECT
    public:
        Actions_listview_widget( TQWidget* parent_P = NULL, const char* name_P = NULL );
        Action_listview_item* current_action() const;
        void set_current_action( Action_listview_item* item );
        Action_data_base* current_action_data() const;
        void set_action_data( Action_data_base* data_P, bool recent_action_P = false );
        void action_name_changed( const TQString& name_P );
        void clear();
        void build_up();
        void new_action( Action_data_base* data_P );
        void delete_action();
    private:
        Action_listview_item* create_item( TQListViewItem* parent_P, TQListViewItem* after_P, Action_data_base* data_P );
        void build_up_recursively( Action_data_group* parent_P,
            Action_listview_item* item_parent_P );
        Action_listview_item* recent_item;
        Action_listview_item* saved_current_item;
    private slots:
        void item_moved( TQListViewItem* item_P, TQListViewItem* was_after_P, TQListViewItem* after_P );
        void current_changed( TQListViewItem* item_P );
    signals:
        void current_action_changed();
    };

class Actions_listview
    : public KHListView
    {
    TQ_OBJECT
    public:
        Actions_listview( TQWidget* parent_P = NULL, const char* name_P = NULL );
        Actions_listview_widget* widget();
    private:
        Actions_listview_widget* _widget;
    };

// CHECKME a jak to bude s parent itemu, kdyz Action_data uz maji vlastni parent ?
class Action_listview_item
    : public TQListViewItem
    {
    public:
        virtual TQString text( int column_P ) const;
        Action_data_base* data() const;
        void set_data( Action_data_base* data_P );
        Action_listview_item( TQListView* parent_P, TQListViewItem* after_P,
            Action_data_base* data_P );
        Action_listview_item( TQListViewItem* parent_P, TQListViewItem* after_P,
            Action_data_base* data_P );
    protected:
        Action_data_base* _data; // CHECKME doesn't own !!!
    };

//***************************************************************************
// Inline
//***************************************************************************

// Actions_listview_widget

inline
Action_listview_item* Actions_listview_widget::current_action() const
    {
    return saved_current_item;
    }

inline
Action_data_base* Actions_listview_widget::current_action_data() const
    {
    return current_action() != NULL ? current_action()->data() : NULL;
    }

inline
void Actions_listview_widget::clear()
    {
    actions_listview->clear();
    recent_item = 0;
    saved_current_item = 0;
    }

// Actions_listview

inline
Actions_listview_widget* Actions_listview::widget()
    {
    return _widget;
    }

// Action_listview_item

inline
Action_listview_item::Action_listview_item( TQListView* parent_P, TQListViewItem* after_P,
    Action_data_base* data_P )
    : TQListViewItem( parent_P, after_P ), _data( data_P )
    {
    if( dynamic_cast< Action_data_group* >( data_P ))
        setExpandable( true );
    }

inline
Action_listview_item::Action_listview_item( TQListViewItem* parent_P, TQListViewItem* after_P,
    Action_data_base* data_P )
    : TQListViewItem( parent_P, after_P ), _data( data_P )
    {
    if( dynamic_cast< Action_data_group* >( data_P ))
        setExpandable( true );
    }

inline
Action_data_base* Action_listview_item::data() const
    {
    return _data;
    }

inline
void Action_listview_item::set_data( Action_data_base* data_P )
    {
    _data = data_P;
    }

} // namespace KHotKeys

#endif
