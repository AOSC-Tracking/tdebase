/*****************************************************************

Copyright (c) 2004 Zack Rusin <zrusin@kde.org>
                   Sami Kyostil <skyostil@kempele.fi>
                   Aaron J. Seigo <aseigo@kde.org>

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

#include <tqapplication.h>
#include <tqpainter.h>
#include <tqsimplerichtext.h>
#include <tqtimer.h>
#include <tqtooltip.h>

#include <kdialog.h>

#include "global.h"

#include "kickertip.h"
#include "kickerSettings.h"

// putting this #include higher results in compile errors
#include <netwm.h>
#include <assert.h>

static const int DEFAULT_FRAMES_PER_SECOND = 30;

KickerTip* KickerTip::m_self = 0;
int KickerTip::m_tippingEnabled = 1;

void KickerTip::Client::updateKickerTip() const
{
    if (KickerTip::the()->isTippingFor(dynamic_cast<const TQWidget*>(this)) &&
        KickerTip::the()->isVisible())
    {
        KickerTip::the()->display();
    }
}

KickerTip* KickerTip::the()
{
    if (!m_self)
    {
        m_self = new KickerTip(0);
    }

    return m_self;
}

KickerTip::KickerTip(TQWidget * parent)
    : TQWidget(parent, "animtt",WX11BypassWM),
      m_richText(0),
      m_mimeFactory(0),
      m_dissolveSize(0),
      m_dissolveDelta(-1),
      m_direction(KPanelApplet::Up),
      m_dirty(false),
      m_tippingFor(0),
      m_timer(0, "KickerTip::m_timer"),
      m_frameTimer(0, "KickerTip::m_frameTimer")
{
    setFocusPolicy(TQ_NoFocus);
    setBackgroundMode(NoBackground);
    resize(0, 0);
    hide();
    connect(&m_frameTimer, TQT_SIGNAL(timeout()), TQT_SLOT(internalUpdate()));
//     // FIXME: The settingsChanged(SettingsCategory) signal is not available under Trinity; where was it originally supposed to come from?
//     connect(kapp, TQT_SIGNAL(settingsChanged(SettingsCategory)), TQT_SLOT(slotSettingsChanged()));
}

KickerTip::~KickerTip()
{
    delete m_richText;
    delete m_mimeFactory;
}

void KickerTip::slotSettingsChanged()
{
    TQToolTip::setGloballyEnabled(KickerSettings::showToolTips());
}

void KickerTip::display()
{
    if (!tippingEnabled())
    {
        return;
    }

    {
        // prevent tips from showing when the active window is fullscreened
        NETRootInfo ri(qt_xdisplay(), NET::ActiveWindow);
        NETWinInfo wi(qt_xdisplay(), ri.activeWindow(), ri.rootWindow(), NET::WMState);
        if (wi.state() & NET::FullScreen)
        {
            return;
        }
    }

    TQWidget *widget = const_cast<TQWidget*>(m_tippingFor);
    KickerTip::Client *client = dynamic_cast<KickerTip::Client*>(widget);

    if (!client)
    {
        return;
    }

    // delete the mimefactory and create a new one so any old pixmaps used in the
    // richtext area are freed but the mimefactory is ready to be added to in 
    // the call to updateKickerTip
    delete m_mimeFactory;
    m_mimeFactory = new TQMimeSourceFactory();

    // Declare interchange object and define defaults.
    Data data;
    data.maskEffect = Dissolve;
    data.duration = 2000;
    data.direction = KPanelApplet::Up;
    data.mimeFactory = m_mimeFactory;

    // Tickle the information out of the bastard.
    client->updateKickerTip(data);

    // Hide the tip if there is nothing to show
    if (data.message.isEmpty() && data.subtext.isEmpty() && data.icon.isNull())
    {
        hide();
        return;
    }

    delete m_richText;
    m_richText = new TQSimpleRichText("<qt><h3>" + data.message + "</h3><p>" +
                                     data.subtext + "</p></qt>", font(), TQString(), 0,
                                     m_mimeFactory);
    m_richText->setWidth(640);
    m_direction = data.direction;

    if (KickerSettings::mouseOversShowIcon())
    {
        m_icon = data.icon;
    }
    else if (KickerSettings::mouseOversShowText())
    {
        m_icon = TQPixmap();
    }
    else
    {
        // don't bother since we have NOTHING to show
        return;
    }

    m_maskEffect = isVisible() ? Plain : data.maskEffect;
    m_dissolveSize = 24;
    m_dissolveDelta = -1;

    displayInternal();

    m_frameTimer.start(1000 / DEFAULT_FRAMES_PER_SECOND);

    // close the message window after given mS
    if (data.duration > 0)
    {
        disconnect(&m_timer, TQT_SIGNAL(timeout()), 0, 0);
        connect(&m_timer, TQT_SIGNAL(timeout()), TQT_SLOT(hide()));
        m_timer.start(data.duration, true);
    }
    else
    {
        m_timer.stop();
    }

    move(KickerLib::popupPosition(m_direction, this, m_tippingFor));
    show();
}

void KickerTip::paintEvent(TQPaintEvent * e)
{
    if (m_dirty)
    {
        displayInternal();
        m_dirty = false;
    }

    TQPainter p(this);
    p.drawPixmap(e->rect().topLeft(), m_pixmap, e->rect());
}

void KickerTip::mousePressEvent(TQMouseEvent * /*e*/)
{
    m_timer.stop();
    hide();
}

