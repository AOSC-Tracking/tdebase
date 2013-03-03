/* This file is part of the KDE Display Manager Configuration package

    Copyright (C) 2000 Oswald Buddenhagen <ossi@kde.org>
    Based on several other files.

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

#ifndef __KDMCONV_H__
#define __KDMCONV_H__

#include <tqptrlist.h>
#include <tqstring.h>
#include <tqimage.h>
#include <tqgroupbox.h>
#include <tqradiobutton.h>
#include <tqcheckbox.h>
#include <tqspinbox.h>

#include <klistbox.h>
#include <kcombobox.h>
#include <kcolorbutton.h>
#include <klistview.h>
#include <kurl.h>

#include <pwd.h>


class KDMConvenienceWidget : public TQWidget
{
	Q_OBJECT

public:
	KDMConvenienceWidget(TQWidget *parent=0, const char *name=0);

        void load();
        void save();
	void defaults();
	void makeReadOnly();

public slots:
	void slotClearUsers();
	void slotAddUsers(const TQMap<TQString,int> &);
	void slotDelUsers(const TQMap<TQString,int> &);


signals:
	void changed( bool state );

private slots:
	void slotPresChanged();
	void slotChanged();
	void slotSetAutoUser( const TQString &user );
	void slotSetPreselUser( const TQString &user );
	void slotUpdateNoPassUser( TQListViewItem *item );

private:
	TQGroupBox	*alGroup, *puGroup, *npGroup, *btGroup;
	TQCheckBox	*againcb, *cbarlen, *cbjumppw, *autoLockCheck, *allowRootLogin;
	TQRadioButton	*npRadio, *ppRadio, *spRadio;
	KComboBox	*userlb, *puserlb;
	TQSpinBox	*delaysb;
	KListView	*npuserlv;
	TQLabel		*u_label, *d_label, *pu_label, *w_label, *n_label, *pl_label;
	TQString	autoUser, preselUser;
	TQStringList	noPassUsers;
};

#endif


