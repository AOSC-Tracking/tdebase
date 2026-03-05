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

#include <tqpainter.h>
#include <tqtooltip.h>
#include <tqlineedit.h>
#include <tqpopupmenu.h>
#include <tqlayout.h>
#include <tqbuttongroup.h>

#include <dcopref.h>
#include <tdeglobalsettings.h>
#include <twin.h>
#include <twinmodule.h>
#include <tdeapplication.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <tdeprocess.h>
#include <tdepopupmenu.h>
#include <tdestandarddirs.h>
#include <kiconloader.h>
#include <dcopclient.h>
#include <netwm.h>
#include <kmanagerselection.h>

#include "global.h"
#include "kickertip.h"
#include "kickerSettings.h"
#include "kshadowengine.h"
#include "kshadowsettings.h"
#include "paneldrag.h"
#include "taskmanager.h"

#include "pagerapplet.h"
#include "pagerapplet.moc"

#ifdef FocusOut
#undef FocusOut
#endif

const int knDesktopPreviewSize = 12;
const int knBtnSpacing = 1;

// The previews tend to have a 4/3 aspect ratio
static const int smallHeight = 32;
static const int smallWidth = 42;

// config menu id offsets
static const int rowOffset = 2000;
static const int labelOffset = 200;
static const int bgOffset = 300;

extern "C"
{
    TDE_EXPORT KPanelApplet* init(TQWidget *parent, const TQString& configFile)
    {
      TDEGlobal::locale()->insertCatalogue("kminipagerapplet");
      return new KMiniPager(configFile, KPanelApplet::Normal, 0, parent, "kminipagerapplet");
    }
}

KMiniPager::KMiniPager(const TQString& configFile, Type type, int actions,
                       TQWidget *parent, const char *name)
    : KPanelApplet( configFile, type, actions, parent, name ),
      m_layout(0),
      m_desktopLayoutOwner( NULL ),
      m_shadowEngine(0),
      m_contextMenu(0),
      m_settings( new PagerSettings(sharedConfig()) )
{
    setBackgroundOrigin( AncestorOrigin );
    int scnum = TQApplication::desktop()->screenNumber(this);
    TQRect desk = TQApplication::desktop()->screenGeometry(scnum);
    if (desk.width() <= 800)
    {
        TDEConfigSkeleton::ItemBool* item = dynamic_cast<TDEConfigSkeleton::ItemBool*>(m_settings->findItem("Preview"));
        if (item)
        {
            item->setDefaultValue(false);
        }
    }
    m_settings->readConfig();
    m_windows.setAutoDelete(true);
    if (m_settings->preview())
    {
        TaskManager::the()->trackGeometry();
    }

    m_group = new TQButtonGroup(this);
    m_group->setBackgroundOrigin(AncestorOrigin);
    m_group->setFrameStyle(TQFrame::NoFrame);
    m_group->setExclusive( true );

    setFont( TDEGlobalSettings::taskbarFont() );

    m_twin = new TWinModule(this);
    m_activeWindow = m_twin->activeWindow();
    m_curDesk = m_twin->currentDesktop();

    if (m_curDesk == 0) // twin not yet launched
    {
        m_curDesk = 1;
    }

    desktopLayoutOrientation = TQt::Horizontal;
    desktopLayoutX = -1;
    desktopLayoutY = -1;

    TQSize s(m_twin->numberOfViewports(m_twin->currentDesktop()));
    m_useViewports = s.width() * s.height() > 1;

    drawButtons();

    connect( m_twin, TQ_SIGNAL( currentDesktopChanged(int)), TQ_SLOT( slotSetDesktop(int) ) );
    connect( m_twin, TQ_SIGNAL( currentDesktopViewportChanged(int, const TQPoint&)), TQ_SLOT(slotSetDesktopViewport(int, const TQPoint&)));
    connect( m_twin, TQ_SIGNAL( numberOfDesktopsChanged(int)), TQ_SLOT( slotSetDesktopCount(int) ) );
    connect( m_twin, TQ_SIGNAL( desktopGeometryChanged(int)), TQ_SLOT( slotRefreshViewportCount(int) ) );
    connect( m_twin, TQ_SIGNAL( activeWindowChanged(WId)), TQ_SLOT( slotActiveWindowChanged(WId) ) );
    connect( m_twin, TQ_SIGNAL( windowAdded(WId) ), this, TQ_SLOT( slotWindowAdded(WId) ) );
    connect( m_twin, TQ_SIGNAL( windowRemoved(WId) ), this, TQ_SLOT( slotWindowRemoved(WId) ) );
    connect( m_twin, TQ_SIGNAL( windowChanged(WId,unsigned int) ), this, TQ_SLOT( slotWindowChanged(WId,unsigned int) ) );
    connect( m_twin, TQ_SIGNAL( desktopNamesChanged() ), this, TQ_SLOT( slotDesktopNamesChanged() ) );
    connect( tdeApp, TQ_SIGNAL(backgroundChanged(int)), TQ_SLOT(slotBackgroundChanged(int)) );

    if (tdeApp->authorizeTDEAction("kicker_rmb") && tdeApp->authorizeControlModule("tde-kcmtaskbar.desktop"))
    {
        m_contextMenu = new TQPopupMenu();
        connect(m_contextMenu, TQ_SIGNAL(aboutToShow()), TQ_SLOT(aboutToShowContextMenu()));
        connect(m_contextMenu, TQ_SIGNAL(activated(int)), TQ_SLOT(contextMenuActivated(int)));
        setCustomMenu(m_contextMenu);
    }

    TQValueList<WId>::ConstIterator it;
    TQValueList<WId>::ConstIterator itEnd = m_twin->windows().end();
    for ( it = m_twin->windows().begin(); it != itEnd; ++it)
    {
        slotWindowAdded( (*it) );
    }

    slotSetDesktop( m_curDesk );
    updateLayout();
}

