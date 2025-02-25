/***************************************************************************
 *   Copyright (C) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>      *
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

#include <config.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <tdeapplication.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdestandarddirs.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <tdeconfig.h>

#include <pwd.h>
#include <signal.h>

#ifdef WITH_XRANDR
#include <libtderandr/libtderandr.h>
#endif

static const char description[] = I18N_NOOP("TDE Initialization Display Configuration");

static const char version[] = "0.1";

static TDECmdLineOptions options[] =
{
	TDECmdLineLastOption
};

int main(int argc, char **argv)
{
	int return_code = -1;

	TDEAboutData about("tdeinit_displayconfig", I18N_NOOP("tdeinit_displayconfig"), version, description,
			TDEAboutData::License_GPL, "(C) 2013 Timothy Pearson", 0, 0, "kb9vqf@pearsoncomputing.net");
	about.addAuthor( "Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net" );
	TDECmdLineArgs::init(argc, argv, &about);
	TDECmdLineArgs::addCmdLineOptions( options );

	TDEApplication::disableAutoDcopRegistration();
	TDEApplication app;

#ifdef WITH_XRANDR
	// Load up user specific display settings
	KRandrSimpleAPI *randrsimple = new KRandrSimpleAPI();
	randrsimple->applyStartupDisplayConfiguration(locateLocal("config", "/", true));
	randrsimple->applyHotplugRules(locateLocal("config", "/", true));
	delete randrsimple;
#endif

	return return_code = 0;
}

