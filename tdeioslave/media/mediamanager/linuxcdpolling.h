/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _LINUXCDPOLLING_H_
#define _LINUXCDPOLLING_H_

#include "backendbase.h"

#include <tqobject.h>
#include <tqcstring.h>
#include <tqmap.h>
#include <tqtimer.h>

class DiscType
{
public:
	enum Type { None, Unknown, Audio, Data, DVD, Mixed,
	            Blank, VCD, SVCD, BLURAY, UnknownType, Broken };

	DiscType(Type type = Unknown);

	bool isKnownDisc() const;
	bool isDisc() const;
	bool isNotDisc() const;
	bool isData() const;

	operator int() const;

private:
	Type m_type;
};

class PollingThread;

class LinuxCDPolling : public TQObject, public BackendBase
{
TQ_OBJECT

public:

	LinuxCDPolling(MediaList &list);
	virtual ~LinuxCDPolling();

	/**
	 * Find the disc type of the medium inserted in a drive
	 * (considered to be a cdrom or dvdrom)
	 *
	 * @param devNode the path to the device to test
	 * @param current the current known state of the drive
	 * @return the disc type
	 */
	static DiscType identifyDiscType(const TQCString &devNode,
		 const DiscType &current = DiscType::Unknown);

private slots:
	void slotMediumAdded(const TQString &id);
	void slotMediumRemoved(const TQString &id);
	void slotMediumStateChanged(const TQString &id);
	void slotTimeout();

private:
	void applyType(DiscType type, const Medium *medium);

	static bool hasDirectory(const TQCString &devNode, const TQCString &dir);

	TQMap<TQString, PollingThread*> m_threads;
	TQStringList m_excludeNotification;
	TQTimer m_timer;
};

#endif
