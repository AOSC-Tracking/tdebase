/*
 *  applettab.cpp
 *
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
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

#include <tqlayout.h>
#include <tqgroupbox.h>
#include <tqvbuttongroup.h>
#include <tqwhatsthis.h>
#include <tqradiobutton.h>
#include <tqpushbutton.h>
#include <tqtoolbutton.h>
#include <tqvbox.h>
#include <tqfileinfo.h>

#include <tdeconfig.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <kdialog.h>
#include <kstandarddirs.h>
#include <tdelistview.h>
#include <kdebug.h>

#include "applettab_impl.h"
#include "applettab_impl.moc"

AppletTab::AppletTab( TQWidget *parent, const char* name )
  : AppletTabBase (parent, name)
{

  connect(level_group, TQ_SIGNAL(clicked(int)), TQ_SLOT(level_changed(int)));

  connect(lb_trusted, TQ_SIGNAL(selectionChanged(TQListViewItem*)),
          TQ_SLOT(trusted_selection_changed(TQListViewItem*)));

  connect(pb_add, TQ_SIGNAL(clicked()), TQ_SLOT(add_clicked()));
  connect(pb_remove, TQ_SIGNAL(clicked()), TQ_SLOT(remove_clicked()));

  connect(lb_available, TQ_SIGNAL(selectionChanged(TQListViewItem*)),
          TQ_SLOT(available_selection_changed(TQListViewItem*)));

  pb_add->setEnabled(false);
  pb_remove->setEnabled(false);

  TQWhatsThis::add( level_group, i18n("Panel applets can be started in two different ways:"
    " internally or externally. While 'internally' is the preferred way to load applets, this can"
    " raise stability or security problems when you are using poorly-programmed third-party applets."
    " To address these problems, applets can be marked 'trusted'. You might want to configure"
    " Kicker to treat trusted applets differently to untrusted ones; your options are:"
    " <ul><li><em>Load only trusted applets internally:</em> All applets but the ones marked 'trusted'"
    " will be loaded using an external wrapper application.</li>"
    " <li><em>Load startup config applets internally:</em> The applets shown on TDE startup"
    " will be loaded internally, others will be loaded using an external wrapper application.</li>"
    " <li><em>Load all applets internally</em></li></ul>") );

  TQWhatsThis::add( lb_trusted, i18n("Here you can see a list of applets that are marked"
    " 'trusted', i.e. will be loaded internally by Kicker in any case. To move an applet"
    " from the list of available applets to the trusted ones, or vice versa, select it and"
    " press the left or right buttons.") );

  TQWhatsThis::add( pb_add, i18n("Click here to add the selected applet from the list of available,"
    " untrusted applets to the list of trusted applets.") );

  TQWhatsThis::add( pb_remove, i18n("Click here to remove the selected applet from the list of trusted"
    " applets to the list of available, untrusted applets.") );

  TQWhatsThis::add( lb_available, i18n("Here you can see a list of available applets that you"
    " currently do not trust. This does not mean you cannot use those applets, but rather that"
    " the panel's policy using them depends on your applet security level. To move an applet"
    " from the list of available applets to the trusted ones or vice versa, select it and"
    " press the left or right buttons.") );

  load();
}

void AppletTab::load()
{
   load( false );
}

void AppletTab::load( bool useDefaults )
{
  TDEConfig c(KickerConfig::the()->configName(), false, false);
  c.setReadDefaults( useDefaults );
  c.setGroup("General");

  available.clear();
  l_available.clear();
  l_trusted.clear();

  int level = c.readNumEntry("SecurityLevel", 1);

  switch(level)
    {
    case 0:
    default:
      trusted_rb->setChecked(true);
      break;
    case 1:
      new_rb->setChecked(true);
      break;
    case 2:
      all_rb->setChecked(true);
      break;
    }

  list_group->setEnabled(trusted_rb->isChecked());

  TQStringList list = TDEGlobal::dirs()->findAllResources("applets", "*.desktop");
  for ( TQStringList::Iterator it = list.begin(); it != list.end(); ++it )
    {
      TQFileInfo fi(*it);
      available << fi.baseName();
    }

  if(c.hasKey("TrustedApplets"))
    {
      TQStringList list = c.readListEntry("TrustedApplets");
      for ( TQStringList::Iterator it = list.begin(); it != list.end(); ++it )
        {
          if(available.contains(*it))
            l_trusted << (*it);
        }
    }
  else
      l_trusted << "clockapplet" << "ksystemtrayapplet" << "krunapplet" << "quicklauncher"
                << "kminipagerapplet" << "ktaskbarapplet" << "eyesapplet" << "kmixapplet";

  for ( TQStringList::Iterator it = available.begin(); it != available.end(); ++it )
    {
      if(!l_trusted.contains(*it))
        l_available << (*it);
    }

  updateTrusted();
  updateAvailable();
  emit changed(  useDefaults );
}

void AppletTab::save()
{
    TDEConfig c(KickerConfig::the()->configName(), false, false);
  c.setGroup("General");

  int level = 0;
  if(new_rb->isChecked()) level = 1;
  else if (all_rb->isChecked()) level = 2;

  c.writeEntry("SecurityLevel", level);
  c.writeEntry("TrustedApplets", l_trusted);
  c.sync();
}

void AppletTab::defaults()
{
   load( true );
}

TQString AppletTab::quickHelp() const
{
  return TQString::null;
}

void AppletTab::level_changed(int)
{
  list_group->setEnabled(trusted_rb->isChecked());
  setChanged();
}

void AppletTab::updateTrusted()
{
  lb_trusted->clear();
  for ( TQStringList::Iterator it = l_trusted.begin(); it != l_trusted.end(); ++it )
    (void) new TQListViewItem(lb_trusted, (*it));
}

void AppletTab::updateAvailable()
{
  lb_available->clear();
  for ( TQStringList::Iterator it = l_available.begin(); it != l_available.end(); ++it )
    (void) new TQListViewItem(lb_available, (*it));
}

void AppletTab::trusted_selection_changed(TQListViewItem * item)
{
  pb_remove->setEnabled(item != 0);
  setChanged();
}

void AppletTab::available_selection_changed(TQListViewItem * item)
{
  pb_add->setEnabled(item != 0);
  setChanged();
}

void AppletTab::add_clicked()
{
  TQListViewItem *item = lb_available->selectedItem();
  if (!item) return;
  l_available.remove(item->text(0));
  l_trusted.append(item->text(0));

  updateTrusted();
  updateAvailable();
  updateAddRemoveButton();
}

void AppletTab::remove_clicked()
{
  TQListViewItem *item = lb_trusted->selectedItem();
  if (!item) return;
  l_trusted.remove(item->text(0));
  l_available.append(item->text(0));

  updateTrusted();
  updateAvailable();
  updateAddRemoveButton();
}


void AppletTab::updateAddRemoveButton()
{
    pb_remove->setEnabled(l_trusted.count ()>0);
    pb_add->setEnabled(l_available.count()>0);
}
