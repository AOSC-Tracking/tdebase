/* 
 * Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __RANDRPASSIVEPOPUP_H__
#define __RANDRPASSIVEPOPUP_H__

#include <kpassivepopup.h>
#include <tqvaluelist.h>
#include <tqtimer.h>
#include <X11/Xlib.h>

class KRandrPassivePopup
    : public KPassivePopup
    {
    TQ_OBJECT
    public:
	static KRandrPassivePopup *message( const TQString &caption, const TQString &text,
	    const TQPixmap &icon, TQWidget *parent, const char *name=0, int timeout = -1 );
    protected:
	virtual bool eventFilter( TQObject* o, TQEvent* e );
	virtual bool x11Event( XEvent* e );
    private slots:
	void slotPositionSelf();
    private:
        KRandrPassivePopup( TQWidget *parent=0, const char *name=0, WFlags f=0 );
	void startWatchingWidget( TQWidget* w );
	TQValueList< TQWidget* > watched_widgets;
	TQValueList< Window > watched_windows;
	TQTimer update_timer;
    };

#endif
