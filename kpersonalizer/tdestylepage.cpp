/***************************************************************************
                          tdestylepage.cpp  -  description
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
#include <stdlib.h>

#include <tqdir.h>
#include <tqlabel.h>
#include <tqlistview.h>
#include <tqcolor.h>
#include <tqstylefactory.h>
#include <tqpixmapcache.h>
#include <tqstyle.h>
#include <tqobjectlist.h>

#include <tdeconfig.h>
#include <kstandarddirs.h>
#include <tdeapplication.h>
#include <tdeglobal.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <tdelocale.h>
#include <tdelistview.h>
#include <kipc.h>
#include <ksimpleconfig.h>
#include <dcopclient.h>
#include <tdestyle.h>
#include <kicontheme.h>
#include <kiconloader.h>

#include "stylepreview.h"
#include "tdestylepage.h"

TDEStylePage::TDEStylePage(TQWidget *parent, const char *name ) : TDEStylePageDlg(parent,name) {

	px_stylesSidebar->setPixmap(UserIcon("step4.png"));

	klv_styles->addColumn(i18n("Style"));
	klv_styles->addColumn( i18n( "Description" ) );
	klv_styles->setAllColumnsShowFocus(true);

	kde = new TQListViewItem( klv_styles);
	kde->setText( 0, TQPixmap::defaultDepth() > 8 ? i18n( "Plastik" ) : i18n( "Light" ) );
	kde->setText( 1, i18n( "TDE default style" ) );

	classic = new TQListViewItem( klv_styles);
	classic->setText( 0, i18n( "TDE Classic" ) );
	classic->setText( 1, i18n( "Classic TDE style" ) );

	keramik = new TQListViewItem( klv_styles );
	keramik->setText( 0, i18n( "Keramik" ) );
	keramik->setText( 1, i18n( "The previous default style" ) );

	cde = new TQListViewItem( klv_styles);
	cde->setText( 0, i18n( "Sunshine" ) );
	cde->setText( 1, i18n( "A very common desktop" ) );

	win = new TQListViewItem( klv_styles );
	win->setText( 0, i18n( "Redmond" ) );
	win->setText( 1, i18n( "A style from the northwest of the USA" ) );

	platinum = new TQListViewItem( klv_styles );
	platinum->setText( 0, i18n( "Platinum" ) );
	platinum->setText( 1, i18n( "The platinum style" ) );

	connect(klv_styles, TQ_SIGNAL(selectionChanged()),
			this, TQ_SLOT(slotCurrentChanged()));

	// Note: if the default is changed here it needs to be changed in tdebase/twin/plugins.cpp
	//       and tdebase/twin/kcmtwin/twindecoration/twindecoration.cpp as well.
	defaultKWinStyle = TQPixmap::defaultDepth() > 8 ? "twin_plastik" : "twin_quartz";
	appliedStyle = NULL;

	getAvailability();
	getUserDefaults();
	initColors();
}

TDEStylePage::~TDEStylePage(){
	delete ctwin;
	delete appliedStyle;
}

void TDEStylePage::save(bool curSettings){
	kdDebug() << "TDEStylePage::save()" << endl;
	// First, the style, then the colors as styles overwrite color settings
	saveStyle(curSettings);
	saveColors(curSettings);
	saveKWin(curSettings);
	saveIcons(curSettings);
	liveUpdate();
}

/** save the widget-style */
void TDEStylePage::saveStyle(bool curSettings){
	TQString style = curSettings ? currentStyle : origStyle;
	TDEConfig cfg( "kdeglobals" );
	cfg.setGroup("General");
	cfg.writeEntry( "widgetStyle", style, true, true );
	cfg.sync();
	kdDebug() << "TDEStylePage::saveStyle(): " << style << endl;
}

