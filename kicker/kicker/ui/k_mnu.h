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

#ifndef __k_mnu_h__
#define __k_mnu_h__

#include <dcopobject.h>
#include <tqintdict.h>
#include <tqpixmap.h>
#include <tqtimer.h>

#include "service_mnu.h"

namespace KPIM {
    // Yes, ClickLineEdit was copied from libkdepim.
    // Can we have it in kdelibs please?
    class ClickLineEdit;
}

class KickerClientMenu;
class KBookmarkMenu;
class KActionCollection;
class KBookmarkOwner;
class Panel;

class PanelKMenu : public PanelServiceMenu, public DCOPObject
{
    Q_OBJECT
    K_DCOP

k_dcop:
    void slotServiceStartedByStorageId(TQString starter, TQString desktopPath);
    void hideMenu();

public:
    PanelKMenu();
    ~PanelKMenu();

    int insertClientMenu(KickerClientMenu *p);
    void removeClientMenu(int id);

    virtual TQSize sizeHint() const;
    virtual void setMinimumSize(const TQSize &);
    virtual void setMaximumSize(const TQSize &);
    virtual void setMinimumSize(int, int);
    virtual void setMaximumSize(int, int);
    virtual void showMenu();
    void clearRecentMenuItems();

public slots:
    virtual void initialize();

    //### KDE4: workaround for Qt bug, remove later
    virtual void resize(int width, int height);

protected slots:
    void slotLock();
    void slotLogout();
    void slotPopulateSessions();
    void slotSessionActivated( int );
    void slotSaveSession();
    void slotRunCommand();
    void slotEditUserContact();
    void slotUpdateSearch(const TQString &searchtext);
    void slotClearSearch();
    void paletteChanged();
    virtual void configChanged();
    void updateRecent();
    void repairDisplay();
    void windowClearTimeout();

protected:
    TQRect sideImageRect();
    TQMouseEvent translateMouseEvent(TQMouseEvent* e);
    void resizeEvent(TQResizeEvent *);
    void paintEvent(TQPaintEvent *);
    void mousePressEvent(TQMouseEvent *);
    void mouseReleaseEvent(TQMouseEvent *);
    void mouseMoveEvent(TQMouseEvent *);
    bool loadSidePixmap();
    void doNewSession(bool lock);
    void filterMenu(PanelServiceMenu* menu, const TQString &searchString);
    void keyPressEvent(TQKeyEvent* e);
    void createRecentMenuItems();
    virtual void clearSubmenus();

private:
    TQPopupMenu                 *sessionsMenu;
    TQPixmap                     sidePixmap;
    TQPixmap                     sideTilePixmap;
    int                         client_id;
    bool                        delay_init;
    TQIntDict<KickerClientMenu>  clients;
    KBookmarkMenu              *bookmarkMenu;
    KActionCollection          *actionCollection;
    KBookmarkOwner             *bookmarkOwner;
    PopupMenuList               dynamicSubMenus;
    KPIM::ClickLineEdit        *searchEdit;
    static const int            searchLineID;
    TQTimer                    *displayRepairTimer;
    bool                        displayRepaired;
    bool                        windowTimerTimedOut;
};

#endif
