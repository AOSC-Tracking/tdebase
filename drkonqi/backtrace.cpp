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

#include "config.h"

#include <tqfile.h>
#include <tqregexp.h>

#include <kprocess.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <tdemessagebox.h>
#include <tdelocale.h>
#include <tdetempfile.h>
#ifdef WITH_TDEHWLIB
#include <tdehardwaredevices.h>
#endif

#ifdef HAVE_ELFICON
	// Elven things
	extern "C" {
		#include <libr.h>
		#include <libr-icons.h>
	}
	#include <tqfileinfo.h>
	#include <tdeio/tdelficon.h>
#endif // HAVE_ELFICON

#include "krashconf.h"
#include "backtrace.h"
#include "backtrace.moc"

BackTrace::BackTrace(const KrashConfig *krashconf, TQObject *parent,
                     const char *name)
  : TQObject(parent, name),
    m_krashconf(krashconf), m_temp(NULL), m_temp_cmd(NULL)
{
  m_proc = new TDEProcess;
}

BackTrace::~BackTrace()
{
  pid_t pid = m_proc ? m_proc->pid() : 0;
  // we don't want the gdb process to hang around
  delete m_proc; // this will kill gdb (SIGKILL, signal 9)

  // continue the process we ran backtrace on. Gdb sends SIGSTOP to the
  // process. For some reason it doesn't work if we send the signal before
  // gdb has exited, so we better wait for it.
  // Do not touch it if we never ran backtrace.
  if (pid)
  {
    waitpid(pid, NULL, 0);
    kill(m_krashconf->pid(), SIGCONT);
  }

  delete m_temp;
  delete m_temp_cmd;
}

void BackTrace::start()
{
  TQString exec = m_krashconf->tryExec();
  if ( !exec.isEmpty() && TDEStandardDirs::findExe(exec).isEmpty() )
  {
    TQObject * o = parent();

    if (o && !o->inherits("TQWidget"))
    {
      o = NULL;
    }

    KMessageBox::error(
                        (TQWidget *)o,
			i18n("Could not generate a backtrace as the debugger '%1' was not found.").arg(exec));
    return;
  }
  m_temp = new KTempFile;
  m_temp->setAutoDelete(true);
  int handle = m_temp->handle();
  TQString backtraceCommand = m_krashconf->backtraceCommand();
  const char* bt = backtraceCommand.latin1();
  ::write(handle, bt, strlen(bt)); // the command for a backtrace
  ::write(handle, "\n", 1);
  ::fsync(handle);

  // build the debugger command
  TQString str = m_krashconf->debuggerBatch();
  m_krashconf->expandString(str, true, m_temp->name());

  // write the debugger command
  m_temp_cmd = new KTempFile(TQString::null, TQString::null, 0700);
  m_temp_cmd->setAutoDelete(true);
  handle = m_temp_cmd->handle();
  const char* dbgcommand = str.latin1();
  ::write(handle, dbgcommand, strlen(dbgcommand)); // the command to execute the debugger
  ::write(handle, "\n", 1);
  ::fsync(handle);
  m_temp_cmd->close();

  // determine if yama has been used to prevent ptrace as normal user
  bool need_root_access = false;
  TQFile yamactl("/proc/sys/kernel/yama/ptrace_scope");
  if (yamactl.exists()) {
    if (yamactl.open(IO_ReadOnly)) {
      TQTextStream stream(&yamactl);
      TQString line;
      line = stream.readLine();
      if (line.toInt() != 0) {
        need_root_access = true;
      }
      yamactl.close();
    }
  }

  // start the debugger
  m_proc = new TDEProcess;
  m_proc->setUseShell(true);

  if (need_root_access == false) {
    *m_proc << m_temp_cmd->name();
  }
  else {
    *m_proc << "tdesu -t --comment \"" << i18n("Administrative access is required to generate a backtrace") << "\" -c \"" << m_temp_cmd->name() << "\"";
  }

  connect(m_proc, TQ_SIGNAL(receivedStdout(TDEProcess*, char*, int)),
          TQ_SLOT(slotReadInput(TDEProcess*, char*, int)));
  connect(m_proc, TQ_SIGNAL(processExited(TDEProcess*)),
          TQ_SLOT(slotProcessExited(TDEProcess*)));

  m_proc->start ( TDEProcess::NotifyOnExit, TDEProcess::All );
}

void BackTrace::slotReadInput(TDEProcess *, char* buf, int buflen)
{
  TQString newstr = TQString::fromLocal8Bit(buf, buflen);
  newstr.replace("\n\n", "\n");
  if (m_strBt.isEmpty()) {
    if (newstr == "\n") {
      newstr = "";
    }
  }
  if (!newstr.startsWith(": ")) {
    m_strBt.append(newstr);
    emit append(newstr);
  }
}

