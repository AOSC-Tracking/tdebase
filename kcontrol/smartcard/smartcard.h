/**
 * smartcard.h
 *
 * Copyright (c) 2001 George Staikos <staikos@kde.org>
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

#ifndef _KCM_SMARTCARD_H
#define _KCM_SMARTCARD_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dcopobject.h>

#include <tdecmodule.h>

#include "smartcardbase.h"
#include "nosmartcardbase.h"

class TDEConfig;
class KCardDB;
class TDEPopupMenu;
class TDEListViewItem;

class KSmartcardConfig : public TDECModule, public DCOPObject
{
  K_DCOP
    TQ_OBJECT


public:
  KSmartcardConfig(TQWidget *parent = 0L, const char *name = 0L);
  virtual ~KSmartcardConfig();

  SmartcardBase *base;

  void load();
  void load( bool useDefaults);
  void save();
  void defaults();

  int buttons();
  TQString quickHelp() const;

 k_dcop:


 void updateReadersState (TQString readerName,
                          bool isCardPresent,
                          TQString atr);
 void loadReadersTab (TQStringList lr);

  private slots:

  void slotShowPopup(TQListViewItem * item ,const TQPoint & _point,int i);
  void slotLaunchChooser();



private:

  TDEConfig *config;
  bool _ok;
  KCardDB * _cardDB;
  TDEPopupMenu * _popUpKardChooser;
  
  void getSupportingModule( TDEListViewItem * ant,
                            TQString & cardATR) const ;


};

#endif

