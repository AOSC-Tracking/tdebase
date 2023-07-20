/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

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

#ifndef SHELLCOMMANDDIALOG_H
#define SHELLCOMMANDDIALOG_H

#include <tqstring.h>

#include <kpushbutton.h>
#include <kdialog.h>
class TQPushButton;
class KShellCommandExecutor;

class KShellCommandDialog:public KDialog
{
   TQ_OBJECT
   public:
      KShellCommandDialog(const TQString& title, const TQString& command, TQWidget* parent=0, bool modal=false);
      virtual ~KShellCommandDialog();
      //blocking
      int executeCommand();
   protected:

      KShellCommandExecutor *m_shell;
      KPushButton *cancelButton;
      KPushButton *closeButton;
   protected slots:
      void disableStopButton();
      void slotClose();
};

#endif
