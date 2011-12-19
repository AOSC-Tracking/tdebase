/*
 * locale.cpp
 *
 * Copyright (c) 1998 Matthias Hoelzer (hoelzer@physik.uni-wuerzburg.de)
 * Copyright (c) 1999 Preston Brown <pbrown@kde.org>
 * Copyright (c) 1999-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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

#include <tqhbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqpushbutton.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>

#include <kdebug.h>
#include <kdialog.h>
#include <kprocess.h>
#include <kiconloader.h>
#include <klanguagebutton.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

#include "kcmlocale.h"
#include "kcmlocale.moc"
#include "toplevel.h"

KLocaleConfig::KLocaleConfig(KLocale *locale,
                             TQWidget *parent, const char *name)
  : TQWidget (parent, name),
    m_locale(locale)
{
  TQGridLayout *lay = new TQGridLayout(this, 4, 3,
                                     KDialog::marginHint(),
                                     KDialog::spacingHint());

  m_labCountry = new TQLabel(this, I18N_NOOP("Country or region:"));
  m_comboCountry = new KLanguageButton( this );
  m_labCountry->setBuddy(m_comboCountry);
  connect( m_comboCountry, TQT_SIGNAL(activated(const TQString &)),
           this, TQT_SLOT(changedCountry(const TQString &)) );

  m_labLang = new TQLabel(this, I18N_NOOP("Languages:"));
  m_labLang->setAlignment( AlignTop );

  m_languages = new TQListBox(this);
  connect(m_languages, TQT_SIGNAL(selectionChanged()),
          TQT_SLOT(slotCheckButtons()));

  TQWidget * vb = new TQWidget(this);
  TQVBoxLayout * boxlay = new TQVBoxLayout(vb, 0, KDialog::spacingHint());
  m_addLanguage = new KLanguageButton(TQString::null, vb, I18N_NOOP("Add Language"));
  boxlay->add(m_addLanguage);
  connect(m_addLanguage, TQT_SIGNAL(activated(const TQString &)),
          TQT_SLOT(slotAddLanguage(const TQString &)));
  m_removeLanguage = new TQPushButton(vb, I18N_NOOP("Remove Language"));
  m_upButton = new TQPushButton(vb, I18N_NOOP("Move Up"));
  m_downButton = new TQPushButton(vb, I18N_NOOP("Move Down"));
  boxlay->add(m_removeLanguage);
  boxlay->add(m_upButton);
  boxlay->add(m_downButton);
  connect(m_removeLanguage, TQT_SIGNAL(clicked()),
          TQT_SLOT(slotRemoveLanguage()));
  connect(m_upButton, TQT_SIGNAL(clicked()),
          TQT_SLOT(slotLanguageUp()));
  connect(m_downButton, TQT_SIGNAL(clicked()),
          TQT_SLOT(slotLanguageDown()));
  boxlay->insertStretch(-1);

  // #### HPB: This should be implemented for KDE 3
  //  new TQLabel(this, I18N_NOOP("Encoding:"));
  //TQComboBox * cb = new TQComboBox( this );
  //cb->insertStringList( KGlobal::charsets()->descriptiveEncodingNames() );

  lay->addMultiCellWidget(m_labCountry, 0, 0, 0, 1);
  lay->addWidget(m_comboCountry, 0, 2);
  lay->addWidget(m_labLang, 1, 0);
  lay->addWidget(m_languages, 1, 1);
  lay->addWidget(vb, 1, 2);

  lay->setRowStretch(2, 5);

  lay->setColStretch(1, 1);
  lay->setColStretch(2, 1);

#if 0
  // Added jriddell 2007-01-08, for Kubuntu Language Selector spec
  TQHBoxLayout* languageSelectorLayout = new TQHBoxLayout();
  installLanguage = new TQPushButton(i18n("Install New Language"), this);
  languageSelectorLayout->addWidget(installLanguage);
  uninstallLanguage = new TQPushButton(i18n("Uninstall Language"), this);
  languageSelectorLayout->addWidget(uninstallLanguage);
  selectLanguage = new TQPushButton(i18n("Select System Language"), this);
  languageSelectorLayout->addWidget(selectLanguage);
  languageSelectorLayout->addStretch();
  lay->addMultiCellLayout(languageSelectorLayout, 3, 3, 0, 2);

  connect( installLanguage, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotInstallLanguage()) );
  connect( uninstallLanguage, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotUninstallLanguage()) );
  connect( selectLanguage, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotSelectLanguage()) );
#endif

}

void KLocaleConfig::slotInstallLanguage()
{
  KProcess *proc = new KProcess;

  *proc << "kdesu";
  *proc << "qt-language-selector --mode install";
  TQApplication::connect(proc, TQT_SIGNAL(processExited(KProcess *)),
			this, TQT_SLOT(slotLanguageSelectorExited(KProcess *)));
  setEnabled(false);
  proc->start();
}

void KLocaleConfig::slotUninstallLanguage()
{
  KProcess *proc = new KProcess;

  *proc << "kdesu";
  *proc << "qt-language-selector --mode uninstall";
  TQApplication::connect(proc, TQT_SIGNAL(processExited(KProcess *)),
			this, TQT_SLOT(slotLanguageSelectorExited(KProcess *)));
  setEnabled(false);
  proc->start();
}

void KLocaleConfig::slotSelectLanguage()
{
  KProcess *proc = new KProcess;

  *proc << "kdesu";
  *proc << "qt-language-selector --mode select";
  TQApplication::connect(proc, TQT_SIGNAL(processExited(KProcess *)),
			this, TQT_SLOT(slotLanguageSelectorExited(KProcess *)));
  setEnabled(false);
  proc->start();
}

void KLocaleConfig::slotLanguageSelectorExited(KProcess *)
{
  //reload here
  loadLanguageList();
  setEnabled(true);
}

void KLocaleConfig::slotAddLanguage(const TQString & code)
{
  TQStringList languageList = m_locale->languageList();

  int pos = m_languages->currentItem();
  if ( pos < 0 )
    pos = 0;

  // If it's already in list, just move it (delete the old, then insert a new)
  int oldPos = languageList.findIndex( code );
  if ( oldPos != -1 )
    languageList.remove( languageList.at(oldPos) );

  if ( oldPos != -1 && oldPos < pos )
    --pos;

  TQStringList::Iterator it = languageList.at( pos );

  languageList.insert( it, code );

  m_locale->setLanguage( languageList );

  emit localeChanged();
  if ( pos == 0 )
    emit languageChanged();
}

void KLocaleConfig::slotRemoveLanguage()
{
  TQStringList languageList = m_locale->languageList();
  int pos = m_languages->currentItem();

  TQStringList::Iterator it = languageList.at( pos );

  if ( it != languageList.end() )
    {
      languageList.remove( it );

      m_locale->setLanguage( languageList );

      emit localeChanged();
      if ( pos == 0 )
        emit languageChanged();
    }
}

void KLocaleConfig::slotLanguageUp()
{
  TQStringList languageList = m_locale->languageList();
  int pos = m_languages->currentItem();

  TQStringList::Iterator it1 = languageList.at( pos - 1 );
  TQStringList::Iterator it2 = languageList.at( pos );

  if ( it1 != languageList.end() && it2 != languageList.end()  )
  {
    TQString str = *it1;
    *it1 = *it2;
    *it2 = str;

    m_locale->setLanguage( languageList );

    emit localeChanged();
    if ( pos == 1 ) // at the lang before the top
      emit languageChanged();
  }
}

void KLocaleConfig::slotLanguageDown()
{
  TQStringList languageList = m_locale->languageList();
  int pos = m_languages->currentItem();

  TQStringList::Iterator it1 = languageList.at( pos );
  TQStringList::Iterator it2 = languageList.at( pos + 1 );

  if ( it1 != languageList.end() && it2 != languageList.end()  )
    {
      TQString str = *it1;
      *it1 = *it2;
      *it2 = str;

      m_locale->setLanguage( languageList );

      emit localeChanged();
      if ( pos == 0 ) // at the top
        emit languageChanged();
    }
}

void KLocaleConfig::loadLanguageList()
{
  // temperary use of our locale as the global locale
  KLocale *lsave = KGlobal::_locale;
  KGlobal::_locale = m_locale;

  // clear the list
  m_addLanguage->clear();

  TQStringList first = languageList();

  TQStringList prilang;
  // add the primary languages for the country to the list
  for ( TQStringList::ConstIterator it = first.begin();
        it != first.end();
        ++it )
  {
    TQString str = locate("locale", TQString::fromLatin1("%1/entry.desktop")
                         .arg(*it));
    if (!str.isNull())
      prilang << str;
  }

  // add all languages to the list
  TQStringList alllang = KGlobal::dirs()->findAllResources("locale",
                               TQString::fromLatin1("*/entry.desktop"), 
                               false, true);
  TQStringList langlist = prilang;
  if (langlist.count() > 0)
    langlist << TQString::null; // separator
  langlist += alllang;

  int menu_index = -2;
  TQString submenu; // we are working on this menu
  for ( TQStringList::ConstIterator it = langlist.begin();
        it != langlist.end(); ++it )
  {
    if ((*it).isNull())
    {
      m_addLanguage->insertSeparator();
      submenu = TQString::fromLatin1("other");
      m_addLanguage->insertSubmenu(m_locale->translate("Other"),
                                   submenu, TQString::null, -1);
      menu_index = -2; // first entries should _not_ be sorted
      continue;
    }
    KSimpleConfig entry(*it);
    entry.setGroup("KCM Locale");
    TQString name = entry.readEntry("Name",
                                   m_locale->translate("without name"));

    TQString tag = *it;
    int index = tag.findRev('/');
    tag = tag.left(index);
    index = tag.findRev('/');
    tag = tag.mid(index + 1);
    m_addLanguage->insertItem(name, tag, submenu, menu_index);
  }

  // restore the old global locale
  KGlobal::_locale = lsave;
}

