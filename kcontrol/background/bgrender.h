/*
 *
 * This file is part of the KDE project, module kdesktop.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 *
 * You can Freely distribute this program under the GNU Library General
 * Public License. See the file "COPYING.LIB" for the exact licensing terms.
 */

#ifndef BGRender_h_Included
#define BGRender_h_Included

#include <tqobject.h>
#include <tqptrvector.h>
#include <tqimage.h>
#include <tqpixmap.h>

#include "bgsettings.h"

class TQSize;
class TQRect;
class TQString;
class TQTimer;

class TDEConfig;
class TDEProcess;
class KTempFile;
class KShellProcess;
class TDEStandardDirs;
class KCrossBGRender;

/**
 * This class renders a desktop background to a TQImage. The operation is
 * asynchronous: connect to the signal imageDone() to find out when the
 * rendering is finished. It also has support for preview images, like
 * the monitor in kcmdisplay.
 */
class KBackgroundRenderer:
    public TQObject,
    public KBackgroundSettings
{
    TQ_OBJECT

public:
    KBackgroundRenderer(int desk, int screen, bool drawBackgroundPerScreen, TDEConfig *config=0);
    ~KBackgroundRenderer();
    
    void load(int desk, int screen, bool drawBackgroundPerScreen, bool reparseConfig=true);

    void setPreview(const TQSize &size);
    void setSize(const TQSize &size);
    
    TQPixmap pixmap();
    TQImage image();
    bool isActive() { return m_State & Rendering; }
    void cleanup();
    void saveCacheFile();
    void enableTiling( bool enable ) { m_TilingEnabled = enable; }

public slots:
    void start(bool enableBusyCursor = false);
    void stop();
    void desktopResized();

signals:
    void imageDone(int desk, int screen);
    void programFailure(int desk, int exitstatus); //Guaranteed either programFailure or 
    void programSuccess(int desk);                 //programSuccess is emitted after imageDone

private slots:
    void slotBackgroundDone(TDEProcess *);
    void render();
    void done();

private:
    enum { Error, Wait, WaitUpdate, Done };
    enum { Rendering = 1, InitCheck = 2,
           BackgroundStarted = 4, BackgroundDone = 8,
           WallpaperStarted = 0x10, WallpaperDone = 0x20,
           AllDone = 0x40 };

    TQString buildCommand();
    void createTempFile();
    void tile(TQImage& dst, TQRect rect, const TQImage& src);
    void blend(TQImage& dst, TQRect dr, const TQImage& src, TQPoint soffs = TQPoint(0, 0), int blendFactor=100);

    void wallpaperBlend();
    void fastWallpaperBlend();
    void fullWallpaperBlend();

    int doBackground(bool quit=false);
    int doWallpaper(bool quit=false);
    void setBusyCursor(bool isBusy);
    TQString cacheFileName();
    bool useCacheFile() const;
    bool canTile() const;
    
    bool m_isBusyCursor;
    bool m_enableBusyCursor;
    bool m_bPreview;
    int m_State;
    bool m_Cached;
    bool m_TilingEnabled;

    KTempFile* m_Tempfile;
    TQSize m_Size, m_rSize;
    TQRect m_WallpaperRect;
    TQImage m_Image, m_Background, m_Wallpaper;
    TQPixmap m_Pixmap;
    TQTimer *m_pTimer;

    TDEStandardDirs *m_pDirs;
    KShellProcess *m_pProc;
    
};

/**
 * In xinerama mode, each screen is rendered seperately by KBackgroundRenderer.
 * This class controls a set of renderers for a desktop, and coallates the
 * images. Usage is similar to KBackgroundRenderer: connect to the imageDone
 * signal.
 */
class KVirtualBGRenderer : public TQObject
{
    TQ_OBJECT
public:
    KVirtualBGRenderer(int desk, TDEConfig *config=0l);
    ~KVirtualBGRenderer();
    
    KCrossBGRender * renderer(unsigned screen);
    unsigned numRenderers() const { return m_numRenderers; }

    TQPixmap pixmap();
    
    void setPreview(const TQSize & size);
    
    bool needProgramUpdate();
    void programUpdate();
    
    bool needWallpaperChange();
    void changeWallpaper();
    
    int hash();
    bool isActive();
    void setEnabled( bool enable );
    void desktopResized();
    
    void load(int desk, bool reparseConfig=true);
    void start();
    void stop();
    void cleanup();
    void saveCacheFile();
    void enableTiling( bool enable );

signals:
    void imageDone(int desk);

private slots:
    void screenDone(int desk, int screen);

private:
    TQSize renderSize(int screen); // the size the renderer should be
    void initRenderers();
    
    TDEConfig *m_pConfig;
    float m_scaleX;
    float m_scaleY;
    int m_desk;
    unsigned m_numRenderers;
    bool m_bDrawBackgroundPerScreen;
    bool m_bCommonScreen;
    bool m_bDeleteConfig;
    TQSize m_size;
    
    TQMemArray<bool> m_bFinished;
    TQPtrVector<KCrossBGRender> m_renderer;
    TQPixmap *m_pPixmap;
};


#endif // BGRender_h_Included

