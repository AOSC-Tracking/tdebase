/*
 * localemon.cpp
 *
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

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqlineedit.h>
#include <tqobjectlist.h>
#include <tqwhatsthis.h>
#include <tqlayout.h>
#include <tqvgroupbox.h>
#include <tqvbox.h>
#include <tqregexp.h>

#include <knuminput.h>
#include <kdialog.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

#include "toplevel.h"
#include "localemon.h"
#include "localemon.moc"

KLocaleConfigMoney::KLocaleConfigMoney(KLocale *locale,
                                       TQWidget *parent, const char*name)
  : TQWidget(parent, name),
    m_locale(locale)
{
  // Money
  TQGridLayout *lay = new TQGridLayout(this, 6, 2,
                                     KDialog::marginHint(),
                                     KDialog::spacingHint());

  m_labMonCurSym = new TQLabel(this, I18N_NOOP("Currency symbol:"));
  lay->addWidget(m_labMonCurSym, 0, 0);
  m_edMonCurSym = new TQLineEdit(this);
  lay->addWidget(m_edMonCurSym, 0, 1);
  connect( m_edMonCurSym, TQT_SIGNAL( textChanged(const TQString &) ),
           TQT_SLOT( slotMonCurSymChanged(const TQString &) ) );

  m_labMonDecSym = new TQLabel(this, I18N_NOOP("Decimal symbol:"));
  lay->addWidget(m_labMonDecSym, 1, 0);
  m_edMonDecSym = new TQLineEdit(this);
  lay->addWidget(m_edMonDecSym, 1, 1);
  connect( m_edMonDecSym, TQT_SIGNAL( textChanged(const TQString &) ),
           TQT_SLOT( slotMonDecSymChanged(const TQString &) ) );

  m_labMonThoSep = new TQLabel(this, I18N_NOOP("Thousands separator:"));
  lay->addWidget(m_labMonThoSep, 2, 0);
  m_edMonThoSep = new TQLineEdit(this);
  lay->addWidget(m_edMonThoSep, 2, 1);
  connect( m_edMonThoSep, TQT_SIGNAL( textChanged(const TQString &) ),
           TQT_SLOT( slotMonThoSepChanged(const TQString &) ) );

  m_labMonFraDig = new TQLabel(this, I18N_NOOP("Fract digits:"));
  lay->addWidget(m_labMonFraDig, 3, 0);
  m_inMonFraDig = new KIntNumInput(this);
  m_inMonFraDig->setRange(0, 10, 1, false);
  lay->addWidget(m_inMonFraDig, 3, 1);

  connect( m_inMonFraDig, TQT_SIGNAL( valueChanged(int) ),
           TQT_SLOT( slotMonFraDigChanged(int) ) );

  TQWidget *vbox = new TQVBox(this);
  lay->addMultiCellWidget(vbox, 4, 4, 0, 1);
  TQVGroupBox *vgrp;
  vgrp = new TQVGroupBox( vbox, I18N_NOOP("Positive") );
  m_chMonPosPreCurSym = new TQCheckBox(vgrp, I18N_NOOP("Prefix currency symbol"));
  connect( m_chMonPosPreCurSym, TQT_SIGNAL( clicked() ),
           TQT_SLOT( slotMonPosPreCurSymChanged() ) );

  TQHBox *hbox;
  hbox = new TQHBox( vgrp );
  m_labMonPosMonSignPos = new TQLabel(hbox, I18N_NOOP("Sign position:"));
  m_cmbMonPosMonSignPos = new TQComboBox(hbox, "signpos");
  connect( m_cmbMonPosMonSignPos, TQT_SIGNAL( activated(int) ),
           TQT_SLOT( slotMonPosMonSignPosChanged(int) ) );

  vgrp = new TQVGroupBox( vbox, I18N_NOOP("Negative") );
  m_chMonNegPreCurSym = new TQCheckBox(vgrp, I18N_NOOP("Prefix currency symbol"));
  connect( m_chMonNegPreCurSym, TQT_SIGNAL( clicked() ),
           TQT_SLOT( slotMonNegPreCurSymChanged() ) );

  hbox = new TQHBox( vgrp );
  m_labMonNegMonSignPos = new TQLabel(hbox, I18N_NOOP("Sign position:"));
  m_cmbMonNegMonSignPos = new TQComboBox(hbox, "signpos");
  connect( m_cmbMonNegMonSignPos, TQT_SIGNAL( activated(int) ),
           TQT_SLOT( slotMonNegMonSignPosChanged(int) ) );

  // insert some items
  int i = 5;
  while (i--)
  {
    m_cmbMonPosMonSignPos->insertItem(TQString());
    m_cmbMonNegMonSignPos->insertItem(TQString());
  }

  lay->setColStretch(1, 1);
  lay->addRowSpacing(5, 0);

  adjustSize();
}

KLocaleConfigMoney::~KLocaleConfigMoney()
{
}

void KLocaleConfigMoney::save()
{
  KConfig *config = KGlobal::config();
  KConfigGroupSaver saver(config, "Locale");

  KSimpleConfig ent(locate("locale",
                           TQString::fromLatin1("l10n/%1/entry.desktop")
                           .arg(m_locale->country())), true);
  ent.setGroup("KCM Locale");

  TQString str;
  int i;
  bool b;

  str = ent.readEntry("CurrencySymbol", TQString::fromLatin1("$"));
  config->deleteEntry("CurrencySymbol", false, true);
  if (str != m_locale->currencySymbol())
    config->writeEntry("CurrencySymbol",
                       m_locale->currencySymbol(), true, true);

  str = ent.readEntry("MonetaryDecimalSymbol", TQString::fromLatin1("."));
  config->deleteEntry("MonetaryDecimalSymbol", false, true);
  if (str != m_locale->monetaryDecimalSymbol())
    config->writeEntry("MonetaryDecimalSymbol",
                       m_locale->monetaryDecimalSymbol(), true, true);

  str = ent.readEntry("MonetaryThousandsSeparator", TQString::fromLatin1(","));
  str.replace(TQString::fromLatin1("$0"), TQString());
  config->deleteEntry("MonetaryThousandsSeparator", false, true);
  if (str != m_locale->monetaryThousandsSeparator())
    config->writeEntry("MonetaryThousandsSeparator",
                       TQString::fromLatin1("$0%1$0")
                       .arg(m_locale->monetaryThousandsSeparator()),
                       true, true);

  i = ent.readNumEntry("FracDigits", 2);
  config->deleteEntry("FracDigits", false, true);
  if (i != m_locale->fracDigits())
    config->writeEntry("FracDigits", m_locale->fracDigits(), true, true);

  b = ent.readBoolEntry("PositivePrefixCurrencySymbol", true);
  config->deleteEntry("PositivePrefixCurrencySymbol", false, true);
  if (b != m_locale->positivePrefixCurrencySymbol())
    config->writeEntry("PositivePrefixCurrencySymbol",
                       m_locale->positivePrefixCurrencySymbol(), true, true);

  b = ent.readBoolEntry("NegativePrefixCurrencySymbol", true);
  config->deleteEntry("NegativePrefixCurrencySymbol", false, true);
  if (b != m_locale->negativePrefixCurrencySymbol())
    config->writeEntry("NegativePrefixCurrencySymbol",
                       m_locale->negativePrefixCurrencySymbol(), true, true);

  i = ent.readNumEntry("PositiveMonetarySignPosition",
                       (int)KLocale::BeforeQuantityMoney);
  config->deleteEntry("PositiveMonetarySignPosition", false, true);
  if (i != m_locale->positiveMonetarySignPosition())
    config->writeEntry("PositiveMonetarySignPosition",
                       (int)m_locale->positiveMonetarySignPosition(),
                       true, true);

  i = ent.readNumEntry("NegativeMonetarySignPosition",
                       (int)KLocale::ParensAround);
  config->deleteEntry("NegativeMonetarySignPosition", false, true);
  if (i != m_locale->negativeMonetarySignPosition())
    config->writeEntry("NegativeMonetarySignPosition",
                       (int)m_locale->negativeMonetarySignPosition(),
                       true, true);

  config->sync();
}

void KLocaleConfigMoney::slotLocaleChanged()
{
  m_edMonCurSym->setText( m_locale->currencySymbol() );
  m_edMonDecSym->setText( m_locale->monetaryDecimalSymbol() );
  m_edMonThoSep->setText( m_locale->monetaryThousandsSeparator() );
  m_inMonFraDig->setValue( m_locale->fracDigits() );

  m_chMonPosPreCurSym->setChecked( m_locale->positivePrefixCurrencySymbol() );
  m_chMonNegPreCurSym->setChecked( m_locale->negativePrefixCurrencySymbol() );
  m_cmbMonPosMonSignPos->setCurrentItem( m_locale->positiveMonetarySignPosition() );
  m_cmbMonNegMonSignPos->setCurrentItem( m_locale->negativeMonetarySignPosition() );
}

void KLocaleConfigMoney::slotMonCurSymChanged(const TQString &t)
{
  m_locale->setCurrencySymbol(t);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonDecSymChanged(const TQString &t)
{
  m_locale->setMonetaryDecimalSymbol(t);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonThoSepChanged(const TQString &t)
{
  m_locale->setMonetaryThousandsSeparator(t);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonFraDigChanged(int value)
{
  m_locale->setFracDigits(value);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonPosPreCurSymChanged()
{
  m_locale->setPositivePrefixCurrencySymbol(m_chMonPosPreCurSym->isChecked());
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonNegPreCurSymChanged()
{
  m_locale->setNegativePrefixCurrencySymbol(m_chMonNegPreCurSym->isChecked());
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonPosMonSignPosChanged(int i)
{
  m_locale->setPositiveMonetarySignPosition((KLocale::SignPosition)i);
  emit localeChanged();
}

void KLocaleConfigMoney::slotMonNegMonSignPosChanged(int i)
{
  m_locale->setNegativeMonetarySignPosition((KLocale::SignPosition)i);
  emit localeChanged();
}

void KLocaleConfigMoney::slotTranslate()
{
  TQObjectList list;
  list.append(TQT_TQOBJECT(m_cmbMonPosMonSignPos));
  list.append(TQT_TQOBJECT(m_cmbMonNegMonSignPos));

  TQComboBox *wc;
  for (TQObjectListIt li(list) ; (wc = (TQComboBox *)li.current()) != 0; ++li)
  {
    wc->changeItem(m_locale->translate("Parentheses Around"), 0);
    wc->changeItem(m_locale->translate("Before Quantity Money"), 1);
    wc->changeItem(m_locale->translate("After Quantity Money"), 2);
    wc->changeItem(m_locale->translate("Before Money"), 3);
    wc->changeItem(m_locale->translate("After Money"), 4);
  }

  TQString str;

  str = m_locale->translate( "Here you can enter your usual currency "
                             "symbol, e.g. $ or DM."
                             "<p>Please note that the Euro symbol may not be "
                             "available on your system, depending on the "
                             "distribution you use." );
  TQWhatsThis::add( m_labMonCurSym, str );
  TQWhatsThis::add( m_edMonCurSym, str );
  str = m_locale->translate( "Here you can define the decimal separator used "
                             "to display monetary values."
                             "<p>Note that the decimal separator used to "
                             "display other numbers has to be defined "
                             "separately (see the 'Numbers' tab)." );
  TQWhatsThis::add( m_labMonDecSym, str );
  TQWhatsThis::add( m_edMonDecSym, str );

  str = m_locale->translate( "Here you can define the thousands separator "
                             "used to display monetary values."
                             "<p>Note that the thousands separator used to "
                             "display other numbers has to be defined "
                             "separately (see the 'Numbers' tab)." );
  TQWhatsThis::add( m_labMonThoSep, str );
  TQWhatsThis::add( m_edMonThoSep, str );

  str = m_locale->translate( "This determines the number of fract digits for "
                             "monetary values, i.e. the number of digits you "
                             "find <em>behind</em> the decimal separator. "
                             "Correct value is 2 for almost all people." );
  TQWhatsThis::add( m_labMonFraDig, str );
  TQWhatsThis::add( m_inMonFraDig, str );

  str = m_locale->translate( "If this option is checked, the currency sign "
                             "will be prefixed (i.e. to the left of the "
                             "value) for all positive monetary values. If "
                             "not, it will be postfixed (i.e. to the right)." );
  TQWhatsThis::add( m_chMonPosPreCurSym, str );

  str = m_locale->translate( "If this option is checked, the currency sign "
                             "will be prefixed (i.e. to the left of the "
                             "value) for all negative monetary values. If "
                             "not, it will be postfixed (i.e. to the right)." );
  TQWhatsThis::add( m_chMonNegPreCurSym, str );

  str = m_locale->translate( "Here you can select how a positive sign will be "
                             "positioned. This only affects monetary values." );
  TQWhatsThis::add( m_labMonPosMonSignPos, str );
  TQWhatsThis::add( m_cmbMonPosMonSignPos, str );

  str = m_locale->translate( "Here you can select how a negative sign will "
                             "be positioned. This only affects monetary "
                             "values." );
  TQWhatsThis::add( m_labMonNegMonSignPos, str );
  TQWhatsThis::add( m_cmbMonNegMonSignPos, str );
}
