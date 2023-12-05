#ifndef KWIN_INTERFACE_H
#define KWIN_INTERFACE_H

#include <dcopobject.h>

class KWinInterface : virtual public DCOPObject
    {
    K_DCOP

    k_dcop:

    virtual ASYNC cascadeDesktop() = 0;
    virtual ASYNC unclutterDesktop() = 0;
    virtual ASYNC reconfigure() = 0;
    virtual ASYNC killWindowId(unsigned long winId) = 0;
    virtual ASYNC suspendWindowId(unsigned long winId) = 0;
    virtual ASYNC resumeWindowId(unsigned long winId) = 0;
    virtual bool isResumeableWindowID(unsigned long winId) = 0;
    virtual void refresh() = 0;
    virtual void doNotManage(TQString)= 0;
    virtual void showWindowMenuAt(unsigned long winId, int x, int y) = 0;
    virtual void showWindowMenu(unsigned long winId) = 0;
    virtual void kDestopResized() = 0;
    virtual void setDesktopLayout(int orientation, int x, int y)= 0;
    virtual bool setCurrentDesktop(int)= 0;
    virtual int currentDesktop() const = 0;
    virtual void nextDesktop() = 0;
    virtual void previousDesktop() = 0;
    virtual void circulateDesktopApplications() = 0;
    virtual void updateOverlappingShadows(unsigned long window) = 0;
    virtual void setShadowed(unsigned long window, bool shadowed) = 0;
    virtual void tileHorizontally(unsigned long w1, unsigned long w2) = 0;
    virtual void tileVertically(unsigned long w1, unsigned long w2) = 0;
    virtual void tileGrid(unsigned long w1, unsigned long w2, unsigned long w3, unsigned long w4) = 0;

    // kompmgr stuff
    virtual void startKompmgr() = 0;
    virtual void stopKompmgr() = 0;
    virtual bool kompmgrIsRunning() = 0;
    virtual void kompmgrReloadSettings() = 0;
    virtual void setOpacity(unsigned long winId, unsigned int opacityPercent) = 0;
    virtual void setShadowSize(unsigned long winId, unsigned int shadowSizePercent) = 0;
    virtual void setUnshadowed(unsigned long winId) = 0;

    k_dcop_signals:

    virtual void kompmgrStarted() = 0;
    virtual void kompmgrStopped() = 0;

    // never emitted  
    virtual void dcopResetAllClients();
    };

#endif
