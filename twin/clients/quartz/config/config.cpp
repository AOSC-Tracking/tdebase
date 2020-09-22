/* 	
 *
 *	This file contains the quartz configuration widget
 *
 *	Copyright (c) 2001
 *		Karol Szwed <gallium@kde.org>
 *		http://gallium.n3.net/
 */

#include "config.h"
#include <tdeglobal.h>
#include <tqwhatsthis.h>
#include <tdelocale.h>


extern "C"
{
	KDE_EXPORT TQObject* allocate_config( TDEConfig* conf, TQWidget* parent )
	{
		return(new QuartzConfig(conf, parent));
	}
}


/* NOTE: 
 * 'conf' 	is a pointer to the twindecoration modules open twin config,
 *			and is by default set to the "Style" group.
 *
 * 'parent'	is the parent of the TQObject, which is a VBox inside the
 *			Configure tab in twindecoration
 */

QuartzConfig::QuartzConfig( TDEConfig* conf, TQWidget* parent )
	: TQObject( parent )
{
	quartzConfig = new TDEConfig("twinquartzrc");
	TDEGlobal::locale()->insertCatalogue("twin_clients");
	gb = new TQVBox( parent );
	cbColorBorder = new TQCheckBox( 
						i18n("Draw window frames using &titlebar colors"), gb );
	TQWhatsThis::add( cbColorBorder, 
						i18n("When selected, the window decoration borders "
						"are drawn using the titlebar colors; otherwise, they are "
						"drawn using normal border colors instead.") );
	cbExtraSmall = new TQCheckBox( i18n("Quartz &extra slim"), gb );
	TQWhatsThis::add( cbExtraSmall,
		i18n("Quartz window decorations with extra-small title bar.") );
	// Load configuration options
	load( conf );

	// Ensure we track user changes properly
	connect( cbColorBorder, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotSelectionChanged()) );
	connect( cbExtraSmall,  TQT_SIGNAL(clicked()), this, TQT_SLOT(slotSelectionChanged()) );

	// Make the widgets visible in twindecoration
	gb->show();
}


QuartzConfig::~QuartzConfig()
{
	delete gb;
	delete quartzConfig;
}


void QuartzConfig::slotSelectionChanged()
{
	emit changed();
}


// Loads the configurable options from the twinrc config file
// It is passed the open config from twindecoration to improve efficiency
void QuartzConfig::load( TDEConfig* /*conf*/ )
{
	quartzConfig->setGroup("General");
	bool override = quartzConfig->readBoolEntry( "UseTitleBarBorderColors", true );
	cbColorBorder->setChecked( override );
	override = quartzConfig->readBoolEntry( "UseQuartzExtraSlim", false );
	cbExtraSmall->setChecked( override );
}


// Saves the configurable options to the twinrc config file
void QuartzConfig::save( TDEConfig* /*conf*/ )
{
	quartzConfig->setGroup("General");
	quartzConfig->writeEntry( "UseTitleBarBorderColors", cbColorBorder->isChecked() );
	quartzConfig->writeEntry( "UseQuartzExtraSlim", cbExtraSmall->isChecked() );
	// Ensure others trying to read this config get updated
	quartzConfig->sync();
}


// Sets UI widget defaults which must correspond to style defaults
void QuartzConfig::defaults()
{
	cbColorBorder->setChecked( true );
	cbExtraSmall->setChecked( false );
}

#include "config.moc"
