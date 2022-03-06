/*
  Copyright (c) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 
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

#ifndef __deviceiconview_h__
#define __deviceiconview_h__

#include <tdelistview.h>
#include <kiconloader.h>
#include <tdehardwaredevices.h>

#include "devicepropsdlg.h"

class ConfigModule;
class ConfigModuleList;

class DeviceIconItem : public TDEListViewItem
{
public:
	DeviceIconItem(TQListViewItem *parent, const TQString& text, const TQPixmap& pm, TQString deviceUid)
		: TDEListViewItem(parent, text)
		, _deviceUid(deviceUid)
		{
			setPixmap(0, pm);
		}
	DeviceIconItem(TQListView *parent, const TQString& text, const TQPixmap& pm, TQString deviceUid)
		: TDEListViewItem(parent, text)
		, _deviceUid(deviceUid)
		{
			setPixmap(0, pm);
		}
		
		void setDevice(TQString deviceUid) { _deviceUid = deviceUid; }
		
		TDEGenericDevice* device() { return TDEGlobal::hardwareDevices()->findByUniqueID(_deviceUid); }
	
private:
		TQString _deviceUid;  // device unique id
};

class DeviceIconView : public TDEListView
{
	Q_OBJECT

public:
	DeviceIconView(TQWidget * parent = 0, const char * name = 0);
	TDEIcon::StdSizes iconSize();

signals:
	void deviceSelected(TDEGenericDevice*);

protected slots:
	void slotItemSelected(TQListViewItem*);
	void slotItemDoubleClicked(TQListViewItem*);

protected:
	void keyPressEvent(TQKeyEvent *);
	TQPixmap loadIcon( const TQString &name );
};

#endif
