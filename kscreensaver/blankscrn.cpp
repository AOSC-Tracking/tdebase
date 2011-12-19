//-----------------------------------------------------------------------------
//
// kblankscrn - Basic screen saver for KDE
//
// Copyright (c)  Martin R. Jones 1996
//
// 1998/04/19 Layout management added by Mario Weilguni <mweilguni@kde.org>
// 2001/03/04 Converted to use libkscreensaver by Martin R. Jones

#include <stdlib.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcolordialog.h>
#include <kbuttonbox.h>
#include <kcolorbutton.h>
#include <kglobal.h>
#include "blankscrn.h"
#include "blankscrn.moc"

// libkscreensaver interface
extern "C"
{
    KDE_EXPORT const char *kss_applicationName = "kblankscrn.kss";
    KDE_EXPORT const char *kss_description = I18N_NOOP( "KBlankScreen" );
    KDE_EXPORT const char *kss_version = "2.2.0";

    KDE_EXPORT KScreenSaver* kss_create( WId id )
    {
        return new KBlankSaver( id );
    }

    KDE_EXPORT TQDialog* kss_setup()
    {
        return new KBlankSetup();
    }
}

//-----------------------------------------------------------------------------
// dialog to setup screen saver parameters
//
KBlankSetup::KBlankSetup( TQWidget *parent, const char *name )
	: KDialogBase( parent, name, true, i18n( "Setup Blank Screen Saver" ),
		Ok|Cancel, Ok, true )
{
	readSettings();

	TQFrame *main = makeMainWidget();
	TQGridLayout *grid = new TQGridLayout(main, 4, 2, 0, spacingHint() );

	TQLabel *label = new TQLabel( i18n("Color:"), main );
	grid->addWidget(label, 0, 0);

	KColorButton *colorPush = new KColorButton( color, main );
	colorPush->setMinimumWidth(80);
	connect( colorPush, TQT_SIGNAL( changed(const TQColor &) ),
		TQT_SLOT( slotColor(const TQColor &) ) );
	grid->addWidget(colorPush, 1, 0);

	preview = new TQWidget( main );
	preview->setFixedSize( 220, 165 );
	preview->setBackgroundColor( black );
	preview->show();    // otherwise saver does not get correct size
	saver = new KBlankSaver( preview->winId() );
	grid->addMultiCellWidget(preview, 0, 2, 1, 1);

	grid->setRowStretch( 2, 10 );
	grid->setRowStretch( 3, 20 );

	setMinimumSize( sizeHint() );
}

// read settings from config file
void KBlankSetup::readSettings()
{
	KConfig *config = KGlobal::config();
	config->setGroup( "Settings" );

	color = config->readColorEntry( "Color", &black );
}

void KBlankSetup::slotColor( const TQColor &col )
{
    color = col;
    saver->setColor( color );
}

// Ok pressed - save settings and exit
void KBlankSetup::slotOk()
{
	KConfig *config = KGlobal::config();
	config->setGroup( "Settings" );
	config->writeEntry( "Color", color );
	config->sync();

	accept();
}

//-----------------------------------------------------------------------------


KBlankSaver::KBlankSaver( WId id ) : KScreenSaver( id )
{
	readSettings();
	blank();
}

KBlankSaver::~KBlankSaver()
{
}

// set the color
void KBlankSaver::setColor( const TQColor &col )
{
	color = col;
	blank();
}

// read configuration settings from config file
void KBlankSaver::readSettings()
{
	KConfig *config = KGlobal::config();
	config->setGroup( "Settings" );

	color = config->readColorEntry( "Color", &black );
}

void KBlankSaver::blank()
{
    setBackgroundColor( color );
    erase();
}

