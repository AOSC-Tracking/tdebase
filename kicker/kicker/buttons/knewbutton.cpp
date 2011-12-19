/*****************************************************************

Copyright (c) 2006 Stephan Binner <binner@kde.org>
                   Stephan Kulow <coolo@kde.org>
                   Dirk Mueller <mueller@kde.org>

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

#include <cassert>
#include <cmath>

#include <tqtooltip.h>
#include <tqpainter.h>
#include <tqcursor.h>
#include <tqeffects_p.h>

#include <klocale.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kdebug.h>

#include "kickerSettings.h"

#include "config.h"
#include "global.h"

#include "menumanager.h"
#include "k_mnu_stub.h"
#include "k_new_mnu.h"

#include "knewbutton.h"
#include "knewbutton.moc"

KNewButton *KNewButton::m_self = 0;

KNewButton::KNewButton( TQWidget* parent )
    : KButton( parent ),
      m_oldPos(0,0)
{
    Q_ASSERT( !m_self );
    m_self = this;
    m_hoverTimer = -1;
    m_openTimer = -1;
    m_active = false;
    m_mouseInside = false;
    m_drag = false;

    setIconAlignment((TQ_Alignment)(AlignTop|AlignRight));
    setAcceptDrops(true);
    setIcon("kmenu-suse");
    setDrawArrow(false);

    m_movie = new TQMovie(locate("data", "kicker/pics/kmenu_basic.mng"));
    m_movie->connectUpdate(this, TQT_SLOT(updateMovie()));
    m_movie->connectStatus(TQT_TQOBJECT(this), TQT_SLOT(slotStatus(int)));
    m_movie->connectResize(this, TQT_SLOT(slotSetSize(const TQSize&)));

    TQApplication::desktop()->screen()->installEventFilter(this);
    setMouseTracking(true);
}

KNewButton::~KNewButton()
{
    if ( m_self == this )
        m_self = 0;
    setMouseTracking(false);
    delete m_movie;
}

void KNewButton::slotStatus(int status)
{
    if(status == TQMovie::EndOfLoop)
        slotStopAnimation();
}

TQColor KNewButton::borderColor() const
{
    TQImage img = m_active_pixmap.convertToImage();

    for (int i = 0; i < img.width(); ++i) {
        QRgb rgb = img.pixel(orientation() == Qt::Horizontal ? img.width() - i - 1 :
                    i, 2);

        if (tqGreen(rgb) > 0x50)
            return rgb;
    }

    return img.pixel( orientation() == Qt::Horizontal ? img.width() - 2 : 2, 2);
}

void KNewButton::show()
{
     KButton::show();

     if (KickerSettings::firstRun()) {
         TQTimer::singleShot(500,this,TQT_SLOT(slotExecMenu()));
         KickerSettings::setFirstRun(false);
         KickerSettings::writeConfig();
     }
}

void KNewButton::updateMovie()
{
    m_oldPos = TQPoint( -1, -1 );
    drawEye();

    if (!m_active && m_movie->running())
        m_movie->pause();
}

void KNewButton::setPopupDirection(KPanelApplet::Direction d)
{
    KButton::setPopupDirection(d);

    delete m_movie;

    switch (d) {
    case KPanelApplet::Left:
        setIconAlignment((TQ_Alignment)(AlignTop|AlignLeft));
        m_movie = new TQMovie(locate("data", "kicker/pics/kmenu_vertical.mng"));
        break;
    case KPanelApplet::Right:
        setIconAlignment((TQ_Alignment)(AlignTop|AlignRight));
        m_movie = new TQMovie(locate("data", "kicker/pics/kmenu_vertical.mng"));
        break;
    case KPanelApplet::Up:
        setIconAlignment((TQ_Alignment)(AlignTop|AlignHCenter));
        m_movie = new TQMovie(locate("data", "kicker/pics/kmenu_basic.mng"));
        break;
    case KPanelApplet::Down:
        setIconAlignment((TQ_Alignment)(AlignBottom|AlignHCenter));
        m_movie = new TQMovie(locate("data", "kicker/pics/kmenu_flipped.mng"));
    }

    m_movie->connectUpdate(this, TQT_SLOT(updateMovie()));
    m_movie->connectStatus(TQT_TQOBJECT(this), TQT_SLOT(slotStatus(int)));
    m_movie->connectResize(this, TQT_SLOT(slotSetSize(const TQSize&)));
}

void KNewButton::slotSetSize(const TQSize& s)
{
    m_iconSize = s;
}

double KNewButton::buttonScaleFactor(const TQSize& s) const
{
    double sf = 1.0;

    switch (popupDirection()) {
        case KPanelApplet::Left:
        case KPanelApplet::Right:
//            sf = kMin(double(s.width()) / m_iconSize.height(), double(s.height()) / m_iconSize.width());
//            break;
        case KPanelApplet::Up:
        case KPanelApplet::Down:
            sf = kMin(double(s.width()) / m_iconSize.width(), double(s.height()) / m_iconSize.height());
            break;
    }

    if (sf > 0.8) sf = 1.0;
    return sf;
}

int KNewButton::widthForHeight(int height) const
{
    int r = m_iconSize.width() * buttonScaleFactor(TQSize(m_iconSize.width(), height));

    if (!m_movie->running() && height != m_active_pixmap.height())
    {
        KNewButton* that = const_cast<KNewButton*>(this);
        TQTimer::singleShot(0, that, TQT_SLOT(slotStopAnimation()));
    }
 
    return r;
}

int KNewButton::preferredDimension(int panelDim) const
{
    return kMax(m_icon.width(), m_icon.height());
}

int KNewButton::heightForWidth(int width) const
{
    int r = m_iconSize.width() * buttonScaleFactor(TQSize(width, m_iconSize.height()));
    if (!m_movie->running() && width != m_active_pixmap.width())
    {
        KNewButton* that = const_cast<KNewButton*>(this);
        TQTimer::singleShot(0, that, TQT_SLOT(slotStopAnimation()));
    }
    return r;
}

bool KNewButton::eventFilter(TQObject *o, TQEvent *e)
{
    if (e->type() == TQEvent::MouseButtonRelease ||
        e->type() == TQEvent::MouseButtonPress   ||
        e->type() == TQEvent::MouseButtonDblClick )
    {
        TQMouseEvent *me = TQT_TQMOUSEEVENT(e);
        if (TQT_TQRECT_OBJECT(rect()).contains(mapFromGlobal(me->globalPos())))
        {
            if (m_pressedDuringPopup && m_popup && m_openTimer != -1
                    && (me->button() & Qt::LeftButton) )
                return true;
        }
    }

    if (KickerSettings::kickoffDrawGeekoEye() && e->type() == TQEvent::MouseMove)
    {
        TQMouseEvent *me = TQT_TQMOUSEEVENT(e);
        if ((me->state() & Qt::MouseButtonMask) == Qt::NoButton) 
            drawEye();
    }

    return KButton::eventFilter(o, e);
}

void KNewButton::drawEye()
{
#define eye_x 62
#define eye_y 13
    TQPoint mouse = TQCursor::pos();
    TQPoint me = mapToGlobal(TQPoint(eye_x, eye_y));
    double a = atan2(mouse.y() - me.y(), mouse.x() - me.x());
    int dx = int(2.1 * cos(a));
    int dy = int(2.1 * sin(a));

    TQPoint newpos(eye_x+dx,eye_y+dy);
    if (newpos!=m_oldPos) {
        m_oldPos = newpos;
        TQPixmap pixmap = m_active_pixmap;

        double sf = 1.0;

        if(!m_movie->framePixmap().isNull())
        {
            pixmap = m_movie->framePixmap();
            pixmap.detach();
            m_iconSize = pixmap.size();
            sf = buttonScaleFactor(size());

            if (KickerSettings::kickoffDrawGeekoEye()) {
               TQPainter p(&pixmap);
               p.setPen(white);
               p.setBrush(white);
               //      p.setPen(TQColor(110,185,55));
               p.drawRect(eye_x+dx, eye_y+dy, 2, 2);
               p. end();
            }
        }

        TQWMatrix matrix;
        switch (popupDirection()) {
        case KPanelApplet::Left:
            matrix.scale(sf, -sf);
            matrix.rotate(90);
            break;
        case KPanelApplet::Up:
            matrix.scale(sf, sf);
            break;
        case KPanelApplet::Right:
            matrix.scale(sf, -sf);
            matrix.rotate(90);
            break;
        case KPanelApplet::Down:
            matrix.scale(sf, sf);
            break;
        }
        m_active_pixmap = pixmap.xForm(matrix);

        repaint(false);
    }
#undef eye_x
#undef eye_y
}

void KNewButton::enterEvent(TQEvent* e)
{
    KButton::enterEvent(e);

    TQSize s(size());
    s *= 0.25;
    s = s.expandedTo(TQSize(6,6));

    switch (popupDirection()) {
    case KPanelApplet::Left:
        m_sloppyRegion = TQRect(rect().topRight() - TQPoint(s.width()-1, 0), s);
        break;
    case KPanelApplet::Right:
        m_sloppyRegion = TQRect(rect().topLeft(), s);
        break;
    case KPanelApplet::Up:
        m_sloppyRegion = TQRect(rect().bottomLeft() - TQPoint(0, s.height()-1), s);
        break;
    case KPanelApplet::Down:
        m_sloppyRegion = TQRect(rect().topLeft(), s);
    }

    m_active = true;
    m_movie->unpause();
    m_movie->restart();
}

void KNewButton::rewindMovie()
{
    m_oldPos = TQPoint( -1, -1 );
    m_movie->unpause();
}

void KNewButton::dragEnterEvent(TQDragEnterEvent* /*e*/)
{
    if (m_hoverTimer != -1)
        killTimer(m_hoverTimer);

    m_hoverTimer = startTimer(TQApplication::startDragTime());
    m_mouseInside = true;
    m_drag = true;
}