static void drawRoundRect(TQPainter &p, const TQRect &r)
{
    static int line[8] = { 1, 3, 4, 5, 6, 7, 7, 8 };
    static int border[8] = { 1, 2, 1, 1, 1, 1, 1, 1 };
    int xl, xr, y1, y2;
    TQPen pen = p.pen();
    bool drawBorder = pen.style() != TQPen::NoPen;
    
    if (r.width() < 16 || r.height() < 16)
    {
        p.drawRect(r);
        return;
    }
    
    p.fillRect(r.x(), r.y() + 8, r.width(), r.height() - 16, p.brush());
    p.fillRect(r.x() + 8, r.y(), r.width() - 16, r.height(), p.brush());
    
    p.setPen(p.brush().color());
        
    for (int i = 0; i < 8; i++)
    {
        xl = i;
        xr = r.width() - i - 1;
        y1 = 7;
        y2 = 7 - (line[i] - 1);
        
        p.drawLine(xl, y1, xl, y2);
        p.drawLine(xr, y1, xr, y2);
        
        y1 = r.height() - y1 - 1;
        y2 = r.height() - y2 - 1;
        
        p.drawLine(xl, y1, xl, y2);
        p.drawLine(xr, y1, xr, y2);
        
    }
    
    if (drawBorder)
    {
        p.setPen(pen);
        
        if (r.height() > 16)
        {
            p.drawLine(r.x(), r.y() + 8, r.x(), r.y() + r.height() - 9);
            p.drawLine(r.x() + r.width() - 1, r.y() + 8, r.x() + r.width() - 1, r.y() + r.height() - 9);
        }
        if (r.width() > 16)
        {
            p.drawLine(r.x() + 8, r.y(), r.x() + r.width() - 9, r.y());
            p.drawLine(r.x() + 8, r.y() + r.height() - 1, r.x() + r.width() - 9, r.y() + r.height() - 1);
        }
    
        for (int i = 0; i < 8; i++)
        {
            xl = i;
            xr = r.width() - i - 1;
            y2 = 7 - (line[i] - 1);
            y1 = y2 + (border[i] - 1);
            
            p.drawLine(xl, y1, xl, y2);
            p.drawLine(xr, y1, xr, y2);
            
            y1 = r.height() - y1 - 1;
            y2 = r.height() - y2 - 1;
            
            p.drawLine(xl, y1, xl, y2);
            p.drawLine(xr, y1, xr, y2);
            
        }
    }
}

