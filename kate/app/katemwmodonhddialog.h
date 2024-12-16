/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#ifndef _KATE_MW_MODONHD_DIALOG_H_
#define _KATE_MW_MODONHD_DIALOG_H_

#include <kdialogbase.h>
#include <tqptrvector.h>
#include <kate/document.h>

typedef  TQPtrVector<Kate::Document> DocVector;
class TDEProcIO;
class TDEProcess;
/**
 * A dialog for handling multiple documents modified on disk
 * from within KateMainWindow
 */
class KateMwModOnHdDialog : public KDialogBase
{
  TQ_OBJECT
  public:
    KateMwModOnHdDialog( DocVector docs, TQWidget *parent=0, const char *name=0 );
    ~KateMwModOnHdDialog();

  protected slots:
    void slotUser1();
    void slotUser2();
    void slotUser3();

  private slots:
    void slotDiff();
    void slotSelectionChanged();
    void slotPRead(TDEProcIO*);
    void slotPDone(TDEProcess*);

  private:
    enum Action { Ignore, Overwrite, Reload };
    void handleSelected( int action );
    class TDEListView *lvDocuments;
    class TQPushButton *btnDiff;
    class KTempFile *m_tmpfile;
};

#endif // _KATE_MW_MODONHD_DIALOG_H_
