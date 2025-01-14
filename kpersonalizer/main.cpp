/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Die Mai 22 17:24:18 CEST 2001
    copyright            : (C) 2001 by Ralf Nolden
    email                : nolden@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>
#include <tdelocale.h>
#include <dcopclient.h>
#include <tdeglobal.h>

#include "kpersonalizer.h"
#include "kfindlanguage.h"

static const char description[] = I18N_NOOP("KPersonalizer");

static TDECmdLineOptions options[] =
{
	{ "r", I18N_NOOP("Personalizer is restarted by itself"), 0 },
	{ "before-session", I18N_NOOP("Personalizer is running before Trinity session"), 0 },
        TDECmdLineLastOption
};

int main(int argc, char *argv[])
{
	TDEAboutData aboutData( "kpersonalizer", I18N_NOOP("KPersonalizer"),
		VERSION, description, TDEAboutData::License_GPL,
		"(c) 2001, Ralf Nolden", 0, 0, "nolden@kde.org");
	aboutData.addAuthor("Ralf Nolden",0, "nolden@kde.org");
	aboutData.addAuthor("Carsten Wolff",0, "wolff@kde.org");
	aboutData.addAuthor("qwertz",0, "kraftw@gmx.de");
	aboutData.addAuthor("Bernhard Rosenkraenzer", 0, "bero@redhat.com");
	TDECmdLineArgs::init( argc, argv, &aboutData );
	TDECmdLineArgs::addCmdLineOptions( options ); // Add our own options.

	TDELocale::setMainCatalogue("kpersonalizer");

	TDEApplication a;
	if ( !tdeApp->dcopClient()->isAttached() )
		tdeApp->dcopClient()->attach();

	TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

	if (args->isSet("before-session"))
		KPersonalizer::setBeforeSession();

	if (!args->isSet("r")) {	// we'll first show the first page
		KFindLanguage *flang = new KFindLanguage();
		if( !flang->getBestLang().isEmpty())	// if we have the users language, use it
			TDEGlobal::locale()->setLanguage( flang->getBestLang() );
		delete flang;
	}

	KPersonalizer *kpersonalizer = new KPersonalizer();
	// is personalizer restarted by itself?
	if (args->isSet("r"))
		kpersonalizer->restarted();
	a.setMainWidget(kpersonalizer);
	kpersonalizer->show();

	return a.exec();
}
