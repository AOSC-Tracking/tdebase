/***************************************************************************
 *   Copyright (C) 2011 by Timothy Pearson <kb9vqf@pearsoncomputing.net>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <tqobject.h>
#include <tqtimer.h>

#include <tdeapplication.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <kdebug.h>

#include "krootbacking.h"

bool argb_visual = false;

static const char description[] =
    I18N_NOOP("A program to grab the current TDE desktop backgrounds for xscreensaver");

static const char version[] = "0.1";

static TDECmdLineOptions options[] =
{
	TDECmdLineLastOption
};

int main(int argc, char **argv)
{
	TDEAboutData about("krootbacking", I18N_NOOP("krootbacking"), version, description,
			TDEAboutData::License_GPL, "(C) 2011 Timothy Pearson", 0, 0, "kb9vqf@pearsoncomputing.net");
	about.addAuthor( "Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net" );
	TDECmdLineArgs::init(argc, argv, &about);
	TDECmdLineArgs::addCmdLineOptions( options );
	
	TDEApplication app;

	// no session.. just start up normally
	TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
	
	/// @todo do something with the command line args here
	args->clear();

	TQObject* mainWin = new KRootBacking();
	TQTimer *timer = new TQTimer( mainWin );
        TQObject::connect( timer, TQ_SIGNAL(timeout()), mainWin, TQ_SLOT(start()) );
        timer->start( 100, true ); // 100ms single shot timer

	app.exec();

	delete timer;
	delete mainWin;
}

