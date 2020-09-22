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

#include "ListViewSettings.h"
#include "ListViewSettingsWidget.h"

#include <tdelocale.h>

ListViewSettings::ListViewSettings( TQWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n( "List View Settings" ),
      Ok|Apply|Cancel, Ok, true )
{
  m_settingsWidget = new ListViewSettingsWidget( this, "m_settingsWidget" );
  setMainWidget( m_settingsWidget );
}

TQString ListViewSettings::title() const
{
  return m_settingsWidget->title();
}

TQColor ListViewSettings::textColor() const
{
  return m_settingsWidget->textColor();
}

TQColor ListViewSettings::backgroundColor() const
{
  return m_settingsWidget->backgroundColor();
}

TQColor ListViewSettings::gridColor() const
{
  return m_settingsWidget->gridColor();
}

void ListViewSettings::setTitle( const TQString &title )
{
  m_settingsWidget->setTitle( title );
}

void ListViewSettings::setBackgroundColor( const TQColor &c )
{
  m_settingsWidget->setBackgroundColor( c );
}

void ListViewSettings::setTextColor( const TQColor &c )
{
  m_settingsWidget->setTextColor( c );
}

void ListViewSettings::setGridColor( const TQColor &c )
{
  m_settingsWidget->setGridColor( c );
}

#include "ListViewSettings.moc"
