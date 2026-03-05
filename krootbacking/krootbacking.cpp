/***************************************************************************
 *   Copyright (C) 2011 by Timothy Pearson <kb9vqf@pearsoncomputing.net>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <tqwidget.h>
#include <tqtimer.h>
#include <tqrect.h>
#include <tqimage.h>

#include <tdeapplication.h>
#include <kimageeffect.h>
#include <kpixmapio.h>
#include <twinmodule.h>
#include <twin.h>
#include <kdebug.h>
#include <netwm.h>
#include <dcopclient.h>
#include <dcopref.h>

#include <ksharedpixmap.h>
#include <tdestandarddirs.h>
#include <krootbacking.h>

static TQString wallpaperForDesktop(int desktop)
{
    return DCOPRef("kdesktop", "KBackgroundIface").call("currentWallpaper", desktop);
}

class KRootBackingData
{
public:
    TQWidget *toplevel;
#ifdef TQ_WS_X11
    TWinModule *twin;
#endif
};


KRootBacking::KRootBacking()
    : TQObject(TDEApplication::desktop(), "KRootBacking" ), m_Desk(0), m_timeout(0)
{
    enableExports();

    init();
}

void KRootBacking::init()
{
    d = new KRootBackingData;
    m_Fade = 0;
    m_pPixmap = new TDESharedPixmap; //ordinary KPixmap on win32
    m_pTimer = new TQTimer( this );
    m_bInit = false;
    m_bActive = false;

    connect(tdeApp, TQ_SIGNAL(backgroundChanged(int)), TQ_SLOT(slotBackgroundChanged(int)));
    connect(m_pTimer, TQ_SIGNAL(timeout()), TQ_SLOT(repaint()));
#ifdef TQ_WS_X11
    connect(m_pPixmap, TQ_SIGNAL(done(bool)), TQ_SLOT(slotDone(bool)));

    d->twin = new TWinModule( this );
#endif

    m_bInit = true;
}

KRootBacking::~KRootBacking()
{
    delete m_pPixmap;
    delete d;
}


int KRootBacking::currentDesktop() const
{
#ifdef TQ_WS_X11
    NETRootInfo rinfo( tqt_xdisplay(), NET::CurrentDesktop );
    rinfo.activate();
    return rinfo.currentDesktop();
#endif
}


void KRootBacking::start()
{
    if (m_bActive)
	return;

    m_bActive = true;
    if ( !isAvailable() )
    {
	// We should get a KIPC message when the shared pixmap is available...
	enableExports();
	if (m_timeout < 50) {
		TQTimer::singleShot( 100, this, TQ_SLOT(show()) );	// ...but it doesn't always work!
		m_timeout++;
		return;
	}
    }
    if (m_bInit) {
	repaint(true);
    }
}


void KRootBacking::stop()
{
    m_bActive = false;
    m_pTimer->stop();
}


void KRootBacking::setFadeEffect(double fade, const TQColor &color)
{
    if (fade < 0)
	m_Fade = 0;
    else if (fade > 1)
	m_Fade = 1;
    else
	m_Fade = fade;
    m_FadeColor = color;

    if ( m_bActive && m_bInit ) repaint(true);
}

void KRootBacking::repaint()
{
    repaint(false);
}


void KRootBacking::repaint(bool force)
{
    TQWidget* desktopWidget = TDEApplication::desktop();
    TQPoint p1 = desktopWidget->mapToGlobal(desktopWidget->rect().topLeft());
    TQPoint p2 = desktopWidget->mapToGlobal(desktopWidget->rect().bottomRight());
    if (!force && (m_Rect == TQRect(p1, p2)))
	return;

    m_Rect = TQRect(p1, p2);
#ifdef TQ_WS_X11
    m_Desk = currentDesktop();

    // TDESharedPixmap will correctly generate a tile for us.
    m_pPixmap->loadFromShared(pixmapName(m_Desk), m_Rect);
#else
    m_Desk = currentDesktop();
    // !x11 note: tile is not generated!
    // TODO: pixmapName() is a nonsense now!
    m_pPixmap->load( pixmapName(m_Desk) );
    if (!m_pPixmap->isNull()) {
        m_pPixmap->resize( m_Rect.size() );
        slotDone(true);
    }
#endif
}

bool KRootBacking::isAvailable() const
{
// #ifdef TQ_WS_X11
//     return m_pPixmap->isAvailable(pixmapName(m_Desk));
// #else
    return m_pPixmap->isNull();
// #endif
}

TQString KRootBacking::pixmapName(int desk) {
    TQString pattern = TQString("DESKTOP%1");
#ifdef TQ_WS_X11
    int screen_number = DefaultScreen(tqt_xdisplay());
    if (screen_number) {
        pattern = TQString("SCREEN%1-DESKTOP").arg(screen_number) + "%1";
    }
#endif
    return pattern.arg( desk );
}


void KRootBacking::enableExports()
{
#ifdef TQ_WS_X11
    kdDebug(270) << k_lineinfo << "activating background exports.\n";
    DCOPClient *client = tdeApp->dcopClient();
    if (!client->isAttached())
	client->attach();
    TQByteArray data;
    TQDataStream args( data, IO_WriteOnly );
    args << 1;

    TQCString appname( "kdesktop" );
    int screen_number = DefaultScreen(tqt_xdisplay());
    if ( screen_number )
        appname.sprintf("kdesktop-screen-%d", screen_number );

    client->send( appname, "KBackgroundIface", "setExport(int)", data );
#endif
}


void KRootBacking::slotDone(bool success)
{
    if (!success)
    {
	kdWarning(270) << k_lineinfo << "loading of desktop background failed.\n";
	if (m_timeout < 50) {
		TQTimer::singleShot( 100, this, TQ_SLOT(show()) );
		m_timeout++;
		return;
	}
    }

    // We need to test active as the pixmap might become available
    // after the widget has been destroyed.
    if ( m_bActive )
	updateBackground( m_pPixmap );
}

void KRootBacking::updateBackground( TDESharedPixmap *spm )
{
    TQPixmap pm = *spm;

    if (m_Fade > 1e-6)
    {
	KPixmapIO io;
	TQImage img = io.convertToImage(pm);
	img = KImageEffect::fade(img, m_Fade, m_FadeColor);
	pm = io.convertToPixmap(img);
    }

    TQString filename = getenv("USER");
    filename.prepend("/tmp/tde-");
    filename.append("/krootbacking.png");
    pm.save(filename, "PNG");
    printf("%s\n", filename.ascii()); fflush(stdout);
    exit(0);
}


void KRootBacking::slotBackgroundChanged(int desk)
{
    if (!m_bInit || !m_bActive)
	return;

    if (desk == m_Desk)
	repaint(true);
}

#include "krootbacking.moc"
