/*

Copyright (C) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
Copyright (C) 2002,2004 Oswald Buddenhagen <ossi@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.	 If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.

*/

#include <config.h>

#include <tdecmdlineargs.h>
#include <tdesimpleconfig.h>
#include <tdelocale.h>

#include <tqfile.h>

#include "krootimage.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdlib.h>

static const char description[] =
	I18N_NOOP( "Fancy desktop background for tdm" );

static const char version[] = "v2.0";

static TDECmdLineOptions options[] = {
	{ "+config", I18N_NOOP( "Name of the configuration file" ), 0 },
	TDECmdLineLastOption
};

static Atom prop_root;
static bool properties_inited = false;

MyApplication::MyApplication( const char *conf )
	: TDEApplication(),
	  renderer( 0, new TDESimpleConfig( TQFile::decodeName( conf ) ) )
{
	connect( &timer, TQ_SIGNAL(timeout()), TQ_SLOT(slotTimeout()) );
	connect( &renderer, TQ_SIGNAL(imageDone( int )), this, TQ_SLOT(renderDone()) );
	renderer.enableTiling( true ); // optimize
	renderer.changeWallpaper(); // cannot do it when we're killed, so do it now
	timer.start( 60000 );
	renderer.start();

	if( !properties_inited ) {
		prop_root = XInternAtom(tqt_xdisplay(), "_XROOTPMAP_ID", False);
		properties_inited = true;
	}
}


void
MyApplication::renderDone()
{
	// Get the newly drawn pixmap...
	TQPixmap pm = renderer.pixmap();

	// ...set it to the desktop widget...
	desktop()->setBackgroundPixmap( pm );
	desktop()->repaint( true );

	// ...and export it via Esetroot-style so that composition managers can use it!
	Pixmap bgPm = pm.handle(); // fetch the actual X handle to it
	XChangeProperty(tqt_xdisplay(), tqt_xrootwin(), prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &bgPm, 1);

	renderer.saveCacheFile();
	renderer.cleanup();
	for (unsigned i=0; i<renderer.numRenderers(); ++i)
	{
		KBackgroundRenderer * r = renderer.renderer(i);
		if (r->backgroundMode() == KBackgroundSettings::Program ||
		    (r->multiWallpaperMode() != KBackgroundSettings::NoMulti &&
		     r->multiWallpaperMode() != KBackgroundSettings::NoMultiRandom)) {
			return;
		}
	}

}

void
MyApplication::slotTimeout()
{
	bool change = false;

	if (renderer.needProgramUpdate()) {
		renderer.programUpdate();
		change = true;
	}

	if (renderer.needWallpaperChange()) {
		renderer.changeWallpaper();
		change = true;
	}

	if (change)
		renderer.start();
}

int
main( int argc, char *argv[] )
{
	TDEApplication::disableAutoDcopRegistration();

	TDELocale::setMainCatalogue( "kdesktop" );
	TDECmdLineArgs::init( argc, argv, "krootimage", I18N_NOOP( "KRootImage" ), description, version );
	TDECmdLineArgs::addCmdLineOptions( options );

	TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
	if (!args->count())
		args->usage();
	MyApplication app( args->arg( 0 ) );
	args->clear();

	app.exec();

	app.flushX();

	// Keep color resources after termination
	XSetCloseDownMode( tqt_xdisplay(), RetainTemporary );

	return 0;
}

#include "krootimage.moc"
