/*****************************************************************

Copyright (c) 2001 the kicker authors. See file AUTHORS.

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

#ifndef __ServiceButton_h__
#define __ServiceButton_h__

#include "panelbutton.h"

#include <kservice.h>

class ServiceButton : public PanelButton
{
    TQ_OBJECT

public:
    ServiceButton( const TQString& desktopFile, TQWidget* parent );
    ServiceButton( const KService::Ptr& service, TQWidget* parent );
    ServiceButton( const TDEConfigGroup& config, TQWidget* parent );

    ~ServiceButton();

    virtual void saveConfig(TDEConfigGroup& config) const;
    virtual void properties();

protected slots:
    void slotUpdate();
    void slotSaveAs(const KURL&, KURL&);
    void slotExec();
    void performExec();

protected:
    void initialize();
    void loadServiceFromId(const TQString &id);
    void readDesktopFile();
    virtual TQString tileName() { return "URL"; }
    virtual void startDrag();
    virtual void dropEvent(TQDropEvent *);
    virtual void dragEnterEvent(TQDragEnterEvent *);
    TQString defaultIcon() const { return "application-x-executable"; };
    bool checkForBackingFile();

    KService::Ptr  _service;
    TQString        _id;
};

#endif
