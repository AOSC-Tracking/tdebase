//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 2010-2011 Timothy Pearson <kb9vqf@pearsoncomputing.net>

#include <config.h>

#include "sakdlg.h"

#include <dmctl.h>

#include <kapplication.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kseparator.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kdesu/defaults.h>
#include <kpassdlg.h>
#include <kdebug.h>
#include <kuser.h>
#include <dcopref.h>
#include <kmessagebox.h>
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
#include <tqtimer.h>

#include <fcntl.h>
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>

#include "kfdialog.h"

#ifndef AF_LOCAL
# define AF_LOCAL	AF_UNIX
#endif

#define FIFO_DIR "/tmp/ksocket-global/kdm"
#define FIFO_FILE "/tmp/ksocket-global/kdm/kdmctl-%1"
#define FIFO_SAK_FILE "/tmp/ksocket-global/kdm/kdmctl-sak-%1"

bool trinity_desktop_lock_use_system_modal_dialogs = TRUE;
extern bool trinity_desktop_lock_use_sak;

//===========================================================================
//
// Simple dialog for displaying an unlock status or recurring error message
//
SAKDlg::SAKDlg(TQWidget *parent)
    : TQDialog(parent, "information dialog", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM))),
      mUnlockingFailed(false), mPipe_fd(-1), closingDown(false)
{
    if (trinity_desktop_lock_use_system_modal_dialogs) {
        // Signal that we do not want any window controls to be shown at all
        Atom kde_wm_system_modal_notification;
        kde_wm_system_modal_notification = XInternAtom(qt_xdisplay(), "_KDE_WM_MODAL_SYS_NOTIFICATION", False);
        XChangeProperty(qt_xdisplay(), winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
    }
    setCaption(KDM_LOGIN_SCREEN_BASE_TITLE);

    frame = new TQFrame( this );
    if (trinity_desktop_lock_use_system_modal_dialogs)
        frame->setFrameStyle( TQFrame::NoFrame );
    else
        frame->setFrameStyle( TQFrame::Panel | TQFrame::Raised );
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

    mSAKProcess = new KProcess;
    *mSAKProcess << "kdmtsak" << "dm";
    connect(mSAKProcess, TQT_SIGNAL(processExited(KProcess*)), this, TQT_SLOT(slotSAKProcessExited()));
    mSAKProcess->start();

    TQTimer::singleShot( 0, this, TQT_SLOT(handleInputPipe()) );
}

void SAKDlg::slotSAKProcessExited()
{
    int retcode = mSAKProcess->exitStatus();
    if (retcode != 0) trinity_desktop_lock_use_sak = false;
    closingDown = true;
    hide();
}

void SAKDlg::handleInputPipe(void) {
	if (closingDown) {
		::unlink(mPipeFilename.ascii());
		return;
	}

	if (isShown() == false) {
		TQTimer::singleShot( 100, this, TQT_SLOT(handleInputPipe()) );
		return;
	}

	char readbuf[2048];
	int displayNumber;
	TQString currentDisplay;
	currentDisplay = TQString(getenv("DISPLAY"));
	currentDisplay = currentDisplay.replace(":", "");
	displayNumber = currentDisplay.toInt();
	mPipeFilename = TQString(FIFO_SAK_FILE).arg(displayNumber);
	::unlink((TQString(FIFO_FILE).arg(displayNumber)).ascii());

	/* Create the FIFOs if they do not exist */
	umask(0);
	struct stat buffer;
	int status;
	status = stat(FIFO_DIR, &buffer);
	if (status == 0) {
		int file_mode = ((buffer.st_mode & S_IRWXU) >> 6) * 100;
		file_mode = file_mode + ((buffer.st_mode & S_IRWXG) >> 3) * 10;
		file_mode = file_mode + ((buffer.st_mode & S_IRWXO) >> 0) * 1;
		if ((file_mode != 600) || (buffer.st_uid != 0) || (buffer.st_gid != 0)) {
			::unlink(mPipeFilename.ascii());
			printf("[WARNING] Possible security breach!  Please check permissions on " FIFO_DIR " (must be 600 and owned by root/root, got %d %d/%d).  Not listening for login credentials on remote control socket.\n", file_mode, buffer.st_uid, buffer.st_gid); fflush(stdout);
			return;
		}
	}
	mkdir(FIFO_DIR,0600);
	mknod(mPipeFilename.ascii(), S_IFIFO|0600, 0);
	chmod(mPipeFilename.ascii(), 0600);

	mPipe_fd = ::open(mPipeFilename.ascii(), O_RDONLY | O_NONBLOCK);
	int numread;
	TQString inputcommand = "";
	while ((!inputcommand.contains('\n')) && (!closingDown)) {
		numread = ::read(mPipe_fd, readbuf, 2048);
		readbuf[numread] = 0;
		readbuf[2047] = 0;
		inputcommand += readbuf;
		tqApp->processEvents();
	}
	if (closingDown) {
		::unlink(mPipeFilename.ascii());
		return;
	}
	inputcommand = inputcommand.replace('\n', "");
	TQStringList commandList = TQStringList::split('\t', inputcommand, false);
	if ((*(commandList.at(0))) == "CLOSE") {
		mSAKProcess->kill();
	}
	if (!closingDown) {
		TQTimer::singleShot( 0, this, TQT_SLOT(handleInputPipe()) );
		::close(mPipe_fd);
		::unlink(mPipeFilename.ascii());
	}
	else {
		::unlink(mPipeFilename.ascii());
	}
}

SAKDlg::~SAKDlg()
{
    if ((mSAKProcess) && (mSAKProcess->isRunning())) {
        mSAKProcess->kill(SIGTERM);
        delete mSAKProcess;
    }
    if (mPipe_fd != -1) {
        closingDown = true;
        ::close(mPipe_fd);
        ::unlink(mPipeFilename.ascii());
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
