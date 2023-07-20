/*
   kmanualproxydlg.h - Base dialog box for proxy configuration

   Copyright (C) 2001-2004 Dawit Alemayehu <adawit@kde.org>

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

#ifndef KMANUAL_PROXY_DIALOG_H
#define KMANUAL_PROXY_DIALOG_H

#include "kproxydlgbase.h"

class TQSpinBox;
class KLineEdit;
class ManualProxyDlgUI;

class KManualProxyDlg : public KProxyDialogBase
{
  TQ_OBJECT
  
public:
  KManualProxyDlg( TQWidget* parent = 0, const char* name = 0 );
  ~KManualProxyDlg() {};
  
  virtual void setProxyData( const KProxyData &data );
  virtual const KProxyData data() const;
  
protected:
  void init();
  bool validate();
  
protected slots:
  virtual void slotOk();
  
  void copyDown();
  void sameProxy( bool );
  void valueChanged (int value);
  void textChanged (const TQString&);
  
  void newPressed();
  void updateButtons();
  void changePressed();
  void deletePressed();
  void deleteAllPressed();
  
private:
  TQString urlFromInput( const KLineEdit* edit, const TQSpinBox* spin ) const;
  bool isValidURL( const TQString&, KURL* = 0 ) const;
  bool handleDuplicate( const TQString& );
  bool getException ( TQString&, const TQString&,
                      const TQString& value = TQString::null );
  void showErrorMsg( const TQString& caption = TQString::null,
                     const TQString& message = TQString::null );
  
private:
  ManualProxyDlgUI* mDlg;

  int mOldFtpPort;
  int mOldHttpsPort;
  TQString mOldFtpText;
  TQString mOldHttpsText;
};
#endif
