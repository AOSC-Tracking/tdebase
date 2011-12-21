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

#undef None	// Qt4
#include <tqcursor.h>
#include <tqfile.h>
#include <tqfontmetrics.h>
#include <tqpainter.h>
#include <tqpopupmenu.h>
#include <tqstyle.h>
#include <tqstylesheet.h>
#include <tqtooltip.h>
#include <tqpixmap.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdialog.h>
#include <kdirwatch.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kicontheme.h>
#include <kiconeffect.h>
#include <kipc.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>

#include "global.h"

#include "kshadowengine.h"
#include "kshadowsettings.h"

#include "kickerSettings.h"
#include "panelbutton.h"
#include "panelbutton.moc"

// For now link these two
#define m_disableHighlighting m_forceStandardCursor

// init static variable
KShadowEngine* PanelButton::s_textShadowEngine = 0L;

PanelButton::PanelButton( TQWidget* parent, const char* name, bool forceStandardCursor )
    : TQButton(parent, name),
      m_valid(true),
      m_isLeftMouseButtonDown(false),
      m_drawArrow(false),
      m_highlight(false),
      m_changeCursorOverItem(forceStandardCursor?false:true),
      m_hasAcceptedDrag(false),
      m_arrowDirection(KPanelExtension::Bottom),
      m_popupDirection(KPanelApplet::Up),
      m_iconAlignment(AlignCenter),
      m_orientation(Qt::Horizontal),
      m_size((KIcon::StdSizes)-1),
      m_fontPercent(0.40),
      m_forceStandardCursor(forceStandardCursor)
{
    setBackgroundOrigin(AncestorOrigin);
    setWFlags(TQt::WNoAutoErase);
    KGlobal::locale()->insertCatalogue("libkicker");
    calculateIconSize();
    setAcceptDrops(true);

    m_textColor = KGlobalSettings::textColor();

    updateSettings(KApplication::SETTINGS_MOUSE);

    kapp->addKipcEventMask(KIPC::SettingsChanged | KIPC::IconChanged);

    installEventFilter(KickerTip::the());

    connect(kapp, TQT_SIGNAL(settingsChanged(int)), TQT_SLOT(updateSettings(int)));
    connect(kapp, TQT_SIGNAL(iconChanged(int)), TQT_SLOT(updateIcon(int)));
}

void PanelButton::configure()
{
    TQString name = tileName();
    if( name.isEmpty() )
        return;

    if (!KickerSettings::enableTileBackground())
    {
        setTile(TQString::null);
        return;
    }

    KConfigGroup tilesGroup( KGlobal::config(), "button_tiles" );
    if( !tilesGroup.readBoolEntry( "Enable" + name + "Tiles", true ) ) {
            setTile( TQString::null );
        return;
    }

    TQString tile = tilesGroup.readEntry( name + "Tile" );
    TQColor color = TQColor();

    if (tile == "Colorize")
    {
        color = tilesGroup.readColorEntry( name + "TileColor" );
        tile = TQString::null;
    }

    setTile( tile, color );
}

void PanelButton::setTile(const TQString& tile, const TQColor& color)
{
    if (tile == m_tile && m_tileColor == color)
    {
        return;
    }

    m_tile = tile;
    m_tileColor = color;
    loadTiles();
    update();
}

void PanelButton::setDrawArrow(bool drawArrow)
{
    if (m_drawArrow == drawArrow)
    {
        return;
    }

    m_drawArrow = drawArrow;
    update();
}

TQImage PanelButton::loadTile(const TQString& tile,
                             const TQSize& size,
                             const TQString& state)
{
    TQString name = tile;

    if (size.height() < 42)
    {
        name += "_tiny_";
    }
    else if (size.height() < 54)
    {
        name += "_normal_";
    }
    else
    {
        name += "_large_";
    }

    name += state + ".png";

    TQImage tileImg(KGlobal::dirs()->findResource("tiles", name));

    // scale if size does not match exactly
    if (!tileImg.isNull() && tileImg.size() != size)
    {
        tileImg = tileImg.smoothScale(size);
    }

    return tileImg;
}

