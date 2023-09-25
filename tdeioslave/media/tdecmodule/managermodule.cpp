/* This file is part of the KDE Project
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>
   Copyright (c) 2006 Valentine Sinitsyn <e_val@inbox.ru>

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

#include <config.h>

#include "managermodule.h"

#include <tdeconfig.h>
#include <tdelocale.h>
#include <dcopref.h>
#include <tqbutton.h>
#include <tqcheckbox.h>
#include <kcombobox.h>
#include <tqlayout.h>
#include <tqobjectlist.h>
#include <kdirnotify_stub.h>

#include "managermoduleview.h"
#include "mediamanagersettings.h"

ManagerModule::ManagerModule( TQWidget* parent, const char* name )
	: TDECModule( parent, name )
{
	TQBoxLayout *layout = new TQVBoxLayout( this, 0, KDialog::spacingHint() );

	view = new ManagerModuleView( this );
	layout->addWidget( view );

	addConfig(  MediaManagerSettings::self(), view );

#ifndef COMPILE_LINUXCDPOLLING
	TQString poll_text = view->kcfg_CdPollingEnabled->text();
	poll_text += " ("+i18n("No support for CD polling on this system")+")";
	view->kcfg_CdPollingEnabled->setText( poll_text );
#endif
	view->kcfg_CdPollingEnabled->setEnabled( false );

	connect( view->option_automount, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_ro, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_quiet, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_flush, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_uid, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_utf8, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_sync, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_atime, SIGNAL( stateChanged(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_shortname, SIGNAL( activated(int) ), this, SLOT( emitChanged() ) );
	connect( view->option_journaling, SIGNAL( activated(int) ), this, SLOT( emitChanged() ) );	

	load();
}	


void ManagerModule::load()
{
	TDECModule::load();
	
	TDEConfig config("mediamanagerrc");
	config.setGroup("DefaultOptions");
	
	view->option_automount->setChecked( config.readBoolEntry("automount", false) );
	view->option_ro->setChecked( config.readBoolEntry("ro", false) );
	view->option_quiet->setChecked( config.readBoolEntry("quiet", false) );
	if (config.hasKey("flush"))
	    view->option_flush->setChecked( config.readBoolEntry("flush") );
	else
	    view->option_flush->setNoChange();
	view->option_uid->setChecked( config.readBoolEntry("uid", true) );	
	view->option_utf8->setChecked( config.readBoolEntry("utf8", true) );
	if (config.hasKey("sync"))
	    view->option_sync->setChecked( config.readBoolEntry("sync") );
	else
	    view->option_sync->setNoChange();
	if (config.hasKey("atime"))    
	    view->option_atime->setChecked( config.readBoolEntry("atime") );
	else
	    view->option_atime->setNoChange();
	
	TQString value;	
	
	value = config.readEntry("shortname", "lower").lower();
	for (int i = 0; i < view->option_shortname->count(); i++)
	    if (view->option_shortname->text(i).lower() == value) view->option_shortname->setCurrentItem(i);
		    
	value = config.readEntry("journaling", "ordered").lower();
	for (int i = 0; i < view->option_journaling->count(); i++)
	    if (view->option_journaling->text(i).lower() == value) view->option_journaling->setCurrentItem(i);
	    
	rememberSettings();
}

void ManagerModule::save()
{
	TDECModule::save();
	
	TDEConfig config("mediamanagerrc");
	config.setGroup("DefaultOptions");
	
	config.writeEntry("automount", view->option_automount->isChecked());
	config.writeEntry("ro", view->option_ro->isChecked());
	config.writeEntry("quiet", view->option_quiet->isChecked());
	if (view->option_flush->state() == TQButton::NoChange)
	    config.deleteEntry("flush");
	else
	    config.writeEntry("flush", view->option_flush->isChecked());
	config.writeEntry("uid", view->option_uid->isChecked());	
	config.writeEntry("utf8", view->option_utf8->isChecked());
	if (view->option_sync->state() == TQButton::NoChange)
	    config.deleteEntry("sync");
	else
	    config.writeEntry("sync", view->option_sync->isChecked());
	if (view->option_atime->state() == TQButton::NoChange)
	    config.deleteEntry("atime");
	else    
	    config.writeEntry("atime", view->option_atime->isChecked());
	config.writeEntry("journaling", view->option_journaling->currentText().lower());
	config.writeEntry("shortname", view->option_shortname->currentText().lower());
	
	rememberSettings();

	DCOPRef kded( "kded", "kded" );
	// DCOPReply reply = mediamanager.call( "reloadBackends" );
	// Well... reloadBackends had issues with HAL, it seems it was linked
	// to a bug in the unmaintained Qt3 DBUS binding, but it is not quite clear.
	// It may be ok now that HAL is no longer supported but needs to be tested.
	// So we use this hack instead...
	kded.call( "unloadModule", "mediamanager" );
	kded.call( "loadModule", "mediamanager" );

	KDirNotify_stub notifier( "*", "*" );
	notifier.FilesAdded( "media:/" );
}

void ManagerModule::defaults()
{
	TDECModule::defaults();
	
	view->option_automount->setChecked(false);
	view->option_ro->setChecked(false);
	view->option_quiet->setChecked(false);	
	view->option_flush->setNoChange();
	view->option_uid->setChecked(true);	
	view->option_utf8->setChecked(true);	
	view->option_sync->setNoChange();	
	view->option_atime->setNoChange();
	view->option_journaling->setCurrentItem(1);
	view->option_shortname->setCurrentItem(0);
}

void ManagerModule::rememberSettings()
{
	TQObjectList *options = view->queryList(0, "^option_");
	TQObject *current = 0;
	TQObjectListIterator it(*options);

	settings.clear();	
	while ( (current = it.current()) != 0 ) {
	    if (current->isA("TQCheckBox"))
		settings[current] = ((TQCheckBox *)current)->state();
	    else if (current->isA("TQComboBox"))
		settings[current] = ((TQComboBox *)current)->currentItem();
	    ++it;	
	}
	delete options;

}

void ManagerModule::emitChanged()
{
	TQObjectList *options = view->queryList(0, "^option_");
	TQObject *current = 0;
	TQObjectListIterator it(*options);
	int value = -1;
	bool somethingChanged = false;
	
	while ( (current = it.current()) != 0 ) {
	    if (current->isA("TQCheckBox"))
		value = ((TQCheckBox *)current)->state();
	    else if (current->isA("TQComboBox"))
		value = ((TQComboBox *)current)->currentItem();
		
	    if (settings[current] != value) {
		somethingChanged = true;
		break;
	    }
	    
	    ++it;	
	}
	delete options;

	emit changed(somethingChanged);
}

#include "managermodule.moc"
