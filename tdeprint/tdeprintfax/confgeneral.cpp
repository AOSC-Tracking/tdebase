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

#include "confgeneral.h"

#include <tqlineedit.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqcheckbox.h>

#include <tdeglobal.h>
#include <tdeconfig.h>
#include <tdelocale.h>
#include <kseparator.h>

#include <stdlib.h>

ConfGeneral::ConfGeneral(TQWidget *parent, const char *name)
: TQWidget(parent, name)
{
	m_name = new TQLineEdit(this);
	m_company = new TQLineEdit(this);
	m_number = new TQLineEdit(this);
	TQLabel	*m_namelabel = new TQLabel(i18n("&Name:"), this);
	m_namelabel->setBuddy(m_name);
	TQLabel	*m_companylabel = new TQLabel(i18n("&Company:"), this);
	m_companylabel->setBuddy(m_company);
	TQLabel	*m_numberlabel = new TQLabel(i18n("N&umber:"), this);
        m_numberlabel->setBuddy(m_number);
	KSeparator *sep = new KSeparator( this );
	m_replace_int_char = new TQCheckBox( i18n( "Replace international prefix '+' with:" ), this );
	m_replace_int_char_val = new TQLineEdit( this );
	m_replace_int_char_val->setEnabled( false );

	connect( m_replace_int_char, TQ_SIGNAL( toggled( bool ) ), m_replace_int_char_val, TQ_SLOT( setEnabled( bool ) ) );

	TQGridLayout	*l0 = new TQGridLayout(this, 6, 2, 10, 10);
	l0->setColStretch(1, 1);
	l0->setRowStretch(5, 1);
	l0->addWidget(m_namelabel, 0, 0);
	l0->addWidget(m_companylabel, 1, 0);
	l0->addWidget(m_numberlabel, 2, 0);
	l0->addWidget(m_name, 0, 1);
	l0->addWidget(m_company, 1, 1);
	l0->addWidget(m_number, 2, 1);
	l0->addMultiCellWidget( sep, 3, 3, 0, 1 );
	TQHBoxLayout *l1 = new TQHBoxLayout( this, 0, 10 );
	l0->addMultiCellLayout( l1, 4, 4, 0, 1 );
	l1->addWidget( m_replace_int_char );
	l1->addWidget( m_replace_int_char_val );
}

void ConfGeneral::load()
{
	TDEConfig	*conf = TDEGlobal::config();
	conf->setGroup("Personal");
	m_name->setText(conf->readEntry("Name", getenv("USER")));
	m_number->setText(conf->readEntry("Number"));
	m_company->setText(conf->readEntry("Company"));
	m_replace_int_char->setChecked( conf->readBoolEntry( "ReplaceIntChar", false ) );
	m_replace_int_char_val->setText( conf->readEntry( "ReplaceIntCharVal" ) );
}

void ConfGeneral::save()
{
	TDEConfig	*conf = TDEGlobal::config();
	conf->setGroup("Personal");
	conf->writeEntry("Name", m_name->text());
	conf->writeEntry("Number", m_number->text());
	conf->writeEntry("Company", m_company->text());
	conf->writeEntry( "ReplaceIntChar", m_replace_int_char->isChecked() );
	conf->writeEntry( "ReplaceIntCharVal", m_replace_int_char_val->text() );
}
