/*

Base class for various tdm greeter dialogs

Copyright (C) 1997, 1998 Steffen Hansen <hansen@kde.org>
Copyright (C) 2000-2004 Oswald Buddenhagen <ossi@kde.org>


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


#ifndef KGDIALOG_H
#define KGDIALOG_H

#include <config.h> // for WITH_TDM_XCONSOLE

#include "tdmconfig.h"
#include "kfdialog.h"

class TQPopupMenu;
class TQGridLayout;
class KConsole;
class KGVerify;

#define ex_exit    1
#define ex_greet   2
#define ex_choose  3

class KGDialog : public FDialog {
	TQ_OBJECT
	typedef FDialog inherited;

  public:
	KGDialog( bool themed = false );

  public slots:
	void slotActivateMenu( int id );
	void slotExit();
	void slotSwitch();
	void slotReallySwitch();
	void slotConsole();
	void slotShutdown( int id );

  protected:
#ifdef XDMCP
	void completeMenu( int _switchIf, int _switchCode, const TQString &_switchMsg, int _switchAccel );
#else
	void completeMenu();
#endif
	void inserten( const TQString& txt, int accel, const char *member );
	int inserten( const TQString& txt, int accel, TQPopupMenu *cmnu );

	bool needSep;
	TQPopupMenu *optMenu;
	KGVerify *verify;
#ifdef WITH_TDM_XCONSOLE
	KConsole *consoleView;
#endif

  private slots:
	void slotDisplaySelected( int vt );
	void slotPopulateDisplays();

  private:
	void ensureMenu();

#ifdef HAVE_VTS
	TQPopupMenu *dpyMenu;
#endif
	int switchCode;
};

#endif /* KGDIALOG_H */