KMiniPager::~KMiniPager()
{
    TDEGlobal::locale()->removeCatalogue("kminipagerapplet");
    delete m_contextMenu;
    delete m_settings;
    delete m_shadowEngine;
}

void KMiniPager::slotBackgroundChanged(int desk)
{
    unsigned numDesktops = m_twin->numberOfDesktops();
    if (numDesktops != m_desktops.count())
    {
        slotSetDesktopCount(numDesktops);
    }

    if (desk < 1 || (unsigned) desk > m_desktops.count())
    {
        // should not happen, but better to be paranoid than crash
        return;
    }

    m_desktops[desk - 1]->backgroundChanged();
}

void KMiniPager::slotSetDesktop(int desktop)
{
    if (m_twin->numberOfDesktops() > static_cast<int>(m_desktops.count()))
    {
        slotSetDesktopCount( m_twin->numberOfDesktops() );
    }

    if (!m_useViewports && (desktop != KWin::currentDesktop()))
    {
        // this can happen when the user clicks on a desktop,
        // holds down the key combo to switch desktops, lets the
        // mouse go but keeps the key combo held. the desktop will switch
        // back to the desktop associated with the key combo and then it
        // becomes a race condition between twin's signal and the button's
        // signal. usually twin wins.
        return;
    }

    m_curDesk = desktop;
    if (m_curDesk < 1)
    {
        m_curDesk = 1;
    }

    KMiniPagerButton* button = m_desktops[m_curDesk - 1];
    if (!button->isOn())
    {
        button->toggle();
    }
}

void KMiniPager::slotSetDesktopViewport(int desktop, const TQPoint& viewport)
{
    // ###
    Q_UNUSED(desktop);
    TQSize s(m_twin->numberOfViewports(m_twin->currentDesktop()));
    slotSetDesktop((viewport.y()-1) * s.width() + viewport.x() );
}

void KMiniPager::slotButtonSelected( int desk )
{
    if (m_twin->numberOfViewports(m_twin->currentDesktop()).width() *
        m_twin->numberOfViewports(m_twin->currentDesktop()).height() > 1)
    {
        TQPoint p;

        p.setX( (desk-1) * TQApplication::desktop()->width());
        p.setY( 0 );

        KWin::setCurrentDesktopViewport(m_twin->currentDesktop(), p);
    }
    else
        KWin::setCurrentDesktop( desk );

    slotSetDesktop( desk );
}