/** save the KWin-style*/
void TDEStylePage::saveKWin(bool curSettings){
	TQString twin = origKWinStyle;
	if(curSettings) {
		KDesktopFile* kdf = 0L;
		TDEStandardDirs* kstd = TDEGlobal::dirs();
		if (cde->isSelected() && twin_cde_exist)
			kdf = new KDesktopFile(kstd->findResource("data", "twin/cde.desktop"));
		else if (win->isSelected() && twin_win_exist)
			kdf = new KDesktopFile(kstd->findResource("data", "twin/redmond.desktop"));
		else if (platinum->isSelected() && twin_system_exist)
			kdf = new KDesktopFile(kstd->findResource("data", "twin/system.desktop"));
		else if (keramik->isSelected() && twin_keramik_exist)
			kdf = new KDesktopFile(kstd->findResource("data", "twin/keramik.desktop"));
		else if (kde->isSelected()) {
			if (twin_plastik_exist && (TQColor::numBitPlanes() > 8))
				kdf = new KDesktopFile(kstd->findResource("data", "twin/plastik.desktop"));
			else if (twin_quartz_exist)
				kdf = new KDesktopFile(kstd->findResource("data", "twin/quartz.desktop"));
		}

		if (kdf) {
			kdf->setGroup("Desktop Entry");
			twin = kdf->readEntry("X-TDE-Library", defaultKWinStyle);
			delete kdf;
		}
		else {
			// if we get here classic is selected (there's no .desktop file for the
			// kde2 twin style),  or none of the other twin styles were found.
			twin = "twin_default";
		}
	}
	ctwin->writeEntry("PluginLib", twin);
	ctwin->sync();
	kdDebug() << "TDEStylePage::saveKWin(): " << twin << endl;
}

/** Save the color-scheme */
void TDEStylePage::saveColors(bool curSettings){
	struct colorSet* toSave;
	if(curSettings)
		toSave=&currentColors;  // set the color struct to save as the style colors
	else
		toSave=&usrColors;

	// the GLOBAL config entries must be taken from the kcsrc file and written to it. Use the default values
	// equals that the file is <default> which is no file. TODO: use the default values in that case (kde selected)
	TDEConfig *config = TDEGlobal::config();
	config->setGroup( "General" );
	config->writeEntry("foreground", toSave->foreground, true, true);
	config->writeEntry("background", toSave->background, true, true);
	config->writeEntry("windowForeground", toSave->windowForeground, true, true);
	config->writeEntry("windowBackground", toSave->windowBackground, true, true);
	config->writeEntry("selectForeground",  toSave->selectForeground, true, true);
	config->writeEntry("selectBackground", toSave->selectBackground, true, true);
	config->writeEntry("buttonForeground", toSave->buttonForeground, true, true);
	config->writeEntry("buttonBackground", toSave->buttonBackground, true, true);
	config->writeEntry("linkColor", toSave->linkColor, true, true);
	config->writeEntry("visitedLinkColor", toSave->visitedLinkColor, true, true);

	// set to the WM group, *only* the TDEGlobal one, a kcsrc file only has the group "Color Scheme"  hmpf...
	config->setGroup( "WM" );
	config->writeEntry("activeForeground", toSave->activeForeground, true, true);
	config->writeEntry("inactiveForeground", toSave->inactiveForeground, true, true);
	config->writeEntry("activeBackground", toSave->activeBackground, true, true);
	config->writeEntry("inactiveBackground", toSave->inactiveBackground, true, true);
	config->writeEntry("activeBlend", toSave->activeBlend, true, true);
	config->writeEntry("inactiveBlend", toSave->inactiveBlend, true, true);
	config->writeEntry("activeTitleBtnBg", toSave->activeTitleBtnBg, true, true);
	config->writeEntry("inactiveTitleBtnBg", toSave->inactiveTitleBtnBg, true, true);
        config->writeEntry("alternateBackground", toSave->alternateBackground, true, true);

	////////////////////////////////////////////////////
	// KDE-1.x support
	KSimpleConfig *tdeconfig =
	new KSimpleConfig( TQDir::homeDirPath() + "/.tderc" );
	tdeconfig->setGroup( "General" );
	tdeconfig->writeEntry("background", toSave->background );
	tdeconfig->writeEntry("selectBackground", toSave->selectBackground );
	tdeconfig->writeEntry("foreground", toSave->foreground );
	tdeconfig->writeEntry("windowForeground", toSave->windowForeground );
	tdeconfig->writeEntry("windowBackground", toSave->windowBackground );
	tdeconfig->writeEntry("selectForeground", toSave->selectForeground );
	tdeconfig->sync();
	delete tdeconfig;

	config->setGroup("KDE");
	// write the color scheme filename and the contrast, default 7, otherwise from file
	config->writeEntry("colorScheme", toSave->colorFile,true, true);
	config->writeEntry("contrast", toSave->contrast, true, true);
	config->sync();

	// background color changes
	TDEConfig kdesktop("kdesktoprc");
	kdesktop.setGroup("Desktop0"); // we only need to set one desktop

	kdesktop.writeEntry("BackgroundMode", toSave->bgMode);
	kdesktop.writeEntry("Color1", toSave->usrCol1);
	kdesktop.writeEntry("Color2", toSave->usrCol2);
	kdesktop.sync();
	kdDebug() << "TDEStylePage::saveColors(): colorFile: " << toSave->colorFile << endl;
}

