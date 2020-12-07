//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
// Copyright (c) 2003 Chris Howells <howells@kde.org>
// Copyright (c) 2003 Oswald Buddenhagen <ossi@kde.org>

#include <config.h>

#include "lockprocess.h"
#include "lockdlg.h"
#include "kdesktopsettings.h"

#include <kcheckpass.h>
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

#define PASSDLG_HIDE_TIMEOUT dialogHideTimeout

extern bool trinity_desktop_lock_autohide_lockdlg;
extern bool trinity_desktop_lock_delay_screensaver_start;
extern bool trinity_desktop_lock_use_system_modal_dialogs;
extern bool trinity_desktop_lock_use_sak;
extern bool trinity_desktop_lock_hide_cancel_button;

int dialogHideTimeout = 10*1000;

//===========================================================================
//
// Simple dialog for entering a password.
//
PasswordDlg::PasswordDlg(LockProcess *parent, GreeterPluginHandle *plugin)
    : TQDialog(parent, "password dialog", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM))),
      mPlugin( plugin ),
      mCapsLocked(-1),
      mUnlockingFailed(false)
{
    init(plugin);
}

//
// Simple dialog for entering a password.
// This version includes support for displaying the date and time the lock process was started
//
PasswordDlg::PasswordDlg(LockProcess *parent, GreeterPluginHandle *plugin, TQDateTime lockStartDateTime)
    : TQDialog(parent, "password dialog", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM))),
      mPlugin( plugin ),
      mCapsLocked(-1),
      mUnlockingFailed(false)
{
    m_lockStartDT = lockStartDateTime;
    init(plugin);
}

