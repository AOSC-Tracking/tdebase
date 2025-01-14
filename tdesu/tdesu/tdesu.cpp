/*
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 1998 Pietro Iglio <iglio@fub.it>
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 */

#include <config.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <stdlib.h>

#include <sys/time.h>
#include <sys/resource.h>
#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif

#include <tqstring.h>
#include <tqfileinfo.h>
#include <tqglobal.h>
#include <tqfile.h>
#include <tqdir.h>

#include <dcopclient.h>

#include <kdebug.h>
#include <tdeglobal.h>
#include <tdeapplication.h>
#include <kstandarddirs.h>
#include <tdeconfig.h>
#include <tdelocale.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdemessagebox.h>
#include <krun.h>
#include <kuser.h>

#include <tdesu/defaults.h>
#include <tdesu/su.h>
#include <tdesu/client.h>

#include "sudlg.h"

#define ERR strerror(errno)

TQCString command;
const char *Version = "1.0";

// NOTE: if you change the position of the -u switch, be sure to adjust it
// at the beginning of main()
static TDECmdLineOptions options[] = {
    { "+command", I18N_NOOP("Specifies the command to run"), 0 },
    { "c <command>", I18N_NOOP("Specifies the command to run"), "" },
    { "f <file>", I18N_NOOP("Run command under target uid if <file> is not writable"), "" },
    { "u <user>", I18N_NOOP("Specifies the target uid"), "root" },
    { "n", I18N_NOOP("Do not keep password"), 0 },
    { "s", I18N_NOOP("Stop the daemon (forgets all passwords)"), 0 },
    { "t", I18N_NOOP("Enable terminal output (no password keeping)"), 0 },
    { "p <prio>", I18N_NOOP("Set priority value: 0 <= prio <= 100, 0 is lowest"), "50" },
    { "r", I18N_NOOP("Use realtime scheduling"), 0 },
    { "nonewdcop", I18N_NOOP("Let command use existing dcopserver"), 0 },
    { "comment <comment>", I18N_NOOP("Ignored"), "" },
    { "ignorebutton", I18N_NOOP("Display the ignore button"), 0 },
    { "i <icon name>", I18N_NOOP("Specify icon to use in the password dialog"), 0},
    { "d", I18N_NOOP("Do not show the command to be run in the dialog"), 0},
    TDECmdLineLastOption
};


TQCString dcopNetworkId()
{
    TQCString result;
    result.resize(1025);
    TQFile file(DCOPClient::dcopServerFile());
    if (!file.open(IO_ReadOnly))
        return "";
    int i = file.readLine(result.data(), 1024);
    if (i <= 0)
        return "";
    result.data()[i-1] = '\0'; // strip newline
    return result;
}

static int startApp();

int main(int argc, char *argv[])
{
    // FIXME: this can be considered a poor man's solution, as it's not
    // directly obvious to a gui user. :)
    // anyway, i vote against removing it even when we have a proper gui
    // implementation.  -- ossi
    const char *duser = ::getenv("ADMIN_ACCOUNT");
    if (duser && duser[0])
        options[3].def = duser;

    TDEAboutData aboutData("tdesu", I18N_NOOP("TDE su"),
            Version, I18N_NOOP("Runs a program with elevated privileges."),
            TDEAboutData::License_Artistic,
            "Copyright (c) 1998-2000 Geert Jansen, Pietro Iglio");
    aboutData.addAuthor("Geert Jansen", I18N_NOOP("Maintainer"),
            "jansen@kde.org", "http://www.stack.nl/~geertj/");
    aboutData.addAuthor("Pietro Iglio", I18N_NOOP("Original author"),
            "iglio@fub.it");

    TDECmdLineArgs::init(argc, argv, &aboutData);
    TDECmdLineArgs::addCmdLineOptions(options);
    TDEApplication::disableAutoDcopRegistration();
    // tdesu doesn't process SM events, so don't even connect to ksmserver
    TQCString session_manager = getenv( "SESSION_MANAGER" );
    unsetenv( "SESSION_MANAGER" );
    TDEApplication app;
    // but propagate it to the started app
    if (session_manager.data())
    {
        setenv( "SESSION_MANAGER", session_manager.data(), 1 );
    }
    
    {
        TDEStartupInfoId id;
        id.initId( tdeApp->startupId());
        id.setupStartupEnv(); // make DESKTOP_STARTUP_ID env. var. available again
    }

    int result = startApp();

    if (result == 127)
    {
        KMessageBox::sorry(0, i18n("Command '%1' not found.").arg(static_cast<const char *>(command)));
    }

    return result;
}

