/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <config.h>

#include <kuniqueapplication.h>
#include <tdelocale.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdemessagebox.h>
#include <kuser.h>
#include <kdebug.h>

#include "passwd.h"
#include "passwddlg.h"

static TDECmdLineOptions options[] = 
{
    { "+[user]", I18N_NOOP("Change password of this user"), 0 },
    TDECmdLineLastOption
};


int main(int argc, char **argv)
{
    TDEAboutData aboutData("tdepasswd", I18N_NOOP("TDE passwd"),
            VERSION, I18N_NOOP("Changes a UNIX password."),
            TDEAboutData::License_Artistic, "Copyright (c) 2000 Geert Jansen");
    aboutData.addAuthor("Geert Jansen", I18N_NOOP("Maintainer"),
            "jansen@kde.org", "http://www.stack.nl/~geertj/");
 
    TDECmdLineArgs::init(argc, argv, &aboutData);
    TDECmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication::addCmdLineOptions();


    if (!KUniqueApplication::start()) {
	kdDebug() << "tdepasswd is already running" << endl;
	return 0;
    }

    KUniqueApplication app;

    KUser ku;
    TQString user;
    bool bRoot = ku.isSuperUser();
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

    if (args->count())
	user = TQString::fromLocal8Bit(args->arg(0));

    /* You must be able to run "tdepasswd loginName" */
    if ( !user.isEmpty() && user!=KUser().loginName() && !bRoot)
    {
        KMessageBox::sorry(0, i18n("You need to be root to change the password of other users."));
        return 0;
    }

    TQString oldpass;
    if (!bRoot)
    {
        int result = TDEpasswd1Dialog::getPassword(oldpass);
        if (result != TDEpasswd1Dialog::Accepted)
	    return 0;
    }

    TDEpasswd2Dialog *dlg = new TDEpasswd2Dialog(oldpass, user);


    dlg->exec();

    return 0;
}

