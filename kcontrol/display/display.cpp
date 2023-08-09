/* This file is part of the KDE project
   Copyright (C) 2003-2004 Nadeem Hasan <nhasan@kde.org>

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

#include <tqapplication.h>
#include <tqlayout.h>
#include <tqtabwidget.h>

#include <tdecmoduleloader.h>
#include <kdialog.h>
#include <kgenericfactory.h>

#include "display.h"

typedef KGenericFactory<KCMDisplay, TQWidget> DisplayFactory;
K_EXPORT_COMPONENT_FACTORY ( kcm_display, DisplayFactory( "display" ) )

KCMDisplay::KCMDisplay( TQWidget *parent, const char *name, const TQStringList& )
    : TDECModule( parent, name )
    , m_changed(false)
{
  m_tabs = new TQTabWidget( this );

  addTab( "randr", i18n( "Size && Orientation" ) );
  addTab( "nvidiadisplay", i18n( "Graphics Adaptor" ) );
  addTab( "nvidia3d", i18n( "3D Options" ) );
  addTab( "kgamma", i18n( "Monitor Gamma" ) );
  if ( TQApplication::desktop()->isVirtualDesktop() )
    addTab( "xinerama", i18n( "Multiple Monitors" ) );
  addTab( "energy", i18n( "Power Control" ) );

  TQVBoxLayout *top = new TQVBoxLayout( this, 0, KDialog::spacingHint() );
  top->addWidget( m_tabs );

  setButtons( Apply|Help );
  load();
}

void KCMDisplay::addTab( const TQString &name, const TQString &label )
{
  TQWidget *page = new TQWidget( m_tabs, name.latin1() );
  TQVBoxLayout *top = new TQVBoxLayout( page, KDialog::marginHint() );

  TDECModule *kcm = TDECModuleLoader::loadModule( name, page );

  if ( kcm )
  {
    top->addWidget( kcm );
    m_tabs->addTab( page, label );

    connect( kcm, TQT_SIGNAL( changed(bool) ), TQT_SLOT( moduleChanged(bool) ) );
    m_modules.insert(kcm, false);
  }
  else
    delete page;
}

void KCMDisplay::load()
{
  for (TQMap<TDECModule*, bool>::ConstIterator it = m_modules.begin(); it != m_modules.end(); ++it)
    it.key()->load();
}

void KCMDisplay::save()
{
  for (TQMap<TDECModule*, bool>::Iterator it = m_modules.begin(); it != m_modules.end(); ++it)
    if (it.data())
      it.key()->save();
}

void KCMDisplay::moduleChanged( bool isChanged )
{
  TQMap<TDECModule*, bool>::Iterator currentModule = m_modules.find(static_cast<TDECModule*>(const_cast<TQObject*>(sender())));
  Q_ASSERT(currentModule != m_modules.end());
  if (currentModule.data() == isChanged)
    return;
    
  currentModule.data() = isChanged;

  bool c = false;
  
  for (TQMap<TDECModule*, bool>::ConstIterator it = m_modules.begin(); it != m_modules.end(); ++it) {
    if (it.data()) {
      c = true;
      break;
    }
  }
    
  if (m_changed != c) {
    m_changed = c;
    emit changed(c);
  }
}

#include "display.moc"
