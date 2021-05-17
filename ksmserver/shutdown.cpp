/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2005 Lubos Lunak <l.lunak@kde.org>

relatively small extensions by Oswald Buddenhagen <ob6@inf.tu-dresden.de>

some code taken from the dcopserver (part of the KDE libraries), which is
Copyright (c) 1999 Matthias Ettrich <ettrich@kde.org>
Copyright (c) 1999 Preston Brown <pbrown@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <tqfile.h>
#include <tqtextstream.h>
#include <tqdatastream.h>
#include <tqptrstack.h>
#include <tqpushbutton.h>
#include <tqmessagebox.h>
#include <tqguardedptr.h>
#include <tqtimer.h>
#include <tqregexp.h>

#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <kstandarddirs.h>
#include <unistd.h>
#include <tdeapplication.h>
#include <kstaticdeleter.h>
#include <tdetempfile.h>
#include <kprocess.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <dmctl.h>
#include <kdebug.h>
#include <knotifyclient.h>

#include <libtdersync/tdersync.h>

#include "server.h"
#include "global.h"
#include "shutdowndlg.h"
#include "client.h"

#ifdef BUILD_PROFILE_SHUTDOWN
#define PROFILE_SHUTDOWN 1
#endif

#ifdef PROFILE_SHUTDOWN
	#define SHUTDOWN_MARKER(x) printf("[ksmserver] '%s' [%s]\n", x, TQTime::currentTime().toString("hh:mm:ss:zzz").ascii()); fflush(stdout);
#else // PROFILE_SHUTDOWN
	#define SHUTDOWN_MARKER(x)
#endif // PROFILE_SHUTDOWN

// Time to wait after close request for graceful application termination
// If set too high running applications may be ungracefully terminated on slow machines or when many X11 applications are running
#define KSMSERVER_SHUTDOWN_CLIENT_UNRESPONSIVE_TIMEOUT 20000

// Time to wait before showing manual termination options
// If set too low the user may be confused by buttons briefly flashing up on the screen during an otherwise normal logout process
#define KSMSERVER_NOTIFICATION_MANUAL_OPTIONS_TIMEOUT 3000

void KSMServer::logout( int confirm, int sdtype, int sdmode )
{
    shutdown( (TDEApplication::ShutdownConfirm)confirm,
              (TDEApplication::ShutdownType)sdtype,
              (TDEApplication::ShutdownMode)sdmode );
}

bool KSMServer::checkStatus( bool &logoutConfirmed, bool &maysd, bool &mayrb,
                             TDEApplication::ShutdownConfirm confirm,
                             TDEApplication::ShutdownType sdtype,
                             TDEApplication::ShutdownMode sdmode )
{
    pendingShutdown.stop();
    if( dialogActive ) {
        return false;
    }
    if( state >= Shutdown ) { // already performing shutdown
        return false;
    }
    if( state != Idle ) { // performing startup
    // perform shutdown as soon as startup is finished, in order to avoid saving partial session
        if( !pendingShutdown.isActive()) {
            pendingShutdown.start( 1000 );
            pendingShutdown_confirm = confirm;
            pendingShutdown_sdtype = sdtype;
            pendingShutdown_sdmode = sdmode;
        }
        return false;
    }

    TDEConfig *config = TDEGlobal::config();
    config->reparseConfiguration(); // config may have changed in the KControl module

    config->setGroup("General" );
    logoutConfirmed =
        (confirm == TDEApplication::ShutdownConfirmYes) ? false :
        (confirm == TDEApplication::ShutdownConfirmNo) ? true :
        !config->readBoolEntry( "confirmLogout", true );
    maysd = false;
    mayrb = false;
    if (config->readBoolEntry( "offerShutdown", true )) {
        if (DM().canShutdown()) {
            maysd = true;
            mayrb = true;
        }
        else {
#ifdef WITH_TDEHWLIB
            TDERootSystemDevice* rootDevice = hwDevices->rootSystemDevice();
            if (rootDevice) {
                if (rootDevice->canPowerOff()) {
                    maysd = true;
                }
                if (rootDevice->canReboot()) {
                    mayrb = true;
                }
            }
#endif
        }
    }
    if (!maysd) {
        if (sdtype != TDEApplication::ShutdownTypeNone &&
            sdtype != TDEApplication::ShutdownTypeDefault &&
            sdtype != TDEApplication::ShutdownTypeReboot &&
            logoutConfirmed)
            return false; /* unsupported fast shutdown */
    }
    if (!mayrb) {
        if (sdtype != TDEApplication::ShutdownTypeNone &&
            sdtype != TDEApplication::ShutdownTypeDefault &&
            sdtype != TDEApplication::ShutdownTypeHalt &&
            logoutConfirmed)
            return false; /* unsupported fast shutdown */
    }

    return true;
}

