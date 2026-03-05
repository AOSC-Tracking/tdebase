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

#ifndef __MINIPAGER_H
#define __MINIPAGER_H

#include <tqvaluelist.h>
#include <tqintdict.h>

#include <kpanelapplet.h>
#include <twin.h>

#include "pagerbutton.h"
#include "pagersettings.h"

class TQButtonGroup;
class TQGridLayout;
class TQTimer;

class TDEProcess;
class TWinModule;
class KTextShadowEngine;
class TDESelectionOwner;

class PagerSettings;

class KMiniPager : public KPanelApplet
{
    TQ_OBJECT

public:
    KMiniPager(const TQString& configFile, Type t = Normal, int actions = 0,
               TQWidget *parent = 0, const char *name = 0);

    virtual ~KMiniPager();

    int widthForHeight(int height) const;
    int heightForWidth(int width) const;

    KWin::WindowInfo* info( WId win );
    TWinModule* twin() { return m_twin; }
    KTextShadowEngine* shadowEngine();

    void setActive( WId active ) { m_activeWindow = active; }
    WId activeWindow() { return m_activeWindow; }

    enum ConfigOptions { LaunchExtPager = 96, WindowThumbnails, Cycle, Border3D,
                         WindowIcons, ConfigureDesktops, RenameDesktop };
    int labelType() const { return m_settings->labelType(); }

    int bgType() const { return m_settings->backgroundType(); }

    bool desktopPreview() const { return m_settings->preview(); }
    bool cycleWindow() const { return m_settings->cycle(); }
    bool windowIcons() const { return m_settings->icons(); }
    bool border3D() const { return m_settings->border3D(); }


    Orientation orientation() const { return KPanelApplet::orientation(); }
    Direction popupDirection() { return KPanelApplet::popupDirection(); }

    void emitRequestFocus() { emit requestFocus(); }

    TQPoint clickPos;

public slots:
    void slotSetDesktop(int desktop);
    void slotSetDesktopViewport(int desktop, const TQPoint& viewport);
    void slotSetDesktopCount(int count);
    void slotRefreshViewportCount(int currentDesktop);
    void slotButtonSelected(int desk );
    void slotActiveWindowChanged( WId win );
    void slotWindowAdded( WId );
    void slotWindowRemoved( WId );
    void slotWindowChanged( WId, unsigned int );
    void slotShowMenu( const TQPoint&, int );
    void slotDesktopNamesChanged();
    void slotBackgroundChanged( int );

    void refresh();

protected:
    void drawButtons();
    void startDrag( const TQPoint &point );

    void updateDesktopLayout(int,int,int);
    void resizeEvent(TQResizeEvent*);
    void wheelEvent( TQWheelEvent* e );
    void showKPager(bool toggleShow);

protected slots:
    void showPager();
    void applicationRegistered(const TQCString &appName);
    void aboutToShowContextMenu();
    void contextMenuActivated(int);

private:
    TQValueList<KMiniPagerButton*> m_desktops;
    int m_curDesk;
    int m_rmbDesk;

    TQIntDict<KWin::WindowInfo> m_windows;
    WId m_activeWindow;

    TQButtonGroup *m_group;

    TQGridLayout *m_layout;
    bool m_useViewports;
    int desktopLayoutOrientation;
    int desktopLayoutX;
    int desktopLayoutY;
    TDESelectionOwner* m_desktopLayoutOwner;

    TWinModule *m_twin;
    KTextShadowEngine* m_shadowEngine;

    TQPopupMenu *m_contextMenu;
    PagerSettings *m_settings;
};

#endif

