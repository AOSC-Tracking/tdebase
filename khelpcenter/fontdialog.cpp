/*
 *  This file is part of the TDE Help Center
 *
 *  Copyright (C) 2003 Frerich Raabe <raabe@kde.org>
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
#include "fontdialog.h"

#include <tdeapplication.h>
#include <kcharsets.h>
#include <kcombobox.h>
#include <tdeconfig.h>
#include <tdefontcombo.h>
#include <tdehtmldefaults.h>
#include <tdelocale.h>
#include <knuminput.h>

#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqspinbox.h>

using namespace KHC;

FontDialog::FontDialog( TQWidget *parent, const char *name )
	: KDialogBase( parent, name, true, i18n( "Font Configuration" ),
	               Ok | Cancel )
{
	makeVBoxMainWidget();

	setupFontSizesBox();
	setupFontTypesBox();
	setupFontEncodingBox();

	load();
}

void FontDialog::slotOk()
{
	save();
	accept();
}

void FontDialog::setupFontSizesBox()
{
	TQGroupBox *gb = new TQGroupBox( i18n( "Sizes" ), mainWidget() );

	TQGridLayout *layout = new TQGridLayout( gb );
	layout->setSpacing( KDialog::spacingHint() );
	layout->setMargin( KDialog::marginHint() * 2 );

	TQLabel *lMinFontSize = new TQLabel( i18n( "M&inimum font size:" ), gb );
	layout->addWidget( lMinFontSize, 0, 0 );
	m_minFontSize = new KIntNumInput( gb );
	layout->addWidget( m_minFontSize, 0, 1 );
	m_minFontSize->setRange( 1, 20 );
	lMinFontSize->setBuddy( m_minFontSize );

	TQLabel *lMedFontSize = new TQLabel( i18n( "M&edium font size:" ), gb );
	layout->addWidget( lMedFontSize, 1, 0 );
	m_medFontSize = new KIntNumInput( gb );
	layout->addWidget( m_medFontSize, 1, 1 );
	m_medFontSize->setRange( 4, 28 );
	lMedFontSize->setBuddy( m_medFontSize );
}

void FontDialog::setupFontTypesBox()
{
	TQGroupBox *gb = new TQGroupBox( i18n( "Fonts" ), mainWidget() );

	TQGridLayout *layout = new TQGridLayout( gb );
	layout->setSpacing( KDialog::spacingHint() );
	layout->setMargin( KDialog::marginHint() * 2 );

	TQLabel *lStandardFont = new TQLabel( i18n( "S&tandard font:" ), gb );
	layout->addWidget( lStandardFont, 0, 0 );
	m_standardFontCombo = new TDEFontCombo( gb );
	layout->addWidget( m_standardFontCombo, 0, 1 );
	lStandardFont->setBuddy( m_standardFontCombo );

	TQLabel *lFixedFont = new TQLabel( i18n( "F&ixed font:" ), gb );
	layout->addWidget( lFixedFont, 1, 0 );
	m_fixedFontCombo = new TDEFontCombo( gb );
	layout->addWidget( m_fixedFontCombo, 1, 1 );
	lFixedFont->setBuddy( m_fixedFontCombo );

	TQLabel *lSerifFont = new TQLabel( i18n( "S&erif font:" ), gb );
	layout->addWidget( lSerifFont, 2, 0 );
	m_serifFontCombo = new TDEFontCombo( gb );
	layout->addWidget( m_serifFontCombo, 2, 1 );
	lSerifFont->setBuddy( m_serifFontCombo );

	TQLabel *lSansSerifFont = new TQLabel( i18n( "S&ans serif font:" ), gb );
	layout->addWidget( lSansSerifFont, 3, 0 );
	m_sansSerifFontCombo = new TDEFontCombo( gb );
	layout->addWidget( m_sansSerifFontCombo, 3, 1 );
	lSansSerifFont->setBuddy( m_sansSerifFontCombo );

	TQLabel *lItalicFont = new TQLabel( i18n( "&Italic font:" ), gb );
	layout->addWidget( lItalicFont, 4, 0 );
	m_italicFontCombo = new TDEFontCombo( gb );
	layout->addWidget( m_italicFontCombo, 4, 1 );
	lItalicFont->setBuddy( m_italicFontCombo );

	TQLabel *lFantasyFont = new TQLabel( i18n( "&Fantasy font:" ), gb );
	layout->addWidget( lFantasyFont, 5, 0 );
	m_fantasyFontCombo = new TDEFontCombo( gb );
	layout->addWidget( m_fantasyFontCombo, 5, 1 );
	lFantasyFont->setBuddy( m_fantasyFontCombo );
}

void FontDialog::setupFontEncodingBox()
{
	TQGroupBox *gb = new TQGroupBox( i18n( "Encoding" ), mainWidget() );

	TQGridLayout *layout = new TQGridLayout( gb );
	layout->setSpacing( KDialog::spacingHint() );
	layout->setMargin( KDialog::marginHint() * 2 );

	TQLabel *lDefaultEncoding = new TQLabel( i18n( "&Default encoding:" ), gb );
	layout->addWidget( lDefaultEncoding, 0, 0 );
	m_defaultEncoding = new KComboBox( false, gb );
	layout->addWidget( m_defaultEncoding, 0, 1 );
	TQStringList encodings = TDEGlobal::charsets()->availableEncodingNames();
	encodings.prepend( i18n( "Use Language Encoding" ) );
	m_defaultEncoding->insertStringList( encodings );
	lDefaultEncoding->setBuddy( m_defaultEncoding );

	TQLabel *lFontSizeAdjustement = new TQLabel( i18n( "&Font size adjustment:" ), gb );
	layout->addWidget( lFontSizeAdjustement, 1, 0 );
	m_fontSizeAdjustement = new TQSpinBox( -5, 5, 1, gb );
	layout->addWidget( m_fontSizeAdjustement, 1, 1 );
	lFontSizeAdjustement->setBuddy( m_fontSizeAdjustement );
}

void FontDialog::load()
{
	TDEConfig *cfg = kapp->config();
	{
		TDEConfigGroupSaver groupSaver( cfg, "HTML Settings" );

		m_minFontSize->setValue( cfg->readNumEntry( "MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE ) );
		m_medFontSize->setValue( cfg->readNumEntry( "MediumFontSize", 10 ) );

		TQStringList fonts = cfg->readListEntry( "Fonts" );
		if ( fonts.isEmpty() )
			fonts << TDEGlobalSettings::generalFont().family()
			      << TDEGlobalSettings::fixedFont().family()
			      << HTML_DEFAULT_VIEW_SERIF_FONT
			      << HTML_DEFAULT_VIEW_SANSSERIF_FONT
			      << HTML_DEFAULT_VIEW_CURSIVE_FONT
			      << HTML_DEFAULT_VIEW_FANTASY_FONT;

		m_standardFontCombo->setCurrentFont( fonts[ 0 ] );
		m_fixedFontCombo->setCurrentFont( fonts[ 1 ] );
		m_serifFontCombo->setCurrentFont( fonts[ 2 ] );
		m_sansSerifFontCombo->setCurrentFont( fonts[ 3 ] );
		m_italicFontCombo->setCurrentFont( fonts[ 4 ] );
		m_fantasyFontCombo->setCurrentFont( fonts[ 5 ] );

		m_defaultEncoding->setCurrentItem( cfg->readEntry( "DefaultEncoding" ) );
		m_fontSizeAdjustement->setValue( fonts[ 6 ].toInt() );
	}
}

void FontDialog::save()
{
	TDEConfig *cfg = kapp->config();
	{
		TDEConfigGroupSaver groupSaver( cfg, "General" );
		cfg->writeEntry( "UseKonqSettings", false );
	}
	{
		TDEConfigGroupSaver groupSaver( cfg, "HTML Settings" );

		cfg->writeEntry( "MinimumFontSize", m_minFontSize->value() );
		cfg->writeEntry( "MediumFontSize", m_medFontSize->value() );

		TQStringList fonts;
		fonts << m_standardFontCombo->currentText()
		      << m_fixedFontCombo->currentText()
		      << m_serifFontCombo->currentText()
		      << m_sansSerifFontCombo->currentText()
		      << m_italicFontCombo->currentText()
		      << m_fantasyFontCombo->currentText()
		      << TQString::number( m_fontSizeAdjustement->value() );

		cfg->writeEntry( "Fonts", fonts );

		if ( m_defaultEncoding->currentText() == i18n( "Use Language Encoding" ) )
			cfg->writeEntry( "DefaultEncoding", TQString::null );
		else
			cfg->writeEntry( "DefaultEncoding", m_defaultEncoding->currentText() );
	}
	cfg->sync();
}

#include "fontdialog.moc"
