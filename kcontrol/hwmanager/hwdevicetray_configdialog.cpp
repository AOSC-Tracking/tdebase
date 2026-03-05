/*
 * Copyright 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 * 
 * This file is part of hwdevicetray, the TDE Hardware Device Monitor System Tray Application
 * 
 * hwdevicetray is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * hwdevicetray is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with cryptocardwatcher. If not, see http://www.gnu.org/licenses/.
 */

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqpushbutton.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>
#include <tqvbuttongroup.h>
#include <assert.h>

#include <kiconloader.h>
#include <tdelocale.h>
#include <tdepopupmenu.h>
#include <twinmodule.h>
#include <kregexpeditorinterface.h>
#include <tdeparts/componentfactory.h>

#include "hwdevicetray_configdialog.h"

ConfigDialog::ConfigDialog(TDEGlobalAccel *accel, bool isApplet)
	: KDialogBase(Tabbed, i18n("Configure"), Ok | Cancel | Help, Ok, 0L, "config dialog") {
	if (isApplet) {
		setHelp(TQString::null, "tdehwdevicetray");
	}
	
	TQFrame *w = 0L; // the parent for the widgets
	
	w = addVBoxPage(i18n("Global &Shortcuts"));
	keysWidget = new KKeyChooser(accel, w);
}


ConfigDialog::~ConfigDialog() {
}

// prevent huge size due to long regexps in the action-widget
void ConfigDialog::show() {
	if (!isVisible()) {
		TWinModule module(0, TWinModule::INFO_DESKTOP);
		TQSize s1 = sizeHint();
		TQSize s2 = module.workArea().size();
		int w = s1.width();
		int h = s1.height();
	
		if (s1.width() >= s2.width())
		w = s2.width();
		if (s1.height() >= s2.height())
		h = s2.height();
	
		resize(w, h);
	}
	
	KDialogBase::show();
}

void ConfigDialog::commitShortcuts() {
	keysWidget->commitChanges();
}

#include "hwdevicetray_configdialog.moc"
