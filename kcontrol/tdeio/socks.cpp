/**
 * socks.cpp
 *
 * Copyright (c) 2001 George Staikos <staikos@kde.org>
 * Copyright (c) 2001 Daniel Molkentin <molkentin@kde.org> (designer port)
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


#include <tqlayout.h>
#include <tqlabel.h>
#include <tqvbuttongroup.h>
#include <tqcheckbox.h>

#include <tdefiledialog.h>
#include <tdelistview.h>
#include <tdemessagebox.h>
#include <ksocks.h>
#include <tdeapplication.h>

#include "socks.h"
#include <tdeaboutdata.h>

KSocksConfig::KSocksConfig(TQWidget *parent)
  : TDECModule(parent, "kcmtdeio")
{

  TDEAboutData *about =
  new TDEAboutData(I18N_NOOP("kcmsocks"), I18N_NOOP("TDE SOCKS Control Module"),
                0, 0, TDEAboutData::License_GPL,
                I18N_NOOP("(c) 2001 George Staikos"));

  about->addAuthor("George Staikos", 0, "staikos@kde.org");

  setAboutData( about );


  TQVBoxLayout *layout = new TQVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
  base = new SocksBase(this);
  layout->add(base);

  connect(base->_c_enableSocks, TQ_SIGNAL(clicked()), this, TQ_SLOT(enableChanged()));
  connect(base->bg, TQ_SIGNAL(clicked(int)), this, TQ_SLOT(methodChanged(int)));

  // The custom library
  connect(base->_c_customPath, TQ_SIGNAL(openFileDialog(KURLRequester *)), this, TQ_SLOT(chooseCustomLib(KURLRequester *)));
  connect(base->_c_customPath, TQ_SIGNAL(textChanged(const TQString&)),
                     this, TQ_SLOT(customPathChanged(const TQString&)));

  // Additional libpaths
  connect(base->_c_newPath, TQ_SIGNAL(openFileDialog(KURLRequester *)), this, TQ_SLOT(chooseCustomLib(KURLRequester *)));
  connect(base->_c_newPath, TQ_SIGNAL(returnPressed(const TQString&)),
          this, TQ_SLOT(addThisLibrary(const TQString&)));
  connect(base->_c_newPath, TQ_SIGNAL(textChanged(const TQString&)),
          this, TQ_SLOT(libTextChanged(const TQString&)));
  connect(base->_c_add, TQ_SIGNAL(clicked()), this, TQ_SLOT(addLibrary()));
  connect(base->_c_remove, TQ_SIGNAL(clicked()), this, TQ_SLOT(removeLibrary()));
  connect(base->_c_libs, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(libSelection()));

  // The "Test" button
  connect(base->_c_test, TQ_SIGNAL(clicked()), this, TQ_SLOT(testClicked()));

  // The config backend
  load();
}

KSocksConfig::~KSocksConfig()
{
}

void KSocksConfig::configChanged()
{
    emit changed(true);
}

void KSocksConfig::enableChanged()
{
  KMessageBox::information(NULL,
                           i18n("These changes will only apply to newly "
                                "started applications."),
                           i18n("SOCKS Support"),
                           "SOCKSdontshowagain");
  emit changed(true);
}


void KSocksConfig::methodChanged(int id)
{
  if (id == 4) {
    base->_c_customLabel->setEnabled(true);
    base->_c_customPath->setEnabled(true);
  } else {
    base->_c_customLabel->setEnabled(false);
    base->_c_customPath->setEnabled(false);
  }
  emit changed(true);
}


void KSocksConfig::customPathChanged(const TQString&)
{
  emit changed(true);
}


void KSocksConfig::testClicked()
{
  save();   // we have to save before we can test!  Perhaps
            // it would be best to warn, though.

  if (KSocks::self()->hasSocks()) {
     KMessageBox::information(NULL,
                              i18n("Success: SOCKS was found and initialized."),
                              i18n("SOCKS Support"));
     // Eventually we could actually attempt to connect to a site here.
  } else {
      KMessageBox::information(NULL,
                               i18n("SOCKS could not be loaded."),
                               i18n("SOCKS Support"));
  }

  KSocks::self()->die();

}


void KSocksConfig::chooseCustomLib(KURLRequester * url)
{
  url->setMode( KFile::Directory );
/*  TQString newFile = KFileDialog::getOpenFileName();
  if (newFile.length() > 0) {
    base->_c_customPath->setURL(newFile);
    emit changed(true);
  }*/
}



