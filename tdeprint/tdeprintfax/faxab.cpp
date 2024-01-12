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

#include "faxab.h"

#include <tqlabel.h>
#include <tqlayout.h>
#include <kpushbutton.h>
#include <tdelistview.h>
#include <tqheader.h>
#include <tdelocale.h>
#include <tdeabc/stdaddressbook.h>
#include <tdemessagebox.h>
#include <kiconloader.h>
#include <krun.h>
#include <kstdguiitem.h>
#include <tdeglobal.h>
#include <tdeconfig.h>

FaxAB::FaxAB(TQWidget *parent, const char *name)
: KDialog(parent, name, true)
{
	m_list = new TDEListView( this );
	m_list->addColumn( i18n( "Name" ) );
	m_list->addColumn( i18n( "Fax Number" ) );
	m_list->header()->setStretchEnabled( true, 0 );
	TQLabel	*m_listlabel = new TQLabel(i18n("Entries:"), this);
	m_ok = new KPushButton(KStdGuiItem::ok(), this);
	TQPushButton	*m_cancel = new KPushButton(KStdGuiItem::cancel(), this);
	TQPushButton	*m_ab = new KPushButton(KGuiItem(i18n("&Edit Addressbook"), "contents"), this);
	connect(m_ok, TQ_SIGNAL(clicked()), TQ_SLOT(accept()));
	connect(m_cancel, TQ_SIGNAL(clicked()), TQ_SLOT(reject()));
	connect(m_ab, TQ_SIGNAL(clicked()), TQ_SLOT(slotEditAb()));
	m_ok->setDefault(true);

	TQVBoxLayout	*l0 = new TQVBoxLayout(this, 10, 10);
	l0->addWidget( m_listlabel );
	l0->addWidget( m_list );
	TQHBoxLayout	*l2 = new TQHBoxLayout(0, 0, 10);
	l0->addLayout(l2, 0);
	l2->addWidget(m_ab, 0);
	l2->addStretch(1);
	l2->addWidget(m_ok, 0);
	l2->addWidget(m_cancel, 0);

	TDEConfig *conf = TDEGlobal::config();
	conf->setGroup( "General" );
	TQSize defsize( 400, 200 );
	resize( conf->readSizeEntry( "ABSize", &defsize ) );

	initialize();
	connect(TDEABC::StdAddressBook::self(), TQ_SIGNAL(addressBookChanged(AddressBook*)), TQ_SLOT(slotAbChanged(AddressBook*)));
}

FaxAB::~FaxAB()
{
	TDEConfig *conf = TDEGlobal::config();
	conf->setGroup( "General" );
	conf->writeEntry( "ABSize", size() );
}

void FaxAB::initialize()
{
	m_entries.clear();
	m_list->clear();

	TDEABC::AddressBook	*bk = TDEABC::StdAddressBook::self();
	for (TDEABC::AddressBook::Iterator it=bk->begin(); it!=bk->end(); ++it)
	{
		TDEABC::PhoneNumber::List	numbers = (*it).phoneNumbers();
		TDEABC::PhoneNumber::List faxNumbers;
		for (TQValueList<TDEABC::PhoneNumber>::Iterator nit=numbers.begin(); nit!=numbers.end(); ++nit)
		{
			if (((*nit).type() & TDEABC::PhoneNumber::Fax) && !(*nit).number().isEmpty())
				faxNumbers << ( *nit );
		}
		if (faxNumbers.count() > 0)
		{
			for ( TDEABC::PhoneNumber::List::ConstIterator nit = faxNumbers.begin(); nit != faxNumbers.end(); ++nit )
			{
				FaxABEntry entry;
				entry.m_number = ( *nit );
				entry.m_enterprise = ( *it ).organization();
				if ( !( *it ).formattedName().isEmpty() )
					entry.m_name = ( *it ).formattedName();
				else
				{
					TQString key = ( *it ).familyName();
					if ( !( *it ).givenName().isEmpty() )
					{
						if ( !key.isEmpty() )
							key.append( " " );
						key.append( ( *it ).givenName() );
					}
					entry.m_name = key;
				}
				entry.m_name += ( " (" + ( *nit ).typeLabel() + ")" );
				m_entries[ entry.m_name ] = entry;
			}
		}
	}

	if (m_entries.count() > 0)
	{
		for (TQMap<TQString,FaxABEntry>::ConstIterator it=m_entries.begin(); it!=m_entries.end(); ++it)
		{
			TQCheckListItem *item = new TQCheckListItem( m_list, it.key(), TQCheckListItem::CheckBox );
			item->setText( 1, ( *it ).m_number.number() );
			item->setText( 2, ( *it ).m_enterprise );
		}
		m_list->sort();
		m_ok->setEnabled(true);
	}
	else
		m_ok->setDisabled(true);
}

void FaxAB::slotEditAb()
{
	KRun::runCommand("kaddressbook");
}

void FaxAB::slotAbChanged(AddressBook*)
{
	initialize();
}

bool FaxAB::getEntry(TQStringList& number, TQStringList& name, TQStringList& enterprise, TQWidget *parent)
{
	FaxAB	kab(parent);
	if (!kab.isValid())
	{
		KMessageBox::error(parent, i18n("No fax number found in your address book."));
		return false;
	}
	if (kab.exec())
	{
		TQListViewItemIterator it( kab.m_list, TQListViewItemIterator::Checked );
		while ( it.current() )
		{
			number << it.current()->text( 1 );
			name << it.current()->text( 0 );
			enterprise << it.current()->text( 2 );
			++it;
		}
		/*
		number = kab.m_fax->currentText();
		name = kab.m_name->currentText();
		if (kab.m_entries.contains(name))
		{
			enterprise = kab.m_entries[name][0];
		}
		*/
		return true;
	}

	return false;
}

bool FaxAB::getEntryByNumber(const TQString& number, TQString& name, TQString& enterprise)
{
	TDEABC::AddressBook *bk = TDEABC::StdAddressBook::self();
	for (TDEABC::AddressBook::Iterator it=bk->begin(); it!=bk->end(); ++it)
	{
		TDEABC::PhoneNumber::List	numbers = (*it).phoneNumbers();
		TQStringList	filteredNumbers;
		for (TQValueList<TDEABC::PhoneNumber>::Iterator nit=numbers.begin(); nit!=numbers.end(); ++nit)
		{
			if (((*nit).type() & TDEABC::PhoneNumber::Fax) )
			{
				TQString strippedNumber;
				for (uint i = 0; i < (*nit).number().length(); ++i)
					if ((*nit).number()[i].isDigit() || ( *nit ).number()[ i ] == '+')
						strippedNumber.append((*nit).number()[i]);

				if ( strippedNumber == number)
				{
					enterprise = (*it).organization();
					name = (*it).formattedName();
					return true;
				}
			}
		}
	}

	return false;
}

bool FaxAB::isValid()
{
	return true;
	//return (m_name->count() > 0);
}

#include "faxab.moc"
