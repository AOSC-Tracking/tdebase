/* This file is part of the KDE project
   Copyright ( C ) 2003 Nadeem Hasan <nhasan@kde.org>

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

#include "SensorLoggerSettings.h"
#include "SensorLoggerSettingsWidget.h"

#include <tdelocale.h>

SensorLoggerSettings::SensorLoggerSettings( TQWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n( "Sensor Logger Settings" ),
      Ok|Apply|Cancel, Ok, true )
{
  m_settingsWidget = new SensorLoggerSettingsWidget( this, "m_settingsWidget" );
  setMainWidget( m_settingsWidget );
}

TQString SensorLoggerSettings::title()
{
  return m_settingsWidget->title();
}

TQColor SensorLoggerSettings::foregroundColor()
{
  return m_settingsWidget->foregroundColor();
}

TQColor SensorLoggerSettings::backgroundColor()
{
  return m_settingsWidget->backgroundColor();
}

TQColor SensorLoggerSettings::alarmColor()
{
  return m_settingsWidget->alarmColor();
}

void SensorLoggerSettings::setTitle( const TQString &title )
{
  m_settingsWidget->setTitle( title );
}

void SensorLoggerSettings::setBackgroundColor( const TQColor &c )
{
  m_settingsWidget->setBackgroundColor( c );
}

void SensorLoggerSettings::setForegroundColor( const TQColor &c )
{
  m_settingsWidget->setForegroundColor( c );
}

void SensorLoggerSettings::setAlarmColor( const TQColor &c )
{
  m_settingsWidget->setAlarmColor( c );
}

#include "SensorLoggerSettings.moc"
