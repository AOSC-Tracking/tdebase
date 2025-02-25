/*****************************************************************
 * drkonqi - The KDE Crash Handler
 *
 * Copyright (C) 2000-2003 Hans Petter Bieker <bieker@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************/

#include <tqstring.h>
#include <tqlabel.h>
#include <tqhbox.h>

#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdestandarddirs.h>
#include <kbugreport.h>
#include <tdefiledialog.h>
#include <tdemessagebox.h>
#include <tdeprocess.h>
#include <tdeapplication.h>
#include <dcopclient.h>
#include <tdetempfile.h>

#include "netwm.h"

#include "backtrace.h"
#include "drbugreport.h"
#include "bugdescription.h"
#include "debugger.h"
#include "krashconf.h"
#include "sha1.h"
#include "toplevel.h"
#include "toplevel.moc"

Toplevel :: Toplevel(KrashConfig *krashconf, TQWidget *parent, const char *name)
  : KDialogBase( Tabbed,
                 krashconf->programName(),
                 User3 | User2 | User1 | Close,
                 Close,
                 parent,
                 name,
                 true, // modal
                 false, // no separator
                 i18n("&Bug report"),
                 i18n("&Debugger"),
                 i18n("&Report Crash")
                 ),
    m_krashconf(krashconf), m_bugreport(0), m_bugdescription(0)
{
  TQHBox *page = addHBoxPage(i18n("&General"));
  page->setSpacing(20);

  // picture of konqi
  TQLabel *lab = new TQLabel(page);
  lab->setFrameStyle(TQFrame::Panel | TQFrame::Sunken);
  TQPixmap pix(locate("appdata", TQString::fromLatin1("pics/konqi.png")));
  lab->setPixmap(pix);
  lab->setFixedSize( lab->sizeHint() );

  TQLabel * info = new TQLabel(generateText(), page);
  info->setMinimumSize(info->sizeHint());

  if (m_krashconf->showBacktrace())
  {
    page = addHBoxPage(i18n("&Backtrace"));
    new KrashDebugger(m_krashconf, page);
  }

  showButton( User1, m_krashconf->showBugReport() );
  showButton( User2, m_krashconf->showDebugger() );
  showButton( User3, true );

  connect(this, TQ_SIGNAL(closeClicked()), TQ_SLOT(accept()));
  connect(m_krashconf, TQ_SIGNAL(newDebuggingApplication(const TQString&)), TQ_SLOT(slotNewDebuggingApp(const TQString&)));

  if ( !m_krashconf->safeMode() && tdeApp->dcopClient()->attach() )
    tdeApp->dcopClient()->registerAs( tdeApp->name() );
}

Toplevel :: ~Toplevel()
{
}

TQString Toplevel :: generateText() const
{
  TQString str;

  if (!m_krashconf->errorDescriptionText().isEmpty())
    str += i18n("<p><b>Short description</b></p><p>%1</p>")
      .arg(m_krashconf->errorDescriptionText());

  if (!m_krashconf->signalText().isEmpty())
    str += i18n("<p><b>What is this?</b></p><p>%1</p>")
      .arg(m_krashconf->signalText());

  if (!m_krashconf->whatToDoText().isEmpty())
    str += i18n("<p><b>What can I do?</b></p><p>%1</p>")
      .arg(m_krashconf->whatToDoText());

  // check if the string is still empty. if so, display a default.
  if (str.isEmpty())
    str = i18n("<p><b>Application crashed</b></p>"
               "<p>The program %appname crashed.</p>");

  // scan the string for %appname etc
  m_krashconf->expandString(str, false);

  return str;
}

// starting bug report
void Toplevel :: slotUser1()
{
  if (m_bugreport)
    return;

  int i = KMessageBox::No;
  if ( m_krashconf->pid() != 0 )
    i = KMessageBox::warningYesNoCancel
      (0,
       i18n("<p>Do you want to generate a "
            "backtrace? This will help the "
            "developers to figure out what went "
            "wrong.</p>\n"
            "<p>Unfortunately this will take some "
            "time on slow machines.</p>"
            "<p><b>Note: A backtrace is not a "
            "substitute for a proper description "
            "of the bug and information on how to "
            "reproduce it. It is not possible "
            "to fix the bug without a proper "
            "description.</b></p>"),
       i18n("Include Backtrace"),i18n("Generate"),i18n("Do Not Generate"));

    if (i == KMessageBox::Cancel) return;

  m_bugreport = new DrKBugReport(0, true, m_krashconf->aboutData());

  if (i == KMessageBox::Yes) {
    TQApplication::setOverrideCursor ( TQt::waitCursor );

    // generate the backtrace
    BackTrace *backtrace = new BackTrace(m_krashconf, this);
    connect(backtrace, TQ_SIGNAL(someError()), TQ_SLOT(slotBacktraceSomeError()));
    connect(backtrace, TQ_SIGNAL(done(const TQString &)), TQ_SLOT(slotBacktraceDone(const TQString &)));

    backtrace->start();

    return;
  }

  int result = m_bugreport->exec();
  delete m_bugreport;
  m_bugreport = 0;
  if (result == KDialogBase::Accepted)
     close();
}

