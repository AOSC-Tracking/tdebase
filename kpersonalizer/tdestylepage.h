/***************************************************************************
                          tdestylepage.h  -  description
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

#ifndef TDESTYLEPAGE_H
#define TDESTYLEPAGE_H

#include <tqcolor.h>
#include "tdestylepagedlg.h"

/**Abstract class for the style page
  *@author Ralf Nolden
  */
class TQListViewItem;

class TDEStylePage : public TDEStylePageDlg  {
	TQ_OBJECT

public:
	TDEStylePage(TQWidget *parent=0, const char *name=0);
	~TDEStylePage();
	void save(bool curSettings=true);
	/** resets to KDE style as default */
	void setDefaults();
	/** set the preview-widgets' style to the currently selected */
	void switchPrevStyle();

private:
	TQString origStyle;
	TQString origKWinStyle;
	TQString origIcons;
	TQString defaultKWinStyle;
	TQString currentStyle;
	TDEConfig* ctwin;
	struct colorSet {
		TQString colorFile, bgMode;
		int contrast;
		TQColor usrCol1, usrCol2;
		TQColor foreground;
		TQColor background;
		TQColor windowForeground;
		TQColor windowBackground;
		TQColor selectForeground;
		TQColor selectBackground;
		TQColor buttonForeground;
		TQColor buttonBackground;
		TQColor linkColor;
		TQColor visitedLinkColor;
		TQColor activeForeground;
		TQColor inactiveForeground;
		TQColor activeBackground;
		TQColor inactiveBackground;
		TQColor activeBlend;
		TQColor inactiveBlend;
		TQColor activeTitleBtnBg;
		TQColor inactiveTitleBtnBg;
                TQColor alternateBackground;
	} usrColors, currentColors;
	// first, the KDE 2 default color values
	TQColor widget;
	TQColor trinity4Blue;
        TQColor inactiveBackground;
        TQColor activeBackground;
	TQColor button;
	TQColor link;
	TQColor visitedLink;
        TQColor activeBlend;
        TQColor activeTitleBtnBg;
        TQColor inactiveTitleBtnBg;
        TQColor inactiveForeground;
        TQColor alternateBackground;

	TQListViewItem * kde;
	TQListViewItem * classic;
	TQListViewItem * keramik;
	TQListViewItem * cde;
	TQListViewItem * win;
	TQListViewItem * platinum;

	TQStyle *appliedStyle;

	// widget-style existence
	bool kde_hc_exist, kde_def_exist, kde_keramik_exist, kde_light_exist,
		cde_exist, win_exist, platinum_exist, kde_plastik_exist;

	// twin-style-existence
	bool twin_keramik_exist, twin_default_exist, twin_system_exist,
		twin_win_exist, twin_cde_exist, twin_quartz_exist, twin_plastik_exist;

	// icon-theme-existence
	bool icon_crystalsvg_exist, icon_kdeclassic_exist, icon_Locolor_exist;

public slots: // Public slots
	/** to be connected to the OS page. Catches either KDE, CDE, win or mac and pre-sets the style.  */
	void presetStyle(const TQString& style);

private:
	void saveColors(bool curSettings=true);
	void saveStyle(bool curSettings=true);
	void saveKWin(bool curSettings=true);
	void saveIcons(bool curSettings=true);
	void getAvailability();
	void getUserDefaults();
	void initColors();
	void liveUpdate();
	void getColors(colorSet *set, bool colorfile );
	void setStyleRecursive(TQWidget* w, TQPalette &, TQStyle* s);
	void changeCurrentStyle();
	TQPalette createPalette();

private slots:
	void slotCurrentChanged();
};

#endif
