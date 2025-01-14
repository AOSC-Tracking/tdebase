/*
 *  kcmsmserver.cpp
 *  Copyright (c) 2000,2002 Oswald Buddenhagen <ossi@kde.org>
 *
 *  based on kcmtaskbar.cpp
 *  Copyright (c) 2000 Kurt Granroth <granroth@kde.org>
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
 */
#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqlayout.h>
#include <tqradiobutton.h>

#include <dcopclient.h>

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kgenericfactory.h>
#include <klineedit.h>

#include "kcmsmserver.h"
#include "smserverconfigimpl.h"

typedef KGenericFactory<SMServerConfig, TQWidget > SMSFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_smserver, SMSFactory("kcmsmserver") )

SMServerConfig::SMServerConfig( TQWidget *parent, const char* name, const TQStringList & )
  : TDECModule (SMSFactory::instance(), parent, name)
{
    setQuickHelp( i18n("<h1>Session Manager</h1>"
    " You can configure the session manager here."
    " This includes options such as whether or not the session exit (logout)"
    " should be confirmed, whether the session should be restored again when logging in"
    " and whether the computer should be automatically shut down after session"
    " exit by default."));

    TQVBoxLayout *topLayout = new TQVBoxLayout(this);
    dialog = new SMServerConfigImpl(this);
    connect(dialog, TQ_SIGNAL(changed()), TQ_SLOT(changed()));

    dialog->show();
    topLayout->add(dialog);
    load();

}

void SMServerConfig::load()
{
	load( false );
}

void SMServerConfig::load(bool useDefaults )
{
  TDEConfig *c = new TDEConfig("ksmserverrc", false, false);
  c->setReadDefaults( useDefaults );
  c->setGroup("General");
  dialog->confirmLogoutCheck->setChecked(c->readBoolEntry("confirmLogout", true));
  bool en = c->readBoolEntry("offerShutdown", true);
  dialog->offerShutdownCheck->setChecked(en);
  dialog->sdGroup->setEnabled(en);

  TQString s = c->readEntry( "loginMode" );
  if ( s == "default" )
      dialog->emptySessionRadio->setChecked(true);
  else if ( s == "restoreSavedSession" )
      dialog->savedSessionRadio->setChecked(true);
  else // "restorePreviousLogout"
      dialog->previousSessionRadio->setChecked(true);

  switch (c->readNumEntry("shutdownType", int(TDEApplication::ShutdownTypeNone))) {
  case int(TDEApplication::ShutdownTypeHalt):
    dialog->haltRadio->setChecked(true);
    break;
  case int(TDEApplication::ShutdownTypeReboot):
    dialog->rebootRadio->setChecked(true);
    break;
  default:
    dialog->logoutRadio->setChecked(true);
    break;
  }
  dialog->excludeLineedit->setText( c->readEntry("excludeApps"));

  c->setGroup("Logout");
  dialog->showLogoutStatusDialog->setChecked(c->readBoolEntry("showLogoutStatusDlg", true));
  dialog->showUbuntuStyleDialog->setChecked(c->readBoolEntry("doUbuntuLogout", false));
  dialog->showFadeAway->setChecked(c->readBoolEntry("doFadeaway", true));
  dialog->showFancyFadeAway->setChecked(c->readBoolEntry("doFancyLogout", true));
  dialog->showFancyFadeAway->setEnabled(dialog->confirmLogoutCheck->isChecked() && dialog->showFadeAway->isChecked()),

  delete c;

  emit changed(useDefaults);
}

void SMServerConfig::save()
{
  TDEConfig *c = new TDEConfig("ksmserverrc", false, false);
  c->setGroup("General");
  c->writeEntry( "confirmLogout", dialog->confirmLogoutCheck->isChecked());
  c->writeEntry( "offerShutdown", dialog->offerShutdownCheck->isChecked());

  TQString s = "restorePreviousLogout";
  if ( dialog->emptySessionRadio->isChecked() )
      s = "default";
  else if ( dialog->savedSessionRadio->isChecked() )
      s = "restoreSavedSession";
  c->writeEntry( "loginMode", s );

  c->writeEntry( "shutdownType",
                 dialog->haltRadio->isChecked() ?
                   int(TDEApplication::ShutdownTypeHalt) :
                   dialog->rebootRadio->isChecked() ?
                     int(TDEApplication::ShutdownTypeReboot) :
                     int(TDEApplication::ShutdownTypeNone));
  c->writeEntry("excludeApps", dialog->excludeLineedit->text());
  c->setGroup("Logout");
  c->writeEntry( "showLogoutStatusDlg", dialog->showLogoutStatusDialog->isChecked());
  c->writeEntry( "doUbuntuLogout", dialog->showUbuntuStyleDialog->isChecked());
  c->writeEntry( "doFadeaway", dialog->showFadeAway->isChecked());
  c->writeEntry( "doFancyLogout", dialog->showFancyFadeAway->isChecked());
  c->sync();
  delete c;

  // update the k menu if necessary
  TQByteArray data;
  tdeApp->dcopClient()->send( "kicker", "kicker", "configure()", data );
}

void SMServerConfig::defaults()
{
	load( true );
}

#include "kcmsmserver.moc"

