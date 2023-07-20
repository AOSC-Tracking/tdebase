/* This file is part of the KDE Display Manager Configuration package
    Copyright (C) 1997 Thomas Tanghus (tanghus@earthling.net)

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


#ifndef __TDMAPPEAR_H__
#define __TDMAPPEAR_H__


#include <tqdir.h>
#include <tqimage.h>
#include <tqfileinfo.h>
#include <tqpushbutton.h>

#include <kcolorbutton.h>
#include <kurl.h>
#include <tdefiledialog.h>


#include "klanguagebutton.h"

class TQComboBox;
class KBackedComboBox;
class TQLabel;
class TQRadioButton;
class TQLineEdit;
class KLineEdit;


class TDMAppearanceWidget : public TQWidget
{
	TQ_OBJECT

public:
	TDMAppearanceWidget(TQWidget *parent, const char *name=0);

	void load();
	void save();
	void defaults();
	void makeReadOnly();
	TQString quickHelp() const;

	void loadColorSchemes(KBackedComboBox *combo);
	void loadGuiStyles(KBackedComboBox *combo);
	void loadLanguageList(KLanguageButton *combo);

	bool eventFilter(TQObject *, TQEvent *);

signals:
	void changed( bool state );

protected:
	void iconLoaderDragEnterEvent(TQDragEnterEvent *event);
	void iconLoaderDropEvent(TQDropEvent *event);
	bool setLogo(TQString logo);

private slots:
	void slotAreaRadioClicked(int id);
	void slotLogoButtonClicked();
	void changed();

private:
	enum { KdmNone, KdmClock, KdmLogo };
	TQLabel      *logoLabel;
	TQPushButton *logobutton;
	KLineEdit    *greetstr_lined;
	TQString      logopath;
	TQRadioButton *noneRadio;
	TQRadioButton *clockRadio;
	TQRadioButton *logoRadio;
	TQLineEdit    *xLineEdit;
	TQLineEdit    *yLineEdit;
	KBackedComboBox    *compositorcombo;
	KBackedComboBox    *guicombo;
	KBackedComboBox    *colcombo;
	KBackedComboBox    *echocombo;
	KLanguageButton *langcombo;
	TQCheckBox    *sakbox;
	TQLabel    *sakwarning;
	TQCheckBox    *kbdledbox;

};

#endif
