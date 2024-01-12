/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2003 Daniel Molkentin <molkentin@kde.org>

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

#include <tqheader.h>
#include <tqcursor.h>

#include <tdelocale.h>
#include <kstandarddirs.h>
#include <kservicegroup.h>
#include <kiconloader.h>
#include <tdemessagebox.h>

#include <kdebug.h>

#include "deviceiconview.h"
#include "deviceiconview.moc"

DeviceIconView::DeviceIconView(TQWidget * parent, const char * name)
	: TDEListView(parent, name)
{
	setSorting(0, true);
	addColumn(TQString::null);
	
	// Show expand/collapse widgets on root items
	setRootIsDecorated(true);
	
	header()->hide();
	
	connect(this, TQ_SIGNAL(clicked(TQListViewItem*)), this, TQ_SLOT(slotItemSelected(TQListViewItem*)));
	connect(this, TQ_SIGNAL(executed(TQListViewItem*)), this, TQ_SLOT(slotItemDoubleClicked(TQListViewItem*)));
}

void DeviceIconView::slotItemSelected(TQListViewItem* item)
{
	TQApplication::restoreOverrideCursor();
	if (!item) {
		return;
	}
}

void DeviceIconView::slotItemDoubleClicked(TQListViewItem* item)
{
	TQApplication::restoreOverrideCursor();
	if (!item) {
		return;
	}

	DeviceIconItem* divi = dynamic_cast<DeviceIconItem*>(item);
	if (!divi) {
		return;
	}

	if (divi->device()) {
		DevicePropertiesDialog* propsDlg = new DevicePropertiesDialog(divi->device(), this);
		propsDlg->exec();
		delete propsDlg;
	}
	else {
		KMessageBox::sorry(this, i18n("Detailed information is not available for this device"),
		                   i18n("Information Unavailable"));
	}
}

void DeviceIconView::keyPressEvent(TQKeyEvent *e)
{
	if (e->key() == Key_Return
		|| e->key() == Key_Enter
		|| e->key() == Key_Space)
	{
		if (currentItem()) {
			slotItemSelected(currentItem());
		}
	}
	else {
		TDEListView::keyPressEvent(e);
	}
}

TQPixmap DeviceIconView::loadIcon( const TQString &name )
{
	TQPixmap icon = DesktopIcon( name, iconSize() );
	
	if(icon.isNull()) {
		icon = DesktopIcon( "misc", iconSize() );
	}
	
	return icon;
}

TDEIcon::StdSizes DeviceIconView::iconSize() {
	return TDEIcon::SizeSmall;
}
