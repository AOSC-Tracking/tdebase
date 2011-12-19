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

#ifndef __container_base_h__
#define __container_base_h__

#include <tqwidget.h>
#include <tqpoint.h>
#include <tqvaluelist.h>

#include <kpanelextension.h>
#include <kpanelapplet.h>

class KConfigGroup;
class TQPopupMenu;

class BaseContainer : public TQWidget
{
    Q_OBJECT

public:
    typedef TQValueList<BaseContainer*> List;
    typedef TQValueListIterator<BaseContainer*> Iterator;
    typedef TQValueListConstIterator<BaseContainer*> ConstIterator;

    BaseContainer( TQPopupMenu* appletOpMenu, TQWidget* parent = 0, const char * name = 0 );
    ~BaseContainer();

    virtual void reparent(TQWidget * parent, WFlags f, const TQPoint & p, bool showIt = false);

    virtual int widthForHeight(int height) const = 0;
    virtual int heightForWidth(int width)  const = 0;

    virtual bool isStretch() const { return false; }

    virtual void completeMoveOperation() {}
    virtual void about() {}
    virtual void help() {}
    virtual void preferences() {}
    virtual void reportBug() {}

    virtual bool isValid() const { return true; }
    bool isImmutable() const;
    virtual void setImmutable(bool immutable);

    double freeSpace() const { return _fspace; }
    void setFreeSpace(double f) { _fspace = f; }

    TQString appletId() const { return _aid; }
    void setAppletId(const TQString& s) { _aid = s; }

    virtual int actions() const { return _actions; }

    KPanelApplet::Direction popupDirection() const { return _dir; }
    KPanelExtension::Orientation orientation() const { return _orient; }
    KPanelExtension::Alignment tqalignment() const { return _tqalignment; }

    virtual void setBackground() {}

    TQPopupMenu* opMenu();
    void clearOpMenu();

    void loadConfiguration( KConfigGroup& );
    void saveConfiguration( KConfigGroup&, bool layoutOnly = false ) const;

    void configure(KPanelExtension::Orientation, KPanelApplet::Direction);
    virtual void configure() {}

    TQPoint moveOffset() const { return _moveOffset; }

    virtual TQString appletType() const = 0;
    virtual TQString icon() const { return "unknown"; }
    virtual TQString visibleName() const = 0;

public slots:
    virtual void slotRemoved(KConfig* config);
    virtual void setPopupDirection(KPanelApplet::Direction d) { _dir = d; }
    virtual void setOrientation(KPanelExtension::Orientation o) { _orient = o; }

    void setAlignment(KPanelExtension::Alignment a);

signals:
    void removeme(BaseContainer*);
    void takeme(BaseContainer*);
    void moveme(BaseContainer*);
    void maintainFocus(bool);
    void requestSave();
    void focusReqested(bool);

protected:
    virtual void doLoadConfiguration( KConfigGroup& ) {}
    virtual void doSaveConfiguration( KConfigGroup&,
                                      bool /* layoutOnly */ ) const {}
    virtual void alignmentChange(KPanelExtension::Alignment) {}

    virtual TQPopupMenu* createOpMenu() = 0;
    TQPopupMenu *appletOpMenu() const { return _appletOpMnu; }

    KPanelApplet::Direction _dir;
    KPanelExtension::Orientation _orient;
    KPanelExtension::Alignment _tqalignment;
    double             _fspace;
    TQPoint             _moveOffset;
    TQString            _aid;
    int                _actions;
    bool               m_immutable;

private:
    TQPopupMenu        *_opMnu;
    TQPopupMenu        *_appletOpMnu;
};

#endif

