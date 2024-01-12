/****************************************************************************

 KHotKeys

 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.

****************************************************************************/

#define _WAITING_WIDGET_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "waiting_widget.h"

#include <knuminput.h>

#include <kdebug.h>

#include <actions.h>
#include <action_data.h>

#include "windowdef_list_widget.h"
#include "kcmkhotkeys.h"

namespace KHotKeys
{

Waiting_widget::Waiting_widget( TQWidget* parent_P, const char* name_P )
    : Waiting_widget_ui( parent_P, name_P )
    {
    // KHotKeys::Module::changed()
    connect(waiting_spinbox, TQ_SIGNAL(valueChanged(int)),
        module, TQ_SLOT(changed()));
    }

void Waiting_widget::set_data( const Waiting_action* data_P )
    {
    if( data_P == NULL )
        {
        return;
        }
    waiting_spinbox->setValue(data_P->_waiting_time);
    }

Waiting_action* Waiting_widget::get_data( Action_data* data_P ) const
    {
    return new Waiting_action( data_P, waiting_spinbox->value());
    }

} // namespace KHotKeys

#include "waiting_widget.moc"