int KMiniPager::widthForHeight(int h) const
{
    if (orientation() == TQt::Vertical)
    {
        return width();
    }

    int deskNum = m_twin->numberOfDesktops() * m_twin->numberOfViewports(0).width()
        * m_twin->numberOfViewports(0).height();

    int rowNum = m_settings->numberOfRows();
    if (rowNum == 0)
    {
        if (h <= 32 || deskNum <= 1)
        {
            rowNum = 1;
        }
        else
        {
            rowNum = 2;
        }
    }

    int deskCols = deskNum/rowNum;
    if(deskNum == 0 || deskNum % rowNum != 0)
        deskCols += 1;

    int bw = (h / rowNum);
    if( m_settings->labelType() != PagerSettings::EnumLabelType::LabelName )
    {
        if (desktopPreview() || m_settings->backgroundType() == PagerSettings::EnumBackgroundType::BgLive)
        {
            bw = (int) ( bw * (double) TQApplication::desktop()->width() / TQApplication::desktop()->height() );
        }
    }
    else
    {
        // scale to desktop width as a minimum
        bw = (int) (bw * (double) TQApplication::desktop()->width() / TQApplication::desktop()->height());
        TQFontMetrics fm = fontMetrics();
        for (int i = 1; i <= deskNum; i++)
        {
            int sw = fm.width( m_twin->desktopName( i ) ) + 8;
            if (sw > bw)
            {
                bw = sw;
            }
        }
    }

    // we add one to the width for the spacing in between the buttons
    // however, the last button doesn't have a space on the end of it (it's
    // only _between_ buttons) so we then remove that one pixel
    return (deskCols * (bw + 1)) - 1;
}

int KMiniPager::heightForWidth(int w) const
{
    if (orientation() == TQt::Horizontal)
    {
        return height();
    }

    int deskNum = m_twin->numberOfDesktops() * m_twin->numberOfViewports(0).width()
        * m_twin->numberOfViewports(0).height();
    int rowNum = m_settings->numberOfRows(); // actually these are columns now... oh well.
    if (rowNum == 0)
    {
        if (w <= 48 || deskNum == 1)
        {
            rowNum = 1;
        }
        else
        {
            rowNum = 2;
        }
    }

    int deskCols = deskNum/rowNum;
    if(deskNum == 0 || deskNum % rowNum != 0)
    {
        deskCols += 1;
    }

    int bh = (w/rowNum) + 1;
    if ( desktopPreview() )
    {
        bh = (int) ( bh *  (double) TQApplication::desktop()->height() / TQApplication::desktop()->width() );
    }
    else if ( m_settings->labelType() == PagerSettings::EnumLabelType::LabelName )
    {
       bh = fontMetrics().lineSpacing() + 8;
    }

    // we add one to the width for the spacing in between the buttons
    // however, the last button doesn't have a space on the end of it (it's
    // only _between_ buttons) so we then remove that one pixel
    int nHg = (deskCols * (bh + 1)) - 1;

    return nHg;
}

void KMiniPager::updateDesktopLayout(int o, int x, int y)
{
    if ((desktopLayoutOrientation == o) &&
       (desktopLayoutX == x) &&
       (desktopLayoutY == y))
    {
      return;
    }

    desktopLayoutOrientation = o;
    desktopLayoutX = x;
    desktopLayoutY = y;
    if( x == -1 ) // do-the-maths-yourself is encoded as 0 in the wm spec
        x = 0;
    if( y == -1 )
        y = 0;
    if( m_desktopLayoutOwner == NULL )
    { // must own manager selection before setting global desktop layout
        int screen = DefaultScreen( tqt_xdisplay());
        m_desktopLayoutOwner = new TDESelectionOwner( TQString( "_NET_DESKTOP_LAYOUT_S%1" ).arg( screen ).latin1(),
            screen, this );
        if( !m_desktopLayoutOwner->claim( false ))
        {
            delete m_desktopLayoutOwner;
            m_desktopLayoutOwner = NULL;
            return;
        }
    }
    NET::Orientation orient = o == TQt::Horizontal ? NET::OrientationHorizontal : NET::OrientationVertical;
    NETRootInfo i( tqt_xdisplay(), 0 );
    i.setDesktopLayout( orient, x, y, NET::DesktopLayoutCornerTopLeft );
}

