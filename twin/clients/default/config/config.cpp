/*
 *
 *	KDE2 Default configuration widget
 *
 *	Copyright (c) 2001
 *		Karol Szwed <gallium@kde.org>
 *		http://gallium.n3.net/
 */

#include "config.h"
#include <tdeglobal.h>
#include <tqwhatsthis.h>
#include <kdialog.h>
#include <tdelocale.h>
#include <tqpixmap.h>
#include <tqvbox.h>

extern "C"
{
	KDE_EXPORT TQObject* allocate_config( TDEConfig* conf, TQWidget* parent )
	{
		return(new KDEDefaultConfig(conf, parent));
	}
}

// NOTE:
// 'conf' is a pointer to the twindecoration modules open twin config,
//		  and is by default set to the "Style" group.
// 'parent' is the parent of the TQObject, which is a VBox inside the
//		  Configure tab in twindecoration

KDEDefaultConfig::KDEDefaultConfig( TDEConfig* conf, TQWidget* parent )
	: TQObject( parent )
{
	TDEGlobal::locale()->insertCatalogue("twin_clients");
	highcolor = TQPixmap::defaultDepth() > 8;
	gb = new TQVBox( parent );
        gb->setSpacing( KDialog::spacingHint() );

	cbShowStipple = new TQCheckBox( i18n("Draw titlebar &stipple effect"), gb );
	TQWhatsThis::add( cbShowStipple, 
		i18n("When selected, active titlebars are drawn "
		 "with a stipple (dotted) effect; otherwise, they are "
		 "drawn without the stipple."));

	cbShowGrabBar = new TQCheckBox( i18n("Draw g&rab bar below windows"), gb );
	TQWhatsThis::add( cbShowGrabBar, 
		i18n("When selected, decorations are drawn with a \"grab bar\" "
		"below windows; otherwise, no grab bar is drawn."));

	// Only show the gradient checkbox for highcolor displays
	if (highcolor)
	{
		cbUseGradients = new TQCheckBox( i18n("Draw &gradients"), gb );
		TQWhatsThis::add( cbUseGradients, 
			i18n("When selected, decorations are drawn with gradients "
			"for high-color displays; otherwise, no gradients are drawn.") );
	}

	// Load configuration options
	load( conf );

	// Ensure we track user changes properly
	connect( cbShowStipple, TQT_SIGNAL(clicked()), 
			 this, TQT_SLOT(slotSelectionChanged()) );
	connect( cbShowGrabBar, TQT_SIGNAL(clicked()), 
			 this, TQT_SLOT(slotSelectionChanged()) );
	if (highcolor)
		connect( cbUseGradients, TQT_SIGNAL(clicked()), 
				 this, TQT_SLOT(slotSelectionChanged()) );

	// Make the widgets visible in twindecoration
	gb->show();
}


KDEDefaultConfig::~KDEDefaultConfig()
{
	delete gb;
}


void KDEDefaultConfig::slotSelectionChanged()
{
	emit changed();
}


// Loads the configurable options from the twinrc config file
// It is passed the open config from twindecoration to improve efficiency
void KDEDefaultConfig::load( TDEConfig* conf )
{
	conf->setGroup("KDEDefault");
	bool override = conf->readBoolEntry( "ShowTitleBarStipple", true );
	cbShowStipple->setChecked( override );

	override = conf->readBoolEntry( "ShowGrabBar", true );
	cbShowGrabBar->setChecked( override );

	if (highcolor) {
		override = conf->readBoolEntry( "UseGradients", true );
		cbUseGradients->setChecked( override );
	}
}


// Saves the configurable options to the twinrc config file
void KDEDefaultConfig::save( TDEConfig* conf )
{
	conf->setGroup("KDEDefault");
	conf->writeEntry( "ShowTitleBarStipple", cbShowStipple->isChecked() );
	conf->writeEntry( "ShowGrabBar", cbShowGrabBar->isChecked() );

	if (highcolor)
		conf->writeEntry( "UseGradients", cbUseGradients->isChecked() );
	// No need to conf->sync() - twindecoration will do it for us
}


// Sets UI widget defaults which must correspond to style defaults
void KDEDefaultConfig::defaults()
{
	cbShowStipple->setChecked( true );
	cbShowGrabBar->setChecked( true );

	if (highcolor)
		cbUseGradients->setChecked( true );
}

#include "config.moc"
