/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _GENERAL_SETTINGS_TAB_H_
#define _GENERAL_SETTINGS_TAB_H_

#include <general_settings_tab_ui.h>

namespace KHotKeys
{

class General_settings_tab
    : public General_settings_tab_ui
    {
    TQ_OBJECT
    public:
        General_settings_tab( TQWidget* parent = NULL, const char* name = NULL );
    public slots:
        void clear_data();
    protected slots:
        virtual void import_clicked();
    };

//***************************************************************************
// Inline
//***************************************************************************

} // namespace KHotKeys

#endif
