/* This file is part of TDE
   Copyright (C) 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>

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
#include <config.h>

#include <tqradiobutton.h>
#include <tqpushbutton.h>
#include <tqvalidator.h>
#include <tqlineedit.h>
#include <tqiconset.h>
#include <tqlabel.h>
#include <tqtabwidget.h>
#include <tqgroupbox.h>
#include <tqlayout.h>
#include <tqslider.h>
#include <tqpainter.h>
#include <tqstyle.h>
#include <tqfile.h>
#include <private/tqinternal_p.h>
#undef Unsorted // Required for --enable-final (tqdir.h)
#include <tqfiledialog.h>

#include <kpassdlg.h>
#include <kbuttonbox.h>
#include <kcombobox.h>
#include <tdelocale.h>
#include <kiconloader.h>
#include <kurlrequester.h>
#include <tdeapplication.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <tdemessagebox.h>
#include <ksslcertificate.h>

#include "cryptpassworddlg.h"

CryptPasswordDialog::CryptPasswordDialog(TQWidget *parent, TQString passwordPrompt, TQString caption, bool allow_card, KSSLCertificate* card_cert, bool* use_card)
	: KDialogBase(Plain, ((caption == "")?i18n("Enter Password"):caption), Ok|Cancel, Ok, parent, 0L, true, true),
	m_useCard(use_card)
{
	m_base = new CryptPasswordDialogBase(plainPage());

	TQGridLayout *mainGrid = new TQGridLayout(plainPage(), 1, 1, 0, spacingHint());
	mainGrid->setRowStretch(1, 1);
	mainGrid->addWidget(m_base, 0, 0);

	m_base->passwordPrompt->setText(passwordPrompt);
	m_base->passwordIcon->setPixmap(SmallIcon("password.png"));

	if (!allow_card) {
		m_base->cardKeyButton->hide();
		m_base->cardKeyInfo->hide();
	}
	else {
		if (card_cert) {
			m_base->cardKeyInfo->setText(card_cert->getSubject());
		}
	}

	connect(m_base->textPasswordButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(processLockouts()));
	connect(m_base->filePasswordButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(processLockouts()));
	connect(m_base->cardKeyButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(processLockouts()));
	connect(m_base->textPasswordEntry, TQ_SIGNAL(textChanged(const TQString&)), this, TQ_SLOT(processLockouts()));
	connect(m_base->filePasswordURL, TQ_SIGNAL(textChanged(const TQString&)), this, TQ_SLOT(processLockouts()));

	m_base->textPasswordEntry->setFocus();

	processLockouts();
}

CryptPasswordDialog::~CryptPasswordDialog()
{
}

TQByteArray CryptPasswordDialog::password() {
	if (m_base->textPasswordButton->isOn() == true) {
		TQCString pass = m_base->textPasswordEntry->password().utf8();
		m_password.duplicate(pass, pass.length());
		if (m_useCard) *m_useCard = false;
	}
	else if (m_base->filePasswordButton->isOn() == true) {
		m_password = TQFile(m_base->filePasswordURL->url()).readAll();
		if (m_useCard) *m_useCard = false;
	}
	else {
		if (m_useCard) *m_useCard = true;
	}

	return m_password;
}

void CryptPasswordDialog::processLockouts() {
	if (m_base->textPasswordButton->isOn() == true) {
		m_base->textPasswordEntry->setEnabled(true);
		m_base->filePasswordURL->setEnabled(false);
		m_base->textPasswordEntry->setFocus();
		if (m_base->textPasswordEntry->password().length() > 0) {
			enableButtonOK(true);
		}
		else {
			enableButtonOK(false);
		}
	}
	else if (m_base->filePasswordButton->isOn() == true) {
		m_base->textPasswordEntry->setEnabled(false);
		m_base->filePasswordURL->setEnabled(true);
		m_base->filePasswordURL->setFocus();
		if (TQFile(m_base->filePasswordURL->url()).exists()) {
			enableButtonOK(true);
		}
		else {
			enableButtonOK(false);
		}
	}
	else {
		m_base->textPasswordEntry->setEnabled(false);
		m_base->filePasswordURL->setEnabled(false);
		enableButtonOK(true);
	}
}

void CryptPasswordDialog::virtual_hook( int id, void* data )
{ KDialogBase::virtual_hook( id, data ); }

#include "cryptpassworddlg.moc"
