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

#include <tqlayout.h>
#include <tqhbox.h>
#include <tqlabel.h>

#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <ktextbrowser.h>
#include <ktempfile.h>

#include "backtrace.h"
#include "krashconf.h"
#include "debugger.h"
#include "debugger.moc"

KrashDebugger :: KrashDebugger (const KrashConfig *krashconf, TQWidget *parent, const char *name)
  : TQWidget( parent, name ),
    m_krashconf(krashconf),
    m_proctrace(0)
{
  TQVBoxLayout *vbox = new TQVBoxLayout( this, 0, KDialog::marginHint() );
  vbox->setAutoAdd(TRUE);

  m_backtrace = new KTextBrowser(this);
  m_backtrace->setTextFormat(TQt::PlainText);
  m_backtrace->setFont(KGlobalSettings::fixedFont());

  TQWidget *w = new TQWidget( this );
  ( new TQHBoxLayout( w, 0, KDialog::marginHint() ) )->setAutoAdd( true );
  m_status = new TQLabel( w );
  m_status->setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Preferred ) );
  //m_copyButton = new KPushButton( KStdGuiItem::copy(), w );
  KGuiItem item( i18n( "C&opy" ), TQString::fromLatin1( "editcopy" ) );
  m_copyButton = new KPushButton( item, w );
  connect( m_copyButton, TQT_SIGNAL( clicked() ), this, TQT_SLOT( slotCopy() ) );
  m_copyButton->setEnabled( false );
  m_saveButton = new KPushButton( m_krashconf->safeMode() ? KStdGuiItem::save() : KStdGuiItem::saveAs(), w );
  connect( m_saveButton, TQT_SIGNAL( clicked() ), this, TQT_SLOT( slotSave() ) );
  m_saveButton->setEnabled( false );
}

KrashDebugger :: ~KrashDebugger()
{
  // This will SIGKILL gdb and SIGCONT program which crashed.
  //  delete m_proctrace;
}

void KrashDebugger :: slotDone(const TQString& str)
{
  m_status->setText(i18n("Done."));
  m_copyButton->setEnabled( true );
  m_saveButton->setEnabled( true );
  m_backtrace->setText( m_prependText + str ); // replace with possibly post-processed backtrace
}

void KrashDebugger :: slotCopy()
{
  m_backtrace->selectAll();
  m_backtrace->copy();
}

void KrashDebugger :: slotSave()
{
  if (m_krashconf->safeMode())
  {
    KTempFile tf(TQString::fromAscii("/tmp/"), TQString::fromAscii(".kcrash"), 0600);
    if (!tf.status())
    {
      *tf.textStream() << m_backtrace->text();
      tf.close();
      KMessageBox::information(this, i18n("Backtrace saved to %1").arg(tf.name()));
    }
    else
    {
      KMessageBox::sorry(this, i18n("Cannot create a file in which to save the backtrace"));
    }
  }
  else
  {
    TQString defname = m_krashconf->execName() + TQString::fromLatin1( ".kcrash" );
    if( defname.contains( '/' ))
        defname = defname.mid( defname.findRev( '/' ) + 1 );
    TQString filename = KFileDialog::getSaveFileName(defname, TQString::null, this, i18n("Select Filename"));
    if (!filename.isEmpty())
    {
      TQFile f(filename);
      
      if (f.exists()) {
        if (KMessageBox::Cancel == 
            KMessageBox::warningContinueCancel( 0,
              i18n( "A file named \"%1\" already exists. "
                    "Are you sure you want to overwrite it?" ).arg( filename ),
              i18n( "Overwrite File?" ),
              i18n( "&Overwrite" ) ))
            return;       
      }           
      
      if (f.open(IO_WriteOnly))
      {
        TQTextStream ts(&f);
        ts << m_backtrace->text();
        f.close();
      }
      else
      {
        KMessageBox::sorry(this, i18n("Cannot open file %1 for writing").arg(filename));
      }
    }
  }
}

void KrashDebugger :: slotSomeError()
{
  m_status->setText(i18n("Unable to create a valid backtrace."));
  m_backtrace->setText(i18n("This backtrace appears to be of no use.\n"
      "This is probably because your packages are built in a way "
      "which prevents creation of proper backtraces, or the stack frame "
      "was seriously corrupted in the crash.\n\n" )
      + m_backtrace->text());
}

void KrashDebugger :: slotAppend(const TQString &str)
{
  m_status->setText(i18n("Loading backtrace..."));

  // append doesn't work here because it will add a newline as well
  m_backtrace->setText(m_backtrace->text() + str);
}

void KrashDebugger :: showEvent(TQShowEvent *e)
{
  TQWidget::showEvent(e);
  startDebugger();
}

void KrashDebugger :: startDebugger()
{
  // Only start one copy
  if (m_proctrace || !m_backtrace->text().isEmpty())
    return;

  TQString msg;
  bool checks = performChecks( &msg );
  if( !checks && !m_krashconf->disableChecks())
  {
    m_backtrace->setText( m_prependText +
        i18n( "The following options are enabled:\n\n" )
        + msg
        + i18n( "\nAs the usage of these options is not recommended -"
                " because they can, in rare cases, be responsible for KDE problems - a backtrace"
                " will not be generated.\n"
                "You need to turn these options off and reproduce"
                " the problem again in order to get a backtrace.\n" ));
    m_status->setText( i18n( "Backtrace will not be created."));
    return;
  }
  if( !msg.isEmpty())
  {
    m_prependText += msg + '\n';
    m_backtrace->setText( m_prependText );
  }
  m_status->setText(i18n("Loading symbols..."));

  m_proctrace = new BackTrace(m_krashconf, TQT_TQOBJECT(this));

  connect(m_proctrace, TQT_SIGNAL(append(const TQString &)),
          TQT_SLOT(slotAppend(const TQString &)));
  connect(m_proctrace, TQT_SIGNAL(done(const TQString&)), TQT_SLOT(slotDone(const TQString&)));
  connect(m_proctrace, TQT_SIGNAL(someError()), TQT_SLOT(slotSomeError()));

  m_proctrace->start();
}

// this function check for "dangerous" settings, returns false
// and message in case some of them are activated
bool KrashDebugger::performChecks( TQString* msg )
{
  bool ret = true;
  KConfig kdedcfg( TQString::fromLatin1( "kdedrc" ), true );
  kdedcfg.setGroup( "General" );
  if( kdedcfg.readBoolEntry( "DelayedCheck", false ))
  {
    // ret = false; it's not that dangerous
    *msg += i18n( "System configuration startup check disabled.\n" );
  }
  return ret;
}
