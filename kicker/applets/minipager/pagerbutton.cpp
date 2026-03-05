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

#include <stdlib.h>

#include <tqcursor.h>
#include <tqdrawutil.h>
#include <tqlineedit.h>
#include <tqpainter.h>
#include <tqpopupmenu.h>
#include <tqstylesheet.h>
#include <tqinputdialog.h>

#include <netwm.h>
#include <dcopclient.h>

#include <twinmodule.h>
#include <ksharedpixmap.h>
#include <kpixmapio.h>
#include <kpixmapeffect.h>
#include <kstringhandler.h>
#include <kiconloader.h>

#include "global.h"
#include "kickertip.h"
#include "kickerSettings.h"
#include "kshadowengine.h"
#include "paneldrag.h"

#include "pagerapplet.h"
#include "pagerbutton.h"
#include "pagerbutton.moc"
#include "pagersettings.h"

#ifdef FocusOut
#undef FocusOut
#endif

TDESharedPixmap* KMiniPagerButton::s_commonSharedPixmap;
KPixmap* KMiniPagerButton::s_commonBgPixmap;

KMiniPagerButton::KMiniPagerButton(int desk, bool useViewPorts, const TQPoint& viewport,
        KMiniPager *parent, const char *name)
    : TQButton(parent, name),
      m_pager(parent),
      m_desktop(desk),
      m_useViewports(useViewPorts),
      m_viewport(viewport),
      m_lineEdit(0),
      m_sharedPixmap(0),
      m_bgPixmap(0),
      m_isCommon(false),
      m_currentWindow(0),
      m_updateCompressor(0, "KMiniPagerButton::updateCompressor"),
      m_dragSwitchTimer(0, "KMiniPagerButton::dragSwitchTimer"),
      m_inside(false)
{
    setToggleButton(true);
    setAcceptDrops(true);
    setWFlags(TQt::WNoAutoErase);

    setBackgroundOrigin(AncestorOrigin);
    installEventFilter(KickerTip::the());

    m_desktopName = m_pager->twin()->desktopName(m_desktop);

    connect(this, TQ_SIGNAL(clicked()), TQ_SLOT(slotClicked()));
    connect(this, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotToggled(bool)));
    connect(&m_dragSwitchTimer, TQ_SIGNAL(timeout()), this, TQ_SLOT(slotDragSwitch()));
    connect(&m_updateCompressor, TQ_SIGNAL(timeout()), this, TQ_SLOT(update()));

    if (m_pager->desktopPreview())
    {
        setMouseTracking(true);
    }
    loadBgPixmap();
}

KMiniPagerButton::~KMiniPagerButton()
{
    delete m_sharedPixmap;
    delete m_bgPixmap;
}

TQRect KMiniPagerButton::mapGeometryToViewport(const KWin::WindowInfo& info) const
{
    if (!m_useViewports)
        return info.frameGeometry();

    // ### fix vertically layouted viewports
    TQRect _r(info.frameGeometry());
    TQPoint vx(m_pager->twin()->currentViewport(m_pager->twin()->currentDesktop()));

    _r.moveBy( - (m_desktop - vx.x()) * TQApplication::desktop()->width(),
               0);

    if ((info.state() & NET::Sticky))
    {
        _r.moveTopLeft(TQPoint(_r.x() % TQApplication::desktop()->width(),
                    _r.y() % TQApplication::desktop()->height()));

    }

    return _r;
}

TQPoint KMiniPagerButton::mapPointToViewport(const TQPoint& _p) const
{
    if (!m_useViewports) return _p;

    TQPoint vx(m_pager->twin()->currentViewport(m_pager->twin()->currentDesktop()));

    // ### fix vertically layouted viewports
    TQPoint p(_p);
    p.setX(p.x() + (m_desktop - vx.x()) * TQApplication::desktop()->width());
    return p;
}

