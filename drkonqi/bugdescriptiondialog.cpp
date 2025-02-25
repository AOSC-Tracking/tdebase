/* This file is part of the TDE project
   Copyright (C) 2014 Timothy Pearson <kb9vqf@pearsoncomputing.net>

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

#include <tqhbuttongroup.h>
#include <tqpushbutton.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqmultilineedit.h>
#include <tqradiobutton.h>
#include <tqregexp.h>

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <klineedit.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdeprocess.h>
#include <tdestandarddirs.h>
#include <kstdguiitem.h>
#include <kurl.h>
#include <kurllabel.h>

#include "bugdescriptiondialog.h"

#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

#include <sys/utsname.h>

#include <kcombobox.h>
#include <config.h>
#include <tdetempfile.h>
#include <tqtextstream.h>
#include <tqfile.h>

class BugDescriptionDialogPrivate {
	public:
};

BugDescriptionDialog::BugDescriptionDialog( TQWidget * parentw, bool modal, const TDEAboutData *aboutData )
  : KDialogBase( Plain,
                 i18n("Bug Description"),
                 Ok | Cancel,
                 Ok,
                 parentw,
                 "BugDescriptionDialog",
                 modal, // modal
                 true // separator
                 )
{
	d = new BugDescriptionDialogPrivate;

	TQWidget * parent = plainPage();
	
	TQVBoxLayout * lay = new TQVBoxLayout( parent, 0, spacingHint() );
	
	TQGridLayout *glay = new TQGridLayout( lay, 4, 3 );
	glay->setColStretch( 1, 10 );
	glay->setColStretch( 2, 10 );
	
	showButtonOK( true );

	// Email
	TQHBoxLayout * hlay = new TQHBoxLayout( lay );
	m_emailLabel = new TQLabel( i18n("Contact Email: "), parent );
	hlay->addWidget( m_emailLabel );
	m_email = new KLineEdit( parent );
	m_email->setFocus();
	m_emailLabel->setBuddy(m_email);
	hlay->addWidget( m_email );
	
	TQString text = i18n("Enter the text (in English if possible) that you wish to submit with this crash report.\n");
	m_descriptionLabel = new TQLabel( parent, "label" );
	
	m_descriptionLabel->setText( text );
	lay->addWidget( m_descriptionLabel );
	
	// The multiline-edit
	m_lineedit = new TQMultiLineEdit( parent, "TQMultiLineEdit" );
	m_lineedit->setMinimumHeight( 180 ); // make it big
	m_lineedit->setWordWrap(TQMultiLineEdit::WidgetWidth);
	lay->addWidget( m_lineedit, 10 /*stretch*/ );
}

BugDescriptionDialog::~BugDescriptionDialog()
{
	delete d;
}

void BugDescriptionDialog::slotOk( void )
{
	accept();
}

void BugDescriptionDialog::slotCancel()
{
	KDialogBase::slotCancel();
}

void BugDescriptionDialog::virtual_hook( int id, void* data )
{ KDialogBase::virtual_hook( id, data ); }

#include "bugdescriptiondialog.moc"
