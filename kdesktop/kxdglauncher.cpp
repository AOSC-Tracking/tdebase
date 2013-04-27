/* This file is part of the KDE project
   Copyright (C) 2010 Timothy Pearson <kb9vqf@pearsoncomputing.net>

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

#include <tqfile.h>
#include <tqdir.h>
#include <tqtimer.h>
#include <tqstring.h>
#include <tqtextcodec.h>

#include <krun.h>
#include <klocale.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobalsettings.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kconfig.h>

#include <stdlib.h>

#include "kxdglauncher.h"

// helper function for reading xdg user dirs: it is required for obvious reasons
void readXdgUserDirs(TQString *desktop, TQString *documents)
{
	TQFile f( TQDir::homeDirPath() + "/.config/user-dirs.dirs" );

	if (!f.open(IO_ReadOnly))
		return;

	// set the codec for the current locale
	TQTextStream s(&f);
	s.setCodec( TQTextCodec::codecForLocale() );

	TQString line = s.readLine();
	while (!line.isNull())
	{
		if (line.startsWith("XDG_DESKTOP_DIR="))
			*desktop = TQString(line.remove("XDG_DESKTOP_DIR=").remove("\"")).replace("$HOME", TQDir::homeDirPath());
		else if (line.startsWith("XDG_DOCUMENTS_DIR="))
			*documents = TQString(line.remove("XDG_DOCUMENTS_DIR=").remove("\"")).replace("$HOME", TQDir::homeDirPath());

		line = s.readLine();
	}
}

TQString getDocumentPath()
{
	TQString s_desktopPath;
	TQString s_documentPath;

	readXdgUserDirs(&s_desktopPath, &s_documentPath);

	if (s_documentPath.isEmpty() == true) {
#ifdef Q_WS_WIN
	s_documentPath = getWin32ShellFoldersPath("Personal");
#else
	s_documentPath = TQDir::homeDirPath() + "/Documents/";
#endif
	}
	s_documentPath = TQDir::cleanDirPath( s_documentPath );
	if ( !s_documentPath.endsWith("/"))
	s_documentPath.append('/');

	return s_documentPath;
}

static KCmdLineOptions options[] =
{
	{ "xdgname <argument>", I18N_NOOP("XDG variable name to open"), 0 },
        { "getpath",		I18N_NOOP("Do not launch Konqueror; instead print path to directory if it exists)"), 0 },
	KCmdLineLastOption
};

int main( int argc, char **argv)
{
	KCmdLineArgs::init( argc, argv, "kxdglauncher", I18N_NOOP("KDE XDG File Browser Launcher and Prompter"), I18N_NOOP("Prompts if directory does not exist, otherwise launches"), "1.0" );
	KCmdLineArgs::addCmdLineOptions( options );
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	KApplication app;
	app.disableSessionManagement();

	if (args->isSet( "xdgname" ) == true) {
		TQString desiredFolder = args->getOption("xdgname");
		if (desiredFolder == "DOCUMENTS") {
			TQDir myqdir;
			if (myqdir.exists(getDocumentPath(), TRUE) == true) {
				if (args->isSet( "getpath" ) == true) {
					printf("%s\n", (const char *)getDocumentPath().local8Bit());
					return 0;
				}
				else {
					KRun * run = new KRun( KURL(getDocumentPath()), 0, false, false );
					TQObject::connect( run, TQT_SIGNAL( finished() ), &app, TQT_SLOT( quit() ));
					TQObject::connect( run, TQT_SIGNAL( error() ), &app, TQT_SLOT( quit() ));
					app.exec();
					return 0;
				}
			}
			else {
				TQString newDirectory = KInputDialog::text("Create Documents directory", "Please confirm your Documents directory location<br>Upon confimation a new directory will be created", getDocumentPath());
				if (newDirectory == TQString::null) {
					return 1;
				}
				else {
					if (newDirectory.length() < 4096) {
						bool directoryOk = false;
						if (myqdir.exists(newDirectory, TRUE) == false) {
							if (myqdir.mkdir(newDirectory, TRUE) == true) {
								directoryOk = TRUE;
							}
						}
						else {
							directoryOk = TRUE;
						}
						if (directoryOk == true) {
							TQString xdgModifiedDirectory = newDirectory;
							xdgModifiedDirectory = xdgModifiedDirectory.replace(TQDir::homeDirPath(), "$HOME");
							while (xdgModifiedDirectory.endsWith("/")) {
								xdgModifiedDirectory.truncate(xdgModifiedDirectory.length()-1);
							}
							KConfig config(TQDir::homeDirPath() + "/.config/user-dirs.dirs", false, false);
							config.writeEntry("XDG_DOCUMENTS_DIR", TQString("\"") + xdgModifiedDirectory + TQString("\""), true);
							config.sync();
							if (args->isSet( "getpath" ) == true) {
								printf("%s\n", (const char *)getDocumentPath().local8Bit());
								return 0;
							}
							else {
								KRun * run = new KRun( getDocumentPath(), 0, false, false );
								TQObject::connect( run, TQT_SIGNAL( finished() ), &app, TQT_SLOT( quit() ));
								TQObject::connect( run, TQT_SIGNAL( error() ), &app, TQT_SLOT( quit() ));
								app.exec();
							}
							return 0;
						}
						else {
							KMessageBox::error(0, i18n("Unable to create directory ") + TQString("\"") + newDirectory + TQString("\"\n") + i18n("Please check folder permissions and try again"), i18n("Unable to create directory"));
							return 1;
						}
					}
					else {
						KMessageBox::error(0, i18n("Unable to create the directory ") + newDirectory + TQString("\n") + i18n("Directory path cannot be longer than 4096 characters"), i18n("Unable to create directory"));
						return 1;
					}
				}
			}
		}
		else {
			printf("[kxdglauncher] XDG variable not recognized\n");
			return 1;
		}
	}
	else {
		printf("[kxdglauncher] Please specify the desired XDG variable name with --xdgname\n");
		return 1;
	}
}
