/*****************************************************************

Copyright (c) 2004-2005 Aaron J. Seigo <aseigo@kde.org>
Copyright (c) 2004 Zack Rusin <zrusin@kde.org>
                   Sami Kyostil <skyostil@kempele.fi>

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
#include <tqlabel.h>
#include <tqpainter.h>
#include <tqsimplerichtext.h>
#include <tqtimer.h>

#include <kdialog.h>
#include <klocale.h>

#include "global.h"

#include "appletinfo.h"
#include "appletwidget.h"
#include "addappletvisualfeedback.h"
#include "kickerSettings.h"

#define DEFAULT_FRAMES_PER_SECOND 30

AddAppletVisualFeedback::AddAppletVisualFeedback(AppletWidget* widget,
                                                 const TQWidget* target,
                                                 KPanelApplet::Direction direction)
    : TQWidget(0, "animtt", WX11BypassWM),
      m_target(target),
      m_direction(direction),
      m_icon(*widget->itemPixmap->pixmap()),
      m_richText(0),
      m_dissolveDelta(-1),
      m_frames(1),
      m_moveTimer(0, "m_moveTimer"),
      m_dirty(false)
{
    setFocusPolicy(TQ_NoFocus);
    setBackgroundMode(NoBackground);
    connect(&m_moveTimer, TQT_SIGNAL(timeout()), TQT_SLOT(swoopCloser()));

    TQString m = "<qt><h3>" + i18n("%1 Added").arg(widget->info().name());

    if (widget->info().name() != widget->info().comment())
    {
        m += "</h3><p>" + widget->info().comment() + "</p></qt>";
    }

    m_richText = new TQSimpleRichText(m, font());
    m_richText->setWidth(400);

    displayInternal();

    m_destination = KickerLib::popupPosition(m_direction, this, m_target);
    TQPoint startAt = widget->itemPixmap->geometry().topLeft();
    startAt = widget->itemPixmap->mapToGlobal(startAt);
    move(startAt);

    m_frames = (m_destination - startAt).manhattanLength() / 20;
    m_moveTimer.start(10);

    show();
}

AddAppletVisualFeedback::~AddAppletVisualFeedback()
{
    delete m_richText;
}

void AddAppletVisualFeedback::paintEvent(TQPaintEvent * e)
{
    if (m_dirty)
    {
        displayInternal();
        m_dirty = false;
    }

    TQPainter p(this);
    p.drawPixmap(e->rect().topLeft(), m_pixmap, e->rect());
}

void AddAppletVisualFeedback::mousePressEvent(TQMouseEvent *)
{
    m_moveTimer.stop();
    hide();
    deleteLater();
}

void AddAppletVisualFeedback::makeMask()
{
    TQPainter maskPainter(&m_mask);

    m_mask.fill(Qt::black);

    maskPainter.setBrush(Qt::white);
    maskPainter.setPen(Qt::white);
    maskPainter.drawRoundRect(m_mask.rect(), 1600 / m_mask.rect().width(),
                              1600 / m_mask.rect().height());
    setMask(m_mask);
}

void AddAppletVisualFeedback::displayInternal()
{
    // determine text rectangle
    TQRect textRect(0, 0, 0, 0);

    if (m_frames < 1)
    {
        textRect.setWidth(m_richText->widthUsed());
        textRect.setHeight(m_richText->height());
        textRect.moveBy(-textRect.left(), -textRect.top());
        textRect.addCoords(0, 0, 2, 2);
    }

    int margin = KDialog::marginHint();
    int height = QMAX(m_icon.height(), textRect.height()) + 2 * margin;
    int textX = m_icon.isNull() ? margin : 2 + m_icon.width() + 2 * margin;
    int width = textX;

    if (m_frames < 1)
    {
        width += textRect.width() + margin;
    }

    // resize pixmap, mask and widget
    m_mask.resize(width, height);
    m_pixmap.resize(width, height);
    resize(width, height);

    if (m_frames < 1)
    {
        move(KickerLib::popupPosition(m_direction, this, m_target));
    }

    // create and set transparency mask
    makeMask();

    // draw background
    TQPainter bufferPainter(&m_pixmap);
    bufferPainter.setPen(Qt::black);
    bufferPainter.setBrush(colorGroup().background());
    bufferPainter.drawRoundRect(0, 0, width, height,
                                1600 / width, 1600 / height);

    // draw icon if present
    if (!m_icon.isNull())
    {
        bufferPainter.drawPixmap(margin,
                                 margin,
                                 m_icon, 0, 0,
                                 m_icon.width(), m_icon.height());
    }

    if (m_frames < 1)
    {
        int textY = (height - textRect.height()) / 2;

        // draw text shadow
        TQColorGroup cg = colorGroup();
        cg.setColor(TQColorGroup::Text, cg.background().dark(115));
        int shadowOffset = TQApplication::reverseLayout() ? -1 : 1;
        m_richText->draw(&bufferPainter, 5 + textX + shadowOffset,
                         textY + 1, TQRect(), cg);

        // draw text
        cg = colorGroup();
        m_richText->draw(&bufferPainter, 5 + textX, textY, rect(), cg);
    }
}

void AddAppletVisualFeedback::swoopCloser()
{
    if (m_destination.isNull() || m_frames == 0)
    {
        return;
    }

    TQPoint loc = geometry().topLeft();
    bool isLeft = m_destination.x() > loc.x();
    if (loc.x() != m_destination.x())
    {
        int newX = loc.x() + ((m_destination.x() - loc.x()) / m_frames * 2);
        if ((m_destination.x() > newX) != isLeft)
        {
            newX = m_destination.x();
        }
        loc.setX(newX);
    }

    if (loc.y() != m_destination.y())
    {
        loc.setY(loc.y() + ((m_destination.y() - loc.y()) / m_frames));
    }

    move(loc);
    --m_frames;

    if (m_frames < 1)
    {
        m_moveTimer.stop();
        displayInternal();
        TQTimer::singleShot(2000, this, TQT_SLOT(deleteLater()));
    }
}

void AddAppletVisualFeedback::internalUpdate()
{
    m_dirty = true;
    tqrepaint(false);
}

#include "addappletvisualfeedback.moc"
