/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _GENERAL_TAB_H_
#define _GENERAL_TAB_H_

#include <general_tab_ui.h>

namespace KHotKeys
{

class Action_data;
class Trigger_list;
class Action_data_group;
class Windowdef_list;
class Action_list;

class General_tab
    : public General_tab_ui
    {
    TQ_OBJECT
    public:
        General_tab( TQWidget* parent_P = NULL, const char* name_P = NULL );
        void set_data( const Action_data* data_P );
        void get_data( TQString& name_O, TQString& comment_O, bool& enabled_O );
    public slots:
        void clear_data();
    signals:
        void action_type_changed( int type_P );
    protected slots:
        void action_name_changed( const TQString& name_P );
    };
    
//***************************************************************************
// Inline
//***************************************************************************

} // namespace KHotKeys

#endif