/** save the icon-theme*/
void TDEStylePage::saveIcons(bool curSettings) {
	TQString theme = origIcons;
	if (curSettings) {
		if ( (kde->isSelected() || platinum->isSelected() || keramik->isSelected())
			&& icon_crystalsvg_exist)
				theme = "crystalsvg";
		else if ( (classic->isSelected() || cde->isSelected() || win->isSelected())
			&& (TQColor::numBitPlanes() > 8) && icon_kdeclassic_exist)
				theme = "kdeclassic";
		else if ( (classic->isSelected() || cde->isSelected() || win->isSelected())
			&& (TQColor::numBitPlanes() <= 8) && icon_Locolor_exist)
				theme = "Locolor";
	}
	// save, what we got
	TDEGlobal::config()->setGroup("Icons");
	TDEGlobal::config()->writeEntry("Theme", theme, true, true);
	TDEIconTheme icontheme(theme);
	const char * const groups[] = { "Desktop", "Toolbar", "MainToolbar", "Small", 0L };
	for (TDEIcon::Group i=TDEIcon::FirstGroup; i<TDEIcon::LastGroup; i++) {
		if (groups[i] == 0L)
			break;
		TDEGlobal::config()->setGroup(TQString::fromLatin1(groups[i]) + "Icons");
		TDEGlobal::config()->writeEntry("Size", icontheme.defaultSize(i));
	}
	TDEGlobal::config()->sync();
	kdDebug() << "TDEStylePage::saveIcons(): " << theme << endl;
}

/** called whenever the selection in the listview changes */
void TDEStylePage::slotCurrentChanged() {
	// fill the currentColors-struct with the selected color-scheme
	getColors(&currentColors, true);
	// fill the currentStyle-string with the selected style-name
	changeCurrentStyle();
	switchPrevStyle();
}

void TDEStylePage::changeCurrentStyle() {
	// set the style
	if (cde->isSelected() && cde_exist) {
		currentStyle="Motif";
	}
	else if (win->isSelected() && win_exist) {
		currentStyle="Windows";
	}
	else if (platinum->isSelected() && platinum_exist) {
		currentStyle="Platinum";
	}
	else if (keramik->isSelected() && kde_plastik_exist) {
		currentStyle="Keramik";
	}
	else if (classic->isSelected()) {
		// Use the highcolor style if the display supports it
		if ( (TQColor::numBitPlanes() > 8) && kde_hc_exist ) {
			currentStyle="HighColor";
		}
		else if (kde_def_exist) {
			currentStyle="Default";
		}
	}
	else if (kde->isSelected()) {
		// Use the plastik style if the display supports it
		if ( (TQColor::numBitPlanes() > 8) && kde_plastik_exist ) {
			currentStyle="Plastik";
		}
		else if (kde_light_exist) {
			currentStyle="Light, 3rd revision";
		}
		// We should never get here
		else if (kde_def_exist) {
			currentStyle="Default";
		}
	}
	// update the preview-widget
	kdDebug() << "TDEStylePage::changeCurrentStyle(): "<< currentStyle << endl;
}

/** to be connected to the OS page. Catches
 *  either KDE, CDE, win or mac and pre-sets the style.
 */
void TDEStylePage::presetStyle(const TQString& style){
	kdDebug() << "TDEStylePage::presetStyle(): "<< style << endl;
	if(style=="KDE") {
		if (kde_plastik_exist)
			klv_styles->setSelected(kde,true);
		else if (kde_hc_exist || kde_def_exist)
			klv_styles->setSelected(classic,true);
	}
	else if(style=="CDE" && cde_exist)
		klv_styles->setSelected(cde,true);
	else if(style=="win" && win_exist)
		klv_styles->setSelected(win,true);
	else if(style=="mac") {
		klv_styles->setSelected(platinum,true);
	}
}

