//===========================================================================
//
// This file is part of the TDE project
//
// Copyright (c) 2004 Chris Howells <howells@kde.org>

#include "lockprocess.h"
#include "autologout.h"

#include <tdeapplication.h>
#include <tdelocale.h>
#include <tdeglobalsettings.h>
#include <tdeconfig.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <dcopref.h>
#include <tdemessagebox.h>
#include <kdialog.h>

#include <tqlayout.h>
#include <tqmessagebox.h>
#include <tqlabel.h>
#include <tqstyle.h>
#include <tqapplication.h>
#include <tqdialog.h>
#include <tqprogressbar.h>

#include <X11/Xatom.h>

#define COUNTDOWN 30 

extern bool trinity_desktop_lock_use_system_modal_dialogs;

AutoLogout::AutoLogout(LockProcess *parent) : TQDialog(parent, "password dialog", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM)))
{
	if (trinity_desktop_lock_use_system_modal_dialogs) {
		// Signal that we do not want any window controls to be shown at all
		Atom kde_wm_system_modal_notification;
		kde_wm_system_modal_notification = XInternAtom(tqt_xdisplay(), "_TDE_WM_MODAL_SYS_NOTIFICATION", False);
		XChangeProperty(tqt_xdisplay(), winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
	}
	setCaption(i18n("Automatic Logout Notification"));
	
	frame = new TQFrame(this);
	if (trinity_desktop_lock_use_system_modal_dialogs) {
		frame->setFrameStyle( TQFrame::NoFrame );
	}
	else {
		frame->setFrameStyle(TQFrame::Panel | TQFrame::Raised);
	}
	frame->setLineWidth(2);
	
	TQLabel *pixLabel = new TQLabel( frame, "pixlabel" );
	pixLabel->setPixmap(DesktopIcon("system-log-out"));
	
	TQLabel *greetLabel = new TQLabel(i18n("<nobr><qt><b>Automatic Log Out</b></qt><nobr>"), frame);
	TQLabel *infoLabel = new TQLabel(i18n("<qt>To prevent being logged out, resume using this session by moving the mouse or pressing a key.</qt>"), frame);
	
	mStatusLabel = new TQLabel("<b> </b>", frame);
	mStatusLabel->setAlignment(TQLabel::AlignCenter);
	
	TQLabel *mProgressLabel = new TQLabel("Time Remaining:", frame);
	mProgressRemaining = new TQProgressBar(frame);
	mProgressRemaining->setPercentageVisible(false);
	
	TQVBoxLayout *unlockDialogLayout = new TQVBoxLayout( this );
	unlockDialogLayout->addWidget( frame );
	
	frameLayout = new TQGridLayout(frame, 1, 1, KDialog::marginHint(), KDialog::spacingHint());
	frameLayout->addMultiCellWidget(pixLabel, 0, 2, 0, 0, Qt::AlignCenter | Qt::AlignTop);
	frameLayout->addWidget(greetLabel, 0, 1);
	frameLayout->addWidget(mStatusLabel, 1, 1);
	frameLayout->addWidget(infoLabel, 2, 1);
	frameLayout->addWidget(mProgressLabel, 3, 1);
	frameLayout->addWidget(mProgressRemaining, 4, 1);
	
	// get the time remaining in seconds for the status label
	mRemaining = COUNTDOWN * 25;
	
	mProgressRemaining->setTotalSteps(COUNTDOWN * 25);
	
	updateInfo(mRemaining);
	
	mCountdownTimerId = startTimer(1000/25);
	
	connect(tqApp, TQT_SIGNAL(activity()), TQT_SLOT(slotActivity()));
	
	setFixedSize( sizeHint() );
}

AutoLogout::~AutoLogout()
{
	hide();
}

void AutoLogout::updateInfo(int timeout)
{
	mStatusLabel->setText(i18n("<nobr><qt>You will be automatically logged out in 1 second</qt></nobr>",
				"<nobr><qt>You will be automatically logged out in %n seconds</qt></nobr>",
				timeout / 25) );
	mProgressRemaining->setProgress(timeout);
}

void AutoLogout::timerEvent(TQTimerEvent *ev)
{
	if (ev->timerId() == mCountdownTimerId) {
		updateInfo(mRemaining);
		--mRemaining;
		if (mRemaining < 0) {
			logout();
		}
	}
}

void AutoLogout::slotActivity()
{
	accept();
}

void AutoLogout::logout()
{
	TQT_TQOBJECT(this)->killTimers();
	DCOPRef("ksmserver","ksmserver").send("logout", 0, 0, 0);
}

void AutoLogout::show()
{
	TQDialog::show();
	TQApplication::flushX();
}

#include "autologout.moc"
