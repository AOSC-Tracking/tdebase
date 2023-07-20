/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _WINDOWDEF_LIST_WIDGET_H_
#define _WINDOWDEF_LIST_WIDGET_H_

#include <tqlistview.h>

#include <kdialogbase.h>

#include <windows.h>
#include <windowdef_list_widget_ui.h>

namespace KHotKeys
{

class Action_data;
class Action_data_base;
class Windowdef_simple_widget;

class Windowdef_list_item;

class Windowdef_list_widget
    : public Windowdef_list_widget_ui
    {
    TQ_OBJECT
    public:
        Windowdef_list_widget( TQWidget* parent_P = NULL, const char* name_P = NULL );
        virtual ~Windowdef_list_widget();
        void set_data( const Windowdef_list* data_P );
        Windowdef_list* get_data() const;
        void set_autodetect( TQObject* obj_P, const char* slot_P );
    public slots:
        void clear_data();
    protected:
        Windowdef_list_item* create_listview_item( Windowdef* window_P, TQListView* parent1_P,
            TQListViewItem* parent2_P, TQListViewItem* after_P, bool copy_P );
        void edit_listview_item( Windowdef_list_item* item_P );
        enum type_t { TYPE_WINDOWDEF_SIMPLE };
    protected slots:
        void new_selected( int type_P );
        virtual void copy_pressed();
        virtual void delete_pressed();
        virtual void modify_pressed();
        virtual void current_changed( TQListViewItem* item_P );
    protected:
        TQObject* autodetect_object;
        const char* autodetect_slot;
        Windowdef_list_item* selected_item;
    };

typedef Windowdef_list_widget Windowdef_list_tab;

class Windowdef_list_item
    : public TQListViewItem
    {
    public:
        Windowdef_list_item( TQListView* parent_P, Windowdef* window_P );
        Windowdef_list_item( TQListViewItem* parent_P, Windowdef* window_P );
        Windowdef_list_item( TQListView* parent_P, TQListViewItem* after_P, Windowdef* window_P );
        Windowdef_list_item( TQListViewItem* parent_P, TQListViewItem* after_P, Windowdef* window_P );
        virtual ~Windowdef_list_item();
        virtual TQString text( int column_P ) const;
        Windowdef* window() const;
        void set_window( Windowdef* window_P );
    protected:
        Windowdef* _window; // owns it
    };
        
class Windowdef_dialog
    {
    public:
        virtual Windowdef* edit_windowdef() = 0;
        virtual ~Windowdef_dialog();
    };
    
class Windowdef_simple_dialog
    : public KDialogBase, public Windowdef_dialog
    {
    TQ_OBJECT
    public:
        Windowdef_simple_dialog( Windowdef_simple* window_P, TQObject* obj_P, const char* slot_P );
        virtual Windowdef* edit_windowdef();
    protected:
        virtual void accept();
        Windowdef_simple_widget* widget;
        Windowdef_simple* window;
    };
        
//***************************************************************************
// Inline
//***************************************************************************

// Windowdef_list_widget

inline
void Windowdef_list_widget::set_autodetect( TQObject* obj_P, const char* slot_P )
    {
    autodetect_object = obj_P;
    autodetect_slot = slot_P;
    }

// Windowdef_list_item

inline
Windowdef_list_item::Windowdef_list_item( TQListView* parent_P, Windowdef* window_P )
    : TQListViewItem( parent_P ), _window( window_P )
    {
    }
    
inline
Windowdef_list_item::Windowdef_list_item( TQListViewItem* parent_P, Windowdef* window_P )
    : TQListViewItem( parent_P ), _window( window_P )
    {
    }

inline
Windowdef_list_item::Windowdef_list_item( TQListView* parent_P, TQListViewItem* after_P,
    Windowdef* window_P )
    : TQListViewItem( parent_P, after_P ), _window( window_P )
    {
    }

inline
Windowdef_list_item::Windowdef_list_item( TQListViewItem* parent_P, TQListViewItem* after_P,
    Windowdef* window_P )
    : TQListViewItem( parent_P, after_P ), _window( window_P )
    {
    }

inline
Windowdef* Windowdef_list_item::window() const
    {
    return _window;
    }
    
inline
void Windowdef_list_item::set_window( Windowdef* window_P )
    {
    delete _window;
    _window = window_P;
    }

// Windowdef_dialog

inline
Windowdef_dialog::~Windowdef_dialog()
    {
    }

} // namespace KHotKeys

#endif
