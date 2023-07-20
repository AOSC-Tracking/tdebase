/*

Copyright (C) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
Copyright (C) 2002,2004 Oswald Buddenhagen <ossi@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.

*/

#ifndef __TDMDESKTOP_H__
#define __TDMDESKTOP_H__


#include <tdeapplication.h>
#include <tqtimer.h>

#include <bgrender.h>
#include <KCrossBGRender.h>


class MyApplication : public TDEApplication
{
	TQ_OBJECT

  public:
	MyApplication( const char * );

  private slots:
	void renderDone();
	void slotTimeout();

  private:
	KVirtualBGRenderer renderer;
	TQTimer timer;
};

#endif
