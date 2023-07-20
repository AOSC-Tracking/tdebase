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

#ifndef DIALOG_H_
#define DIALOG_H_

#include <tdemessagebox.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kiconloader.h>

#include <tqlineedit.h>
#include <tqlabel.h>
#include <tqgroupbox.h>

#include "unlockdialog.h"

class Dialog : public KDialogBase
{

TQ_OBJECT	

public:
	Dialog(TQString url, TQString iconName);
	~Dialog();

	TQString getPassword();

public slots:
	void slotPasswordChanged(const TQString &text);

private:
	UnlockDialog *unlockDialog;
};

#endif // DIALOG_H_

