/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#define _GENERAL_SETTINGS_TAB_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "general_settings_tab.h"

#include <tqcheckbox.h>

#include "kcmkhotkeys.h"

namespace KHotKeys
{

General_settings_tab::General_settings_tab( TQWidget* parent_P, const char* name_P )
    : General_settings_tab_ui( parent_P, name_P )
    {
    // KHotKeys::Module::changed()
    connect( disable_daemon_checkbox, TQ_SIGNAL( clicked()),
        module, TQ_SLOT( changed()));
    }

void General_settings_tab::import_clicked()
    {
    module->import();
    }

void General_settings_tab::write_data() const
    {
    module->set_daemon_disabled( disable_daemon_checkbox->isChecked());
    }

void General_settings_tab::read_data()
    {
    disable_daemon_checkbox->setChecked( module->daemon_disabled());
    }

void General_settings_tab::clear_data()
    {
    // "global" tab, not action specific, do nothing
    }

} // namespace KHotKeys

#include "general_settings_tab.moc"
