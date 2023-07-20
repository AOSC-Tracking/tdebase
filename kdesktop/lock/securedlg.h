//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 2010 - 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

#ifndef __SECUREDLG_H__
#define __SECUREDLG_H__

#include <tqdialog.h>
#include <tqstringlist.h>
#include <tqbutton.h>

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
class SecureDlg : public TQDialog
{
	TQ_OBJECT

	public:
		SecureDlg(LockProcess *parent);
		~SecureDlg();
		virtual void show();

		void closeDialogForced();
		void setRetInt(int *);

	private slots:
		void slotBtnCancel();
		void slotBtnLock();
		void slotBtnTask();
		void slotBtnShutdown();
		void slotBtnSwitchUser();

	protected slots:
		virtual void reject();

	private:
		TQFrame      *frame;
		TQGridLayout *frameLayout;
		TQLabel      *mLogonStatus;
		TQButton     *mCancelButton;
		TQButton     *mLockButton;
		TQButton     *mTaskButton;
		TQButton     *mShutdownButton;
		TQButton     *mSwitchButton;
		int         mCapsLocked;
		bool        mUnlockingFailed;
		TQStringList layoutsList;
		TQStringList::iterator currLayout;
		int         sPid, sFd;
		int*        retInt;
};

#endif

