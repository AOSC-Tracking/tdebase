/**
 *  kcmhtmlsearch.cpp
 *
 *  Copyright (c) 2000 Matthias Hölzer-Klüpfel <hoelzer@kde.org>
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
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurllabel.h>
#include <kapplication.h>
#include <tqcheckbox.h>
#include <tqwhatsthis.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include "klangcombo.h"
#include <kurlrequester.h>
#include <klineedit.h>

#include "kcmhtmlsearch.moc"


KHTMLSearchConfig::KHTMLSearchConfig(TQWidget *parent, const char *name)
  : KCModule(parent, name), indexProc(0)
{
  TQVBoxLayout *vbox = new TQVBoxLayout(this, 5);


  TQGroupBox *gb = new TQGroupBox(i18n("ht://dig"), this);
  vbox->addWidget(gb);

  TQGridLayout *grid = new TQGridLayout(gb, 3,2, 6,6);

  grid->addRowSpacing(0, gb->fontMetrics().lineSpacing());

  TQLabel *l = new TQLabel(i18n("The fulltext search feature makes use of the "
                  "ht://dig HTML search engine. "
                  "You can get ht://dig at the"), gb);
  l->setAlignment(TQLabel::WordBreak);
  l->setMinimumSize(l->sizeHint());
  grid->addMultiCellWidget(l, 1, 1, 0, 1);
  TQWhatsThis::add( gb, i18n( "Information about where to get the ht://dig package." ) );

  KURLLabel *url = new KURLLabel(gb);
  url->setURL("http://www.htdig.org");
  url->setText(i18n("ht://dig home page"));
  url->setAlignment(TQLabel::AlignHCenter);
  grid->addMultiCellWidget(url, 2,2, 0, 1);
  connect(url, TQT_SIGNAL(leftClickedURL(const TQString&)),
      this, TQT_SLOT(urlClicked(const TQString&)));

  gb = new TQGroupBox(i18n("Program Locations"), this);

  vbox->addWidget(gb);
  grid = new TQGridLayout(gb, 4,2, 6,6);
  grid->addRowSpacing(0, gb->fontMetrics().lineSpacing());

  htdigBin = new KURLRequester(gb);
  l = new TQLabel(htdigBin, i18n("ht&dig"), gb);
  l->setBuddy( htdigBin );
  grid->addWidget(l, 1,0);
  grid->addWidget(htdigBin, 1,1);
  connect(htdigBin->lineEdit(), TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(configChanged()));
  TQString wtstr = i18n( "Enter the path to your htdig program here, e.g. /usr/local/bin/htdig" );
  TQWhatsThis::add( htdigBin, wtstr );
  TQWhatsThis::add( l, wtstr );

  htsearchBin = new KURLRequester(gb);
  l = new TQLabel(htsearchBin, i18n("ht&search"), gb);
  l->setBuddy( htsearchBin );
  grid->addWidget(l, 2,0);
  grid->addWidget(htsearchBin, 2,1);
  connect(htsearchBin->lineEdit(), TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(configChanged()));
  wtstr = i18n( "Enter the path to your htsearch program here, e.g. /usr/local/bin/htsearch" );
  TQWhatsThis::add( htsearchBin, wtstr );
  TQWhatsThis::add( l, wtstr );

  htmergeBin = new KURLRequester(gb);
  l = new TQLabel(htmergeBin, i18n("ht&merge"), gb);
  l->setBuddy( htmergeBin );
  grid->addWidget(l, 3,0);
  grid->addWidget(htmergeBin, 3,1);
  connect(htmergeBin->lineEdit(), TQT_SIGNAL(textChanged(const TQString&)), this, TQT_SLOT(configChanged()));
  wtstr = i18n( "Enter the path to your htmerge program here, e.g. /usr/local/bin/htmerge" );
  TQWhatsThis::add( htmergeBin, wtstr );
  TQWhatsThis::add( l, wtstr );

  TQHBoxLayout *hbox = new TQHBoxLayout(vbox);

  gb = new TQGroupBox(i18n("Scope"), this);
  hbox->addWidget(gb);
  TQWhatsThis::add( gb, i18n( "Here you can select which parts of the documentation should be included in the fulltext search index. Available options are the KDE Help pages, the installed man pages, and the installed info pages. You can select any number of these." ) );

  TQVBoxLayout *vvbox = new TQVBoxLayout(gb, 6,2);
  vvbox->addSpacing(gb->fontMetrics().lineSpacing());

  indexKDE = new TQCheckBox(i18n("&KDE help"), gb);
  vvbox->addWidget(indexKDE);
  connect(indexKDE, TQT_SIGNAL(clicked()), this, TQT_SLOT(configChanged()));

  indexMan = new TQCheckBox(i18n("&Man pages"), gb);
  vvbox->addWidget(indexMan);
  indexMan->setEnabled(false),
  connect(indexMan, TQT_SIGNAL(clicked()), this, TQT_SLOT(configChanged()));

  indexInfo = new TQCheckBox(i18n("&Info pages"), gb);
  vvbox->addWidget(indexInfo);
  indexInfo->setEnabled(false);
  connect(indexInfo, TQT_SIGNAL(clicked()), this, TQT_SLOT(configChanged()));

  gb = new TQGroupBox(i18n("Additional Search Paths"), this);
  hbox->addWidget(gb);
  TQWhatsThis::add( gb, i18n( "Here you can add additional paths to search for documentation. To add a path, click on the <em>Add...</em> button and select the folder from where additional documentation should be searched. You can remove folders by clicking on the <em>Delete</em> button." ) );

  grid = new TQGridLayout(gb, 4,3, 6,2);
  grid->addRowSpacing(0, gb->fontMetrics().lineSpacing());

  addButton = new TQPushButton(i18n("Add..."), gb);
  grid->addWidget(addButton, 1,0);

  delButton = new TQPushButton(i18n("Delete"), gb);
  grid->addWidget(delButton, 2,0);

  searchPaths = new KListBox(gb);
  grid->addMultiCellWidget(searchPaths, 1,3, 1,1);
  grid->setRowStretch(2,2);

  gb = new TQGroupBox(i18n("Language Settings"), this);
  vbox->addWidget(gb);
  TQWhatsThis::add(gb, i18n("Here you can select the language you want to create the index for."));
  language = new KLanguageCombo(gb);
  l = new TQLabel(language, i18n("&Language"), gb);
  vvbox = new TQVBoxLayout(gb, 6,2);
  vvbox->addSpacing(gb->fontMetrics().lineSpacing());
  hbox = new TQHBoxLayout(vvbox, 6);
  hbox->addWidget(l);
  hbox->addWidget(language,1);
  hbox->addStretch(1);

  loadLanguages();

  vbox->addStretch(1);

  runButton = new TQPushButton(i18n("Generate Index..."), this);
  TQWhatsThis::add( runButton, i18n( "Click this button to generate the index for the fulltext search." ) );
  runButton->setFixedSize(runButton->sizeHint());
  vbox->addWidget(runButton, AlignRight);
  connect(runButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(generateIndex()));

  connect(addButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(addClicked()));
  connect(delButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(delClicked()));
  connect(searchPaths, TQT_SIGNAL(highlighted(const TQString &)),
      this, TQT_SLOT(pathSelected(const TQString &)));

  checkButtons();

  load();
}


void KHTMLSearchConfig::loadLanguages()
{
  // clear the list
  language->clear();

  // add all languages to the list
  TQStringList langs = KGlobal::dirs()->findAllResources("locale",
							TQString::fromLatin1("*/entry.desktop"));
  langs.sort();

  for (TQStringList::ConstIterator it = langs.begin(); it != langs.end(); ++it)
    {
      KSimpleConfig entry(*it);
      entry.setGroup(TQString::fromLatin1("KCM Locale"));
      TQString name = entry.readEntry(TQString::fromLatin1("Name"), KGlobal::locale()->translate("without name"));

      TQString path = *it;
      int index = path.findRev('/');
      path = path.left(index);
      index = path.findRev('/');
      path = path.mid(index+1);
      language->insertLanguage(path, name);
    }
}


