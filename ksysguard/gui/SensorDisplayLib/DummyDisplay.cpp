/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999, 2000, 2001 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
    Please do not commit any changes without consulting me first. Thanks!

*/

#include <tdelocale.h>
#include <ksgrd/SensorManager.h>

#include <tqwhatsthis.h>

#include "DummyDisplay.h"

DummyDisplay::DummyDisplay( TQWidget* parent, const char* name,
                            const TQString&, double, double )
  : KSGRD::SensorDisplay( parent, name, i18n( "Drop Sensor Here" ) )
{
  setMinimumSize( 16, 16 );

  TQWhatsThis::add( this, i18n(
                   "This is an empty space in a worksheet. Drag a sensor from "
                   "the Sensor Browser and drop it here. A sensor display will "
                   "appear that allows you to monitor the values of the sensor "
                   "over time." ) );
}

void DummyDisplay::resizeEvent( TQResizeEvent* )
{
  frame()->setGeometry( 0, 0, width(), height() );
}

bool DummyDisplay::eventFilter( TQObject* object, TQEvent* event )
{
  if ( event->type() == TQEvent::MouseButtonRelease &&
       ( (TQMouseEvent*)event)->button() == Qt::LeftButton )
    setFocus();

	return TQWidget::eventFilter( object, event );
}

#include "DummyDisplay.moc"