void KLocaleConfig::loadCountryList()
{
  // temperary use of our locale as the global locale
  KLocale *lsave = KGlobal::_locale;
  KGlobal::_locale = m_locale;

  TQString sub = TQString::fromLatin1("l10n/");

  // clear the list
  m_comboCountry->clear();

  TQStringList regionlist = KGlobal::dirs()->findAllResources("locale",
                                 sub + TQString::fromLatin1("*.desktop"), 
                                 false, true );

  for ( TQStringList::ConstIterator it = regionlist.begin();
    it != regionlist.end();
    ++it )
  {
    TQString tag = *it;
    int index;

    index = tag.findRev('/');
    if (index != -1)
      tag = tag.mid(index + 1);

    index = tag.findRev('.');
    if (index != -1)
      tag.truncate(index);

    KSimpleConfig entry(*it);
    entry.setGroup("KCM Locale");
    TQString name = entry.readEntry("Name",
                                   m_locale->translate("without name"));

    TQString map( locate( "locale",
                          TQString::fromLatin1( "l10n/%1.png" )
                          .arg(tag) ) );
    TQIconSet icon;
    if ( !map.isNull() )
      icon = KGlobal::iconLoader()->loadIconSet(map, KIcon::Small);
    m_comboCountry->insertSubmenu( icon, name, tag, sub, -2 );
  }

  // add all languages to the list
  TQStringList countrylist = KGlobal::dirs()->findAllResources
    ("locale", sub + TQString::fromLatin1("*/entry.desktop"), false, true);

  for ( TQStringList::ConstIterator it = countrylist.begin();
        it != countrylist.end(); ++it )
  {
    KSimpleConfig entry(*it);
    entry.setGroup("KCM Locale");
    TQString name = entry.readEntry("Name",
                                   m_locale->translate("without name"));
    TQString submenu = entry.readEntry("Region");

    TQString tag = *it;
    int index = tag.findRev('/');
    tag.truncate(index);
    index = tag.findRev('/');
    tag = tag.mid(index + 1);
    int menu_index = submenu.isEmpty() ? -1 : -2;

    TQString flag( locate( "locale",
                          TQString::fromLatin1( "l10n/%1/flag.png" )
                          .arg(tag) ) );
    TQIconSet icon( KGlobal::iconLoader()->loadIconSet(flag, KIcon::Small) );
    m_comboCountry->insertItem( icon, name, tag, submenu, menu_index );
  }

  // restore the old global locale
  KGlobal::_locale = lsave;
}

