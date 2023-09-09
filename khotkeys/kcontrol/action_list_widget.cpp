/****************************************************************************

 KHotKeys

 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.

****************************************************************************/

#define _ACTION_LIST_WIDGET_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "action_list_widget.h"

#include <assert.h>
#include <tqpopupmenu.h>
#include <tqpushbutton.h>
#include <tqheader.h>
#include <tqlineedit.h>

#include <tdelocale.h>
#include <kdebug.h>

#include <khlistview.h>

#include "command_url_widget.h"
#include "menuentry_widget.h"
#include "dcop_widget.h"
#include "keyboard_input_widget.h"
#include "activate_window_widget.h"
#include "waiting_widget.h"
#include "kcmkhotkeys.h"

namespace KHotKeys
{

// Action_list_widget

Action_list_widget::Action_list_widget( TQWidget* parent_P, const char* name_P )
    : Action_list_widget_ui( parent_P, name_P ), selected_item( NULL )
    {
    TQPopupMenu* popup = new TQPopupMenu; // CHECKME looks like setting parent doesn't work
    popup->insertItem( i18n( "Command/URL…" ), TYPE_COMMAND_URL_ACTION );
    popup->insertItem( i18n( "TDE Menu Entry…" ), TYPE_MENUENTRY_ACTION );
    popup->insertItem( i18n( "DCOP Call…" ), TYPE_DCOP_ACTION );
    popup->insertItem( i18n( "Keyboard Input…" ), TYPE_KEYBOARD_INPUT_ACTION );
    popup->insertItem( i18n( "Activate Window…" ), TYPE_ACTIVATE_WINDOW_ACTION );
    popup->insertItem( i18n( "Waiting…" ), TYPE_WAITING_ACTION );
    connect( popup, TQT_SIGNAL( activated( int )), TQT_SLOT( new_selected( int )));
    new_button->setPopup( popup );
    actions_listview->header()->hide();
    actions_listview->addColumn( "" );
    actions_listview->setSorting( -1 );
    actions_listview->setForceSelect( true );
    copy_button->setEnabled( false );
    modify_button->setEnabled( false );
    delete_button->setEnabled( false );
    clear_data();
    connect( actions_listview, TQT_SIGNAL( doubleClicked ( TQListViewItem *, const TQPoint &, int ) ),
             this, TQT_SLOT( modify_pressed() ) );

    // KHotKeys::Module::changed()
    connect(new_button, TQT_SIGNAL(clicked()), module, TQT_SLOT(changed()));
    connect(copy_button, TQT_SIGNAL(clicked()), module, TQT_SLOT(changed()));
    connect(modify_button, TQT_SIGNAL(clicked()), module, TQT_SLOT(changed()));
    connect(delete_button, TQT_SIGNAL(clicked()), module, TQT_SLOT(changed()));
    connect(move_up_button, TQT_SIGNAL(clicked()), module, TQT_SLOT(changed()));
    connect(move_down_button, TQT_SIGNAL(clicked()), module, TQT_SLOT(changed()));
    connect(comment_lineedit, TQT_SIGNAL(textChanged(const TQString&)), module, TQT_SLOT(changed()));
    }

Action_list_widget::~Action_list_widget()
    {
    delete new_button->popup();
    }

void Action_list_widget::clear_data()
    {
    comment_lineedit->clear();
    actions_listview->clear();
    }

void Action_list_widget::set_data( const Action_list* data_P )
    {
    if( data_P == NULL )
        {
        clear_data();
        return;
        }
    comment_lineedit->setText( data_P->comment());
    Action_list_item* after = NULL;
    actions_listview->clear();
    for( Action_list::Iterator it( *data_P );
         *it;
         ++it )
        after = create_listview_item( *it, actions_listview, NULL, after, true );
    }

Action_list* Action_list_widget::get_data( Action_data* data_P ) const
    {
// CHECKME TODO hmm, tady to bude chtit asi i children :(
    Action_list* list = new Action_list( comment_lineedit->text());
    for( TQListViewItem* pos = actions_listview->firstChild();
         pos != NULL;
         pos = pos->nextSibling())
        list->append( static_cast< Action_list_item* >( pos )->action()->copy( data_P ));
    return list;
    }

void Action_list_widget::new_selected( int type_P )
    {
    Action_dialog* dlg = NULL;
    switch( type_P )
        {
        case TYPE_COMMAND_URL_ACTION: // Command_url_action_dialog
            dlg = new Command_url_action_dialog( NULL );
          break;
        case TYPE_MENUENTRY_ACTION: // Menuentry_action_dialog
            dlg = new Menuentry_action_dialog( NULL );
          break;
        case TYPE_DCOP_ACTION: // Dcop_action_dialog
            dlg = new Dcop_action_dialog( NULL );
          break;
        case TYPE_KEYBOARD_INPUT_ACTION: // Keyboard_input_action_dialog
            dlg = new Keyboard_input_action_dialog( NULL );
          break;
        case TYPE_ACTIVATE_WINDOW_ACTION: // Activate_window_action_dialog
            dlg = new Activate_window_action_dialog( NULL );
          break;
        case TYPE_WAITING_ACTION: // Waiting_action_dialog
            dlg = new Waiting_action_dialog( NULL );
          break;
        default:
          assert( false );
        }
    if( dlg != NULL )
        {
        Action* action = dlg->edit_action();
        if( action != NULL )
            actions_listview->setSelected( create_listview_item( action, actions_listview,
                NULL, selected_item, false ), true );
                // CHECKME tady pak jeste spravne vnoreni, kdyz bude skupina
        delete dlg;
        }
    }

void Action_list_widget::copy_pressed()
    {
    if ( !selected_item )
      {
      return;
      }

    actions_listview->setSelected( create_listview_item( selected_item->action(),
    selected_item->parent() ? NULL : actions_listview, selected_item->parent(),
    selected_item, true ), true );
    }

void Action_list_widget::delete_pressed()
    {
    if ( !selected_item )
        {
        return;
        }

    Action_list_item *nextItem = static_cast< Action_list_item* >(selected_item->nextSibling());
    delete selected_item;
    selected_item = NULL;
    if (!nextItem)
        {
        // If the last item of the list was deleted, get the new last item
        nextItem = static_cast< Action_list_item* >(actions_listview->lastItem());
        }
    if (nextItem)
        {
        actions_listview->setSelected(nextItem, true);
        current_changed(nextItem);
        }
    }

void Action_list_widget::modify_pressed()
    {
    if ( !selected_item )
        {
        return;
        }
    edit_listview_item( selected_item );
    }

void Action_list_widget::move_up_pressed()
    {
    if ( !selected_item )
        {
        return;
        }

    Action_list_item *prevItem = nullptr;
    TQListViewItem *currItem = actions_listview->firstChild();
    while (currItem != selected_item)
        {
        prevItem = static_cast< Action_list_item* >(currItem);
        currItem = currItem->nextSibling();
        }
    if (prevItem)
        {
        prevItem->moveItem(selected_item);
        current_changed(selected_item);
        }
    }

void Action_list_widget::move_down_pressed()
    {
    if ( !selected_item )
        {
        return;
        }

    Action_list_item *nextItem = static_cast< Action_list_item* >(selected_item->nextSibling());
    if (nextItem)
        {
        selected_item->moveItem(nextItem);
        current_changed(selected_item);
        }
    }

void Action_list_widget::current_changed( TQListViewItem* item_P )
    {
//    if( item_P == selected_item )
//        return;
    selected_item = static_cast< Action_list_item* >( item_P );
//    actions_listview->setSelected( item_P, true );
    copy_button->setEnabled( item_P != NULL );
    modify_button->setEnabled( item_P != NULL );
    delete_button->setEnabled( item_P != NULL );
    move_up_button->setEnabled(selected_item != actions_listview->firstChild());
    move_down_button->setEnabled(selected_item != actions_listview->lastChild());
    }

Action_list_item* Action_list_widget::create_listview_item( Action* action_P,
    TQListView* parent1_P, TQListViewItem* parent2_P, TQListViewItem* after_P, bool copy_P )
    {
    Action* new_win = copy_P ? action_P->copy( NULL ) : action_P;
// CHECKME uz by nemelo byt treba
/*    if( after_P == NULL )
        {
        if( parent1_P == NULL )
            return new Action_list_item( parent2_P, new_win );
        else
            return new Action_list_item( parent1_P, new_win );
        }
    else*/
        {
        if( parent1_P == NULL )
            return new Action_list_item( parent2_P, after_P, new_win );
        else
            return new Action_list_item( parent1_P, after_P, new_win );
        }
    }

void Action_list_widget::edit_listview_item( Action_list_item* item_P )
    {
    Action_dialog* dlg = NULL;
    if( Command_url_action* action = dynamic_cast< Command_url_action* >( item_P->action()))
        dlg = new Command_url_action_dialog( action );
    else if( Menuentry_action* action = dynamic_cast< Menuentry_action* >( item_P->action()))
        dlg = new Menuentry_action_dialog( action );
    else if( Dcop_action* action = dynamic_cast< Dcop_action* >( item_P->action()))
        dlg = new Dcop_action_dialog( action );
    else if( Keyboard_input_action* action
            = dynamic_cast< Keyboard_input_action* >( item_P->action()))
        dlg = new Keyboard_input_action_dialog( action );
    else if( Activate_window_action* action
        = dynamic_cast< Activate_window_action* >( item_P->action()))
        dlg = new Activate_window_action_dialog( action );
    else if( Waiting_action* action = dynamic_cast< Waiting_action* >( item_P->action()))
        dlg = new Waiting_action_dialog( action );
    else // CHECKME TODO pridat dalsi
        assert( false );
    Action* new_action = dlg->edit_action();
    if( new_action != NULL )
        {
        item_P->set_action( new_action );
        item_P->widthChanged( 0 );
        actions_listview->repaintItem( item_P );
        }
    delete dlg;
    }

// Action_list_item

TQString Action_list_item::text( int column_P ) const
    {
    return column_P == 0 ? action()->description() : TQString::null;
    }

Action_list_item::~Action_list_item()
    {
    delete _action;
    }

// Command_url_action_dialog

Command_url_action_dialog::Command_url_action_dialog( Command_url_action* action_P )
    : KDialogBase( NULL, NULL, true, "", Ok | Cancel ), action( NULL ) // CHECKME caption
    {
    widget = new Command_url_widget( this );
    widget->set_data( action_P );
    setMainWidget( widget );
    }

Action* Command_url_action_dialog::edit_action()
    {
    exec();
    return action;
    }

void Command_url_action_dialog::accept()
    {
    KDialogBase::accept();
    action = widget->get_data( NULL ); // CHECKME NULL ?
    }

// Menuentry_action_dialog

Menuentry_action_dialog::Menuentry_action_dialog( Menuentry_action* action_P )
    : KDialogBase( NULL, NULL, true, "", Ok | Cancel ), action( NULL ) // CHECKME caption
    {
    widget = new Menuentry_widget( this );
    widget->set_data( action_P );
    setMainWidget( widget );
    }

Action* Menuentry_action_dialog::edit_action()
    {
    exec();
    return action;
    }

void Menuentry_action_dialog::accept()
    {
    KDialogBase::accept();
    action = widget->get_data( NULL ); // CHECKME NULL ?
    }

// Dcop_action_dialog

Dcop_action_dialog::Dcop_action_dialog( Dcop_action* action_P )
    : KDialogBase( NULL, NULL, true, "", Ok | Cancel ), action( NULL ) // CHECKME caption
    {
    widget = new Dcop_widget( this );
    widget->set_data( action_P );
    setMainWidget( widget );
    }

Action* Dcop_action_dialog::edit_action()
    {
    exec();
    return action;
    }

void Dcop_action_dialog::accept()
    {
    KDialogBase::accept();
    action = widget->get_data( NULL ); // CHECKME NULL ?
    }

// Keyboard_input_action_dialog

Keyboard_input_action_dialog::Keyboard_input_action_dialog( Keyboard_input_action* action_P )
    : KDialogBase( NULL, NULL, true, "", Ok | Cancel ), action( NULL ) // CHECKME caption
    {
    widget = new Keyboard_input_widget( this );
    widget->set_data( action_P );
    setMainWidget( widget );
    }

Action* Keyboard_input_action_dialog::edit_action()
    {
    exec();
    return action;
    }

void Keyboard_input_action_dialog::accept()
    {
    KDialogBase::accept();
    action = widget->get_data( NULL ); // CHECKME NULL ?
    }

// Activate_window_action_dialog

Activate_window_action_dialog::Activate_window_action_dialog( Activate_window_action* action_P )
    : KDialogBase( NULL, NULL, true, "", Ok | Cancel ), action( NULL ) // CHECKME caption
    {
    widget = new Activate_window_widget( this );
    widget->set_data( action_P ? action_P->window() : NULL );
    setMainWidget( widget );
    }

Action* Activate_window_action_dialog::edit_action()
    {
    exec();
    return action;
    }

void Activate_window_action_dialog::accept()
    {
    KDialogBase::accept();
    action = new Activate_window_action( NULL, widget->get_data()); // CHECKME NULL ?
    }

// Waiting_action_dialog

Waiting_action_dialog::Waiting_action_dialog( Waiting_action* action_P )
    : KDialogBase( NULL, NULL, true, "", Ok | Cancel ), action( NULL )
    {
    widget = new Waiting_widget( this );
    widget->set_data( action_P );
    setMainWidget( widget );
    }

Action* Waiting_action_dialog::edit_action()
    {
    exec();
    return action;
    }

void Waiting_action_dialog::accept()
    {
    KDialogBase::accept();
    action = widget->get_data( NULL );
    }

} // namespace KHotKeys

#include "action_list_widget.moc"
