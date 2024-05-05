/*
 *
 * Copyright (c) 2001 Waldo Bastian <bastian@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <tqlayout.h>

#include <dcopclient.h>

#include <tdeapplication.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <kgenericfactory.h>
#include <tdeaboutdata.h>
#include <kdialog.h>

#include "mouse.h"
#include "windows.h"

#include "main.h"

extern "C"
{
	TDE_EXPORT TDECModule *create_twinfocus(TQWidget *parent, const char *name)
	{
		//CT there's need for decision: kwm or twin?
		TDEGlobal::locale()->insertCatalogue("kcmkwm");
		TDEConfig *c = new TDEConfig("twinrc", false, true);
		return new KFocusConfig(true, c, parent, name);
	}

	TDE_EXPORT TDECModule *create_twinactions(TQWidget *parent, const char *name)
	{
		//CT there's need for decision: kwm or twin?
		TDEGlobal::locale()->insertCatalogue("kcmkwm");
		return new TDEActionsOptions( parent, name);
	}

	TDE_EXPORT TDECModule *create_twinmoving(TQWidget *parent, const char *name)
	{
		//CT there's need for decision: kwm or twin?
		TDEGlobal::locale()->insertCatalogue("kcmkwm");
		TDEConfig *c = new TDEConfig("twinrc", false, true);
		return new KMovingConfig(true, c, parent, name);
	}

	TDE_EXPORT TDECModule *create_twinaborders(TQWidget *parent, const char *name)
	{
	   TDEGlobal::locale()->insertCatalogue("kcmkwm");
	   TDEConfig *c = new TDEConfig("twinrc", false, true);
	   return new KActiveBorderConfig(true, c, parent, name);
	}

	TDE_EXPORT TDECModule *create_twinadvanced(TQWidget *parent, const char *name)
	{
		//CT there's need for decision: kwm or twin?
		TDEGlobal::locale()->insertCatalogue("kcmkwm");
		TDEConfig *c = new TDEConfig("twinrc", false, true);
		return new KAdvancedConfig(true, c, parent, name);
	}

	TDE_EXPORT TDECModule *create_twintranslucency(TQWidget *parent, const char *name)
	{
		//CT there's need for decision: kwm or twin?
		TDEGlobal::locale()->insertCatalogue("kcmkwm");
		TDEConfig *c = new TDEConfig("twinrc", false, true);
		return new KTranslucencyConfig(true, c, parent, name);
	}

	TDE_EXPORT TDECModule *create_twinoptions ( TQWidget *parent, const char* name)
	{
		//CT there's need for decision: kwm or twin?
		TDEGlobal::locale()->insertCatalogue("kcmkwm");
		return new KWinOptions( parent, name);
	}
}

KWinOptions::KWinOptions(TQWidget *parent, const char *name)
  : TDECModule(parent, name)
{
  mConfig = new TDEConfig("twinrc", false, true);

  TQVBoxLayout *layout = new TQVBoxLayout(this);
  tab = new TQTabWidget(this);
  layout->addWidget(tab);

  mFocus = new KFocusConfig(false, mConfig, this, "TWin Focus Config");
  mFocus->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mFocus, i18n("&Focus"));
  connect(mFocus, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  mTitleBarActions = new KTitleBarActionsConfig(false, mConfig, this, "TWin TitleBar Actions");
  mTitleBarActions->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mTitleBarActions, i18n("&Titlebar Actions"));
  connect(mTitleBarActions, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  mWindowActions = new KWindowActionsConfig(false, mConfig, this, "TWin Window Actions");
  mWindowActions->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mWindowActions, i18n("Window Actio&ns"));
  connect(mWindowActions, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  mMoving = new KMovingConfig(false, mConfig, this, "TWin Moving");
  mMoving->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mMoving, i18n("&Moving"));
  connect(mMoving, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  mABorders = new KActiveBorderConfig(false, mConfig, this, "TWin Active Borders");
  mABorders->layout()->setMargin(KDialog::marginHint());
  tab->addTab(mABorders, i18n("Active &Borders"));
  connect(mABorders, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  mAdvanced = new KAdvancedConfig(false, mConfig, this, "TWin Advanced");
  mAdvanced->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mAdvanced, i18n("Ad&vanced"));
  connect(mAdvanced, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  mTranslucency = new KTranslucencyConfig(false, mConfig, this, "TWin Translucency");
  mTranslucency->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mTranslucency, i18n("&Translucency"));
  connect(mTranslucency, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  TDEAboutData *about =
    new TDEAboutData(I18N_NOOP("kcmtwinoptions"), I18N_NOOP("Window Behavior Configuration Module"),
                  0, 0, TDEAboutData::License_GPL,
                  I18N_NOOP("(c) 1997 - 2002 KWin and KControl Authors"));

  about->addAuthor("Matthias Ettrich",0,"ettrich@kde.org");
  about->addAuthor("Waldo Bastian",0,"bastian@kde.org");
  about->addAuthor("Cristian Tibirna",0,"tibirna@kde.org");
  about->addAuthor("Matthias Kalle Dalheimer",0,"kalle@kde.org");
  about->addAuthor("Daniel Molkentin",0,"molkentin@kde.org");
  about->addAuthor("Wynn Wilkes",0,"wynnw@caldera.com");
  about->addAuthor("Pat Dowler",0,"dowler@pt1B1106.FSH.UVic.CA");
  about->addAuthor("Bernd Wuebben",0,"wuebben@kde.org");
  about->addAuthor("Matthias Hoelzer-Kluepfel",0,"hoelzer@kde.org");
  setAboutData(about);
}

KWinOptions::~KWinOptions()
{
  delete mConfig;
}

void KWinOptions::load()
{
  mConfig->reparseConfiguration();
  mFocus->load();
  mTitleBarActions->load();
  mWindowActions->load();
  mMoving->load();
  mABorders->load();
  mAdvanced->load();
  mTranslucency->load();
  emit TDECModule::changed( false );
}


void KWinOptions::save()
{
  mFocus->save();
  mTitleBarActions->save();
  mWindowActions->save();
  mMoving->save();
  mABorders->save();
  mAdvanced->save();
  mTranslucency->save();

  emit TDECModule::changed( false );
  // Send signal to twin
  mConfig->sync();
  if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
  kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
}


void KWinOptions::defaults()
{
  mFocus->defaults();
  mTitleBarActions->defaults();
  mWindowActions->defaults();
  mMoving->defaults();
  mABorders->defaults();
  mAdvanced->defaults();
  mTranslucency->defaults();
}

TQString KWinOptions::quickHelp() const
{
  return i18n("<h1>Window Behavior</h1> Here you can customize the way windows behave when being"
    " moved, resized or clicked on. You can also specify a focus policy as well as a placement"
    " policy for new windows."
    " <p>Please note that this configuration will not take effect if you do not use"
    " TWin as your window manager. If you do use a different window manager, please refer to its documentation"
    " for how to customize window behavior.");
}

void KWinOptions::moduleChanged(bool state)
{
  emit TDECModule::changed(state);
}

TQString KWinOptions::handbookSection() const
{
    int index = tab->currentPageIndex();
    if (index == 0)
    {
        //return "focus";
        return TQString::null;
    }
    else if (index == 1)
    {
        return "titlebar-actions";
    }
    else if (index == 2)
    {
        return "window-actions";
    }
    else if (index == 3)
    {
        return "moving";
    }
    else if (index == 4)
    {
        return "active-borders";
    }
    else if (index == 5)
    {
        return "advanced";
    }
    else if (index == 6)
    {
        return "translucency";
    }
    else
    {
        return TQString::null;
    }
}


TDEActionsOptions::TDEActionsOptions(TQWidget *parent, const char *name)
  : TDECModule(parent, name)
{
  mConfig = new TDEConfig("twinrc", false, true);

  TQVBoxLayout *layout = new TQVBoxLayout(this);
  tab = new TQTabWidget(this);
  layout->addWidget(tab);

  mTitleBarActions = new KTitleBarActionsConfig(false, mConfig, this, "TWin TitleBar Actions");
  mTitleBarActions->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mTitleBarActions, i18n("&Titlebar Actions"));
  connect(mTitleBarActions, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));

  mWindowActions = new KWindowActionsConfig(false, mConfig, this, "TWin Window Actions");
  mWindowActions->layout()->setMargin( KDialog::marginHint() );
  tab->addTab(mWindowActions, i18n("Window Actio&ns"));
  connect(mWindowActions, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(moduleChanged(bool)));
}

TDEActionsOptions::~TDEActionsOptions()
{
  delete mConfig;
}

void TDEActionsOptions::load()
{
  mTitleBarActions->load();
  mWindowActions->load();
  emit TDECModule::changed( false );
}


void TDEActionsOptions::save()
{
  mTitleBarActions->save();
  mWindowActions->save();

  emit TDECModule::changed( false );
  // Send signal to twin
  mConfig->sync();
  if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
  kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
}


void TDEActionsOptions::defaults()
{
  mTitleBarActions->defaults();
  mWindowActions->defaults();
}

void TDEActionsOptions::moduleChanged(bool state)
{
  emit TDECModule::changed(state);
}

#include "main.moc"
