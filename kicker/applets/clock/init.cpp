/************************************************************

Copyright (c) 1996-2002 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <cstdlib>
#include <ctime>
#include <time.h>

#include <tqcheckbox.h>
#include <tqcursor.h>
#include <tqgroupbox.h>
#include <tqimage.h>
#include <tqpainter.h>
#include <tqtimer.h>
#include <tqtooltip.h>
#include <tqclipboard.h>
#include <tqtabwidget.h>
#include <tqwidgetstack.h>
#include <tqcombobox.h>

#include <tdeapplication.h>
#include <kdebug.h>
#include <kcolorbutton.h>
#include <kiconloader.h>
#include <tdestandarddirs.h>
#include <tdeapplication.h>
#include <tdeprocess.h>
#include <tdelocale.h>
#include <tdepopupmenu.h>
#include <kstringhandler.h>
#include <tdefiledialog.h>
#include <tdefontrequester.h>
#include <tdeglobalsettings.h>
#include <tdeconfigdialogmanager.h>
#include <kcalendarsystem.h>
#include <kicontheme.h>
#include <kiconloader.h>

#include <global.h> // libkickermain

#include "kickerSettings.h"
#include "clock.h"
#include "datepicker.h"
#include "zone.h"
#include "analog.h"
#include "digital.h"
#include "fuzzy.h"
#include "prefs.h"

extern "C"
{
    TDE_EXPORT KPanelApplet* init(TQWidget *parent, const TQString& configFile)
    {
        TDEGlobal::locale()->insertCatalogue("clockapplet");
        TDEGlobal::locale()->insertCatalogue("timezones"); // For time zone translations
        return new ClockApplet(configFile, KPanelApplet::Normal,
                               KPanelApplet::Preferences, parent, "clockapplet");
    }
}
