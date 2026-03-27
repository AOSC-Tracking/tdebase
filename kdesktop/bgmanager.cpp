/*
 *
 * This file is part of the KDE project, module kdesktop.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#include <config.h>
#include "bgrender.h"
#include "bgmanager.h"
#include "bgdefaults.h"
#include "kdesktopsettings.h"
#include "bgsettings.h"
#include "kdesktopapp.h"

#include "KCrossBGRender.h"
#include "crossfade.h"

#include <assert.h>

#include <tqtimer.h>
#include <tqscrollview.h>
#include <tqpainter.h>
#include <tqdesktopwidget.h>

#include <kiconloader.h>
#include <tdeconfig.h>
#include <twin.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kipc.h>
#include <tdepopupmenu.h>
#include <twinmodule.h>
#include <krootpixmap.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <unistd.h>

#ifndef None
#define None 0L
#endif

#ifdef COMPOSITE
# include <X11/Xlib.h>
# include <X11/extensions/Xrender.h>
# include <fixx11h.h>
#endif

#include "pixmapserver.h"

template class TQPtrVector<KCrossBGRender>;
template class TQPtrVector<KBackgroundCacheEntry>;
template class TQMemArray<int>;

static Atom prop_root;
static bool properties_inited = false;

extern bool argb_visual;
extern KDesktopApp *myApp;

/**** KBackgroundManager ****/

KBackgroundManager::KBackgroundManager(TQWidget *desktop, TWinModule* twinModule)
    : KBackgroundIface()
{
    if( !properties_inited )
    {
        prop_root = XInternAtom(tqt_xdisplay(), "_XROOTPMAP_ID", False);
        properties_inited = true;
    }
    m_bBgInitDone = false;
    m_bEnabled = true;

    m_pDesktop = desktop;
    if (desktop == 0L)
        desktop = TDEApplication::desktop()->screen();

    m_Renderer.resize( 1 );
    m_Cache.resize( 1 );

    m_Serial = 0; m_Hash = 0;
    m_pConfig = TDEGlobal::config();
    m_bExport = m_bCommon = m_bInit = false;
    m_pTWinModule = twinModule;
    m_pPixmapServer = new KPixmapServer();
    m_xrootpmap = None;

    for (unsigned i=0; i<m_Renderer.size(); i++)
    {
	m_Cache.insert(i, new KBackgroundCacheEntry);
        m_Cache[i]->pixmap = 0L;
        m_Cache[i]->hash = 0;
        m_Cache[i]->exp_from = -1;
        m_Renderer.insert (i, new KVirtualBGRenderer(i,m_pConfig));
        connect(m_Renderer[i], TQ_SIGNAL(imageDone(int)), TQ_SLOT(slotImageDone(int)));
        m_Renderer[i]->enableTiling( true ); // optimize
    }

#ifdef COMPOSITE
    m_tPixmap = new KPixmap(tdeApp->desktop()->size());
    m_tPixmap->fill(TQColor(0, 0x0));
    connect(myApp, TQ_SIGNAL(cmBackgroundChanged( bool )),
            TQ_SLOT(slotCmBackgroundChanged( bool )));
#endif

    configure();

    m_pTimer = new TQTimer(this);
    connect(m_pTimer, TQ_SIGNAL(timeout()), TQ_SLOT(slotTimeout()));
    m_pTimer->start( 60000 );

    /*CrossFade's config*/
    m_crossTimer = new TQTimer(this);
    connect(m_crossTimer, TQ_SIGNAL(timeout()), TQ_SLOT(slotCrossFadeTimeout()));
    resizingDesktop = true;
    /*Ends here*/


    connect(m_pTWinModule, TQ_SIGNAL(currentDesktopChanged(int)),
	    TQ_SLOT(slotChangeDesktop(int)));
    connect(m_pTWinModule, TQ_SIGNAL(numberOfDesktopsChanged(int)),
	    TQ_SLOT(slotChangeNumberOfDesktops(int)));
    connect(m_pTWinModule, TQ_SIGNAL(currentDesktopViewportChanged(int, const TQPoint&)),
            TQ_SLOT(slotChangeViewport(int, const TQPoint&)));


#if (TQT_VERSION-0 >= 0x030200)
    connect( tdeApp->desktop(), TQ_SIGNAL( resized( int )), TQ_SLOT( desktopResized())); // RANDR support
#endif

    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();
    if (m_numberOfViewports < 1) {
        m_numberOfViewports = 1;
    }
    for (signed j=0;j<(m_pTWinModule->numberOfDesktops() * m_numberOfViewports);j++) {
        renderBackground(j);
    }
}


