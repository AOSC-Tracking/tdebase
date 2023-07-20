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

#ifndef __TDMSESS_H__
#define __TDMSESS_H__


#include <tqstring.h>

class TQComboBox;
class TQCheckBox;
class KURLRequester;
class KBackedComboBox;

class TDMSessionsWidget : public TQWidget
{
	TQ_OBJECT

public:
	TDMSessionsWidget(TQWidget *parent=0, const char *name=0);

	void load();
	void save();
	void defaults();
	void makeReadOnly();

	enum SdModes { SdAll, SdRoot, SdNone };

signals:
	void changed( bool state );
	
protected slots:
	void changed();

private:
	void readSD (TQComboBox *, TQString);
	void writeSD (TQComboBox *);

	TQComboBox	*sdlcombo, *sdrcombo;
	TQLabel		*sdllabel, *sdrlabel;
	KURLRequester	*restart_lined, *shutdown_lined;
	KBackedComboBox	*bm_combo;
	TQCheckBox	*tsbox;
};


#endif


