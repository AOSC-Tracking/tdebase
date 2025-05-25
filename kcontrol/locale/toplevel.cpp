/*
  toplevel.cpp - A KControl Application

  Copyright 1998 Matthias Hoelzer
  Copyright 1999-2003 Hans Petter Bieker <bieker@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  */

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqobjectlist.h>
#include <tqpushbutton.h>
#include <tqtabwidget.h>
#include <tqvgroupbox.h>

#include <tdeaboutdata.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <tdemessagebox.h>
#include <tdeprocess.h>
#include <kservice.h>

#include "localenum.h"
#include "localemon.h"
#include "localetime.h"
#include "localeother.h"
#include "klocalesample.h"
#include "toplevel.h"
#include "kcmlocale.h"
#include "toplevel.moc"

TDELocaleApplication::TDELocaleApplication(TQWidget *parent, const char* /*name*/, 
                                       const TQStringList &args)
  : TDECModule( TDELocaleFactory::instance(), parent, args)
{
  TDEAboutData* aboutData = new TDEAboutData("kcmlocale",
        I18N_NOOP("KCMLocale"),
        "3.0",
        I18N_NOOP("Regional settings"),
        TDEAboutData::License_GPL,
        "(C) 1998 Matthias Hoelzer, "
        "(C) 1999-2003 Hans Petter Bieker",
        0, 0, "bieker@kde.org");
  setAboutData( aboutData );

  m_nullConfig = new TDEConfig(TQString::null, false, false);
  m_globalConfig = new TDEConfig(TQString::null, false, true);

  m_locale = new TDELocale(TQString::fromLatin1("kcmlocale"), m_nullConfig);
  TQVBoxLayout *l = new TQVBoxLayout(this, 0, KDialog::spacingHint());
  l->setAutoAdd(true);

  m_tab = new TQTabWidget(this);

  m_localemain = new TDELocaleConfig(m_locale, this);
  m_tab->addTab( m_localemain, TQString::null);
  m_localenum = new TDELocaleConfigNumber(m_locale, this);
  m_tab->addTab( m_localenum, TQString::null );
  m_localemon = new TDELocaleConfigMoney(m_locale, this);
  m_tab->addTab( m_localemon, TQString::null );
  m_localetime = new TDELocaleConfigTime(m_locale, this);
  m_tab->addTab( m_localetime, TQString::null );
  m_localeother = new TDELocaleConfigOther(m_locale, this);
  m_tab->addTab( m_localeother, TQString::null );

  // Examples
  m_gbox = new TQVGroupBox(this);
  m_sample = new TDELocaleSample(m_locale, m_gbox);

  // getting signals from childs
  connect(m_localemain, TQ_SIGNAL(localeChanged()),
          this, TQ_SIGNAL(localeChanged()));
  connect(m_localemain, TQ_SIGNAL(languageChanged()),
          this, TQ_SIGNAL(languageChanged()));

  // run the slots on the childs
  connect(this, TQ_SIGNAL(localeChanged()),
          m_localemain, TQ_SLOT(slotLocaleChanged()));
  connect(this, TQ_SIGNAL(localeChanged()),
          m_localenum, TQ_SLOT(slotLocaleChanged()));
  connect(this, TQ_SIGNAL(localeChanged()),
          m_localemon, TQ_SLOT(slotLocaleChanged()));
  connect(this, TQ_SIGNAL(localeChanged()),
          m_localetime, TQ_SLOT(slotLocaleChanged()));
  connect(this, TQ_SIGNAL(localeChanged()),
          m_localeother, TQ_SLOT(slotLocaleChanged()));

  // keep the example up to date
  // NOTE: this will make the sample be updated 6 times the first time
  // because combo boxes++ emits the change signal not only when the user changes
  // it, but also when it's changed by the program.
  connect(m_localenum, TQ_SIGNAL(localeChanged()),
          m_sample, TQ_SLOT(slotLocaleChanged()));
  connect(m_localemon, TQ_SIGNAL(localeChanged()),
          m_sample, TQ_SLOT(slotLocaleChanged()));
  connect(m_localetime, TQ_SIGNAL(localeChanged()),
          m_sample, TQ_SLOT(slotLocaleChanged()));
  // No examples for this yet
  //connect(m_localeother, TQ_SIGNAL(slotLocaleChanged()),
  //m_sample, TQ_SLOT(slotLocaleChanged()));
  connect(this, TQ_SIGNAL(localeChanged()),
          m_sample, TQ_SLOT(slotLocaleChanged()));

  // make sure we always have translated interface
  connect(this, TQ_SIGNAL(languageChanged()),
          this, TQ_SLOT(slotTranslate()));
  connect(this, TQ_SIGNAL(languageChanged()),
          m_localemain, TQ_SLOT(slotTranslate()));
  connect(this, TQ_SIGNAL(languageChanged()),
          m_localenum, TQ_SLOT(slotTranslate()));
  connect(this, TQ_SIGNAL(languageChanged()),
          m_localemon, TQ_SLOT(slotTranslate()));
  connect(this, TQ_SIGNAL(languageChanged()),
          m_localetime, TQ_SLOT(slotTranslate()));
  connect(this, TQ_SIGNAL(languageChanged()),
          m_localeother, TQ_SLOT(slotTranslate()));

  // mark it as changed when we change it.
  connect(m_localemain, TQ_SIGNAL(localeChanged()),
          TQ_SLOT(slotChanged()));
  connect(m_localenum, TQ_SIGNAL(localeChanged()),
          TQ_SLOT(slotChanged()));
  connect(m_localemon, TQ_SIGNAL(localeChanged()),
          TQ_SLOT(slotChanged()));
  connect(m_localetime, TQ_SIGNAL(localeChanged()),
          TQ_SLOT(slotChanged()));
  connect(m_localeother, TQ_SIGNAL(localeChanged()),
          TQ_SLOT(slotChanged()));

  load();
}

