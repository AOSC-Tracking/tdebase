/*****************************************************************

Copyright (c) 2000 Bill Nagel

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

#ifndef __quickbutton_h__
#define __quickbutton_h__

#include <tqbutton.h>
#include <tqpoint.h>
#include <tqstring.h>
#include <tqpixmap.h>
#include <tqcursor.h>

#include <kickertip.h>
#include <kicontheme.h>
#include <kmimetype.h>
#include <kpanelapplet.h>
#include <kservice.h>
#include <kurl.h>

#include "simplebutton.h"

class TQPopupMenu;
class TDEAction;
class TDEToggleAction;

class QuickURL {
public:
    QuickURL(const TQString &u);
    KURL kurl() const {return _kurl;};
    TQString url() const {return _kurl.url();};
    TQString menuId() const  {return _menuId;};
    TQString genericName() const { return m_genericName; }
    TQString name() const { return m_name; }
    KService::Ptr service() const {return _service;};
    void run() const;
    TQPixmap pixmap(mode_t _mode = 0, TDEIcon::Group _group = TDEIcon::Desktop,
                   int _force_size = 0, int _state = 0, TQString * _path = 0L) const;

private:
    KURL _kurl;
    TQString _menuId;
    TQString m_genericName;
    TQString m_name;
    KService::Ptr _service;
};


class QuickButton: public SimpleButton, public KickerTip::Client {
    TQ_OBJECT

public:
    enum { DEFAULT_ICON_DIM = 16 };
    enum { ICON_MARGIN = 1 };
    QuickButton(const TQString &u, TDEAction* configAction,
                TQWidget *parent=0, const char *name=0);
    ~QuickButton();
    TQString url() const;
    TQString menuId() const;
    TQPixmap icon() const{ return _icon;}
    bool sticky() { return m_sticky; }
    void setSticky(bool bSticky);
    void setPopupDirection(KPanelApplet::Direction d);

    void setDragging(bool drag);
    void setEnableDrag(bool enable);
    void setDynamicModeEnabled(bool enabled);
    void flash();

signals:
    void removeApp(QuickButton *);
    void executed(TQString serviceStorageID);
    void stickyToggled(bool isSticky);

protected:
    void mousePressEvent(TQMouseEvent *e);
    void mouseMoveEvent(TQMouseEvent *e);
    void resizeEvent(TQResizeEvent *rsevent);
    void loadIcon();
    void updateKickerTip(KickerTip::Data &data);

protected slots:
    void slotIconChanged(int);
    void launch();
    void removeApp();
    void slotFlash();
    void slotStickyToggled(bool isSticky);
    void toggle(bool);

private:
    int m_flashCounter;
    QuickURL *_qurl;
    TQPoint _dragPos;
    TQPopupMenu *_popup;
    TQPixmap _icon, _iconh;
    TQCursor _oldCursor;
    bool _highlight, _changeCursorOverItem, _dragEnabled;
    int _iconDim;
    bool m_sticky;
    TDEToggleAction *m_stickyAction;
    int m_stickyId;
    KPanelApplet::Direction m_popupDirection;
};

#endif

