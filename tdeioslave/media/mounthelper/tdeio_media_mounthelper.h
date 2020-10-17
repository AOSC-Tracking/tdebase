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

#ifndef _TDEIO_MEDIA_MOUNTHELPER_H_
#define _TDEIO_MEDIA_MOUNTHELPER_H_

#include <tdeapplication.h>
#include <tqstring.h>
#include <tdeio/job.h>

#include "medium.h"

class Dialog;

class MountHelper : public TDEApplication
{
        Q_OBJECT
public:
	MountHelper();

private:
	TQString m_errorStr;
	TQString m_mediumId;
	Dialog *m_dialog;
	DCOPRef m_mediamanager;

	const Medium findMedium(const TQString &device);
	void error();

	void mount(const Medium &medium);
	void unmount(const Medium &medium);
	void unlock(const Medium &medium);
	void lock(const Medium &medium);
	void eject(const TQString &device, bool quiet=false);
	void safeRemoval(const Medium &medium);
	void releaseHolders(const Medium &medium, bool handleThis = false);

private slots:
	void slotSendPassword();
	void slotCancel();
	void ejectFinished(TDEProcess* proc);
	void errorAndExit();
};

#endif
