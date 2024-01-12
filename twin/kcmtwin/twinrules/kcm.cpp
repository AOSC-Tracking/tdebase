/*
 * Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kcm.h"

#include <tdeglobal.h>
#include <tqlayout.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <dcopclient.h>
#include <tdeaboutdata.h>

#include "ruleslist.h"

extern "C"
    KDE_EXPORT TDECModule *create_twinrules( TQWidget *parent, const char *name )
    {
    //CT there's need for decision: kwm or twin?
    TDEGlobal::locale()->insertCatalogue( "kcmtwinrules" );
    return new KWinInternal::KCMRules( parent, name );
    }

namespace KWinInternal
{

KCMRules::KCMRules( TQWidget *parent, const char *name )
: TDECModule( parent, name )
, config( "twinrulesrc" )
    {
    TQVBoxLayout *layout = new TQVBoxLayout( this );
    widget = new KCMRulesList( this );
    layout->addWidget( widget );
    connect( widget, TQ_SIGNAL( changed( bool )), TQ_SLOT( moduleChanged( bool )));
    TDEAboutData *about = new TDEAboutData(I18N_NOOP( "kcmtwinrules" ),
        I18N_NOOP( "Window-Specific Settings Configuration Module" ),
        0, 0, TDEAboutData::License_GPL, I18N_NOOP( "(c) 2004 KWin and KControl Authors" ));
    about->addAuthor("Lubos Lunak",0,"l.lunak@kde.org");
    setAboutData(about);
    }

void KCMRules::load()
    {
    config.reparseConfiguration();
    widget->load();
    emit TDECModule::changed( false );
    }

void KCMRules::save()
    {
    widget->save();
    emit TDECModule::changed( false );
    // Send signal to twin
    config.sync();
    if( !kapp->dcopClient()->isAttached())
        kapp->dcopClient()->attach();
    kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
    }

void KCMRules::defaults()
    {
    widget->defaults();
    }

TQString KCMRules::quickHelp() const
    {
    return i18n("<h1>Window-specific Settings</h1> Here you can customize window settings specifically only"
        " for some windows."
        " <p>Please note that this configuration will not take effect if you do not use"
        " TWin as your window manager. If you do use a different window manager, please refer to its documentation"
        " for how to customize window behavior.");
    }

void KCMRules::moduleChanged( bool state )
    {
    emit TDECModule::changed( state );
    }

}

// i18n freeze :-/
#if 0
I18N_NOOP("Remember settings separately for every window")
I18N_NOOP("Show internal settings for remembering")
I18N_NOOP("Internal setting for remembering")
#endif


#include "kcm.moc"
