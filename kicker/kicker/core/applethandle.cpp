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

#include <tqcursor.h>
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <tqpixmapcache.h>
#include <tqtimer.h>
#include <tqtooltip.h>
#include <tqimage.h>

#include <kpushbutton.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include "global.h"
#include "container_applet.h"
#include "kickerSettings.h"

#include "applethandle.h"

AppletHandle::AppletHandle(AppletContainer* parent)
    : TQWidget(parent),
      m_applet(parent),
      m_menuButton(0),
      m_drawHandle(false),
      m_popupDirection(KPanelApplet::Up),
      m_handleHoverTimer(0)
{
    setBackgroundOrigin(AncestorOrigin);
    setMinimumSize(widthForHeight(0), heightForWidth(0));
    m_layout = new TQBoxLayout(this, TQBoxLayout::BottomToTop, 0, 0);

    m_dragBar = new AppletHandleDrag(this);
    m_dragBar->installEventFilter(this);
    m_layout->addWidget(m_dragBar);

    if (kapp->authorizeKAction("kicker_rmb"))
    {
        m_menuButton = new AppletHandleButton( this );
        m_menuButton->installEventFilter(this);
        m_layout->addWidget(m_menuButton);

        connect(m_menuButton, TQT_SIGNAL(pressed()),
                this, TQT_SLOT(menuButtonPressed()));
        TQToolTip::add(m_menuButton, i18n("%1 menu").arg(parent->info().name()));
    }

    TQToolTip::add(this, i18n("%1 applet handle").arg(parent->info().name()));
    resetLayout();
}

int AppletHandle::heightForWidth( int /* w */ ) const
{
    int size = style().pixelMetric(TQStyle::PM_DockWindowHandleExtent, this);

    return size;
}

int AppletHandle::widthForHeight( int /* h */ ) const
{
    int size = style().pixelMetric(TQStyle::PM_DockWindowHandleExtent, this);

    return size;
}

void AppletHandle::setPopupDirection(KPanelApplet::Direction d)
{
    Qt::ArrowType a = Qt::UpArrow;

    if (d == m_popupDirection || !m_menuButton)
    {
        return;
    }

    m_popupDirection = d;

    switch (m_popupDirection)
    {
        case KPanelApplet::Up:
            m_layout->setDirection(TQBoxLayout::BottomToTop);
            a = Qt::UpArrow;
            break;
        case KPanelApplet::Down:
            m_layout->setDirection(TQBoxLayout::TopToBottom);
            a = Qt::DownArrow;
            break;
        case KPanelApplet::Left:
            m_layout->setDirection(TQBoxLayout::RightToLeft);
            a = Qt::LeftArrow;
            break;
        case KPanelApplet::Right:
            m_layout->setDirection(TQBoxLayout::LeftToRight);
            a = Qt::RightArrow;
            break;
    }

    m_menuButton->setArrowType(a);
    m_layout->activate();
}

void AppletHandle::resetLayout()
{
    if (m_handleHoverTimer && !m_drawHandle)
    {
        m_dragBar->hide();

        if (m_menuButton)
        {
            m_menuButton->hide();
        }
    }
    else
    {
        m_dragBar->show();

        if (m_menuButton)
        {
            m_menuButton->show();
        }
    }
}

void AppletHandle::setFadeOutHandle(bool fadeOut)
{
    if (fadeOut)
    {
        if (!m_handleHoverTimer)
        {
            m_handleHoverTimer = new TQTimer(this, "m_handleHoverTimer");
            connect(m_handleHoverTimer, TQT_SIGNAL(timeout()),
                    this, TQT_SLOT(checkHandleHover()));
            m_applet->installEventFilter(this);
        }
    }
    else
    {
        delete m_handleHoverTimer;
        m_handleHoverTimer = 0;
        m_applet->removeEventFilter(this);
    }

    resetLayout();
}

bool AppletHandle::eventFilter(TQObject *o, TQEvent *e)
{
    if (o == parent())
    {
        switch (e->type())
        {
            case TQEvent::Enter:
            {
                m_drawHandle = true;
                resetLayout();

               break;
            }

            case TQEvent::Leave:
            {
                if (m_menuButton && m_menuButton->isOn())
                {
                    break;
                }

                if (m_handleHoverTimer)
                {
                    m_handleHoverTimer->start(250);
                }

                TQWidget* w = dynamic_cast<TQWidget*>(o);

                bool nowDrawIt = false;
                if (w)
                {
                    // a hack for applets that have out-of-process
                    // elements (e.g the systray) so that the handle
                    // doesn't flicker when moving over those elements
                    if (TQT_TQRECT_OBJECT(w->rect()).contains(w->mapFromGlobal(TQCursor::pos())))
                    {
                        nowDrawIt = true;
                    }
                }

                if (nowDrawIt != m_drawHandle)
                {
                    m_drawHandle = nowDrawIt;
                    resetLayout();
                }
                break;
            }

            default:
                break;
        }

        return TQWidget::eventFilter( o, e );
    }
    else if (TQT_BASE_OBJECT(o) == TQT_BASE_OBJECT(m_dragBar))
    {
        if (e->type() == TQEvent::MouseButtonPress)
        {
            TQMouseEvent* ev = TQT_TQMOUSEEVENT(e);
            if (ev->button() == Qt::LeftButton || ev->button() == Qt::MidButton)
            {
                emit moveApplet(m_applet->mapFromGlobal(ev->globalPos()));
            }
        }
    }

    if (m_menuButton && e->type() == TQEvent::MouseButtonPress)
    {
        TQMouseEvent* ev = TQT_TQMOUSEEVENT(e);
        if (ev->button() == Qt::RightButton)
        {
            if (!m_menuButton->isDown())
            {
                m_menuButton->setDown(true);
                menuButtonPressed();
            }

            return true;
        }
    }

    return TQWidget::eventFilter(o, e);    // standard event processing
}