void KLocaleConfig::readLocale(const TQString &path, TQString &name,
                               const TQString &sub) const
{
  // temperary use of our locale as the global locale
  KLocale *lsave = KGlobal::_locale;
  KGlobal::_locale = m_locale;

  // read the name
  TQString filepath = TQString::fromLatin1("%1%2/entry.desktop")
    .arg(sub)
    .arg(path);

  KSimpleConfig entry(locate("locale", filepath));
  entry.setGroup("KCM Locale");
  name = entry.readEntry("Name");

  // restore the old global locale
  KGlobal::_locale = lsave;
}

void KLocaleConfig::save()
{
  KConfigBase *config = KGlobal::config();

  config->setGroup("Locale");

  config->writeEntry("Country", m_locale->country(), true, true);
  if ( m_locale->languageList().isEmpty() )
    config->writeEntry("Language", TQString::fromLatin1(""), true, true);
  else
    config->writeEntry("Language",
                       m_locale->languageList(), ':', true, true);

  config->sync();
}

void KLocaleConfig::slotCheckButtons()
{
  m_removeLanguage->setEnabled( m_languages->currentItem() != -1 );
  m_upButton->setEnabled( m_languages->currentItem() > 0 );
  m_downButton->setEnabled( m_languages->currentItem() != -1 &&
                            m_languages->currentItem() < (signed)(m_languages->count() - 1) );
}

