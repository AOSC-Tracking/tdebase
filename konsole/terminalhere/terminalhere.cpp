/* This file is part of the TDE project

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <tdecmdlineargs.h>
#include <tdeapplication.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <tdeglobal.h>
#include <tdeprocess.h>
#include <tdestartupinfo.h>
#include <stdlib.h>

#include "terminalhere.h"


TerminalHere::TerminalHere() : TDEApplication()
{
	TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

	TDEConfig *config = tdeApp->config();
	config->setGroup("General");
	TQString term = config->readPathEntry("TerminalApplication", "konsole");

	TDEProcess proc;
	proc << term;
	if (args->isSet("wd"))
	{
		proc.setWorkingDirectory(args->getOption("wd"));
	}
	proc.start(TDEProcess::DontCare);
	::exit(0);
}

static TDECmdLineOptions options[] =
{
  { "wd <dir>",   I18N_NOOP("Set working directory to 'dir'"), 0 },
	TDECmdLineLastOption
};

int main(int argc, char **argv)
{
	TDECmdLineArgs::init(argc, argv, "terminalhere", "terminalhere", "terminalhere", "0.1");
	TDECmdLineArgs::addCmdLineOptions(options);
	TDEApplication::addCmdLineOptions();
	TDEApplication *app = new TerminalHere();

	TDEStartupInfo::appStarted();
	return app->exec();
}

#include "terminalhere.moc"
