//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 2010 - 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

#ifndef __QUERYDLG_H__
#define __QUERYDLG_H__

#include <tqdialog.h>
#include <tqstringlist.h>

#include <kpassdlg.h>

#include "lockprocess.h"

class TQFrame;
class TQGridLayout;
class TQLabel;
class KPushButton;
class TQListView;

//===========================================================================
//
// Simple dialog for displaying an query dialog.
// It does not handle password validation.
//
class QueryDlg : public TQDialog
{
	TQ_OBJECT

	public:
		QueryDlg(LockProcess *parent);
		~QueryDlg();
		virtual void show();

		void updateLabel( TQString &txt );
		void setUnlockIcon();
		void setWarningIcon();
		TQString getEntry();

	private slots:
		void slotOK();

	private:
		TQFrame      *frame;
		TQGridLayout *frameLayout;
		TQLabel      *mStatusLabel;
		TQLabel      *mpixLabel;
		int         mCapsLocked;
		bool        mUnlockingFailed;
		TQStringList layoutsList;
		TQStringList::iterator currLayout;
		int         sPid, sFd;
		KPushButton *ok;
		KPasswordEdit *pin_box;
};

#endif