bool KMiniPagerButton::shouldPaintWindow( KWin::WindowInfo *info ) const
{
    if (!info)
      return false;

//  if (info->mappingState != NET::Visible)
//    return false;

    NET::WindowType type = info->windowType( NET::NormalMask | NET::DesktopMask
        | NET::DockMask | NET::ToolbarMask | NET::MenuMask | NET::DialogMask
        | NET::OverrideMask | NET::TopMenuMask | NET::UtilityMask | NET::SplashMask );

    if (type == NET::Desktop || type == NET::Dock || type == NET::TopMenu)
      return false;

    if (!m_useViewports && !info->isOnDesktop(m_desktop))
      return false;

    if (m_useViewports) {
        TQRect r = mapGeometryToViewport(*info);

        if (!info->hasState(NET::Sticky) &&
            !TQApplication::desktop()->geometry().contains(r.topLeft()) &&
            !TQApplication::desktop()->geometry().contains(r.topRight()))
            return false;
    }

    if (info->state() & NET::SkipPager || info->state() & NET::Shaded )
      return false;

    if (info->win() == m_pager->winId())
      return false;

    if ( info->isMinimized() )
      return false;

    return true;
}

void KMiniPagerButton::resizeEvent(TQResizeEvent *ev)
{
    if (m_lineEdit)
    {
        m_lineEdit->setGeometry(rect());
    }

    delete m_bgPixmap;
    m_bgPixmap = 0;

    TQButton::resizeEvent(ev);
}

void KMiniPagerButton::windowsChanged()
{
    m_currentWindow = 0;

    if (!m_updateCompressor.isActive())
    {
        m_updateCompressor.start(50, true);
    }
}

void KMiniPagerButton::backgroundChanged()
{
    delete s_commonSharedPixmap;
    s_commonSharedPixmap = 0;
    delete s_commonBgPixmap;
    s_commonBgPixmap = 0;
    loadBgPixmap();
}

void KMiniPagerButton::loadBgPixmap()
{
    bool retval;

    if (m_pager->bgType() != PagerSettings::EnumBackgroundType::BgLive)
        return; // not needed

    DCOPClient *client = tdeApp->dcopClient();
    if (!client->isAttached())
    {
        client->attach();
    }

    TQCString kdesktop_name;
    int screen_number = DefaultScreen(tqt_xdisplay());
    if (screen_number == 0)
        kdesktop_name = "kdesktop";
    else
        kdesktop_name.sprintf("kdesktop-screen-%d", screen_number);

    TQByteArray data, replyData;
    TQCString replyType;
    if (client->call(kdesktop_name, "KBackgroundIface", "isCommon()",
                     data, replyType, replyData))
    {
        if (replyType == "bool")
        {
            TQDataStream reply(replyData, IO_ReadOnly);
            reply >> m_isCommon;
        }
    }

    if (m_isCommon)
    {
        if (s_commonBgPixmap)
        { // pixmap is already ready, just use it
            backgroundLoaded( true );
            return;
        }
        else if (s_commonSharedPixmap)
        { // other button is already fetching the pixmap
            connect(s_commonSharedPixmap, TQ_SIGNAL(done(bool)),
                    TQ_SLOT(backgroundLoaded(bool)));
            return;
        }
    }

    if (m_isCommon)
    {
        if (!s_commonSharedPixmap)
        {
            s_commonSharedPixmap = new TDESharedPixmap;
            connect(s_commonSharedPixmap, TQ_SIGNAL(done(bool)),
                    TQ_SLOT(backgroundLoaded(bool)));
        }
        retval = s_commonSharedPixmap->loadFromShared(TQString("DESKTOP1"));
        if (retval == false) {
            TQDataStream args( data, IO_WriteOnly );
            args << 1;	// Argument is 1 (true)
            client->send(kdesktop_name, "KBackgroundIface", "setExport(int)", data);
            retval = s_commonSharedPixmap->loadFromShared(TQString("DESKTOP1"));
        }
    }
    else
    {
        if (!m_sharedPixmap)
        {
            m_sharedPixmap = new TDESharedPixmap;
            connect(m_sharedPixmap, TQ_SIGNAL(done(bool)),
                    TQ_SLOT(backgroundLoaded(bool)));
        }
        retval = m_sharedPixmap->loadFromShared(TQString("DESKTOP%1").arg(m_desktop));
        if (retval == false) {
            TQDataStream args( data, IO_WriteOnly );
            args << 1;
            client->send(kdesktop_name, "KBackgroundIface", "setExport(int)", data);
            retval = m_sharedPixmap->loadFromShared(TQString("DESKTOP%1").arg(m_desktop));
        }
    }
}