TQString KHTMLSearchConfig::quickHelp() const
{
    return i18n( "<h1>Help Index</h1> This configuration module lets you configure the ht://dig engine which can be used for fulltext search in the KDE documentation as well as other system documentation like man and info pages." );
}


void KHTMLSearchConfig::pathSelected(const TQString &)
{
  checkButtons();
}


void KHTMLSearchConfig::checkButtons()
{

  delButton->setEnabled(searchPaths->currentItem() >= 0);
}


void KHTMLSearchConfig::addClicked()
{
  TQString dir = KFileDialog::getExistingDirectory();

  if (!dir.isEmpty())
    {
      for (uint i=0; i<searchPaths->count(); ++i)
    if (searchPaths->text(i) == dir)
      return;
      searchPaths->insertItem(dir);
      configChanged();
    }
}


void KHTMLSearchConfig::delClicked()
{
  searchPaths->removeItem(searchPaths->currentItem());
  checkButtons();
  configChanged();
}


KHTMLSearchConfig::~KHTMLSearchConfig()
{
}


void KHTMLSearchConfig::configChanged()
{
  emit changed(true);
}


void KHTMLSearchConfig::load()
{
  KConfig *config = new KConfig("khelpcenterrc", true);

  config->setGroup("htdig");
  htdigBin->lineEdit()->setText(config->readPathEntry("htdig", kapp->dirs()->findExe("htdig")));
  htsearchBin->lineEdit()->setText(config->readPathEntry("htsearch", kapp->dirs()->findExe("htsearch")));
  htmergeBin->lineEdit()->setText(config->readPathEntry("htmerge", kapp->dirs()->findExe("htmerge")));

  config->setGroup("Scope");
  indexKDE->setChecked(config->readBoolEntry("KDE", true));
  indexMan->setChecked(config->readBoolEntry("Man", false));
  indexInfo->setChecked(config->readBoolEntry("Info", false));

  TQStringList l = config->readPathListEntry("Paths");
  searchPaths->clear();
  TQStringList::Iterator it;
  for (it=l.begin(); it != l.end(); ++it)
    searchPaths->insertItem(*it);

  config->setGroup("Locale");
  TQString lang = config->readEntry("Search Language", KGlobal::locale()->language());
  language->setCurrentItem(lang);

  emit changed(false);
}


