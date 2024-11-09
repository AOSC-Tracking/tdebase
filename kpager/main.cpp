/**************************************************************************

    main.cpp  - The main function for KPager
    Copyright (C) 1998-2000  Antonio Larrosa Jimenez

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    Send comments and bug fixes to larrosa@kde.org

***************************************************************************/

#include <tdeuniqueapplication.h>
#include <tdelocale.h>
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>
#include <tqcolor.h>
#include <kdebug.h>
#include <stdlib.h>
#include <tqsessionmanager.h>

#include "kpager.h"

static TDECmdLineOptions pagerOpts[] =
{
    { "hidden", I18N_NOOP("Create pager but keep the window hidden"), 0 },
    TDECmdLineLastOption
};

bool closed_by_sm = false;

class KPagerApplication : public TDEUniqueApplication
{
public:
  KPagerApplication() : TDEUniqueApplication() {}

  void commitData(TQSessionManager& sm) {
    if (mainWidget()->isHidden()) {
      sm.setRestartHint( TQSessionManager::RestartNever );
      return;
    }
    closed_by_sm = true;
    TDEUniqueApplication::commitData( sm );
    closed_by_sm = false;
  }

  int newInstance() {
    mainWidget()->show();
    return 0;
  }

};

int main(int argc, char **argv)
{
    TDEAboutData *aboutdata = new TDEAboutData("kpager", "KPager", "1.5",
					   I18N_NOOP("Desktop Overview"), TDEAboutData::License_GPL,
					   "(C) 1998-2002, Antonio Larrosa Jimenez","");

    aboutdata->addAuthor("Antonio Larrosa Jimenez",
			 I18N_NOOP("Original Developer/Maintainer"),"larrosa@kde.org",
			 "http://developer.kde.org/~larrosa/index.html");
    aboutdata->addAuthor("Matthias Elter",
			 I18N_NOOP("Developer"),"elter@kde.org", "");
    aboutdata->addAuthor("Matthias Ettrich",
			 I18N_NOOP("Developer"),"ettrich@kde.org", "");

    TDECmdLineArgs::init(argc, argv, aboutdata);
    TDECmdLineArgs::addCmdLineOptions(pagerOpts);
    TDEUniqueApplication::addCmdLineOptions();

    if (!TDEUniqueApplication::start())
    {
      kdError() << "kpager is already running!" << endl;
      return 0;
    }


    TDEApplication * app = new KPagerApplication;
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

    KPagerMainWindow *kpager = new KPagerMainWindow(0,"KPager");
    kpager->setPlainCaption( i18n("Desktop Pager") );


    app->setMainWidget(kpager);
    if (!args->isSet("hidden")) kpager->show();
    else kpager->hide();

    int ret = app->exec();

    delete app;
    return ret;
}