static TQPixmap scalePixmap(const TQPixmap &pixmap, int width, int height)
{
    if (pixmap.width()>100)
    {
        KPixmapIO io;
        TQImage img( io.convertToImage( pixmap ) );
        return io.convertToPixmap( img.smoothScale( width, height ) );
    }

    TQImage img( pixmap.convertToImage().smoothScale( width, height ) );
    TQPixmap pix;
    pix.convertFromImage( img );

    return pix;
}

void KMiniPagerButton::backgroundLoaded( bool loaded )
{
    if (loaded)
    {
        if (!m_bgPixmap)
        {
            m_bgPixmap = new KPixmap;
        }
        if (m_isCommon)
        {
            if (!s_commonBgPixmap)
            {
                s_commonBgPixmap = new KPixmap;
                *s_commonBgPixmap = scalePixmap(*s_commonSharedPixmap, width(), height());
                s_commonSharedPixmap->deleteLater(); // let others get the signal too
                s_commonSharedPixmap = 0;
            }
            *m_bgPixmap = *s_commonBgPixmap;
        }
        else
        {
            *m_bgPixmap = scalePixmap(*m_sharedPixmap, width(), height());
            delete m_sharedPixmap;
            m_sharedPixmap = 0L;
        }

        update();
    }
    else
    {
        kdWarning() << "Error getting the background\n";
    }
}

void KMiniPagerButton::enterEvent(TQEvent *)
{
    m_inside = true;
    update();
}

void KMiniPagerButton::leaveEvent(TQEvent *)
{
    m_inside = false;
    update();
}