void KMiniPager::resizeEvent(TQResizeEvent*)
{
    bool horiz = orientation() == TQt::Horizontal;

    int deskNum = m_desktops.count();
    int rowNum = m_settings->numberOfRows();
    if (rowNum == 0)
    {
        if (((horiz && height()<=32)||(!horiz && width()<=48)) || deskNum <= 1)
            rowNum = 1;
        else
            rowNum = 2;
    }

    int deskCols = deskNum/rowNum;
    if(deskNum == 0 || deskNum % rowNum != 0)
        deskCols += 1;

    if (m_layout)
    {
        delete m_layout;
        m_layout = 0;
    }

    int nDX, nDY;
    if (horiz)
    {
        nDX = rowNum;
        nDY = deskCols;
        updateDesktopLayout(TQt::Horizontal, -1, nDX);
    }
    else
    {
        nDX = deskCols;
        nDY = rowNum;
        updateDesktopLayout(TQt::Horizontal, nDY, -1);
    }

    // 1 pixel spacing.
    m_layout = new TQGridLayout(this, nDX, nDY, 0, 1);

    TQValueList<KMiniPagerButton*>::Iterator it = m_desktops.begin();
    TQValueList<KMiniPagerButton*>::Iterator itEnd = m_desktops.end();
    int c = 0,
        r = 0;
    while( it != itEnd ) {
        c = 0;
        while( (it != itEnd) && (c < nDY) ) {
            m_layout->addWidget( *it, r, c );
            ++it;
            ++c;
        }
        ++r;
    }

    m_layout->activate();
    updateGeometry();
}

void KMiniPager::wheelEvent( TQWheelEvent* e )
{
    int newDesk;
    int desktops = KWin::numberOfDesktops();


    if(cycleWindow()){

    if (m_twin->numberOfViewports(0).width() * m_twin->numberOfViewports(0).height() > 1 )
        desktops = m_twin->numberOfViewports(0).width() * m_twin->numberOfViewports(0).height();
    if (e->delta() < 0)
    {
        newDesk = m_curDesk % desktops + 1;
    }
    else
    {
        newDesk = (desktops + m_curDesk - 2) % desktops + 1;
    }

    slotButtonSelected(newDesk);
    }
}

void KMiniPager::drawButtons()
{
    int deskNum = m_twin->numberOfDesktops();
    KMiniPagerButton *desk;

    int count = 1;
    int i = 1;
    do
    {
        TQSize viewportNum = m_twin->numberOfViewports(i);
        for (int j = 1; j <= viewportNum.width() * viewportNum.height(); ++j)
        {
            TQSize s(m_twin->numberOfViewports(m_twin->currentDesktop()));
            TQPoint viewport( (j-1) % s.width(), (j-1) / s.width());
            desk = new KMiniPagerButton( count, m_useViewports, viewport, this );
            if ( m_settings->labelType() != PagerSettings::EnumLabelType::LabelName )
            {
                TQToolTip::add( desk, desk->desktopName() );
            }

            m_desktops.append( desk );
            m_group->insert( desk, count );

            connect(desk, TQ_SIGNAL(buttonSelected(int)),
                    TQ_SLOT(slotButtonSelected(int)) );
            connect(desk, TQ_SIGNAL(showMenu(const TQPoint&, int )),
                    TQ_SLOT(slotShowMenu(const TQPoint&, int )) );

            desk->show();
            ++count;
        }
    }
    while ( ++i <= deskNum );
}

void KMiniPager::slotSetDesktopCount( int )
{
    TQSize s(m_twin->numberOfViewports(m_twin->currentDesktop()));
    m_useViewports = s.width() * s.height() > 1;

    TQValueList<KMiniPagerButton*>::ConstIterator it;
    TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
    for( it = m_desktops.begin(); it != itEnd; ++it )
    {
        delete (*it);
    }
    m_desktops.clear();

    drawButtons();

    m_curDesk = m_twin->currentDesktop();
    if ( m_curDesk == 0 )
    {
        m_curDesk = 1;
    }

    resizeEvent(0);
    updateLayout();
}

void KMiniPager::slotRefreshViewportCount( int )
{
    TQSize s(m_twin->numberOfViewports(m_twin->currentDesktop()));
    m_useViewports = s.width() * s.height() > 1;

    TQValueList<KMiniPagerButton*>::ConstIterator it;
    TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
    for( it = m_desktops.begin(); it != itEnd; ++it )
    {
        delete (*it);
    }
    m_desktops.clear();

    drawButtons();

    m_curDesk = m_twin->currentDesktop();
    if ( m_curDesk == 0 )
    {
        m_curDesk = 1;
    }

    resizeEvent(0);
    updateLayout();
}

