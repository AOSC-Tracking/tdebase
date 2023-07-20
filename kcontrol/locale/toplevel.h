/*
  toplevel.h - A KControl Application

  written 1998 by Matthias Hoelzer

  Copyright 1998 Matthias Hoelzer.
  Copyright 1999-2003 Hans Petter Bieker <bieker@kde.org>.

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

#ifndef __TOPLEVEL_H__
#define __TOPLEVEL_H__

#include <tdecmodule.h>
#include <kgenericfactory.h>

class TQTabWidget;
class TQGroupBox;

class TDEConfig;
class TDELocale;
class TDELocaleConfig;
class TDELocaleConfigMoney;
class TDELocaleConfigNumber;
class TDELocaleConfigTime;
class TDELocaleConfigOther;
class TDELocaleSample;

class TDELocaleApplication : public TDECModule
{
  TQ_OBJECT

public:
  TDELocaleApplication(TQWidget *parent, const char *name, const TQStringList &);
  virtual ~TDELocaleApplication();

  virtual void load();
  virtual void load(bool useDefault);
  virtual void save();
  virtual void defaults();
  virtual TQString quickHelp() const;
  virtual TQString handbookSection() const;

signals:
  void languageChanged();
  void localeChanged();

public slots:
  /**
   * Retranslates the current widget.
   */
  void slotTranslate();
  void slotChanged();

private:
  TDELocale *m_locale;

  TQTabWidget           *m_tab;
  TDELocaleConfig       *m_localemain;
  TDELocaleConfigNumber *m_localenum;
  TDELocaleConfigMoney  *m_localemon;
  TDELocaleConfigTime   *m_localetime;
  TDELocaleConfigOther  *m_localeother;

  TQGroupBox            *m_gbox;
  TDELocaleSample       *m_sample;

  TDEConfig * m_globalConfig;
  TDEConfig * m_nullConfig;
};

typedef KGenericFactory<TDELocaleApplication, TQWidget > TDELocaleFactory;

#endif