void PasswordDlg::init(GreeterPluginHandle *plugin)
{
    dialogHideTimeout = trinity_desktop_lock_delay_screensaver_start?KDesktopSettings::timeout()*1000:10*1000;

    if (trinity_desktop_lock_use_system_modal_dialogs) {
        // Signal that we do not want any window controls to be shown at all
        Atom kde_wm_system_modal_notification;
        kde_wm_system_modal_notification = XInternAtom(tqt_xdisplay(), "_TDE_WM_MODAL_SYS_NOTIFICATION", False);
        XChangeProperty(tqt_xdisplay(), winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
    }
    setCaption(i18n("Desktop Session Locked"));

    frame = new TQFrame( this );
    if (trinity_desktop_lock_use_system_modal_dialogs)
        frame->setFrameStyle( TQFrame::NoFrame );
    else
        frame->setFrameStyle( TQFrame::Panel | TQFrame::Raised );
    frame->setLineWidth( 2 );

    TQLabel *pixLabel = NULL;
    if (!trinity_desktop_lock_use_system_modal_dialogs) {
        pixLabel = new TQLabel( frame, "pixlabel" );
        pixLabel->setPixmap(DesktopIcon("system-lock-screen"));
    }

    KUser user;
    TQLabel *greetLabel;
    if (trinity_desktop_lock_use_system_modal_dialogs) {
        greetLabel = new TQLabel( user.fullName().isEmpty() ?
                "<b>" + i18n("This computer is in use and has been locked.") + "</b>" :
                "<b>" + i18n("This computer is in use and has been locked.") + "</b><br><nobr>" + i18n("Only '%1' may unlock this session.").arg( user.fullName() ), frame );
    }
    else {
        greetLabel = new TQLabel( user.fullName().isEmpty() ?
                i18n("<nobr><b>The session is locked</b><br>") :
                i18n("<nobr><b>The session was locked by %1</b><br>").arg( user.fullName() ), frame );
    }

    TQLabel *lockDTLabel = NULL;
    if ((trinity_desktop_lock_use_system_modal_dialogs) && (!m_lockStartDT.isNull())) {
        lockDTLabel = new TQLabel(i18n("This session has been locked since %1").arg(m_lockStartDT.toString()), frame);
    }

    mStatusLabel = new TQLabel( "<b> </b>", frame );
    mStatusLabel->setAlignment( TQLabel::AlignCenter );

    mLayoutButton = new TQPushButton( frame );
    mLayoutButton->setFlat( true );

    KSeparator *sep = new KSeparator( KSeparator::HLine, frame );

    mNewSessButton = new KPushButton( KGuiItem(i18n("Sw&itch User..."), "fork"), frame );
    ok = new KPushButton( i18n("Unl&ock"), frame );
    ok->setDefault(true);

    bool show_cancel_button = !trinity_desktop_lock_hide_cancel_button || 
                              trinity_desktop_lock_use_sak ||
                              !trinity_desktop_lock_use_system_modal_dialogs;
    if (show_cancel_button)
      cancel = new KPushButton( KStdGuiItem::cancel(), frame );
    else
      cancel = NULL;

    greet = plugin->info->create( this, 0, this, mLayoutButton, TQString::null,
              KGreeterPlugin::Authenticate, KGreeterPlugin::ExUnlock );

    TQVBoxLayout *unlockDialogLayout = new TQVBoxLayout( this );
    unlockDialogLayout->addWidget( frame );

    TQHBoxLayout *layStatus = new TQHBoxLayout( 0, 0, KDialog::spacingHint());
    layStatus->addWidget( mStatusLabel );
    layStatus->addWidget( mLayoutButton );

    TQHBoxLayout *layButtons = new TQHBoxLayout( 0, 0, KDialog::spacingHint());
    layButtons->addWidget( mNewSessButton );
    layButtons->addStretch();
    layButtons->addWidget(ok);
    if (show_cancel_button)
      layButtons->addWidget(cancel);

    if (trinity_desktop_lock_use_system_modal_dialogs) {
        KSMModalDialogHeader* theader = new KSMModalDialogHeader( frame );

        if (!m_lockStartDT.isNull()) {
            frameLayout = new TQGridLayout( frame, 1, 1, KDialog::marginHint(), KDialog::spacingHint() );
            frameLayout->addMultiCellWidget( theader, 0, 0, 0, 2, Qt::AlignTop );
            frameLayout->addWidget( greetLabel, 1, 1 );
            frameLayout->addWidget( lockDTLabel, 2, 1 );
            frameLayout->addItem( greet->getLayoutItem(), 3, 1 );
            frameLayout->addLayout( layStatus, 4, 1 );
            frameLayout->addMultiCellWidget( sep, 5, 5, 0, 1 );
            frameLayout->addMultiCellLayout( layButtons, 6, 6, 0, 1 );
        }
        else {
            frameLayout = new TQGridLayout( frame, 1, 1, KDialog::marginHint(), KDialog::spacingHint() );
            frameLayout->addMultiCellWidget( theader, 0, 0, 0, 2, Qt::AlignTop );
            frameLayout->addWidget( greetLabel, 1, 1 );
            frameLayout->addItem( greet->getLayoutItem(), 2, 1 );
            frameLayout->addLayout( layStatus, 3, 1 );
            frameLayout->addMultiCellWidget( sep, 4, 4, 0, 1 );
            frameLayout->addMultiCellLayout( layButtons, 5, 5, 0, 1 );
        }
    }
    else {
        frameLayout = new TQGridLayout( frame, 1, 1, KDialog::marginHint(), KDialog::spacingHint() );
        frameLayout->addMultiCellWidget( pixLabel, 0, 2, 0, 0, Qt::AlignTop );
        frameLayout->addWidget( greetLabel, 0, 1 );
        frameLayout->addItem( greet->getLayoutItem(), 1, 1 );
        frameLayout->addLayout( layStatus, 2, 1 );
        frameLayout->addMultiCellWidget( sep, 3, 3, 0, 1 );
        frameLayout->addMultiCellLayout( layButtons, 4, 4, 0, 1 );
    }

    if (show_cancel_button)
    {
      setTabOrder( ok, cancel );
      setTabOrder( cancel, mNewSessButton );
    }
    else
    {
      setTabOrder( ok, mNewSessButton );
    }
    setTabOrder( mNewSessButton, mLayoutButton );

    connect(mLayoutButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(layoutClicked()));
    if (show_cancel_button)
    { 
      connect(cancel, TQT_SIGNAL(clicked()), TQT_SLOT(reject()));
    }
    connect(ok, TQT_SIGNAL(clicked()), TQT_SLOT(slotOK()));
    connect(mNewSessButton, TQT_SIGNAL(clicked()), TQT_SLOT(slotSwitchUser()));

    if (!DM().isSwitchable() || !kapp->authorize("switch_user"))
        mNewSessButton->hide();

    installEventFilter(this);
    setFixedSize( sizeHint() );

    mFailedTimerId = 0;
    mTimeoutTimerId = startTimer(PASSDLG_HIDE_TIMEOUT);
    connect(tqApp, TQT_SIGNAL(activity()), TQT_SLOT(slotActivity()) );

    greet->start();

    DCOPRef kxkb("kxkb", "kxkb");
    if( !kxkb.isNull() ) {
        DCOPReply reply = kxkb.call("getLayoutsList");
        if (reply.isValid()) {
            layoutsList = reply;
        }
        reply = kxkb.call("getCurrentLayout");
        TQString currentLayout;
        if (reply.isValid()) {
             reply.get(currentLayout);
        }
        if( !currentLayout.isEmpty() && layoutsList.count() > 1 ) {
            currLayout = layoutsList.find(currentLayout);
            if (currLayout == layoutsList.end())
                setLayoutText("err");
            else
                setLayoutText(*currLayout);
        } else
            mLayoutButton->hide();
    } else {
        mLayoutButton->hide(); // no kxkb running
    }
    capsLocked();
}

PasswordDlg::~PasswordDlg()
{
    hide();
    frameLayout->removeItem( greet->getLayoutItem() );
    delete greet;
}

void PasswordDlg::reject()
{
  if (!trinity_desktop_lock_hide_cancel_button || trinity_desktop_lock_use_sak ||   
      !trinity_desktop_lock_use_system_modal_dialogs || trinity_desktop_lock_autohide_lockdlg)
  {
    TQDialog::reject();
  }
} 

void PasswordDlg::layoutClicked()
{

    if( ++currLayout == layoutsList.end() )
        currLayout = layoutsList.begin();

    DCOPRef kxkb("kxkb", "kxkb");
    setLayoutText( kxkb.call("setLayout", *currLayout) ? *currLayout : "err" );

}

void PasswordDlg::setLayoutText( const TQString &txt )
{
    mLayoutButton->setText( txt );
    TQSize sz = mLayoutButton->fontMetrics().size( 0, txt );
    int mrg = mLayoutButton->style().pixelMetric( TQStyle::PM_ButtonMargin ) * 2;
    mLayoutButton->setFixedSize( sz.width() + mrg, sz.height() + mrg );
}

void PasswordDlg::updateLabel()
{
    if (mUnlockingFailed)
    {
        mStatusLabel->setPaletteForegroundColor(Qt::black);
        mStatusLabel->setText(i18n("<b>Unlocking failed</b>"));
//         mStatusLabel->show();
    }
    else
    if (mCapsLocked)
    {
        mStatusLabel->setPaletteForegroundColor(Qt::red);
        mStatusLabel->setText(i18n("<b>Warning: Caps Lock on</b>"));
//         mStatusLabel->show();
    }
    else
    {
        mStatusLabel->setText("<b> </b>");
//         mStatusLabel->hide();
    }
}

//---------------------------------------------------------------------------
//
// Handle timer events.
//
void PasswordDlg::timerEvent(TQTimerEvent *ev)
{
    if (ev->timerId() == mTimeoutTimerId)
    {
        if (trinity_desktop_lock_autohide_lockdlg)
        {
          // Force dialog rejection regardless of the value of trinity_desktop_lock_hide_cancel_button
          TQDialog::reject();
        }
        else
        {
          slotActivity();
        }
    }
    else if (ev->timerId() == mFailedTimerId)
    {
        killTimer(mFailedTimerId);
        mFailedTimerId = 0;
        // Show the normal password prompt.
        mUnlockingFailed = false;
        updateLabel();
        ok->setEnabled(true);
        if (cancel) cancel->setEnabled(true);
        mNewSessButton->setEnabled( true );
        greet->revive();
        greet->start();
    }
}

bool PasswordDlg::eventFilter(TQObject *, TQEvent *ev)
{
    if (ev->type() == TQEvent::KeyPress || ev->type() == TQEvent::KeyRelease)
        capsLocked();
    return false;
}

void PasswordDlg::slotActivity()
{
    if (mTimeoutTimerId) {
        killTimer(mTimeoutTimerId);
        mTimeoutTimerId = startTimer(PASSDLG_HIDE_TIMEOUT);
    }
}

////// kckeckpass interface code

int PasswordDlg::Reader (void *buf, int count)
{
    int ret, rlen;

    for (rlen = 0; rlen < count; ) {
      dord:
        ret = ::read (sFd, (void *)((char *)buf + rlen), count - rlen);
        if (ret < 0) {
            if (errno == EINTR)
                goto dord;
            if (errno == EAGAIN)
                break;
            return -1;
        }
        if (!ret)
            break;
        rlen += ret;
    }
    return rlen;
}

bool PasswordDlg::GRead (void *buf, int count)
{
    return Reader (buf, count) == count;
}

bool PasswordDlg::GWrite (const void *buf, int count)
{
    return ::write (sFd, buf, count) == count;
}

bool PasswordDlg::GSendInt (int val)
{
    return GWrite (&val, sizeof(val));
}

bool PasswordDlg::GSendStr (const char *buf)
{
    int len = buf ? ::strlen (buf) + 1 : 0;
    return GWrite (&len, sizeof(len)) && GWrite (buf, len);
}

bool PasswordDlg::GSendArr (int len, const char *buf)
{
    return GWrite (&len, sizeof(len)) && GWrite (buf, len);
}

bool PasswordDlg::GRecvInt (int *val)
{
    return GRead (val, sizeof(*val));
}

bool PasswordDlg::GRecvArr (char **ret)
{
    int len;
    char *buf;

    if (!GRecvInt(&len))
        return false;
    if (!len) {
        *ret = 0;
        return true;
    }
    if (!(buf = (char *)::malloc (len)))
        return false;
    *ret = buf;
    return GRead (buf, len);
}

void PasswordDlg::reapVerify()
{
    ::close( sFd );
    int status;
    pid_t retpid = ::waitpid( sPid, &status, 0 );
    if (retpid < 0) {
        // FIXME
        // ERROR
    }
    else {
        if (WIFEXITED(status)) {
            switch (WEXITSTATUS(status)) {
            case AuthOk:
                greet->succeeded();
                accept();
                return;
            case AuthBad:
                greet->failed();
                mUnlockingFailed = true;
                updateLabel();
                mFailedTimerId = startTimer(1500);
                ok->setEnabled(false);
                if (cancel) cancel->setEnabled(false);
                mNewSessButton->setEnabled( false );
                return;
            case AuthAbort:
                return;
            }
        }
        else if (WIFSIGNALED(status)) {
            // FIXME
            // ERROR
        }
    }
    cantCheck();
}

void PasswordDlg::handleVerify()
{
    int ret;
    char *arr;

    while (GRecvInt( &ret )) {
        switch (ret) {
        case ConvGetBinary:
            if (!GRecvArr( &arr ))
                break;
            greet->binaryPrompt( arr, false );
            if (arr)
                ::free( arr );
            return;
        case ConvGetNormal:
            if (!GRecvArr( &arr ))
                break;
            greet->textPrompt( arr, true, false );
            if (arr)
                ::free( arr );
            return;
        case ConvGetHidden:
            if (!GRecvArr( &arr ))
                break;
            greet->textPrompt( arr, false, false );
            if (arr)
                ::free( arr );
            return;
        case ConvPutInfo:
            if (!GRecvArr( &arr ))
                break;
            if (!greet->textMessage( arr, false ))
                static_cast< LockProcess* >(parent())->msgBox( TQMessageBox::Information, TQString::fromLocal8Bit( arr ) );
            ::free( arr );
            continue;
        case ConvPutError:
            if (!GRecvArr( &arr ))
                break;
            if (!greet->textMessage( arr, true ))
                static_cast< LockProcess* >(parent())->msgBox( TQMessageBox::Warning, TQString::fromLocal8Bit( arr ) );
            ::free( arr );
            continue;
        }
        break;
    }
    reapVerify();
}

////// greeter plugin callbacks

void PasswordDlg::gplugReturnText( const char *text, int tag )
{
    GSendStr( text );
    if (text)
        GSendInt( tag );
    handleVerify();
}

void PasswordDlg::gplugReturnBinary( const char *data )
{
    if (data) {
        unsigned const char *up = (unsigned const char *)data;
        int len = up[3] | (up[2] << 8) | (up[1] << 16) | (up[0] << 24);
        if (!len)
            GSendArr( 4, data );
        else
            GSendArr( len, data );
    } else
        GSendArr( 0, 0 );
    handleVerify();
}

void PasswordDlg::gplugSetUser( const TQString & )
{
    // ignore ...
}

void PasswordDlg::cantCheck()
{
    greet->failed();
    static_cast< LockProcess* >(parent())->msgBox( TQMessageBox::Critical,
        i18n("Cannot unlock the session because the authentication system failed to work;\n"
             "you must kill kdesktop_lock (pid %1) manually.").arg(getpid()) );
    greet->revive();
}

//---------------------------------------------------------------------------
//
// Starts the kcheckpass process to check the user's password.
//
void PasswordDlg::gplugStart()
{
    int sfd[2];
    char fdbuf[16];

    if (::socketpair(AF_LOCAL, SOCK_STREAM, 0, sfd)) {
        cantCheck();
        return;
    }
    if ((sPid = ::fork()) < 0) {
        ::close(sfd[0]);
        ::close(sfd[1]);
        cantCheck();
        return;
    }
    if (!sPid) {
        ::close(sfd[0]);
        sprintf(fdbuf, "%d", sfd[1]);
        execlp("kcheckpass", "kcheckpass",
#ifdef HAVE_PAM
               "-c", TDESCREENSAVER_PAM_SERVICE,
#endif
               "-m", mPlugin->info->method,
               "-S", fdbuf,
               (char *)0);
        exit(20);
    }
    ::close(sfd[1]);
    sFd = sfd[0];
    handleVerify();
}

void PasswordDlg::gplugActivity()
{
    slotActivity();
}

void PasswordDlg::gplugMsgBox( TQMessageBox::Icon type, const TQString &text )
{
    TQDialog dialog( this, 0, true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM)) );
    if (trinity_desktop_lock_use_system_modal_dialogs) {
        // Signal that we do not want any window controls to be shown at all
        Atom kde_wm_system_modal_notification;
        kde_wm_system_modal_notification = XInternAtom(tqt_xdisplay(), "_TDE_WM_MODAL_SYS_NOTIFICATION", False);
        XChangeProperty(tqt_xdisplay(), dialog.winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
    }
    dialog.setCaption(i18n("Authentication Subsystem Notice"));
    TQFrame *winFrame = new TQFrame( &dialog );
    if (trinity_desktop_lock_use_system_modal_dialogs)
        winFrame->setFrameStyle( TQFrame::NoFrame );
    else
        winFrame->setFrameStyle( TQFrame::WinPanel | TQFrame::Raised );
    winFrame->setLineWidth( 2 );
    TQVBoxLayout *vbox = new TQVBoxLayout( &dialog );
    vbox->addWidget( winFrame );

    TQLabel *label1 = new TQLabel( winFrame );
    label1->setPixmap( TQMessageBox::standardIcon( type ) );
    TQLabel *label2 = new TQLabel( text, winFrame );
    KPushButton *button = new KPushButton( KStdGuiItem::ok(), winFrame );
    button->setDefault( true );
    button->setSizePolicy( TQSizePolicy( TQSizePolicy::Preferred, TQSizePolicy::Preferred ) );
    connect( button, TQT_SIGNAL( clicked() ), TQT_SLOT( accept() ) );

    TQGridLayout *grid = new TQGridLayout( winFrame, 2, 2, 10 );
    grid->addWidget( label1, 0, 0, Qt::AlignCenter );
    grid->addWidget( label2, 0, 1, Qt::AlignCenter );
    grid->addMultiCellWidget( button, 1,1, 0,1, Qt::AlignCenter );

    static_cast< LockProcess* >(parent())->execDialog( &dialog );
}

