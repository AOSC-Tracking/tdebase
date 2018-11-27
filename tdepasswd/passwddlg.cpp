/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <tdelocale.h>
#include <tdemessagebox.h>

#include "passwd.h"
#include "passwddlg.h"


TDEpasswd1Dialog::TDEpasswd1Dialog()
    : KPasswordDialog(Password, false, Help)
{
    setCaption(i18n("Change Password"));
    setPrompt(i18n("Please enter your current password:"));
}


TDEpasswd1Dialog::~TDEpasswd1Dialog()
{
}


bool TDEpasswd1Dialog::checkPassword(const TQString &password)
{
    PasswdProcess proc(0);

    int ret = proc.checkCurrent(password);
    switch (ret)
    {
    case -1:
    {
        TQString msg = TQString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Conversation with 'passwd' failed.") + msg;
	KMessageBox::error(this, msg);
	done(Rejected);
	return false;
    }

    case 0:
	return true;

    case PasswdProcess::PasswdNotFound:
	KMessageBox::error(this, i18n("Could not find the program 'passwd'."));
	done(Rejected);
	return false;

    case PasswdProcess::PasswordIncorrect:
        KMessageBox::sorry(this, i18n("Incorrect password. Please try again."));
	return false;

    default:
	KMessageBox::error(this, i18n("Internal error: illegal return value "
		"from PasswdProcess::checkCurrent."));
	done(Rejected);
	return false;
    }
}


// static
int TDEpasswd1Dialog::getPassword(TQString &password)
{
    TDEpasswd1Dialog *dlg = new TDEpasswd1Dialog();
    int res = dlg->exec();
    if (res == Accepted)
	password = dlg->password();
    delete dlg;
    return res;
}



TDEpasswd2Dialog::TDEpasswd2Dialog(const TQString &oldpass, const TQString &user)
    : KPasswordDialog(NewPassword, false, Help)
{
    m_Pass = oldpass;
    m_User = user;

    setCaption(i18n("Change Password"));
    if (m_User.isEmpty())
        setPrompt(i18n("Please enter your new password:"));
    else
        setPrompt(i18n("Please enter the new password for user <b>%1</b>:").arg(m_User.utf8().data()));
}


TDEpasswd2Dialog::~TDEpasswd2Dialog()
{
}


bool TDEpasswd2Dialog::checkPassword(const TQString &password)
{
    PasswdProcess proc(m_User.utf8());
    TQString edit_password = password;

    if (edit_password.length() > 8)
    {
	switch(KMessageBox::warningYesNoCancel(this,
		m_User.isEmpty() ?
		i18n("Your password is longer than 8 characters. On some "
			"systems, this can cause problems. You can truncate "
			"the password to 8 characters, or leave it as it is.") :
		i18n("The password is longer than 8 characters. On some "
			"systems, this can cause problems. You can truncate "
			"the password to 8 characters, or leave it as it is.")
			,
		i18n("Password Too Long"),
		i18n("Truncate"),
		i18n("Use as Is"),
		"truncatePassword"))
	{
	case KMessageBox::Yes :
		edit_password.truncate(8);
		break;
	case KMessageBox::No :
		break;
	default : return false;
	}
    }

    int ret = proc.exec(m_Pass.utf8(), edit_password.utf8());
    switch (ret)
    {
    case 0:
    {
        hide();
        TQString msg = TQString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Your password has been changed.") + msg;
        KMessageBox::information(0L, msg);
        return true;
    }

    case PasswdProcess::PasswordNotGood:
    {
        TQString msg = TQString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Your password has not been changed.") + msg;

        // The pw change did not succeed. Print the error.
        KMessageBox::sorry(this, msg);
        return false;
    }

    default:
        TQString msg = TQString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Conversation with 'passwd' failed.") + msg;
	KMessageBox::sorry(this, msg);
	done(Rejected);
	return true;
    }

    return true;
}


#include "passwddlg.moc"
