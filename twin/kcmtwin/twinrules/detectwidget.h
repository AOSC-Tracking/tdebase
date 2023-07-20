/*
 * Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef __DETECTWIDGET_H__
#define __DETECTWIDGET_H__

#include "detectwidgetbase.h"

#include <kdialogbase.h>
#include <twin.h>

#include "../../rules.h"

namespace KWinInternal
{

class DetectWidget
    : public DetectWidgetBase
    {
    TQ_OBJECT
    public:
        DetectWidget( TQWidget* parent = NULL, const char* name = NULL );
    };

class DetectDialog
    : public KDialogBase
    {
    TQ_OBJECT
    public:
        DetectDialog( TQWidget* parent = NULL, const char* name = NULL );
        void detect( WId window );
        TQCString selectedClass() const;
        bool selectedWholeClass() const;
        TQCString selectedRole() const;
        bool selectedWholeApp() const;
        NET::WindowType selectedType() const;
        TQString selectedTitle() const;
        Rules::StringMatch titleMatch() const;
        TQCString selectedMachine() const;
        const KWin::WindowInfo& windowInfo() const;
    signals:
        void detectionDone( bool );
    protected:
        virtual bool eventFilter( TQObject* o, TQEvent* e );
    private:
        void selectWindow();
        void readWindow( WId window );
        void executeDialog();
        WId findWindow();
        TQCString wmclass_class;
        TQCString wmclass_name;
        TQCString role;
        NET::WindowType type;
        TQString title;
        TQCString extrarole;
        TQCString machine;
        DetectWidget* widget;
        TQDialog* grabber;
        KWin::WindowInfo info;
    };

inline
const KWin::WindowInfo& DetectDialog::windowInfo() const
    {
    return info;
    }

} // namespace

#endif
