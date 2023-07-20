/****************************************************************************

 KHotKeys

 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.

****************************************************************************/

#ifndef _WAITING_WIDGET_H_
#define _WAITING_WIDGET_H_

#include <waiting_widget_ui.h>

namespace KHotKeys
{

class Waiting_action;
class Action_data;

class Waiting_widget
    : public Waiting_widget_ui
    {
    TQ_OBJECT
    public:
        Waiting_widget( TQWidget* parent_P = NULL, const char* name_P = NULL );
        void set_data( const Waiting_action* data_P );
        Waiting_action* get_data( Action_data* data_P ) const;
    };

typedef Waiting_widget Waiting_tab;

} // namespace KHotKeys

#endif
