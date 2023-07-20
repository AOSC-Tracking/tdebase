/**
 * iccconfig.h
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

#ifndef _KCM_ICCCONFIG_H
#define _KCM_ICCCONFIG_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dcopobject.h>

#include <libtderandr/libtderandr.h>

#include "iccconfigbase.h"

class TDEConfig;
class TDEPopupMenu;
class TDEListViewItem;

class KICCConfig : public TDECModule, public DCOPObject
{
  K_DCOP
    TQ_OBJECT


public:
  //KICCConfig(TQWidget *parent = 0L, const char *name = 0L);
  KICCConfig(TQWidget *parent, const char *name, const TQStringList &);
  virtual ~KICCConfig();

  ICCConfigBase *base;

  void load();
  void load( bool useDefaults);
  void save();
  void defaults();

  TQString quickHelp() const;

 k_dcop:

private:

  TDEConfig *config;
  bool _ok;
  Display *randr_display;
  ScreenInfo *randr_screen_info;
  int numberOfProfiles;
  int numberOfScreens;
  TQStringList cfgScreenInfo;
  TQStringList cfgProfiles;
  void updateDisplayedInformation ();
  TQString extractFileName(TQString displayName, TQString profileName);
  TQString *iccFileArray;
  int findProfileIndex(TQString profileName);
  int findScreenIndex(TQString screenName);
  TQString m_defaultProfile;

private slots:
  void selectProfile (int slotNumber);
  void selectScreen (int slotNumber);
  void updateArray (void);
  void addProfile (void);
  void renameProfile (void);
  void deleteProfile (void);
};

#endif

