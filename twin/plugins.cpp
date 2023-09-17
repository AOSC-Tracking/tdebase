/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000    Daniel M. Duley <mosfet@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#include "plugins.h"

#include <tdeglobal.h>
#include <tdelocale.h>
#include <stdlib.h>
#include <tqpixmap.h>

namespace KWinInternal
{

PluginMgr::PluginMgr()
    : KDecorationPlugins( TDEGlobal::config())
    {
    defaultPlugin = (TQPixmap::defaultDepth() > 8) ?
            "twin3_plastik" : "twin3_quartz";
    loadPlugin( "" ); // load the plugin specified in cfg file
    }

void PluginMgr::error( const TQString &error_msg )
    {
    tqWarning( "%s", (i18n("TWin: ") + error_msg +
                    i18n("\nTWin will now exit…")).local8Bit().data() );
    exit(1);
    }

bool PluginMgr::provides( Requirement )
    {
    return false;
    }

} // namespace
