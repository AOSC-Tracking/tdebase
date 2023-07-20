/*  This file is part of the KDE project

    Copyright (C) 2000, 2005 Alexander Neundorf <neundorf@kde.org>

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

#ifndef __SMBRODLG_H
#define __SMBRODLG_H

#include <tqwidget.h>
#include <tqlineedit.h>
#include <tqcheckbox.h>

#include <tdecmodule.h>

class KComboBox;

class SMBRoOptions : public TDECModule
{
   TQ_OBJECT
   public:
      SMBRoOptions(TQWidget *parent = 0);
      ~SMBRoOptions();

      virtual void load();
      virtual void save();
      virtual void defaults();
      TQString quickHelp() const;

   private slots:
      void changed();

   private:
      TQLineEdit *m_userLe;
      TQLineEdit *m_passwordLe;
//      TQLineEdit *m_workgroupLe; //currently unused, Alex
//      TQCheckBox *m_showHiddenShares; //currently unused, Alex
//      KComboBox *m_encodingList; //currently unused
};

#endif
