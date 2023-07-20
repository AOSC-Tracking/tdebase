/*

clock module for tdm

Copyright (C) 2000 Espen Sand, espen@kde.org
  Based on work by NN (yet to be determined)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef _TDM_CLOCK_H_
#define _TDM_CLOCK_H_

#include <tqframe.h>

class KdmClock : public TQFrame {
	TQ_OBJECT
	typedef TQFrame inherited;

  public:
	KdmClock( TQWidget *parent=0, const char *name=0 );

  protected:
	virtual void showEvent( TQShowEvent * );
	virtual void paintEvent( TQPaintEvent * );

  private slots:
	void timeout();

  private:
	TQBrush mBackgroundBrush;
	TQFont  mFont;
	bool   mSecond;
	bool   mDigital;
	bool   mDate;
	bool   mBorder;
};

#endif
