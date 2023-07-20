/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

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

#ifndef __addbutton_mnu_h__
#define __addbutton_mnu_h__

#include "service_mnu.h"

class ContainerArea;

class PanelAddButtonMenu : public PanelServiceMenu
{
    TQ_OBJECT

public:
    PanelAddButtonMenu(ContainerArea* cArea, const TQString & label, const TQString & relPath,
		       TQWidget * parent  = 0, const char * name = 0,const TQString& _inlineHeader= TQString::null);
    PanelAddButtonMenu(ContainerArea* cArea, TQWidget * parent = 0, const char * name = 0, const TQString& _inlineHeader= TQString::null);

protected slots:
    virtual void slotExec(int id);
    virtual void addNonKDEApp();

protected:
    virtual PanelServiceMenu * newSubMenu(const TQString & label, const TQString & relPath,
					  TQWidget * parent, const char * name, const TQString & _inlineHeader=TQString::null);
private:
    ContainerArea *containerArea;
};

#endif
