/* This file is part of the KDE project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>
   Parts of this file are
   Copyright 2003 Waldo Bastian <bastian@kde.org>

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
#include <tdelocale.h>
#include <tdeapplication.h>
#include <kurl.h>
#include <tdemessagebox.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <tqtimer.h>
#include <stdlib.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <kprocess.h>
#include <tdestartupinfo.h>
#include <kmimetype.h>
#ifdef __TDE_HAVE_TDEHWLIB
#include <tdehardwaredevices.h>
#endif

#include "dialog.h"
#include "tdeio_media_mounthelper.h"

const Medium MountHelper::findMedium(const KURL &url)
{
	DCOPRef mediamanager("kded", "mediamanager");

	// Try filename first
	DCOPReply reply = mediamanager.call("properties", url.fileName());
	if (!reply.isValid()) {
		m_errorStr = i18n("The TDE mediamanager is not running.")+"\n";
		return Medium(TQString::null, TQString::null, TQString::null);
	}
	const Medium& medium = Medium::create(reply);
	if (medium.id().isEmpty()) {
		// Try full URL now
		reply = mediamanager.call("properties", url.prettyURL());
		if (!reply.isValid()) {
			m_errorStr = i18n("Internal Error");
			return Medium(TQString::null, TQString::null, TQString::null);
		}
		return Medium::create(reply);
	} else {
		return medium;
	}
}

MountHelper::MountHelper() : TDEApplication()
{
	TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

	m_errorStr = TQString::null;

	KURL url(args->url(0));
	const Medium medium = findMedium(url);
	if (medium.id().isEmpty())
	{
		if (m_errorStr.isEmpty()) {
			m_errorStr+= i18n("%1 cannot be found.").arg(url.prettyURL());
		}
		TQTimer::singleShot(0, this, TQT_SLOT(error()));
		return;
	}

	if (!medium.isMountable() && !args->isSet("e") && !args->isSet("s"))
	{
		m_errorStr = i18n("%1 is not a mountable media.").arg(url.prettyURL());
		TQTimer::singleShot(0, this, TQT_SLOT(error()));
		return;
	}

	TQString device = medium.deviceNode();
	TQString mount_point = medium.mountPoint();

	m_isCdrom = medium.mimeType().find("dvd") != -1 || medium.mimeType().find("cd") != -1;

	if (args->isSet("m"))
	{
		// Mount drive
		DCOPRef mediamanager("kded", "mediamanager");
		DCOPReply reply = mediamanager.call("mount", medium.id());
		TQStringVariantMap mountResult;
		if (reply.isValid()) {
			reply.get(mountResult);
		}
		if (mountResult.contains("result") && mountResult["result"].toBool()) {
			::exit(0);
		}
		else {
			m_errorStr = mountResult.contains("errStr") ? mountResult["errStr"].toString() : i18n("Unknown mount error.");
			TQTimer::singleShot(0, this, TQT_SLOT(error()));
		}
	}
	else if (args->isSet("u"))
	{
		// Unmount drive
		DCOPRef mediamanager("kded", "mediamanager");
		DCOPReply reply = mediamanager.call("unmount", medium.id());
		TQStringVariantMap unmountResult;
		if (reply.isValid()) {
			reply.get(unmountResult);
		}
		if (unmountResult.contains("result") && unmountResult["result"].toBool()) {
			::exit(0);
		}
		else {
			m_errorStr = unmountResult.contains("errStr") ? unmountResult["errStr"].toString() : i18n("Unknown unmount error.");
			kdDebug() << "medium unmount " << m_errorStr << endl;
			TQTimer::singleShot(0, this, TQT_SLOT(error()));
		}
	}
	else if (args->isSet("k"))
	{
		// Unlock drive
		if (!medium.isEncrypted())
		{
			m_errorStr = i18n("%1 is not an encrypted media.").arg(url.prettyURL());
			TQTimer::singleShot(0, this, TQT_SLOT(error()));
			return;
		}
		if (!medium.needDecryption())
		{
			m_errorStr = i18n("%1 is already unlocked.").arg(url.prettyURL());
			TQTimer::singleShot(0, this, TQT_SLOT(error()));
			return;
		}

		TQString iconName = medium.iconName();
		if (iconName.isEmpty())
		{
			TQString mime = medium.mimeType();
			iconName = KMimeType::mimeType(mime)->icon(mime, false);
		}
		m_mediumId = medium.id();
		dialog = new Dialog(url.prettyURL(), iconName);
		connect(dialog, TQT_SIGNAL(user1Clicked()), this, TQT_SLOT(slotSendPassword()));
		connect(dialog, TQT_SIGNAL(cancelClicked()), this, TQT_SLOT(slotCancel()));
		connect(this, TQT_SIGNAL(signalPasswordError(TQString)), dialog, TQT_SLOT(slotDialogError(TQString)));
		dialog->show();
	}
	else if (args->isSet("l"))
	{
		// Lock drive
		DCOPRef mediamanager("kded", "mediamanager");
		DCOPReply reply = mediamanager.call("lock", medium.id());
		TQStringVariantMap lockResult;
		if (reply.isValid()) {
			reply.get(lockResult);
		}
		if (lockResult.contains("result") && lockResult["result"].toBool()) {
	    ::exit(0);
	  }
	  else {
			m_errorStr = lockResult.contains("errStr") ? lockResult["errStr"].toString() : i18n("Unknown lock error.");
			kdDebug() << "medium lock " << m_errorStr << endl;
			TQTimer::singleShot(0, this, TQT_SLOT(error()));
		}
	}
	else if (args->isSet("e"))
	{
		invokeEject(device, true);
	}
	else if (args->isSet("s"))
	{
		// Safely remove drive
		DCOPRef mediamanager("kded", "mediamanager");

		/*
		* We want to call mediamanager unmount before invoking eject. That's
		* because unmount would provide an informative error message in case of
		* failure. However, there are cases when unmount would fail
		* (supermount, slackware, see bug#116209) but eject would succeed.
		* Thus if unmount fails, save unmount error message and invokeEject()
		* anyway. Only if both unmount and eject fail, notify the user by
		* displaying the saved error message (see ejectFinished()).
		*/
		TQStringVariantMap unmountResult;
		if (medium.isMounted())
		{
			DCOPReply reply = mediamanager.call("unmount", medium.id());
			if (reply.isValid()) {
				reply.get(unmountResult);
				if (unmountResult.contains("result") && !unmountResult["result"].toBool()) {
					m_errorStr = unmountResult.contains("errStr") ? unmountResult["errStr"].toString() : i18n("Unknown unmount error.");
				}
			}
		}

		// If this is an unlocked encrypted volume and there is no error yet, we try to lock it
		if (unmountResult.contains("result") && unmountResult["result"].toBool() &&
		    medium.isEncrypted() && !medium.clearDeviceUdi().isNull())
		{
			DCOPReply reply = mediamanager.call("lock", medium.id());
			if (reply.isValid()) {
				TQStringVariantMap lockResult;
				reply.get(lockResult);
				if (lockResult.contains("result") && !lockResult["result"].toBool()) {
					m_errorStr = lockResult.contains("errStr") ? lockResult["errStr"].toString() : i18n("Unknown lock error.");
				}
			}
		}

		if (m_errorStr.isEmpty()) {
			invokeEject(device, true);
		}
		else {
			TQTimer::singleShot(0, this, TQT_SLOT(error()));
		}
	}
	else
	{
		TDECmdLineArgs::usage();
	}
}