void KLocaleConfig::slotLocaleChanged()
{
  loadLanguageList();
  loadCountryList();

  // update language widget
  m_languages->clear();
  TQStringList languageList = m_locale->languageList();
  for ( TQStringList::Iterator it = languageList.begin();
        it != languageList.end();
        ++it )
  {
    TQString name;
    readLocale(*it, name, TQString::null);

    m_languages->insertItem(name);
  }
  slotCheckButtons();

  m_comboCountry->setCurrentItem( m_locale->country() );
}

void KLocaleConfig::slotTranslate()
{
  kdDebug() << "slotTranslate()" << endl;

  TQToolTip::add(m_comboCountry, m_locale->translate
        ( "This is where you live. KDE will use the defaults for "
          "this country or region.") );
  TQToolTip::add(m_addLanguage, m_locale->translate
        ( "This will add a language to the list. If the language is already "
          "in the list, the old one will be moved instead." ) );

  TQToolTip::add(m_removeLanguage, m_locale->translate
        ( "This will remove the highlighted language from the list." ) );

  TQToolTip::add(m_languages, m_locale->translate
        ( "KDE programs will be displayed in the first available language in "
          "this list.\nIf none of the languages are available, US English "
          "will be used.") );

  TQString str;

  str = m_locale->translate
    ( "Here you can choose your country or region. The settings "
      "for languages, numbers etc. will automatically switch to the "
      "corresponding values." );
  TQWhatsThis::add( m_labCountry, str );
  TQWhatsThis::add( m_comboCountry, str );

  str = m_locale->translate
    ( "Here you can choose the languages that will be used by KDE. If the "
      "first language in the list is not available, the second will be used, "
      "etc. If only US English is available, no translations "
      "have been installed. You can get translation packages for many "
      "languages from the place you got KDE from.<p>"
      "Note that some applications may not be translated to your languages; "
      "in this case, they will automatically fall back to US English." );
  TQWhatsThis::add( m_labLang, str );
  TQWhatsThis::add( m_languages, str );
  TQWhatsThis::add( m_addLanguage, str );
  TQWhatsThis::add( m_removeLanguage, str );
}

TQStringList KLocaleConfig::languageList() const
{
  TQString fileName = locate("locale",
                            TQString::fromLatin1("l10n/%1/entry.desktop")
                            .arg(m_locale->country()));

  KSimpleConfig entry(fileName);
  entry.setGroup("KCM Locale");

  return entry.readListEntry("Languages");
}

void KLocaleConfig::changedCountry(const TQString & code)
{
  m_locale->setCountry(code);

  // change to the preferred languages in that country, installed only
  TQStringList languages = languageList();
  TQStringList newLanguageList;
  for ( TQStringList::Iterator it = languages.begin();
        it != languages.end();
        ++it )
  {
    TQString name;
    readLocale(*it, name, TQString::null);

    if (!name.isEmpty())
      newLanguageList += *it;
  }
  m_locale->setLanguage( newLanguageList );

  emit localeChanged();
  emit languageChanged();
}