/** set the defaults for this page */
void TDEStylePage::setDefaults(){
}

/** Fill a colorSet with a colorfile, or the default. */
void TDEStylePage::getColors(colorSet *set, bool colorfile ){
	TDEConfig* config;
	bool deleteConfig = false;
	// get the color scheme file and go to the color scheme group
	if(colorfile){
		TDEGlobal::dirs()->addResourceType("colors", TDEStandardDirs::kde_default("data")+"tdedisplay/color-schemes");
		// set the style
		if (kde->isSelected()) {
			set->bgMode="Flat";
                        set->usrCol1.setNamedColor("#003082");
			set->usrCol2.setNamedColor("#6C8BB9");
			set->colorFile="<default>";
		}
		else if(classic->isSelected()){
			set->bgMode="VerticalGradient";
			set->usrCol1.setNamedColor ("#1E72A0");
			set->usrCol2.setNamedColor ("#C0C0C0");
			set->colorFile=locate("colors", "KDETwo.kcsrc");
		}
		else if(keramik->isSelected()){
			set->bgMode="VerticalGradient";
			set->usrCol1.setNamedColor ("#1E72A0");
			set->usrCol2.setNamedColor ("#C0C0C0");
			set->colorFile=locate("colors","Keramik.kcsrc");
		}
		else if(cde->isSelected()){
			set->bgMode="Flat";
			set->usrCol1.setNamedColor("#718BA5");
			set->usrCol2.setNamedColor ("#C0C0C0");
			set->colorFile=locate("colors","SolarisCDE.kcsrc");
		}
		else if(win->isSelected()){
			set->bgMode="Flat";
			set->usrCol1.setNamedColor("#008183");
			set->usrCol2.setNamedColor ("#C0C0C0");
			set->colorFile=locate("colors","Windows2000.kcsrc");
		}
		else if(platinum->isSelected()){
			set->bgMode="VerticalGradient";
			set->usrCol1.setNamedColor("#2A569D");
			set->usrCol2.setNamedColor("#6C8BB9");
			set->colorFile=locate("colors","EveX.kcsrc");
		}
		set->contrast=7;
		config = new KSimpleConfig(set->colorFile, true);
		config->setGroup("Color Scheme");
		kdDebug() << "TDEStylePage::getColors(): schemefile: " << set->colorFile << endl;
                deleteConfig = true;
	}
	else {
		TDEConfig kdesktop("kdesktoprc");
		kdesktop.setGroup("Desktop0");
		// set Background (userSettings if available, else default)
		set->bgMode=kdesktop.readEntry("BackgroundMode", "Flat");
                TQColor tmp1("#003082");
		TQColor tmp2("#C0C0C0");
		set->usrCol1=kdesktop.readColorEntry("Color1", &tmp1);
		set->usrCol2=kdesktop.readColorEntry("Color2", &tmp2);
		// write the color scheme filename and the contrast, default 7, otherwise from file
		config=TDEGlobal::config();
		config->setGroup("KDE");
		set->colorFile=config->readEntry("colorScheme", "<default>");
		set->contrast=config->readNumEntry("contrast", 7);
		config->setGroup( "General" );
		kdDebug() << "TDEStylePage::getColors(): schemefile: "<< set->colorFile << endl;
    }
	set->foreground=config->readColorEntry( "foreground", &black );
	set->background=config->readColorEntry( "background", &widget );
	set->windowForeground=config->readColorEntry( "windowForeground", &black );
	set->windowBackground=config->readColorEntry( "windowBackground", &white );
	set->selectForeground=config->readColorEntry( "selectForeground", &white );
	set->selectBackground=config->readColorEntry( "selectBackground", &trinity4Blue );
	set->buttonForeground=config->readColorEntry( "buttonForeground", &black );
	set->buttonBackground=config->readColorEntry( "buttonBackground", &button );
	set->linkColor=config->readColorEntry( "linkColor", &link );
	set->visitedLinkColor=config->readColorEntry( "visitedLinkColor", &visitedLink );
	// it's necessary to set the group, when reading from globalrc
	if(!colorfile)
		config->setGroup( "WM" );
	set->activeForeground=config->readColorEntry("activeForeground", &white);
        set->inactiveForeground=config->readColorEntry("inactiveForeground", &inactiveForeground);
        set->activeBackground=config->readColorEntry("activeBackground", &activeBackground);
        set->inactiveBackground=config->readColorEntry("inactiveBackground", &inactiveBackground);
        set->activeBlend=config->readColorEntry("activeBlend", &activeBlend);
        set->inactiveBlend=config->readColorEntry("inactiveBlend", &inactiveBackground);
        set->activeTitleBtnBg=config->readColorEntry("activeTitleBtnBg", &activeTitleBtnBg);
        set->inactiveTitleBtnBg=config->readColorEntry("inactiveTitleBtnBg", &inactiveTitleBtnBg);
        set->alternateBackground=config->readColorEntry("alternateBackground", &alternateBackground);
        if ( deleteConfig)
            delete config;
}

