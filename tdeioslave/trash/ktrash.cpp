/* This file is part of the TDE project
   Copyright (C) 2004 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <tdeapplication.h>
#include <tdeio/netaccess.h>
#include <tdeio/job.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <tdeversion.h>
#include <kdirnotify_stub.h>
#include <kdebug.h>

static TDECmdLineOptions options[] =
{
    { "empty", I18N_NOOP( "Empty the contents of the trash" ), 0 },
    //{ "migrate", I18N_NOOP( "Migrate contents of old trash" ), 0 },
    { "restore <file>", I18N_NOOP( "Restore a trashed file to its original location" ), 0 },
    // This hack is for the servicemenu on trash.desktop which uses Exec=ktrash -empty. %f is implied...
    { "+[ignored]", I18N_NOOP( "Ignored" ), 0 },
    TDECmdLineLastOption
};

int main(int argc, char *argv[])
{
    TDEApplication::disableAutoDcopRegistration();
    TDECmdLineArgs::init( argc, argv, "ktrash",
                        I18N_NOOP( "ktrash" ),
                        I18N_NOOP( "Helper program to handle the TDE trash can\n"
				   "Note: to move files to the trash, do not use ktrash, but \"kfmclient move 'url' trash:/\"" ),
                        TDE_VERSION_STRING );
    TDECmdLineArgs::addCmdLineOptions( options );
    TDEApplication app;

    TDECmdLineArgs* args = TDECmdLineArgs::parsedArgs();
    if ( args->isSet( "empty" ) ) {
        // We use a tdeio job instead of linking to TrashImpl, for a smaller binary
        // (and the possibility of a central service at some point)
        TQByteArray packedArgs;
        TQDataStream stream( packedArgs, IO_WriteOnly );
        stream << (int)1;
        TDEIO::Job* job = TDEIO::special( "trash:/", packedArgs );
        (void)TDEIO::NetAccess::synchronousRun( job, 0 );

        // Update konq windows opened on trash:/
        KDirNotify_stub allDirNotify("*", "KDirNotify*");
        allDirNotify.FilesAdded( "trash:/" ); // yeah, files were removed, but we don't know which ones...
        return 0;
    }

#if 0
    // This is only for testing. KDesktop handles it automatically.
    if ( args->isSet( "migrate" ) ) {
        TQByteArray packedArgs;
        TQDataStream stream( packedArgs, IO_WriteOnly );
        stream << (int)2;
        TDEIO::Job* job = TDEIO::special( "trash:/", packedArgs );
        (void)TDEIO::NetAccess::synchronousRun( job, 0 );
        return 0;
    }
#endif

    TQCString restoreArg = args->getOption( "restore" );
    if ( !restoreArg.isEmpty() ) {

        if (restoreArg.find("system:/trash")==0) {
            restoreArg.remove(0, 13);
            restoreArg.prepend("trash:");
        }

        KURL trashURL( restoreArg );
        if ( !trashURL.isValid() || trashURL.protocol() != "trash" ) {
            kdError() << "Invalid URL for restoring a trashed file:" << trashURL << endl;
            return 1;
        }

        TQByteArray packedArgs;
        TQDataStream stream( packedArgs, IO_WriteOnly );
        stream << (int)3 << trashURL;
        TDEIO::Job* job = TDEIO::special( trashURL, packedArgs );
        bool ok = TDEIO::NetAccess::synchronousRun( job, 0 );
        if ( !ok )
            kdError() << TDEIO::NetAccess::lastErrorString() << endl;
        return 0;
    }

    return 0;
}