void KSMServer::shutdownInternal( TDEApplication::ShutdownConfirm confirm,
                                  TDEApplication::ShutdownType sdtype,
                                  TDEApplication::ShutdownMode sdmode,
                                  TQString bopt )
{
    bool maysd = false;
    bool mayrb = false;
    bool logoutConfirmed = false;
    if ( !checkStatus( logoutConfirmed, maysd, mayrb, confirm, sdtype, sdmode ) ) {
        return;
    }

    TDEConfig *config = TDEGlobal::config();

    config->setGroup("General" );
    if ((!maysd) && (sdtype != TDEApplication::ShutdownTypeReboot)) {
        sdtype = TDEApplication::ShutdownTypeNone;
    }
    if ((!mayrb) && (sdtype != TDEApplication::ShutdownTypeHalt)) {
        sdtype = TDEApplication::ShutdownTypeNone;
    }
    if (sdtype == TDEApplication::ShutdownTypeDefault) {
        sdtype = (TDEApplication::ShutdownType) config->readNumEntry( "shutdownType", (int)TDEApplication::ShutdownTypeNone );
    }
    if (sdmode == TDEApplication::ShutdownModeDefault) {
        sdmode = TDEApplication::ShutdownModeInteractive;
    }

    // shall we show a logout status dialog box?
    bool showLogoutStatusDlg = TDEConfigGroup(TDEGlobal::config(), "Logout").readBoolEntry("showLogoutStatusDlg", true);

    if (showLogoutStatusDlg) {
        KSMShutdownIPFeedback::start();
    }

    dialogActive = true;
    if ( !logoutConfirmed ) {
        int selection;
        KSMShutdownFeedback::start(); // make the screen gray
        logoutConfirmed =
            KSMShutdownDlg::confirmShutdown( maysd, mayrb, sdtype, bopt, &selection );
        // ###### We can't make the screen remain gray while talking to the apps,
        // because this prevents interaction ("do you want to save", etc.)
        // TODO: turn the feedback widget into a list of apps to be closed,
        // with an indicator of the current status for each.
        KSMShutdownFeedback::stop(); // make the screen become normal again
        if (selection != 0) {
		// respect lock on resume & disable suspend/hibernate settings
		// from power-manager
		TDEConfig config("power-managerrc");
		bool lockOnResume = config.readBoolEntry("lockOnResume", true);
		if (lockOnResume) {
			TQCString replyType;
			TQByteArray replyData;
			// Block here until lock is complete
			// If this is not done the desktop of the locked session will be shown after suspend/hibernate until the lock fully engages!
			kapp->dcopClient()->call("kdesktop", "KScreensaverIface", "lock()", TQCString(""), replyType, replyData);
		}
#ifdef WITH_TDEHWLIB
		TDERootSystemDevice* rootDevice = hwDevices->rootSystemDevice();
		if (rootDevice) {
			if (selection == 1) {	// Suspend
				rootDevice->setPowerState(TDESystemPowerState::Suspend);
			}
			if (selection == 2) {	// Hibernate
				rootDevice->setPowerState(TDESystemPowerState::Hibernate);
			}
			if (selection == 3) {	// Freeze
				rootDevice->setPowerState(TDESystemPowerState::Freeze);
			}
		}
#endif
        }
    }

    if ( logoutConfirmed ) {
	SHUTDOWN_MARKER("Shutdown initiated");
	shutdownType = sdtype;
	shutdownMode = sdmode;
	bootOption = bopt;
	shutdownNotifierIPDlg = 0;
	if (showLogoutStatusDlg) {
		shutdownNotifierIPDlg = KSMShutdownIPDlg::showShutdownIP();
		if (shutdownNotifierIPDlg) {
			connect(shutdownNotifierIPDlg, SIGNAL(abortLogoutClicked()), this, SLOT(cancelShutdown()));
			connect(shutdownNotifierIPDlg, SIGNAL(skipNotificationClicked()), this, SLOT(forceSkipSaveYourself()));
			static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Notifying applications of logout request..."));
			notificationTimer.start( KSMSERVER_NOTIFICATION_MANUAL_OPTIONS_TIMEOUT, true );
		}
	}

        // shall we save the session on logout?
        saveSession = ( config->readEntry( "loginMode", "restorePreviousLogout" ) == "restorePreviousLogout" );

        if ( saveSession ) {
            sessionGroup = TQString("Session: ") + SESSION_PREVIOUS_LOGOUT;
        }

        // Set the real desktop background to black so that exit looks
        // clean regardless of what was on "our" desktop.
        if (!showLogoutStatusDlg) {
            TQT_TQWIDGET(kapp->desktop())->setBackgroundColor( Qt::black );
        }
        state = Shutdown;
        wmPhase1WaitingCount = 0;
        saveType = saveSession?SmSaveBoth:SmSaveGlobal;
	performLegacySessionSave();
	SHUTDOWN_MARKER("Legacy save complete");
        startProtection();
        for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
            c->resetState();
            // Whoever came with the idea of phase 2 got it backwards
            // unfortunately. Window manager should be the very first
            // one saving session data, not the last one, as possible
            // user interaction during session save may alter
            // window positions etc.
            // Moreover, KWin's focus stealing prevention would lead
            // to undesired effects while session saving (dialogs
            // wouldn't be activated), so it needs be assured that
            // KWin will turn it off temporarily before any other
            // user interaction takes place.
            // Therefore, make sure the WM finishes its phase 1
            // before others a chance to change anything.
            // KWin will check if the session manager is ksmserver,
            // and if yes it will save in phase 1 instead of phase 2.
            if( isWM( c )) {
                ++wmPhase1WaitingCount;
                SmsSaveYourself( c->connection(), saveType,
                             true, SmInteractStyleAny, false );
            }

        }
        if( wmPhase1WaitingCount == 0 ) { // no WM, simply start them all
            for ( KSMClient* c = clients.first(); c; c = clients.next() )
                SmsSaveYourself( c->connection(), saveType,
                             true, SmInteractStyleAny, false );
        }
        if ( clients.isEmpty() ) {
            completeShutdownOrCheckpoint();
        }
    }
    else {
       if (showLogoutStatusDlg) {
           KSMShutdownIPFeedback::stop();
       }
    }
    dialogActive = false;
}

