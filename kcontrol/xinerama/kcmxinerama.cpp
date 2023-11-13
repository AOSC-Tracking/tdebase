/**
 * kcmxinerama.cpp
 *
 * Copyright (c) 2002-2004 George Staikos <staikos@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "kcmxinerama.h"
#include <dcopclient.h>
#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <twin.h>

#include <tqcheckbox.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqcombobox.h>
#include <tqtable.h>
#include <tqcolor.h>
#include <tqpushbutton.h>


KCMXinerama::KCMXinerama(TQWidget *parent, const char *name)
  : TDECModule(parent, name) {
	_indicators.setAutoDelete(true);

	TDEAboutData *about =
	new TDEAboutData(I18N_NOOP("kcmxinerama"),
			I18N_NOOP("TDE Multiple Monitor Configurator"),
			0, 0, TDEAboutData::License_GPL,
			I18N_NOOP("(c) 2002-2003 George Staikos"));
 
	about->addAuthor("George Staikos", 0, "staikos@kde.org");
	setAboutData( about );

	setQuickHelp( i18n("<h1>Multiple Monitors</h1> This module allows you to configure TDE support"
     " for multiple monitors."));

	config = new TDEConfig("kdeglobals", false, false);
	ksplashrc = new TDEConfig("ksplashrc", false, false);

	connect(&_timer, TQT_SIGNAL(timeout()), this, TQT_SLOT(clearIndicator()));

	TQGridLayout *grid = new TQGridLayout(this, 1, 1, KDialog::marginHint(),
							KDialog::spacingHint());

	// Setup the panel
	_displays = TQApplication::desktop()->numScreens();

	if (TQApplication::desktop()->isVirtualDesktop()) {
		TQStringList dpyList;
		xw = new XineramaWidget(this);
		grid->addWidget(xw, 0, 0);
		TQString label = i18n("Display %1");

		xw->headTable->setNumRows(_displays);

		for (int i = 0; i < _displays; i++) {
			TQString l = label.arg(i+1);
			TQRect geom = TQApplication::desktop()->screenGeometry(i);
			xw->_unmanagedDisplay->insertItem(l);
			xw->_ksplashDisplay->insertItem(l);
			dpyList.append(l);
			xw->headTable->setText(i, 0, TQString::number(geom.x()));
			xw->headTable->setText(i, 1, TQString::number(geom.y()));
			xw->headTable->setText(i, 2, TQString::number(geom.width()));
			xw->headTable->setText(i, 3, TQString::number(geom.height()));
		}

		xw->_unmanagedDisplay->insertItem(i18n("Display Containing the Pointer"));

		xw->headTable->setRowLabels(dpyList);

		connect(xw->_ksplashDisplay, TQT_SIGNAL(activated(int)),
			this, TQT_SLOT(windowIndicator(int)));
		connect(xw->_unmanagedDisplay, TQT_SIGNAL(activated(int)),
			this, TQT_SLOT(windowIndicator(int)));
		connect(xw->_identify, TQT_SIGNAL(clicked()),
			this, TQT_SLOT(indicateWindows()));

		connect(xw, TQT_SIGNAL(configChanged()), this, TQT_SLOT(changed()));
	} else { // no Xinerama
		TQLabel *ql = new TQLabel(i18n("<qt><p>This module is only for configuring systems with a single desktop spread across multiple monitors. You do not appear to have this configuration.</p></qt>"), this);
		grid->addWidget(ql, 0, 0);
	}

	grid->activate();

	load();
}

KCMXinerama::~KCMXinerama() {
	_timer.stop();
	delete ksplashrc;
	ksplashrc = 0;
	delete config;
	config = 0;
	clearIndicator();
}

#define KWIN_XINERAMA              "XineramaEnabled"
#define KWIN_XINERAMA_MOVEMENT     "XineramaMovementEnabled"
#define KWIN_XINERAMA_PLACEMENT    "XineramaPlacementEnabled"
#define KWIN_XINERAMA_MAXIMIZE     "XineramaMaximizeEnabled"
#define KWIN_XINERAMA_FULLSCREEN   "XineramaFullscreenEnabled"

void KCMXinerama::load() {
   load( false );
}

void KCMXinerama::load(bool useDefaults) {
	if (TQApplication::desktop()->isVirtualDesktop()) {
		int item = 0;
		config->setReadDefaults( useDefaults );
		config->setGroup("Windows");
		xw->_enableXinerama->setChecked(config->readBoolEntry(KWIN_XINERAMA, true));
		xw->_enableResistance->setChecked(config->readBoolEntry(KWIN_XINERAMA_MOVEMENT, true));
		xw->_enablePlacement->setChecked(config->readBoolEntry(KWIN_XINERAMA_PLACEMENT, true));
		xw->_enableMaximize->setChecked(config->readBoolEntry(KWIN_XINERAMA_MAXIMIZE, true));
		xw->_enableFullscreen->setChecked(config->readBoolEntry(KWIN_XINERAMA_FULLSCREEN, true));
		item = config->readNumEntry("Unmanaged", TQApplication::desktop()->primaryScreen());
		if ((item < 0 || item >= _displays) && (item != -3))
			xw->_unmanagedDisplay->setCurrentItem(TQApplication::desktop()->primaryScreen());
		else if (item == -3) // pointer warp
			xw->_unmanagedDisplay->setCurrentItem(_displays);
		else	xw->_unmanagedDisplay->setCurrentItem(item);

		ksplashrc->setGroup("Xinerama");
		item = ksplashrc->readNumEntry("KSplashScreen", TQApplication::desktop()->primaryScreen());
		if (item < 0 || item >= _displays)
			xw->_ksplashDisplay->setCurrentItem(TQApplication::desktop()->primaryScreen());
		else xw->_ksplashDisplay->setCurrentItem(item);
		
		emit changed(useDefaults);
	}
	else
		emit changed( false );
}


void KCMXinerama::save() {
	if (TQApplication::desktop()->isVirtualDesktop()) {
		config->setGroup("Windows");
		config->writeEntry(KWIN_XINERAMA,
					xw->_enableXinerama->isChecked());
		config->writeEntry(KWIN_XINERAMA_MOVEMENT,
					xw->_enableResistance->isChecked());
		config->writeEntry(KWIN_XINERAMA_PLACEMENT,
					xw->_enablePlacement->isChecked());
		config->writeEntry(KWIN_XINERAMA_MAXIMIZE,
					xw->_enableMaximize->isChecked());
		config->writeEntry(KWIN_XINERAMA_FULLSCREEN,
					xw->_enableFullscreen->isChecked());
		int item = xw->_unmanagedDisplay->currentItem();
		config->writeEntry("Unmanaged", item == _displays ? -3 : item);
		config->sync();

		if (!kapp->dcopClient()->isAttached())
			kapp->dcopClient()->attach();
		kapp->dcopClient()->send("twin", "", "reconfigure()", TQString(""));

		ksplashrc->setGroup("Xinerama");
		ksplashrc->writeEntry("KSplashScreen", xw->_enableXinerama->isChecked() ? xw->_ksplashDisplay->currentItem() : -2 /* ignore Xinerama */);
		ksplashrc->sync();
	}

	KMessageBox::information(this, i18n("Your settings will only affect newly started applications."), i18n("TDE Multiple Monitors"), "nomorexineramaplease");

	emit changed(false);
}

