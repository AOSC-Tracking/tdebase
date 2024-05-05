/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#define _INIT_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dcopref.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <stdlib.h>

extern "C"
{
    TDE_EXPORT void init_khotkeys()
    {
    TDEConfig cfg( "khotkeysrc", true );
    cfg.setGroup( "Main" );
    if( !cfg.readBoolEntry( "Autostart", false ))
        return;
    // Non-xinerama multhead support in KDE is just a hack
    // involving forking apps per-screen. Don't bother with
    // kded modules in such case.
    TQCString multiHead = getenv("TDE_MULTIHEAD");
    if (multiHead.lower() == "true")
        kapp->tdeinitExec( "khotkeys" );
    else
        {
        DCOPRef ref( "kded", "kded" );
        if( !ref.call( "loadModule", TQCString( "khotkeys" )))
            {
            kdWarning( 1217 ) << "Loading of khotkeys module failed." << endl;
            kapp->tdeinitExec( "khotkeys" );
            }
        }
    }
}