void KSMServer::shutdown( TDEApplication::ShutdownConfirm confirm,
    TDEApplication::ShutdownType sdtype, TDEApplication::ShutdownMode sdmode )
{
    shutdownInternal( confirm, sdtype, sdmode );
}

#include <tdemessagebox.h>

void KSMServer::logoutTimed( int sdtype, int sdmode, TQString bootOption )
{
    int confirmDelay = 0;

    TDEConfig* config = TDEGlobal::config();
    config->reparseConfiguration(); // config may have changed in the KControl module
    config->setGroup( "General" );

    if ( sdtype == TDEApplication::ShutdownTypeHalt ) {
        confirmDelay = config->readNumEntry( "confirmShutdownDelay", 31 );
    }
    else if ( sdtype == TDEApplication::ShutdownTypeReboot ) {
        confirmDelay = config->readNumEntry( "confirmRebootDelay", 31 );
    }
    else {
        if(config->readBoolEntry("confirmLogout", true)) {
            confirmDelay = config->readNumEntry( "confirmLogoutDelay", 31 );
        }
    }

    bool result = true;
    if (confirmDelay > 0) {
        if(config->readBoolEntry("doFancyLogout", true)) {
            KSMShutdownFeedback::start(); // make the screen gray
        }
        result = KSMDelayedMessageBox::showTicker( (TDEApplication::ShutdownType)sdtype, bootOption, confirmDelay );
        if(config->readBoolEntry("doFancyLogout", true)) {
            KSMShutdownFeedback::stop(); // make the screen become normal again
        }
    }

    if ( result )
        shutdownInternal( TDEApplication::ShutdownConfirmNo,
                          (TDEApplication::ShutdownType)sdtype,
                          (TDEApplication::ShutdownMode)sdmode,
                           bootOption );
}

void KSMServer::pendingShutdownTimeout()
{
    shutdown( pendingShutdown_confirm, pendingShutdown_sdtype, pendingShutdown_sdmode );
}

void KSMServer::saveCurrentSession()
{
    if ( state != Idle || dialogActive )
        return;

    if ( currentSession().isEmpty() || currentSession() == SESSION_PREVIOUS_LOGOUT )
        sessionGroup = TQString("Session: ") + SESSION_BY_USER;

    state = Checkpoint;
    wmPhase1WaitingCount = 0;
    saveType = SmSaveLocal;
    saveSession = true;
    performLegacySessionSave();
    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        c->resetState();
        if( isWM( c )) {
            ++wmPhase1WaitingCount;
            SmsSaveYourself( c->connection(), saveType, false, SmInteractStyleNone, false );
        }
    }
    if( wmPhase1WaitingCount == 0 ) {
        for ( KSMClient* c = clients.first(); c; c = clients.next() )
            SmsSaveYourself( c->connection(), saveType, false, SmInteractStyleNone, false );
    }
    if ( clients.isEmpty() )
        completeShutdownOrCheckpoint();
}

