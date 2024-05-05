/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <dcopref.h>
#include <tqfile.h>

#include <X11/Xlib.h>

#ifdef HAVE_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
#endif

#include "mouse.h"

extern "C"
{
  TDE_EXPORT TDECModule *create_mouse(TQWidget *parent, const char *)
  {
    return new MouseConfig(parent, "kcminput");
  }

  TDE_EXPORT void init_mouse()
  {
    TDEConfig *config = new TDEConfig("kcminputrc", true, false); // Read-only, no globals
    MouseSettings settings;
    settings.load(config);
    settings.apply(true); // force

#ifdef HAVE_XCURSOR
    config->setGroup("Mouse");
    TQCString theme = TQFile::encodeName(config->readEntry("cursorTheme", TQString()));
    TQCString size = config->readEntry("cursorSize", TQString()).local8Bit();

    // Note: If you update this code, update kapplymousetheme as well.

    // use a default value for theme only if it's not configured at all, not even in X resources
    if( theme.isEmpty()
        && TQCString( XGetDefault( tqt_xdisplay(), "Xcursor", "theme" )).isEmpty()
        && TQCString( XcursorGetTheme( tqt_xdisplay())).isEmpty())
    {
        theme = "default";
    }

     // Apply the KDE cursor theme to ourselves
    if( !theme.isEmpty())
        XcursorSetTheme(tqt_xdisplay(), theme.data());

    if (!size.isEmpty())
    	XcursorSetDefaultSize(tqt_xdisplay(), size.toUInt());

    // Load the default cursor from the theme and apply it to the root window.
    Cursor handle = XcursorLibraryLoadCursor(tqt_xdisplay(), "left_ptr");
    XDefineCursor(tqt_xdisplay(), tqt_xrootwin(), handle);
    XFreeCursor(tqt_xdisplay(), handle); // Don't leak the cursor

    // Tell tdelauncher to set the XCURSOR_THEME and XCURSOR_SIZE environment
    // variables when launching applications.
    DCOPRef tdelauncher("tdelauncher");
    if( !theme.isEmpty())
        tdelauncher.send("setLaunchEnv", TQCString("XCURSOR_THEME"), theme);
    if( !size.isEmpty())
        tdelauncher.send("setLaunchEnv", TQCString("XCURSOR_SIZE"), size);
#endif

    delete config;
  }
}


