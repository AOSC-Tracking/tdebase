/*
 *   tdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef TDEPRINTFAX_H
#define TDEPRINTFAX_H

#include <tdemainwindow.h>
#include <kurl.h>

class TDEListBox;
class TDEListView;
class TQLineEdit;
class TQTextEdit;
class FaxCtrl;
class TQCheckBox;
class TQLabel;
class TQTimeEdit;
class TQComboBox;
class TQPushButton;
class TQListViewItem;

class KdeprintFax : public TDEMainWindow
{
	TQ_OBJECT
public:
	struct FaxItem
	{
		TQString number;
		TQString name;
		TQString enterprise;
	};
	typedef TQValueList<FaxItem> FaxItemList;
	typedef TQValueList<FaxItem>::ConstIterator FaxItemListIterator;

	KdeprintFax(TQWidget *parent = 0, const char *name = 0);
	~KdeprintFax();

	void addURL(KURL url);
	void setPhone(TQString phone);
	void sendFax( bool quitAfterSend );
	TQStringList files();
	int faxCount() const;
	//TQString number( int i = 0 ) const;
	//TQString name( int i = 0 ) const;
	//TQString enterprise( int i = 0 ) const;
	FaxItemList faxList() const;
	TQString comment() const;
	TQString time() const;
	TQString subject() const;
	bool cover() const;

protected slots:
	void slotToggleMenuBar();
	void slotKab();
	void slotAdd();
	void slotRemove();
	void slotFax();
	void slotAbort();
	void slotMessage(const TQString&);
	void slotFaxSent(bool);
	void slotViewLog();
	void slotConfigure();
	void slotQuit();
	void slotView();
	void slotTimeComboActivated(int);
	void slotMoveUp();
	void slotMoveDown();
	void slotCurrentChanged();
	void slotFaxSelectionChanged();
	void slotFaxRemove();
	void slotFaxAdd();
	void slotFaxExecuted( TQListViewItem* );
	void slotCoverToggled(bool toggle);

protected:
	void initActions();
	void dragEnterEvent(TQDragEnterEvent*);
	void dropEvent(TQDropEvent*);
	void updateState();
	bool manualFaxDialog( TQString& number, TQString& name, TQString& enterprise );
	//TQListViewItem* faxItem( int i = 0 ) const;

private:
	TDEListBox	*m_files;
	TDEListView   *m_numbers;
	TQLineEdit	*m_subject;
	TQTextEdit	*m_comment;
	FaxCtrl		*m_faxctrl;
	TQCheckBox	*m_cover;
	TQLabel		*m_msglabel;
	TQTimeEdit	*m_time;
	TQComboBox	*m_timecombo;
	TQPushButton *m_upbtn, *m_downbtn;
	TQPushButton *m_newbtn, *m_abbtn, *m_delbtn;
	bool m_quitAfterSend;
};

#endif
