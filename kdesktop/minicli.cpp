/* This file is part of the KDE project

   Autocompletion code:
   Copyright (C) 2009 Timothy Pearson <kb9vqf@pearsoncomputing.net>

   Copyright (C) 1999-2002,2003 Dawit Alemayehu <adawit@kde.org>
   Copyright (C) 2000 Malte Starostik <starosti@zedat.fu-berlin.de>
   Copyright (C) 2003 Sven Leiber <s.leiber@web.de>

   Kdesu integration:
   Copyright (C) 2000 Geert Jansen <jansen@kde.org>

   Original authors:
   Copyright (C) 1997 Matthias Ettrich <ettrich@kde.org>
   Copyright (C) 1997 Torben Weis [ Added command completion ]
   Copyright (C) 1999 Preston Brown <pbrown@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <pwd.h>
#include <string.h>
#include <errno.h>

#include <tqvbox.h>
#include <tqlabel.h>
#include <tqbitmap.h>
#include <tqfile.h>
#include <tqslider.h>
#include <tqlayout.h>
#include <tqgroupbox.h>
#include <tqcheckbox.h>
#include <tqregexp.h>
#include <tqwhatsthis.h>
#include <tqstylesheet.h>

#include <dcopclient.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kprocess.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kpassdlg.h>
#include <krun.h>
#include <twin.h>
#include <tdesu/su.h>
#include <kstandarddirs.h>
#include <tdeconfig.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <kguiitem.h>
#include <kstdguiitem.h>
#include <kmimetype.h>
#include <kurifilter.h>
#include <tdecompletionbox.h>

#include "minicli.moc"
#include "minicli_ui.h"
#include "kdesktopsettings.h"

#define TDESU_ERR strerror(errno)

Minicli::Minicli( TQWidget *parent, const char *name)
        :KDialog( parent, name, false, (WFlags)WType_TopLevel ),
         m_autoCheckedRunInTerm(false), m_pURLCompletion(NULL), m_pEXECompletion(NULL)
{
  setPlainCaption( i18n("Run Command") );
  KWin::setIcons( winId(), DesktopIcon("system-run"), SmallIcon("system-run") );

  TQVBoxLayout* mainLayout = new TQVBoxLayout( this, 0, KDialog::spacingHint() );
  m_dlg = new MinicliDlgUI (this);
  mainLayout->addWidget(m_dlg);

  m_dlg->lbRunIcon->setPixmap(DesktopIcon("kmenu"));
  m_dlg->lbComment->setAlignment( TQt::WordBreak );

  m_dlg->cbCommand->setDuplicatesEnabled( false );
  m_dlg->cbCommand->setTrapReturnKey( true );

  // Options button...
  m_dlg->pbOptions->setGuiItem (KGuiItem( i18n("&Options >>"), "configure" ));

  // Run button...
  m_dlg->pbRun->setGuiItem (KGuiItem( i18n("&Run"), "run" ));

  // Cancel button...
  m_dlg->pbCancel->setGuiItem ( KStdGuiItem::cancel() );

  if (!kapp->authorize("shell_access"))
    m_dlg->pbOptions->hide();

  m_dlg->pbRun->setEnabled(!m_dlg->cbCommand->currentText().isEmpty());
  m_dlg->pbRun->setDefault( true );

  // Do not show the advanced group box on startup...
  m_dlg->gbAdvanced->hide();

  // URI Filter meta object...
  m_filterData = new KURIFilterData();

  // Create a timer object...
  m_parseTimer = new TQTimer(this);

  m_FocusWidget = 0;

  m_prevCached = false;
  m_iPriority = 50;
  m_iScheduler = StubProcess::SchedNormal;

  m_dlg->leUsername->setText("root");

  // Autocomplete system
  m_filesystemAutocomplete = 0;
  m_histfilesystemAutocomplete = 0;
  m_systempathAutocomplete = 0;
  m_pURLCompletion = new KURLCompletion(KURLCompletion::FileCompletion);
  m_pEXECompletion = new KURLCompletion(KURLCompletion::SystemExeCompletion);
  //m_pURLCompletion->setCompletionMode( TDEGlobalSettings::completionMode() );
  connect( m_pURLCompletion, TQT_SIGNAL( match(const TQString&) ), TQT_SLOT( slotMatch(const TQString&) ));
  connect( m_pEXECompletion, TQT_SIGNAL( match(const TQString&) ), TQT_SLOT( slotEXEMatch(const TQString&) ));

  // Main widget buttons...
  connect( m_dlg->pbRun, TQT_SIGNAL(clicked()), this, TQT_SLOT(accept()) );
  connect( m_dlg->pbCancel, TQT_SIGNAL(clicked()), this, TQT_SLOT(reject()) );
  connect( m_dlg->pbOptions, TQT_SIGNAL(clicked()), TQT_SLOT(slotAdvanced()) );
  connect( m_parseTimer, TQT_SIGNAL(timeout()), TQT_SLOT(slotParseTimer()) );

  connect( m_dlg->cbCommand, TQT_SIGNAL( textChanged( const TQString& ) ),
           TQT_SLOT( slotCmdChanged(const TQString&) ) );

  connect( m_dlg->cbCommand, TQT_SIGNAL( returnPressed() ),
           m_dlg->pbRun, TQT_SLOT( animateClick() ) );

  m_dlg->cbCommand->setHistoryEditorEnabled( true );
  connect( m_dlg->cbCommand, TQT_SIGNAL(removed( const TQString&) ), TQT_SLOT(saveConfig()) );

  // Advanced group box...
  connect(m_dlg->cbPriority, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChangeScheduler(bool)));
  connect(m_dlg->slPriority, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(slotPriority(int)));
  connect(m_dlg->cbRealtime, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotRealtime(bool)));
  connect(m_dlg->cbAppcomplete, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotAppcompleteToggled(bool)));
  connect(m_dlg->cbAutocomplete, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotAutocompleteToggled(bool)));
  connect(m_dlg->cbAutohistory, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotAutohistoryToggled(bool)));
  connect(m_dlg->cbRunAsOther, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChangeUid(bool)));
  connect(m_dlg->leUsername, TQT_SIGNAL(lostFocus()), TQT_SLOT(updateAuthLabel()));
  connect(m_dlg->cbRunInTerminal, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotTerminal(bool)));

  m_dlg->slPriority->setValue(50);

  loadConfig();
}

Minicli::~Minicli()
{
  delete m_filterData;
  delete m_pURLCompletion;
  delete m_pEXECompletion;
}

void Minicli::setCommand(const TQString& command)
{
  m_dlg->cbCommand->lineEdit()->setText(command);
  m_dlg->cbCommand->lineEdit()->deselect();
  int firstSpace = command.find(' ');
  if (firstSpace > 0) {
    m_dlg->cbCommand->lineEdit()->setSelection(firstSpace+1, command.length());
  }
}

TQSize Minicli::sizeHint() const
{
  int maxWidth = tqApp->desktop()->screenGeometry((TQWidget*)this).width();
  if (maxWidth < 603)
  {
    // a sensible max for smaller screens
    maxWidth = (maxWidth > 240) ? 240 : maxWidth;
  }
  else
  {
    maxWidth = maxWidth * 2 / 5;
  }

  return TQSize(maxWidth, -1);
}

void Minicli::show()
{
  KWin::setState( winId(), NET::StaysOnTop );
  KDialog::show();
}

void Minicli::loadConfig()
{
  TQStringList histList = KDesktopSettings::history();
  int maxHistory = KDesktopSettings::historyLength();
  m_terminalAppList = KDesktopSettings::terminalApps();

  if (m_terminalAppList.isEmpty())
    m_terminalAppList << "ls"; // Default

  bool block = m_dlg->cbCommand->signalsBlocked();
  m_dlg->cbCommand->blockSignals( true );
  m_dlg->cbCommand->setMaxCount( maxHistory );
  m_dlg->cbCommand->setHistoryItems( histList );
  m_dlg->cbCommand->blockSignals( block );

  m_dlg->cbAutocomplete->setChecked( KDesktopSettings::miniCLIFilesystemAutoComplete() );
  m_dlg->cbAppcomplete->setChecked( KDesktopSettings::miniCLISystempathAutoComplete() );
  m_dlg->cbAutohistory->setChecked( KDesktopSettings::miniCLIHistoryAndFilesystemAutoComplete() );

  m_filesystemAutocomplete = KDesktopSettings::miniCLIFilesystemAutoComplete();
  m_systempathAutocomplete = KDesktopSettings::miniCLISystempathAutoComplete();
  m_histfilesystemAutocomplete = KDesktopSettings::miniCLIHistoryAndFilesystemAutoComplete();

  if (m_histfilesystemAutocomplete == true) {
      m_dlg->cbAutocomplete->setDisabled( true );
  }
  else {
      m_dlg->cbAutocomplete->setDisabled( false );
  }

  TQStringList compList = KDesktopSettings::completionItems();
  if( compList.isEmpty() )
    m_dlg->cbCommand->completionObject()->setItems( histList );
  else
    m_dlg->cbCommand->completionObject()->setItems( compList );

  int mode = KDesktopSettings::completionMode();
  m_dlg->cbCommand->setCompletionMode( (TDEGlobalSettings::Completion) mode );

  TDECompletionBox* box = m_dlg->cbCommand->completionBox();
  if (box)
    box->setActivateOnSelect( false );

  m_finalFilters = KURIFilter::self()->pluginNames();
  m_finalFilters.remove("kuriikwsfilter");

  m_middleFilters = m_finalFilters;
  m_middleFilters.remove("localdomainurifilter");

  // Provide username completions. Use saner and configurable maximum values.
  int maxEntries = KDesktopSettings::maxUsernameCompletions();
  TQStringList users;

  struct passwd *pw;
  setpwent();
  for (int count=0; ((pw = getpwent()) != 0L) && (count < maxEntries); count++)
    users << TQString::fromLocal8Bit(pw->pw_name);
  endpwent();

  TDECompletion *completion = new TDECompletion;
  completion->setOrder(TDECompletion::Sorted);
  completion->insertItems (users);

  m_dlg->leUsername->setCompletionObject(completion, true);
  m_dlg->leUsername->setCompletionMode(TDEGlobalSettings::completionMode());
  m_dlg->leUsername->setAutoDeleteCompletionObject( true );

}

void Minicli::saveConfig()
{
  KDesktopSettings::setHistory( m_dlg->cbCommand->historyItems() );
  KDesktopSettings::setTerminalApps( m_terminalAppList );
  //KDesktopSettings::setCompletionItems( m_dlg->cbCommand->completionObject()->items() );
  KDesktopSettings::setCompletionMode( m_dlg->cbCommand->completionMode() );
  KDesktopSettings::setMiniCLIFilesystemAutoComplete( m_filesystemAutocomplete );
  KDesktopSettings::setMiniCLISystempathAutoComplete( m_systempathAutocomplete );
  KDesktopSettings::setMiniCLIHistoryAndFilesystemAutoComplete( m_histfilesystemAutocomplete );
  KDesktopSettings::writeConfig();
}

void Minicli::clearHistory()
{
  m_dlg->cbCommand->clearHistory();
  saveConfig();
}

void Minicli::accept()
{
  TQString cmd = m_dlg->cbCommand->currentText().stripWhiteSpace();
  if (!cmd.isEmpty() && (cmd[0].isNumber() || (cmd[0] == '(') ||
      ((cmd[0] == '-') && cmd[1].isNumber())) &&
      (TQRegExp("[a-zA-Z\\]\\[]").search(cmd) == -1))
  {
     TQString result = calculate(cmd);
     if (!result.isEmpty())
        m_dlg->cbCommand->setCurrentText(result);
     return;
  }

  bool logout = (cmd == "logout");
  bool lock = (cmd == "lock");

  if( !logout && !lock && runCommand() == 1 )
     return;

  m_dlg->cbCommand->addToHistory( m_dlg->cbCommand->currentText().stripWhiteSpace() );
  reset();
  saveConfig();
  TQDialog::accept();

  if ( logout )
  {
     kapp->propagateSessionManager();
     kapp->requestShutDown();
  }
  if ( lock )
  {
     TQCString appname( "kdesktop" );
     int kicker_screen_number = tqt_xscreen();
     if ( kicker_screen_number )
         appname.sprintf("kdesktop-screen-%d", kicker_screen_number);
     kapp->dcopClient()->send(appname, "KScreensaverIface", "lock()", TQString(""));
  }
}

void Minicli::reject()
{
  reset();
  TQDialog::reject();
}

void Minicli::reset()
{
  if( m_dlg->gbAdvanced->isShown() )
    slotAdvanced();

  bool block = m_dlg->cbCommand->signalsBlocked();
  m_dlg->cbCommand->blockSignals( true );
  m_dlg->cbCommand->clearEdit();
  m_dlg->cbCommand->setFocus();
  m_dlg->cbCommand->reset();
  m_dlg->cbCommand->blockSignals( block );
  m_dlg->pbRun->setEnabled( false );

  m_iPriority = 50;
  m_iScheduler = StubProcess::SchedNormal;

  m_dlg->cbRunInTerminal->setChecked(false);
  m_dlg->cbRunAsOther->setChecked(false);

  m_dlg->leUsername->setText("root");

  m_dlg->cbPriority->setChecked(false);

  m_dlg->slPriority->setValue(m_iPriority);

  m_dlg->cbRealtime->setChecked( m_iScheduler == StubProcess::SchedRealtime );
  m_dlg->lePassword->erase();

  m_FocusWidget = 0;
  m_iconName = TQString::null;
  m_prevIconName = TQString::null;

  m_prevCached = false;
  updateAuthLabel();
  setIcon();
}

void Minicli::keyPressEvent( TQKeyEvent* e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    e->accept();
    m_dlg->pbCancel->animateClick();
    return;
  }

  TQDialog::keyPressEvent( e );
}

TQString Minicli::terminalCommand (const TQString& cmd, const TQString& args)
{
  TQString terminal = KDesktopSettings::terminalApplication().stripWhiteSpace();
  if (terminal.endsWith("konsole"))
    terminal += " --noclose";

  if( args.isEmpty() )
    terminal += TQString(" -e /bin/sh -c \"%1\"").arg(cmd);
  else
    terminal += TQString(" -e /bin/sh -c \"%1 %2\"").arg(cmd).arg(args);

  if (!m_terminalAppList.contains(cmd))
    m_terminalAppList << cmd;

  return terminal;
}

int Minicli::runCommand()
{
  m_parseTimer->stop();

  // Make sure we have an updated data
  parseLine( true );

  // Ignore empty commands...
  if ( m_dlg->cbCommand->currentText().isEmpty() )
    return 1;

  TQString cmd;
  KURL uri = m_filterData->uri();
  if ( uri.isLocalFile() && !uri.hasRef() && uri.query().isEmpty() )
    cmd = uri.path();
  else
    cmd = uri.url();
    
  TQCString asn;
  if( tqApp->desktop()->isVirtualDesktop())
  {
    asn = TDEStartupInfo::createNewStartupId();
    TDEStartupInfoId id;
    id.initId( asn );
    TDEStartupInfoData data;
    data.setXinerama( tqApp->desktop()->screenNumber( this ));
    TDEStartupInfo::sendChange( id, data );
  }

  // Determine whether the application should be run through
  // the command line (terminal) interface...
  bool useTerminal = m_dlg->cbRunInTerminal->isChecked();

  kdDebug (1207) << "Use terminal ? " << useTerminal << endl;

  if (!kapp->authorize("shell_access"))
    useTerminal = false;

  if( needsTDEsu() )
  {
    TQCString user;
    struct passwd *pw;

    if (m_dlg->cbRunAsOther)
    {
      pw = getpwnam(m_dlg->leUsername->text().local8Bit());
      if (pw == 0L)
      {
          KMessageBox::sorry( this, i18n("<qt>The user <b>%1</b> "
          "does not exist on this system.</qt>").arg(m_dlg->leUsername->text()));
        return 1;
      }
    }
    else
    {
      pw = getpwuid(getuid());
      if (pw == 0L)
      {
          KMessageBox::error( this, i18n("You do not exist.\n"));
        return 1;
      }
    }

    user = pw->pw_name;

    {
      // Scoped. we want this object to go away before the fork
      // (maybe not necessary, but can't hurt) according to the cvs log,
      // creating the SuProcess object in the parent and using it in the
      // child creates crashes, but we need to check password before the
      // fork in order to not get in trouble with X for the messagebox

      SuProcess proc_checkpwd;
      proc_checkpwd.setUser( user );

      if (m_dlg->cbPriority->isChecked())
      {
        proc_checkpwd.setPriority(m_iPriority);
        proc_checkpwd.setScheduler(m_iScheduler);
      }

      if (proc_checkpwd.checkInstall(m_dlg->lePassword->password().local8Bit()) != 0)
      {
          KMessageBox::sorry(this, i18n("Incorrect password; please try again."));
        return 1;
      }
    }

    TQApplication::flushX();

    int pid = fork();

    if (pid < 0)
    {
      kdError(1207) << "fork(): " << TDESU_ERR << "\n";
      return -1;
    }

    if (pid > 0)
      return 0;

    // From here on, this is child...

    SuProcess proc;
    proc.setUser(user);

    if (m_dlg->cbPriority->isChecked())
    {
      proc.setPriority(m_iPriority);
      proc.setScheduler(m_iScheduler);
    }

    TQCString command;

    if (useTerminal)
      command = terminalCommand( cmd, m_filterData->argsAndOptions() ).local8Bit();
    else
    {
      command = cmd.local8Bit();
      if( m_filterData->hasArgsAndOptions() )
        command += m_filterData->argsAndOptions().local8Bit();
    }

    proc.setCommand(command);

    // Block SIGCHLD because SuProcess::exec() uses waitpid()
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sset, 0L);
    proc.setTerminal(true);
    proc.setErase(true);
    _exit(proc.exec(m_dlg->lePassword->password().local8Bit()));
    return 0;
  }
  else
  {
    TQString exec;

    // yes, this is a hack, but there is no way of doing it
    // through SuProcess without providing the user password
    if (m_iPriority < 50)
    {
      // from tdesu_stub.c
      int val = 20 - (int) (((double) m_iPriority) * 40 / 100 + 0.5);
      cmd = "nice -n " + TQString::number( val ) + " " + cmd;
    }

    if (useTerminal)
    {
      cmd = terminalCommand( cmd, m_filterData->argsAndOptions() );
      kdDebug(1207) << "Terminal command: " << cmd << endl;
    }
    else
    {
      switch( m_filterData->uriType() )
      {
        case KURIFilterData::LOCAL_FILE:
        case KURIFilterData::LOCAL_DIR:
        case KURIFilterData::NET_PROTOCOL:
        case KURIFilterData::HELP:
        {
          // No need for kfmclient, KRun does it all (David)
          (void) new KRun( m_filterData->uri(), parentWidget(), asn );
          return 0;
        }
        case KURIFilterData::EXECUTABLE:
        {
          if( !m_filterData->hasArgsAndOptions() )
          {
            // Look for desktop file
            KService::Ptr service = KService::serviceByDesktopName(cmd);
            if (service && service->isValid() && service->type() == "Application")
            {
              notifyServiceStarted(service);
              KRun::run(*service, KURL::List(), parentWidget(), asn );
              return 0;
            }
          }
        }
        // fall-through to shell case
        case KURIFilterData::SHELL:
        {
          if (kapp->authorize("shell_access"))
          {
            exec = cmd;

            if( m_filterData->hasArgsAndOptions() )
              cmd += m_filterData->argsAndOptions();

            break;
          }
          else
          {
            KMessageBox::sorry( this, i18n("<center><b>%1</b></center>\n"
                                      "You do not have permission to execute "
                                      "this command.")
                                      .arg( TQStyleSheet::convertFromPlainText(cmd) ));
            return 1;
          }
        }
        case KURIFilterData::UNKNOWN:
        case KURIFilterData::ERROR:
        default:
        {
          // Look for desktop file
          KService::Ptr service = KService::serviceByDesktopName(cmd);
          if (service && service->isValid() && service->type() == "Application")
          {
            notifyServiceStarted(service);
            KRun::run(*service, KURL::List(), parentWidget(), asn );
            return 0;
          }

          service = KService::serviceByName(cmd);
          if (service && service->isValid() && service->type() == "Application")
          {
            notifyServiceStarted(service);
            KRun::run(*service, KURL::List(), parentWidget(), asn );
            return 0;
          }

          KMessageBox::sorry( this, i18n("<center><b>%1</b></center>\n"
                                    "Could not run the specified command.")
                                    .arg( TQStyleSheet::convertFromPlainText(cmd) ));
          return 1;
        }
      }
    }

    if ( KRun::runCommand( cmd, exec, m_iconName, parentWidget(), asn ) )
      return 0;
    else
    {
      KMessageBox::sorry( this, i18n("<center><b>%1</b></center>\n"
                                "The specified command does not exist.").arg(cmd) );
      return 1; // Let the user try again...
    }
  }
}

void Minicli::notifyServiceStarted(KService::Ptr service)
{
    // Inform other applications (like the quickstarter applet)
    // that an application was started
    TQByteArray params;
    TQDataStream stream(params, IO_WriteOnly);
    stream << "minicli" << service->storageId();
    kdDebug() << "minicli appLauncher dcop signal: " << service->storageId() << endl;
    TDEApplication::kApplication()->dcopClient()->emitDCOPSignal("appLauncher",
        "serviceStartedByStorageId(TQString,TQString)", params);
}

void Minicli::slotCmdChanged(const TQString& text)
{
  bool isEmpty = text.isEmpty();
  m_dlg->pbRun->setEnabled( !isEmpty );

  if( isEmpty )
  {
    // Reset values to default
    m_filterData->setData(KURL());

    // Empty String is certainly no terminal application
    slotTerminal(false);

    // Reset the icon if needed...
    const TQPixmap pixmap = DesktopIcon("kmenu");

    if ( pixmap.serialNumber() != m_dlg->lbRunIcon->pixmap()->serialNumber())
      m_dlg->lbRunIcon->setPixmap(pixmap);

    return;
  }
  else if ((m_filesystemAutocomplete == true) && ( m_pURLCompletion )) {
    // Attempt to autocomplete the entered URL if it starts with the / character, meaning I am looking for something on the filesystem
    // Also use autocompletion if it appears that I am using some kind of ioslave, except the http:// ioslave
    m_urlCompletionStarted = true; // flag for slotMatch()

    if ((text.startsWith( "/" ) || text.startsWith( "~" ) || (text.contains("://", false) != 0)) && (text.contains("http://", false) == 0)) {
        TQString completion = m_pURLCompletion->makeCompletion( text );
    }
  }
  if ((m_systempathAutocomplete == true) && ( m_pEXECompletion )) {
    // Attempt to autocomplete the entered URL if it starts with the / character, meaning I am looking for something on the filesystem
    // Also use autocompletion if it appears that I am using some kind of ioslave, except the http:// ioslave
    m_exeCompletionStarted = true; // flag for slotEXEMatch()

    if (!((text.startsWith( "/" ) || text.startsWith( "~" ) || (text.contains("://", false) != 0)) && (text.contains("http://", false) == 0))) {
        TQString completion = m_pEXECompletion->makeCompletion( text );
    }
  }

  m_parseTimer->start(250, true);
}

// Handle match() from m_pURLCompletion
void Minicli::slotMatch( const TQString &match )
{
  TQString current_text;
  TQStringList histList = KDesktopSettings::history();
  int maxHistory = KDesktopSettings::historyLength();
  int maxAutocompletion = KDesktopSettings::miniCLIAutocompletionLength();

  if ( match.isEmpty() ) // this case is handled directly
    return;

  // Check flag to avoid match() raised by rotation
  if ( m_urlCompletionStarted ) {
    m_urlCompletionStarted = false;

    if (m_filesystemAutocomplete == true) {
        bool block = m_dlg->cbCommand->signalsBlocked();
        m_dlg->cbCommand->blockSignals( true );
        TQStringList items = m_pURLCompletion->allMatches();
        items.sort();
        if (m_histfilesystemAutocomplete == true) {
            // Add the history to the list
            histList += items;
            maxHistory += maxAutocompletion;
        }
        else {
            histList = items;
            maxHistory = maxAutocompletion;
        }
        current_text = m_dlg->cbCommand->currentText();
        //histList.prepend ( current_text );	// Add the current text to the autocompletion list
        m_dlg->cbCommand->setMaxCount( maxHistory );
        m_dlg->cbCommand->completionObject()->setItems( histList );
        m_dlg->cbCommand->setCurrentText( current_text );
        m_dlg->cbCommand->blockSignals( block );
    }
  }
}

// Handle match() from m_pEXECompletion
void Minicli::slotEXEMatch( const TQString &match )
{
  TQString current_text;
  TQStringList histList = KDesktopSettings::history();
  int maxHistory = KDesktopSettings::historyLength();
  int maxAutocompletion = KDesktopSettings::miniCLIAutocompletionLength();

  if ( match.isEmpty() ) // this case is handled directly
    return;

  // Check flag to avoid match() raised by rotation
  if ( m_exeCompletionStarted ) {
    m_exeCompletionStarted = false;

    if (m_systempathAutocomplete == true) {
        bool block = m_dlg->cbCommand->signalsBlocked();
        m_dlg->cbCommand->blockSignals( true );
        TQStringList items = m_pEXECompletion->allMatches();
        items.sort();
        if (m_histfilesystemAutocomplete == true) {
            // Add the history to the list
            histList += items;
            maxHistory += maxAutocompletion;
        }
        else {
            histList = items;
            maxHistory = maxAutocompletion;
        }
        current_text = m_dlg->cbCommand->currentText();
        //histList.prepend ( current_text );	// Add the current text to the autocompletion list
        m_dlg->cbCommand->setMaxCount( maxHistory );
        m_dlg->cbCommand->completionObject()->setItems( histList );
        m_dlg->cbCommand->setCurrentText( current_text );
        m_dlg->cbCommand->blockSignals( block );
    }
  }
}

void Minicli::slotAdvanced()
{
  if (m_dlg->gbAdvanced->isHidden())
  {
    m_dlg->gbAdvanced->show();
    m_dlg->pbOptions->setText(i18n("&Options <<"));

    // Set the focus back to the widget that had it to begin with, i.e.
    // do not put the focus on the "Options" button.
    m_FocusWidget = focusWidget();

    if( m_FocusWidget )
      m_FocusWidget->setFocus();
  }
  else
  {
    m_dlg->gbAdvanced->hide();
    m_dlg->pbOptions->setText(i18n("&Options >>"));

    if( m_FocusWidget && m_FocusWidget->parent() != m_dlg->gbAdvanced )
      m_FocusWidget->setFocus();
  }
  adjustSize();
}

void Minicli::slotParseTimer()
{
  //kdDebug (1207) << "Minicli::slotParseTimer: Timed out..." << endl;
  parseLine( false );
}

void Minicli::parseLine( bool final )
{
  TQString cmd = m_dlg->cbCommand->currentText().stripWhiteSpace();
  m_filterData->setData( cmd );

  if( final )
    KURIFilter::self()->filterURI( *(m_filterData), m_finalFilters );
  else
    KURIFilter::self()->filterURI( *(m_filterData), m_middleFilters );

  bool isTerminalApp = ((m_filterData->uriType() == KURIFilterData::EXECUTABLE) &&
                        m_terminalAppList.contains(m_filterData->uri().url()));

  if( !isTerminalApp )
  {
    m_iconName = m_filterData->iconName();
    setIcon();
  }

  if ( isTerminalApp && final && !m_dlg->cbRunInTerminal->isChecked() )
  {
    m_terminalAppList.remove( m_filterData->uri().url() );
    isTerminalApp = false;
  }
  else
  {
    bool wasAutoChecked = m_autoCheckedRunInTerm;
    bool willBeAutoChecked = isTerminalApp && !m_dlg->cbRunInTerminal->isChecked();
    slotTerminal(isTerminalApp || (m_dlg->cbRunInTerminal->isChecked() && !m_autoCheckedRunInTerm));
    if (!wasAutoChecked && willBeAutoChecked)
        m_autoCheckedRunInTerm = true;
  }

  kdDebug (1207) << "Command: " << m_filterData->uri().url() << endl;
  kdDebug (1207) << "Arguments: " << m_filterData->argsAndOptions() << endl;
}

void Minicli::setIcon ()
{
  if( m_iconName.isEmpty() || m_iconName == "unknown" || m_iconName == "kde" )
    m_iconName = TQString::fromLatin1("kmenu");

  TQPixmap icon;
  if ((m_iconName == "application-x-executable") && (m_filterData->iconName() == "application-x-executable")) {
    TQPixmap potentialIcon = m_filterData->customIconPixmap();
    if (!potentialIcon.isNull()) {
      icon = potentialIcon;
    }
    else {
      icon = DesktopIcon( m_iconName );
    }
  }
  else {
    icon = DesktopIcon( m_iconName );
  }

  if ( m_iconName == "www" )
  {
    // Not using TDEIconEffect::overlay as that requires the same size
    // for the icon and the overlay, also the overlay definately doesn't
    // have a more that one-bit alpha channel here
    TQPixmap overlay( locate ( "icon", KMimeType::favIconForURL( m_filterData->uri() ) + ".png" ) );
    if ( !overlay.isNull() )
    {
      int x = icon.width() - overlay.width();
      int y = icon.height() - overlay.height();
      if ( icon.mask() )
      {
        TQBitmap mask = *icon.mask();
        bitBlt( &mask, x, y,
                overlay.mask() ? TQT_TQPIXMAP(const_cast<TQBitmap *>(overlay.mask())) : &overlay,
                0, 0, overlay.width(), overlay.height(),
                overlay.mask() ? OrROP : SetROP );
        icon.setMask(mask);
      }
      bitBlt( &icon, x, y, &overlay );
    }
  }

  m_dlg->lbRunIcon->setPixmap( icon );
}

void Minicli::updateAuthLabel()
{
  if ((m_dlg->cbPriority->isChecked() && (m_iPriority > 50)) ||
      (m_iScheduler != StubProcess::SchedNormal))
  {
    if (!m_prevCached && !m_dlg->leUsername->text().isEmpty())
    {
      //kdDebug(1207) << k_funcinfo << "Caching: user=" << m_dlg->leUsername->text() <<
      //  ", checked=" << m_dlg->cbRunAsOther->isChecked() << endl;

      m_prevUser = m_dlg->leUsername->text();
      m_prevPass = m_dlg->lePassword->text();
      m_prevChecked = m_dlg->cbRunAsOther->isChecked();
      m_prevCached = true;
    }
    if (m_dlg->leUsername->text() != TQString::fromLatin1("root"))
      m_dlg->lePassword->setText(TQString::null);
    m_dlg->leUsername->setText(TQString::fromLatin1("root"));
    m_dlg->cbRunAsOther->setChecked(true);
    m_dlg->cbRunAsOther->setEnabled(false);
    m_dlg->leUsername->setEnabled(false);
    m_dlg->lbUsername->setEnabled(true);
    m_dlg->lePassword->setEnabled(true);
    m_dlg->lbPassword->setEnabled(true);
  }
  else if (m_dlg->cbRunAsOther->isEnabled() &&
    m_dlg->cbRunAsOther->isChecked() && !m_dlg->leUsername->text().isEmpty())
  {
    m_dlg->lePassword->setEnabled(true);
    m_dlg->lbPassword->setEnabled(true);
  }
  else
  {
    if (m_prevCached)
    {
      m_dlg->leUsername->setText(m_prevUser);
      m_dlg->lePassword->setText(m_prevPass);
      m_dlg->cbRunAsOther->setChecked(m_prevChecked);
      m_dlg->leUsername->setEnabled(m_prevChecked);
      m_dlg->lbUsername->setEnabled(m_prevChecked);
    }
    else
    {
      m_dlg->cbRunAsOther->setChecked(false);
      m_dlg->leUsername->setEnabled(false);
      m_dlg->lbUsername->setEnabled(false);
    }
    m_dlg->cbRunAsOther->setEnabled(true);
    m_dlg->lePassword->setEnabled(false);
    m_dlg->lbPassword->setEnabled(false);
    m_prevCached = false;
  }
}

void Minicli::slotTerminal(bool enable)
{
  m_dlg->cbRunInTerminal->setChecked(enable);
  m_autoCheckedRunInTerm = false;

  if (enable)
  {
    m_prevIconName = m_iconName;
    m_iconName = TQString::fromLatin1( "konsole" );
    setIcon();
  }
  else if (!m_prevIconName.isEmpty())
  {
      m_iconName = m_prevIconName;
      setIcon();
  }
}

void Minicli::slotChangeUid(bool enable)
{
  m_dlg->leUsername->setEnabled(enable);
  m_dlg->lbUsername->setEnabled(enable);

  if(enable)
  {
    m_dlg->leUsername->selectAll();
    m_dlg->leUsername->setFocus();
  }

  updateAuthLabel();
}

void Minicli::slotChangeScheduler(bool enable)
{
  m_dlg->slPriority->setEnabled(enable);
  m_dlg->lbLowPriority->setEnabled(enable);
  m_dlg->lbHighPriority->setEnabled(enable);

  updateAuthLabel();
}

bool Minicli::needsTDEsu()
{
  return ((m_dlg->cbPriority->isChecked() && ((m_iPriority > 50) ||
          (m_iScheduler != StubProcess::SchedNormal))) ||
          (m_dlg->cbRunAsOther->isChecked() && !m_dlg->leUsername->text().isEmpty()));
}

void Minicli::slotRealtime(bool enabled)
{
  m_iScheduler = enabled ? StubProcess::SchedRealtime : StubProcess::SchedNormal;

  if (enabled)
  {
    if (KMessageBox::warningContinueCancel(this,
                i18n("Running a realtime application can be very dangerous. "
                    "If the application misbehaves, the system might hang "
                    "unrecoverably.\nAre you sure you want to continue?"),
                i18n("Warning - Run Command"), KGuiItem(i18n("&Run Realtime")),TQString::null,KMessageBox::Notify|KMessageBox::PlainCaption)
        != KMessageBox::Continue )
    {
      m_iScheduler = StubProcess::SchedNormal;
      m_dlg->cbRealtime->setChecked(false);
    }
  }

  updateAuthLabel();
}

void Minicli::slotAutocompleteToggled(bool enabled)
{
  if (enabled)
  {
    // Enable filesystem autocompletion
    m_filesystemAutocomplete = true;
  }
  else {
    // Enable history only autocompletion
    m_filesystemAutocomplete = false;
  }

  TQString current_text = m_dlg->cbCommand->currentText();
  m_dlg->cbCommand->setCurrentText( current_text );    // Force an update of the autocompletion list
}

void Minicli::slotAppcompleteToggled(bool enabled)
{
  m_systempathAutocomplete = enabled;

  TQString current_text = m_dlg->cbCommand->currentText();
  m_dlg->cbCommand->setCurrentText( current_text );    // Force an update of the autocompletion list
}

void Minicli::slotAutohistoryToggled(bool enabled)
{
  if (enabled)
  {
    // Enable history and filesystem autocompletion
    m_histfilesystemAutocomplete = true;
    m_filesystemAutocomplete = true;
    m_dlg->cbAutocomplete->setChecked( true );
    m_dlg->cbAutocomplete->setDisabled ( true );
  }
  else {
    // Disable history and filesystem autocompletion
    m_histfilesystemAutocomplete = false;
    m_dlg->cbAutocomplete->setDisabled ( false );
  }

  TQString current_text = m_dlg->cbCommand->currentText();
  m_dlg->cbCommand->setCurrentText( current_text );    // Force an update of the autocompletion list
}

void Minicli::slotPriority(int priority)
{
  // Provide a way to easily return to the default priority
  if ((priority > 40) && (priority < 60))
  {
    priority = 50;
    m_dlg->slPriority->setValue(50);
  }

  m_iPriority = priority;

  updateAuthLabel();
}

TQString Minicli::calculate(const TQString &exp)
{
   TQString result, cmd;
   const TQString bc = TDEStandardDirs::findExe("bc");
   if ( !bc.isEmpty() )
      cmd = TQString("echo %1 | %2").arg(TDEProcess::quote(TQString("scale=8; ")+exp), TDEProcess::quote(bc));
   else
      cmd = TQString("echo $((%1))").arg(exp);
   FILE *fs = popen(TQFile::encodeName(cmd).data(), "r");
   if (fs)
   {
       { // scope for QTextStream
           TQTextStream ts(fs, IO_ReadOnly);
           result = ts.read().stripWhiteSpace().replace( TQRegExp( "^(-?)(\\.[0-9])" ), "\\10\\2" );
       }
       pclose(fs);
   }
   return result;
}

void Minicli::fontChange( const TQFont & )
{
   adjustSize();
}

// vim: set et ts=2 sts=2 sw=2:

