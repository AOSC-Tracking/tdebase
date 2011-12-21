/* This file is part of the KDE project
   Copyright (C) 2003-2004 Nadeem Hasan <nhasan@kde.org>
   Copyright (C) 2004-2005 Aaron J. Seigo <aseigo@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "simplebutton.h"

#include <tqpainter.h>
#include <tqstyle.h>

#include <kapplication.h>
#include <kcursor.h>
#include <kdialog.h>
#include <kglobalsettings.h>
#include <kiconeffect.h>
#include <kicontheme.h>
#include <kipc.h>
#include <kstandarddirs.h>

#include "kickerSettings.h"

#define BUTTON_MARGIN KDialog::spacingHint()

// For now link these two
#define m_disableHighlighting m_forceStandardCursor

SimpleButton::SimpleButton(TQWidget *parent, const char *name, bool forceStandardCursor)
    : TQButton(parent, name),
      m_highlight(false),
      m_orientation(Qt::Horizontal),
      m_forceStandardCursor(forceStandardCursor)
{
    setBackgroundOrigin( AncestorOrigin );

    connect( kapp, TQT_SIGNAL( settingsChanged( int ) ),
       TQT_SLOT( slotSettingsChanged( int ) ) );
    connect( kapp, TQT_SIGNAL( iconChanged( int ) ),
       TQT_SLOT( slotIconChanged( int ) ) );

    kapp->addKipcEventMask( KIPC::SettingsChanged );
    kapp->addKipcEventMask( KIPC::IconChanged );

    slotSettingsChanged( KApplication::SETTINGS_MOUSE );
}

void SimpleButton::setPixmap(const TQPixmap &pix)
{
    TQButton::setPixmap(pix);
    generateIcons();
    update();
}

void SimpleButton::setOrientation(Qt::Orientation orientation)
{
    m_orientation = orientation;
    update();
}

TQSize SimpleButton::sizeHint() const
{
    const TQPixmap* pm = pixmap();

    if (!pm)
        return TQButton::sizeHint();
    else
        return TQSize(pm->width() + BUTTON_MARGIN, pm->height() + BUTTON_MARGIN);
}

TQSize SimpleButton::minimumSizeHint() const
{
    const TQPixmap* pm = pixmap();

    if (!pm)
        return TQButton::minimumSizeHint();
    else
        return TQSize(pm->width(), pm->height());
}

void SimpleButton::drawButton( TQPainter *p )
{
    TQRect r(0, 0, width(), height());

    if (m_disableHighlighting == TRUE) {
        if (m_highlight || isDown() || isOn()) {
            int flags = TQStyle::Style_Default | TQStyle::Style_Enabled;
            if (isDown() || isOn()) flags |= TQStyle::Style_Down;
            style().tqdrawPrimitive(TQStyle::PE_ButtonTool, p, r, colorGroup(), flags);
        }
    }

    drawButtonLabel(p);
}

void SimpleButton::drawButtonLabel( TQPainter *p )
{
    if (!pixmap())
    {
        return;
    }

    TQPixmap pix = isEnabled() ? ((m_highlight&&(!m_disableHighlighting))? m_activeIcon : m_normalIcon) : m_disabledIcon;

    if ((isOn() || isDown()) && (m_disableHighlighting == FALSE))
    {
        pix = TQImage(pix.convertToImage()).smoothScale(pix.width() - 2,
                                               pix.height() - 2);
    }

    if (m_disableHighlighting == TRUE) {
        pix = TQImage(pix.convertToImage()).smoothScale(pix.width() - 4,
                                               pix.height() - 4);
    }

    int h = height();
    int w = width();
    int ph = pix.height();
    int pw = pix.width();
    int margin = BUTTON_MARGIN;
    TQPoint origin(margin / 2, margin / 2);

    if (ph < (h - margin))
    {
        origin.setY((h - ph) / 2);
    }

    if (pw < (w - margin))
    {
        origin.setX((w - pw) / 2);
    }

    p->drawPixmap(origin, pix);
}

void SimpleButton::generateIcons()
{
    if (!pixmap())
    {
        return;
    }

    TQImage image = pixmap()->convertToImage();
    KIconEffect effect;

    m_normalIcon = effect.apply(image, KIcon::Panel, KIcon::DefaultState);
    m_activeIcon = effect.apply(image, KIcon::Panel, KIcon::ActiveState);
    m_disabledIcon = effect.apply(image, KIcon::Panel, KIcon::DisabledState);
    
    updateGeometry();
}

void SimpleButton::slotSettingsChanged(int category)
{
    if (category != KApplication::SETTINGS_MOUSE)
    {
        return;
    }

    bool changeCursor;
    if (m_forceStandardCursor == FALSE)
        changeCursor = KGlobalSettings::changeCursorOverIcon();
    else
        changeCursor = FALSE;

    if (changeCursor)
    {
        setCursor(KCursor::handCursor());
    }
    else
    {
        unsetCursor();
    }
}

void SimpleButton::slotIconChanged( int group )
{
    if (group != KIcon::Panel)
    {
        return;
    }

    generateIcons();
    update();
}

void SimpleButton::enterEvent( TQEvent *e )
{
    if (KickerSettings::showMouseOverEffects())
        m_highlight = true;

    repaint( false );
    TQButton::enterEvent( e );
}

void SimpleButton::leaveEvent( TQEvent *e )
{
    m_highlight = false;

    repaint( false );
    TQButton::enterEvent( e );
}

void SimpleButton::resizeEvent( TQResizeEvent * )
{
    generateIcons();
}


SimpleArrowButton::SimpleArrowButton(TQWidget *parent, Qt::ArrowType arrow, const char *name, bool forceStandardCursor)
    : SimpleButton(parent, name, forceStandardCursor),
      m_forceStandardCursor(forceStandardCursor)
{
    setBackgroundOrigin(AncestorOrigin);
    _arrow = arrow;
    _inside = false;
}

TQSize SimpleArrowButton::sizeHint() const
{
    return TQSize( 12, 12 );
}

void SimpleArrowButton::setArrowType(Qt::ArrowType a)
{
    if (_arrow != a) 
    {
        _arrow = a;
        update();
    }
}

Qt::ArrowType SimpleArrowButton::arrowType() const
{
    return _arrow;
}

void SimpleArrowButton::drawButton( TQPainter *p )
{
    TQRect r(1, 1, width() - 2, height() - 2);

    TQStyle::PrimitiveElement pe = TQStyle::PE_ArrowLeft;
    switch (_arrow)
    {
        case Qt::LeftArrow: pe = TQStyle::PE_ArrowLeft; break;
        case Qt::RightArrow: pe = TQStyle::PE_ArrowRight; break;
        case Qt::UpArrow: pe = TQStyle::PE_ArrowUp; break;
        case Qt::DownArrow: pe = TQStyle::PE_ArrowDown; break;
    }

    int flags = TQStyle::Style_Default | TQStyle::Style_Enabled;
    if (isDown() || isOn())	flags |= TQStyle::Style_Down;
    style().tqdrawPrimitive(pe, p, r, colorGroup(), flags);

    if (m_forceStandardCursor) {
        SimpleButton::drawButton(p);
    }
}

void SimpleArrowButton::enterEvent( TQEvent *e )
{
    _inside = true;
    SimpleButton::enterEvent( e );
    update();
}

void SimpleArrowButton::leaveEvent( TQEvent *e )
{
    _inside = false;
    SimpleButton::leaveEvent( e );
    update();
}

#include "simplebutton.moc"

// vim:ts=4:sw=4:et
