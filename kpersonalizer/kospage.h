/***************************************************************************
                          kospage.h  -  description
                             -------------------
    begin                : Tue May 22 2001
    copyright            : (C) 2001 by Ralf Nolden
    email                : nolden@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KOSPAGE_H
#define KOSPAGE_H


#include"kospagedlg.h"

/**Abstract class for the second page.  Uses save() to change the according settings and applies them.
  *@author Ralf Nolden
  */

class KOSPage : public KOSPageDlg  {
	TQ_OBJECT
public:
	KOSPage(TQWidget *parent=0, const char *name=0);
	~KOSPage();
	void save(bool currSettings=true);
	void saveCheckState(bool currSettings);
	void writeKDE();
	void writeUNIX();
	void writeWindows();
	void writeMacOS();
	void writeKeyEntrys(TQString keyfile);
	void writeUserKeys();
	void writeUserDefaults();
	/** retrieve the user's local values */
	void getUserDefaults();
	void slotMacDescription();
	void slotWindowsDescription();
	void slotUnixDescription();
	void slotKDEDescription();
	/** resets the radio button selected to kde */
	void setDefaults();
signals: // Signals
	/** emits either of: KDE, CDE, win or mac in save() depending
	on the selection made by the user. */
	void selectedOS(const TQString&);
private:
	TDEConfig* cglobal;
	TDEConfig* claunch;
	TDEConfig* cwin;
	TDEConfig* cdesktop;
	TDEConfig* ckcminput;
	TDEConfig* ckcmdisplay;
	TDEConfig* ckonqueror;
	TDEConfig* cklipper;
	TDEConfig* ckaccess;
	// DEFAULT VALUES SET BY USER
	bool b_Gestures, b_MacMenuBar, b_SingleClick, b_BusyCursor, b_ShowMenuBar,
		 b_DesktopUnderline, b_KonqUnderline, b_ChangeCursor, b_syncClipboards;
	TQString	s_TitlebarDCC, s_FocusPolicy, s_AltTabStyle, s_MMB,
			s_TitlebarMMB, s_TitlebarRMB;
	TQMap<TQString, TQString> map_AppUserKeys, map_GlobalUserKeys;
	// DEFAULT VALLUES SET BY USER (END)
};
#endif