void KMiniPagerButton::drawButton(TQPainter *bp)
{
    int w = width();
    int h = height();
    bool on = isOn();
    bool down = isDown();

    TQBrush background;

    bool liveBkgnd = m_pager->bgType() == PagerSettings::EnumBackgroundType::BgLive;
    bool transparent = m_pager->bgType() == PagerSettings::EnumBackgroundType::BgTransparent;

    // background

    if (backgroundPixmap())
    {
        TQPoint pt = backgroundOffset();
        bp->drawTiledPixmap(0, 0, width(), height(), *backgroundPixmap(), pt.x(), pt.y());
    }
    else
    {
        bp->fillRect(0, 0, width(), height(), paletteBackgroundColor());
    }
    

    // desktop background
    
    if (liveBkgnd)
    {
        if (m_bgPixmap && !m_bgPixmap->isNull())
        {
            if (on)
            {
                KPixmap tmp = *m_bgPixmap;
                KPixmapEffect::intensity(tmp, 0.33);
                bp->drawPixmap(0, 0, tmp);
            }
            else
            {
                bp->drawPixmap(0, 0, *m_bgPixmap);
            }
        }
        else
        {
            liveBkgnd = false;
        }
    }

    // frame
    if (liveBkgnd || transparent)
    {
        if (m_pager->border3D())
        {
            qDrawShadeRect(bp, 0, 0, w, h, on ? palette().active() : palette().inactive());
        }
        else
        {
            bp->setPen(on ? colorGroup().midlight()
                          : KickerLib::blendColors(colorGroup().mid(), colorGroup().midlight()));
            bp->drawRect(0, 0, w, h);
        }
    }
    else
    {
        TQBrush background;

        if (on)
        {
            background = colorGroup().brush(TQColorGroup::Midlight);
        }
        else if (down)
        {
            background = TQBrush(KickerLib::blendColors(colorGroup().mid(),
                                                colorGroup().midlight()));
        }
        else
        {
            background = colorGroup().brush(TQColorGroup::Mid);
        }

        bp->fillRect(0, 0, w, h, background);
    }

    // window preview...
    if (m_pager->desktopPreview())
    {
        TWinModule* twin = m_pager->twin();
        KWin::WindowInfo *info = 0;
        int dw = TQApplication::desktop()->width();
        int dh = TQApplication::desktop()->height();

        TQValueList<WId> windows = twin->stackingOrder();
        TQValueList<WId>::const_iterator itEnd = windows.constEnd();
        for (TQValueList<WId>::ConstIterator it = windows.constBegin(); it != itEnd; ++it)
        {
            info = m_pager->info(*it);

            if (shouldPaintWindow(info))
            {
                TQRect r = mapGeometryToViewport(*info);
                r = TQRect(r.x() * width() / dw, 2 + r.y() * height() / dh,
                          r.width() * width() / dw, r.height() * height() / dh);

                if (twin->activeWindow() == info->win())
                {
                    TQBrush brush = colorGroup().brush(TQColorGroup::Highlight);
                    qDrawShadeRect(bp, r, colorGroup(), false, 1, 0, &brush);
                }
                else
                {
                    TQBrush brush = colorGroup().brush(TQColorGroup::Button);

                    if (on)
                    {
                        brush.setColor(brush.color().light(120));
                    }

                    bp->fillRect(r, brush);
                    qDrawShadeRect(bp, r, colorGroup(), true, 1, 0);
                }

                if (m_pager->windowIcons() && r.width() > 15 && r.height() > 15)
                {
                    TQPixmap icon = KWin::icon(*it, 16, 16, true);
                    if (!icon.isNull())
                    {
                        bp->drawPixmap(r.left() + ((r.width() - 16) / 2),
                                      r.top() + ((r.height() - 16) / 2),
                                      icon);
                    }
                }
            }
        }
    }

    if (m_pager->labelType() != PagerSettings::EnumLabelType::LabelNone)
    {
        TQString label = (m_pager->labelType() == PagerSettings::EnumLabelType::LabelNumber) ?
                            TQString::number(m_desktop) : m_desktopName;
        
        if (transparent || liveBkgnd)
        {
            bp->setPen(on ? colorGroup().midlight() : colorGroup().buttonText());
            m_pager->shadowEngine()->drawText(*bp, TQRect(0, 0, w, h), AlignCenter, label, size());
        }
        else
            bp->drawText(0, 0, w, h, AlignCenter, label);
    }
    
    if (m_inside)
        KickerLib::drawBlendedRect(bp, TQRect(1, 1, width() - 2, height() - 2), colorGroup().foreground());
}

void KMiniPagerButton::mousePressEvent(TQMouseEvent * e)
{
    if (e->button() == TQt::RightButton)
    {
        // prevent LMB down -> RMB down -> LMB up sequence
        if ((e->state() & TQt::MouseButtonMask ) == TQt::NoButton)
        {
            emit showMenu(e->globalPos(), m_desktop);
            return;
        }
    }

    if (m_pager->desktopPreview())
    {
        m_pager->clickPos = e->pos();
    }

    TQButton::mousePressEvent(e);
}

void KMiniPagerButton::mouseReleaseEvent(TQMouseEvent* e)
{
    m_pager->clickPos = TQPoint();
    TQButton::mouseReleaseEvent(e);
}

