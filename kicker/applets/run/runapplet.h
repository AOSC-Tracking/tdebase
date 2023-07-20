/*****************************************************************

Copyright (c) 2000 Matthias Elter

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

#ifndef __runapplet_h__
#define __runapplet_h__

#include <tqstring.h>
#include <kpanelapplet.h>

class TQLabel;
class TQHBox;
class TQPushButton;
class KHistoryCombo;
class KURIFilterData;

class RunApplet : public KPanelApplet
{
    TQ_OBJECT

public:
    RunApplet(const TQString& configFile, Type t = Stretch, int actions = 0,
	      TQWidget *parent = 0, const char *name = 0);
    virtual ~RunApplet();

    int widthForHeight(int height) const;
    int heightForWidth(int width) const;

protected:
    void resizeEvent(TQResizeEvent*);
    void positionChange(KPanelApplet::Position);

protected slots:
    void run_command(const TQString&);
    void popup_combo();
    void setButtonText();

private:
    KHistoryCombo  *_input;
    KURIFilterData *_filterData;
    TQLabel         *_label;
    TQPushButton    *_btn;
    TQHBox          *_hbox;
};

#endif