void Toplevel :: slotUser2()
{
  TQString str = m_krashconf->debugger();
  m_krashconf->expandString(str, true);

  TDEProcess proc;
  proc.setUseShell(true);
  proc << str;
  proc.start(TDEProcess::DontCare);
}

void Toplevel :: slotNewDebuggingApp(const TQString& launchName)
{
  setButtonText( User3, launchName );
  showButton( User3, true );
}

void Toplevel :: slotUser3()
{
	enableButton(User3, false);
	TQApplication::setOverrideCursor ( TQt::waitCursor );

	// generate the backtrace
	BackTrace *backtrace = new BackTrace(m_krashconf, this);
	connect(backtrace, TQ_SIGNAL(someError()), TQ_SLOT(slotSendReportBacktraceSomeError()));
	connect(backtrace, TQ_SIGNAL(done(const TQString &)), TQ_SLOT(slotSendReportBacktraceDone(const TQString &)));

	backtrace->start();

	return;
}

void Toplevel :: slotBacktraceDone(const TQString &str)
{
  // Do not translate.. This will be included in the _MAIL_.
  TQString buf = TQString::fromLatin1
    ("\n\n\nHere is a backtrace generated by DrKonqi:\n") + str;

  m_bugreport->setText(buf);

  TQApplication::restoreOverrideCursor();

  m_bugreport->exec();
  delete m_bugreport;
  m_bugreport = 0;
}

void Toplevel :: slotBacktraceSomeError()
{
  TQApplication::restoreOverrideCursor();

  KMessageBox::sorry(0, i18n("It was not possible to generate a backtrace."),
                     i18n("Backtrace Not Possible"));

  m_bugreport->exec();
  delete m_bugreport;
  m_bugreport = 0;
}

void Toplevel::slotSendReportBacktraceSomeError()
{
	TQApplication::restoreOverrideCursor();

	KMessageBox::sorry(0, i18n("It was not possible to generate a backtrace."), i18n("Backtrace Not Possible"));

	delete m_bugdescription;
	m_bugdescription = 0;

	enableButton(User3, true);
}

