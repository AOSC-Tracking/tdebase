//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 2010 - 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

#ifndef __INFODLG_H__
#define __INFODLG_H__

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
class InfoDlg : public TQDialog
{
	TQ_OBJECT

	public:
		InfoDlg(LockProcess *parent);
		~InfoDlg();
		virtual void show();

		void updateLabel( TQString &txt );
		void setUnlockIcon();
		void setKDEIcon();
		void setInfoIcon();
		void setWarningIcon();
		void setErrorIcon();

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
};

#endif