void PanelButton::setEnabled(bool enable)
{
    TQButton::setEnabled(enable);
    loadIcons();
    update();
}

void PanelButton::setPopupDirection(KPanelApplet::Direction d)
{
    m_popupDirection = d;
    setArrowDirection(KickerLib::directionToPopupPosition(d));
}

void PanelButton::setIconAlignment(TQ_Alignment align)
{
    m_iconAlignment = align;
    update();
}

void PanelButton::setOrientation(Orientation o)
{
    m_orientation = o;
}

void PanelButton::updateIcon(int group)
{
    if (group != KIcon::Panel)
    {
        return;
    }

    loadIcons();
    update();
}

void PanelButton::updateSettings(int category)
{
    if (category != KApplication::SETTINGS_MOUSE)
    {
        return;
    }

    if (m_forceStandardCursor == FALSE)
        m_changeCursorOverItem = KGlobalSettings::changeCursorOverIcon();
    else
        m_changeCursorOverItem = FALSE;

    if (m_changeCursorOverItem)
    {
        setCursor(KCursor::handCursor());
    }
    else
    {
        unsetCursor();
    }
}

void PanelButton::checkForDeletion(const TQString& path)
{
    if (path == m_backingFile)
    {
        setEnabled(false);
        TQTimer::singleShot(1000, this, TQT_SLOT(scheduleForRemoval()));
    }
}

bool PanelButton::checkForBackingFile()
{
    return TQFile::exists(m_backingFile);
}

void PanelButton::scheduleForRemoval()
{
    static int timelapse = 1000;
    if (checkForBackingFile())
    {
        setEnabled(true);
        timelapse = 1000;
        emit hideme(false);
        return;
    }
    else if (KickerSettings::removeButtonsWhenBroken())
    {
        if (timelapse > 255*1000) // we'v given it ~8.5 minutes by this point
        {
            emit removeme();
            return;
        }

        if (timelapse > 3000 && isVisible())
        {
            emit hideme(true);
        }

        timelapse *= 2;
        TQTimer::singleShot(timelapse, this, TQT_SLOT(scheduleForRemoval()));
    }
}

// return the dimension that the button wants to be for a given panel dimension (panelDim)
int PanelButton::preferredDimension(int panelDim) const
{
    // determine the upper limit on the size.  Normally, this is panelDim,
    // but if conserveSpace() is true, we restrict size to comfortably fit the icon
    if (KickerSettings::conserveSpace())
    {
        int newSize = preferredIconSize(panelDim);
        if (newSize > 0)
        {
            return QMIN(panelDim, newSize + (KDialog::spacingHint() * 2));
        }
    }

    return panelDim;
}

int PanelButton::widthForHeight(int height) const
{
    int rc = preferredDimension(height);

    // we only paint the text when horizontal, so make sure we're horizontal
    // before adding the text in here
    if (orientation() == Qt::Horizontal && !m_buttonText.isEmpty())
    {
        TQFont f(font());
        //f.setPixelSize(KMIN(height, KMAX(int(float(height) * m_fontPercent), 16)));
        TQFontMetrics fm(f);

        //rc += fm.width(m_buttonText) + KMIN(25, KMAX(5, fm.width('m') / 2));
        rc += fm.width(m_buttonText);
    }

    return rc;
}

int PanelButton::heightForWidth(int width) const
{
    int rc=preferredDimension(width);

    return rc;
}

const TQPixmap& PanelButton::labelIcon() const
{
    if (m_disableHighlighting || (!KickerSettings::showMouseOverEffects()))
        return m_icon;
    else
        return m_highlight ? m_iconh : m_icon;
}

const TQPixmap& PanelButton::zoomIcon() const
{
    return m_iconz;
}

bool PanelButton::isValid() const
{
    return m_valid;
}

void PanelButton::setTitle(const TQString& t)
{
    m_title = t;
}

void PanelButton::setIcon(const TQString& icon)
{
    if (icon == m_iconName)
    {
        return;
    }

    m_iconName = icon;
    loadIcons();
    update();
    emit iconChanged();
}

