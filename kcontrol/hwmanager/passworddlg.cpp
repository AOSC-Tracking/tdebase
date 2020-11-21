/*  This file is part of the KDE project
 *  Copyright (C) 2007  Jan Klötzke <jan kloetzke at freenet de>
 *
 *  Based on kryptomedia- Another KDE cryto media application.
 *  Copyright (C) 2006  Daniel Gollub <dgollub@suse.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "passworddlg.h"

PasswordDlg::PasswordDlg() :
        KDialogBase(NULL, "PasswordDlg", true, i18n("Unlock Storage Device"), (Cancel|User1),
        User1, false, KGuiItem(i18n("Unlock"), "unlocked" ))
{
	unlockDialog = new UnlockDialog(this);
	enableButton( User1, false );
	unlockDialog->encryptedIcon->setPixmap(TDEGlobal::iconLoader()->loadIcon("drive-harddisk-locked",
				  TDEIcon::NoGroup, TDEIcon::SizeLarge));
	connect(unlockDialog->passwordEdit, TQT_SIGNAL (textChanged(const TQString &)), this, TQT_SLOT (slotPasswordChanged(const TQString &)));

	setMainWidget(unlockDialog);
}

PasswordDlg::~PasswordDlg()
{
	delete unlockDialog;
}

void PasswordDlg::setDevice(TQString deviceName)
{
	unlockDialog->descLabel->setText("<p><b>" + deviceName + "</b> is an encrypted storage device.</p>"
	        "<p>Please enter the password to unlock the storage device.</p>");
	unlockDialog->descLabel->adjustSize();
	unlockDialog->adjustSize();
}

void PasswordDlg::clearPassword()
{
	unlockDialog->passwordEdit->setText(TQString::null);
}

TQString PasswordDlg::getPassword()
{
	return unlockDialog->passwordEdit->text();
}

void PasswordDlg::slotPasswordChanged(const TQString &text)
{
	enableButton( User1, !text.isEmpty() );
}

#include "passworddlg.moc"
