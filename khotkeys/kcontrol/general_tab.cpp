/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#define _GENERAL_TAB_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "general_tab.h"

#include <tqlineedit.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>

#include <tdelocale.h>
#include <kdebug.h>
#include <ktextedit.h>

#include <actions.h>
#include <action_data.h>

#include "tab_widget.h"
#include "kcmkhotkeys.h"

namespace KHotKeys
{

General_tab::General_tab( TQWidget* parent_P, const char* name_P )
    : General_tab_ui( parent_P, name_P )
    {
    for( Tab_widget::action_type_t i = Tab_widget::TYPE_FIRST;
         i < Tab_widget::TYPE_END;
         ++i )
        switch( i )
            {
            case Tab_widget::TYPE_GENERIC:
                action_type_combo->insertItem( i18n( "Generic" ), i );
              break;
            case Tab_widget::TYPE_COMMAND_URL_SHORTCUT:
                action_type_combo->insertItem(
                    i18n( "Keyboard Shortcut -> Command/URL (simple)" ), i );
              break;
            case Tab_widget::TYPE_MENUENTRY_SHORTCUT:
                action_type_combo->insertItem(
                    i18n( "TDE Menu Entry (simple)" ), i );
              break;
            case Tab_widget::TYPE_DCOP_SHORTCUT:
                action_type_combo->insertItem(
                    i18n( "Keyboard Shortcut -> DCOP Call (simple)" ), i );
              break;
            case Tab_widget::TYPE_KEYBOARD_INPUT_SHORTCUT:
                action_type_combo->insertItem(
                    i18n( "Keyboard Shortcut -> Keyboard Input (simple)" ), i );
              break;
            case Tab_widget::TYPE_KEYBOARD_INPUT_GESTURE:
                action_type_combo->insertItem(
                    i18n( "Gesture -> Keyboard Input (simple)" ), i );
              break;
            case Tab_widget::TYPE_ACTIVATE_WINDOW_SHORTCUT:
                action_type_combo->insertItem(
                    i18n( "Keyboard Shortcut -> Activate Window (simple)" ), i );
              break;
            case Tab_widget::TYPE_END:
              assert( false );
            }
    clear_data();
    // KHotKeys::Module::changed()
    connect( action_name_lineedit, TQ_SIGNAL( textChanged( const TQString& )),
        module, TQ_SLOT( changed()));
    connect( disable_checkbox, TQ_SIGNAL( clicked()),
        module, TQ_SLOT( changed()));
    connect( comment_multilineedit, TQ_SIGNAL( textChanged()),
        module, TQ_SLOT( changed()));
    connect( action_type_combo, TQ_SIGNAL( activated( int )),
        module, TQ_SLOT( changed()));
    }
    
void General_tab::clear_data()
    {
    disconnect( action_name_lineedit, TQ_SIGNAL( textChanged( const TQString& )),
        this, TQ_SLOT( action_name_changed( const TQString& )));
    disconnect( action_type_combo, TQ_SIGNAL( activated( int )),
        this, TQ_SIGNAL( action_type_changed( int ))); // CHECKME neodpoji to sloty od nej ?
    action_name_lineedit->clear();
    disable_checkbox->setChecked( false );
    disable_checkbox->setText( i18n( "&Disable" ));
    comment_multilineedit->clear();
    action_type_combo->setCurrentItem( Tab_widget::TYPE_GENERIC );
//    module->set_action_type( data_P->type()); CHECKME tohle asi tady ne
    }

void General_tab::set_data( const Action_data* data_P )
    {
    if( data_P == NULL )
        {
        clear_data();
        return;
        }
    action_name_lineedit->setText( data_P->name());
    disable_checkbox->setChecked( !data_P->enabled( true ));
    if( !data_P->parent()->enabled( false ))
        disable_checkbox->setText( i18n( "&Disable (group is disabled)" ));
    else
        disable_checkbox->setText( i18n( "&Disable" ));
    comment_multilineedit->setText( data_P->comment());
    action_type_combo->setCurrentItem( Tab_widget::type( data_P ));
//    module->set_action_type( data_P->type());
    connect( action_name_lineedit, TQ_SIGNAL( textChanged( const TQString& )),
        TQ_SLOT( action_name_changed( const TQString& )));
    connect( action_type_combo, TQ_SIGNAL( activated( int )),
        TQ_SIGNAL( action_type_changed( int )));
    }

void General_tab::get_data( TQString& name_O, TQString& comment_O, bool& enabled_O )
    {
    name_O = action_name_lineedit->text();
    comment_O = comment_multilineedit->text();
    enabled_O = !disable_checkbox->isChecked();
    }
    
void General_tab::action_name_changed( const TQString& name_P )
    {
    module->action_name_changed( name_P );
    }
    
} // namespace KHotKeys

#include "general_tab.moc"