void PasswordDlg::slotOK()
{
    greet->next();
}


void PasswordDlg::show()
{
    TQDialog::show();
    TQApplication::flushX();
    setFixedSize( sizeHint() );
}

void PasswordDlg::slotStartNewSession()
{
    if (!KMessageBox::shouldBeShownContinue( ":confirmNewSession" )) {
        DM().startReserve();
        return;
    }

    killTimer(mTimeoutTimerId);
    mTimeoutTimerId = 0;

    TQDialog *dialog = new TQDialog( this, "warnbox", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM)));

    if (trinity_desktop_lock_use_system_modal_dialogs) {
        // Signal that we do not want any window controls to be shown at all
        Atom kde_wm_system_modal_notification;
        kde_wm_system_modal_notification = XInternAtom(tqt_xdisplay(), "_TDE_WM_MODAL_SYS_NOTIFICATION", False);
        XChangeProperty(tqt_xdisplay(), dialog->winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
    }
    dialog->setCaption(i18n("New Session"));

    TQFrame *winFrame = new TQFrame( dialog );
    if (trinity_desktop_lock_use_system_modal_dialogs)
        winFrame->setFrameStyle( TQFrame::NoFrame );
    else
        winFrame->setFrameStyle( TQFrame::WinPanel | TQFrame::Raised );
    winFrame->setLineWidth( 2 );
    TQVBoxLayout *vbox = new TQVBoxLayout( dialog );
    vbox->addWidget( winFrame );

    TQLabel *label1 = new TQLabel( winFrame );
    label1->setPixmap( TQMessageBox::standardIcon( TQMessageBox::Warning ) );
    TQString qt_text =
          i18n("You have chosen to open another desktop session "
               "instead of resuming the current one.<br>"
               "The current session will be hidden "
               "and a new login screen will be displayed.<br>"
               "An F-key is assigned to each session; "
               "F%1 is usually assigned to the first session, "
               "F%2 to the second session and so on. "
               "You can switch between sessions by pressing "
               "Ctrl, Alt and the appropriate F-key at the same time. "
               "Additionally, the TDE Panel and Desktop menus have "
               "actions for switching between sessions.")
            .arg(7).arg(8);
    TQLabel *label2 = new TQLabel( qt_text, winFrame );
    KPushButton *okbutton = new KPushButton( KGuiItem(i18n("&Start New Session"), "fork"), winFrame );
    okbutton->setDefault( true );
    connect( okbutton, TQT_SIGNAL( clicked() ), dialog, TQT_SLOT( accept() ) );
    KPushButton *cbutton = new KPushButton( KStdGuiItem::cancel(), winFrame );
    connect( cbutton, TQT_SIGNAL( clicked() ), dialog, TQT_SLOT( reject() ) );

    TQBoxLayout *mbox = new TQVBoxLayout( winFrame, KDialog::marginHint(), KDialog::spacingHint() );

    TQGridLayout *grid = new TQGridLayout( mbox, 2, 2, 2 * KDialog::spacingHint() );
    grid->setMargin( KDialog::marginHint() );
    grid->addWidget( label1, 0, 0, Qt::AlignCenter );
    grid->addWidget( label2, 0, 1, Qt::AlignCenter );
    TQCheckBox *cb = new TQCheckBox( i18n("&Do not ask again"), winFrame );
    grid->addMultiCellWidget( cb, 1,1, 0,1 );

    TQBoxLayout *hbox = new TQHBoxLayout( mbox, KDialog::spacingHint() );
    hbox->addStretch( 1 );
    hbox->addWidget( okbutton );
    hbox->addStretch( 1 );
    hbox->addWidget( cbutton );
    hbox->addStretch( 1 );

    // stolen from kmessagebox
    int pref_width = 0;
    int pref_height = 0;
    // Calculate a proper size for the text.
    {
       TQSimpleRichText rt(qt_text, dialog->font());
       TQRect rect = TDEGlobalSettings::desktopGeometry(dialog);

       pref_width = rect.width() / 3;
       rt.setWidth(pref_width);
       int used_width = rt.widthUsed();
       pref_height = rt.height();
       if (used_width <= pref_width)
       {
          while(true)
          {
             int new_width = (used_width * 9) / 10;
             rt.setWidth(new_width);
             int new_height = rt.height();
             if (new_height > pref_height)
                break;
             used_width = rt.widthUsed();
             if (used_width > new_width)
                break;
          }
          pref_width = used_width;
       }
       else
       {
          if (used_width > (pref_width *2))
             pref_width = pref_width *2;
          else
             pref_width = used_width;
       }
    }
    label2->setFixedSize(TQSize(pref_width+10, pref_height));

    int ret = static_cast< LockProcess* >( parent())->execDialog( dialog );

    delete dialog;

    if (ret == TQDialog::Accepted) {
        if (cb->isChecked())
            KMessageBox::saveDontShowAgainContinue( ":confirmNewSession" );
        DM().startReserve();
    }

    mTimeoutTimerId = startTimer(PASSDLG_HIDE_TIMEOUT);
}

