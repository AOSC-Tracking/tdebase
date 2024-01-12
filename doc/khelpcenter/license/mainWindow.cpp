/*
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


#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>

#include "TDELicenseDlg.h"

int main(int argc, char *argv[])
{
  TDELocale::setMainCatalogue("tdelicense");
  TDEAboutData aboutData("TDELicense", I18N_NOOP("TDE License"),
          "0.1", I18N_NOOP("TDE License"), TDEAboutData::License_GPL,
          "(c) 2023, TDE Developers");
  TDECmdLineArgs::init(argc,argv, &aboutData);
  TDEApplication::addCmdLineOptions();
  TDEApplication app;
  TQObject::connect(tqApp, TQ_SIGNAL(lastWindowClosed()), tqApp, TQ_SLOT(quit()));

  TDELicenseDlg *licenseDlg = new TDELicenseDlg();
  app.setMainWidget(licenseDlg);
  licenseDlg->show();

  return app.exec();
}