void KMiniPager::slotActiveWindowChanged( WId win )
{
    if (desktopPreview())
    {
        KWin::WindowInfo* inf1 = m_activeWindow ? info( m_activeWindow ) : NULL;
        KWin::WindowInfo* inf2 = win ? info( win ) : NULL;
        m_activeWindow = win;

        TQValueList<KMiniPagerButton*>::ConstIterator it;
        TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
        for ( it = m_desktops.begin(); it != itEnd; ++it)
        {
            if ( ( inf1 && (*it)->shouldPaintWindow(inf1)) ||
                 ( inf2 && (*it)->shouldPaintWindow(inf2)) )
            {
                (*it)->windowsChanged();
            }
        }
    }
}

void KMiniPager::slotWindowAdded( WId win)
{
    if (desktopPreview())
    {
        KWin::WindowInfo* inf = info( win );

        if (inf->state() & NET::SkipPager)
        {
            return;
        }

        TQValueList<KMiniPagerButton*>::ConstIterator it;
        TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
        for ( it = m_desktops.begin(); it != itEnd; ++it)
        {
            if ( (*it)->shouldPaintWindow(inf) )
            {
                (*it)->windowsChanged();
            }
        }
    }
}

void KMiniPager::slotWindowRemoved(WId win)
{
    if (desktopPreview())
    {
        KWin::WindowInfo* inf = info(win);
        bool onAllDesktops = inf->onAllDesktops();
        bool onAllViewports = inf->hasState(NET::Sticky);
        bool skipPager = inf->state() & NET::SkipPager;
        int desktop = inf->desktop();

        if (win == m_activeWindow)
            m_activeWindow = 0;

        m_windows.remove((long) win);

        if (skipPager)
        {
            return;
        }

        TQValueList<KMiniPagerButton*>::ConstIterator it;
        TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
        for (it = m_desktops.begin(); it != itEnd; ++it)
        {
            if (onAllDesktops || onAllViewports || desktop == (*it)->desktop())
            {
                (*it)->windowsChanged();
            }
        }
    }
    else
    {
        m_windows.remove(win);
        return;
    }

}

void KMiniPager::slotWindowChanged( WId win , unsigned int properties )
{
    if ((properties & (NET::WMState | NET::XAWMState | NET::WMDesktop)) == 0 &&
        (!desktopPreview() || (properties & NET::WMGeometry) == 0) &&
        !(desktopPreview() && windowIcons() &&
         (properties & NET::WMIcon | NET::WMIconName | NET::WMVisibleIconName) == 0))
    {
        return;
    }

    if (desktopPreview())
    {
        KWin::WindowInfo* inf = m_windows[win];
        bool skipPager = inf->hasState(NET::SkipPager);
        TQMemArray<bool> old_shouldPaintWindow(m_desktops.size());
        TQValueList<KMiniPagerButton*>::ConstIterator it;
        TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
        int i = 0;
        for ( it = m_desktops.begin(); it != itEnd; ++it)
        {
            old_shouldPaintWindow[i++] = (*it)->shouldPaintWindow(inf);
        }

        m_windows.remove(win);
        inf = info(win);

        if (inf->hasState(NET::SkipPager) || skipPager)
        {
            return;
        }

        for ( i = 0, it = m_desktops.begin(); it != itEnd; ++it)
        {
            if ( old_shouldPaintWindow[i++] || (*it)->shouldPaintWindow(inf))
            {
                (*it)->windowsChanged();
            }
        }
    }
    else
    {
        m_windows.remove(win);
        return;
    }
}

KWin::WindowInfo* KMiniPager::info( WId win )
{
    if (!m_windows[win])
    {
        KWin::WindowInfo* info = new KWin::WindowInfo( win,
            NET::WMWindowType | NET::WMState | NET::XAWMState | NET::WMDesktop | NET::WMGeometry | NET::WMKDEFrameStrut, 0 );

        m_windows.insert(win, info);
        return info;
    }

    return m_windows[win];
}

KTextShadowEngine* KMiniPager::shadowEngine()
{
    if (!m_shadowEngine)
        m_shadowEngine = new KTextShadowEngine();

    return m_shadowEngine;
}

void KMiniPager::refresh()
{
    TQValueList<KMiniPagerButton*>::ConstIterator it;
    TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
    for ( it = m_desktops.begin(); it != itEnd; ++it)
    {
        (*it)->update();
    }
}

