/*

chooser widget for TDM

Copyright (C) 2002-2003 Oswald Buddenhagen <ossi@kde.org>
based on the chooser (C) 1999 by Harald Hoyer <Harald.Hoyer@RedHat.de>

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

#ifndef KCHOOSER_H
#define KCHOOSER_H

#include "kgdialog.h"

class TQSocketNotifier;
class TQPopupMenu;
class TQLineEdit;
class TQListView;
class TQListViewItem;

class ChooserDlg : public KGDialog {
	TQ_OBJECT
	typedef KGDialog inherited;

  public:
	ChooserDlg();

  public slots:
	void slotReadPipe();
	void addHostname();
//	void slotHelp();
	void pingHosts();
	void accept();
	void reject();

  private:
	TQString recvStr();
	TQListViewItem *findItem( int id );

	TQListView *host_view;
	TQLineEdit *iline;
	TQSocketNotifier *sn;
};

#endif /* KCHOOSER_H */