TQString PanelButton::icon() const
{
    return m_iconName;
}

bool PanelButton::hasText() const
{
    return !m_buttonText.isEmpty();
}

void PanelButton::setButtonText(const TQString& text)
{
    m_buttonText = " " + text;
    update();
}

TQString PanelButton::buttonText() const
{
    return m_buttonText;
}

void PanelButton::setTextColor(const TQColor& c)
{
    m_textColor = c;
}

TQColor PanelButton::textColor() const
{
    return m_textColor;
}

void PanelButton::setFontPercent(double p)
{
    m_fontPercent = p;
}

double PanelButton::fontPercent() const
{
    return m_fontPercent;
}

KPanelExtension::Orientation PanelButton::orientation() const
{
    return m_orientation;
}

KPanelApplet::Direction PanelButton::popupDirection() const
{
    return m_popupDirection;
}

TQPoint PanelButton::center() const
{
    return mapToGlobal(rect().center());
}

TQString PanelButton::title() const
{
    return m_title;
}

void PanelButton::triggerDrag()
{
    setDown(false);

    startDrag();
}

void PanelButton::startDrag()
{
    emit dragme(m_icon);
}

void PanelButton::enterEvent(TQEvent* e)
{
    if (!m_highlight && m_disableHighlighting == FALSE)
    {
        m_highlight = true;
        repaint(false);
    }

    TQButton::enterEvent(e);
}

void PanelButton::leaveEvent(TQEvent* e)
{
    if (m_highlight)
    {
        m_highlight = false;
        repaint(false);
    }

    TQButton::leaveEvent(e);
}

void PanelButton::dragEnterEvent(TQDragEnterEvent* e)
{
    if (e->isAccepted())
    {
        m_hasAcceptedDrag = true;
    }

    update();
    TQButton::dragEnterEvent( e );
}

void PanelButton::dragLeaveEvent(TQDragLeaveEvent* e)
{
    m_hasAcceptedDrag = false;
    update();
    TQButton::dragLeaveEvent( e );
}

void PanelButton::dropEvent(TQDropEvent* e)
{
    m_hasAcceptedDrag = false;
    update();
    TQButton::dropEvent( e );
}

void PanelButton::mouseMoveEvent(TQMouseEvent *e)
{
    if (!m_isLeftMouseButtonDown || (e->state() & Qt::LeftButton) == 0)
    {
        return;
    }

    TQPoint p(e->pos() - m_lastLeftMouseButtonPress);
    if (p.manhattanLength() <= 16)
    {
        // KGlobalSettings::dndEventDelay() is not enough!
        return;
    }

    m_isLeftMouseButtonDown = false;
    triggerDrag();
}

void PanelButton::mousePressEvent(TQMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        m_lastLeftMouseButtonPress = e->pos();
        m_isLeftMouseButtonDown = true;
    }
    TQButton::mousePressEvent(e);
}

void PanelButton::mouseReleaseEvent(TQMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        m_isLeftMouseButtonDown = false;
	
	TQPixmap pix = labelIcon();
	if (KickerSettings::showIconActivationEffect() == true) {
		KIconEffect::visualActivate(this, this->geometry(), &pix);
	}
    }
    TQButton::mouseReleaseEvent(e);
}

void PanelButton::resizeEvent(TQResizeEvent*)
{
    loadTiles();

    if (calculateIconSize())
    {
        loadIcons();
    }
}

