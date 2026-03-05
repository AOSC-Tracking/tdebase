/**************************************************************************

    kpager.h  - KPager's main window
    Copyright (C) 2000  Antonio Larrosa Jimenez
			Matthias Ettrich
			Matthias Elter

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    Send comments and bug fixes to larrosa@kde.org

***************************************************************************/
#ifndef __KPAGER_H
#define __KPAGER_H

#include <tqwidget.h>
#include <tqintdict.h>
#include <tdemainwindow.h>
#include <twin.h>
#include "kpagerIface.h"

class KPager;
class TQPopupMenu;

class KPagerMainWindow : public TDEMainWindow, public virtual KPagerIface
{
    TQ_OBJECT

public:
    KPagerMainWindow(TQWidget *parent=0, const char *name=0);
    ~KPagerMainWindow();

    virtual void showAt(int x, int y);
    virtual void toggleShow(int x, int y);

public slots:
    virtual void reallyClose();
    
protected:
    bool queryClose();
    void showEvent(TQShowEvent *ev);

    KPager *m_pPager;
    class TQTimer *timeout;
    bool m_reallyClose;
};

class KPager : public TQFrame
{
    TQ_OBJECT

    friend class KPagerMainWindow;

public:
    KPager(KPagerMainWindow *parent=0, const char *name=0);
    ~KPager();

    class TWinModule *twin() const { return m_winmodule; };
    void updateLayout();

    void redrawDesktops();

    void showPopupMenu( WId wid, TQPoint pos);

    KWin::WindowInfo* info( WId win );

    TQSize sizeHint() const;

    enum LayoutTypes { Classical=0, Horizontal, Vertical };

public slots:
    void configureDialog();

    void slotActiveWindowChanged( WId win );
    void slotWindowAdded( WId );
    void slotWindowRemoved( WId );
    void slotWindowChanged( WId, unsigned int );
    void slotStackingOrderChanged();
    void slotDesktopNamesChanged();
    void slotNumberOfDesktopsChanged(int ndesktops);
    void slotCurrentDesktopChanged(int);

    void slotGrabWindows();

protected slots:
    void slotBackgroundChanged(int);
    void clientPopupAboutToShow();
    void clientPopupActivated(int);
    void desktopPopupAboutToShow();
    void sendToDesktop(int desk);

protected:
    enum WindowOperation {
        MaximizeOp = 100,
        IconifyOp,
        StickyOp,
        CloseOp
    };

protected:
    TWinModule *m_winmodule;
    TQValueList<class Desktop *> m_desktops;

    TQIntDict<KWin::WindowInfo> m_windows;
    WId m_activeWin;

    const TQString lWidth();
    const TQString lHeight();

    LayoutTypes m_layoutType;

    class TQGridLayout *m_layout;
    TDEPopupMenu *m_mnu;
    TQPopupMenu *m_smnu, *m_dmnu;
    TDEAction *m_quit_action;
    TDEAction *m_prefs_action;
    KWin::WindowInfo m_winfo;
    bool m_showStickyOption; // To be removed after the message freeze

    TQTimer *m_grabWinTimer;
    int     m_currentDesktop;

public:
    static const LayoutTypes c_defLayout;
};

#endif
