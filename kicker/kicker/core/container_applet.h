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

#ifndef __container_applet_h__
#define __container_applet_h__

#include <kpanelapplet.h>
#include <dcopobject.h>
#include <twin.h>

#include "appletinfo.h"
#include "container_base.h"

class TQHBox;
class QXEmbed;
class TQBoxLayout;
class TDEConfig;

class AppletHandle;

class AppletContainer : public BaseContainer
{
    TQ_OBJECT

public:
    AppletContainer(const AppletInfo& info, TQPopupMenu* opMenu, bool isImmutable = false, TQWidget* parent = 0);

    KPanelApplet::Type type() const { return _type; }
    const AppletInfo& info() const { return _info; }
    const TQPopupMenu* appletsOwnMenu() const;
    bool isStretch() const { return type() ==  KPanelApplet::Stretch; }
    void resetLayout();

    virtual void configure();
    virtual void about();
    virtual void help();
    virtual void preferences();
    virtual void reportBug();
    virtual void setBackground();
    virtual bool isValid() const { return _valid; }
    virtual TQString appletType() const { return "Applet"; }
    virtual TQString icon() const { return _info.icon(); }
    virtual TQString visibleName() const { return _info.name(); }

    int widthForHeight(int height) const;
    int heightForWidth(int width)  const;

    void setWidthForHeightHint(int w) { _widthForHeightHint = w; }
    void setHeightForWidthHint(int h) { _heightForWidthHint = h; }

signals:
    void updateLayout();

public slots:
    virtual void slotRemoved(TDEConfig* config);
    virtual void setPopupDirection(KPanelApplet::Direction d);
    virtual void setOrientation(KPanelExtension::Orientation o);
    virtual void setImmutable(bool immutable);
    void moveApplet( const TQPoint& moveOffset );
    void showAppletMenu();
    void slotReconfigure();
    void activateWindow();

protected:
    virtual void doLoadConfiguration( TDEConfigGroup& );
    virtual void doSaveConfiguration( TDEConfigGroup&, bool layoutOnly ) const;
    virtual void alignmentChange(KPanelExtension::Alignment a);

    virtual TQPopupMenu* createOpMenu();

    AppletInfo         _info;
    AppletHandle      *_handle;
    TQHBox             *_appletframe;
    TQBoxLayout        *_layout;
    KPanelApplet::Type _type;
    int                _widthForHeightHint;
    int                _heightForWidthHint;
    TQString            _deskFile, _configFile;
    bool               _firstuse;
    TQCString           _id;
    KPanelApplet *     _applet;
    bool               _valid;

protected slots:
    void slotRemoveApplet();
    void slotUpdateLayout();
    void signalToBeRemoved();
    void slotDelayedDestruct();
    void focusRequested(bool);
};

#endif