void PanelButton::drawButton(TQPainter *p)
{
    const TQPixmap& tile = (isDown() || isOn()) ? m_down : m_up;
    
    if (m_tileColor.isValid())
    {
        p->fillRect(rect(), m_tileColor);
        style().tqdrawPrimitive(TQStyle::PE_Panel, p, rect(), colorGroup());
    }
    else if (paletteBackgroundPixmap())
    {
        // Draw the background. This is always needed, even when using tiles,
        // because they don't have to cover the entire button.
        TQPoint offset = backgroundOffset();
        int ox = offset.x();
        int oy = offset.y();
        p->drawTiledPixmap( 0, 0, width(), height(),*paletteBackgroundPixmap(), ox, oy);
    }

    if (!tile.isNull())
    {
        // Draw the tile.
        p->drawPixmap(0, 0, tile);
    }
    else if (isDown() || isOn())
    {
        // Draw shapes to indicate the down state.
        style().tqdrawPrimitive(TQStyle::PE_Panel, p, rect(), colorGroup(), TQStyle::Style_Sunken);
    }

    drawButtonLabel(p);

    if (hasFocus() || m_hasAcceptedDrag)
    {
        int x1, y1, x2, y2;
        TQT_TQRECT_OBJECT(rect()).coords(&x1, &y1, &x2, &y2);
        TQRect r(x1+2, y1+2, x2-x1-3, y2-y1-3);
        style().tqdrawPrimitive(TQStyle::PE_FocusRect, p, r, colorGroup(),
        TQStyle::Style_Default, colorGroup().button());
    }
}

void PanelButton::drawDeepButton(TQPainter *p)
{
    const TQPixmap& tile = (isDown() || isOn()) ? m_down : m_up;
    
    if (m_tileColor.isValid())
    {
        p->fillRect(rect(), m_tileColor);
        style().tqdrawPrimitive(TQStyle::PE_Panel, p, rect(), colorGroup());
    }
    else if (paletteBackgroundPixmap())
    {
        // Draw the background. This is always needed, even when using tiles,
        // because they don't have to cover the entire button.
        TQPoint offset = backgroundOffset();
        int ox = offset.x();
        int oy = offset.y();
        p->drawTiledPixmap( 0, 0, width(), height(),*paletteBackgroundPixmap(), ox, oy);
    }

    TQRect btn_rect = TQRect(rect().x(), rect().y()+1, rect().width(), rect().height()-2);
    if (isDown() || isOn()) {
        style().tqdrawPrimitive(TQStyle::PE_ButtonBevel, p, btn_rect, colorGroup(), TQStyle::Style_Down);
    }
    else {
        style().tqdrawPrimitive(TQStyle::PE_ButtonBevel, p, btn_rect, colorGroup(), TQStyle::Style_Raised);
    }

    drawButtonLabel(p,0,FALSE);

    if (hasFocus() || m_hasAcceptedDrag)
    {
        int x1, y1, x2, y2;
        TQT_TQRECT_OBJECT(rect()).coords(&x1, &y1, &x2, &y2);
        TQRect r(x1+2, y1+2, x2-x1-3, y2-y1-3);
        style().tqdrawPrimitive(TQStyle::PE_FocusRect, p, r, colorGroup(),
        TQStyle::Style_Default, colorGroup().button());
    }
}

void PanelButton::drawButtonLabel(TQPainter *p)
{
    drawButtonLabel(p,0,TRUE);
}

