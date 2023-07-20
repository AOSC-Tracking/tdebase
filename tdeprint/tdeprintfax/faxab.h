/*
 *   tdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef FAXAB_H
#define FAXAB_H

#include <kdialog.h>
#include <tqmap.h>
#include <tqstringlist.h>
#include <tdeabc/phonenumber.h>

class TDEListView;
class AddressBook;
class TQPushButton;

class FaxAB : public KDialog
{
	TQ_OBJECT
public:
	FaxAB(TQWidget *parent = 0, const char *name = 0);
	~FaxAB();
	bool isValid();

	static bool getEntry(TQStringList& number, TQStringList& name, TQStringList& enterprise, TQWidget *parent = 0);
	static bool getEntryByNumber(const TQString& number, TQString& name, TQString& enterprise);

protected slots:
	void slotEditAb();
	void slotAbChanged(AddressBook*);

protected:
	void initialize();

private:
	struct FaxABEntry
	{
		TQString           m_name;
		TDEABC::PhoneNumber m_number;
		TQString           m_enterprise;
	};

	TDEListView*                  m_list;
	TQMap<TQString,FaxABEntry>    m_entries;
	TQPushButton*                m_ok;
};

#endif