KBackgroundManager::~KBackgroundManager()
{
    for (unsigned i=0; i<m_Renderer.size(); i++)
        delete m_Renderer[i];

    //delete m_pConfig; Very bad idea, this is TDEGlobal::config !
    delete m_pPixmapServer;
    delete m_pTimer;

    // clear the Esetroot properties, as the pixmaps they refer to are going away...
    Pixmap pm = None;
    Atom type;
    int format;
    unsigned long length, after;
    unsigned char* data_root;
    if( XGetWindowProperty( tqt_xdisplay(), tqt_xrootwin(), prop_root, 0L, 1L, False, AnyPropertyType,
	&type, &format, &length, &after, &data_root) == Success && data_root != NULL )
    {
	if (type == XA_PIXMAP)
	    pm = *((Pixmap*)data_root);
        XFree( data_root );
    }
    // only if it's our pixmap
    if( pm == m_xrootpmap )
	XDeleteProperty(tqt_xdisplay(), tqt_xrootwin(), prop_root);
    m_xrootpmap = None;

    if (m_bExport)
        return;

    for (unsigned i=0; i<m_Cache.size(); i++)
    {
        delete m_Cache[i]->pixmap;
        delete m_Cache[i];
    }
}


void KBackgroundManager::applyExport(bool exp)
{
    if (exp == m_bExport)
	return;

    // If export mode changed from true -> false, remove all shared pixmaps.
    // If it changed false -> true force a redraw because the current screen
    // image might not have an associated pixmap in the cache.
    if (!exp)
    {
	for (unsigned i=0; i<m_Cache.size(); i++)
	    removeCache(i);
    } else
	m_Hash = 0;

    m_bExport = exp;
}


void KBackgroundManager::applyCommon(bool common)
{
    if (common == m_bCommon)
	return;
    m_bCommon = common;

    // If common changed from false -> true, remove all cache entries, except
    // at index 0 if exports are on.
    if (m_bCommon)
    {
	if (!m_bExport)
	    removeCache(0);
	for (unsigned i=1; i<m_Cache.size(); i++)
	    removeCache(i);
    }
}


void KBackgroundManager::applyCache(bool limit, int size)
{
    m_bLimitCache = limit;
    m_CacheLimit = size;
    freeCache(0);
}


/*
 * Call this when the configuration has changed.
 * This method is exported with DCOP.
 */
void KBackgroundManager::configure()
{
    // Global settings
    m_pConfig->reparseConfiguration();
    KDesktopSettings::self()->readConfig();

    // Read individual settings
    KVirtualBGRenderer *r;
    for (unsigned i=0; i<m_Renderer.size(); i++)
    {
        r = m_Renderer[i];
        int ohash = r->hash();
        r->load(i,false);
        if ((r->hash() != ohash))
            removeCache(i);
    }

    applyCommon(KDesktopSettings::commonDesktop());

    bool limit = KDesktopSettings::limitCache();
    int size = KDesktopSettings::cacheSize() * 1024;
    applyCache(limit, size);

    // Repaint desktop
    slotChangeDesktop(0);

    // Redraw all desktops so that applications relying on exported data, e.g. kpager, continue to work properly
    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();
    if (m_numberOfViewports < 1) {
        m_numberOfViewports = 1;
    }
    for (signed j=0;j<(m_pTWinModule->numberOfDesktops() * m_numberOfViewports);j++) {
        renderBackground(j);
    }
}


int KBackgroundManager::realDesktop()
{
    int desk = m_pTWinModule->currentDesktop();
    if (desk) desk--;
    return desk;
}


int KBackgroundManager::effectiveDesktop()
{
    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();

    if (m_numberOfViewports > 1) {
        if (m_bCommon) {
            return 0;
        }
        else {
            TQPoint vx(m_pTWinModule->currentViewport(m_pTWinModule->currentDesktop()));
            return (realDesktop() * m_numberOfViewports) + ((vx.x() * vx.y()) - 1);
        }
    }
    else {
        return m_bCommon ? 0 : realDesktop();
    }
}