void KickerTip::plainMask()
{
    TQPainter maskPainter(&m_mask);

    m_mask.fill(Qt::color0);

    maskPainter.setBrush(Qt::color1);
    maskPainter.setPen(Qt::NoPen);
    //maskPainter.drawRoundRect(m_mask.rect(), 1600 / m_mask.rect().width(), 1600 / m_mask.rect().height());
    drawRoundRect(maskPainter, m_mask.rect());
    setMask(m_mask);
    m_frameTimer.stop();
}

void KickerTip::dissolveMask()
{
    TQPainter maskPainter(&m_mask);

    m_mask.fill(Qt::color0);

    maskPainter.setBrush(Qt::color1);
    maskPainter.setPen(Qt::NoPen);
    //maskPainter.drawRoundRect(m_mask.rect(), 1600 / m_mask.rect().width(), 1600 / m_mask.rect().height());
    drawRoundRect(maskPainter, m_mask.rect());

    m_dissolveSize += m_dissolveDelta;

    if (m_dissolveSize > 0)
    {
        maskPainter.setRasterOp(TQt::EraseROP);

        int x, y, s;
        const int size = 16;

        for (y = 0; y < height() + size; y += size)
        {
            x = width();
            s = 4 * m_dissolveSize * x / 128;
            for (; x > -size; x -= size, s -= 2)
            {
                if (s < 0)
                {
                    break;
                }
                maskPainter.drawEllipse(x - s / 2, y - s / 2, s, s);
            }
        }
    }
    else if (m_dissolveSize < 0)
    {
        m_frameTimer.stop();
        m_dissolveDelta = 1;
    }

    setMask(m_mask);
}

void KickerTip::displayInternal()
{
    // we need to check for m_tippingFor here as well as m_richText
    // since if one is really persistant and moves the mouse around very fast
    // you can trigger a situation where m_tippingFor gets reset to 0 but
    // before display() is called!
    if (!m_tippingFor || !m_richText)
    {
        return;
    }

    // determine text rectangle
    TQRect textRect(0, 0, 0, 0);
    if (KickerSettings::mouseOversShowText())
    {
        textRect.setWidth(m_richText->widthUsed());
        textRect.setHeight(m_richText->height());
    }

    int margin = KDialog::marginHint();
    int height = QMAX(m_icon.height(), textRect.height()) + 2 * margin;
    int textX = m_icon.isNull() ? margin : 2 + m_icon.width() + 2 * margin;
    int width = textX + textRect.width() + margin;
    int textY = (height - textRect.height()) / 2;

    // resize pixmap, mask and widget
    bool firstTime = m_dissolveSize == 24;
    if (firstTime)
    {
        m_mask.resize(width, height);
        m_pixmap.resize(width, height);
        resize(width, height);
        if (isVisible())
        {
            // we've already been shown before, but we may grow larger.
            // in the case of Up or Right displaying tips, this growth can
            // result in the tip occluding the panel and causing it to redraw
            // once we return back to display() causing horrid flicker
            move(KickerLib::popupPosition(m_direction, this, m_tippingFor));
        }
    }

    // create and set transparency mask
    switch(m_maskEffect)
    {
        case Plain:
            plainMask();
            break;

        case Dissolve:
            dissolveMask();
            break;
    }

    // draw background
    TQPainter bufferPainter(&m_pixmap);
    bufferPainter.setPen(colorGroup().foreground());
    bufferPainter.setBrush(colorGroup().background());
    //bufferPainter.drawRoundRect(0, 0, width, height, 1600 / width, 1600 / height);
    drawRoundRect(bufferPainter, TQRect(0, 0, width, height));

    // draw icon if present
    if (!m_icon.isNull())
    {
        bufferPainter.drawPixmap(margin,
                                 margin,
                                 m_icon, 0, 0,
                                 m_icon.width(), m_icon.height());
    }

    if (KickerSettings::mouseOversShowText())
    {
        // draw text shadow
        TQColorGroup cg = colorGroup();
        cg.setColor(TQColorGroup::Text, cg.background().dark(115));
        int shadowOffset = TQApplication::reverseLayout() ? -1 : 1;
        m_richText->draw(&bufferPainter, textX + shadowOffset, textY + 1, TQRect(), cg);

        // draw text
        cg = colorGroup();
        m_richText->draw(&bufferPainter, textX, textY, rect(), cg);
    }
}

