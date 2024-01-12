/*
 * Copyright 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 * 
 * This file is part of cryptocardwatcher, the TDE Cryptographic Card Session Monitor
 * 
 * cryptocardwatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * cryptocardwatcher is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with cryptocardwatcher. If not, see http://www.gnu.org/licenses/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <stdint.h>

#include <tqobject.h>

#include <tdeapplication.h>
#include <tdecmdlineargs.h>

#include <ksslcertificate.h>

#include <tdehardwaredevices.h>
#include <tdecryptographiccarddevice.h>

#include "watcher.h"

int lockfd = -1;
char lockFileName[256];

// --------------------------------------------------------------------------------------
// Useful function from Stack Overflow
// http://stackoverflow.com/questions/1599459/optimal-lock-file-method
// --------------------------------------------------------------------------------------
int tryGetLock(char const *lockName) {
	mode_t m = umask( 0 );
	int fd = open( lockName, O_RDWR|O_CREAT, 0666 );
	umask( m );
	if( fd >= 0 && flock( fd, LOCK_EX | LOCK_NB ) < 0 ) {
		close( fd );
		fd = -1;
	}
	return fd;
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Useful function from Stack Overflow
// http://stackoverflow.com/questions/1599459/optimal-lock-file-method
// --------------------------------------------------------------------------------------
void releaseLock(int fd, char const *lockName) {
	if( fd < 0 ) {
		return;
	}
	remove( lockName );
	close( fd );
}
// --------------------------------------------------------------------------------------

void handle_sigterm(int signum) {
	if (lockfd >= 0) {
		releaseLock(lockfd, lockFileName);
	}
	exit(0);
}

static TDECmdLineOptions options[] =
{
	TDECmdLineLastOption
};

int main(int argc, char *argv[]) {
	int ret = -1;

	// Register cleanup handlers
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = handle_sigterm;
	sigaction(SIGTERM, &action, NULL);

	// Ensure only one process is running
	sprintf(lockFileName, "/var/lock/cryptocardwatcher.lock");
	lockfd = tryGetLock(lockFileName);
	if (lockfd < 0) {
		printf ("[cryptocardwatcher] Another instance of this program is already running!\n[cryptocardwatcher] Lockfile detected at '%s'\n", lockFileName);
		return -2;
	}

	// Parse command line arguments
	TDECmdLineArgs::init(argc, argv, "cryptocardwatcher", "cryptocardwatcher", "TDE Cryptographic Card Session Monitor", "0.1");
	TDECmdLineArgs::addCmdLineOptions(options);
	TDEApplication::addCmdLineOptions();

	// Initialize TDE application
	TDEApplication tdeapp(false, false);
	tdeapp.disableAutoDcopRegistration();
	CardWatcher* watcher = new CardWatcher();

	// Initialize SmartCard readers
	TDEGenericDevice *hwdevice;
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList cardReaderList = hwdevices->listByDeviceClass(TDEGenericDeviceType::CryptographicCard);
	for (hwdevice = cardReaderList.first(); hwdevice; hwdevice = cardReaderList.next()) {
		TDECryptographicCardDevice* cdevice = static_cast<TDECryptographicCardDevice*>(hwdevice);
		TQObject::connect(cdevice, TQ_SIGNAL(cardInserted(TDECryptographicCardDevice*)), watcher, TQ_SLOT(cryptographicCardInserted(TDECryptographicCardDevice*)));
		TQObject::connect(cdevice, TQ_SIGNAL(cardRemoved(TDECryptographicCardDevice*)), watcher, TQ_SLOT(cryptographicCardRemoved(TDECryptographicCardDevice*)));
		cdevice->enableCardMonitoring(true);
	}

	// Start TDE application
	ret = tdeapp.exec();

	// Clean up
	delete watcher;

	releaseLock(lockfd, lockFileName);
	return ret;
}
