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

#include "dialog.h"

Dialog::Dialog(TQString url, TQString iconName) :
	KDialogBase(NULL, "Unlock", true, i18n("Unlock Storage Device"), (Cancel|User1), User1, false, KGuiItem(i18n("Unlock"), "unlocked" ))
{
	unlockDialog = new UnlockDialog(this);

	unlockDialog->errorBox->hide();
	unlockDialog->descLabel->setText(unlockDialog->descLabel->text().arg(url));
	unlockDialog->descLabel->adjustSize();
	unlockDialog->adjustSize();

	enableButton( User1, false );

	TQPixmap pixmap = TDEGlobal::iconLoader()->loadIcon(iconName, TDEIcon::NoGroup, TDEIcon::SizeLarge);
	unlockDialog->encryptedIcon->setPixmap( pixmap );

	connect(unlockDialog->passwordEdit, TQ_SIGNAL (textChanged(const TQString &)), this, TQ_SLOT (slotPasswordChanged(const TQString &)));

	setMainWidget(unlockDialog);
}

Dialog::~Dialog()
{
	delete unlockDialog;
}

TQString Dialog::getPassword()
{
	return unlockDialog->passwordEdit->text();
}

void Dialog::slotDialogError(TQString errorMsg)
{
	kdDebug() << __func__ << "(" << errorMsg << " )" << endl;

	unlockDialog->errorLabel->setText(TQString("<b>%1</b>").arg(errorMsg));
	unlockDialog->errorBox->show();
}

void Dialog::slotPasswordChanged(const TQString &text)
{
	enableButton( User1, !text.isEmpty() );
}

#include "dialog.moc"
