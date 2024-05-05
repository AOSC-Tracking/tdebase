/*
    Naughty applet - Runaway process monitor for the TDE panel

    Copyright 2000 Rik Hemsley (rikkus) <rik@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "NaughtyApplet.h"
#include "NaughtyProcessMonitor.h"
#include "NaughtyConfigDialog.h"

#include <tqmessagebox.h>
#include <tqtoolbutton.h>
#include <tqlayout.h>

#include <kiconloader.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <tdeaboutapplication.h>
#include <tdeaboutdata.h>
#include <tdelocale.h>
#include <tdepopupmenu.h>
#include <tdemessagebox.h>
#include <tqpushbutton.h>

extern "C"
{
  TDE_EXPORT KPanelApplet*  init(TQWidget * parent, const TQString & configFile)
  {
    TDEGlobal::locale()->insertCatalogue("naughtyapplet");

    return new NaughtyApplet
      (
       configFile,
       KPanelApplet::Normal,
       KPanelApplet::About | KPanelApplet::Preferences,
       parent,
       "naughtyapplet"
      );
  }
}

NaughtyApplet::NaughtyApplet
(
 const TQString & configFile,
 Type t,
 int actions,
 TQWidget * parent,
 const char * name
)
  : KPanelApplet(configFile, t, actions, parent, name)
{
  TDEGlobal::iconLoader()->addAppDir("naughtyapplet");
  setBackgroundOrigin( AncestorOrigin );

  button_ = new SimpleButton(this);

  TQVBoxLayout * layout = new TQVBoxLayout(this);
  layout->addWidget(button_);

  monitor_ = new NaughtyProcessMonitor(2, 20, this);

  connect
    (
     button_,   TQ_SIGNAL(clicked()),
     this,      TQ_SLOT(slotPreferences())
    );

  connect
    (
     monitor_,  TQ_SIGNAL(runawayProcess(ulong, const TQString &)),
     this,      TQ_SLOT(slotWarn(ulong, const TQString &))
    );

  connect
    (
     monitor_,  TQ_SIGNAL(load(uint)),
     this,      TQ_SLOT(slotLoad(uint))
    );

  loadSettings();

  monitor_->start();
}

NaughtyApplet::~NaughtyApplet()
{
    TDEGlobal::locale()->removeCatalogue("naughtyapplet");
}

  void
NaughtyApplet::slotWarn(ulong pid, const TQString & name)
{
  if (ignoreList_.contains(name))
    return;

  TQString s = i18n("A program called '%1' is slowing down the others "
                   "on your machine. It may have a bug that is causing "
                   "this, or it may just be busy.\n"
                   "Would you like to try to stop the program?");

  int retval = KMessageBox::warningYesNo(this, s.arg(name), TQString::null, i18n("Stop"), i18n("Keep Running"));

  if (KMessageBox::Yes == retval)
    monitor_->kill(pid);
  else
  {
    s = i18n("In future, should busy programs called '%1' be ignored?");

    retval = KMessageBox::questionYesNo(this, s.arg(name), TQString::null, i18n("Ignore"), i18n("Do Not Ignore"));

    if (KMessageBox::Yes == retval)
    {
      ignoreList_.append(name);
      config()->writeEntry("IgnoreList", ignoreList_);
      config()->sync();
    }
  }
}


  void
NaughtyApplet::slotLoad(uint l)
{
  button_->setPixmap(TDEGlobal::iconLoader()->loadIcon(
     (l > monitor_->triggerLevel() ? "naughty-sad" : "naughty-happy"),
     TDEIcon::Panel,
     TQMIN(size().width(),size().height())-2
  ));
}

  void
NaughtyApplet::about()
{
  TDEAboutData about
    (
     "naughtyapplet",
     I18N_NOOP("Naughty applet"),
     "1.0",
     I18N_NOOP("Runaway process catcher"),
     TDEAboutData::License_GPL_V2,
     "(C) 2000 Rik Hemsley (rikkus) <rik@kde.org>"
   );

  TDEAboutApplication a(&about, this);
  a.exec();
}

  void
NaughtyApplet::slotPreferences()
{
  preferences();
}

  void
NaughtyApplet::preferences()
{
  NaughtyConfigDialog d
    (
     ignoreList_,
     monitor_->interval(),
     monitor_->triggerLevel(),
     this
    );

  TQDialog::DialogCode retval = TQDialog::DialogCode(d.exec());

  if (TQDialog::Accepted == retval)
  {
    ignoreList_ = d.ignoreList();
    monitor_->setInterval(d.updateInterval());
    monitor_->setTriggerLevel(d.threshold());
    saveSettings();
  }
}

  void
NaughtyApplet::loadSettings()
{
  ignoreList_ = config()->readListEntry("IgnoreList");
  monitor_->setInterval(config()->readUnsignedNumEntry("UpdateInterval", 2));
  monitor_->setTriggerLevel(config()->readUnsignedNumEntry("Threshold", 20));

  // Add 'X' as a default.
  if (ignoreList_.isEmpty() && !config()->hasKey("IgnoreList"))
    ignoreList_.append("X");
}

  void
NaughtyApplet::saveSettings()
{
  config()->writeEntry("IgnoreList",      ignoreList_);
  config()->writeEntry("UpdateInterval",  monitor_->interval());
  config()->writeEntry("Threshold",       monitor_->triggerLevel());
  config()->sync();
}

#include "NaughtyApplet.moc"

