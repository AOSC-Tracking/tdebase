/* This file is part of the KDE Display Manager Configuration package
    Copyright (C) 1997-1998 Thomas Tanghus (tanghus@earthling.net)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <unistd.h>
#include <sys/types.h>


#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>

#include <ksimpleconfig.h>
#include <karrowbutton.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdialog.h>
#include <kurlrequester.h>

#include "kdm-shut.h"
#include "kbackedcombobox.h"

extern KSimpleConfig *config;


KDMSessionsWidget::KDMSessionsWidget(TQWidget *parent, const char *name)
  : TQWidget(parent, name)
{
      TQString wtstr;

      TQGroupBox *group0 = new TQGroupBox( i18n("Allow Shutdown"), this );

      sdlcombo = new TQComboBox( FALSE, group0 );
      sdllabel = new TQLabel (sdlcombo, i18n ("&Local:"), group0);
      sdlcombo->insertItem(i18n("Everybody"), SdAll);
      sdlcombo->insertItem(i18n("Only Root"), SdRoot);
      sdlcombo->insertItem(i18n("Nobody"), SdNone);
      connect(sdlcombo, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));
      sdrcombo = new TQComboBox( FALSE, group0 );
      sdrlabel = new TQLabel (sdrcombo, i18n ("&Remote:"), group0);
      sdrcombo->insertItem(i18n("Everybody"), SdAll);
      sdrcombo->insertItem(i18n("Only Root"), SdRoot);
      sdrcombo->insertItem(i18n("Nobody"), SdNone);
      connect(sdrcombo, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));
      TQWhatsThis::add( group0, i18n("Here you can select who is allowed to shutdown"
        " the computer using KDM. You can specify different values for local (console) and remote displays. "
	"Possible values are:<ul>"
        " <li><em>Everybody:</em> everybody can shutdown the computer using KDM</li>"
        " <li><em>Only root:</em> KDM will only allow shutdown after the user has entered the root password</li>"
        " <li><em>Nobody:</em> nobody can shutdown the computer using KDM</li></ul>") );


      TQGroupBox *group1 = new TQGroupBox( i18n("Commands"), this );

      shutdown_lined = new KURLRequester(group1);
      TQLabel *shutdown_label = new TQLabel(shutdown_lined, i18n("H&alt:"), group1);
      connect(shutdown_lined, TQT_SIGNAL(textChanged(const TQString&)),
	      TQT_SLOT(changed()));
      wtstr = i18n("Command to initiate the system halt. Typical value: /sbin/halt");
      TQWhatsThis::add( shutdown_label, wtstr );
      TQWhatsThis::add( shutdown_lined, wtstr );

      restart_lined = new KURLRequester(group1);
      TQLabel *restart_label = new TQLabel(restart_lined, i18n("Reb&oot:"), group1);
      connect(restart_lined, TQT_SIGNAL(textChanged(const TQString&)),
	      TQT_SLOT(changed()));
      wtstr = i18n("Command to initiate the system reboot. Typical value: /sbin/reboot");
      TQWhatsThis::add( restart_label, wtstr );
      TQWhatsThis::add( restart_lined, wtstr );


      TQGroupBox *group4 = new TQGroupBox( i18n("Miscellaneous"), this );

      bm_combo = new KBackedComboBox( group4 );
      bm_combo->insertItem("None", i18n("boot manager", "None"));
      bm_combo->insertItem("Grub", i18n("Grub"));
#if defined(__linux__) && ( defined(__i386__) || defined(__amd64__) )
      bm_combo->insertItem("Lilo", i18n("Lilo"));
#endif
      TQLabel *bm_label = new TQLabel( bm_combo, i18n("Boot manager:"), group4 );
      connect(bm_combo, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));
      wtstr = i18n("Enable boot options in the \"Shutdown...\" dialog.");
      TQWhatsThis::add( bm_label, wtstr );
      TQWhatsThis::add( bm_combo, wtstr );

      tsbox = new TQCheckBox( i18n("Restart X-Server with session exit"), this );
      connect( tsbox, TQT_SIGNAL(toggled(bool)), TQT_SLOT(changed()) );
      wtstr = i18n("Whether the login manager should restart the local X-Server after a session exit instead of resetting. Use this when the X-Server leaks memory, crashes the system on reset attempts, or otherwise exhibits display issues.");
      TQWhatsThis::add( tsbox, wtstr );

      TQBoxLayout *main = new TQVBoxLayout( this, 10 );
      TQGridLayout *lgroup0 = new TQGridLayout( group0, 1, 1, 10);
      TQGridLayout *lgroup1 = new TQGridLayout( group1, 1, 1, 10);
      TQGridLayout *lgroup4 = new TQGridLayout( group4, 1, 1, 10);

      main->addWidget(group0);
      main->addWidget(group1);
      main->addWidget(group4);
      main->addWidget(tsbox);
      main->addStretch();

      lgroup0->addRowSpacing(0, group0->fontMetrics().height()/2);
      lgroup0->addColSpacing(2, KDialog::spacingHint() * 2);
      lgroup0->setColStretch(1, 1);
      lgroup0->setColStretch(4, 1);
      lgroup0->addWidget(sdllabel, 1, 0);
      lgroup0->addWidget(sdlcombo, 1, 1);
      lgroup0->addWidget(sdrlabel, 1, 3);
      lgroup0->addWidget(sdrcombo, 1, 4);

      lgroup1->addRowSpacing(0, group1->fontMetrics().height()/2);
      lgroup1->addColSpacing(2, KDialog::spacingHint() * 2);
      lgroup1->setColStretch(1, 1);
      lgroup1->setColStretch(4, 1);
      lgroup1->addWidget(shutdown_label, 1, 0);
      lgroup1->addWidget(shutdown_lined, 1, 1);
      lgroup1->addWidget(restart_label, 1, 3);
      lgroup1->addWidget(restart_lined, 1, 4);

      lgroup4->addRowSpacing(0, group4->fontMetrics().height()/2);
      lgroup4->addWidget(bm_label, 1, 0);
      lgroup4->addWidget(bm_combo, 1, 1);
      lgroup4->setColStretch(2, 1);

      main->activate();

}

void KDMSessionsWidget::makeReadOnly()
{
    sdlcombo->setEnabled(false);
    sdrcombo->setEnabled(false);

    restart_lined->lineEdit()->setReadOnly(true);
    restart_lined->button()->setEnabled(false);
    shutdown_lined->lineEdit()->setReadOnly(true);
    shutdown_lined->button()->setEnabled(false);

    bm_combo->setEnabled(false);
    tsbox->setEnabled(false);
}

void KDMSessionsWidget::writeSD(TQComboBox *combo)
{
    TQString what;
    switch (combo->currentItem()) {
    case SdAll: what = "All"; break;
    case SdRoot: what = "Root"; break;
    default: what = "None"; break;
    }
    config->writeEntry( "AllowShutdown", what);
}

void KDMSessionsWidget::save()
{
    config->setGroup("X-:*-Core");
    writeSD(sdlcombo);
    config->writeEntry( "TerminateServer", tsbox->isChecked() );

    config->setGroup("X-*-Core");
    writeSD(sdrcombo);

    config->setGroup("Shutdown");
    config->writeEntry("HaltCmd", shutdown_lined->url(), true);
    config->writeEntry("RebootCmd", restart_lined->url(), true);

    config->writeEntry("BootManager", bm_combo->currentId());
}

void KDMSessionsWidget::readSD(TQComboBox *combo, TQString def)
{
  TQString str = config->readEntry("AllowShutdown", def);
  SdModes sdMode;
  if(str == "All")
    sdMode = SdAll;
  else if(str == "Root")
    sdMode = SdRoot;
  else
    sdMode = SdNone;
  combo->setCurrentItem(sdMode);
}

void KDMSessionsWidget::load()
{
  config->setGroup("X-:*-Core");
  readSD(sdlcombo, "All");
  tsbox->setChecked(config->readBoolEntry("TerminateServer", false));


  config->setGroup("X-*-Core");
  readSD(sdrcombo, "Root");

  config->setGroup("Shutdown");
  restart_lined->setURL(config->readEntry("RebootCmd", "/sbin/reboot"));
  shutdown_lined->setURL(config->readEntry("HaltCmd", "/sbin/poweroff"));

  bm_combo->setCurrentId(config->readEntry("BootManager", "None"));
}



void KDMSessionsWidget::defaults()
{
  restart_lined->setURL("/sbin/reboot");
  shutdown_lined->setURL("/sbin/poweroff");

  sdlcombo->setCurrentItem(SdAll);
  sdrcombo->setCurrentItem(SdRoot);

  bm_combo->setCurrentId("None");
}


void KDMSessionsWidget::changed()
{
  emit changed(true);
}

#include "kdm-shut.moc"
