/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "main.h"

#include <tqtabwidget.h>
#include <tqlayout.h>

#include <klocale.h>
#include <kaboutdata.h>
#include <kdialog.h>

#include <kgenericfactory.h>

#include "notifiermodule.h"
#include "managermodule.h"


typedef KGenericFactory<MediaModule, TQWidget> MediaFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_media, MediaFactory( "kcmmedia" ) )


MediaModule::MediaModule( TQWidget *parent, const char *name, const TQStringList& )
	: KCModule(MediaFactory::instance(), parent, name )
{
	KGlobal::locale()->insertCatalogue("kio_media");
	TQVBoxLayout *layout = new TQVBoxLayout( this, 0, KDialog::spacingHint() );
	TQTabWidget *tab = new TQTabWidget( this );
	
	layout->addWidget( tab );


	
	m_notifierModule = new NotifierModule( this, "notifier" );
	tab->addTab( m_notifierModule, i18n( "&Notifications" ) );
	connect( m_notifierModule, TQT_SIGNAL( changed( bool ) ),
	         this, TQT_SLOT( moduleChanged( bool ) ) );

	m_managerModule = new ManagerModule( this, "manager" );
	tab->addTab( m_managerModule, i18n( "&Advanced" ) );
	connect( m_managerModule, TQT_SIGNAL( changed( bool ) ),
	         this, TQT_SLOT( moduleChanged( bool ) ) );



	KAboutData * about = new KAboutData("kcmmedia",
	                                    I18N_NOOP("Storage Media"),
	                                    "0.6",
	                                    I18N_NOOP("Storage Media Control Panel Module"),
	                                    KAboutData::License_GPL_V2,
	                                    I18N_NOOP("(c) 2005 Jean-Remy Falleri"));
	about->addAuthor("Jean-Remy Falleri", I18N_NOOP("Maintainer"), "jr.falleri@laposte.net");
	about->addAuthor("Kevin Ottens", 0, "ervin ipsquad net");
	about->addAuthor("Valentine Sinitsyn", 0, "e_val@inbox.ru");
	about->addCredit("Achim Bohnet", I18N_NOOP("Help for the application design"));

	setAboutData( about );
}

void MediaModule::load()
{
	m_notifierModule->load();
	m_managerModule->load();
}

void MediaModule::save()
{
	m_notifierModule->save();
	m_managerModule->save();
}

void MediaModule::defaults()
{
	m_notifierModule->defaults();
	m_managerModule->defaults();
}

void MediaModule::moduleChanged( bool state )
{
	emit changed( state );
}

TQString MediaModule::quickHelp() const
{
	return i18n("FIXME : Write me...");
}

#include "main.moc"
