/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <tdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>

#include "kjobviewer.h"
#include <tdelocale.h>
#include <stdlib.h>

static TDECmdLineOptions options[] = {
	{ "d <printer-name>", I18N_NOOP("The printer for which jobs are requested"), 0 },
	{ "noshow", I18N_NOOP("Show job viewer at startup"), 0},
	{ "all", I18N_NOOP("Show jobs for all printers"), 0},
        TDECmdLineLastOption
};


extern "C" int TDE_EXPORT kdemain(int argc, char *argv[])
{
	TDEAboutData	aboutData("kjobviewer",I18N_NOOP("KJobViewer"),"0.1",I18N_NOOP("A print job viewer"),TDEAboutData::License_GPL,"(c) 2001, Michael Goffioul", 0, 0);
	aboutData.addAuthor("Michael Goffioul",0,"tdeprint@swing.be");
	TDECmdLineArgs::init(argc,argv,&aboutData);
	TDECmdLineArgs::addCmdLineOptions(options);
	KJobViewerApp::addCmdLineOptions();

	if (!KJobViewerApp::start())
		exit(0);

	KJobViewerApp	a;
	return a.exec();
}