void KSocksConfig::libTextChanged(const TQString& lib)
{
   if (lib.length() > 0)
     base-> _c_add->setEnabled(true);
   else base->_c_add->setEnabled(false);
}


void KSocksConfig::addThisLibrary(const TQString& lib)
{
   if (lib.length() > 0) {
      new TQListViewItem(base->_c_libs, lib);
      base->_c_newPath->clear();
      base->_c_add->setEnabled(false);
      base->_c_newPath->setFocus();
      emit changed(true);
   }
}


void KSocksConfig::addLibrary()
{
   addThisLibrary(base->_c_newPath->url());
}


void KSocksConfig::removeLibrary()
{
 TQListViewItem *thisitem = base->_c_libs->selectedItem();
   base->_c_libs->takeItem(thisitem);
   delete thisitem;
   base->_c_libs->clearSelection();
   base->_c_remove->setEnabled(false);
   emit changed(true);
}


void KSocksConfig::libSelection()
{
   base->_c_remove->setEnabled(true);
}


void KSocksConfig::load()
{
  TDEConfigGroup config(kapp->config(), "Socks");
  base->_c_enableSocks->setChecked(config.readBoolEntry("SOCKS_enable", false));
  int id = config.readNumEntry("SOCKS_method", 1);
  base->bg->setButton(id);
  if (id == 4) {
    base->_c_customLabel->setEnabled(true);
    base->_c_customPath->setEnabled(true);
  } else {
    base->_c_customLabel->setEnabled(false);
    base->_c_customPath->setEnabled(false);
  }
  base->_c_customPath->setURL(config.readPathEntry("SOCKS_lib"));

  TQListViewItem *thisitem;
  while ((thisitem = base->_c_libs->firstChild())) {
     base->_c_libs->takeItem(thisitem);
     delete thisitem;
  }

  TQStringList libs = config.readPathListEntry("SOCKS_lib_path");
  for(TQStringList::Iterator it = libs.begin();
                            it != libs.end();
                            ++it ) {
     new TQListViewItem(base->_c_libs, *it);
  }
  base->_c_libs->clearSelection();
  base->_c_remove->setEnabled(false);
  base->_c_add->setEnabled(false);
  base->_c_newPath->clear();
  emit changed(false);
}

void KSocksConfig::save()
{
  TDEConfigGroup config(kapp->config(), "Socks");
  config.writeEntry("SOCKS_enable",base-> _c_enableSocks->isChecked(), true, true);
  config.writeEntry("SOCKS_method", base->bg->id(base->bg->selected()), true, true);
  config.writePathEntry("SOCKS_lib", base->_c_customPath->url(), true, true);
  TQListViewItem *thisitem = base->_c_libs->firstChild();

  TQStringList libs;
  while (thisitem) {
    libs << thisitem->text(0);
    thisitem = thisitem->itemBelow();
  }
  config.writePathEntry("SOCKS_lib_path", libs, ',', true, true);

  kapp->config()->sync();

  emit changed(false);
}

void KSocksConfig::defaults()
{

  base->_c_enableSocks->setChecked(false);
  base->bg->setButton(1);
  base->_c_customLabel->setEnabled(false);
  base->_c_customPath->setEnabled(false);
  base->_c_customPath->setURL("");
  TQListViewItem *thisitem;
  while ((thisitem = base->_c_libs->firstChild())) {
     base->_c_libs->takeItem(thisitem);
     delete thisitem;
  }
  base->_c_newPath->clear();
  base->_c_add->setEnabled(false);
  base->_c_remove->setEnabled(false);
  emit changed(true);
}

TQString KSocksConfig::quickHelp() const
{
  return i18n("<h1>SOCKS</h1><p>This module allows you to configure TDE support"
     " for a SOCKS server or proxy.</p><p>SOCKS is a protocol to traverse firewalls"
     " as described in <a href=\"http://rfc.net/rfc1928.html\">RFC 1928</a>."
     " <p>If you have no idea what this is and if your system administrator does not"
     " tell you to use it, leave it disabled.</p>");
}


#include "socks.moc"