void KNewButton::dragLeaveEvent(TQDragLeaveEvent* /*e*/)
{
    m_mouseInside = false;
    m_drag = false;
}

void KNewButton::leaveEvent(TQEvent* e)
{
    m_mouseInside = false;
    if (m_hoverTimer != -1)
        killTimer(m_hoverTimer);
    m_hoverTimer = -1;

    KButton::leaveEvent(e);
}

void KNewButton::mouseMoveEvent(TQMouseEvent* e)
{
    KButton::mouseMoveEvent(e);

    m_mouseInside = m_sloppyRegion.contains(e->pos());

    if ( m_sloppyRegion.contains(e->pos())) 
    {
        if (m_hoverTimer == -1 && KickerSettings::openOnHover())
            m_hoverTimer = startTimer(kMax(200,TQApplication::doubleClickInterval()/2));
    }
    else if (m_hoverTimer != -1) 
    {
        killTimer(m_hoverTimer);
        m_hoverTimer = -1;
    }
}

void KNewButton::slotStopAnimation()
{
    m_active = false;
    m_movie->pause();
    m_movie->restart();
    TQTimer::singleShot(200, this, TQT_SLOT(rewindMovie()));
}

const TQPixmap& KNewButton::labelIcon() const
{
    return m_active_pixmap;
}