void KSMServer::saveCurrentSessionAs( TQString session )
{
    if ( state != Idle || dialogActive )
        return;
    sessionGroup = "Session: " + session;
    saveCurrentSession();
}

// callbacks
void KSMServer::saveYourselfDone( KSMClient* client, bool success )
{
    if ( state == Idle ) {
        // State saving when it's not shutdown or checkpoint. Probably
        // a shutdown was cancelled and the client is finished saving
        // only now. Discard the saved state in order to avoid
        // the saved data building up.
        TQStringList discard = client->discardCommand();
        if( !discard.isEmpty())
            executeCommand( discard );
        return;
    }
    if ( success ) {
        client->saveYourselfDone = true;
        completeShutdownOrCheckpoint();
    } else {
        // fake success to make KDE's logout not block with broken
        // apps. A perfect ksmserver would display a warning box at
        // the very end.
        client->saveYourselfDone = true;
        completeShutdownOrCheckpoint();
    }
    startProtection();
    if( isWM( client ) && !client->wasPhase2 && wmPhase1WaitingCount > 0 ) {
        --wmPhase1WaitingCount;
        // WM finished its phase1, save the rest
        if( wmPhase1WaitingCount == 0 ) {
            for ( KSMClient* c = clients.first(); c; c = clients.next() )
                if( !isWM( c ))
                    SmsSaveYourself( c->connection(), saveType, saveType != SmSaveLocal,
                        saveType != SmSaveLocal ? SmInteractStyleAny : SmInteractStyleNone,
                        false );
        }
    }

    notificationTimer.stop();
    if (shutdownNotifierIPDlg) {
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->hideNotificationActionButtons();
    }

    updateLogoutStatusDialog();
}

void KSMServer::updateLogoutStatusDialog()
{
    bool inPhase2 = true;
    bool pendingInteraction = false;
    for( KSMClient* c = clients.first(); c; c = clients.next()) {
        if ( !c->saveYourselfDone && !c->waitForPhase2 ) {
            inPhase2 = false;
        }
        if ( c->pendingInteraction ) {
            pendingInteraction = true;
        }
    }
    if (clientInteracting) {
        pendingInteraction = true;
    }

    if (shutdownNotifierIPDlg) {
        int waitingClients = 0;
        TQString nextClientToKill;
        TQDateTime currentDateTime = TQDateTime::currentDateTime();
        TQDateTime oldestFoundDateTime = currentDateTime;
        for( KSMClient* c = clients.first(); c; c = clients.next()) {
            if (c->saveYourselfDone) {
                continue;
            }
            if( isWM( c ) || isCM( c ) || isNotifier( c ) || isDesktop( c ) ) {
                continue;
            }
            waitingClients++;
            if (c->program() != "") {
                if (c->terminationRequestTimeStamp < oldestFoundDateTime) {
                    nextClientToKill = c->program();
                    oldestFoundDateTime = c->terminationRequestTimeStamp;
                }
            }
        }
        if (inPhase2) {
            if (phase2ClientCount > 0) {
                if (!notificationTimer.isActive()) {
                    notificationTimer.start( KSMSERVER_NOTIFICATION_MANUAL_OPTIONS_TIMEOUT, true );
                }
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setNotificationActionButtonsSkipText(i18n("Skip Notification (%1)").arg(((KSMSERVER_SHUTDOWN_CLIENT_UNRESPONSIVE_TIMEOUT - (protectionTimerCounter*1000))/1000)+1));
                if (nextClientToKill == "") {
                    static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Notifying remaining applications of logout request (%1/%2)...").arg(phase2ClientCount-waitingClients).arg(phase2ClientCount));
                }
                else {
                     static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Notifying remaining applications of logout request (%1/%2, %3)...").arg(phase2ClientCount-waitingClients).arg(phase2ClientCount).arg(nextClientToKill));
                }
            }
        }
        else {
            if (pendingInteraction) {
#if 0
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setNotificationActionButtonsSkipText(i18n("Ignore and Resume Logout"));
#else
                // Hide dialog and buttons
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->hide();
                notificationTimer.stop();
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->hideNotificationActionButtons();
#endif
                if (nextClientToKill == "") {
                    static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("An application is requesting attention, logout paused..."));
                }
                else {
                    static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("%3 is requesting attention, logout paused...").arg(nextClientToKill));
                }
            }
            else {
                if (!notificationTimer.isActive()) {
                    notificationTimer.start( KSMSERVER_NOTIFICATION_MANUAL_OPTIONS_TIMEOUT, true );
                }
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setNotificationActionButtonsSkipText(i18n("Skip Notification (%1)").arg(((KSMSERVER_SHUTDOWN_CLIENT_UNRESPONSIVE_TIMEOUT - (protectionTimerCounter*1000))/1000)+1));
                if (nextClientToKill == "") {
                    static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Notifying applications of logout request (%1/%2)...").arg(clients.count()-waitingClients).arg(clients.count()));
                }
                else {
                    static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Notifying applications of logout request (%1/%2, %3)...").arg(clients.count()-waitingClients).arg(clients.count()).arg(nextClientToKill));
                }
            }
        }
    }
}

