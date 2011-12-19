 /*
 *  This file is part of the Trinity Desktop Environment
 *
 *  Original file taken from the OpenSUSE kdebase builds
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "tqtkdeintegration_x11_p.h"

#include <tqcolordialog.h>
#include <tqfiledialog.h>
#include <tqfontdialog.h>
#include <tqlibrary.h>
#include <tqregexp.h>
#include <tqmessagebox.h>
#include <tqapplication.h>
#include <stdlib.h>

bool TQKDEIntegration::inited = false;
bool TQKDEIntegration::enable = false;

bool TQKDEIntegration::enabled()
    {
    if( !inited )
        initLibrary();
    return enable;
    }

static TQCString findLibrary()
    {
    if( getenv( "KDE_FULL_SESSION" ) == NULL )
        return "";
    if( getenv( "KDE_FULL_SESSION" )[ 0 ] != 't' && getenv( "KDE_FULL_SESSION" )[ 0 ] != '1' )
        return "";
    if( getenv( "TQT_NO_KDE_INTEGRATION" ) == NULL
        || getenv( "TQT_NO_KDE_INTEGRATION" )[ 0 ] == '0' )
        {
        return TQCString( TQTKDELIBDIR ) + "/libqtkde";
        }
    return "";
    }

static long parentToWinId( const TQWidget* w )
    {
    if( w != NULL )
        return w->topLevelWidget()->winId();
    // try to find some usable parent
    if( tqApp->activeWindow() && w != tqApp->activeWindow())
        return tqApp->activeWindow()->winId();
    if( tqApp->mainWidget() && w != tqApp->mainWidget())
        return tqApp->mainWidget()->winId();
    return 0;
    }

inline static TQFont fontPtrToFontRef( const TQFont* f )
    {
    return f != NULL ? *f : TQFont();
    }

// ---
