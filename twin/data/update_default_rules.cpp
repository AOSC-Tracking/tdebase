/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2005 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

// read addtional window rules and add them to twinrulesrc

#include <dcopclient.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeinstance.h>
#include <kstandarddirs.h>

int main( int argc, char* argv[] )
    {
    if( argc != 2 )
        return 1;
    TDEInstance inst( "twin_update_default_rules" );
    TQString file = locate( "data", TQString( "twin/default_rules/" ) + argv[ 1 ] );
    if( file.isEmpty())
        {
        kdWarning() << "File " << argv[ 1 ] << " not found!" << endl;
        return 1;
        }
    TDEConfig src_cfg( file );
    TDEConfig dest_cfg( "twinrulesrc" );
    src_cfg.setGroup( "General" );
    dest_cfg.setGroup( "General" );
    int count = src_cfg.readNumEntry( "count", 0 );
    int pos = dest_cfg.readNumEntry( "count", 0 );
    for( int group = 1;
         group <= count;
         ++group )
        {
        TQMap< TQString, TQString > entries = src_cfg.entryMap( TQString::number( group ));
        ++pos;
        dest_cfg.deleteGroup( TQString::number( pos ));
        dest_cfg.setGroup( TQString::number( pos ));
        for( TQMap< TQString, TQString >::ConstIterator it = entries.begin();
             it != entries.end();
             ++it )
            dest_cfg.writeEntry( it.key(), *it );
        }
    dest_cfg.setGroup( "General" );
    dest_cfg.writeEntry( "count", pos );
    src_cfg.sync();
    dest_cfg.sync();
    DCOPClient client;
    client.attach();
    client.send("twin*", "", "reconfigure()", TQString(""));
    }
