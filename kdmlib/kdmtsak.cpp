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

#include "kdmtsak.h"

#include <tqstringlist.h>

#define FIFO_FILE "/tmp/ksocket-global/tsak"

TQString exec(const char * cmd) {
	FILE* pipe = popen(cmd, "r");
	if (!pipe) return "ERROR";
	char buffer[128];
	TQString result = "";
	while(!feof(pipe)) {
	if(fgets(buffer, 128, pipe) != NULL)
		result += buffer;
	}
	pclose(pipe);
	return result;
}

bool is_vt_local() {
	const char * currentDisplay;
	currentDisplay = getenv ("DISPLAY");
	if (currentDisplay == NULL) {
		return false;
	}
	else {
		TQString cvtName = "";
		TQString output = exec("kdmctl list");
		TQStringList sessionList = TQStringList::split('\t', output, false);
		// See if the current session is local
		for ( TQStringList::Iterator it = sessionList.begin(); it != sessionList.end(); ++it ) {
			TQStringList sessionInfoList = TQStringList::split(',', *it, true);
			if ((*(sessionInfoList.at(0))).startsWith(":")) {
				if (TQString(currentDisplay).startsWith(*(sessionInfoList.at(0)))) {
					return true;
				}
			}
		}
		// Not local
		return false;
	}
}

bool is_vt_active() {
	const char * currentDisplay;
	currentDisplay = getenv ("DISPLAY");
	if (currentDisplay == NULL) {
		return true;
	}
	else {
		TQString cvtName = "";
		TQString output = exec("kdmctl list");
		TQString curConsole = exec("fgconsole");
		bool intFound;
		int curConsoleNum = curConsole.toInt(&intFound);
		if (intFound == false) {
			return true;
		}
		curConsole = TQString("vt%1").arg(curConsoleNum);;
		TQStringList sessionList = TQStringList::split('\t', output, false);
		for ( TQStringList::Iterator it = sessionList.begin(); it != sessionList.end(); ++it ) {
			TQStringList sessionInfoList = TQStringList::split(',', *it, true);
			if ((*(sessionInfoList.at(0))).startsWith(":")) {
				if ((*(sessionInfoList.at(1))) == TQString(curConsole)) {
					cvtName = (*(sessionInfoList.at(0)));
				}
			}
		}
		if (cvtName != "") {
			if (TQString(currentDisplay).startsWith(cvtName)) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			// See if the current session is local
			// If it is, then the VT is not currently active and the SAK must be requested later when it is active
			for ( TQStringList::Iterator it = sessionList.begin(); it != sessionList.end(); ++it ) {
				TQStringList sessionInfoList = TQStringList::split(',', *it, true);
				if ((*(sessionInfoList.at(0))).startsWith(":")) {
					if (TQString(currentDisplay).startsWith(*(sessionInfoList.at(0)))) {
						return false;
					}
				}
			}
			// Hmm, not local
			// Do not reject the SAK
			return true;
		}
	}
}

int main (int argc, char *argv[])
{
	int mPipe_fd;
	char readbuf[128];
	int numread;

	int verifier_result = tde_sak_verify_calling_process();

	bool isdm = false;
	if (argc == 2) {
		if (strcmp(argv[1], "dm") == 0) {
			isdm = true;
		}
	}

	if (!isdm) {
		// Verify that the session is local
		// Remote sessions cannot press the SAK for obvious reasons
		if (!is_vt_local()) {
			return 6;	// SAK not available
		}
	}

	if (verifier_result == 0) {
			// OK, the calling process is authorized to retrieve SAK data
			// First, flush the buffer
			mPipe_fd = open(FIFO_FILE, O_RDONLY | O_NONBLOCK);
			numread = 1;
			while (numread > 0) {
				numread = read(mPipe_fd, readbuf, 6);
			}
			close(mPipe_fd);
			// Now wait for SAK press
			mPipe_fd = open(FIFO_FILE, O_RDONLY);
			while (mPipe_fd > -1) {
				numread = read(mPipe_fd, readbuf, 6);
				readbuf[numread] = 0;
				readbuf[127] = 0;
				if (strcmp(readbuf, "SAK\n\r") == 0) {
					close(mPipe_fd);
					if (is_vt_active()) {
						return 0;
					}
					else {
						usleep(100);
						// Flush the buffer
						mPipe_fd = open(FIFO_FILE, O_RDONLY | O_NONBLOCK);
						numread = 1;
						while (numread > 0) {
							numread = read(mPipe_fd, readbuf, 6);
						}
						close(mPipe_fd);
						mPipe_fd = open(FIFO_FILE, O_RDONLY);
					}
				}
				else {
					usleep(100);
				}
			}
			close(mPipe_fd);
			return 6;
	}
	else {
		return verifier_result;
	}
}