/*
 * Number of desktops changed
 */
void KBackgroundManager::slotChangeNumberOfDesktops(int num)
{
    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();
    if (m_numberOfViewports < 1) {
        m_numberOfViewports = 1;
    }
    num = (num * m_numberOfViewports);

    if (m_Renderer.size() == (unsigned) num)
	return;

    if (m_Renderer.size() > (unsigned) num)
    {
	for (unsigned i=num; i<m_Renderer.size(); i++)
	{
	    if (m_Renderer[i]->isActive())
		m_Renderer[i]->stop();
	    delete m_Renderer[i];
	    removeCache(i);
	}
	for (unsigned i=num; i<m_Renderer.size(); i++)
	    delete m_Cache[i];
	m_Renderer.resize(num);
	m_Cache.resize(num);
    } else
    {
	// allocate new renderers and caches
	int oldsz = m_Renderer.size();
	m_Renderer.resize(num);
	m_Cache.resize(num);
	for (int i=oldsz; i<num; i++)
	{
	    m_Cache.insert(i, new KBackgroundCacheEntry);
	    m_Cache[i]->pixmap = 0L;
	    m_Cache[i]->hash = 0;
	    m_Cache[i]->exp_from = -1;
            m_Renderer.insert(i, new KVirtualBGRenderer(i,m_pConfig));
	    connect(m_Renderer[i], TQ_SIGNAL(imageDone(int)), TQ_SLOT(slotImageDone(int)));
            m_Renderer[i]->enableTiling( true ); // optimize
	}
    }
}

/*
 * Call this when the desktop has been changed.
 * Desk is in KWin convention: [1..desks], instead of [0..desks-1].
 * 0 repaints the current desktop.
 */
void KBackgroundManager::slotChangeDesktop(int desk)
{
    resizingDesktop = true;
    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();
    if (m_numberOfViewports < 1) {
        m_numberOfViewports = 1;
    }

    if (desk == 0)
	desk = realDesktop();
    else
	desk--;

    // Lazy initialisation of # of desktops
    if ((unsigned)(m_pTWinModule->numberOfDesktops() * m_numberOfViewports) >= m_Renderer.size())
        slotChangeNumberOfDesktops( m_pTWinModule->numberOfDesktops() * m_numberOfViewports);

    int edesk = effectiveDesktop();
    m_Serial++;

    // If the background is the same: do nothing
    if ((m_Hash == m_Renderer[edesk]->hash()) && (desk != 0))
    {
	exportBackground(m_Current, desk);
        return;
    }
    m_Renderer[edesk]->stop();
    m_Renderer[edesk]->cleanup();

    // If we have the background already rendered: set it
    for (unsigned i=0; i<m_Cache.size(); i++)
    {
	if (!m_Cache[i]->pixmap)
	    continue;
	if (m_Cache[i]->hash != m_Renderer[edesk]->hash())
	    continue;
        if (desk == 0)
            continue;
//        kdDebug() << "slotChangeDesktop i=" << i << endl;
	setPixmap(m_Cache[i]->pixmap, m_Cache[i]->hash, i);
	m_Cache[i]->atime = m_Serial;
	exportBackground(i, desk);
	return;
    }

    // Do we have this or an identical config already running?
    for (unsigned i=0; i<m_Renderer.size(); i++)
    {
        if (((m_Renderer[i]->hash() == m_Renderer[edesk]->hash()) && (m_Renderer[i]->isActive())) && (desk != 0)) {
            return;
        }
    }

    renderBackground(edesk);
}

/*
 * Call this when the viewport has been changed.
 * Desk is in KWin convention: [1..desks], instead of [0..desks-1].
 * 0 repaints the current viewport.
 */