void KMiniPagerButton::mouseMoveEvent(TQMouseEvent* e)
{
    if (!m_pager->desktopPreview())
    {
        return;
    }

    int dw = TQApplication::desktop()->width();
    int dh = TQApplication::desktop()->height();
    int w = width();
    int h = height();

    TQPoint pos(m_pager->clickPos.isNull() ? mapFromGlobal(TQCursor::pos()) : m_pager->clickPos);
    TQPoint p = mapPointToViewport(TQPoint(pos.x() * dw / w, pos.y() * dh / h));

    Task::Ptr wasWindow = m_currentWindow;
    m_currentWindow = TaskManager::the()->findTask(m_useViewports ? 1 : m_desktop, p);

    if (wasWindow != m_currentWindow)
    {
        KickerTip::Client::updateKickerTip();
    }

    if (m_currentWindow && !m_pager->clickPos.isNull() &&
        (m_pager->clickPos - e->pos()).manhattanLength() > TDEGlobalSettings::dndEventDelay())
    {
        TQRect r = m_currentWindow->geometry();

        // preview window height, window width
        int ww = r.width() * w / dw;
        int wh = r.height() * h / dh;
        TQPixmap windowImage(ww, wh);
        TQPainter bp(&windowImage, this);

        bp.setPen(colorGroup().foreground());
        bp.drawRect(0, 0, ww, wh);
        bp.fillRect(1, 1, ww - 2, wh - 2, colorGroup().background());

        Task::List tasklist;
        tasklist.append(m_currentWindow);
        TaskDrag* drag = new TaskDrag(tasklist, this);
        TQPoint offset(m_pager->clickPos.x() - (r.x() * w / dw),
                m_pager->clickPos.y() - (r.y() * h / dh));
        drag->setPixmap(windowImage, offset);
        drag->dragMove();

        if (isDown())
        {
            setDown(false);
        }

        m_pager->clickPos = TQPoint();
    }
}

void KMiniPagerButton::dragEnterEvent(TQDragEnterEvent* e)
{
    if (PanelDrag::canDecode(e))
    {
        // ignore container drags
        return;
    }
    else if (TaskDrag::canDecode(e))
    {
        // if it's a task drag don't switch the desktop, just accept it
        e->accept();
        setDown(true);
    }
    else
    {
        // if a dragitem is held for over a pager button for two seconds,
        // activate corresponding desktop
        m_dragSwitchTimer.start(1000, true);
        TQButton::dragEnterEvent(e);
    }
}

void KMiniPagerButton::dropEvent(TQDropEvent* e)
{
    if (TaskDrag::canDecode(e))
    {
        e->accept();
        Task::List tasks(TaskDrag::decode(e));

        if ((m_useViewports || e->source() == this) && tasks.count() == 1)
        {
            Task::Ptr task = tasks[0];
            int dw = TQApplication::desktop()->width();
            int dh = TQApplication::desktop()->height();
            int w = width();
            int h = height();
            TQRect location = mapGeometryToViewport(task->info());
            TQPoint pos = mapPointToViewport(e->pos());
            int deltaX = pos.x() - m_pager->clickPos.x();
            int deltaY = pos.y() - m_pager->clickPos.y();

            if (abs(deltaX) < 3)
            {
                deltaX = 0;
            }
            else
            {
                deltaX = deltaX * dw / w;
            }

            if (abs(deltaY) < 3)
            {
                deltaY = 0;
            }
            else
            {
                deltaY = deltaY * dh / h;
            }

            location.moveBy(deltaX, deltaY);

            XMoveWindow(x11Display(), task->window(), location.x(), location.y());
            if ((e->source() != this || !task->isOnAllDesktops()) &&
                task->desktop() != m_desktop)
            {
                task->toDesktop(m_desktop);
            }
        }
        else
        {
            Task::List::iterator itEnd = tasks.end();
            for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
            {
                (*it)->toDesktop(m_desktop);
            }
        }

        setDown(false);
    }

    TQButton::dropEvent( e );
}

void KMiniPagerButton::enabledChange( bool oldEnabled )
{
    if (m_pager->bgType() == PagerSettings::EnumBackgroundType::BgLive)
    {
        m_pager->refresh();
    }

    TQButton::enabledChange(oldEnabled);
}

void KMiniPagerButton::dragLeaveEvent( TQDragLeaveEvent* e )
{
    m_dragSwitchTimer.stop();

    if (m_pager->twin()->currentDesktop() != m_desktop)
    {
        setDown(false);
    }

    TQButton::dragLeaveEvent( e );
}

void KMiniPagerButton::slotDragSwitch()
{
    emit buttonSelected(m_desktop);
}

void KMiniPagerButton::slotClicked()
{
    emit buttonSelected(m_desktop);
}