void KHTMLSearchConfig::save()
{
  KConfig *config= new KConfig("khelpcenterrc", false);

  config->setGroup("htdig");
  config->writePathEntry("htdig", htdigBin->lineEdit()->text());
  config->writePathEntry("htsearch", htsearchBin->lineEdit()->text());
  config->writePathEntry("htmerge", htmergeBin->lineEdit()->text());

  config->setGroup("Scope");
  config->writeEntry("KDE", indexKDE->isChecked());
  config->writeEntry("Man", indexMan->isChecked());
  config->writeEntry("Info", indexInfo->isChecked());

  TQStringList l;
  for (uint i=0; i<searchPaths->count(); ++i)
    l.append(searchPaths->text(i));
  config->writePathEntry("Paths", l);

  config->setGroup("Locale");
  config->writeEntry("Search Language", language->currentTag());

  config->sync();
  delete config;

  emit changed(false);
}


void KHTMLSearchConfig::defaults()
{
  htdigBin->lineEdit()->setText(kapp->dirs()->findExe("htdig"));
  htsearchBin->lineEdit()->setText(kapp->dirs()->findExe("htsearch"));
  htmergeBin->lineEdit()->setText(kapp->dirs()->findExe("htmerge"));

  indexKDE->setChecked(true);
  indexMan->setChecked(false);
  indexInfo->setChecked(false);

  searchPaths->clear();

  language->setCurrentItem(KGlobal::locale()->language());

  emit changed(true);
}


void KHTMLSearchConfig::urlClicked(const TQString &url)
{
  kapp->invokeBrowser(url);
}


void KHTMLSearchConfig::generateIndex()
{
  save();

  TQString exe = kapp->dirs()->findExe("khtmlindex");
  if (exe.isEmpty())
    return;

  delete indexProc;

  indexProc = new KProcess;
  *indexProc << exe << "--lang" << language->currentTag();

  connect(indexProc, TQT_SIGNAL(processExited(KProcess *)),
      this, TQT_SLOT(indexTerminated(KProcess *)));

  runButton->setEnabled(false);

  indexProc->start();
}


void KHTMLSearchConfig::indexTerminated(KProcess *)
{
  runButton->setEnabled(true);
}


extern "C"
{
  KDE_EXPORT KCModule *create_htmlsearch(TQWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmhtmlsearch");
    return new KHTMLSearchConfig(parent, name);
  };
}
