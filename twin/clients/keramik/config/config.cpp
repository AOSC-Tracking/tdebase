/*
 * 
 * Keramik KWin client configuration module
 *
 * Copyright (C) 2002 Fredrik Höglund <fredrik@kde.org>
 *
 * Based on the Quartz configuration module,
 *     Copyright (c) 2001 Karol Szwed <gallium@kde.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the license, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
 
#include <tdeglobal.h>
#include <tdelocale.h>

#include <tqcheckbox.h>

#include "config.h"
#include "config.moc"

extern "C"
{
	KDE_EXPORT TQObject* allocate_config( TDEConfig* conf, TQWidget* parent )
	{
		return ( new KeramikConfig( conf, parent ) );
	}
}


/* NOTE: 
 * 'conf' 	is a pointer to the twindecoration modules open twin config,
 *			and is by default set to the "Style" group.
 *
 * 'parent'	is the parent of the TQObject, which is a VBox inside the
 *			Configure tab in twindecoration
 */

KeramikConfig::KeramikConfig( TDEConfig* conf, TQWidget* parent )
	: TQObject( parent )
{
	TDEGlobal::locale()->insertCatalogue("twin_clients");
	c = new TDEConfig( "twinkeramikrc" );
	
	ui = new KeramikConfigUI( parent );
	connect( ui->showAppIcons,    TQT_SIGNAL(clicked()), TQT_SIGNAL(changed()) );
	connect( ui->smallCaptions,   TQT_SIGNAL(clicked()), TQT_SIGNAL(changed()) );
	connect( ui->largeGrabBars,   TQT_SIGNAL(clicked()), TQT_SIGNAL(changed()) );
	connect( ui->useShadowedText, TQT_SIGNAL(clicked()), TQT_SIGNAL(changed()) );

	load( conf );
	ui->show();
}


KeramikConfig::~KeramikConfig()
{
	delete ui;
	delete c;
}


// Loads the configurable options from the twinrc config file
// It is passed the open config from twindecoration to improve efficiency
void KeramikConfig::load( TDEConfig* )
{
	c->setGroup("General");
	ui->showAppIcons->setChecked( c->readBoolEntry("ShowAppIcons", true) );
	ui->smallCaptions->setChecked( c->readBoolEntry("SmallCaptionBubbles", false) );
	ui->largeGrabBars->setChecked( c->readBoolEntry("LargeGrabBars", true) );
	ui->useShadowedText->setChecked( c->readBoolEntry("UseShadowedText", true) );
}


// Saves the configurable options to the twinrc config file
void KeramikConfig::save( TDEConfig* )
{
	c->setGroup( "General" );
	c->writeEntry( "ShowAppIcons", ui->showAppIcons->isChecked() );
	c->writeEntry( "SmallCaptionBubbles", ui->smallCaptions->isChecked() );
	c->writeEntry( "LargeGrabBars", ui->largeGrabBars->isChecked() );
	c->writeEntry( "UseShadowedText", ui->useShadowedText->isChecked() );
	c->sync();
}


// Sets UI widget defaults which must correspond to style defaults
void KeramikConfig::defaults()
{
	ui->showAppIcons->setChecked( true );
	ui->smallCaptions->setChecked( false );
	ui->largeGrabBars->setChecked( true );
	ui->useShadowedText->setChecked( true );
	
	emit changed();
}