void KSMServer::interactRequest( KSMClient* client, int /*dialogType*/ )
{
    if ( state == Shutdown )
        client->pendingInteraction = true;
    else
        SmsInteract( client->connection() );

    handlePendingInteractions();
}

void KSMServer::interactDone( KSMClient* client, bool cancelShutdown_ )
{
    if ( client != clientInteracting )
        return; // should not happen
    clientInteracting = 0;
    if ( cancelShutdown_ )
        cancelShutdown( client );
    else
        handlePendingInteractions();
}


void KSMServer::phase2Request( KSMClient* client )
{
    client->waitForPhase2 = true;
    client->wasPhase2 = true;
    completeShutdownOrCheckpoint();
    if( isWM( client ) && wmPhase1WaitingCount > 0 ) {
        --wmPhase1WaitingCount;
        // WM finished its phase1 and requests phase2, save the rest
        if( wmPhase1WaitingCount == 0 ) {
            for ( KSMClient* c = clients.first(); c; c = clients.next() )
                if( !isWM( c ))
                    SmsSaveYourself( c->connection(), saveType, saveType != SmSaveLocal,
                        saveType != SmSaveLocal ? SmInteractStyleAny : SmInteractStyleNone,
                        false );
        }
    }
}

void KSMServer::handlePendingInteractions()
{
    if ( clientInteracting )
        return;

    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        if ( c->pendingInteraction ) {
            clientInteracting = c;
            c->pendingInteraction = false;
            break;
        }
    }
    if ( clientInteracting ) {
        endProtection();
        SmsInteract( clientInteracting->connection() );
    } else {
        startProtection();
    }
}

void KSMServer::cancelShutdown( TQString cancellationText )
{
    if (shutdownNotifierIPDlg) {
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->closeSMDialog();
        shutdownNotifierIPDlg=0;
    }
    KNotifyClient::event( 0, "cancellogout", cancellationText);
    clientInteracting = 0;
    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        SmsShutdownCancelled( c->connection() );
        if( c->saveYourselfDone ) {
            // Discard also saved state.
            TQStringList discard = c->discardCommand();
            if( !discard.isEmpty())
                executeCommand( discard );
        }
    }
    state = Idle;
}

void KSMServer::cancelShutdown( KSMClient* c )
{
    kdDebug( 1218 ) << "Client " << c->program() << " (" << c->clientId() << ") canceled shutdown." << endl;
    cancelShutdown(i18n( "Logout canceled by '%1'" ).arg( c->program()));
}

void KSMServer::cancelShutdown()
{
    kdDebug( 1218 ) << "User canceled shutdown." << endl;
    cancelShutdown(i18n( "Logout canceled by user" ));
}

void KSMServer::startProtection()
{
    protectionTimerCounter = 0;
    protectionTimer.start( 1000, true );
}

void KSMServer::endProtection()
{
    protectionTimerCounter = 0;
    protectionTimer.stop();
}

void KSMServer::protectionTimerTick()
{
    protectionTimerCounter++;
    if ((protectionTimerCounter*1000) > KSMSERVER_SHUTDOWN_CLIENT_UNRESPONSIVE_TIMEOUT) {
        protectionTimerCounter = 0;
        protectionTimeout();
    }
    else {
        protectionTimer.start( 1000, true );
    }
    updateLogoutStatusDialog();
}

/*
   Internal protection slot, invoked when clients do not react during
  shutdown.
 */
void KSMServer::protectionTimeout()
{
    if ( ( state != Shutdown && state != Checkpoint ) || clientInteracting )
        return;

    handleProtectionTimeout();

    startProtection();
}

void KSMServer::forceSkipSaveYourself()
{
    SHUTDOWN_MARKER("forceSkipSaveYourself");

    handleProtectionTimeout();

    startProtection();
}