void BackTrace::slotProcessExited(TDEProcess *proc)
{
  // start it again
  kill(m_krashconf->pid(), SIGCONT);

  if (proc->normalExit() && (proc->exitStatus() == 0) &&
      usefulBacktrace())
  {
    processBacktrace();
    emit done(m_strBt);
  }
  else
    emit someError();
}

// analyze backtrace for usefulness
bool BackTrace::usefulBacktrace()
{
  // remove crap
  if( !m_krashconf->removeFromBacktraceRegExp().isEmpty()) {
    m_strBt.replace(TQRegExp( m_krashconf->removeFromBacktraceRegExp()), TQString());
  }

  // fix threading info output
  if( !m_krashconf->threadRegExp().isEmpty()) {
    int pos = -1;
    TQRegExp threadRegExpression( m_krashconf->threadRegExp());
    do {
      threadRegExpression.search(m_strBt);
      pos = threadRegExpression.pos();
      if (pos > -1) {
        m_strBt.insert(threadRegExpression.pos()+1, "==== ");
      }
    } while (pos > -1);
  }

  if( m_krashconf->disableChecks())
      return true;
  // prepend and append newline, so that regexps like '\nwhatever\n' work on all lines
  TQString strBt = '\n' + m_strBt + '\n';
  // how many " ?? " in the bt ?
  int unknown = 0;
  if( !m_krashconf->invalidStackFrameRegExp().isEmpty()) {
    int isfPos = strBt.find( TQRegExp( m_krashconf->invalidStackFrameRegExp()));
    int islPos = strBt.find( m_krashconf->infoSharedLibraryHeader());
    if ((isfPos >=0) && (isfPos < islPos)) {
      unknown = true;
    }
    else {
      unknown = false;
    }
  }
  // how many stack frames in the bt ?
  int frames = 0;
  if( !m_krashconf->frameRegExp().isEmpty())
    frames = strBt.contains( TQRegExp( m_krashconf->frameRegExp()));
  else
    frames = strBt.contains('\n');
  bool tooShort = false;
  if( !m_krashconf->neededInValidBacktraceRegExp().isEmpty())
    tooShort = ( strBt.find( TQRegExp( m_krashconf->neededInValidBacktraceRegExp())) == -1 );
  return !m_strBt.isNull() && !tooShort && (unknown < frames);
}