/** Test widget- and twin- styles for availability */
void TDEStylePage::getAvailability() {
	// test, wich styles are available
	kde_keramik_exist = kde_hc_exist = kde_def_exist = cde_exist
		= kde_plastik_exist = win_exist = platinum_exist = false;
	TQStringList styles = TQStyleFactory::keys();
	for (TQStringList::iterator it = styles.begin(); it != styles.end(); it++) {
		if (*it == "Keramik") kde_keramik_exist = true;
		else if (*it == "HighColor") kde_hc_exist = true;
		else if (*it == "Default") kde_def_exist = true;
		else if (*it == "Plastik") kde_plastik_exist = true;
		else if (*it == "Motif") cde_exist = true;
		else if (*it == "Windows") win_exist = true;
		else if (*it == "Platinum") platinum_exist = true;
		else if (*it == "Light, 3rd revision") kde_light_exist = true;
	}
	// and disable the ListItems, if they are not.
	if ( !(kde_plastik_exist || kde_light_exist) ) kde->setVisible(false);
	if ( !(kde_hc_exist || kde_def_exist) ) classic->setVisible(false);
	if (!kde_keramik_exist || TQPixmap::defaultDepth() <= 8) keramik->setVisible(false);
	if (!cde_exist) cde->setVisible(false);
	if (!win_exist) win->setVisible(false);
	if (!platinum_exist) platinum->setVisible(false);

	// test, wich KWin-styles are available
	twin_keramik_exist = twin_system_exist = twin_plastik_exist
			= twin_default_exist = twin_win_exist
			= twin_cde_exist = twin_quartz_exist = false;
	TDEStandardDirs* kstd = TDEGlobal::dirs();
	if (!kstd->findResource("data", "twin/keramik.desktop").isNull())
		twin_keramik_exist = true;
	if (!kstd->findResource("data", "twin/plastik.desktop").isNull())
		twin_plastik_exist = true;
	if (!kstd->findResource("data", "twin/system.desktop").isNull())
		twin_system_exist = true;
	if (!kstd->findResource("data", "twin/redmond.desktop").isNull())
		twin_win_exist = true;
	if (!kstd->findResource("data", "twin/cde.desktop").isNull())
		twin_cde_exist = true;
	if (!kstd->findResource("data", "twin/quartz.desktop").isNull())
		twin_quartz_exist = true;
	twin_default_exist = true;	// we can't check for a .desktop-file for the old default because there is none

	// check, wich Icon-themes are available
	icon_crystalsvg_exist = icon_kdeclassic_exist = icon_Locolor_exist = false;
	TQStringList icons(TDEIconTheme::list());
	for (TQStringList::iterator it=icons.begin(); it != icons.end(); it++) {
		TDEIconTheme icontheme(*it);
		if (icontheme.isHidden() || !icontheme.isValid()) continue;
		if (*it == "crystalsvg") icon_crystalsvg_exist = true;
		else if (*it == "kdeclassic") icon_kdeclassic_exist = true;
		else if (*it == "Locolor") icon_Locolor_exist = true;
	}
}

