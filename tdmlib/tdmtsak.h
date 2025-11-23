/*
   This file is part of the TDE project
   Copyright (C) 2011 Timothy Pearson <kb9vqf@pearsoncomputing.net>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>

#include <tqstring.h>

#include "config.h"

// #define DEBUG

inline int tde_sak_verify_calling_process()
{
	bool authorized = false;

	// Root always has access to everything...
	if (getuid() == 0) {
		return 0;
	}

	pid_t parentproc = getppid();
#ifdef DEBUG
	printf("Parent pid is: %d\n", parentproc);
#endif

	char parentexecutable[8192];
	TQString procparent = TQString("/proc/%1/exe").arg(parentproc);
	int chars = readlink(procparent.ascii(), parentexecutable, sizeof(parentexecutable));
	parentexecutable[chars] = 0;
	parentexecutable[8191] = 0;
	procparent = parentexecutable;
#ifdef DEBUG
	printf("Parent executable name and full path is: %s\n", procparent.ascii());
#endif

	TQString tdeBinaryPath = TQString(TDE_BINDIR "/");
#ifdef DEBUG
	printf("The TDE binary path is: %s\n", tdeBinaryPath.ascii());
#endif

	if (!procparent.startsWith(tdeBinaryPath)) {
		printf("Unauthorized path detected in calling process\n");
		return 2;
	}
	else {
		procparent = procparent.mid(tdeBinaryPath.length());
#ifdef DEBUG
		printf("Parent executable name is: %s\n", procparent.ascii());
#endif
		if ((procparent == "kdesktop") || (procparent == "kdesktop_lock") || (procparent == "tdm")) {
			authorized = true;
		}
		else if (procparent == "tdeinit") {
			printf("tdeinit detected\n");
			// A bit more digging is needed to see if this is an authorized process or not
			// Get the tdeinit command
			char tdeinitcmdline[8192];
			FILE *fp = fopen(TQString("/proc/%1/cmdline").arg(parentproc).ascii(),"r");
			if (fp != NULL) {
				if (fgets (tdeinitcmdline, 8192, fp) != NULL)
				fclose (fp);
			}
			tdeinitcmdline[8191] = 0;
			TQString tdeinitCommand = tdeinitcmdline;

			// Also get the environment, specifically the path
			TQString tdeinitEnvironment;
			char tdeinitenviron[8192];
			fp = fopen(TQString("/proc/%1/environ").arg(parentproc).ascii(),"r");
			if (fp != NULL) {
				int c;
				int pos = 0;
				do {
					c = fgetc(fp);
					tdeinitenviron[pos] = c;
					pos++;
					if (c == 0) {
						TQString curEnvLine = tdeinitenviron;
						if (curEnvLine.startsWith("PATH=")) {
							tdeinitEnvironment = curEnvLine.mid(5);
						}
						pos = 0;
					}
				} while ((c != EOF) && (pos < 8192));
				fclose (fp);
			}
			tdeinitenviron[8191] = 0;

#ifdef DEBUG
			printf("Called executable name is: %s\n", tdeinitCommand.ascii());
			printf("Environment is: %s\n", tdeinitEnvironment.ascii());
#endif

			if ((tdeinitCommand == "kdesktop [tdeinit]") && (tdeinitEnvironment.startsWith(TDE_BINDIR))) {
				authorized = true;
			}
			else {
				return 4;
			}
		}
		else {
			printf("Unauthorized calling process detected\n");
			return 3;
		}

		if (authorized == true) {
			return 0;
		}
	}

	return 5;
}

#undef DEBUG
