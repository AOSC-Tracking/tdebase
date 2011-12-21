/*****************************************************************

   Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.
   Copyright (c) 2006 Dirk Mueller <mueller@kde.org>

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

******************************************************************/

#include "kickoff_bar.h"
#include "itemview.h"

#include <tqiconset.h>
#include <tqpainter.h>
#include <tqcursor.h>
#include <tqstyle.h>
#include <tqapplication.h>

#include <kdebug.h>
#include "kickerSettings.h"

KickoffTabBar::KickoffTabBar(TQWidget* parent, const char* name)
        : TQTabBar(parent, name), m_tabsActivated(true)
{
    setAcceptDrops(true);
}

void KickoffTabBar::deactivateTabs(bool b)
{
    m_tabsActivated = !b;

    update();
}

void KickoffTabBar::paint(TQPainter* p, TQTab* t, bool selected) const
{
    TQStyle::SFlags flags = TQStyle::Style_Default;

    if (isEnabled() && t->isEnabled())
        flags |= TQStyle::Style_Enabled;
    if ( m_tabsActivated && selected )
        flags |= TQStyle::Style_Selected;
//    else if(t == d->pressed)
//        flags |= TQStyle::Style_Sunken;
    //selection flags
    if(t->rect().contains(mapFromGlobal(TQCursor::pos())))
        flags |= TQStyle::Style_MouseOver;
    style().drawControl( TQStyle::CE_TabBarTab, p, this, t->rect(),
            colorGroup(), flags, TQStyleOption(t) );

    paintLabel( p, t->rect(), t, t->identifier() == keyboardFocusTab() );
}


void KickoffTabBar::paintLabel(TQPainter* p, const TQRect& br, TQTab* t, bool has_focus) const
{
    TQRect r = br;

    bool selected = m_tabsActivated && (currentTab() == t->identifier());
    int vframe = style().pixelMetric( TQStyle::PM_TabBarTabVSpace, this );

    p->setFont( font() );
    TQFontMetrics fm = p->fontMetrics();
    int fw = fm.size( TQt::SingleLine|TQt::ShowPrefix, t->text() ).width();

    TQRect rt(r);
    rt.setWidth(fw);

    if ( t->iconSet()) 
    {
        // the tab has an iconset, draw it in the right mode
        TQIconSet::Mode mode = (t->isEnabled() && isEnabled())
            ? TQIconSet::Normal : TQIconSet::Disabled;
        if ( mode == TQIconSet::Normal && has_focus )
            mode = TQIconSet::Active;
        TQPixmap pixmap = t->iconSet()->pixmap( TQIconSet::Large, mode );
        int pixw = pixmap.width();
        int pixh = pixmap.height();
        int xoff = br.x() + (br.width() - pixw)/2;
        int yoff = br.y() + (br.height() - 4 - pixh - ((KickerSettings::kickoffTabBarFormat() != KickerSettings::IconOnly) ? fm.height() : 0) - vframe)/2;

        p->drawPixmap( xoff, 4 + yoff, pixmap );

        r.setTop(vframe/2 + yoff + pixh - 8);
        rt.setTop(vframe/2 + yoff + pixh - 8);
        rt.setHeight(((KickerSettings::kickoffTabBarFormat() != KickerSettings::IconOnly) ? fm.height() : 0) + vframe/2);
    }
    else
        rt.setHeight(vframe/2 + fm.height());

    rt.setWidth(fw+8);
    rt.moveCenter(r.center());

    TQStyle::SFlags flags = TQStyle::Style_Default;

    if (isEnabled() && t->isEnabled())
        flags |= TQStyle::Style_Enabled;
    if (has_focus)
        flags |= TQStyle::Style_HasFocus;
    if ( selected )
        flags |= TQStyle::Style_Selected;
 //   else if(t == d->pressed)
 //       flags |= TQStyle::Style_Sunken;
    if(t->rect().contains(mapFromGlobal(TQCursor::pos())))
        flags |= TQStyle::Style_MouseOver;
    style().drawControl( TQStyle::CE_TabBarLabel, p, this, rt,
            t->isEnabled() ? colorGroup(): palette().disabled(),
            flags, TQStyleOption(t) );
}

TQSize KickoffTabBar::sizeHint() const
{
    TQSize s = TQTabBar::sizeHint();

    return s;
}

void KickoffTabBar::layoutTabs()
{
    TQTabBar::layoutTabs();

    TQFontMetrics fm = fontMetrics();
    int fh = ((KickerSettings::kickoffTabBarFormat() != KickerSettings::IconOnly) ? fm.height() : 0) + 4;

    int hframe = style().pixelMetric( TQStyle::PM_TabBarTabHSpace, this );
    int vframe = style().pixelMetric( TQStyle::PM_TabBarTabVSpace, this );
    int overlap = style().pixelMetric( TQStyle::PM_TabBarTabOverlap, this );

    TQSize s;
    for (int t = 0; t < count(); ++t)
    {
        TQTab* tab = tabAt(t);
        if (tab->iconSet())
            s = s.expandedTo(tab->iconSet()->pixmap(TQIconSet::Large, TQIconSet::Normal).size());
    }

    int x = 0;
    for (int t = 0; t < count(); ++t) {
        TQTab* tab = tabAt(TQApplication::reverseLayout() ? count() - t - 1 : t);
        int h = fh;
        if (tab->iconSet())
            h += 4 + s.height() + 4;
        TQRect r = tab->rect();

        int fw = fm.size( TQt::SingleLine|TQt::ShowPrefix, tab->text() ).width();
        int iw = 0;
        if ( tab->iconSet() != 0 )
            iw = tab->iconSet()->pixmap( TQIconSet::Large, TQIconSet::Normal ).width();
        int w = QMAX(iw, fw + 6 + 6 ) + hframe;
        h += ((KickerSettings::kickoffTabBarFormat() != KickerSettings::IconOnly) ? fm.height() : 0) + vframe;
        tab->setRect(TQRect(TQPoint(x, 0), style().tqsizeFromContents(TQStyle::CT_TabBarTab, this,
                    TQSize(w, h), TQStyleOption(tab))));
        x += tab->rect().width() - overlap;
    }
}

void KickoffTabBar::dragEnterEvent(TQDragEnterEvent* event)
{
    event->accept(KMenuItemDrag::canDecode(event));
}

void KickoffTabBar::dragMoveEvent(TQDragMoveEvent* event)
{
    TQTab* t = selectTab(event->pos());

    // ### uhhh, look away
    if (t && t->identifier() == 0)
    {
        setCurrentTab(t);
    }
}

void KickoffTabBar::mousePressEvent( TQMouseEvent * e )
{
    if ( e->button() != Qt::LeftButton ) {
	e->ignore();
	return;
    }
    TQTab *t = selectTab( e->pos() );
    if ( t && t->isEnabled() ) {
	emit tabClicked(t);
    }
    TQTabBar::mousePressEvent(e);
}

#include "kickoff_bar.moc"
// vim:cindent:sw=4:
