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

#include <tqhbox.h>
#include <tqlayout.h>
#include <tqlabel.h>

#include <klocale.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>

#include "kshellcmddialog.h"
#include "kshellcmdexecutor.h"

KShellCommandDialog::KShellCommandDialog(const TQString& title, const TQString& command, TQWidget* parent, bool modal)
   :KDialog(parent,"p",modal)
{
   TQVBoxLayout * box=new TQVBoxLayout (this,marginHint(),spacingHint());

   TQLabel *label=new TQLabel(title,this);
   m_shell=new KShellCommandExecutor(command,this);

   TQHBox *buttonsBox=new TQHBox(this);
   buttonsBox->setSpacing(spacingHint());

   cancelButton= new KPushButton(KStdGuiItem::cancel(), buttonsBox);
   closeButton= new KPushButton(KStdGuiItem::close(), buttonsBox);
   closeButton->setDefault(true);

   label->resize(label->sizeHint());
   m_shell->resize(m_shell->sizeHint());
   closeButton->setFixedSize(closeButton->sizeHint());
   cancelButton->setFixedSize(cancelButton->sizeHint());

   box->addWidget(label,0);
   box->addWidget(m_shell,1);
   box->addWidget(buttonsBox,0);

   m_shell->setFocus();

   connect(cancelButton, TQT_SIGNAL(clicked()), m_shell, TQT_SLOT(slotFinished()));
   connect(m_shell, TQT_SIGNAL(finished()), this, TQT_SLOT(disableStopButton()));
   connect(closeButton,TQT_SIGNAL(clicked()), this, TQT_SLOT(slotClose()));
}

KShellCommandDialog::~KShellCommandDialog()
{
   delete m_shell;
   m_shell=0;
}

void KShellCommandDialog::disableStopButton()
{
   cancelButton->setEnabled(false);
}

void KShellCommandDialog::slotClose()
{
   delete m_shell;
   m_shell=0;
   accept();
}

//blocking
int KShellCommandDialog::executeCommand()
{
   if (m_shell==0)
      return 0;
   //kdDebug()<<"---------- KShellCommandDialog::executeCommand()"<<endl;
   m_shell->exec();
   return exec();
}

#include "kshellcmddialog.moc"