void Toplevel::slotSendReportBacktraceDone(const TQString &str)
{
	int i = KMessageBox::No;
	if ( m_krashconf->pid() != 0 ) {
		i = KMessageBox::warningYesNoCancel
		(0,
		i18n("<p>Do you want to include a "
			"description of what you were doing "
			"when this application crashed? This "
			"would help the "
			"developers to figure out what went "
			"wrong.</p>\n"),
		i18n("Include Description"),i18n("Add Description"),i18n("Just Report the Crash"));
	}

	if (i == KMessageBox::Cancel) {
		TQApplication::restoreOverrideCursor();
		enableButton(User3, true);

		return;
	}

	m_bugdescription = new BugDescription(0, true, m_krashconf->aboutData());

	if (i == KMessageBox::Yes) {
		// Get description
		// Also get Email address if desired
		// Possibly reduce hash difficulty if Email address provided?
		// BugDescription
		if (m_bugdescription->exec() == TQDialog::Rejected) {
			delete m_bugdescription;
			m_bugdescription = 0;

			return;
		}
	}

	// Get automatic system information
	TQString autoSystemInformation;
	KBugReport* kbugreport = new KBugReport(0, true, m_krashconf->aboutData());
	autoSystemInformation += "Application: ";
	autoSystemInformation += m_krashconf->appName();
	autoSystemInformation += "\n";
	autoSystemInformation += "Signal: ";
	autoSystemInformation += TQString("%1").arg(m_krashconf->signalNumber());
	autoSystemInformation += "\n";
	autoSystemInformation += "Compiler: ";
	autoSystemInformation += kbugreport->compilerVersion();
	autoSystemInformation += "\n";
	autoSystemInformation += "Kernel: ";
	autoSystemInformation += kbugreport->operatingSystem();
	autoSystemInformation += "\n";
	autoSystemInformation += "TDE Version: ";
	autoSystemInformation += kbugreport->tdeVersion();
	autoSystemInformation += "\n";
	autoSystemInformation += "Timestamp: ";
	autoSystemInformation += TQString("%1").arg(TQDateTime::currentDateTime().toTime_t());
	autoSystemInformation += "\n";
	delete kbugreport;
	kbugreport = 0;

	// Generate automatic crash description
	TQString autoCrashDescription = m_krashconf->errorDescriptionText();
	m_krashconf->expandString(autoCrashDescription, false);

	// Generate full crash report
	TQString backtraceSubmission = str;
	backtraceSubmission.append("\n==== (tdebugreport) automatic crash description ====\n");
	backtraceSubmission.append(TQString("%1\n").arg(autoCrashDescription));
	backtraceSubmission.append("\n==== (tdebugreport) automatic system description ====\n");
	backtraceSubmission.append(TQString("%1\n").arg(autoSystemInformation));
	if (m_bugdescription->emailAddress().contains("@") && m_bugdescription->emailAddress().contains(".")) {
		backtraceSubmission.append("\n==== (tdebugreport) reporting Email address ====\n");
		backtraceSubmission.append(TQString("%1\n").arg(m_bugdescription->emailAddress()));
	}
	if (m_bugdescription->crashDescription() != "") {
		backtraceSubmission.append("\n==== (tdebugreport) user-generated crash description ====\n");
		backtraceSubmission.append(TQString("%1\n").arg(m_bugdescription->crashDescription()));
	}

	// Calculate proof-of-work hash
	SHA1 sha;
	TQByteArray hash(sha.size() / 8);
	hash.fill(255);

	backtraceSubmission.append("\n==== (tdebugreport) proof of work ====\n");
	int proofOfWorkPos = backtraceSubmission.length();
	backtraceSubmission.append(TQUuid::createUuid().toString());
	m_backtraceSubmissionData = TQCString(backtraceSubmission.ascii());

	while ((hash[0] != 0) || ((hash[1] & 0xfc) != 0)) {	// First 14 bits of the SHA1 hash must be zero
		TQCString proofOfWork(TQUuid::createUuid().toString().ascii());
		memcpy(m_backtraceSubmissionData.data() + proofOfWorkPos, proofOfWork.data(), proofOfWork.size());
		sha.reset();
		sha.process(m_backtraceSubmissionData.data(), m_backtraceSubmissionData.size()-1);
		memcpy(hash.data(), sha.hash(), hash.size());
	}

	TQApplication::restoreOverrideCursor();

	i = KMessageBox::Yes;
	while (i == KMessageBox::Yes) {
		i = KMessageBox::warningYesNoCancel
			(0,
			i18n("<p>The crash report is ready.  Do you want to send it now?</p>\n"),
			i18n("Ready to Send"),i18n("View Report"),i18n("Send Report"));

		if (i == KMessageBox::Cancel) {
			delete m_bugdescription;
			m_bugdescription = 0;
			enableButton(User3, true);

			return;
		}

		if (i == KMessageBox::Yes) {
			BugDescription fullReport(0, true, NULL);
			fullReport.fullReportViewMode(true);
			fullReport.setText(TQString(m_backtraceSubmissionData.data()));
			fullReport.showMaximized();
			fullReport.exec();
		}
	}

	postCrashDataToServer(m_backtraceSubmissionData);

	delete m_bugdescription;
	m_bugdescription = 0;
}

int Toplevel::postCrashDataToServer(TQCString data) {
	m_serverResponse = "";
	TQCString formDataBoundary = "-----------------------------------DrKonqiCrashReporterBoundary";

	TQCString postData;
	postData += "--";
	postData += formDataBoundary;
	postData += "\r\n";
	postData += "Content-Disposition: form-data; name=\"crashreport\"; filename=\"crashreport.txt\"\r\n";
	postData += "Content-Type: application/octet-stream\r\n";
	postData += (TQString("Content-Length: %1\r\n").arg(data.count())).ascii();
	postData += "Content-Transfer-Encoding: binary\r\n\r\n";
	postData += data;
	postData += "\r\n";
	postData += "--";
	postData += formDataBoundary;
	postData += "--\r\n";

	KURL url("https://crashreport.trinitydesktop.org/");
// 	TDEIO::TransferJob* job = TDEIO::http_post(url, postData, false);
	TDEIO::TransferJob* job = TDEIO::http_post(url, postData, true);
	job->addMetaData("content-type", TQString("Content-Type: multipart/form-data; boundary=%1").arg(formDataBoundary));
	job->addMetaData("referrer", "http://drkonqi-client.crashreport.trinitydesktop.org");
	connect(job, TQ_SIGNAL(data(TDEIO::Job *, const TQByteArray &)), TQ_SLOT(postCrashDataToServerData(TDEIO::Job *, const TQByteArray &)));
	connect(job, TQ_SIGNAL(result(TDEIO::Job *)), TQ_SLOT(postCrashDataToServerResult(TDEIO::Job *)));
// 	connect(job, TQ_SIGNAL(totalSize(TDEIO::Job *, TDEIO::filesize_t )),
// 		TQ_SLOT(totalSize(TDEIO::Job *, TDEIO::filesize_t)));
// 	connect(job, TQ_SIGNAL(mimetype(TDEIO::Job *, const TQString &)),
// 		TQ_SLOT(mimetype(TDEIO::Job *, const TQString &)));
	connect(job, TQ_SIGNAL(redirection(TDEIO::Job *, const KURL&)), TQ_SLOT(postCrashDataToServerDataRedirection(TDEIO::Job *, const KURL&)));

	return 0;
}