void KickerTip::tipFor(const TQWidget* w)
{
    if (m_tippingFor)
    {
        disconnect(m_tippingFor, TQT_SIGNAL(destroyed(TQObject*)),
                   this, TQT_SLOT(tipperDestroyed(TQObject*)));
    }

    m_tippingFor = w;

    if (m_tippingFor)
    {
        connect(m_tippingFor, TQT_SIGNAL(destroyed(TQObject*)),
                this, TQT_SLOT(tipperDestroyed(TQObject*)));
    }
}

void KickerTip::untipFor(const TQWidget* w)
{
    if (isTippingFor(w))
        hide();
}

bool KickerTip::isTippingFor(const TQWidget* w) const
{
    return m_tippingFor == w;
}

void KickerTip::tipperDestroyed(TQObject* o)
{
    // we can't do a dynamic cast because we are in the process of dying
    // so static it is.
    untipFor(TQT_TQWIDGET(o));
}

void KickerTip::internalUpdate()
{
    m_dirty = true;
    repaint(false);
}

void KickerTip::enableTipping(bool tip)
{
    if (tip)
    {
        m_tippingEnabled++;
    }
    else
    {
        m_tippingEnabled--;
    }

//    assert(m_tippingEnabled >= -1);

    if (m_tippingEnabled < 1 && m_self)
    {
        m_self->m_timer.stop();
        m_self->hide();
    }
}

bool KickerTip::tippingEnabled()
{
    return m_tippingEnabled > 0;
}

void KickerTip::hide()
{
    tipFor(0);
    m_timer.stop();
    m_frameTimer.stop();
    TQWidget::hide();

    TQToolTip::setGloballyEnabled(KickerSettings::showToolTips());
}

bool KickerTip::eventFilter(TQObject *object, TQEvent *event)
{
    if (!tippingEnabled())
    {
        return false;
    }

    if (!object->isWidgetType())
    {
        return false;
    }

    TQWidget *widget = TQT_TQWIDGET(object);

    switch (event->type())
    {
        case TQEvent::Enter:
            if (!KickerSettings::showMouseOverEffects())
            {
                return false;
            }

            if (!mouseGrabber() &&
                !tqApp->activePopupWidget() &&
                !isTippingFor(widget))
            {
                TQToolTip::setGloballyEnabled(false);

                tipFor(widget);
                m_timer.stop();
                disconnect(&m_timer, TQT_SIGNAL(timeout()), 0, 0);
                connect(&m_timer, TQT_SIGNAL(timeout()), TQT_SLOT(display()));

                // delay to avoid false starts
                // e.g. when the user quickly zooms their mouse over
                // a button then out of kicker
                if (isVisible())
                {
                    m_timer.start(150, true);
                }
                else
                {
                    m_timer.start(KickerSettings::mouseOversShowDelay(), true);
                }
            }
            break;
        case TQEvent::Leave:
            m_timer.stop();

            if (isTippingFor(widget) && isVisible())
            {
                disconnect(&m_timer, TQT_SIGNAL(timeout()), 0, 0);
                connect(&m_timer, TQT_SIGNAL(timeout()), TQT_SLOT(hide()));
                m_timer.start(KickerSettings::mouseOversHideDelay(), true);
            }

            tipFor(0);
            break;
        case TQEvent::MouseButtonPress:
            m_timer.stop();
            hide();
        default:
            break;
    }

    return false;
}

#include <kickertip.moc>