void PanelButton::drawButtonLabel(TQPainter *p, int voffset, bool drawArrow)
{
    TQPixmap icon = labelIcon();
    bool active = isDown() || isOn();

    if (active)
    {
        icon = TQImage(icon.convertToImage()).smoothScale(icon.width() - 2,
                                                 icon.height() - 2);
    }

    int y = 0;
    if (m_iconAlignment & AlignVCenter) 
        y = (height() - icon.height()) / 2;
    else if (m_iconAlignment & AlignBottom)
        y = (height() - icon.height());

    if (!m_buttonText.isEmpty() && orientation() == Qt::Horizontal)
    {
        int h = height();
        int w = width();
        p->save();
        TQFont f = font();

        double fontPercent = m_fontPercent;
        if (active)
        {
            fontPercent *= .8;
        }
        //f.setPixelSize(KMIN(h, KMAX(int(float(h) * m_fontPercent), 16)));
        TQFontMetrics fm(f);
        p->setFont(f);

        /* Draw shadowed text */
        bool reverse = TQApplication::reverseLayout();
        TQPainter::TextDirection rtl = reverse ? TQPainter::RTL : TQPainter::LTR;

        if (!reverse && !icon.isNull())
        {
            /* Draw icon */
            p->drawPixmap(3, y+voffset, icon);
        }

        int tX = reverse ? 3 : icon.width() + KMIN(25, KMAX(5, fm.width('m') / 2));
        int tY = fm.ascent() + ((h - fm.height()) / 2);

        TQColor shadCol = KickerLib::shadowColor(m_textColor);

        // get a transparent pixmap
        TQPainter pixPainter;
        TQPixmap textPixmap(w, h);

        textPixmap.fill(TQColor(0,0,0));
        textPixmap.setMask(textPixmap.createHeuristicMask(true));

        // draw text
        pixPainter.begin(&textPixmap);
        pixPainter.setPen(m_textColor);
        pixPainter.setFont(p->font()); // get the font from the root painter
        pixPainter.drawText(tX, tY, m_buttonText, -1, rtl);
        pixPainter.end();

        if (!s_textShadowEngine)
        {
            KShadowSettings* shadset = new KShadowSettings();
            shadset->setOffsetX(0);
            shadset->setOffsetY(0);
            shadset->setThickness(1);
            shadset->setMaxOpacity(96);
            s_textShadowEngine = new KShadowEngine(shadset);
        }

        // draw shadow
        TQImage img = s_textShadowEngine->makeShadow(textPixmap, shadCol);
        p->drawImage(0, 0, img);
        p->save();
        p->setPen(m_textColor);
        p->drawText(tX, tY+voffset, m_buttonText, -1, rtl);
        p->restore();

        if (reverse && !icon.isNull())
        {
            p->drawPixmap(w - icon.width() - 3, y+voffset, icon);
        }

        p->restore();
    }
    else if (!icon.isNull())
    {
        int x = 0;
        if (m_iconAlignment & AlignHCenter)
           x = (width()  - icon.width()) / 2;
        else if (m_iconAlignment & AlignRight)
           x = (width() - icon.width());
        p->drawPixmap(x, y+voffset, icon);
    }

    if (m_drawArrow && (m_highlight || active) && drawArrow)
    {
        TQStyle::PrimitiveElement e = TQStyle::PE_ArrowUp;
        int arrowSize = style().pixelMetric(TQStyle::PM_MenuButtonIndicator);
        TQRect r((width() - arrowSize)/2, 0, arrowSize, arrowSize);

        switch (m_arrowDirection)
        {
            case KPanelExtension::Top:
                e = TQStyle::PE_ArrowUp;
                break;
            case KPanelExtension::Bottom:
                e = TQStyle::PE_ArrowDown;
                r.moveBy(0, height() - arrowSize);
                break;
            case KPanelExtension::Right:
                e = TQStyle::PE_ArrowRight;
                r = TQRect(width() - arrowSize, (height() - arrowSize)/2, arrowSize, arrowSize);
                break;
            case KPanelExtension::Left:
                e = TQStyle::PE_ArrowLeft;
                r = TQRect(0, (height() - arrowSize)/2, arrowSize, arrowSize);
                break;
            case KPanelExtension::Floating:
                if (orientation() == Qt::Horizontal)
                {
                    e = TQStyle::PE_ArrowDown;
                    r.moveBy(0, height() - arrowSize);
                }
                else if (TQApplication::reverseLayout())
                {
                    e = TQStyle::PE_ArrowLeft;
                    r = TQRect(0, (height() - arrowSize)/2, arrowSize, arrowSize);
                }
                else
                {
                    e = TQStyle::PE_ArrowRight;
                    r = TQRect(width() - arrowSize, (height() - arrowSize)/2, arrowSize, arrowSize);
                }
                break;
        }

        int flags = TQStyle::Style_Enabled;
        if (active)
        {
            flags |= TQStyle::Style_Down;
        }
        style().tqdrawPrimitive(e, p, r, colorGroup(), flags);
    }
}

