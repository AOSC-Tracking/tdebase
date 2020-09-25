/*

    Copyright (C) 2006 William Jon McCann <mccann@jhu.edu>
    Copyright (C) 2007 Kevin Kofler <Kevin@tigcc.ticalc.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */


#ifndef __CONSOLE_KIT_H
#define __CONSOLE_KIT_H

#include <pwd.h>

struct display;

char *      open_ck_session       (struct passwd *pwent,
                                   struct display *display);
void        close_ck_session      (const char    *cookie);
void        unlock_ck_session     (const char    *user,
                                   const char    *x11_display);

#endif /* __CONSOLE_KIT_H */
