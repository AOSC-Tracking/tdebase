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

#include "datepicker.h"
#include "prefs.h"

#include <kdatepicker.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <twin.h>
#include <netwm.h>

DatePicker::DatePicker(TQWidget *parent, const TQDate& date, Prefs* _prefs)
    : TQVBox( parent, 0,
             _prefs->calendarFullWindow()
                ? (WFlags)(WType_TopLevel | WDestructiveClose |
                   WStyle_StaysOnTop)
                : (WFlags)(WStyle_Customize | WStyle_NoBorder |
                   WType_TopLevel | WDestructiveClose |
                   WStyle_StaysOnTop) ),
      prefs(_prefs)
{
    if (prefs->calendarFullWindow())
    {
        KWin::setType(winId(), NET::Utility);
        setFrameStyle(TQFrame::NoFrame);
    }
    else
    {
        setFrameStyle(TQFrame::PopupPanel | TQFrame::Raised);
    }

    KWin::setOnAllDesktops(handle(), true);
    picker = new KDatePicker(this, date);
    picker->setCloseButton(!_prefs->calendarFullWindow());

    /* name and icon for kicker's taskbar */
    setCaption(i18n("Calendar"));
    setIcon(SmallIcon("date"));
}

void DatePicker::closeEvent(TQCloseEvent* e)
{
    prefs->setCalendarSize(size());
    TQVBox::closeEvent(e);
}

void DatePicker::keyPressEvent(TQKeyEvent *e)
{
    TQVBox::keyPressEvent(e);

    if (e->key() == TQt::Key_Escape)
    {
        close();
    }
}

bool DatePicker::setDate(const TQDate& date)
{
    return picker->setDate(date);
}

TQDate DatePicker::date()
{
    return picker->date();
}