/** get the user's former settings */
void TDEStylePage::getUserDefaults() {
	// Get the user's current widget-style
	TDEGlobal::config()->setGroup("General");
	origStyle = TDEGlobal::config()->readEntry( "widgetStyle", TDEStyle::defaultStyle() );

	// get the user's current KWin-style
	ctwin = new TDEConfig("twinrc");
	ctwin->setGroup("Style");
	origKWinStyle = ctwin->readEntry("PluginLib", defaultKWinStyle);

	// get the users current colors
	getColors(&usrColors, false);

	// Get the user's current iconset
	TDEGlobal::config()->setGroup("Icons");
	origIcons = TDEGlobal::config()->readEntry("Theme");

	kdDebug() << "TDEStylePage::getUserDefaults(): style: " << origStyle << endl;
	kdDebug() << "TDEStylePage::getUserDefaults(): KWinStyle: " << origKWinStyle << endl;
	kdDebug() << "TDEStylePage::getUserDefaults(): Colors: " << usrColors.colorFile << endl;
	kdDebug() << "TDEStylePage::getUserDefaults(): Icons: " << origIcons << endl;
}

/** initialize KDE default color values */
void TDEStylePage::initColors() {
	widget.setRgb(239, 239, 239);
	trinity4Blue.setRgb(103,141,178);
        inactiveBackground.setRgb(157,170,186);
        activeBackground.setRgb(65,142,220);
        inactiveForeground.setRgb(221,221,221);
        activeBlend.setRgb(107,145,184);
        activeTitleBtnBg.setRgb(127,158,200);
        inactiveTitleBtnBg.setRgb(167,181,199);
        alternateBackground.setRgb(237,244,249);
        
	if (TQPixmap::defaultDepth() > 8)
		button.setRgb(221, 223, 228);
	else
		button.setRgb(220, 220, 220);

	link.setRgb(0, 0, 238);
	visitedLink.setRgb(82, 24, 139);
}

/** live-update the system */
void TDEStylePage::liveUpdate() {
	// tell all apps about the changed icons
	for (int i=0; i<TDEIcon::LastGroup; i++) {
		KIPC::sendMessageAll(KIPC::IconChanged, i);
	}
	// tell all apps about the changed style
	KIPC::sendMessageAll(KIPC::StyleChanged);
	// color palette changes
	KIPC::sendMessageAll(KIPC::PaletteChanged);
	// twin-style
	kapp->dcopClient()->send("twin*", "", "reconfigure()", TQString(""));
	// kdesktop-background
	kapp->dcopClient()->send("kdesktop", "KBackgroundIface", "configure()", TQString(""));
}

/** show the previewWidget styled with the selected one */
void TDEStylePage::switchPrevStyle() {
	TQStyle* style = TQStyleFactory::create(currentStyle);
	if (!style) return;

	stylePreview->unsetPalette();
	TQPalette palette = createPalette();
	style->polish(palette);
	stylePreview->setPalette(palette);

	// Prevent Qt from wrongly caching radio button images
	TQPixmapCache::clear();
	// go ahead
	setStyleRecursive( stylePreview, palette, style );
	// this flickers, but reliably draws the widgets corretly.
	stylePreview->resize( stylePreview->sizeHint() );

	delete appliedStyle;
	appliedStyle = style;
}

void TDEStylePage::setStyleRecursive(TQWidget* w, TQPalette &palette, TQStyle* s) {
	// Apply the new style.
	w->setStyle(s);
	// Recursively update all children.
	const TQObjectList children = w->childrenListObject();
	if (children.isEmpty())
		return;
	// Apply the style to each child widget.
	TQPtrListIterator<TQObject> childit(children);
	TQObject *child;
	while ((child = childit.current()) != 0) {
		++childit;
		if (child->isWidgetType())
			setStyleRecursive((TQWidget *) child, palette, s);
	}
}

/** create a TQPalette of our current colorset */
TQPalette TDEStylePage::createPalette() {
	colorSet *cc = &currentColors;
	TQColorGroup disabledgrp(cc->windowForeground, cc->background, cc->background.light(150),
		cc->background.dark(), cc->background.dark(120), cc->background.dark(120),
		cc->windowBackground);
	TQColorGroup colgrp(cc->windowForeground, cc->background, cc->background.light(150),
		cc->background.dark(), cc->background.dark(120), cc->foreground,
		cc->windowBackground);
	colgrp.setColor(TQColorGroup::Highlight, cc->selectBackground);
	colgrp.setColor(TQColorGroup::HighlightedText, cc->selectForeground);
	colgrp.setColor(TQColorGroup::Button, cc->buttonBackground);
	colgrp.setColor(TQColorGroup::ButtonText, cc->buttonForeground);
	return TQPalette( colgrp, disabledgrp, colgrp);
}

#include "tdestylepage.moc"
