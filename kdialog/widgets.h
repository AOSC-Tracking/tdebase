//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002 David Faure <faure@kde.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the7 implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#ifndef _WIDGETS_H_
#define _WIDGETS_H_

#include <tqwidget.h>
#include <tqstring.h>

namespace Widgets
{
    bool inputBox(TQWidget *parent, const TQString& title, const TQString& text, const TQString& init, TQString &result);
    bool passwordBox(TQWidget *parent, const TQString& title, const TQString& text, TQString &result);
    int textBox(TQWidget *parent, int width, int height, const TQString& title, const TQString& file);
    int textInputBox(TQWidget *parent, int width, int height, const TQString& title, const TQStringList& args, TQCString &result);
    bool listBox(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, const TQString &defaultEntry, TQString &result);
    bool checkList(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, bool separateOutput, TQStringList &result);
    bool radioBox(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, TQString &result);
    bool comboBox(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, const TQString& defaultEntry, TQString &result);
    bool progressBar(TQWidget *parent, const TQString& title, const TQString& text, int totalSteps);

    void handleXGeometry(TQWidget * dlg);

}

#endif
