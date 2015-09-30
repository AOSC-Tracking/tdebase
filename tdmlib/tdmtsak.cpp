/*
   This file is part of the TDE project
   Copyright (C) 2011 - 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>

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

#include <tqstringlist.h>

#include "dmctl.h"

#include "tdmtsak.h"

#define FIFO_FILE "/tmp/tdesocket-global/tsak"

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
		DM dm;
		SessList sess;
		if (dm.localSessions(sess)) {
			TQString user, loc;
			for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
				DM::sess2Str2(*it, user, loc);
				TQStringList sessionInfoList = TQStringList::split(',', loc, true);
				if ((*(sessionInfoList.at(0))).startsWith(":")) {
					if (TQString(currentDisplay).startsWith(*(sessionInfoList.at(0)))) {
						return true;
					}
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
		DM dm;
		SessList sess;
		TQString cvtName = "";
		TQString curConsole;
		int curConsoleNum = dm.activeVT();
		if (curConsoleNum < 0) {
			return true;
		}
		curConsole = TQString("vt%1").arg(curConsoleNum);
		if (dm.localSessions(sess)) {
			TQString user, loc;
			for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
				DM::sess2Str2(*it, user, loc);
				TQStringList sessionInfoList = TQStringList::split(',', loc, true);
				if ((*(sessionInfoList.at(0))).startsWith(":")) {
					if ((*(sessionInfoList.at(1))).stripWhiteSpace() == TQString(curConsole)) {
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
				for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
					DM::sess2Str2(*it, user, loc);
					if ((*it).self) {
						TQStringList sessionInfoList = TQStringList::split(',', loc, true);
						if ((*(sessionInfoList.at(1))).startsWith(" vt")) {
							// Local and inactive
							return false;
						}
					}
				}
				// Hmm, not local
				// Do not reject the SAK
				return true;
			}
		}
		else {
			// Failure!
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
	bool checkonly = false;
	if (argc == 2) {
		if (strcmp(argv[1], "dm") == 0) {
			isdm = true;
		}
		if (strcmp(argv[1], "check") == 0) {
			checkonly = true;
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
		if (checkonly) {
			if (mPipe_fd < 0) {
				return 6;	// SAK not available
			}
			else {
				return 0;
			}
		}
		numread = 1;
		while (numread > 0) {
			numread = read(mPipe_fd, readbuf, 6);
		}
		close(mPipe_fd);
		// Now wait for SAK press
		while (mPipe_fd > -1) {
			mPipe_fd = open(FIFO_FILE, O_RDONLY);

			if (mPipe_fd <= -1) {
				// This may be a transient glitch, such as when a KVM is being toggled or a new keyboard has been added
				// Wait up to 5 seconds while trying to open the pipe again
				int timeout = 5;
				while ((mPipe_fd <= -1) && (timeout > 0)) {
					sleep(1);
					mPipe_fd = open(FIFO_FILE, O_RDONLY);
					timeout--;
				}
			}

			if (mPipe_fd > -1) {
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
		}
		return 6;
	}
	else {
		return verifier_result;
	}
}
