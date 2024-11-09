/*
    KTip, the KDE Tip Of the Day

	Copyright (c) 2000, Matthias Hoelzer-Kluepfel
	Copyright (c) 2002 Tobias Koenig <tokoe82@yahoo.de>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <dcopclient.h>

#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <ktip.h>
#include <tdeuniqueapplication.h>
#include <twin.h>
#include <stdlib.h>

static const char description[] = I18N_NOOP("Useful tips");

int main(int argc, char *argv[])
{
	TDEAboutData aboutData("ktip", I18N_NOOP("KTip"),
				"0.3", description, TDEAboutData::License_GPL,
				"(c) 1998-2002, KDE Developers");
	TDECmdLineArgs::init( argc, argv, &aboutData );
	TDEUniqueApplication::addCmdLineOptions();

	if (!TDEUniqueApplication::start())
		exit(-1);

	TDEUniqueApplication app;

	KTipDialog *tipDialog = new KTipDialog(new KTipDatabase(locate("data", TQString("tdewizard/tips"))));
	TQ_CHECK_PTR(tipDialog);
#ifdef TQ_WS_X11
	KWin::setState(tipDialog->winId(), NET::StaysOnTop);
#endif
	tipDialog->setCaption(i18n("Useful Tips"));
	app.dcopClient()->send("ksplash", "ksplash", "close()", TQByteArray()); // Close splash screen
	tipDialog->show();

	TQObject::connect(tqApp, TQ_SIGNAL(lastWindowClosed()), tqApp, TQ_SLOT(quit()));

	app.setMainWidget(tipDialog);

	return app.exec();
}
