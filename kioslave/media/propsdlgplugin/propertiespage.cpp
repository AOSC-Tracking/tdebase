/*
    Copyright (c) 2004 Jan Schaefer <j_schaef@informatik.uni-kl.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <tqcheckbox.h>
#include <tqtooltip.h>
#include <tqbuttongroup.h>
#include <tqlineedit.h>
#include <tqfileinfo.h>
#include <tqlabel.h>
#include <tqregexp.h>
#include <kpushbutton.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <tqcombobox.h>
#include <tqtimer.h>
#include <kdebug.h>
#include "propertiespage.h"
#include <dcopref.h>

// keep in sync with .ui and kded module
const char *short_names[] = {"lower", "win95", "winnt", "mixed", 0 };
const char *journales[] = {"data", "ordered", "writeback", 0 };

PropertiesPage::PropertiesPage(TQWidget* parent, const TQString &_id)
  : PropertiesPageGUI(parent), id(_id)
{
  kdDebug() << "props page " << id << endl;
  DCOPRef mediamanager("kded", "mediamanager");
  DCOPReply reply = mediamanager.call( "mountoptions", id);

  TQStringList list;

  if (reply.isValid())
    list = reply;

  if (list.size()) {
    kdDebug() << "list " << list << endl;

    for (TQStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
      {
	TQString key = (*it).left((*it).find('='));
	TQString value = (*it).mid((*it).find('=') + 1);
	kdDebug() << "key '" << key << "' value '" << value << "'\n";
	options[key] = value;
      }

    if (!options.contains("ro"))
      option_ro->hide();
    else
      option_ro->setChecked(options["ro"] == "true");
    connect( option_ro, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    if (!options.contains("quiet"))
      option_quiet->hide();
    else
      option_quiet->setChecked(options["quiet"] == "true");
    connect( option_quiet, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    if (!options.contains("sync"))
      option_sync->hide();
    else
      option_sync->setChecked(options["sync"] == "true");
    connect( option_sync, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    if (!options.contains("atime"))
      option_atime->hide();
    else
      option_atime->setChecked(options["atime"] == "true");
    connect( option_atime, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    if (!options.contains("flush"))
      option_flush->hide();
    else
      option_flush->setChecked(options["flush"] == "true");
    connect( option_flush, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    if (!options.contains("utf8"))
      option_utf8->hide();
    else
      option_utf8->setChecked(options["utf8"] == "true");
    connect( option_utf8, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    if (!options.contains("uid"))
      option_uid->hide();
    else
      option_uid->setChecked(options["uid"] == "true");
    connect( option_uid, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    if (!options.contains("shortname"))
      {
	option_shortname->hide();
	text_shortname->hide();
      }
    else
      {
	for (int index = 0; short_names[index]; ++index)
	  if (options["shortname"] == short_names[index])
	    {
	      option_shortname->setCurrentItem(index);
	      break;
	    }
	connect( option_shortname, TQT_SIGNAL( activated(int) ), TQT_SIGNAL( changed() ) );
      }

    if (!options.contains("journaling"))
      {
	text_journaling->hide();
	option_journaling->hide();
      }
    else
      {
	for (int index = 0; journales[index]; ++index)
	  if (options["journaling"] == journales[index])
	    {
	      option_journaling->setCurrentItem(index);
	      break;
	    }
	connect( option_journaling, TQT_SIGNAL( activated(int) ), TQT_SIGNAL( changed() ) );
      }

    label_filesystem->setText(i18n("Filesystem: %1").arg(options["filesystem"]));
    option_mountpoint->setText(options["mountpoint"]);
    connect( option_mountpoint, TQT_SIGNAL( textChanged( const TQString &) ), TQT_SIGNAL( changed() ) );
    option_automount->setChecked(options["automount"] == "true");
    connect( option_automount, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );

    bool has_groupbox_specific = true;
    if (!options.contains("journaling") &&
	!options.contains("shortname") &&
	!options.contains("uid") &&
	!options.contains("utf8") &&
	!options.contains("flush")) {
      groupbox_specific->hide();
      has_groupbox_specific = false;
    }

    // The order is important - we want groupboxes to hide automatically depending on use_defaults
    // but don't want to emit changed() until user actually changes something.
    connect( option_defaults, TQT_SIGNAL( toggled(bool) ), groupbox_generic, SLOT( setHidden(bool) ) );
    if (has_groupbox_specific)
      connect( option_defaults, TQT_SIGNAL( toggled(bool) ), groupbox_specific, SLOT( setHidden(bool) ) );
    option_defaults->setChecked(options["use_defaults"] == "true");
    connect( option_defaults, TQT_SIGNAL( stateChanged(int) ), TQT_SIGNAL( changed() ) );    

  } else {

    groupbox_generic->setEnabled(false);
    groupbox_specific->setEnabled(false);
    label_filesystem->hide();
  }
}

PropertiesPage::~PropertiesPage()
{
}

bool PropertiesPage::save()
{
  TQStringList result;

  if (options.contains("ro"))
    result << TQString("ro=%1").arg(option_ro->isChecked() ? "true" : "false");

  if (options.contains("quiet"))
    result << TQString("quiet=%1").arg(option_quiet->isChecked() ? "true" : "false");

  if (options.contains("sync"))
    result << TQString("sync=%1").arg(option_sync->isChecked() ? "true" : "false");

  if (options.contains("atime"))
    result << TQString("atime=%1").arg(option_atime->isChecked() ? "true" : "false");

  if (options.contains("flush"))
    result << TQString("flush=%1").arg(option_flush->isChecked() ? "true" : "false");

  if (options.contains("utf8"))
    result << TQString("utf8=%1").arg(option_utf8->isChecked() ? "true" : "false");

  if (options.contains("uid"))
    result << TQString("uid=%1").arg(option_uid->isChecked() ? "true" : "false");

  if (options.contains("shortname"))
    result << TQString("shortname=%1").arg(short_names[option_shortname->currentItem()]);

  if (options.contains("journaling"))
    result << TQString("journaling=%1").arg(journales[option_journaling->currentItem()]);

  TQString mp = option_mountpoint->text();
  if (!mp.startsWith("/media/"))
    {
      KMessageBox::sorry(this, i18n("Mountpoint has to be below /media"));
      return false;
    }
  result << TQString("mountpoint=%1").arg(mp);
  result << TQString("automount=%1").arg(option_automount->isChecked() ? "true" : "false");
  result << TQString("use_defaults=%1").arg(option_defaults->isChecked() ? "true" : "false");

  kdDebug() << result << endl;

  DCOPRef mediamanager("kded", "mediamanager");
  DCOPReply reply = mediamanager.call( "setMountoptions", id, result);

  if (reply.isValid())
    return (bool)reply;
  else {
    KMessageBox::sorry(this,
		       i18n("Saving the changes failed"));

    return false;
  }
}

#include "propertiespage.moc"