// return the icon size that would be used if the panel were proposed_size
// if proposed_size==-1, use the current panel size instead
int PanelButton::preferredIconSize(int proposed_size) const
{
    // (re)calculates the icon sizes and report true if they have changed.
    // Get sizes from icontheme. We assume they are sorted.
    KIconTheme *ith = KGlobal::iconLoader()->theme();

    if (!ith)
    {
        return -1; // unknown icon size
    }

    TQValueList<int> sizes = ith->querySizes(KIcon::Panel);

    int sz = ith->defaultSize(KIcon::Panel);

    if (proposed_size < 0)
    {
        proposed_size = (orientation() == Qt::Horizontal) ? height() : width();
    }

    // determine the upper limit on the size.  Normally, this is panelSize,
    // but if conserve space is requested, the max button size is used instead.
    int upperLimit = proposed_size;
    if (proposed_size > KickerLib::maxButtonDim() &&
        KickerSettings::conserveSpace())
    {
        upperLimit = KickerLib::maxButtonDim();
    }

    //kdDebug()<<endl<<endl<<flush;
    TQValueListConstIterator<int> i = sizes.constBegin();
    while (i != sizes.constEnd())
    {
        if ((*i) + (2 * KickerSettings::iconMargin()) > upperLimit)
        {
            break;
        }
        sz = *i;   // get the largest size under the limit
        ++i;
    }

    //kdDebug()<<"Using icon sizes: "<<sz<<"  "<<zoom_sz<<endl<<flush;
    return sz;
}

void PanelButton::backedByFile(const TQString& localFilePath)
{
    m_backingFile = localFilePath;

    if (m_backingFile.isEmpty())
    {
        return;
    }

    // avoid multiple connections
    disconnect(KDirWatch::self(), TQT_SIGNAL(deleted(const TQString&)),
               this, TQT_SLOT(checkForDeletion(const TQString&)));

    if (!KDirWatch::self()->contains(m_backingFile))
    {
        KDirWatch::self()->addFile(m_backingFile);
    }

    connect(KDirWatch::self(), TQT_SIGNAL(deleted(const TQString&)),
            this, TQT_SLOT(checkForDeletion(const TQString&)));

}

void PanelButton::setArrowDirection(KPanelExtension::Position dir)
{
    if (m_arrowDirection != dir)
    {
        m_arrowDirection = dir;
        update();
    }
}

void PanelButton::loadTiles()
{
    if (m_tileColor.isValid())
    {
        setBackgroundOrigin(WidgetOrigin);
        m_up = m_down = TQPixmap();
    }
    else if (m_tile.isNull())
    {
        setBackgroundOrigin(AncestorOrigin);
        m_up = m_down = TQPixmap();
    }
    else
    {
        setBackgroundOrigin(WidgetOrigin);
        // If only the tiles were named a bit smarter we wouldn't have
        // to pass the up or down argument.
        m_up   = TQPixmap(loadTile(m_tile, size(), "up"));
        m_down = TQPixmap(loadTile(m_tile, size(), "down"));
    }
}

void PanelButton::loadIcons()
{
    KIconLoader * ldr = KGlobal::iconLoader();
    TQString nm = m_iconName;
    KIcon::States defaultState = isEnabled() ? KIcon::DefaultState :
                                               KIcon::DisabledState;
    if (nm=="kmenu-suse")
    {
        TQString pth = locate( "data", "kicker/pics/kmenu_basic.mng" );
        if (!pth.isEmpty())
        {
            m_icon = TQImage(pth);
            m_iconh = TQPixmap(m_icon);
            m_iconz = TQPixmap(m_icon);
            return;
        }
    }
    else
        m_icon = ldr->loadIcon(nm, KIcon::Panel, m_size, defaultState, 0L, true);

    if (m_icon.isNull())
    {
        nm = defaultIcon();
        m_icon = ldr->loadIcon(nm, KIcon::Panel, m_size, defaultState);
    }

    if (!isEnabled())
    {
        m_iconh = m_icon;
    }
    else
    {
        m_iconh = ldr->loadIcon(nm, KIcon::Panel, m_size,
                                KIcon::ActiveState, 0L, true);
    }

    m_iconz = ldr->loadIcon(nm, KIcon::Panel, KIcon::SizeHuge,
                            defaultState, 0L, true );
}

// (re)calculates the icon sizes and report true if they have changed.
//      (false if we don't know, because theme couldn't be loaded?)
bool PanelButton::calculateIconSize()
{
    int size = preferredIconSize();

    if (size < 0)
    {
        // size unknown
        return false;
    }

    if (m_size != size)
    {
        // Size has changed, update
        m_size = size;
        return true;
    }

    return false;
}