void KNewButton::slotExecMenu()
{
    if (m_openTimer != -1)
        killTimer(m_openTimer);

    m_openTimer = startTimer(TQApplication::doubleClickInterval() * 3);

    if (m_active)
    {
        m_active = false;
        m_movie->pause();
        m_movie->restart();
    }

    KButton::slotExecMenu();

    assert(!KickerTip::tippingEnabled());
    assert(dynamic_cast<KMenu*>(m_popup));

    disconnect(dynamic_cast<KMenu*>(m_popup), TQT_SIGNAL(aboutToHide()), this,
            TQT_SLOT(slotStopAnimation()));
    connect(dynamic_cast<KMenu*>(m_popup), TQT_SIGNAL(aboutToHide()),
            TQT_SLOT(slotStopAnimation()));

    m_popup->move(KickerLib::popupPosition(popupDirection(), m_popup, this));
    // I wish KMenu would properly done itself when it closes. But it doesn't.

    bool useEffect = true; // could be TQApplication::isEffectEnabled()
    useEffect = false; // too many TQt bugs to be useful
    if (m_drag)
	useEffect = false;

    m_drag = false; // once is enough

    if (useEffect) 
    {
        switch (popupDirection()) {
        case KPanelApplet::Left:
            qScrollEffect(m_popup, TQEffects::LeftScroll);
            break;
        case KPanelApplet::Up:
            qScrollEffect(m_popup, TQEffects::UpScroll);
            break;
        case KPanelApplet::Right:
            qScrollEffect(m_popup, TQEffects::RightScroll);
            break;
        case KPanelApplet::Down:
            qScrollEffect(m_popup, TQEffects::DownScroll);
            break;
        }
    }
    else
        static_cast<KMenu*>(m_popup)->show();
}

void KNewButton::timerEvent(TQTimerEvent* e)
{
    if (e->timerId() == m_hoverTimer)
    {
        if (m_mouseInside && !isDown())
            showMenu();

        killTimer(m_hoverTimer);
        m_hoverTimer = -1;
    }
    if (e->timerId() == m_openTimer)
    {
        killTimer(m_openTimer);
        m_openTimer = -1;
    }
}
