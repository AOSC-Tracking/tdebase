/**
 * iccconfig.cpp
 *
 * Copyright (c) 2009-2010 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqpushbutton.h>

#include <dcopclient.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kinputdialog.h>
#include <kurlrequester.h>
#include <kgenericfactory.h>

#include <unistd.h>
#include <ksimpleconfig.h>
#include <string>
#include <stdio.h>
#include <tqstring.h>

#include "iccconfig.h"

using namespace std;

/**** DLL Interface ****/
typedef KGenericFactory<KICCConfig, TQWidget> KICCCFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_iccconfig, KICCCFactory("kcmiccconfig") )

KSimpleConfig *config;
KSimpleConfig *systemconfig;

/**** KICCConfig ****/

KICCConfig::KICCConfig(TQWidget *parent, const char *name, const TQStringList &)
  : KCModule(KICCCFactory::instance(), parent, name)
{

  TQVBoxLayout *layout = new TQVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
  config = new KSimpleConfig( TQString::fromLatin1( "kiccconfigrc" ));
  systemconfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/kicc/kiccconfigrc" ));

  KAboutData *about =
  new KAboutData(I18N_NOOP("kcmiccconfig"), I18N_NOOP("KDE ICC Profile Control Module"),
                0, 0, KAboutData::License_GPL,
                I18N_NOOP("(c) 2009,2010 Timothy Pearson"));

  about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
  setAboutData( about );

  base = new ICCConfigBase(this);
  layout->add(base);

  setRootOnlyMsg(i18n("<b>The global ICC color profile is a system wide setting, and requires administrator access</b><br>To alter the system's global ICC profile, click on the \"Administrator Mode\" button below."));
  setUseRootOnlyMsg(true);

  connect(base->systemEnableSupport, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
  connect(base->systemEnableSupport, TQT_SIGNAL(toggled(bool)), base->systemIccFile, TQT_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
  connect(base->enableSupport, TQT_SIGNAL(toggled(bool)), base->iccFile, TQT_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQT_SIGNAL(toggled(bool)), base->randrScreenList, TQT_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQT_SIGNAL(toggled(bool)), base->iccProfileList, TQT_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQT_SIGNAL(toggled(bool)), base->addProfileButton, TQT_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQT_SIGNAL(toggled(bool)), base->renameProfileButton, TQT_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQT_SIGNAL(toggled(bool)), base->deleteProfileButton, TQT_SLOT(setEnabled(bool)));
  connect(base->iccProfileList, TQT_SIGNAL(activated(int)), this, TQT_SLOT(selectProfile(int)));
  connect(base->randrScreenList, TQT_SIGNAL(activated(int)), this, TQT_SLOT(selectScreen(int)));
  connect(base->iccFile, TQT_SIGNAL(textChanged(const TQString&)), TQT_SLOT(updateArray()));
  connect(base->systemIccFile, TQT_SIGNAL(textChanged(const TQString&)), TQT_SLOT(changed()));

  connect(base->addProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(addProfile()));
  connect(base->renameProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(renameProfile()));
  connect(base->deleteProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(deleteProfile()));

  load();

  if (!config->checkConfigFilesWritable( true )) {
    base->enableSupport->setEnabled(false);
    base->randrScreenList->setEnabled(false);
    base->iccProfileList->setEnabled(false);
    base->iccFile->setEnabled(false);
    base->addProfileButton->setEnabled(false);
    base->renameProfileButton->setEnabled(false);
    base->deleteProfileButton->setEnabled(false);
  }

  if (getuid() != 0 || !systemconfig->checkConfigFilesWritable( true )) {
    base->systemEnableSupport->setEnabled(false);
    base->systemIccFile->setEnabled(false);
  }
}

KICCConfig::~KICCConfig()
{
    delete [] iccFileArray;
    delete config;
    delete systemconfig;
}

void KICCConfig::deleteProfile () {
	int i;
	TQString *iccFileArrayNew;

	// Delete the profile
	config->deleteGroup(base->iccProfileList->currentText());
	base->iccProfileList->removeItem(base->iccProfileList->currentItem());
	base->iccProfileList->setCurrentItem(base->iccProfileList->count()-1);

	// Contract the profile memory
	numberOfProfiles--;
	iccFileArrayNew = new TQString[numberOfProfiles*numberOfScreens];
	for (i=0;i<(numberOfProfiles*numberOfScreens);i++) {
		iccFileArrayNew[i] = iccFileArray[i];
	}
	delete [] iccFileArray;
	iccFileArray = iccFileArrayNew;
}

void KICCConfig::renameProfile () {
	int i;
	TQString *iccFileArrayNew;

	// Pop up a text entry box asking for the name of the new profile
	bool _ok = false;
	bool _end = false;
	TQString _new;
	TQString _text = i18n("Please enter the new profile name below:");
	TQString _error;

	while (!_end) {
		_new = KInputDialog::getText( i18n("ICC Profile Configuration"),  _error + _text, TQString::null, &_ok, this);
		if (!_ok ) {
			_end = true;
		} else {
			_error = TQString();
			if (!_new.isEmpty()) {
				if (findProfileIndex(_new) != -1)
					_error = i18n("Error: A profile with that name already exists") + TQString("\n");
				else
					_end = true;
			}
		}
	}

	// Rename the profile
	config->deleteGroup(base->iccProfileList->currentText());
	base->iccProfileList->changeItem(_new, base->iccProfileList->currentItem());

	updateDisplayedInformation();
	emit changed();
}

void KICCConfig::addProfile () {
	int i;
	TQString *iccFileArrayNew;

	// Pop up a text entry box asking for the name of the new profile
	bool _ok = false;
	bool _end = false;
	TQString _new;
	TQString _text = i18n("Please enter the new profile name below:");
	TQString _error;

	while (!_end) {
		_new = KInputDialog::getText( i18n("ICC Profile Configuration"),  _error + _text, TQString::null, &_ok, this);
		if (!_ok ) {
			_end = true;
		} else {
			_error = TQString();
			if (!_new.isEmpty()) {
				if (findProfileIndex(_new) != -1)
					_error = i18n("Error: A profile with that name already exists") + TQString("\n");
				else
					_end = true;
			}
		}
	}

	// Expand the profile memory
	numberOfProfiles++;
	iccFileArrayNew = new TQString[numberOfProfiles*numberOfScreens];
	for (i=0;i<((numberOfProfiles-1)*numberOfScreens);i++) {
		iccFileArrayNew[i] = iccFileArray[i];
	}
	delete [] iccFileArray;
	iccFileArray = iccFileArrayNew;
	for (;i<(numberOfProfiles*numberOfScreens);i++) {
		iccFileArray[i] = "";
	}

	// Insert the new profile name
	base->iccProfileList->insertItem(_new, -1);
	base->iccProfileList->setCurrentItem(base->iccProfileList->count()-1);

	updateDisplayedInformation();
	emit changed();
}

void KICCConfig::load()
{
	load( false );
}

void KICCConfig::selectProfile (int slotNumber) {
	updateDisplayedInformation();
	emit changed();
}

void KICCConfig::selectScreen (int slotNumber) {
	updateDisplayedInformation();
}

void KICCConfig::updateArray (void) {
	iccFileArray[((base->iccProfileList->currentItem())*(base->randrScreenList->count()))+(base->randrScreenList->currentItem())] = base->iccFile->url();
	config->setGroup(base->iccProfileList->currentText());
	if (config->readEntry(base->randrScreenList->currentText()) != iccFileArray[((base->iccProfileList->currentItem())*(base->randrScreenList->count()))+(base->randrScreenList->currentItem())]) {
		emit changed();
	}
}

void KICCConfig::updateDisplayedInformation () {
	base->iccFile->setURL(iccFileArray[((base->iccProfileList->currentItem())*(base->randrScreenList->count()))+(base->randrScreenList->currentItem())]);
}

TQString KICCConfig::extractFileName(TQString displayName, TQString profileName) {
	//
}

int KICCConfig::findProfileIndex(TQString profileName) {
	int i;
	for (i=0;i<numberOfProfiles;i++) {
		if (base->iccProfileList->text(i) == profileName) {
			return i;
		}
	}
	return -1;
}

int KICCConfig::findScreenIndex(TQString screenName) {
	int i;
	for (i=0;i<(base->randrScreenList->count());i++) {
		if (base->randrScreenList->text(i) == screenName) {
			return i;
		}
	}
	return -1;
}

void KICCConfig::load(bool useDefaults )
{
  //Update the toggle buttons with the current configuration
  int i;
  int j;

  // FIXME Should use font size (basically resultant string length) to set button widths...
  base->addProfileButton->setFixedWidth(110);
  base->renameProfileButton->setFixedWidth(90);
  base->deleteProfileButton->setFixedWidth(90);

  XRROutputInfo *output_info;
  KRandrSimpleAPI *randrsimple = new KRandrSimpleAPI();

  config->setReadDefaults( useDefaults );

  config->setGroup(NULL);
  base->enableSupport->setChecked(config->readBoolEntry("EnableICC", false));
  base->randrScreenList->setEnabled(config->readBoolEntry("EnableICC", false));
  base->iccProfileList->setEnabled(config->readBoolEntry("EnableICC", false));
  base->iccFile->setEnabled(config->readBoolEntry("EnableICC", false));
  base->addProfileButton->setEnabled(config->readBoolEntry("EnableICC", false));
  base->renameProfileButton->setEnabled(config->readBoolEntry("EnableICC", false));
  base->deleteProfileButton->setEnabled(config->readBoolEntry("EnableICC", false));

  numberOfScreens = 0;
  if (randrsimple->isValid() == true) {
      randr_display = XOpenDisplay(NULL);
      randr_screen_info = randrsimple->read_screen_info(randr_display);
      for (i = 0; i < randr_screen_info->n_output; i++) {
          output_info = randr_screen_info->outputs[i]->info;
          base->randrScreenList->insertItem(output_info->name, -1);
          numberOfScreens++;
      }
  }
  else {
      base->randrScreenList->insertItem("Default", -1);
      numberOfScreens++;
  }

  // Find all profile names
  numberOfProfiles = 0;
  cfgProfiles = config->groupList();
  for (TQStringList::Iterator i(cfgProfiles.begin()); i != cfgProfiles.end(); ++i) {
      base->iccProfileList->insertItem((*i), -1);
      numberOfProfiles++;
  }
  if (numberOfProfiles == 0) {
      base->iccProfileList->insertItem("<default>", -1);
      numberOfProfiles++;
  }

  // Load all profiles into memory
  iccFileArray = new TQString[numberOfProfiles*numberOfScreens];
  for (i=0;i<(base->iccProfileList->count());i++) {
      config->setGroup(base->iccProfileList->text(i));
      for (j=0;j<(base->randrScreenList->count());j++) {
          iccFileArray[(i*(base->randrScreenList->count()))+j] = config->readEntry(base->randrScreenList->text(j));
      }
  }

  if ((findProfileIndex(base->iccProfileList->currentText()) >= 0) && (findScreenIndex(base->randrScreenList->currentText()) >= 0)) {
      base->iccFile->setURL(iccFileArray[(findProfileIndex(base->iccProfileList->currentText())*base->randrScreenList->count())+findScreenIndex(base->randrScreenList->currentText())]);
  }
  else {
      base->iccFile->setURL("");
  }

  systemconfig->setGroup(NULL);
  base->systemEnableSupport->setChecked(systemconfig->readBoolEntry("EnableICC", false));
  base->systemIccFile->setEnabled(systemconfig->readBoolEntry("EnableICC", false));
  base->systemIccFile->setURL(systemconfig->readEntry("ICCFile"));

  delete randrsimple;

  emit changed(useDefaults);
}

void KICCConfig::save()
{
	int i;
	int j;
	KRandrSimpleAPI *randrsimple = new KRandrSimpleAPI();

	// Write system configuration
	systemconfig->setGroup(NULL);
	systemconfig->writeEntry("EnableICC", base->systemEnableSupport->isChecked());
	systemconfig->writeEntry("ICCFile", base->systemIccFile->url());

	// Write user configuration
	config->setGroup(NULL);
	config->writeEntry("DefaultProfile", m_defaultProfile);
	config->writeEntry("EnableICC", base->enableSupport->isChecked());

	// Save all profiles to disk
	for (i=0;i<(base->iccProfileList->count());i++) {
		config->setGroup(base->iccProfileList->text(i));
		for (j=0;j<(base->randrScreenList->count());j++) {
			config->writeEntry(base->randrScreenList->text(j), iccFileArray[(i*(base->randrScreenList->count()))+j]);
		}
	}

	config->sync();
	systemconfig->sync();

	TQString errorstr;
	if (base->enableSupport->isChecked() == true) {
		errorstr = randrsimple->applyIccConfiguration(base->iccProfileList->currentText(), KDE_CONFDIR);
	}
	else if (base->systemEnableSupport->isChecked() == true) {
		errorstr = randrsimple->applySystemWideIccConfiguration(KDE_CONFDIR);
	}
	else {
		errorstr = randrsimple->clearIccConfiguration();
	}
	if (errorstr != "") {
		KMessageBox::error(this, TQString("Unable to apply ICC configuration:\n\r%1").arg(errorstr));
	}

	emit changed(false);
}

void KICCConfig::defaults()
{
	load( true );
}

TQString KICCConfig::quickHelp() const
{
  return i18n("<h1>ICC Profile Configuration</h1> This module allows you to configure KDE support"
     " for ICC profiles. This allows you to easily color correct your monitor"
     " for a more lifelike and vibrant image.");
}

#include "iccconfig.moc"
