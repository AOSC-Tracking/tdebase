/*
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <config.h>
#include <tqstring.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <tdemessagebox.h>

#include <tdesu/su.h>
#include "sudlg.h"

TDEsuDialog::TDEsuDialog(TQCString user, TQCString auth_user, bool enableKeep,const TQString& icon, bool withIgnoreButton, int timeout)
     : KPasswordDialog(Password, enableKeep, (withIgnoreButton ? User1:NoDefault), icon)
{
    TDEConfig* config = TDEGlobal::config();
    config->setGroup("super-user-command");
    TQString superUserCommand = config->readEntry("super-user-command", DEFAULT_SUPER_USER_COMMAND);
    if ( superUserCommand != "sudo" && superUserCommand != "su" ) {
	kdWarning() << "unknown super user command" << endl;
	superUserCommand = "su";
    }

    m_User = auth_user;
    setCaption(i18n("Run as %1").arg(static_cast<const char *>(user)));

    TQString prompt;
    if (superUserCommand == "sudo" && m_User == "root") {
	    prompt = i18n("Please enter your password." );
    } else {
	if (withIgnoreButton) {
	    if (m_User == "root") {
		prompt = i18n("The action you requested needs root privileges. "
		    "Please enter root's password below or click "
		    "Ignore to continue with your current privileges.");
	    }
	    else {
		prompt = i18n("The action you requested needs additional privileges. "
		    "Please enter the password for \"%1\" below or click "
		    "Ignore to continue with your current privileges.").arg(static_cast<const char *>(m_User));
	    }
	}
	else {
	    if (m_User == "root") {
		prompt = i18n("The action you requested needs root privileges. "
		    "Please enter root's password below.");
	    }
	    else {
		prompt = i18n("The action you requested needs additional privileges. "
		    "Please enter the password for \"%1\" below.").arg(static_cast<const char *>(m_User));
	    }
	}
    }
    setPrompt(prompt);
    setKeepWarning(i18n("<qt>The stored password will be:<br> * Kept for up to %1 minutes<br> * Destroyed on logout").arg(timeout/60));

    if( withIgnoreButton )
	setButtonText(User1, i18n("&Ignore"));
}


TDEsuDialog::~TDEsuDialog()
{
}

bool TDEsuDialog::checkPassword(const TQString &password)
{
    SuProcess proc;
    proc.setUser(m_User);
    int status = proc.checkInstall(password.local8Bit());
    switch (status)
    {
    case -1:
	KMessageBox::sorry(this, i18n("Conversation with su failed."));
	done(Rejected);
	return false;

    case 0:
	return true;

    case SuProcess::SuNotFound:
        KMessageBox::sorry(this,
		i18n("The program 'su' is not found;\n"
		     "make sure your PATH is set correctly."));
	done(Rejected);
	return false;

    case SuProcess::SuNotAllowed:
        KMessageBox::sorry(this,
		i18n("You are not allowed to use 'su';\n"
		     "on some systems, you need to be in a special "
		     "group (often: wheel) to use this program."));
	done(Rejected);
	return false;

    case SuProcess::SuIncorrectPassword:
        KMessageBox::sorry(this, i18n("Incorrect password; please try again."));
	return false;

    default:
        KMessageBox::error(this, i18n("Internal error: illegal return from "
		"SuProcess::checkInstall()"));
	done(Rejected);
	return false;
    }
}

void TDEsuDialog::slotUser1()
{
    done(AsUser);
}

#include "sudlg.moc"
