/* This file is part of the KDE libraries
    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#ifndef _TDEDEBUGDIALOG
#define _TDEDEBUGDIALOG

#include "kabstractdebugdialog.h"

class TQLineEdit;
class TQComboBox;
class TQLabel;
class TQGroupBox;
class TQCheckBox;
class TQPushButton;

class TDEConfig;

/**
 * Control debug/warning/error/fatal output of TDE applications
 *
 * This dialog allows control of debugging output for all TDE apps.
 *
 * @author Kalle Dalheimer (kalle@kde.org)
 */
class TDEDebugDialog : public KAbstractDebugDialog
{
  TQ_OBJECT

public:
  TDEDebugDialog( TQStringList areaList, TQWidget *parent=0, const char *name=0, bool modal=true );
  virtual ~TDEDebugDialog();

  void save();

protected slots:
  void slotDebugAreaChanged( const TQString & );
  void slotDestinationChanged(int);

private:
  TQComboBox* pDebugAreas;
  TQGroupBox* pInfoGroup;
  TQLabel* pInfoLabel1;
  TQComboBox* pInfoCombo;
  TQLabel* pInfoLabel2;
  TQLineEdit* pInfoFile;
  TQLabel* pInfoLabel3;
  TQLineEdit* pInfoShow;
  TQGroupBox* pWarnGroup;
  TQLabel* pWarnLabel1;
  TQComboBox* pWarnCombo;
  TQLabel* pWarnLabel2;
  TQLineEdit* pWarnFile;
  TQLabel* pWarnLabel3;
  TQLineEdit* pWarnShow;
  TQGroupBox* pErrorGroup;
  TQLabel* pErrorLabel1;
  TQComboBox* pErrorCombo;
  TQLabel* pErrorLabel2;
  TQLineEdit* pErrorFile;
  TQLabel* pErrorLabel3;
  TQLineEdit* pErrorShow;
  TQGroupBox* pFatalGroup;
  TQLabel* pFatalLabel1;
  TQComboBox* pFatalCombo;
  TQLabel* pFatalLabel2;
  TQLineEdit* pFatalFile;
  TQLabel* pFatalLabel3;
  TQLineEdit* pFatalShow;

  TQCheckBox* pAbortFatal;

private:
  // Disallow assignment and copy-construction
  TDEDebugDialog( const TDEDebugDialog& );
  TDEDebugDialog& operator= ( const TDEDebugDialog& );
};

#endif