void KMiniPager::aboutToShowContextMenu()
{
    m_contextMenu->clear();

    m_contextMenu->insertItem(SmallIcon("kpager"), i18n("&Launch Pager"), LaunchExtPager);
    m_contextMenu->insertSeparator();

    m_contextMenu->insertItem(i18n("&Rename Desktop \"%1\"")
                                   .arg(twin()->desktopName(m_rmbDesk)), RenameDesktop);
    m_contextMenu->insertSeparator();

    TDEPopupMenu* showMenu = new TDEPopupMenu(m_contextMenu);
    showMenu->setCheckable(true);
    showMenu->insertTitle(i18n("Pager Layout"));

    TQPopupMenu* rowMenu = new TQPopupMenu(showMenu);
    rowMenu->setCheckable(true);
    rowMenu->insertItem(i18n("&Automatic"), 0 + rowOffset);
    rowMenu->insertItem(i18n("one row or column", "&1"), 1 + rowOffset);
    rowMenu->insertItem(i18n("two rows or columns", "&2"), 2 + rowOffset);
    rowMenu->insertItem( i18n("three rows or columns", "&3"), 3 + rowOffset);
    connect(rowMenu, TQ_SIGNAL(activated(int)), TQ_SLOT(contextMenuActivated(int)));
    showMenu->insertItem((orientation()==TQt::Horizontal) ? i18n("&Rows"):
                                                       i18n("&Columns"),
                         rowMenu);

    showMenu->insertItem(i18n("&Window Thumbnails"), WindowThumbnails);
    showMenu->insertItem(i18n("&Window Icons"), WindowIcons);
    showMenu->insertItem(i18n("&Cycle on Wheel"), Cycle);
    showMenu->insertItem(i18n("3&D Desk Borders"), Border3D);

    showMenu->insertTitle(i18n("Text Label"));
    showMenu->insertItem(i18n("Desktop N&umber"),
                         PagerSettings::EnumLabelType::LabelNumber + labelOffset);
    showMenu->insertItem(i18n("Desktop N&ame"),
                         PagerSettings::EnumLabelType::LabelName + labelOffset);
    showMenu->insertItem(i18n("N&o Label"),
                         PagerSettings::EnumLabelType::LabelNone + labelOffset);

    showMenu->insertTitle(i18n("Background"));
    showMenu->insertItem(i18n("&Elegant"),
                         PagerSettings::EnumBackgroundType::BgPlain + bgOffset);
    showMenu->insertItem(i18n("&Transparent"),
                         PagerSettings::EnumBackgroundType::BgTransparent + bgOffset);
    if (m_useViewports == false) {
        showMenu->insertItem(i18n("&Desktop Wallpaper"),
                         PagerSettings::EnumBackgroundType::BgLive + bgOffset);
    }
    connect(showMenu, TQ_SIGNAL(activated(int)), TQ_SLOT(contextMenuActivated(int)));
    m_contextMenu->insertItem(i18n("&Pager Options"),showMenu);

    m_contextMenu->insertItem(SmallIcon("configure"),
                              i18n("&Configure Desktops..."),
                              ConfigureDesktops);

    rowMenu->setItemChecked(m_settings->numberOfRows() + rowOffset, true);
    m_contextMenu->setItemChecked(m_settings->labelType() + labelOffset, showMenu);
    m_contextMenu->setItemChecked(m_settings->backgroundType() + bgOffset, showMenu);

    m_contextMenu->setItemChecked(WindowThumbnails, m_settings->preview());
    m_contextMenu->setItemChecked(WindowIcons, m_settings->icons());
    m_contextMenu->setItemChecked(Cycle, m_settings->cycle());
    m_contextMenu->setItemChecked(Border3D, m_settings->border3D());
    m_contextMenu->setItemEnabled(WindowIcons, m_settings->preview());
}

void KMiniPager::slotShowMenu(const TQPoint& pos, int desktop)
{
    if (!m_contextMenu)
    {
        return;
    }

    m_rmbDesk = desktop;
    m_contextMenu->exec(pos);
    m_rmbDesk = -1;
}

