/**
 * displayconfig.h
 *
 * Copyright (c) 2009-2010 Timothy Pearson <kb9vqf@pearsoncomputing.net>
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

#ifndef _KCM_DisplayCONFIG_H
#define _KCM_DisplayCONFIG_H

#include <tqptrlist.h>
#include <tqslider.h>
#include <tqworkspace.h>
#include <tqobjectlist.h>
#include <tqwidgetlist.h>
#include <tqspinbox.h>

#include <dcopobject.h>

#include <libtderandr/libtderandr.h>
#ifdef WITH_TDEHWLIB
#include <tdehardwaredevices.h>
#else
#define TDEGenericDevice void
#endif

#include "monitorworkspace.h"
#include "displayconfigbase.h"

class TDEConfig;
class TDEPopupMenu;
class TDEListViewItem;

typedef TQMap< TQString, TQPtrList< SingleScreenData > > ScreenConfigurationMap;

class KDisplayConfig : public TDECModule, public DCOPObject
{
  K_DCOP
  Q_OBJECT


public:
	//KDisplayConfig(TQWidget *parent = 0L, const char *name = 0L);
	KDisplayConfig(TQWidget *parent, const char *name, const TQStringList &);
	virtual ~KDisplayConfig();

	DisplayConfigBase *base;

	void load();
	void load( bool useDefaults);
	void save();
	void defaults();
  virtual TQString handbookSection() const;

	TQString quickHelp() const;

k_dcop:

private:

	TDEConfig *config;
	TDECModule *iccTab;
	bool _ok;
	Display *randr_display;
	ScreenInfo *randr_screen_info;
	int numberOfProfiles;
	int numberOfScreens;
	TQStringList cfgScreenInfo;
	TQStringList cfgProfiles;
	void refreshDisplayedInformation ();
	void updateDisplayedInformation ();
	TQString *displayFileArray;
	int findProfileIndex(TQString profileName);
	TQString m_defaultProfile;
	KRandrSimpleAPI *m_randrsimple;
	ScreenConfigurationMap m_screenInfoArray;
	TQPtrList<SingleScreenData> m_hardwareScreenInfoArray;
	TQString activeProfileName;
	TQString startupProfileName;
	int realResolutionSliderValue();
	void setRealResolutionSliderValue(int index);
	TDECModule* addTab( const TQString name, const TQString label );
	void moveMonitor(DraggableMonitor* monitor, int realx, int realy);
	bool applyMonitorLayoutRules(void);
	bool applyMonitorLayoutRules(DraggableMonitor* primary_monitor);
	void updateDraggableMonitorInformationInternal (int, bool);
	TQTimer* m_gammaApplyTimer;
	void gammaSetAverageAllSlider();
	void setGammaLabels();
	void generateSortedResolutions();
	void loadProfileFromDiskHelper(bool forceReload = false);
	void saveActiveSystemWideProfileToDisk();
	void createHotplugRulesGrid();
	TQGridLayout* profileRulesGrid;
	TQStringList availableProfileNames;
	void profileListChanged();
	HotPlugRulesList currentHotplugRules;
	void updateProfileConfigObjectFromGrid();

private slots:
	void selectProfile (int slotNumber);
	void selectScreen (int slotNumber);
	void resolutionSliderChanged(int index);
	void resolutionSliderTextUpdate(int index);
	void updateArray (void);
	void addProfile (void);
	void renameProfile (void);
	void deleteProfile (void);
	void activateProfile (void);
	void reloadProfileFromDisk (void);
	void saveProfile (void);
	void ensurePrimaryMonitorIsAvailable (void);
	void updateDragDropDisplay (void);
	void layoutDragDropDisplay (void);
	void ensureMonitorDataConsistency (void);
	void updateDraggableMonitorInformation (int);
	void updateExtendedMonitorInformation (void);
	void processLockoutControls (void);
	void rotationInfoChanged (void);
	void refreshInfoChanged (void);
	void activatePreview (void);
	void identifyMonitors (void);
	void rescanHardware (void);
	void reloadProfile (void);
	void gammaAllSliderChanged(int index);
	void gammaRedSliderChanged(int index);
	void gammaGreenSliderChanged(int index);
	void gammaBlueSliderChanged(int index);
	void applyGamma (void);
	void gammaselectScreen (int slotNumber);
	void gammaTargetChanged (int slotNumber);
	void dpmsChanged (void);
	void processDPMSControls (void);
	void deviceChanged (TDEGenericDevice*);
	void updateStartupProfileLabel (void);
	void selectDefaultProfile (int index);
	void addNewProfileRule (void);
	void deleteProfileRule (void);
	void profileRuleCheckBoxStateChanged (int state);
};

#endif

