//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 2010 Timothy Pearson <kb9vqf@pearsoncomputing.net>
//

#ifndef __SAKDLG_H__
#define __SAKDLG_H__

#include <tqthread.h>
#include <tqdialog.h>
#include <tqstringlist.h>

#include <tdeprocess.h>

#include "kgreeter.h"

class TQFrame;
class TQGridLayout;
class TQLabel;
class KPushButton;
class TQListView;
class SAKDlg;
class TDECryptographicCardDevice;

//===========================================================================
//
// Simple dialog for displaying an info message.
// It does not handle password validation.
//
class SAKDlg : public TQDialog
{
	TQ_OBJECT

	public:
		SAKDlg(TQWidget *parent);
		~SAKDlg();
		virtual void show();

		void updateLabel( TQString &txt );
		void closeDialogForced();

	private slots:
		void slotSAKProcessExited();
		void processInputPipeCommand(TQString command);

	protected slots:
		virtual void reject();

	private slots:
		void cryptographicCardInserted(TDECryptographicCardDevice*);
		void cryptographicCardRemoved(TDECryptographicCardDevice*);

	protected:
		bool                      closingDown;

	private:
		TQFrame                   *frame;
		TQGridLayout              *frameLayout;
		TQLabel                   *mStatusLabel;
		int                       mCapsLocked;
		bool                      mUnlockingFailed;
		TQStringList              layoutsList;
		TQStringList::iterator    currLayout;
		int                       sPid, sFd;
		TDEProcess*               mSAKProcess;
		ControlPipeHandlerObject* mControlPipeHandler;
		TQEventLoopThread*        mControlPipeHandlerThread;

	friend class ControlPipeHandlerObject;
};

#endif