void KSMServer::handleProtectionTimeout()
{
    SHUTDOWN_MARKER("handleProtectionTimeout");

    notificationTimer.stop();
    if (shutdownNotifierIPDlg) {
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->hideNotificationActionButtons();
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Forcing interacting application termination").append("..."));
    }

    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        if ( !c->saveYourselfDone && !c->waitForPhase2 ) {
            kdDebug( 1218 ) << "protectionTimeout: client " << c->program() << "(" << c->clientId() << ")" << endl;
            c->saveYourselfDone = true;
        }
    }
    completeShutdownOrCheckpoint();
}

void KSMServer::notificationTimeout()
{
	if (shutdownNotifierIPDlg) {
		// Display the buttons in the logout dialog
		static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->showNotificationActionButtons();
	}
}

void KSMServer::completeShutdownOrCheckpoint()
{
    SHUTDOWN_MARKER("completeShutdownOrCheckpoint");
    if ( state != Shutdown && state != Checkpoint ) {
        SHUTDOWN_MARKER("completeShutdownOrCheckpoint state not Shutdown or Checkpoint");
        return;
    }

    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        if ( !c->saveYourselfDone && !c->waitForPhase2 ) {
            SHUTDOWN_MARKER("completeShutdownOrCheckpoint state not done yet");
            return; // not done yet
        }
    }

    // do phase 2
    phase2ClientCount = 0;
    bool waitForPhase2 = false;
    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        if ( !c->saveYourselfDone && c->waitForPhase2 ) {
            c->waitForPhase2 = false;
            phase2ClientCount++;
            SmsSaveYourselfPhase2( c->connection() );
            waitForPhase2 = true;
        }
    }
    if ( waitForPhase2 ) {
        SHUTDOWN_MARKER("completeShutdownOrCheckpoint state still waiting for Phase 2");
        if (shutdownNotifierIPDlg) {
            static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
            static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Notifying remaining applications of logout request..."));
            notificationTimer.start( KSMSERVER_NOTIFICATION_MANUAL_OPTIONS_TIMEOUT, true );
        }
        return;
    }
    SHUTDOWN_MARKER("Phase 2 complete");

    bool showLogoutStatusDlg = TDEConfigGroup(TDEGlobal::config(), "Logout").readBoolEntry("showLogoutStatusDlg", true);
    if (showLogoutStatusDlg && state != Checkpoint) {
        KSMShutdownIPFeedback::showit(); // hide the UGLY logout process from the user
        if (!shutdownNotifierIPDlg) {
            shutdownNotifierIPDlg = KSMShutdownIPDlg::showShutdownIP();
            if (shutdownNotifierIPDlg) {
                connect(shutdownNotifierIPDlg, SIGNAL(abortLogoutClicked()), this, SLOT(cancelShutdown()));
                connect(shutdownNotifierIPDlg, SIGNAL(skipNotificationClicked()), this, SLOT(forceSkipSaveYourself()));
            }
        }
        while (!KSMShutdownIPFeedback::ispainted()) {
            tqApp->processEvents();
        }
    }

    notificationTimer.stop();
    if (shutdownNotifierIPDlg) {
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->hideNotificationActionButtons();
    }

    // synchronize any folders that were requested for shutdown sync
    if (shutdownNotifierIPDlg) {
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Synchronizing remote folders").append("..."));
    }
    KRsync krs(this, "");
    krs.executeLogoutAutoSync();
    if (shutdownNotifierIPDlg) {
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Saving your settings..."));
    }

    if ( saveSession ) {
        storeSession();
        SHUTDOWN_MARKER("Session stored");
    }
    else {
        discardSession();
        SHUTDOWN_MARKER("Session discarded");
    }

    if ( state == Shutdown ) {
        bool waitForKNotify = true;
        if( !kapp->dcopClient()->connectDCOPSignal( "knotify", "",
            "notifySignal(TQString,TQString,TQString,TQString,TQString,int,int,int,int)",
            "ksmserver", "notifySlot(TQString,TQString,TQString,TQString,TQString,int,int,int,int)", false )) {
            waitForKNotify = false;
        }
        if( !kapp->dcopClient()->connectDCOPSignal( "knotify", "",
            "playingFinished(int,int)",
            "ksmserver", "logoutSoundFinished(int,int)", false )) {
            waitForKNotify = false;
        }
        // event() can return -1 if KNotifyClient short-circuits and avoids KNotify
        logoutSoundEvent = KNotifyClient::event( 0, "exittde" ); // TDE says good bye
        if( logoutSoundEvent <= 0 ) {
            waitForKNotify = false;
        }
        initialClientCount = clients.count();
        if (shutdownNotifierIPDlg) {
            static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
            TQString nextClientToKill;
            TQDateTime currentDateTime = TQDateTime::currentDateTime();
            TQDateTime oldestFoundDateTime = currentDateTime;
            for( KSMClient* c = clients.first(); c; c = clients.next()) {
                if( isWM( c ) || isCM( c ) || isNotifier( c ) || isDesktop( c ) ) {
                    continue;
                }
                if (c->program() != "") {
                    if (c->terminationRequestTimeStamp < oldestFoundDateTime) {
                        nextClientToKill = c->program();
                        oldestFoundDateTime = c->terminationRequestTimeStamp;
                    }
                }
            }
            KSMShutdownIPDlg *shutdownNotifierDlg=static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg);
            shutdownNotifierDlg->setProgressBarTotalSteps(initialClientCount);
            shutdownNotifierDlg->setProgressBarProgress(initialClientCount-clients.count());
            if (nextClientToKill == "") {
                shutdownNotifierDlg->setStatusMessage(i18n("Closing applications (%1/%2)...").arg(initialClientCount-clients.count()).arg(initialClientCount));
            }
            else {
                shutdownNotifierDlg->setStatusMessage(i18n("Closing applications (%1/%2, %3)...").arg(initialClientCount-clients.count()).arg(initialClientCount).arg(nextClientToKill));
            }
        }
        if( waitForKNotify ) {
            state = WaitingForKNotify;
            knotifyTimeoutTimer.start( 20000, true );
            return;
        }
        startKilling();
    }
    else if ( state == Checkpoint ) {
        for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
            SmsSaveComplete( c->connection());
        }
        state = Idle;
    }
    SHUTDOWN_MARKER("Fully shutdown");
}

