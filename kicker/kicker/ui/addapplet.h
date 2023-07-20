/*****************************************************************

Copyright (c) 2005 Marc Cramdal
Copyright (c) 2005 Aaron Seigo <aseigo@kde.org>

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

#ifndef __addapplet_h__
#define __addapplet_h__

#include <tqstringlist.h>
#include <tqpixmap.h>
#include <tqvaluelist.h>

#include <tdelocale.h>
#include <kdialogbase.h>

#include "appletinfo.h"

class ContainerArea;
class AppletView;
class AppletWidget;
class TQTimer;

class AddAppletDialog : public KDialogBase
{
    TQ_OBJECT

    public:
        AddAppletDialog(ContainerArea* cArea, TQWidget* parent, const char* name);
        void updateInsertionPoint();

    protected:
        void closeEvent(TQCloseEvent*);
        bool eventFilter(TQObject *o, TQEvent *e);

    private slots:
        void populateApplets();
        void addCurrentApplet();
        void addApplet(AppletWidget* applet);
        void delayedSearch();
        void search();
        void filter(int i);
        void selectApplet(AppletWidget* applet);
        void resizeAppletView();

    private:
        bool appletMatchesSearch(const AppletWidget* w, const TQString& s);

        AppletView *m_mainWidget;
        TQWidget *m_appletBox;

        AppletInfo::List m_applets;

        TQValueList<AppletWidget*> m_appletWidgetList;
        AppletWidget* m_selectedApplet;

        ContainerArea* m_containerArea;
        AppletInfo::AppletType m_selectedType;
        TQPoint m_insertionPoint;
        bool m_closing;
        TQTimer *m_searchDelay;
};

#endif