void PanelButton::updateKickerTip(KickerTip::Data& data)
{
    data.message = TQStyleSheet::escape(title());
    data.subtext = TQStyleSheet::escape(TQToolTip::textFor(this));
    data.icon = zoomIcon();
    data.direction = popupDirection();
}

//
// PanelPopupButton class
//

PanelPopupButton::PanelPopupButton(TQWidget *parent, const char *name, bool forceStandardCursor)
  : PanelButton(parent, name, forceStandardCursor),
    m_popup(0),
    m_pressedDuringPopup(false),
    m_initialized(false)
{
    connect(this, TQT_SIGNAL(pressed()), TQT_SLOT(slotExecMenu()));
}

void PanelPopupButton::setPopup(TQWidget *popup)
{
    if (m_popup)
    {
        m_popup->removeEventFilter(this);
        disconnect(m_popup, TQT_SIGNAL(aboutToHide()), this, TQT_SLOT(menuAboutToHide()));
    }

    m_popup = popup;
    setDrawArrow(m_popup != 0);

    if (m_popup)
    {
        m_popup->installEventFilter(this);
        connect(m_popup, TQT_SIGNAL(aboutToHide()), this, TQT_SLOT(menuAboutToHide()));
    }
}

TQWidget *PanelPopupButton::popup() const
{
    return m_popup;
}

bool PanelPopupButton::eventFilter(TQObject *, TQEvent *e)
{
    if (e->type() == TQEvent::MouseMove)
    {
        TQMouseEvent *me = TQT_TQMOUSEEVENT(e);
        if (TQT_TQRECT_OBJECT(rect()).contains(mapFromGlobal(me->globalPos())) &&
            ((me->state() & ControlButton) != 0 ||
             (me->state() & ShiftButton) != 0))
        {
            PanelButton::mouseMoveEvent(me);
            return true;
        }
    }
    else if (e->type() == TQEvent::MouseButtonPress ||
             e->type() == TQEvent::MouseButtonDblClick)
    {
        TQMouseEvent *me = TQT_TQMOUSEEVENT(e);
        if (TQT_TQRECT_OBJECT(rect()).contains(mapFromGlobal(me->globalPos())))
        {
            m_pressedDuringPopup = true;
            return true;
        }
    }
    else if (e->type() == TQEvent::MouseButtonRelease)
    {
        TQMouseEvent *me = TQT_TQMOUSEEVENT(e);
        if (TQT_TQRECT_OBJECT(rect()).contains(mapFromGlobal(me->globalPos())))
        {
            if (m_pressedDuringPopup && m_popup)
            {
                m_popup->hide();
            }
            return true;
        }
    }
    return false;
}

void PanelPopupButton::showMenu()
{
    if (isDown())
    {
        if (m_popup)
        {
            m_popup->hide();
        }

        setDown(false);
        return;
    }

    setDown(true);
    update();
    slotExecMenu();
}

void PanelPopupButton::slotExecMenu()
{
    if (!m_popup)
    {
        return;
    }

    m_pressedDuringPopup = false;
    KickerTip::enableTipping(false);
    kapp->syncX();
    kapp->processEvents();

    if (!m_initialized)
    {
        initPopup();
    }

    m_popup->adjustSize();
    if(dynamic_cast<TQPopupMenu*>(m_popup))
        static_cast<TQPopupMenu*>(m_popup)->exec(KickerLib::popupPosition(popupDirection(), m_popup, this));
    // else.. hmm. some derived class has to fix it.
}

void PanelPopupButton::menuAboutToHide()
{
    if (!m_popup)
    {
        return;
    }

    if (isDown()) {
        setDown(false);
        KickerTip::enableTipping(true);
    }
}

void PanelPopupButton::triggerDrag()
{
    if (m_popup)
    {
        m_popup->hide();
    }

    PanelButton::triggerDrag();
}

void PanelPopupButton::setInitialized(bool initialized)
{
    m_initialized = initialized;
}