void KCMXinerama::defaults() {
	load( true );
}

void KCMXinerama::indicateWindows() {
	_timer.stop();

	clearIndicator();
	for (int i = 0; i < _displays; i++)
		_indicators.append(indicator(i));

	_timer.start(1500, true);
}

void KCMXinerama::windowIndicator(int dpy) {
	if (dpy >= _displays)
		return;

	_timer.stop();

	clearIndicator();
	_indicators.append(indicator(dpy));

	_timer.start(1500, true);
}

TQWidget *KCMXinerama::indicator(int dpy) {
	TQLabel *si = new TQLabel(TQString::number(dpy+1), 0, "Screen Indicator", (WFlags)WX11BypassWM );

	TQFont fnt = TDEGlobalSettings::generalFont();
	fnt.setPixelSize(100);
	si->setFont(fnt);
	si->setFrameStyle(TQFrame::Panel);
	si->setFrameShadow(TQFrame::Plain);
	si->setAlignment(TQt::AlignCenter);

	TQPoint screenCenter(TQApplication::desktop()->screenGeometry(dpy).center());
	TQRect targetGeometry(TQPoint(0,0), si->sizeHint());
        targetGeometry.moveCenter(screenCenter);
	si->setGeometry(targetGeometry);
	si->show();

	return si;
}

void KCMXinerama::clearIndicator() {
	_indicators.clear();
}

extern "C" {
        KDE_EXPORT TDECModule *create_xinerama(TQWidget *parent, const char *name) {
   	    TDEGlobal::locale()->insertCatalogue("kcmxinerama");
	    return new KCMXinerama(parent, name);
        }

	KDE_EXPORT bool test_xinerama() {
		return TQApplication::desktop()->isVirtualDesktop();
	}
}


#include "kcmxinerama.moc"

