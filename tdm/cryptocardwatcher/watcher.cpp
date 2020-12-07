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

#include "watcher.h"

#include <ksslcertificate.h>

#include <tdehardwaredevices.h>
#include <tdecryptographiccarddevice.h>

#include <dmctl.h>
#include <kuser.h>

CardWatcher::CardWatcher() : TQObject() {
	//
}

CardWatcher::~CardWatcher() {
	//
}

void CardWatcher::cryptographicCardInserted(TDECryptographicCardDevice* cdevice) {
	TQString login_name = TQString::null;
	X509CertificatePtrList certList = cdevice->cardX509Certificates();
	if (certList.count() > 0) {
		KSSLCertificate* card_cert = NULL;
		card_cert = KSSLCertificate::fromX509(certList[0]);
		TQStringList cert_subject_parts = TQStringList::split("/", card_cert->getSubject(), false);
		for (TQStringList::Iterator it = cert_subject_parts.begin(); it != cert_subject_parts.end(); ++it ) {
			TQString lcpart = (*it).lower();
			if (lcpart.startsWith("cn=")) {
				login_name = lcpart.right(lcpart.length() - strlen("cn="));
			}
		}
		delete card_cert;
	}

	if (login_name != "") {
		// Determine if user already has an active session
		DM dm;
		SessList sess;
		bool user_active = false;
		bool unused_session_available = false;
		bool unused_session_active = false;
		int unused_session_vt_number = -1;
		if (dm.localSessions(sess)) {
			TQString user, loc;
			for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
				DM::sess2Str2(*it, user, loc);
				if (user.startsWith(login_name + ": ")) {
					// Found active session
					user_active = true;

					// Switch VTs
					DM().switchVT((*it).vt);

					break;
				}
				if (user == "Unused") {
					// Found active unused session
					unused_session_available = true;
					unused_session_vt_number = (*it).vt;
					if ((*it).vt == dm.activeVT()) {
						unused_session_active = true;
						break;
					}
				}
			}
		}
		if (!user_active || unused_session_available) {
			if (unused_session_available) {
				if (!unused_session_active) {
					// Switch to unused VT
					DM().switchVT(unused_session_vt_number);
				}
			}
			else {
				// Activate new VT
				DM().startReserve();
			}
		}
	}
}

void CardWatcher::cryptographicCardRemoved(TDECryptographicCardDevice* cdevice) {
	//
}

#include "watcher.moc"