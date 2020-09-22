/*
 * Style Preview Widget
 * Copyright (C) 2002 Karol Szwed <gallium@kde.org>
 * Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>
 *
 * Portions Copyright (C) 2000 TrollTech AS.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 ***************************************************************************
 ** ui.h extension file, included from the uic-generated form implementation.
 **
 ** If you wish to add, delete or rename slots use Qt Designer which will
 ** update this file, preserving your code. Create an init() slot in place of
 ** a constructor, and a destroy() slot in place of a destructor.
 *****************************************************************************/

#include <tqobjectlist.h>

#ifdef KeyPress
#undef KeyPress
#endif

#ifdef KeyRelease
#undef KeyRelease
#endif

void StylePreview::init()
{
    // Ensure that the user can't toy with the child widgets.
    // Method borrowed from Qt's qtconfig.
    TQObjectList* l = queryList(TQWIDGET_OBJECT_NAME_STRING);
    TQObjectListIt it(*l);
    TQObject* obj;
    while ((obj = it.current()) != 0)
    {
        ++it;
        obj->installEventFilter(this);
        ((TQWidget*)obj)->setFocusPolicy(TQ_NoFocus);
    }
    delete l;
}

bool StylePreview::eventFilter( TQObject* /* obj */, TQEvent* ev )
{
    switch( ev->type() )
    {
        case TQEvent::MouseButtonPress:
        case TQEvent::MouseButtonRelease:
        case TQEvent::MouseButtonDblClick:
        case TQEvent::MouseMove:
        case TQEvent::KeyPress:
        case TQEvent::KeyRelease:
        case TQEvent::Enter:
        case TQEvent::Leave:
        case TQEvent::Wheel:
        case TQEvent::ContextMenu:
            return TRUE; // ignore
        default:
            break;
    }
    return FALSE;
}