void KSMServer::startKilling()
{
    SHUTDOWN_MARKER("startKilling");
    knotifyTimeoutTimer.stop();
    // kill all clients
    state = Killing;
    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        if( isWM( c ) || isCM( c ) || isNotifier( c ) || isDesktop( c ) ) { // kill the WM and CM as the last one in order to reduce flicker.  Also wait to kill knotify to avoid logout delays
            continue;
        }
        kdDebug( 1218 ) << "completeShutdown: client " << c->program() << "(" << c->clientId() << ")" << endl;
        c->terminationRequestTimeStamp = TQDateTime::currentDateTime();
        SmsDie( c->connection() );
    }

    kdDebug( 1218 ) << " We killed all clients. We have now clients.count()=" << clients.count() << endl;
    completeKilling();
    shutdownTimer.start( KSMSERVER_SHUTDOWN_CLIENT_UNRESPONSIVE_TIMEOUT, TRUE );
}

void KSMServer::completeKilling()
{
    // Activity detected; reset forcible shutdown timer...
    if (shutdownTimer.isActive()) {
        shutdownTimer.start( KSMSERVER_SHUTDOWN_CLIENT_UNRESPONSIVE_TIMEOUT, TRUE );
    }
    SHUTDOWN_MARKER("completeKilling");
    kdDebug( 1218 ) << "KSMServer::completeKilling clients.count()=" << clients.count() << endl;
    if( state == Killing ) {
        bool wait = false;
        TQString nextClientToKill;
        TQDateTime currentDateTime = TQDateTime::currentDateTime();
        TQDateTime oldestFoundDateTime = currentDateTime;
        for( KSMClient* c = clients.first(); c; c = clients.next()) {
            if( isWM( c ) || isCM( c ) || isNotifier( c ) || isDesktop( c ) ) {
                continue;
            }
            if (c->program() != "") {
                if (c->terminationRequestTimeStamp < oldestFoundDateTime) {
                    nextClientToKill = c->program();
                    oldestFoundDateTime = c->terminationRequestTimeStamp;
                }
                wait = true; // still waiting for clients to go away
            }
        }
        if( wait ) {
            if (shutdownNotifierIPDlg) {
                KSMShutdownIPDlg *shutdownNotifierDlg=static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg);
                shutdownNotifierDlg->setProgressBarTotalSteps(initialClientCount);
                shutdownNotifierDlg->setProgressBarProgress(initialClientCount-clients.count());
                shutdownNotifierDlg->show();
                if (nextClientToKill == "") {
                    shutdownNotifierDlg->setStatusMessage(i18n("Closing applications (%1/%2)...").arg(initialClientCount-clients.count()).arg(initialClientCount));
                }
                else {
                    shutdownNotifierDlg->setStatusMessage(i18n("Closing applications (%1/%2, %3)...").arg(initialClientCount-clients.count()).arg(initialClientCount).arg(nextClientToKill));
                }
            }
            return;
        }
        else {
            if (shutdownNotifierIPDlg) {
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->show();
                static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->setStatusMessage(i18n("Terminating services..."));
            }
        }
        killWM();
    }
}