// remove stack frames added because of TDECrash
void BackTrace::processBacktrace()
{
	if( !m_krashconf->kcrashRegExp().isEmpty()) {
		TQRegExp kcrashregexp( m_krashconf->kcrashRegExp());
		kcrashregexp.setMinimal(true);
		int pos = 0;
		int prevpos = 0;
		while ((pos = kcrashregexp.search( m_strBt, pos )) >= 0) {
			if (prevpos == pos) {
				// Avoid infinite loop
				// Shouldn't ever get here, but given that this is a crash handler, better safe than sorry!
				break;
			}
			prevpos = pos;
			if( pos >= 0 ) {
				int len = kcrashregexp.matchedLength();
				int nextinfochunkpos = m_strBt.find("====", pos);
				if (nextinfochunkpos >= 0) {
					// Trying to delete too much!
					int limitedlen = nextinfochunkpos - pos;
					TQString limitedstrBt = m_strBt.mid(pos, limitedlen);
					int limitedpos = kcrashregexp.search( limitedstrBt );
					if (limitedpos >= 0) {
						len = kcrashregexp.matchedLength();
					}
					else {
						len = 0;
					}
				}
				if ((pos >= 0) && (len > 0)) {
					if( m_strBt[ pos ] == '\n' ) {
						++pos;
						--len;
					}
					m_strBt.remove( pos, len );
					m_strBt.insert( pos, TQString::fromLatin1( "[TDECrash handler]\n" ));
				}
			}
			if (pos < 0) {
				// Avoid infinite loop
				// Shouldn't ever get here, but given that this is a crash handler, better safe than sorry!
				break;
			}
			pos++;
			if ((uint)pos >= m_strBt.length()) {
				// Avoid infinite loop
				// Shouldn't ever get here, but given that this is a crash handler, better safe than sorry!
				break;
			}
		}
	}
	if( !m_krashconf->kcrashRegExpSingle().isEmpty()) {
		TQRegExp kcrashregexp( m_krashconf->kcrashRegExpSingle());
		int pos = kcrashregexp.search( m_strBt );
		if( pos >= 0 ) {
			int len = kcrashregexp.matchedLength();
			if( m_strBt[ pos ] == '\n' ) {
				++pos;
				--len;
			}
			m_strBt.remove( pos, len );
		}
	}

#ifdef HAVE_ELFICON
	m_strBt.append("\n==== (tdemetainfo) application version information ====\n");

	// Extract embedded SCM metadata from the crashed application
	TQString crashedExec = TDEStandardDirs::findExe(m_krashconf->execName());
	if (crashedExec.startsWith("/")) {
		libr_file *handle = NULL;
		libr_access_t access = LIBR_READ;

		if((handle = libr_open(const_cast<char*>(crashedExec.ascii()), access)) == NULL) {
			kdWarning() << "failed to open file" << crashedExec << endl;
		}
		else {
			TQString scmModule = elf_get_resource(handle, ".metadata_scmmodule");
			TQString scmRevision = elf_get_resource(handle, ".metadata_scmrevision");
			if (scmRevision != "") {
				m_strBt.append(TQString("%1:\t%2:%3\n").arg(TQFileInfo(crashedExec).fileName()).arg(scmModule).arg(scmRevision));
			}
		}

		libr_close(handle);
	}

	m_strBt.append("\n==== (tdemetainfo) library version information ====\n");

	// Extract embedded SCM metadata from shared libraries
	int islPos = m_strBt.find( m_krashconf->infoSharedLibraryHeader());
	TQString infoSharedLibraryText = m_strBt.mid(islPos);
	TQTextStream infoSharedLibraryTextStream(&infoSharedLibraryText, IO_ReadOnly);
	infoSharedLibraryTextStream.readLine();	// Skip info header x1
	infoSharedLibraryTextStream.readLine();	// Skip info header x2
	TQString infoSharedLibraryLine = infoSharedLibraryTextStream.readLine();
	while (infoSharedLibraryLine != TQString::null) {
		TQStringList libraryInfoList = TQStringList::split(" ", infoSharedLibraryLine, false);
		if (libraryInfoList.count() > 0) {
			TQString libraryName = libraryInfoList[libraryInfoList.count()-1];
			if (libraryName.startsWith("/")) {
				libr_file *handle = NULL;
				libr_access_t access = LIBR_READ;

				if((handle = libr_open(const_cast<char*>(libraryName.ascii()), access)) == NULL) {
					kdWarning() << "failed to open file " << libraryName << endl;
				}
				else {
					TQString scmModule = elf_get_resource(handle, ".metadata_scmmodule");
					TQString scmRevision = elf_get_resource(handle, ".metadata_scmrevision");
					if (scmRevision != "") {
						m_strBt.append(TQString("%1:\t%2:%3\n").arg(TQFileInfo(libraryName).fileName()).arg(scmModule).arg(scmRevision));
					}

					libr_close(handle);
				}
			}
		}
		infoSharedLibraryLine = infoSharedLibraryTextStream.readLine();
	}
#endif // HAVE_ELFICON

#ifdef WITH_TDEHWLIB
	// Append potentially important hardware information
	m_strBt.append("\n==== (tdehwlib) hardware information ====\n");
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	TDEGenericHardwareList hwlist = hwdevices->listAllPhysicalDevices();
	TDEGenericDevice *hwdevice;
	for ( hwdevice = hwlist.first(); hwdevice; hwdevice = hwlist.next() ) {
		if (hwdevice->type() == TDEGenericDeviceType::CPU) {
			TDECPUDevice* cpudevice = static_cast<TDECPUDevice*>(hwdevice);
			m_strBt.append(TQString("CPU core number:\t\t%1\n").arg(cpudevice->coreNumber()));
			m_strBt.append(TQString("\tVendor:\t\t\t%1\n").arg(cpudevice->vendorName()));
			m_strBt.append(TQString("\tModel:\t\t\t%1\n").arg(cpudevice->vendorModel()));
			m_strBt.append(TQString("\tName:\t\t\t%1\n").arg(cpudevice->name()));
			m_strBt.append(TQString("\tCurrent Frequency:\t%1 MHz\n").arg(cpudevice->frequency()));
			m_strBt.append(TQString("\tMinimum Frequency:\t%1 MHz\n").arg(cpudevice->minFrequency()));
			m_strBt.append(TQString("\tMaximum Frequency:\t%1 MHz\n").arg(cpudevice->maxFrequency()));
			m_strBt.append("\n");
		}
	}
#endif

	{
		// Clean up hard to read debug blocks
		TQRegExp kcrashregexp( "[^\n]\n==== ");
		kcrashregexp.setMinimal(true);
		int pos = 0;
		while ((pos = kcrashregexp.search( m_strBt, pos )) >= 0) {
			m_strBt.insert(pos+1, "\n");
		}
	}
}
