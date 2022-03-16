/**
 * hwmanager.h
 *
 * Copyright (c) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>
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

#ifndef _KCM_HWMANAGER_H
#define _KCM_HWMANAGER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tdecmodule.h>
#include <tdelistviewsearchline.h>

#include <dcopobject.h>

#include "hwmanagerbase.h"
#include "devicepropsdlg.h"
#include "deviceiconview.h"

class TDEConfig;
class TDEPopupMenu;
class TDEListViewItem;

class TDEHWManager : public TDECModule, public DCOPObject
{
	K_DCOP
	Q_OBJECT

public:
	//TDEHWManager(TQWidget *parent = 0L, const char *name = 0L);
	TDEHWManager(TQWidget *parent, const char *name, const TQStringList &);
	virtual ~TDEHWManager();
	
	void load();
	void load( bool useDefaults);
	void save();
	void defaults();

	TQString quickHelp() const;
	
k_dcop:

private slots:
	void populateTreeView();
	void populateTreeViewLeaf(DeviceIconItem *parent, bool show_by_connection, TQString selected_syspath);
	void deviceChanged(TDEGenericDevice*);
	void cryptographicCardPinRequested(TQString prompt, TDECryptographicCardDevice* cdevice);

private:
	TDEHWManagerBase *base;

	TDEConfig *config;
};

#endif

