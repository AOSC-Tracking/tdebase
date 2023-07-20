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

#ifndef __TDMUSERS_H__
#define __TDMUSERS_H__

#include <tqwidget.h>
#include <tqptrlist.h>
#include <tqstring.h>
#include <tqimage.h>
#include <tqbuttongroup.h>
#include <tqradiobutton.h>
#include <tqcheckbox.h>
#include <tqwidgetstack.h>

#include <klineedit.h>
#include <tdelistview.h>
#include <kcombobox.h>
#include <kurl.h>

#include <pwd.h>


class TDMUsersWidget : public TQWidget
{
	TQ_OBJECT

public:
	TDMUsersWidget( TQWidget *parent = 0, const char *name = 0 );

	void load();
	void save();
	void defaults();
	void makeReadOnly();

	bool eventFilter( TQObject *o, TQEvent *e );

public slots:
	void slotClearUsers();
	void slotAddUsers( const TQMap<TQString,int> & );
	void slotDelUsers( const TQMap<TQString,int> & );

signals:
	void changed( bool state );
	void setMinMaxUID( int, int );

private slots:
	void slotMinMaxChanged();
	void slotShowOpts();
	void slotUpdateOptIn( TQListViewItem *item );
	void slotUpdateOptOut( TQListViewItem *item );
	void slotUserSelected();
	void slotUnsetUserPix();
	void slotFaceOpts();
	void slotUserButtonClicked();
	void slotChanged();

private:
	void updateOptList( TQListViewItem *item, TQStringList &list );
	void userButtonDropEvent( TQDropEvent *e );
	void changeUserPix( const TQString & );

	TQGroupBox	*minGroup;	// top left
	TQLineEdit	*leminuid, *lemaxuid;

	TQButtonGroup	*usrGroup; // right below
	TQCheckBox	*cbshowlist, *cbcomplete, *cbinverted, *cbusrsrt;

	TQLabel		*s_label; // middle
	TQWidgetStack	*wstack;
	TDEListView	*optoutlv, *optinlv;

	TQButtonGroup	*faceGroup; // right
	TQRadioButton	*rbadmonly, *rbprefadm, *rbprefusr, *rbusronly;

	KComboBox	*usercombo; // right below
	TQPushButton	*userbutton;
	TQPushButton	*rstuserbutton;

	TQString	m_userPixDir;
	TQString	m_defaultText;
	TQStringList	hiddenUsers, selectedUsers;
	TQString	defminuid, defmaxuid;

	bool		m_notFirst;
};

#endif


