/* This file is part of the KDE project
   Copyright (C) 2007 Dennis Kasprzyk <onestone@opencompositing.org>

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

#ifndef __kdesktopapp_h__
#define __kdesktopapp_h__

#include <config.h>
#include <kuniqueapplication.h>

#if defined(Q_WS_X11) && defined(HAVE_XRENDER) && TQT_VERSION >= 0x030300
#define COMPOSITE
#endif

#ifdef COMPOSITE
# include <X11/Xlib.h>
# include <X11/Xatom.h>
# include <fixx11h.h>
#endif

class KDesktopApp : public KUniqueApplication
{
    TQ_OBJECT
    public:
        KDesktopApp();
        KDesktopApp(Display * dpy, TQt::HANDLE visual = 0,
                    TQt::HANDLE colormap = 0);

#ifdef COMPOSITE
        bool x11EventFilter (XEvent *);

        bool cmBackground ()
        {
            return m_bgSupported;
        }
#endif

    signals:
        void cmBackgroundChanged(bool supported);

#ifdef COMPOSITE
    private:
        void initCmBackground();

    private:

        Atom m_cmBackground;
        Bool m_bgSupported;
#endif
};

#endif