void KMiniPagerButton::rename()
{
    if (m_pager->labelType() == PagerSettings::EnumLabelType::LabelName)
    {
        if ( !m_lineEdit ) {
            m_lineEdit = new TQLineEdit(this);
            connect(m_lineEdit, TQ_SIGNAL(returnPressed()), m_lineEdit, TQ_SLOT(hide()));
            m_lineEdit->installEventFilter(this);
        }
        m_lineEdit->setGeometry(rect());
        m_lineEdit->setText(m_desktopName);
        m_lineEdit->show();
        m_lineEdit->setFocus();
        m_lineEdit->selectAll();
        m_pager->emitRequestFocus();
    }
    else
    {
        m_pager->twin()->setDesktopName(
            m_desktop,
            TQInputDialog::getText(
                i18n("Renaming desktop %1").arg(m_desktopName),
                i18n("Enter a new name for desktop %1 (%2):").arg(m_desktop).arg(m_desktopName)
            )
        );
    }
}

void KMiniPagerButton::slotToggled( bool b )
{
    if ( !b && m_lineEdit )
    {
        m_lineEdit->hide();
    }
}

bool KMiniPagerButton::eventFilter( TQObject *o, TQEvent * e)
{
    if (o && o == m_lineEdit &&
        (e->type() == TQEvent::FocusOut || e->type() == TQEvent::Hide))
    {
        m_pager->twin()->setDesktopName( m_desktop, m_lineEdit->text() );
        m_desktopName = m_lineEdit->text();
        TQTimer::singleShot( 0, m_lineEdit, TQ_SLOT( deleteLater() ) );
        m_lineEdit = 0;
        return true;
    }

    return TQButton::eventFilter(o, e);
}

void KMiniPagerButton::updateKickerTip(KickerTip::Data &data)
{
    Task::Dict tasks = TaskManager::the()->tasks();
    Task::Dict::iterator taskEnd = tasks.end();
    uint taskCounter = 0;
    uint taskLimiter = 4;
    TQString lastWindow;

    for (Task::Dict::iterator it = tasks.begin(); it != taskEnd; ++it)
    {
        if (it.data()->desktop() == m_desktop || it.data()->isOnAllDesktops())
        {
            taskCounter++;
            if (taskCounter > taskLimiter)
            {
                lastWindow = it.data()->visibleName();
                continue;
            }

            TQPixmap winIcon = it.data()->pixmap();
            TQString bullet;

            if (winIcon.isNull())
            {
                bullet = "&bull;";
            }
            else
            {
                data.mimeFactory->setPixmap(TQString::number(taskCounter), winIcon);
                bullet = TQString("<img src=\"%1\" width=\"%2\" height=\"%3\">").arg(taskCounter).arg(16).arg(16);
            }

            TQString name = KStringHandler::cPixelSqueeze(it.data()->visibleName(), fontMetrics(), 400);
            name = TQStyleSheet::escape(name);
            if (it.data() == m_currentWindow)
            {
                data.subtext.append(TQString("<br>%1&nbsp; <u>").arg(bullet));
                data.subtext.append(name).append("</u>");
            }
            else
            {
                data.subtext.append(TQString("<br>%1&nbsp; ").arg(bullet));
                data.subtext.append(name);
            }
        }
    }

    if (taskCounter > taskLimiter)
    {
        if (taskCounter - taskLimiter == 1)
        {
            data.subtext.append("<br>&bull; ").append(lastWindow);
        }
        else
        {
            data.subtext.append("<br>&bull; <i>")
                        .append(i18n("and 1 other", "and %n others", taskCounter - taskLimiter))
                        .append("</i>");
        }
    }

    if (taskCounter > 0)
    {
        data.subtext.prepend(i18n("One window:",
                                  "%n windows:",
                                  taskCounter));
    }

    data.duration = 4000;
    data.icon = DesktopIcon("window_duplicate", TDEIcon::SizeMedium);
    data.message = TQStyleSheet::escape(m_desktopName);
    data.direction = m_pager->popupDirection();
}

