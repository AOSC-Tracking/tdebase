/* This file is part of the KDE project
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

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

#include <tqheader.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqtimer.h>
#include <tqvgroupbox.h>
#include <tqwhatsthis.h>

#include <dcopclient.h>
#include <dcopref.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kbuttonbox.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdialog.h>
#include <kgenericfactory.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <kstandarddirs.h>

#include "kcmkded.h"
#include "kcmkded.moc"

typedef KGenericFactory<KDEDConfig, TQWidget> KDEDFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kded, KDEDFactory( "kcmkded" ) )


KDEDConfig::KDEDConfig(TQWidget* parent, const char* name, const TQStringList &) :
	KCModule( KDEDFactory::instance(), parent, name )
{
	KAboutData *about =
		new KAboutData( I18N_NOOP( "kcmkded" ), I18N_NOOP( "KDE Service Manager" ),
				0, 0, KAboutData::License_GPL,
				I18N_NOOP( "(c) 2002 Daniel Molkentin" ) );
	about->addAuthor("Daniel Molkentin",0,"molkentin@kde.org");
	setAboutData( about );

	setQuickHelp( i18n("<h1>Service Manager</h1><p>This module allows you to have an overview of all plugins of the "
			"KDE Daemon, also referred to as KDE Services. Generally, there are two types of service:</p>"
			"<ul><li>Services invoked at startup</li><li>Services called on demand</li></ul>"
			"<p>The latter are only listed for convenience. The startup services can be started and stopped. "
			"In Administrator mode, you can also define whether services should be loaded at startup.</p>"
			"<p><b> Use this with care: some services are vital for KDE; do not deactivate services if you"
			" do not know what you are doing.</b></p>"));

	RUNNING = i18n("Running")+" ";
	NOT_RUNNING = i18n("Not running")+" ";

	TQVBoxLayout *lay = new TQVBoxLayout( this, 0, KDialog::spacingHint() );

	TQGroupBox *gb = new TQVGroupBox(i18n("Load-on-Demand Services"), this );
	TQWhatsThis::add(gb, i18n("This is a list of available KDE services which will "
			"be started on demand. They are only listed for convenience, as you "
			"cannot manipulate these services."));
	lay->addWidget( gb );

	_lvLoD = new KListView( gb );
	_lvLoD->addColumn(i18n("Service"));
	_lvLoD->addColumn(i18n("Description"));
	_lvLoD->addColumn(i18n("Status"));
	_lvLoD->setAllColumnsShowFocus(true);
	_lvLoD->header()->setStretchEnabled(true, 1);

 	gb = new TQVGroupBox(i18n("Startup Services"), this );
	TQWhatsThis::add(gb, i18n("This shows all KDE services that can be loaded "
				"on KDE startup. Checked services will be invoked on next startup. "
				"Be careful with deactivation of unknown services."));
	lay->addWidget( gb );

	_lvStartup = new KListView( gb );
	_lvStartup->addColumn(i18n("Use"));
	_lvStartup->addColumn(i18n("Service"));
	_lvStartup->addColumn(i18n("Description"));
	_lvStartup->addColumn(i18n("Status"));
	_lvStartup->setAllColumnsShowFocus(true);
	_lvStartup->header()->setStretchEnabled(true, 2);

	KButtonBox *buttonBox = new KButtonBox( gb, Qt::Horizontal);
	_pbStart = buttonBox->addButton( i18n("Start"));
	_pbStop = buttonBox->addButton( i18n("Stop"));

	_pbStart->setEnabled( false );
	_pbStop->setEnabled( false );

	connect(_pbStart, TQT_SIGNAL(clicked()), TQT_SLOT(slotStartService()));
	connect(_pbStop,  TQT_SIGNAL(clicked()), TQT_SLOT(slotStopService()));
	connect(_lvStartup, TQT_SIGNAL(selectionChanged(TQListViewItem*)), TQT_SLOT(slotEvalItem(TQListViewItem*)) );

	load();
}

void setModuleGroup(KConfig *config, const TQString &filename)
{
	TQString module = filename;
	int i = module.findRev('/');
	if (i != -1)
	   module = module.mid(i+1);
	i = module.findRev('.');
	if (i != -1)
	   module = module.left(i);

	config->setGroup(TQString("Module-%1").arg(module));
}

bool KDEDConfig::autoloadEnabled(KConfig *config, const TQString &filename)
{
	setModuleGroup(config, filename);
	return config->readBoolEntry("autoload", true);
}

void KDEDConfig::setAutoloadEnabled(KConfig *config, const TQString &filename, bool b)
{
	setModuleGroup(config, filename);
	return config->writeEntry("autoload", b);
}

void KDEDConfig::load() {
   load ( false );
}


void KDEDConfig::load( bool useDefaults ) {
	KConfig kdedrc("kdedrc", true, false);
   kdedrc.setReadDefaults( useDefaults );

	_lvStartup->clear();
	_lvLoD->clear();

	TQStringList files;
	KGlobal::dirs()->findAllResources( "services",
			TQString::fromLatin1( "kded/*.desktop" ),
			true, true, files );

	TQListViewItem* item = 0L;
	CheckListItem* clitem;
	for ( TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it ) {

		if ( KDesktopFile::isDesktopFile( *it ) ) {
			KDesktopFile file( *it, true, "services" );

			if ( file.readBoolEntry("X-KDE-Kded-autoload") ) {
				clitem = new CheckListItem(_lvStartup, TQString::null);
				connect(clitem, TQT_SIGNAL(changed(TQCheckListItem*)), TQT_SLOT(slotItemChecked(TQCheckListItem*)));
				clitem->setOn(autoloadEnabled(&kdedrc, *it));
				item = clitem;
				item->setText(1, file.readName());
				item->setText(2, file.readComment());
				item->setText(3, NOT_RUNNING);
				item->setText(4, file.readEntry("X-KDE-Library"));
			}
			else if ( file.readBoolEntry("X-KDE-Kded-load-on-demand") ) {
				item = new TQListViewItem(_lvLoD, file.readName());
				item->setText(1, file.readComment());
				item->setText(2, NOT_RUNNING);
				item->setText(4, file.readEntry("X-KDE-Library"));
			}
		}
	}

	getServiceStatus();
   emit changed( useDefaults );
}

void KDEDConfig::save() {
	TQCheckListItem* item = 0L;

	TQStringList files;
	KGlobal::dirs()->findAllResources( "services",
			TQString::fromLatin1( "kded/*.desktop" ),
			true, true, files );

	KConfig kdedrc("kdedrc", false, false);

	for ( TQStringList::ConstIterator it = files.begin(); it != files.end(); ++it ) {

		if ( KDesktopFile::isDesktopFile( *it ) ) {

			KConfig file( *it, false, false, "services" );
			file.setGroup("Desktop Entry");

			if (file.readBoolEntry("X-KDE-Kded-autoload")){

				item = static_cast<TQCheckListItem *>(_lvStartup->findItem(file.readEntry("X-KDE-Library"),4));
				if (item) {
					// we found a match, now compare and see what changed
					setAutoloadEnabled(&kdedrc, *it, item->isOn());
				}
			}
		}
	}
	kdedrc.sync();

	DCOPRef( "kded", "kded" ).call( "reconfigure" );
	TQTimer::singleShot(0, this, TQT_SLOT(slotServiceRunningToggled()));
}


void KDEDConfig::defaults()
{
   load( true );
}


void KDEDConfig::getServiceStatus()
{
	QCStringList modules;
	TQCString replyType;
	TQByteArray replyData;


	if (!kapp->dcopClient()->call( "kded", "kded", "loadedModules()", TQByteArray(),
				replyType, replyData ) ) {

		_lvLoD->setEnabled( false );
		_lvStartup->setEnabled( false );
		KMessageBox::error(this, i18n("Unable to contact KDED."));
		return;
	}
	else {

		if ( replyType == "QCStringList" ) {
			TQDataStream reply(replyData, IO_ReadOnly);
			reply >> modules;
		}
	}

	for( TQListViewItemIterator it( _lvLoD); it.current() != 0; ++it )
                it.current()->setText(2, NOT_RUNNING);
	for( TQListViewItemIterator it( _lvStartup); it.current() != 0; ++it )
                it.current()->setText(3, NOT_RUNNING);
	for ( QCStringList::Iterator it = modules.begin(); it != modules.end(); ++it )
	{
		TQListViewItem *item = _lvLoD->findItem(*it, 4);
		if ( item )
		{
			item->setText(2, RUNNING);
		}

		item = _lvStartup->findItem(*it, 4);
		if ( item )
		{
			item->setText(3, RUNNING);
		}
	}
}

void KDEDConfig::slotReload()
{
	TQString current = _lvStartup->currentItem()->text(4);
	load();
	TQListViewItem *item = _lvStartup->findItem(current, 4);
	if (item)
		_lvStartup->setCurrentItem(item);
}

void KDEDConfig::slotEvalItem(TQListViewItem * item)
{
	if (!item)
		return;

	if ( item->text(3) == RUNNING ) {
		_pbStart->setEnabled( false );
		_pbStop->setEnabled( true );
	}
	else if ( item->text(3) == NOT_RUNNING ) {
		_pbStart->setEnabled( true );
		_pbStop->setEnabled( false );
	}
	else // Error handling, better do nothing
	{
		_pbStart->setEnabled( false );
		_pbStop->setEnabled( false );
	}

	getServiceStatus();
}

void KDEDConfig::slotServiceRunningToggled()
{
	getServiceStatus();
	slotEvalItem(_lvStartup->currentItem());
}

void KDEDConfig::slotStartService()
{
	TQCString service = _lvStartup->currentItem()->text(4).latin1();

	TQByteArray data, replyData;
	TQCString replyType;
	TQDataStream arg( data, IO_WriteOnly );
	arg << service;
	if (kapp->dcopClient()->call( "kded", "kded", "loadModule(TQCString)", data, replyType, replyData ) ) {
		TQDataStream reply(replyData, IO_ReadOnly);
		if ( replyType == "bool" )
		{
			bool result;
			reply >> result;
			if ( result )
				slotServiceRunningToggled();
			else
				KMessageBox::error(this, i18n("Unable to start service."));
		} else {
			kdDebug() << "loadModule() on kded returned an unexpected type of reply: " << replyType << endl;
		}
	}
	else {
		KMessageBox::error(this, i18n("Unable to contact KDED."));
	}
}

void KDEDConfig::slotStopService()
{
	TQCString service = _lvStartup->currentItem()->text(4).latin1();
	kdDebug() << "Stopping: " << service << endl;
	TQByteArray data;
	TQDataStream arg( data, IO_WriteOnly );

	arg << service;
	if (kapp->dcopClient()->send( "kded", "kded", "unloadModule(TQCString)", data ) ) {
		slotServiceRunningToggled();
	}
	else {
		KMessageBox::error(this, i18n("Unable to stop service."));
	}

}

void KDEDConfig::slotItemChecked(TQCheckListItem*)
{
	emit changed(true);
}

CheckListItem::CheckListItem(TQListView *parent, const TQString &text)
	: TQObject(parent),
	  TQCheckListItem(parent, text, CheckBox)
{ }

void CheckListItem::stateChange(bool on)
{
	TQCheckListItem::stateChange(on);
	emit changed(this);
}