void KBackgroundManager::slotChangeViewport(int desk, const TQPoint& viewport)
{
    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();
    if (m_numberOfViewports < 1) {
        m_numberOfViewports = 1;
    }

    if (desk == 0)
        desk = realDesktop();
    else
        desk--;

    // Lazy initialisation of # of desktops
    if ((unsigned)(m_pTWinModule->numberOfDesktops() * m_numberOfViewports) >= m_Renderer.size())
        slotChangeNumberOfDesktops( m_pTWinModule->numberOfDesktops() * m_numberOfViewports );

    int edesk = effectiveDesktop();
    m_Serial++;

    // If the background is the same: do nothing
    if ((m_Hash == m_Renderer[edesk]->hash()) && (desk != 0))
    {
        exportBackground(m_Current, desk);
        return;
    }
    m_Renderer[edesk]->stop();
    m_Renderer[edesk]->cleanup();

    // If we have the background already rendered: set it
    for (unsigned i=0; i<m_Cache.size(); i++)
    {
        if (!m_Cache[i]->pixmap)
            continue;
        if (m_Cache[i]->hash != m_Renderer[edesk]->hash())
            continue;
        if (desk == 0)
            continue;
//        kdDebug() << "slotChangeDesktop i=" << i << endl;
        
        //KPixmap * viewport_background = new KPixmap(TQPixmap(m_Cache[i]->pixmap->width()*s.width(), m_Cache[i]->pixmap->height()*s.height()));
        //setPixmap(viewport_background, m_Cache[i]->hash, i);
        //delete viewport_background;
        
        setPixmap(m_Cache[i]->pixmap, m_Cache[i]->hash, i);
        m_Cache[i]->atime = m_Serial;
        exportBackground(i, desk);
        return;
    }

    // Do we have this or an identical config already running?
    for (unsigned i=0; i<m_Renderer.size(); i++)
    {
        if (((m_Renderer[i]->hash() == m_Renderer[edesk]->hash()) && (m_Renderer[i]->isActive())) && (desk != 0))
            return;
    }

    renderBackground(edesk);
}


/*
 * Share a desktop pixmap.
 */
void KBackgroundManager::exportBackground(int pixmap, int desk)
{
    if (!m_bExport || (m_Cache[desk]->exp_from == pixmap))
        return;

    m_Cache[desk]->exp_from = pixmap;
    m_pPixmapServer->add(KRootPixmap::pixmapName(desk+1),
	    m_Cache[pixmap]->pixmap);
    KIPC::sendMessageAll(KIPC::BackgroundChanged, desk+1);
}


/*
 * Paint the pixmap to the root window.
 */
void KBackgroundManager::setPixmap(KPixmap *pm, int hash, int desk)
{
    KPixmap *ep = pm;

#ifdef COMPOSITE
    if (argb_visual && (KDesktopSettings::backgroundOpacity() < 100
        || myApp->cmBackground()))
    {
        ep = m_tPixmap;
        if (KDesktopSettings::backgroundOpacity() > 0 && pm
            && !myApp->cmBackground())
        {
            XRenderPictFormat *format;
            format = XRenderFindStandardFormat (tqt_xdisplay(), PictStandardARGB32);

            XRenderColor fillColor;

            int color = KDesktopSettings::backgroundOpacity() * 0xffff / 100;
            fillColor.red = color;
            fillColor.green = color;
            fillColor.blue = color;
            fillColor.alpha = color;

            Picture fill = XRenderCreateSolidFill (tqt_xdisplay(), &fillColor);
            Picture src = XRenderCreatePicture(tqt_xdisplay(), pm->handle(),
                                               format, 0, NULL);
            Picture dst = XRenderCreatePicture(tqt_xdisplay(), ep->handle(),
                                               format, 0, NULL);

            XRenderComposite (tqt_xdisplay(), PictOpSrc, src, fill, dst, 0, 0, 0,
                              0, 0, 0, pm->width(), pm->height());

            XRenderFreePicture (tqt_xdisplay(), fill);
            XRenderFreePicture (tqt_xdisplay(), src);
            XRenderFreePicture (tqt_xdisplay(), dst);
        }
    }
#endif

    if (m_pDesktop)
    {
       TQScrollView* sv = dynamic_cast<TQScrollView*>( m_pDesktop );
       if ( sv ) {
         // Qt eats repaint events in this case :-((
         sv->viewport()->update();
       }
       m_pDesktop->setErasePixmap(*ep);
       m_pDesktop->repaint();
       static bool root_cleared = false;
       if( !root_cleared )
       { // clear the root window pixmap set by tdm
          root_cleared = true;
	  TQTimer::singleShot( 0, this, TQ_SLOT( clearRoot()));
          // but make the pixmap visible until m_pDesktop is visible
          TDEApplication::desktop()->screen()->setErasePixmap(*ep);
          TDEApplication::desktop()->screen()->erase();
       }
    }
    else
    {
        TDEApplication::desktop()->screen()->setErasePixmap(*ep);
        TDEApplication::desktop()->screen()->erase();
    }

     // and export it via Esetroot-style for gnome/GTK apps to share in the pretties
    Pixmap bgPm = pm->handle(); // fetch the actual X handle to it
    //kdDebug() << "Esetroot compat:  setting pixmap to " << bgPm << endl;

    // don't set the ESETROOT_PMAP_ID property - that would result in possible XKillClient()
    // done on kdesktop

    XChangeProperty(tqt_xdisplay(), tqt_xrootwin(), prop_root, XA_PIXMAP, 32, PropModeReplace,
                    (unsigned char *) &bgPm, 1);
    m_xrootpmap = bgPm;

    m_Hash = hash;
    m_Current = desk;
}

