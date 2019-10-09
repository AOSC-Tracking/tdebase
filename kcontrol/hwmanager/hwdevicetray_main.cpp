/*
 * Copyright 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 * 
 * This file is part of hwdevicetray, the TDE Hardware Device Monitor System Tray Application
 * 
 * hwdevicetray is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * hwdevicetray is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with cryptocardwatcher. If not, see http://www.gnu.org/licenses/.
 */

#include <stdlib.h>
#include <kdebug.h>

#include <tdelocale.h>
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>
#include <tdeglobal.h>

#include "hwdevicetray_app.h"

static const char hwdevicetrayVersion[] = "0.1";
static const TDECmdLineOptions options[] =
{
	{ "login", I18N_NOOP("Application is being auto-started at TDE session start"), 0L },
	TDECmdLineLastOption
};

int main(int argc, char **argv)
{
	TDEAboutData aboutData("hwdevicetray", I18N_NOOP("Device Monitor"), hwdevicetrayVersion, I18N_NOOP("Device Monitor Tray Application\n\nAllows you not only to get informations about hardware changes on your system,\nbut also to open and eject storage devices if possible."), TDEAboutData::License_GPL_V3,
	"(c) 2015 Timothy Pearson\n(c) 2019 The Trinity Desktop Project", 0L, "");
	aboutData.addAuthor("Timothy Pearson",I18N_NOOP("Initial developer and maintainer"), "kb9vqf@pearsoncomputing.net");
	aboutData.setProductName("hwdevices/hwdevicetray");
	TDEGlobal::locale()->setMainCatalogue("tdehwdevicetray");

	TDECmdLineArgs::init(argc,argv,&aboutData);
	TDECmdLineArgs::addCmdLineOptions(options);
	TDEApplication::addCmdLineOptions();

	HwDeviceApp app;

	return app.exec();
}
