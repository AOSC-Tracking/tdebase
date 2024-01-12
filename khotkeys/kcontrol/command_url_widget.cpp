/****************************************************************************

 KHotKeys

 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.

****************************************************************************/

#define _COMMAND_URL_WIDGET_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "command_url_widget.h"

#include <tqpushbutton.h>
#include <tqlineedit.h>
#include <klineedit.h>
#include <tqcheckbox.h>
#include <tqgroupbox.h>
#include <kurlrequester.h>

#include <actions.h>
#include <action_data.h>
#include <windowdef_list_widget.h>

#include "kcmkhotkeys.h"

namespace KHotKeys
{

Command_url_widget::Command_url_widget( TQWidget* parent_P, const char* name_P )
    : Command_url_widget_ui( parent_P, name_P )
    {
    clear_data();
    // KHotKeys::Module::changed()
    connect( command_url_lineedit, TQ_SIGNAL( textChanged( const TQString& )),
        module, TQ_SLOT( changed()));
    }

void Command_url_widget::clear_data()
    {
    command_url_lineedit->lineEdit()->clear();
    }

void Command_url_widget::set_data( const Command_url_action* data_P )
    {
    if( data_P == NULL )
        {
        clear_data();
        return;
        }
    command_url_lineedit->lineEdit()->setText( data_P->command_url());
    }

Command_url_action* Command_url_widget::get_data( Action_data* data_P ) const
    {
    return new Command_url_action( data_P, command_url_lineedit->lineEdit()->text());
    }

void Command_url_widget::browse_pressed()
    { // CHECKME TODO
    }

} // namespace KHotKeys

#include "command_url_widget.moc"
