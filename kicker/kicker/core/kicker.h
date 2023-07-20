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

#ifndef __kicker_h__
#define __kicker_h__

#include <tqcolor.h>

#include <kuniqueapplication.h>
#include <kicontheme.h>

class KCMultiDialog;
class KDirWatch;
class TDEGlobalAccel;
class KWinModule;
class PanelKMenu;
class PanelPopupButton;

class Kicker : public KUniqueApplication
{
    TQ_OBJECT
    K_DCOP

public:
    Kicker();
    ~Kicker();

k_dcop:
    void configure();
    void quit();
    void restart();
    void addExtension( const TQString &desktopFile );
    void popupKMenu( const TQPoint &globalPos );
    void clearQuickStartMenu();
    bool highlightMenuItem( const TQString &menuId );
    void showKMenu();
    void toggleShowDesktop();
    bool desktopShowing();
    void showConfig(const TQString& config, const TQString& configFile = TQString::null, int page = -1);
    void showTaskBarConfig();
    void showTaskBarConfig(const TQString& configFile);
    void hideTaskBarConfig();
    void reshowTaskBarConfig();
    void configureMenubar();
    // return the region on the desktop, which is not covered by panels
    // and therefore allowed to be used by icons placed on the desktop
    TQRect desktopIconsArea(int screen) const;

k_dcop_signals:
    void desktopIconsAreaChanged(TQRect area, int screen);

public:
    static Kicker* the();
    KDirWatch* fileWatcher();
    KWinModule* twinModule();

    bool isImmutable() const;
    bool isKioskImmutable() const;
    bool canAddContainers() const { return m_canAddContainers && !isImmutable(); }

    static TQStringList configModules(bool controlCenter);

    /**
     * Global position where to insert a new item
     */
    TQPoint insertionPoint();

    /**
     * Set the global position where to insert a new item
     * This is not meant to be used by more than one code path
     * at a time! A point is set, then later accessed, then cleared
     * as an atomic action:
     *
     * Kicker::the()->setInsertionPoint(p);
     * ...
     * TQPoint where = Kicker::the()->insertionPoint();
     * ...
     * Kicker::the()->setInsertionPoint(TQPoint());
     *
     * this is not pretty, but it's pragmatic and does everything
     * that is needed
     */
    void setInsertionPoint(const TQPoint &p);


public slots:
    void slotToggleShowDesktop();
    void toggleLock();

signals:
    void configurationChanged();
    void immutabilityChanged(bool immutable);

private slots:
    void configDialogFinished();
    void slotSettingsChanged( int );
    void slotRestart();
    void slotDesktopResized();
    void slotStyleChanged();
    void paletteChanged();
    void setCrashHandler();
    void slotDesktopIconsAreaChanged(const TQRect &area, int screen);

private:
    static void crashHandler(int signal);

    TDEGlobalAccel*  keys;
    KWinModule*      m_twinModule;
    KCMultiDialog*   m_configDialog;
    bool             m_canAddContainers;
    TQPoint          m_insertionPoint;
    bool             m_reloadingConfigDialog;
};

#endif
