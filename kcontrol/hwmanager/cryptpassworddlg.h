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
#ifndef __cryptpassworddlg_h__
#define __cryptpassworddlg_h__

#include <kdialogbase.h>

#include "cryptpassworddlgbase.h"

class KSSLCertificate;

/**
 *
 * Dialog to enter LUKS passwords or password files
 *
 * @version 0.1
 * @author Timothy Pearson <kb9vqf@pearsoncomputing.net>
 */

class TDEUI_EXPORT CryptPasswordDialog : public KDialogBase
{
	TQ_OBJECT
public:
	/**
	* Create a dialog that allows a user to enter LUKS passwords or password files
	* @param parent     Parent widget
	*/
	CryptPasswordDialog(TQWidget *parent, TQString passwordPrompt, TQString caption=TQString::null, bool allow_card=false, KSSLCertificate* card_cert=NULL, bool* use_card=NULL);
	virtual ~CryptPasswordDialog();

	TQByteArray password();

protected:
	virtual void virtual_hook( int id, void* data );

private slots:
	void processLockouts();

private:
	CryptPasswordDialogBase* m_base;
	TQByteArray m_password;
	bool* m_useCard;

	class CryptPasswordDialogPrivate;
	CryptPasswordDialogPrivate* d;
};

#endif
