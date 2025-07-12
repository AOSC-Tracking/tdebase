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

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdepopupmenu.h>
#include <kinputdialog.h>
#include <kurlrequester.h>
#include <kgenericfactory.h>

#include <unistd.h>
#include <tdesimpleconfig.h>
#include <string>
#include <stdio.h>
#include <tqstring.h>

#include "iccconfig.h"

using namespace std;

/**** DLL Interface ****/
typedef KGenericFactory<KICCConfig, TQWidget> KICCCFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_iccconfig, KICCCFactory("kcmiccconfig") )

TDESimpleConfig *config;
TDESimpleConfig *systemconfig;

/**** KICCConfig ****/

KICCConfig::KICCConfig(TQWidget *parent, const char *name, const TQStringList &)
  : TDECModule(KICCCFactory::instance(), parent, name), iccFileArray(NULL)
{

  TQVBoxLayout *layout = new TQVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
  if (getuid() != 0) {
      config = new TDESimpleConfig( TQString::fromLatin1( "kiccconfigrc" ));
  }
  else {
      config = NULL;
  }
  systemconfig = new TDESimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/kicc/kiccconfigrc" ));

  TDEAboutData *about =
  new TDEAboutData(I18N_NOOP("kcmiccconfig"), I18N_NOOP("TDE Color Profile Control Module"),
                0, 0, TDEAboutData::License_GPL,
                I18N_NOOP("(c) 2009, 2010 Timothy Pearson\n(c) 2019 The Trinity Desktop Project"));

  about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
  setAboutData( about );

  base = new ICCConfigBase(this);
  layout->add(base);

  setRootOnlyMsg(i18n("<b>The global color profile is a system wide setting, and requires administrator access</b><br>To alter the system's global profile, click on the \"Administrator Mode\" button below."));
//   setUseRootOnlyMsg(true);	// Setting this hides the Apply button!

  connect(base->systemEnableSupport, TQ_SIGNAL(clicked()), TQ_SLOT(changed()));
  connect(base->systemEnableSupport, TQ_SIGNAL(toggled(bool)), base->systemIccFile, TQ_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQ_SIGNAL(clicked()), TQ_SLOT(changed()));
  connect(base->enableSupport, TQ_SIGNAL(toggled(bool)), base->iccFile, TQ_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQ_SIGNAL(toggled(bool)), base->randrScreenList, TQ_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQ_SIGNAL(toggled(bool)), base->iccProfileList, TQ_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQ_SIGNAL(toggled(bool)), base->addProfileButton, TQ_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQ_SIGNAL(toggled(bool)), base->renameProfileButton, TQ_SLOT(setEnabled(bool)));
  connect(base->enableSupport, TQ_SIGNAL(toggled(bool)), base->deleteProfileButton, TQ_SLOT(setEnabled(bool)));
  connect(base->iccProfileList, TQ_SIGNAL(activated(int)), this, TQ_SLOT(selectProfile(int)));
  connect(base->randrScreenList, TQ_SIGNAL(activated(int)), this, TQ_SLOT(selectScreen(int)));
  connect(base->iccFile, TQ_SIGNAL(textChanged(const TQString&)), TQ_SLOT(updateArray()));
  connect(base->systemIccFile, TQ_SIGNAL(textChanged(const TQString&)), TQ_SLOT(changed()));

  connect(base->addProfileButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(addProfile()));
  connect(base->renameProfileButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(renameProfile()));
  connect(base->deleteProfileButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(deleteProfile()));

  load();

  if (!config || !config->checkConfigFilesWritable( true )) {
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
    if (iccFileArray) {
        delete [] iccFileArray;
    }
    if (config) {
        delete config;
    }
    delete systemconfig;
}

void KICCConfig::deleteProfile () {
	int i;
	TQString *iccFileArrayNew;

	// Delete the profile
	if (config) config->deleteGroup(base->iccProfileList->currentText());
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
		_new = KInputDialog::getText( i18n("Color Profile Configuration"),  _error + _text, TQString::null, &_ok, this);
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
	if (!_ok) return;

	// Rename the profile
	if (config) config->deleteGroup(base->iccProfileList->currentText());
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
		_new = KInputDialog::getText( i18n("Color Profile Configuration"),  _error + _text, TQString::null, &_ok, this);
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
	if (!_ok) return;

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
	if (config) {
		config->setGroup(base->iccProfileList->currentText());
		if (config->readEntry(base->randrScreenList->currentText()) != iccFileArray[((base->iccProfileList->currentItem())*(base->randrScreenList->count()))+(base->randrScreenList->currentItem())]) {
			emit changed();
		}
	}
}

void KICCConfig::updateDisplayedInformation () {
	base->iccFile->setURL(iccFileArray[((base->iccProfileList->currentItem())*(base->randrScreenList->count()))+(base->randrScreenList->currentItem())]);
}

TQString KICCConfig::extractFileName(TQString displayName, TQString profileName) {
	return TQString();
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

  if (config) {
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
  }
  else {
      base->enableSupport->setChecked(false);
      base->randrScreenList->setEnabled(false);
      base->iccProfileList->setEnabled(false);
      base->iccFile->setEnabled(false);
      base->addProfileButton->setEnabled(false);
      base->renameProfileButton->setEnabled(false);
      base->deleteProfileButton->setEnabled(false);
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
	systemconfig->sync();

	if (config) {
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
	}

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
		KMessageBox::error(this, TQString("Unable to apply color profile configuration:\n%1").arg(errorstr));
	}

	emit changed(false);
}

void KICCConfig::defaults()
{
	load( true );
}

TQString KICCConfig::quickHelp() const
{
  return i18n("<h1>Color Profile Configuration</h1> This module allows you to configure TDE support"
     " for ICC color profiles. This allows you to easily color correct your monitor"
     " for a more lifelike and vibrant image.");
}

#include "iccconfig.moc"
