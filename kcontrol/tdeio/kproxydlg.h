/*
   kproxydlg.h - Proxy configuration dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KPROXYDIALOG_H
#define _KPROXYDIALOG_H

#include <tqstring.h>

#include <tdecmodule.h>

class TQTabWidget;

class KProxyData;
class KProxyDialogUI;

class KProxyOptions : public TDECModule
{
  TQ_OBJECT

public:
  KProxyOptions( TQWidget* parent = 0 );
  ~KProxyOptions();

  virtual void load();
  virtual void save();
  virtual void defaults();
  virtual TQString quickHelp() const;

  virtual TQString handbookSection() const;

private:
  TDECModule* mProxy;
  TDECModule* mSocks;
  TQTabWidget* mTab;
};

class KProxyDialog : public TDECModule
{
  TQ_OBJECT

public:
  KProxyDialog( TQWidget* parent = 0 );
  ~KProxyDialog();

  virtual void load();
  virtual void save();
  virtual void defaults();
  TQString quickHelp() const;

private slots:
  void slotChanged();
  void slotUseProxyChanged();

  void setupManProxy();
  void setupEnvProxy();

private:
  void showInvalidMessage( const TQString& _msg = TQString::null );

private:
  KProxyDialogUI* mDlg;
  KProxyData* mData;
  bool mDefaultData;
};

#endif