void KBackgroundManager::clearRoot()
{
    TDEApplication::desktop()->screen()->setErasePixmap( TQPixmap());
    TDEApplication::desktop()->screen()->erase();
}

/*
 * Start the render of a desktop background.
 */
void KBackgroundManager::renderBackground(int desk)
{
    KVirtualBGRenderer *r = m_Renderer[desk];
    if (r->isActive())
    {
        kdDebug() << "renderer " << desk << " already active" << endl;
        return;
    }

    r->start();
}


/*
 * This slot is called when the Timeout is executed
 */
void KBackgroundManager::slotCrossFadeTimeout()
{
    KVirtualBGRenderer *r = m_Renderer[fadeDesk];
    if (crossInit) {
        mBenchmark.start();
    }

    if (mAlpha <= 0.0 || mBenchmark.elapsed() > 300 ) {
        mAlpha = 1;
        m_crossTimer->stop();
        KPixmap pixm(mNextScreen);
        setPixmap(&pixm, r->hash(), fadeDesk);
        mOldScreen = TQPixmap();
        return;
    }
    // Reset Timer
    mBenchmark.start();

    TQPixmap dst = crossFade(mOldScreen, mNextScreen, mAlpha, crossInit);
    KPixmap pixm(dst);
    setPixmap(&pixm, r->hash(), fadeDesk);

    mAlpha -=0.03;
    crossInit = false;
}


/*
 * This slot is called when a renderer is done.
 */
void KBackgroundManager::slotImageDone(int desk)
{
    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();
    if (m_numberOfViewports < 1) {
        m_numberOfViewports = 1;
    }

    KPixmap *pm = new KPixmap();
    KVirtualBGRenderer *r = m_Renderer[desk];
    bool do_cleanup = true;
    fadeDesk = desk;
    mAlpha = 1.0;
    int width,height;


    *pm = r->pixmap();
    // If current: paint it
    bool current = (r->hash() == m_Renderer[effectiveDesktop()]->hash());
    if (current)
    {
        //START
        if (m_Renderer[effectiveDesktop()]->renderer(0)->crossFadeBg() && !m_Renderer[effectiveDesktop()]->renderer(0)->usingCrossXml() && !resizingDesktop) {
            int mode = m_Renderer[effectiveDesktop()]->renderer(0)->wallpaperMode();
            width = TQApplication::desktop()->screenGeometry().width();
            height = TQApplication::desktop()->screenGeometry().height();

            if (mode == KBackgroundSettings::NoWallpaper || mode == KBackgroundSettings::Tiled || mode  == KBackgroundSettings::CenterTiled ){
                mNextScreen = TQPixmap(width,height);
                TQPainter p (&mNextScreen);
                p.drawTiledPixmap(0,0,width,height,*pm);
            } else {
                mNextScreen = TQPixmap(*pm);
            }

            const TQPixmap *pPix;
            if (m_pDesktop){
                pPix = m_pDesktop->backgroundPixmap();
            }else{
                pPix = TQApplication::desktop()->screen()->backgroundPixmap();
            }

            //TODO Find a way to discover if CrossFade effect needs to run
            if (pPix){
                mOldScreen = *pPix;
                crossInit = true;
                m_crossTimer->start(70);
            } else{
                setPixmap(pm, r->hash(), desk);
            }
        }else{
            setPixmap(pm, r->hash(), desk);
        }
        //ENDS HERE  */

        if (!m_bBgInitDone)
        {
            m_bBgInitDone = true;
            emit initDone();
            TQTimer::singleShot( 30000, this, TQ_SLOT( saveImages()));
            do_cleanup = false;
        }
    }
    if (m_bExport || !m_bCommon) {
	addCache(pm, r->hash(), desk);
    }
    else {
        delete pm;
    }

    if (current) {
        exportBackground(desk, realDesktop());
    }

    if( do_cleanup ) {
        r->saveCacheFile();
        r->cleanup();
    }

    resizingDesktop = false;
}


