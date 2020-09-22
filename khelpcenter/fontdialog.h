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
#ifndef FONTDIALOG_H
#define FONTDIALOG_H

#include <kdialogbase.h>

class TQBoxLayout;
class TQSpinBox;

class KComboBox;
class TDEFontCombo;
class KIntNumInput;

namespace KHC {

class FontDialog : public KDialogBase
{
	Q_OBJECT
	public:
		FontDialog( TQWidget *parent, const char *name = 0 );

	protected slots:
		virtual void slotOk();

	private:
		void setupFontSizesBox();
		void setupFontTypesBox();
		void setupFontEncodingBox();

		void load();
		void save();

		KIntNumInput *m_minFontSize;
		KIntNumInput *m_medFontSize;
		TDEFontCombo *m_standardFontCombo;
		TDEFontCombo *m_fixedFontCombo;
		TDEFontCombo *m_serifFontCombo;
		TDEFontCombo *m_sansSerifFontCombo;
		TDEFontCombo *m_italicFontCombo;
		TDEFontCombo *m_fantasyFontCombo;
		KComboBox *m_defaultEncoding;
		TQSpinBox *m_fontSizeAdjustement;
};

}

#endif // FONTDIALOG_H
