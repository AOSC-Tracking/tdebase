//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 2010 - 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

#ifndef __SAKDLG_H__
#define __SAKDLG_H__

#include <tqdialog.h>
#include <tqstringlist.h>

#include "lockprocess.h"

class TQFrame;
class TQGridLayout;
class TQLabel;
class KPushButton;
class TQListView;

//===========================================================================
//
// Simple dialog for displaying an info message.
// It does not handle password validation.
//
class SAKDlg : public TQDialog
{
	TQ_OBJECT

	public:
		SAKDlg(LockProcess *parent);
		~SAKDlg();
		virtual void show();

		void updateLabel( TQString &txt );
		void closeDialogForced();

	private slots:
		void slotSAKProcessExited();

	protected slots:
		virtual void reject();

	private:
		TQFrame      *frame;
		TQGridLayout *frameLayout;
		TQLabel      *mStatusLabel;
		int         mCapsLocked;
		bool        mUnlockingFailed;
		TQStringList layoutsList;
		TQStringList::iterator currLayout;
		int         sPid, sFd;
		TDEProcess*   mSAKProcess;
};

#endif