class LockListViewItem : public TQListViewItem {
public:
    LockListViewItem( TQListView *parent,
		      const TQString &sess, const TQString &loc, int _vt )
	: TQListViewItem( parent )
	, vt( _vt )
    {
	setText( 0, sess );
	setText( 1, loc );
    }

    int vt;
};

void PasswordDlg::slotSwitchUser()
{
    int p = 0;
    DM dm;

    TQDialog dialog( this, "sessbox", true, (trinity_desktop_lock_use_system_modal_dialogs?((WFlags)WStyle_StaysOnTop):((WFlags)WX11BypassWM)) );

    if (trinity_desktop_lock_use_system_modal_dialogs) {
        // Signal that we do not want any window controls to be shown at all
        Atom kde_wm_system_modal_notification;
        kde_wm_system_modal_notification = XInternAtom(tqt_xdisplay(), "_TDE_WM_MODAL_SYS_NOTIFICATION", False);
        XChangeProperty(tqt_xdisplay(), dialog.winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
    }
    dialog.setCaption(i18n("Switch User"));

    TQFrame *winFrame = new TQFrame( &dialog );
    if (trinity_desktop_lock_use_system_modal_dialogs)
        winFrame->setFrameStyle( TQFrame::NoFrame );
    else
        winFrame->setFrameStyle( TQFrame::WinPanel | TQFrame::Raised );
    winFrame->setLineWidth( 2 );
    TQBoxLayout *vbox = new TQVBoxLayout( &dialog );
    vbox->addWidget( winFrame );

    TQBoxLayout *hbox = new TQHBoxLayout( winFrame, KDialog::marginHint(), KDialog::spacingHint() );

    TQBoxLayout *vbox1 = new TQVBoxLayout( hbox, KDialog::spacingHint() );
    TQBoxLayout *vbox2 = new TQVBoxLayout( hbox, KDialog::spacingHint() );

    KPushButton *btn;

    SessList sess;
    if (dm.localSessions( sess )) {

        lv = new TQListView( winFrame );
        connect( lv, TQT_SIGNAL(doubleClicked(TQListViewItem *, const TQPoint&, int)), TQT_SLOT(slotSessionActivated()) );
        connect( lv, TQT_SIGNAL(doubleClicked(TQListViewItem *, const TQPoint&, int)), &dialog, TQT_SLOT(accept()) );
        lv->setAllColumnsShowFocus( true );
        lv->addColumn( i18n("Session") );
        lv->addColumn( i18n("Location") );
        lv->setColumnWidthMode( 0, TQListView::Maximum );
        lv->setColumnWidthMode( 1, TQListView::Maximum );
        TQListViewItem *itm = 0;
        TQString user, loc;
        int ns = 0;
        for (SessList::ConstIterator it = sess.begin(); it != sess.end(); ++it) {
            DM::sess2Str2( *it, user, loc );
            itm = new LockListViewItem( lv, user, loc, (*it).vt );
            if (!(*it).vt)
                itm->setEnabled( false );
            if ((*it).self) {
                lv->setCurrentItem( itm );
                itm->setSelected( true );
            }
            ns++;
        }
        int fw = lv->frameWidth() * 2;
        TQSize hds( lv->header()->sizeHint() );
        lv->setMinimumWidth( fw + hds.width() +
            (ns > 10 ? style().pixelMetric(TQStyle::PM_ScrollBarExtent) : 0 ) );
        lv->setFixedHeight( fw + hds.height() +
            itm->height() * (ns < 6 ? 6 : ns > 10 ? 10 : ns) );
        lv->header()->adjustHeaderSize();
        vbox1->addWidget( lv );

        btn = new KPushButton( KGuiItem(i18n("session", "&Activate"), "fork"), winFrame );
        connect( btn, TQT_SIGNAL(clicked()), TQT_SLOT(slotSessionActivated()) );
        connect( btn, TQT_SIGNAL(clicked()), &dialog, TQT_SLOT(accept()) );
        vbox2->addWidget( btn );
        vbox2->addStretch( 2 );
    }

    if (kapp->authorize("start_new_session") && (p = dm.numReserve()) >= 0)
    {
        btn = new KPushButton( KGuiItem(i18n("Start &New Session"), "fork"), winFrame );
        connect( btn, TQT_SIGNAL(clicked()), TQT_SLOT(slotStartNewSession()) );
        connect( btn, TQT_SIGNAL(clicked()), &dialog, TQT_SLOT(accept()) );
        if (!p)
            btn->setEnabled( false );
        vbox2->addWidget( btn );
        vbox2->addStretch( 1 );
    }

    btn = new KPushButton( KStdGuiItem::cancel(), winFrame );
    connect( btn, TQT_SIGNAL(clicked()), &dialog, TQT_SLOT(reject()) );
    vbox2->addWidget( btn );

    dialog.setFixedSize( dialog.sizeHint() );

    int ret = static_cast< LockProcess* >(parent())->execDialog( &dialog );
    if (ret != TQDialog::Rejected) {
        TQDialog::reject();
    }
}

void PasswordDlg::slotSessionActivated()
{
    LockListViewItem *itm = (LockListViewItem *)lv->currentItem();
    if (itm && itm->vt > 0)
        DM().switchVT( itm->vt );
}

void PasswordDlg::capsLocked()
{
    unsigned int lmask;
    Window dummy1, dummy2;
    int dummy3, dummy4, dummy5, dummy6;
    XQueryPointer(tqt_xdisplay(), DefaultRootWindow( tqt_xdisplay() ), &dummy1, &dummy2, &dummy3, &dummy4, &dummy5, &dummy6, &lmask);
    mCapsLocked = lmask & LockMask;
    updateLabel();
}

#include "lockdlg.moc"
