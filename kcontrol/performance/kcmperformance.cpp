/*
 *  Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
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

#include <tqlayout.h>
#include <tqtabwidget.h>

#include <tdelocale.h>
#include <kdialog.h>

#include "kcmperformance.h"
#include "konqueror.h"
#include "system.h"

extern "C"
{
    TDE_EXPORT TDECModule* create_performance( TQWidget* parent_P, const char* name_P )
    {
    return new KCMPerformance::Config( parent_P, name_P );
    }

    TDE_EXPORT TDECModule* create_konqueror( TQWidget* parent_P, const char* name_P )
    {
    return new KCMPerformance::KonquerorConfig( parent_P, name_P );
    }
}

namespace KCMPerformance
{

Config::Config( TQWidget* parent_P, const char* )
    : TDECModule( parent_P, "kcmperformance" )
    {
    setQuickHelp( i18n( "<h1>TDE Performance</h1>"
        " You can configure settings that improve TDE performance here." ));
    
    TQVBoxLayout *topLayout = new TQVBoxLayout( this );
    tabs = new TQTabWidget( this );
    konqueror_widget = new Konqueror( tabs );
    konqueror_widget->layout()->setMargin( KDialog::marginHint() );
    connect( konqueror_widget, TQ_SIGNAL( changed()), TQ_SLOT( changed()));
    tabs->addTab( konqueror_widget, i18n( "Konqueror" ));
    system_widget = new SystemWidget( tabs );
    system_widget->layout()->setMargin( KDialog::marginHint() );
    connect( system_widget, TQ_SIGNAL( changed()), TQ_SLOT( changed()));
    tabs->addTab( system_widget, i18n( "System" ));
    topLayout->add( tabs );
    load();
    }

void Config::load()
    {
    load( false );
	 }

void Config::load( bool useDefaults)
    {
    konqueror_widget->load( useDefaults );
    system_widget->load( useDefaults );
	 emit changed( useDefaults );
    }

void Config::save()
    {
    konqueror_widget->save();
    system_widget->save();
    }

void Config::defaults()
    {
		 load( true );
    }

TQString Config::handbookSection() const
    {
	int index = tabs->currentPageIndex();
	if (index == 0) {
	    //return "konqueror-performance";
	    return TQString::null;
	}
	else if (index == 1) {
	    return "system-performance";
	}
	else {
	    return TQString::null;
	}
    }

KonquerorConfig::KonquerorConfig( TQWidget* parent_P, const char* )
    : TDECModule( parent_P, "kcmperformance" )
    {
    setQuickHelp( i18n( "<h1>Konqueror Performance</h1>"
        " You can configure several settings that improve Konqueror performance here."
        " These include options for reusing already running instances"
        " and for keeping instances preloaded." ));

    TQVBoxLayout *topLayout = new TQVBoxLayout( this );
    widget = new Konqueror( this );
    connect( widget, TQ_SIGNAL( changed()), TQ_SLOT( changed()));
    topLayout->add( widget );
    load();
    }

void KonquerorConfig::load()
    {
		 load( false );
	 }

void KonquerorConfig::load(bool useDefaults)
    {
    widget->load( useDefaults );
	 emit changed( useDefaults );
    }

void KonquerorConfig::save()
    {
    widget->save();
    }

void KonquerorConfig::defaults()
    {
		 load( true );
    }

} // namespace

#include "kcmperformance.moc"