void KSMServer::killWM()
{
    SHUTDOWN_MARKER("killWM");
    state = KillingWM;
    bool iswm = false;
    if (shutdownNotifierIPDlg) {
        static_cast<KSMShutdownIPDlg*>(shutdownNotifierIPDlg)->closeSMDialog();
        shutdownNotifierIPDlg=0;
    }
    for ( KSMClient* c = clients.first(); c; c = clients.next() ) {
        if( isDesktop( c )) {
            iswm = true;
            c->terminationRequestTimeStamp = TQDateTime::currentDateTime();
            SmsDie( c->connection() );
        }
        if( isNotifier( c )) {
            iswm = true;
            c->terminationRequestTimeStamp = TQDateTime::currentDateTime();
            SmsDie( c->connection() );
        }
        if( isCM( c )) {
            iswm = true;
            c->terminationRequestTimeStamp = TQDateTime::currentDateTime();
            SmsDie( c->connection() );
        }
        if( isWM( c )) {
            iswm = true;
            kdDebug( 1218 ) << "killWM: client " << c->program() << "(" << c->clientId() << ")" << endl;
            c->terminationRequestTimeStamp = TQDateTime::currentDateTime();
            SmsDie( c->connection() );
        }
    }
    if( iswm ) {
        completeKillingWM();
        TQTimer::singleShot( 5000, this, TQT_SLOT( timeoutWMQuit() ) );
    }
    else {
        killingCompleted();
    }
}

void KSMServer::completeKillingWM()
{
    SHUTDOWN_MARKER("completeKillingWM");
    kdDebug( 1218 ) << "KSMServer::completeKillingWM clients.count()=" << clients.count() << endl;
    if( state == KillingWM ) {
        if( clients.isEmpty()) {
            killingCompleted();
        }
    }
}

// shutdown is fully complete
void KSMServer::killingCompleted()
{
    SHUTDOWN_MARKER("killingCompleted");
    DM dmObject;
    int dmType = dmObject.type();
    if ((dmType == DM::NewTDM) || (dmType == DM::OldTDM) || (dmType == DM::GDM)) {
        // Hide any remaining windows until the X server is terminated by the display manager
        pid_t child;
        child = fork();
        if (child != 0) {
            kapp->quit();
        }
        else if (child == 0) {
            // If any remaining client(s) do not exit quickly (e.g. drkonqui) terminate so that they can be seen and interacted with
            sleep(30);
            exit(0);
        }
    }
    else {
        kapp->quit();
    }
}

// called when KNotify performs notification for logout (not when sound is finished though)
void KSMServer::notifySlot(TQString event ,TQString app,TQString,TQString,TQString,int present,int,int,int)
{
    SHUTDOWN_MARKER("notifySlot");
    if( state != WaitingForKNotify ) {
        SHUTDOWN_MARKER("notifySlot state != WaitingForKNotify");
        return;
    }
    if( event != "exittde" || app != "ksmserver" ) {
        SHUTDOWN_MARKER("notifySlot event != \"exittde\" || app != \"ksmserver\"");
        return;
    }
    if( present & KNotifyClient::Sound ) { // logoutSoundFinished() will be called
        SHUTDOWN_MARKER("notifySlot present & KNotifyClient::Sound");
        return;
    }
    startKilling();
}

// This is stupid. The normal DCOP signal connected to notifySlot() above should be simply
// emitted in KNotify only after the sound is finished playing.
void KSMServer::logoutSoundFinished( int event, int )
{
    SHUTDOWN_MARKER("logoutSoundFinished");
    if( state != WaitingForKNotify ) {
        return;
    }
    if( event != logoutSoundEvent ) {
        return;
    }
    startKilling();
}

void KSMServer::knotifyTimeout()
{
    SHUTDOWN_MARKER("knotifyTimeout");
    if( state != WaitingForKNotify ) {
        return;
    }
    startKilling();
}

void KSMServer::timeoutQuit()
{
    SHUTDOWN_MARKER("timeoutQuit");
    for (KSMClient *c = clients.first(); c; c = clients.next()) {
        SHUTDOWN_MARKER(TQString("SmsDie timeout, client %1 (%2)").arg(c->program()).arg(c->clientId()).ascii());
        kdWarning( 1218 ) << "SmsDie timeout, client " << c->program() << "(" << c->clientId() << ")" << endl;
    }
    killWM();
}

void KSMServer::timeoutWMQuit()
{
    SHUTDOWN_MARKER("timeoutWMQuit");
    if( state == KillingWM ) {
        kdWarning( 1218 ) << "SmsDie WM timeout" << endl;
    }
    killingCompleted();
}