static int startApp()
{
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
    // Stop daemon and exit?
    if (args->isSet("s"))
    {
        TDEsuClient client;
        if (client.ping() == -1)
        {
            kdError(1206) << "Daemon not running -- nothing to stop\n";
            exit(1);
        }
        if (client.stopServer() != -1)
        {
            kdDebug(1206) << "Daemon stopped\n";
            exit(0);
        }
        kdError(1206) << "Could not stop daemon\n";
        exit(1);
    }

    TQString icon;
    if ( args->isSet("i"))
	icon = args->getOption("i");

    bool prompt = true;
    if ( args->isSet("d"))
	prompt = false;

    // Get target uid
    TQCString user = args->getOption("u");
    TQCString auth_user = user;
    struct passwd *pw = getpwnam(user);
    if (pw == 0L)
    {
        kdError(1206) << "User " << user << " does not exist\n";
        exit(1);
    }
    bool change_uid = (getuid() != pw->pw_uid);

    // If file is writeable, do not change uid
    TQString file = TQFile::decodeName(args->getOption("f"));
    if (change_uid && !file.isEmpty())
    {
        if (file.at(0) != '/')
        {
            TDEStandardDirs dirs;
            dirs.addKDEDefaults();
            file = dirs.findResource("config", file);
            if (file.isEmpty())
            {
                kdError(1206) << "Config file not found: " << file << "\n";
                exit(1);
            }
        }
        TQFileInfo fi(file);
        if (!fi.exists())
        {
            kdError(1206) << "File does not exist: " << file << "\n";
            exit(1);
        }
        change_uid = !fi.isWritable();
    }

    // Get priority/scheduler
    TQCString tmp = args->getOption("p");
    bool ok;
    int priority = tmp.toInt(&ok);
    if (!ok || (priority < 0) || (priority > 100))
    {
        TDECmdLineArgs::usage(i18n("Illegal priority: %1").arg(static_cast<const char *>(tmp)));
        exit(1);
    }
    int scheduler = SuProcess::SchedNormal;
    if (args->isSet("r"))
        scheduler = SuProcess::SchedRealtime;
    if ((priority > 50) || (scheduler != SuProcess::SchedNormal))
    {
        change_uid = true;
        auth_user = "root";
    }

    // Get command
    if (args->isSet("c")) {
        command = args->getOption("c");
    }
    else {
        if( args->count() ) {
            command = args->arg(0);
        }
    }
    if (command.stripWhiteSpace().isEmpty()) {
        TDECmdLineArgs::usage(i18n("No command specified."));
        exit(1);
    }
    for (int i= args->isSet("c") ? 0 : 1; i<args->count(); i++) {
        TQString arg = TQFile::decodeName(args->arg(i));
        KRun::shellQuote(arg);
        command += " ";
        command += TQFile::encodeName(arg);
    }

    // Don't change uid if we're don't need to.
    if (!change_uid)
    {
        int result = system(command);
        result = WEXITSTATUS(result);
        return result;
    }

    // Check for daemon and start if necessary
    bool just_started = false;
    bool have_daemon = true;
    TDEsuClient client;
    if (!client.isServerSGID())
    {
        kdWarning(1206) << "Daemon not safe (not sgid), not using it.\n";
        have_daemon = false;
    }
    else if (client.ping() == -1)
    {
        if (client.startServer() == -1)
        {
            kdWarning(1206) << "Could not start daemon, reduced functionality.\n";
            have_daemon = false;
        }
        just_started = true;
    }

    // Try to exec the command with tdesud.
    bool keep = !args->isSet("n") && have_daemon;
    bool terminal = args->isSet("t");
    bool new_dcop = args->isSet("newdcop");
    bool withIgnoreButton = args->isSet("ignorebutton");
    
    QCStringList env;
    TQCString options;
    env << ( "DESKTOP_STARTUP_ID=" + tdeApp->startupId());
    
    if (pw->pw_uid)
    {
       // Only propagate TDEHOME for non-root users,
       // root uses TDEROOTHOME
       
       // Translate the TDEHOME of this user to the new user.
       TQString kdeHome = TDEGlobal::dirs()->relativeLocation("home", TDEGlobal::dirs()->localtdedir());
       if (kdeHome[0] != '/')
          kdeHome.prepend("~/"); 
       else
          kdeHome=TQString::null; // Use default

       env << ("TDEHOME="+ TQFile::encodeName(kdeHome));
    }

    KUser u;
    env << (TQCString) ("TDESU_USER=" + u.loginName().local8Bit());
    
    if (!new_dcop)
    {
        TQCString tdesycoca = "TDESYCOCA="+TQFile::encodeName(locateLocal("cache", "tdesycoca"));
        env << tdesycoca;

        options += "xf"; // X-only, dcop forwarding enabled.
    }

    if (keep && !terminal && !just_started)
    {
        client.setPriority(priority);
        client.setScheduler(scheduler);
        int result = client.exec(command, user, options, env);
        if (result == 0)
        {
           result = client.exitCode();
           return result;
        }
    }

    // Set core dump size to 0 because we will have
    // root's password in memory.
    struct rlimit rlim;
    rlim.rlim_cur = rlim.rlim_max = 0;
    if (setrlimit(RLIMIT_CORE, &rlim))
    {
        kdError(1206) << "rlimit(): " << ERR << "\n";
        exit(1);
    }

    // Read configuration
    TDEConfig *config = TDEGlobal::config();
    config->setGroup("Passwords");
    int timeout = config->readNumEntry("Timeout", defTimeout);

    // Check if we need a password
    SuProcess proc;
    proc.setUser(auth_user);
    int needpw = proc.checkNeedPassword();
    if (needpw < 0)
    {
        TQString err = i18n("Su returned with an error.\n");
        KMessageBox::error(0L, err);
        exit(1);
    }
    if (needpw == 0)
    {
        keep = 0;
        kdDebug() << "Don't need password!!\n";
    }

    // Start the dialog
    TQString password;
    if (needpw)
    {
        TDEStartupInfoId id;
        id.initId( tdeApp->startupId());
        TDEStartupInfoData data;
        data.setSilent( TDEStartupInfoData::Yes );
        TDEStartupInfo::sendChange( id, data );
        TDEsuDialog dlg(user, auth_user, keep && !terminal,icon, withIgnoreButton, timeout);
	if (prompt)
	    dlg.addLine(i18n("Command:"), command);
        if ((priority != 50) || (scheduler != SuProcess::SchedNormal))
        {
            TQString prio;
            if (scheduler == SuProcess::SchedRealtime)
                prio += i18n("realtime: ");
            prio += TQString("%1/100").arg(priority);
	    if (prompt)
		dlg.addLine(i18n("Priority:"), prio);
        }
        int ret = dlg.exec();
        if (ret == TDEsuDialog::Rejected)
        {
            TDEStartupInfo::sendFinish( id );
            exit(0);
        }
        if (ret == TDEsuDialog::AsUser)
            change_uid = false;
        password = dlg.password();
        keep = dlg.keep();
        TDEConfigGroup(config,"Passwords").writeEntry("Keep", keep);
        data.setSilent( TDEStartupInfoData::No );
        TDEStartupInfo::sendChange( id, data );
    }

    // Some events may need to be handled (like a button animation)
    tdeApp->processEvents();

    // Run command
    if (!change_uid)
    {
        int result = system(command);
        result = WEXITSTATUS(result);
        return result;
    }
    else if (keep && have_daemon)
    {
        client.setPass(password.local8Bit(), timeout);
        client.setPriority(priority);
        client.setScheduler(scheduler);
        int result = client.exec(command, user, options, env);
        if (result == 0)
        {
            result = client.exitCode();
            return result;
        }
    } else
    {
        SuProcess proc;
        proc.setTerminal(terminal);
        proc.setErase(true);
        proc.setUser(user);
        if (!new_dcop)
        {
            proc.setXOnly(true);
            proc.setDCOPForwarding(true);
        }
        proc.setEnvironment(env);
        proc.setPriority(priority);
        proc.setScheduler(scheduler);
        proc.setCommand(command);
        int result = proc.exec(password.local8Bit());
        return result;
    }
    return -1;
}