void AppletHandle::menuButtonPressed()
{
    if (!kapp->authorizeKAction("kicker_rmb"))
    {
        return;
    }

    emit showAppletMenu();

    if (!onMenuButton(TQCursor::pos()))
    {
        toggleMenuButtonOff();
    }
}

void AppletHandle::checkHandleHover()
{
    if (!m_handleHoverTimer ||
        (m_menuButton && m_menuButton->isOn()) ||
        m_applet->geometry().contains(m_applet->mapToParent(
                                      m_applet->mapFromGlobal(TQCursor::pos()))))
    {
        return;
    }

    m_handleHoverTimer->stop();
    m_drawHandle = false;
    resetLayout();
}

bool AppletHandle::onMenuButton(const TQPoint& point) const
{
    return m_menuButton && (childAt(mapFromGlobal(point)) == m_menuButton);
}

void AppletHandle::toggleMenuButtonOff()
{
    if (!m_menuButton)
    {
        return;
    }

    m_menuButton->setDown(false);

    if (m_handleHoverTimer)
    {
        m_handleHoverTimer->start(250);
    }
}

AppletHandleDrag::AppletHandleDrag(AppletHandle* parent)
    : TQWidget(parent),
      m_parent(parent),
      m_inside(false)
{
   setBackgroundOrigin( AncestorOrigin );
}

TQSize AppletHandleDrag::minimumSizeHint() const
{
    int wh = style().pixelMetric(TQStyle::PM_DockWindowHandleExtent, this);

    if (m_parent->orientation() == Qt::Horizontal)
    {
        return TQSize(wh, 0);
    }

    return TQSize(0, wh);
}

TQSizePolicy AppletHandleDrag::sizePolicy() const
{
    if (m_parent->orientation() == Qt::Horizontal)
    {
        return TQSizePolicy( TQSizePolicy::Fixed, TQSizePolicy::Preferred );
    }

    return TQSizePolicy( TQSizePolicy::Preferred, TQSizePolicy::Fixed );
}

void AppletHandleDrag::enterEvent( TQEvent *e )
{
    m_inside = true;
    TQWidget::enterEvent( e );
    update();
}

void AppletHandleDrag::leaveEvent( TQEvent *e )
{
    m_inside = false;
    TQWidget::enterEvent( e );
    update();
}

void AppletHandleDrag::paintEvent(TQPaintEvent *)
{
    TQPainter p(this);
    
    if (!KickerSettings::transparent())
    {
        if (paletteBackgroundPixmap())
        {
            TQPoint offset = backgroundOffset();
            int ox = offset.x();
            int oy = offset.y();
            p.drawTiledPixmap( 0, 0, width(), height(),*paletteBackgroundPixmap(), ox, oy);
        }
        
        TQStyle::SFlags flags = TQStyle::Style_Default;
        flags |= TQStyle::Style_Enabled;
        if (m_parent->orientation() == Qt::Horizontal)
        {
            flags |= TQStyle::Style_Horizontal;
        }
    
        TQRect r = rect();
    
        style().tqdrawPrimitive(TQStyle::PE_DockWindowHandle, &p, r,
                            colorGroup(), flags);
    }
    else
    {
        KickerLib::drawBlendedRect(&p, TQRect(0, 0, width(), height()), paletteForegroundColor(), m_inside ? 0x40 : 0x20);
    }
}

AppletHandleButton::AppletHandleButton(AppletHandle *parent)
  : SimpleArrowButton(parent),
    m_parent(parent)
{
}

TQSize AppletHandleButton::minimumSizeHint() const
{
    int height = style().pixelMetric(TQStyle::PM_DockWindowHandleExtent, this);
    int width = height;

    if (m_parent->orientation() == Qt::Horizontal)
    {
        return TQSize(width, height);
    }

    return TQSize(height, width);
}

TQSizePolicy AppletHandleButton::sizePolicy() const
{
    return TQSizePolicy( TQSizePolicy::Fixed, TQSizePolicy::Fixed );
}

#include "applethandle.moc"
