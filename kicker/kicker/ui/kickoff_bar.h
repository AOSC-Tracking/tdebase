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

#ifndef __kickoff_bar_h__
#define __kickoff_bar_h__

#include <tqtabbar.h>

class KickoffTabBar : public TQTabBar
{
    Q_OBJECT
public:
    KickoffTabBar(TQWidget* parent, const char* name);

    void deactivateTabs(bool b);
    virtual TQSize sizeHint() const;

protected:
    virtual void paint(TQPainter*, TQTab*, bool) const;
    virtual void paintLabel(TQPainter* p, const TQRect& br, TQTab* t, bool has_focus) const;
    virtual void layoutTabs();
    virtual void dragEnterEvent(TQDragEnterEvent*);
    virtual void dragMoveEvent(TQDragMoveEvent*);
    virtual void mousePressEvent ( TQMouseEvent * );

signals:
    void tabClicked(TQTab*);

private:
    bool m_tabsActivated;
};


#endif
