/* This file is part of the KDE project
   Copyright (C) 2001 Lubos Lunak <l.lunak@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __startup_h__
#define __startup_h__

#include <sys/types.h>

#include <tqwidget.h>
#include <tqpixmap.h>
#include <tqstring.h>
#include <tqtimer.h>
#include <tqmap.h>
#include <tdestartupinfo.h>

class TQStyle;

class StartupId
    : public TQWidget
    {
    TQ_OBJECT
    public:
        StartupId( TQWidget* parent = 0, const char* name = 0 );
        virtual ~StartupId();
        void configure();
    protected:
        virtual bool x11Event( XEvent* e );
        void start_startupid( const TQString& icon );
        void stop_startupid();
    protected slots:
        void update_startupid();
        void gotNewStartup( const TDEStartupInfoId& id, const TDEStartupInfoData& data );
        void gotStartupChange( const TDEStartupInfoId& id, const TDEStartupInfoData& data );
        void gotRemoveStartup( const TDEStartupInfoId& id );
        void finishKDEStartup();
    protected:
        TDEStartupInfo startup_info;
        TQWidget* startup_widget;
        TQTimer update_timer;
        TQMap< TDEStartupInfoId, TQString > startups; // TQString == pixmap
        TDEStartupInfoId current_startup;
        bool blinking;
        bool bouncing;
        unsigned int color_index;
        unsigned int frame;
        enum { NUM_BLINKING_PIXMAPS = 5 };
        TQPixmap pixmaps[ NUM_BLINKING_PIXMAPS ];
    };

#endif