void KBackgroundManager::saveImages()
{
    for (unsigned i=0; i<m_Renderer.size(); i++)
    {
        m_Renderer[i]->saveCacheFile();
        m_Renderer[i]->cleanup();
    }
}

/*
 * Size in bytes of a TQPixmap. For use in the pixmap cache.
 */
int KBackgroundManager::pixmapSize(TQPixmap *pm)
{
    return (pm->width() * pm->height()) * ((pm->depth() + 7) / 8);
}


/*
 * Total size of the pixmap cache.
 */
int KBackgroundManager::cacheSize()
{
    int total = 0;
    for (unsigned i=0; i<m_Cache.size(); i++)
    {
        if (m_Cache[i]->pixmap)
            total += pixmapSize(m_Cache[i]->pixmap);
    }
    return total;
}


/*
 * Remove an entry from the pixmap cache.
 */
void KBackgroundManager::removeCache(int desk)
{
    if (m_bExport)
	m_pPixmapServer->remove(KRootPixmap::pixmapName(desk+1));
    else
        delete m_Cache[desk]->pixmap;
    m_Cache[desk]->pixmap = 0L;
    m_Cache[desk]->hash = 0;
    m_Cache[desk]->exp_from = -1;
    m_Cache[desk]->atime = 0;

    // Remove cache entries pointing to the removed entry
    for (unsigned i=0; i<m_Cache.size(); i++)
    {
	if (m_Cache[i]->exp_from == desk)
	{
	    assert(m_bExport);
	    m_Cache[i]->exp_from = -1;
	    m_pPixmapServer->remove(KRootPixmap::pixmapName(i+1));
	}
    }
}


/*
 * Try to free up to size bytes from the cache.
 */
bool KBackgroundManager::freeCache(int size)
{
    if (m_bExport || !m_bLimitCache)
	return true;

    // If it doesn't fit at all, return now.
    if (size > m_CacheLimit)
	return false;

    // If cache is too full, purge it (LRU)
    while (size+cacheSize() > m_CacheLimit)
    {
	int j, min;
	min = m_Serial+1; j = 0;
	for (unsigned i=0; i<m_Cache.size(); i++)
	{
	    if (m_Cache[i]->pixmap && (m_Cache[i]->atime < min))
	    {
		min = m_Cache[i]->atime;
		j = i;
	    }
	}
	removeCache(j);
    }
    return true;
}


/*
 * Try to add a pixmap to the pixmap cache. We don't use TQPixmapCache here
 * because if we're exporting pixmaps, this needs special care.
 */
void KBackgroundManager::addCache(KPixmap *pm, int hash, int desk)
{
    if (m_Cache[desk]->pixmap)
	removeCache(desk);

    if (m_bLimitCache && !m_bExport && !freeCache(pixmapSize(pm)))
    {
	// pixmap does not fit in cache
	delete pm;
	return;
    }

    m_Cache[desk]->pixmap = pm;
    m_Cache[desk]->hash = hash;
    m_Cache[desk]->atime = m_Serial;
    m_Cache[desk]->exp_from = -1;
    exportBackground(desk, desk);
}

/*
 * Called every minute to check if we need to rerun a background program.
 * or change a wallpaper.
 */
void KBackgroundManager::slotTimeout()
{
    TQMemArray<int> running(m_Renderer.size());
    running.fill(0);

    int NumDesks = m_Renderer.size();
    if (m_bCommon)
       NumDesks = 1;

    int edesk = effectiveDesktop();

    for (int i=0; i<NumDesks; i++)
    {
        KVirtualBGRenderer *r = m_Renderer[i];
        bool change = false;
        
        if (r->needProgramUpdate())
        {
            r->programUpdate();
            change = true;
        }
        
        if (r->needWallpaperChange())
        {
            r->changeWallpaper();
            change = true;
        }

        if (change && (i == edesk))
	{
	    running[i] = r->hash();
            r->start();
	}
    }
}

