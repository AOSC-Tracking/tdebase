/*
 * mouse.h
 *
 * Copyright (c) 1998 Matthias Ettrich <ettrich@kde.org>
 *
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __KKWMMOUSECONFIG_H__
#define __KKWMMOUSECONFIG_H__

class TQCheckBox;
class TDEConfig;

#include <tqwidget.h>
#include <tdecmodule.h>
#include <tqcombobox.h>
#include <tqtooltip.h>


class ToolTipComboBox: public TQComboBox
{
  TQ_OBJECT

public:
  ToolTipComboBox(TQWidget * owner, char const * const * toolTips_)
    : TQComboBox(owner)
    , toolTips(toolTips_) {}

public slots:
  void changed() {TQToolTip::add( this, i18n(toolTips[currentItem()]) );}

protected:
  char const * const * toolTips;
};



class KTitleBarActionsConfig : public TDECModule
{
  TQ_OBJECT

public:

  KTitleBarActionsConfig( bool _standAlone, TDEConfig *_config, TQWidget *parent=0, const char* name=0 );
  ~KTitleBarActionsConfig( );

  void load();
  void save();
  void defaults();

public slots:
  void changed() { emit TDECModule::changed(true); }

private:
  TQComboBox* coTiDbl;
  TQComboBox* coTiAct1;
  TQComboBox* coTiAct2;
  TQComboBox* coTiAct3;
  TQComboBox* coTiInAct1;
  TQComboBox* coTiInAct2;
  TQComboBox* coTiInAct3;
  TQComboBox* coTiW;
  TQCheckBox* cbTiRevW;

  ToolTipComboBox * coMax[3];

  TDEConfig *config;
  bool standAlone;

  const char* functionTiDbl(int);
  const char* functionTiAc(int);
  const char* functionTiWAc(int);
  const char* functionTiInAc(int);
  const char* functionMax(int);

  void setComboText(TQComboBox* combo, const char* text);
  const char* fixup( const char* s );

private slots:
  void paletteChanged();

};

class KWindowActionsConfig : public TDECModule
{
  TQ_OBJECT

public:

  KWindowActionsConfig( bool _standAlone, TDEConfig *_config, TQWidget *parent=0, const char* name=0 );
  ~KWindowActionsConfig( );

  void load();
  void save();
  void defaults();

public slots:
  void changed() { emit TDECModule::changed(true); }

private:
  TQComboBox* coWin1;
  TQComboBox* coWin2;
  TQComboBox* coWin3;

  TQComboBox* coAllKey;
  TQComboBox* coAll1;
  TQComboBox* coAll2;
  TQComboBox* coAll3;
  TQComboBox* coAllW;
  TQCheckBox* cbAllRevW;

  TDEConfig *config;
  bool standAlone;

  const char* functionWin(int);
  const char* functionAllKey(int);
  const char* functionAll(int);
  const char* functionAllW(int);

  void setComboText(TQComboBox* combo, const char* text);
  const char* fixup( const char* s );
};

#endif

