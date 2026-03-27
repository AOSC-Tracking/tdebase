/*
 *
 * This file is part of the KDE project, module kdesktop.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#ifndef __BGMANAGER_H__
#define __BGMANAGER_H__

#include <tqstring.h>
#include <tqptrvector.h>
#include <tqpixmap.h>

#include <tqdatetime.h>
#include <KBackgroundIface.h>

#if defined(TQ_WS_X11) && defined(HAVE_XRENDER) && TQT_VERSION >= 0x030300
#define COMPOSITE
#endif

class TDEConfig;
class TQTimer;
class TQPixmap;
class TDEPopupMenu;
class KWinModule;
class KPixmap;
class KVirtualBGRenderer;
class KPixmapServer;

/**
 * Internal struct for KBackgroundManager.
 */
struct KBackgroundCacheEntry
{
    int hash;
    int atime;
    int exp_from;
    KPixmap *pixmap;
};


/**
 * Background manager for KDE. This class is to be used in kdesktop. Usage is
 * very simple: instantiate this class once and the desktop background will
 * be painted automatically from now on.
 *
 * The background manager also has a DCOP interface to remotely control its
 * operation. See KBackgroundIface.h for details.
 */

class KBackgroundManager
    : public TQObject,
      public KBackgroundIface
{
    TQ_OBJECT
    

public:
    KBackgroundManager(TQWidget *desktop, KWinModule* twinModule);
    ~KBackgroundManager();

    void configure();
    void setCommon(int);
    bool isCommon() { return m_bCommon; };
    void setExport(int);
    bool isExport() { return m_bExport; };
    void setCache(int, int);
    void setWallpaper(int desk, TQString wallpaper, int mode);
    void setWallpaper(TQString wallpaper, int mode);
    void setWallpaper(TQString wallpaper);
    void changeWallpaper();
    TQString currentWallpaper(int desk);
    void setColor(const TQColor & c, bool isColorA = true);
    void setBackgroundEnabled(const bool enable);
    TQStringList wallpaperList(int desk);
    TQStringList wallpaperFiles(int desk);

signals:
    void initDone();

private slots:
    void slotTimeout();
    void slotImageDone(int desk);
    void slotChangeDesktop(int);
    void slotChangeViewport(int, const TQPoint&);
    void slotChangeNumberOfDesktops(int);
    void repaintBackground();
    void desktopResized();
    void clearRoot();
    void saveImages();
    void slotCrossFadeTimeout();
    void slotCmBackgroundChanged(bool);

private:
    void applyCommon(bool common);
    void applyExport(bool _export);
    void applyCache(bool limit, int size);

    int realDesktop();
    int effectiveDesktop();
    int validateDesk(int desk);
    int m_numberOfViewports;

    void renderBackground(int desk);
    void exportBackground(int pixmap, int desk);
    int pixmapSize(TQPixmap *pm);
    int cacheSize();
    void removeCache(int desk);
    bool freeCache(int size);
    void addCache(KPixmap *pm, int hash, int desk);
    void setPixmap(KPixmap *pm, int hash, int desk);

    bool m_bExport, m_bCommon;
    bool m_bLimitCache, m_bInit;
    bool m_bBgInitDone;
    bool m_bEnabled;

    int m_CacheLimit;
    int m_Serial, m_Hash, m_Current;

    TDEConfig *m_pConfig;
    TQWidget *m_pDesktop;
    TQTimer *m_pTimer;

    KPixmap *m_tPixmap;
	
    TQPtrVector<KVirtualBGRenderer> m_Renderer;
    TQPtrVector<KBackgroundCacheEntry> m_Cache;

    KWinModule *m_pKwinmodule;
    KPixmapServer *m_pPixmapServer;
    
    unsigned long m_xrootpmap;

    /*CrossFade vars*/
    TQTimer * m_crossTimer;
    double mAlpha;
    TQPixmap  mNextScreen;
    TQPixmap  mOldScreen;
    int fadeDesk;
    TQTime mBenchmark;
    bool crossInit;
    bool resizingDesktop; // is True if desktop is resizing
};

#endif // __BGMANAGER_H__