TDELocaleApplication::~TDELocaleApplication()
{
  delete m_locale;
  delete m_globalConfig;
  delete m_nullConfig;
}

void TDELocaleApplication::load()
{
	load( false );
}

void TDELocaleApplication::load( bool useDefaults )
{
	m_globalConfig->setReadDefaults( useDefaults );
	m_globalConfig->reparseConfiguration();
	*m_locale = TDELocale(TQString::fromLatin1("kcmlocale"), m_globalConfig);
	
	emit localeChanged();
	emit languageChanged();
	emit changed(useDefaults);
}

void TDELocaleApplication::save()
{
  // temperary use of our locale as the global locale
  TDELocale *lsave = TDEGlobal::_locale;
  TDEGlobal::_locale = m_locale;
  KMessageBox::information(this, m_locale->translate
                           ("Changed language settings apply only to "
                            "newly started applications.\nTo change the "
                            "language of all programs, you will have to "
                            "logout first."),
                           m_locale->translate("Applying Language Settings"),
                           TQString::fromLatin1("LanguageChangesApplyOnlyToNewlyStartedPrograms"));
  // restore the old global locale
  TDEGlobal::_locale = lsave;

  TDEConfig *config = TDEGlobal::config();
  TDEConfigGroupSaver saver(config, "Locale");

  // ##### this doesn't make sense
  bool langChanged = config->readEntry("Language")
    != m_locale->language();

  m_localemain->save();
  m_localenum->save();
  m_localemon->save();
  m_localetime->save();
  m_localeother->save();

  // rebuild the date base if language was changed
  if (langChanged)
  {
    KService::rebuildKSycoca(this);
  }

  emit changed(false);
}

TQString TDELocaleApplication::handbookSection() const
{
 	int index = m_tab->currentPageIndex();
 	if (index == 0)
		return "locale-locale";
	else if (index == 1)
		return "locale-numbers";
	else if (index == 2)
		return "locale-money";
	else if (index == 3)
		return "locale-datetime";
	else if (index == 4)
		return "locale-other";
 	else
 		return TQString::null;
}

void TDELocaleApplication::defaults()
{
	load( true );
}

TQString TDELocaleApplication::quickHelp() const
{
  return m_locale->translate("<h1>Country/Region & Language</h1>\n"
          "<p>From here you can configure language, numeric, and time \n"
          "settings for your particular region. In most cases it will be \n"
          "sufficient to choose the country you live in. For instance TDE \n"
          "will automatically choose \"German\" as language if you choose \n"
          "\"Germany\" from the list. It will also change the time format \n"
          "to use 24 hours and and use comma as decimal separator.</p>\n");
}

void TDELocaleApplication::slotTranslate()
{
  // The untranslated string for TQLabel are stored in
  // the name() so we use that when retranslating
  TQObject *wc;
  TQObjectList *list = queryList("TQWidget");
  TQObjectListIt it(*list);
  while ( (wc = it.current()) != 0 )
  {
    ++it;

    // unnamed labels will cause errors and should not be
    // retranslated. E.g. the example box should not be
    // retranslated from here.
    if (wc->name() == 0)
      continue;
    if (::qstrcmp(wc->name(), "") == 0)
      continue;
    if (::qstrcmp(wc->name(), "unnamed") == 0)
      continue;

    if (::qstrcmp(wc->className(), "TQLabel") == 0)
      ((TQLabel *)wc)->setText( m_locale->translate( wc->name() ) );
    else if (::qstrcmp(wc->className(), "TQGroupBox") == 0 ||
             ::qstrcmp(wc->className(), "TQVGroupBox") == 0)
      ((TQGroupBox *)wc)->setTitle( m_locale->translate( wc->name() ) );
    else if (::qstrcmp(wc->className(), "TQPushButton") == 0 ||
             ::qstrcmp(wc->className(), "KMenuButton") == 0)
      ((TQPushButton *)wc)->setText( m_locale->translate( wc->name() ) );
    else if (::qstrcmp(wc->className(), "TQCheckBox") == 0)
      ((TQCheckBox *)wc)->setText( m_locale->translate( wc->name() ) );
  }
  delete list;

  // Here we have the pointer
  m_gbox->setCaption(m_locale->translate("Examples"));
  m_tab->changeTab(m_localemain, m_locale->translate("&Locale"));
  m_tab->changeTab(m_localenum, m_locale->translate("&Numbers"));
  m_tab->changeTab(m_localemon, m_locale->translate("&Money"));
  m_tab->changeTab(m_localetime, m_locale->translate("&Time && Dates"));
  m_tab->changeTab(m_localeother, m_locale->translate("&Other"));

  // FIXME: All widgets are done now. However, there are
  // still some problems. Popup menus from the QLabels are
  // not retranslated.
}

void TDELocaleApplication::slotChanged()
{
  emit changed(true);
}

