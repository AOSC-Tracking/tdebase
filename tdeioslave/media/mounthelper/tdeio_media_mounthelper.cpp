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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
#ifdef WITH_TDEHWLIB
#include <tdehardwaredevices.h>
#endif

#include "dialog.h"
#include "tdeio_media_mounthelper.h"

const Medium MountHelper::findMedium(const TQString &device)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("properties", device);
	if (!reply.isValid())
	{
		m_errorStr = i18n("The TDE mediamanager is not running.");
		return Medium(TQString::null, TQString::null, TQString::null);
	}
	const Medium &medium = Medium::create(reply);
	return medium;
}

void MountHelper::mount(const Medium &medium)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("mount", medium.id());
	TQStringVariantMap mountResult;
	if (reply.isValid()) {
		reply.get(mountResult);
	}
	if (!mountResult.contains("result") || !mountResult["result"].toBool()) {
		m_errorStr = mountResult.contains("errStr") ? mountResult["errStr"].toString() : i18n("Unknown mount error.");
		errorAndExit();
	}
}

void MountHelper::unmount(const Medium &medium)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("unmount", medium.id());
	TQStringVariantMap unmountResult;
	if (reply.isValid()) {
		reply.get(unmountResult);
	}
	if (!unmountResult.contains("result") || !unmountResult["result"].toBool()) {
		m_errorStr = unmountResult.contains("errStr") ? unmountResult["errStr"].toString() : i18n("Unknown unmount error.");
		kdDebug() << "medium unmount " << m_errorStr << endl;
		errorAndExit();
	}
}

void MountHelper::unlock(const Medium &medium)
{
	TQString device = medium.deviceNode();
	if (device.isEmpty()) {
		m_errorStr = i18n("Try to unlock an unknown medium.");
		errorAndExit();
	}

	TQString iconName = medium.iconName();
	if (iconName.isEmpty())
	{
		TQString mime = medium.mimeType();
		iconName = KMimeType::mimeType(mime)->icon(mime, false);
	}
	m_mediumId = medium.id();
	m_dialog = new Dialog(device, iconName);
	connect(m_dialog, TQ_SIGNAL(user1Clicked()), this, TQ_SLOT(slotSendPassword()));
	connect(m_dialog, TQ_SIGNAL(cancelClicked()), this, TQ_SLOT(slotCancel()));
	m_dialog->show();
}

void MountHelper::lock(const Medium &medium)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("lock", medium.id(), true);
	TQStringVariantMap lockResult;
	if (reply.isValid()) {
		reply.get(lockResult);
	}
	if (!lockResult.contains("result") || !lockResult["result"].toBool()) {
		m_errorStr = lockResult.contains("errStr") ? lockResult["errStr"].toString() : i18n("Unknown lock error.");
		kdDebug() << "medium lock " << m_errorStr << endl;
		errorAndExit();
	}
}

void MountHelper::eject(const Medium &medium)
{
#ifdef WITH_TDEHWLIB
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("eject", medium.id());
	TQStringVariantMap ejectResult;
	if (reply.isValid()) {
		reply.get(ejectResult);
	}
	if (!ejectResult.contains("result") || !ejectResult["result"].toBool()) {
		m_errorStr = ejectResult.contains("errStr") ? ejectResult["errStr"].toString() : i18n("Unknown eject error.");
		kdDebug() << "medium eject " << m_errorStr << endl;
		errorAndExit();
	}
	else
	{
		::exit(0); // Success!
	}
#endif

	// Otherwise fall back to tdeeject
	TDEProcess *proc = new TDEProcess(this);
	*proc << "tdeeject";
	*proc << medium.deviceNode();
	connect(proc, TQ_SIGNAL(processExited(TDEProcess*)),	this, TQ_SLOT(ejectFinished(TDEProcess*)));
	proc->start();
}

void MountHelper::safeRemoval(const Medium &medium)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("safeRemove", medium.id());
	TQStringVariantMap safeRemoveResult;
	if (reply.isValid()) {
		reply.get(safeRemoveResult);
	}
	if (!safeRemoveResult.contains("result") || !safeRemoveResult["result"].toBool()) {
		m_errorStr = safeRemoveResult.contains("errStr") ? safeRemoveResult["errStr"].toString() : i18n("Unknown safe removal error.");
		kdDebug() << "medium safeRemoval " << m_errorStr << endl;
		errorAndExit();
	}
}

void MountHelper::openRealFolder(const Medium &medium)
{
	Medium &m = const_cast<Medium&>(medium);
	if (!m.isMounted())
	{
		// If the medium is not mounted, try mounting it first
		mount(m);
		m = findMedium(m.deviceNode());
	}

	if (m.isMounted())
	{
		system((TQString("kfmclient exec 'file://") + m.mountPoint()).local8Bit() + "'");
	}
	else
	{
		m_errorStr = i18n("Try to open an unknown medium.");
		errorAndExit();
	}
}

MountHelper::MountHelper() : TDEApplication()
{
	TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
	m_errorStr = TQString::null;

	const Medium medium = findMedium(args->arg(0));
	if (medium.id().isEmpty())
	{
		if (m_errorStr.isEmpty()) {
			m_errorStr+= i18n("%1 cannot be found.").arg(args->arg(0));
		}
		errorAndExit();
	}

	TQString device = medium.deviceNode();
	if (!medium.isMountable() && !medium.isEncrypted() && !args->isSet("e") && !args->isSet("s"))
	{
		m_errorStr = i18n("%1 is not a mountable or encrypted media.").arg(device);
		errorAndExit();
	}

	if (args->isSet("m"))
	{
		mount(medium);
		::exit(0);
	}
	else if (args->isSet("u"))
	{
		unmount(medium);
		::exit(0);
	}
	else if (args->isSet("k"))
	{
		unlock(medium);
		// No call to ::exit() here because this will open up the password dialog
		// ::exit() is handled in the invoked code.
	}
	else if (args->isSet("l"))
	{
		lock(medium);
		::exit(0);
	}
	else if (args->isSet("e"))
	{
		eject(medium);
		::exit(0);
	}
	else if (args->isSet("s"))
	{
		safeRemoval(medium);
		::exit(0);
	}
	else if (args->isSet("f"))
	{
		openRealFolder(medium);
		::exit(0);
	}
	else
	{
		TDECmdLineArgs::usage();
		::exit(0);
	}
}

MountHelper::~MountHelper()
{
	if (m_dialog)
	{
		delete m_dialog;
	}
}

void MountHelper::ejectFinished(TDEProcess *proc)
{
	//If eject failed, report the error stored in m_errorStr
	if (proc->normalExit() && proc->exitStatus() == 0) {
		::exit(0);
	}
	else {
		if (!m_errorStr.isEmpty()) {
			errorAndExit();
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
}

void MountHelper::errorAndExit()
{
	error();
	::exit(1);
}

void MountHelper::slotSendPassword()
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call("unlock", m_mediumId, m_dialog->getPassword());
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
		error();
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
	{ "f", I18N_NOOP("Open real medium folder"), 0},
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
