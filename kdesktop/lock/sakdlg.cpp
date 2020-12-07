//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 2010 - 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>

#include <config.h>

#include "sakdlg.h"

#include <dmctl.h>

#include <tdeapplication.h>
#include <tdelocale.h>
#include <kpushbutton.h>
#include <kseparator.h>
#include <kstandarddirs.h>
#include <tdeglobalsettings.h>
#include <tdeconfig.h>
#include <kiconloader.h>
#include <tdesu/defaults.h>
#include <kpassdlg.h>
#include <kdebug.h>
#include <kuser.h>
#include <dcopref.h>
#include <tdemessagebox.h>
#include <kdialog.h>

#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqmessagebox.h>
#include <tqsimplerichtext.h>
#include <tqlabel.h>
#include <tqstringlist.h>
#include <tqfontmetrics.h>
#include <tqstyle.h>
#include <tqapplication.h>
#include <tqlistview.h>
#include <tqheader.h>
#include <tqcheckbox.h>

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <fixx11h.h>

#ifndef AF_LOCAL
# define AF_LOCAL	AF_UNIX
#endif

extern bool trinity_desktop_lock_use_system_modal_dialogs;
extern bool trinity_desktop_lock_use_sak;

//===========================================================================
//
// Simple dialog for displaying an unlock status or recurring error message
//
SAKDlg::SAKDlg(LockProcess *parent)
	: TQDialog(parent, "information dialog", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM))),
	mUnlockingFailed(false)
{
	if (trinity_desktop_lock_use_system_modal_dialogs) {
		// Signal that we do not want any window controls to be shown at all
		Atom kde_wm_system_modal_notification;
		kde_wm_system_modal_notification = XInternAtom(tqt_xdisplay(), "_TDE_WM_MODAL_SYS_NOTIFICATION", False);
		XChangeProperty(tqt_xdisplay(), winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
	}
	setCaption(i18n("Desktop Session Locked"));

	frame = new TQFrame( this );
	if (trinity_desktop_lock_use_system_modal_dialogs) {
		frame->setFrameStyle( TQFrame::NoFrame );
	}
	else {
		frame->setFrameStyle( TQFrame::Panel | TQFrame::Raised );
	}
	frame->setLineWidth( 2 );

	KSMModalDialogHeader* theader = new KSMModalDialogHeader( frame );

	KUser user;

	mStatusLabel = new TQLabel( "<b> </b>", frame );
	mStatusLabel->setAlignment( TQLabel::AlignVCenter );

	TQVBoxLayout *unlockDialogLayout = new TQVBoxLayout( this );
	unlockDialogLayout->addWidget( frame );

	TQHBoxLayout *layStatus = new TQHBoxLayout( 0, 0, KDialog::spacingHint());
	layStatus->addWidget( mStatusLabel );

	frameLayout = new TQGridLayout( frame, 1, 1, KDialog::marginHint(), KDialog::spacingHint() );
	frameLayout->addMultiCellWidget( theader, 0, 0, 0, 1, AlignTop | AlignLeft );
	frameLayout->addMultiCellLayout( layStatus, 1, 1, 0, 1, AlignLeft | AlignVCenter);

	mStatusLabel->setText("<b>" + i18n("Press Ctrl+Alt+Del to begin.") + "</b><p>" + i18n("This process helps keep your password secure.") + "<br>" + i18n("It prevents unauthorized users from emulating the login screen."));

	installEventFilter(this);
	setFixedSize( sizeHint() );

	mSAKProcess = new TDEProcess;
	*mSAKProcess << "tdmtsak";
	connect(mSAKProcess, TQT_SIGNAL(processExited(TDEProcess*)), this, TQT_SLOT(slotSAKProcessExited()));
	mSAKProcess->start();
}

void SAKDlg::slotSAKProcessExited()
{
	int retcode = mSAKProcess->exitStatus();
	if (retcode != 0) {
		trinity_desktop_lock_use_sak = false;
	}
	hide();
}

SAKDlg::~SAKDlg()
{
	if ((mSAKProcess) && (mSAKProcess->isRunning())) {
		mSAKProcess->kill(SIGKILL);
		delete mSAKProcess;
	}
	hide();
}

void SAKDlg::closeDialogForced()
{
	TQDialog::reject();
}

void SAKDlg::reject()
{

}

void SAKDlg::updateLabel(TQString &txt)
{
	mStatusLabel->setPaletteForegroundColor(Qt::black);
	mStatusLabel->setText("<b>" + txt + "</b>");
}

void SAKDlg::show()
{
	TQDialog::show();
	TQApplication::flushX();
}

#include "sakdlg.moc"