void KMiniPager::contextMenuActivated(int result)
{
    if (result < 1)
    {
        return;
    }

    switch (result)
    {
        case LaunchExtPager:
            showPager();
            return;

        case ConfigureDesktops:
            tdeApp->startServiceByDesktopName("desktop");
            return;

        case RenameDesktop:
            m_desktops[(m_rmbDesk == -1) ? m_curDesk - 1 : m_rmbDesk - 1]->rename();
            return;
    }

    if (result >= rowOffset)
    {
        m_settings->setNumberOfRows(result - rowOffset);
        resizeEvent(0);
    }

    switch (result)
    {
        case WindowThumbnails:
            m_settings->setPreview(!m_settings->preview());
            TaskManager::the()->trackGeometry();
            break;
        case Cycle:
             m_settings->setCycle(!m_settings->cycle());
            break;
        case Border3D:
            m_settings->setBorder3D(!m_settings->border3D());
            break;
        case WindowIcons:
            m_settings->setIcons(!m_settings->icons());
            break;
        case PagerSettings::EnumBackgroundType::BgPlain + bgOffset:
            m_settings->setBackgroundType(PagerSettings::EnumBackgroundType::BgPlain);
            break;
        case PagerSettings::EnumBackgroundType::BgTransparent + bgOffset:
            m_settings->setBackgroundType(PagerSettings::EnumBackgroundType::BgTransparent);
            break;
        case PagerSettings::EnumBackgroundType::BgLive + bgOffset:
        {
           if (m_useViewports == false) {
                m_settings->setBackgroundType(PagerSettings::EnumBackgroundType::BgLive);
                TQValueList<KMiniPagerButton*>::ConstIterator it;
                TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();
                for( it = m_desktops.begin(); it != itEnd; ++it )
                {
                    (*it)->backgroundChanged();
                }
           }
           else {
               m_settings->setBackgroundType(PagerSettings::EnumBackgroundType::BgTransparent);
           }
            break;
        }

        case PagerSettings::EnumLabelType::LabelNone + labelOffset:
            m_settings->setLabelType(PagerSettings::EnumLabelType::LabelNone);
            break;
        case PagerSettings::EnumLabelType::LabelNumber + labelOffset:
            m_settings->setLabelType(PagerSettings::EnumLabelType::LabelNumber);
            break;
        case PagerSettings::EnumLabelType::LabelName + labelOffset:
            m_settings->setLabelType(PagerSettings::EnumLabelType::LabelName);
            break;
    }

    m_settings->writeConfig();
    updateGeometry();
    refresh();
}

void KMiniPager::slotDesktopNamesChanged()
{
    TQValueList<KMiniPagerButton*>::ConstIterator it = m_desktops.begin();
    TQValueList<KMiniPagerButton*>::ConstIterator itEnd = m_desktops.end();

    for (int i = 1; it != itEnd; ++it, ++i)
    {
        TQString name = m_twin->desktopName(i);
        (*it)->setDesktopName(name);
        (*it)->repaint();
        TQToolTip::remove((*it));
        TQToolTip::add((*it), name);
    }

    updateLayout();
}

void KMiniPager::showPager()
{
    DCOPClient *dcop=tdeApp->dcopClient();

    if (dcop->isApplicationRegistered("kpager"))
    {
       showKPager(true);
    }
    else
    {
    // Let's run kpager if it isn't running
        connect( dcop, TQ_SIGNAL( applicationRegistered(const TQCString &) ), this, TQ_SLOT(applicationRegistered(const TQCString &)) );
        dcop->setNotifications(true);
        TQString strAppPath(locate("exe", "kpager"));
        if (!strAppPath.isEmpty())
        {
            TDEProcess process;
            process << strAppPath;
            process << "--hidden";
            process.start(TDEProcess::DontCare);
        }
    }
}

void KMiniPager::showKPager(bool toggleShow)
{
    TQPoint pt;
    switch ( position() )
    {
        case pTop:
            pt = mapToGlobal( TQPoint(x(), y() + height()) );
            break;
        case pLeft:
            pt = mapToGlobal( TQPoint(x() + width(), y()) );
            break;
        case pRight:
        case pBottom:
        default:
            pt=mapToGlobal( TQPoint(x(), y()) );
    }

    DCOPClient *dcop=tdeApp->dcopClient();

    TQByteArray data;
    TQDataStream arg(data, IO_WriteOnly);
    arg << pt.x() << pt.y() ;
    if (toggleShow)
    {
        dcop->send("kpager", "KPagerIface", "toggleShow(int,int)", data);
    }
    else
    {
        dcop->send("kpager", "KPagerIface", "showAt(int,int)", data);
    }
}

void KMiniPager::applicationRegistered( const TQCString  & appName )
{
    if (appName == "kpager")
    {
        disconnect( tdeApp->dcopClient(), TQ_SIGNAL( applicationRegistered(const TQCString &) ),
                    this, TQ_SLOT(applicationRegistered(const TQCString &)) );
        showKPager(false);
    }
}