// Return a valid desk number.
int KBackgroundManager::validateDesk(int desk)
{
    if (desk > (int)m_Renderer.size())
	slotChangeNumberOfDesktops( m_pTWinModule->numberOfDesktops() );

    if ( (desk <= 0) || (desk > (int)m_Renderer.size()) )
        return realDesktop();

    return desk - 1;
}

// DCOP exported
// Return current wallpaper for specified desk.
// 0 is for the current visible desktop.
TQString KBackgroundManager::currentWallpaper(int desk)
{
    //TODO Is the behaviour of this function appropriate for multiple screens?
    KCrossBGRender *r = m_Renderer[validateDesk(desk)]->renderer(0);

    return r->currentWallpaper();
}

// DCOP exported
void KBackgroundManager::changeWallpaper()
{
    KVirtualBGRenderer *r = m_Renderer[effectiveDesktop()];

    r->changeWallpaper();
    slotChangeDesktop(0);
}

// DCOP exported
void KBackgroundManager::setExport(int _export)
{
//    kdDebug() << "KBackgroundManager enabling exports.\n";
    bool changed = (_export != m_bExport);
    applyExport(_export);
    if (changed) {
        slotChangeDesktop(0);
    }
}

// DCOP exported
void KBackgroundManager::setCommon(int common)
{
    applyCommon(common);
    KDesktopSettings::setCommonDesktop( m_bCommon );
    KDesktopSettings::writeConfig();
    slotChangeDesktop(0);
}

// DCOP exported
void KBackgroundManager::setWallpaper(TQString wallpaper, int mode)
{
    if (mode < 0 || mode >= KBackgroundSettings::lastWallpaperMode) {
      kdDebug() << "Invalid background mode " << mode << " passed to " << k_funcinfo << "\n";
      return;
    }

    //TODO Is the behaviour of this function appropriate for multiple screens?
    for (unsigned i=0; i < m_Renderer[effectiveDesktop()]->numRenderers(); ++i)
    {
        KCrossBGRender *r = m_Renderer[effectiveDesktop()]->renderer(i);
        r->stop();
        r->setWallpaperMode(mode);
        r->setMultiWallpaperMode(KBackgroundSettings::NoMulti);
        r->setWallpaper(wallpaper);
        r->writeSettings();
    }
    slotChangeDesktop(0);
}

void KBackgroundManager::setWallpaper(TQString wallpaper)
{
    //TODO Is the behaviour of this function appropriate for multiple screens?
    KCrossBGRender *r = m_Renderer[effectiveDesktop()]->renderer(0);
    int mode = r->wallpaperMode();
    if (mode == KBackgroundSettings::NoWallpaper)
       mode = KBackgroundSettings::Tiled;
    setWallpaper(wallpaper, mode);
}

// DCOP exported
// Returns the filenames of all wallpaper entries for specified desk
// 0 is for current visible desktop.
TQStringList KBackgroundManager::wallpaperFiles(int desk)
{
    //TODO Is the behaviour of this function appropriate for multiple screens?
    KCrossBGRender *r = m_Renderer[validateDesk(desk)]->renderer(0);

    return r->wallpaperFiles();
}

// DCOP exported
// Returns the list of wallpaper entries (viewable in background slide
// show window) for specified desk.  0 is for current visible desktop.
TQStringList KBackgroundManager::wallpaperList(int desk)
{
    //TODO Is the behaviour of this function appropriate for multiple screens?
    KCrossBGRender *r = m_Renderer[validateDesk(desk)]->renderer(0);;

    return r->wallpaperList();
}

// DCOP exported
void KBackgroundManager::setCache( int bLimit, int size )
{
    applyCache( bLimit, size*1024 );
    KDesktopSettings::setLimitCache( (bool) bLimit );
    KDesktopSettings::setCacheSize( size );
    KDesktopSettings::writeConfig();
}