void MountHelper::invokeEject(const TQString &device, bool quiet)
{
#ifdef __TDE_HAVE_TDEHWLIB
	// Try TDE HW library eject first...
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericDevice *hwdevice = hwdevices->findByDeviceNode(device);
	if (hwdevice->type() == TDEGenericDeviceType::Disk) {
		TDEStorageDevice *sdevice = static_cast<TDEStorageDevice*>(hwdevice);
		if (sdevice->ejectDrive()) {
			// Success!
			::exit(0);
		}
	}
#endif

	// Then fall back to tdeeject if needed
	TDEProcess *proc = new TDEProcess(TQT_TQOBJECT(this));
	*proc << "tdeeject";
	if (quiet)
	{
		*proc << "-q";
	}
	*proc << device;
	connect(proc, TQT_SIGNAL(processExited(TDEProcess*)),	this, TQT_SLOT(ejectFinished(TDEProcess*)));
	proc->start();
}

void MountHelper::ejectFinished(TDEProcess *proc)
{
	//If eject failed, report the error stored in m_errorStr
	if (proc->normalExit() && proc->exitStatus() == 0) {
		::exit(0);
	}
	else {
		if (!m_errorStr.isEmpty()) {
			TQTimer::singleShot(0, this, TQT_SLOT(error()));
		}
		::exit(0);
	}
}

void MountHelper::error()
{
	TQString prettyErrorString = m_errorStr;
	if (m_errorStr.contains("<") && m_errorStr.contains(">")) {
		if (!m_errorStr.contains("<qt>")) {
			prettyErrorString = TQString("<qt>%1</qt>").arg(m_errorStr);
		}
	}
	KMessageBox::error(0, prettyErrorString);
	::exit(1);
}

void MountHelper::slotSendPassword()
{
	DCOPRef mediamanager("kded", "mediamanager");

	DCOPReply reply = mediamanager.call("unlock", m_mediumId, dialog->getPassword());
	TQStringVariantMap unlockResult;
	if (reply.isValid()) {
		reply.get(unlockResult);
	}
	if (unlockResult.contains("result") && unlockResult["result"].toBool()) {
		::exit(0);
	}
	else {
		m_errorStr = unlockResult.contains("errStr") ? unlockResult["errStr"].toString() : i18n("Unknown unlock error.");
		kdDebug() << "medium unlock " << m_errorStr << endl;
		emit signalPasswordError(m_errorStr);
		TQTimer::singleShot(0, this, TQT_SLOT(error()));
	}
}

void MountHelper::slotCancel()
{
	exit(0);
}

static TDECmdLineOptions options[] =
{
	{ "m", I18N_NOOP("Mount given URL"), 0 },
	{ "u", I18N_NOOP("Unmount given URL"), 0 },
	{ "k", I18N_NOOP("Unlock given URL"), 0 },
	{ "l", I18N_NOOP("Lock given URL"), 0 },
	{ "e", I18N_NOOP("Eject given URL"), 0},
	{ "s", I18N_NOOP("Safely remove (unmount and eject) given URL"), 0},
	{"!+URL", I18N_NOOP("media:/URL to mount/unmount/unlock/lock/eject/remove"), 0 },
	TDECmdLineLastOption
};


int main(int argc, char **argv)
{
	TDECmdLineArgs::init(argc, argv, "tdeio_media_mounthelper",
	                   "tdeio_media_mounthelper", "tdeio_media_mounthelper",
	                   "0.1");

	TDECmdLineArgs::addCmdLineOptions(options);
	TDEGlobal::locale()->setMainCatalogue("tdeio_media");
	TDEApplication::addCmdLineOptions();
	if (TDECmdLineArgs::parsedArgs()->count()==0)
	{
		TDECmdLineArgs::usage();
	}

	TDEApplication *app = new MountHelper();
	TDEStartupInfo::appStarted();
	app->dcopClient()->attach();
	return app->exec();
}

#include "tdeio_media_mounthelper.moc"
