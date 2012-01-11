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
	
	setTitle(i18n("K Menu"));
    Q_ASSERT( !m_self );
    m_self = this;
    m_openTimer = -1;
    m_hoverTimer = -1;
    m_mouseInside = false;
    m_drag = false;
    
    setIcon("kmenu");
    setIcon(KickerSettings::customKMenuIcon());
    
    TQApplication::desktop()->screen()->installEventFilter(this);

  if (KickerSettings::showKMenuText())
    {
        setButtonText(KickerSettings::kMenuText());
        setFont(KickerSettings::buttonFont());
        setTextColor(KickerSettings::buttonTextColor());
    }    
    
    tqrepaint();
}

KNewButton::~KNewButton()
{
    if ( m_self == this )
        m_self = 0;
    setMouseTracking(false);
}


void KNewButton::drawButton(TQPainter *p)
{
    if (KickerSettings::showDeepButtons())
        PanelPopupButton::drawDeepButton(p);
    else
        PanelPopupButton::drawButton(p);
}

void KNewButton::show()
{
     KButton::show();

     if (KickerSettings::firstRun()) {
         TQTimer::singleShot(0,this,TQT_SLOT(slotExecMenu()));
         KickerSettings::setFirstRun(false);
         KickerSettings::writeConfig();
     }
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

    return KButton::eventFilter(o, e);
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

void KNewButton::slotExecMenu()
{

    if (m_active)
    {
        m_active = false;
    }

    KButton::slotExecMenu();

    assert(!KickerTip::tippingEnabled());
    assert(dynamic_cast<KMenu*>(m_popup));

    m_popup->move(KickerLib::popupPosition(popupDirection(), m_popup, this));
    // I wish KMenu would properly done itself when it closes. But it doesn't.

    m_drag = false; // once is enough

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
}
