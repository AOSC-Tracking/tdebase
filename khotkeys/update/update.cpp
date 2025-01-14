/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#define _UPDATE_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <kstandarddirs.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <dcopclient.h>

#include <settings.h>

using namespace KHotKeys;

static const TDECmdLineOptions options[] =
    {
    // no need for I18N_NOOP(), this is not supposed to be used directly
        { "id <id>", "Id of the script to add to khotkeysrc.", 0 },
        TDECmdLineLastOption
    };

int main( int argc, char* argv[] )
    {
    TDECmdLineArgs::init( argc, argv, "khotkeys_update", "KHotKeys Update",
	"KHotKeys update utility", "1.0" );
    TDECmdLineArgs::addCmdLineOptions( options );
    TDEApplication app( false, true ); // X11 connection is necessary for KKey* stuff :-/
    TDECmdLineArgs* args = TDECmdLineArgs::parsedArgs();
    TQCString id = args->getOption( "id" );
    TQString file = locate( "data", "khotkeys/" + id + ".khotkeys" );
    if( file.isEmpty())
        {
        kdWarning() << "File " << id << " not found!" << endl;
        return 1;
        }
    init_global_data( false, &app );
    Settings settings;
    settings.read_settings( true );
    TDEConfig cfg( file, true );
    if( !settings.import( cfg, false ))
        {
        kdWarning() << "Import of " << id << " failed!" << endl;
        return 2;
        }
    settings.write_settings();
    TQByteArray data;
    tdeApp->dcopClient()->send( "khotkeys*", "khotkeys", "reread_configuration()", data );
    return 0;
    }