void Toplevel::postCrashDataToServerData(TDEIO::Job *, const TQByteArray &ba)
{
	uint offset = 0;
	if (m_serverResponse.count() > 0) {
		offset = m_serverResponse.count() - 1;
	}
	uint size = ba.count();

	m_serverResponse.resize(offset + size + 1);
	memcpy(m_serverResponse.data() + offset, ba.data(), size);
	*(m_serverResponse.data() + offset + size) = 0;
}

void Toplevel::postCrashDataToServerResult(TDEIO::Job *job)
{
	int err = job->error();
	if (err == 0) {
		TQString responseString(m_serverResponse);
		if (responseString.startsWith("ACK\n")) {
			responseString = responseString.mid(4);
			KMessageBox::information
			(0,
			i18n("<p>Your crash report has been uploaded!</p><p>You may reference it if desired by its unique ID:<br>%1</p>").arg(responseString),
			i18n("Report uploaded"));
			close();
		}
		else {
			responseString = responseString.mid(4);
// 			KMessageBox::error
// 			(0,
// 			i18n("<p>Your crash report failed to upload!</p><p>Please check your network settings and try again.</p><p>The server responded:<br>%1</p>").arg(responseString),
// 			i18n("Upload failure"));

			int i = KMessageBox::warningYesNoCancel
				(0,
				i18n("<p>Your crash report failed to upload!</p><p>Please check your network settings and try again.</p><p>The server responded:<br>%1</p>").arg(responseString),
				i18n("Upload failure"),i18n("Save Report"),i18n("Retry Upload"));

			if (i == KMessageBox::No) {
				postCrashDataToServer(m_backtraceSubmissionData);
			}
			else if (i == KMessageBox::Yes) {
				saveOfflineCrashReport(m_backtraceSubmissionData);
			}
			else {
				enableButton(User3, true);
			}
		}
	}
	else {
		int i = KMessageBox::warningYesNoCancel
			(0,
			i18n("<p>Your crash report failed to upload!</p><p>Please check your network settings and try again.</p>"),
			i18n("Upload failure"),i18n("Save Report"),i18n("Retry Upload"));

		if (i == KMessageBox::No) {
			postCrashDataToServer(m_backtraceSubmissionData);
		}
		else if (i == KMessageBox::Yes) {
			saveOfflineCrashReport(m_backtraceSubmissionData);
		}
		else {
			enableButton(User3, true);
		}
	}
}

int Toplevel::saveOfflineCrashReport(TQCString data)
{
	TQString defname = m_krashconf->execName() + TQString::fromLatin1( ".tdecrash" );
	if( defname.contains( '/' ))
		defname = defname.mid( defname.findRev( '/' ) + 1 );
	TQString filename = KFileDialog::getSaveFileName(defname, TQString::null, this, i18n("Select Filename"));
	if (filename.isEmpty()) {
		enableButton(User3, true);
		return 1;
	}
	else {
		TQFile f(filename);

		if (f.exists()) {
			if (KMessageBox::Cancel ==
				KMessageBox::warningContinueCancel( 0,
				i18n( "A file named \"%1\" already exists. "
					"Are you sure you want to overwrite it?" ).arg( filename ),
				i18n( "Overwrite File?" ),
				i18n( "&Overwrite" ) ))
				return 2;
		}

		if (f.open(IO_WriteOnly)) {
			f.writeBlock(data.data(), data.count()-1);
			f.close();
			enableButton(User3, true);
			return 0;
		}
		else {
			KMessageBox::sorry(this, i18n("Cannot open file %1 for writing").arg(filename));
			enableButton(User3, true);
			return 3;
		}
	}
}

void Toplevel::postCrashDataToServerDataRedirection(TDEIO::Job * /*job*/, const KURL& url)
{
	//
}