// DCOP exported
void KBackgroundManager::setWallpaper(int desk, TQString wallpaper, int mode)
{
    if (mode < 0 || mode >= KBackgroundSettings::lastWallpaperMode) {
      kdDebug() << "Invalid background mode " << mode << " passed to " << k_funcinfo << "\n";
      return;
    }

    int sdesk = validateDesk(desk);

    //TODO Is the behaviour of this function appropriate for multiple screens?
    for (unsigned i=0; i < m_Renderer[sdesk]->numRenderers(); ++i)
    {
        KCrossBGRender *r = m_Renderer[sdesk]->renderer(i);
    
        setCommon(false);   // Force each desktop to have it's own wallpaper
    
        r->stop();
        r->setWallpaperMode(mode);
        r->setMultiWallpaperMode(KBackgroundSettings::NoMulti);
        r->setWallpaper(wallpaper);
        r->writeSettings();
    }
    slotChangeDesktop(sdesk);
}

void KBackgroundManager::repaintBackground()
{
    if (m_pDesktop)
       m_pDesktop->repaint();
    else
        TDEApplication::desktop()->screen()->erase();
}

void KBackgroundManager::desktopResized()
{
    resizingDesktop = true;
    for (unsigned i=0; i<m_Renderer.size(); i++)
    {
        KVirtualBGRenderer * r = m_Renderer[i];
        if( r->isActive())
            r->stop();
        removeCache(i);
        // make the renderer update its desktop size
        r->desktopResized();
        for (unsigned j=0; j<(r->numRenderers()); ++j) {
            r->renderer(j)->desktopResized();
        }
    }

#ifdef COMPOSITE
    if (m_tPixmap)
	delete m_tPixmap;
    m_tPixmap = new KPixmap(tdeApp->desktop()->size());
    m_tPixmap->fill(TQColor(0, 0x0));
#endif
    
    m_Hash = 0;
    if( m_pDesktop ) {
        m_pDesktop->resize( tdeApp->desktop()->geometry().size());
        if (m_Renderer[effectiveDesktop()]->renderer(0)->usingCrossXml()){
            m_Renderer[effectiveDesktop()]->renderer(0)->changeWallpaper();
        }
    }
    // Repaint desktop
    slotChangeDesktop(0);
    repaintBackground();

    // Redraw all desktops so that applications relying on exported data, e.g. kpager, continue to work properly
    TQSize s(m_pTWinModule->numberOfViewports(m_pTWinModule->currentDesktop()));
    m_numberOfViewports = s.width() * s.height();
    if (m_numberOfViewports < 1) {
        m_numberOfViewports = 1;
    }
    for (signed j=0;j<(m_pTWinModule->numberOfDesktops() * m_numberOfViewports);j++) {
        renderBackground(j);
    }
}

// DCOP exported
void KBackgroundManager::setColor(const TQColor & c, bool isColorA)
{
    //TODO Is the behaviour of this function appropriate for multiple screens?
    for (unsigned i=0; i < m_Renderer[effectiveDesktop()]->numRenderers(); ++i)
    {
        KCrossBGRender *r = m_Renderer[effectiveDesktop()]->renderer(i);
        r->stop();
    
        if (isColorA)
            r->setColorA(c);
        else
            r->setColorB(c);
    
        int mode = r->backgroundMode();
        if (mode == KBackgroundSettings::Program)
        mode = KBackgroundSettings::Flat;
    
        if (!isColorA && (mode == KBackgroundSettings::Flat))
        mode = KBackgroundSettings::VerticalGradient;
        r->setBackgroundMode(mode);
    
        r->writeSettings();
    }
    slotChangeDesktop(0);
}

void KBackgroundManager::setBackgroundEnabled( const bool enable )
{
  if (m_bEnabled == enable)
    return;

  m_bEnabled= enable;

  int NumDesks = m_Renderer.size();
  if (m_bCommon)
    NumDesks = 1;

  for (int i=0; i<NumDesks; i++)
    {
      m_Renderer[i]->setEnabled(enable);
    }
  slotChangeDesktop(0);
}

#ifdef COMPOSITE
void KBackgroundManager::slotCmBackgroundChanged( bool )
{
    m_tPixmap->fill(TQColor(0, 0x0));
    m_Hash = 0;
    slotChangeDesktop(0);
}
#endif

#include "bgmanager.moc"
