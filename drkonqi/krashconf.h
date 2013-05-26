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

#ifndef KRASHCONF_H
#define KRASHCONF_H

#include <kaboutdata.h>
#include <tqstring.h>
#include <tqobject.h>

#include "krashdcopinterface.h"

class KrashConfig : public TQObject, public KrashDCOPInterface
{
  Q_OBJECT

public:
  KrashConfig();
  virtual ~KrashConfig();

k_dcop:
  virtual TQString programName() const { return m_aboutData->programName(); };
  virtual TQCString appName() const { return m_aboutData->appName(); };
  virtual int signalNumber() const { return m_signalnum; };
  virtual int pid() const { return m_pid; };
  virtual bool startedByKdeinit() const { return m_startedByKdeinit; };
  virtual bool safeMode() const { return m_safeMode; };
  virtual TQString signalName() const { return m_signalName; };
  virtual TQString signalText() const { return m_signalText; };
  virtual TQString whatToDoText() const { return m_whatToDoText; }
  virtual TQString errorDescriptionText() const { return m_errorDescriptionText; };

  virtual ASYNC registerDebuggingApplication(const TQString& launchName);

public:
  TQString debugger() const { return m_debugger; }
  TQString debuggerBatch() const { return m_debuggerBatch; }
  TQString tryExec() const { return m_tryExec; }
  TQString backtraceCommand() const { return m_backtraceCommand; }
  TQString removeFromBacktraceRegExp() const { return m_removeFromBacktraceRegExp; }
  TQString invalidStackFrameRegExp() const { return m_invalidStackFrameRegExp; }
  TQString frameRegExp() const { return m_frameRegExp; }
  TQString neededInValidBacktraceRegExp() const { return m_neededInValidBacktraceRegExp; }
  TQString kcrashRegExp() const { return m_kcrashRegExp; }
  TQString kcrashRegExpSingle() const { return m_kcrashRegExpSingle; }
  TQString threadRegExp() const { return m_threadRegExp; }
  bool showBacktrace() const { return m_showbacktrace; };
  bool showDebugger() const { return m_showdebugger && !m_debugger.isNull(); };
  bool showBugReport() const { return m_showbugreport; };
  bool disableChecks() const { return m_disablechecks; };
  const KAboutData *aboutData() const { return m_aboutData; }
  TQString execName() const { return m_execname; }

  void expandString(TQString &str, bool shell, const TQString &tempFile = TQString::null) const;

  void acceptDebuggingApp();

signals:
  void newDebuggingApplication(const TQString& launchName);

private:
  void readConfig();

private:
  KAboutData *m_aboutData;
  int m_pid;
  int m_signalnum;
  bool m_showdebugger;
  bool m_showbacktrace;
  bool m_showbugreport;
  bool m_startedByKdeinit;
  bool m_safeMode;
  bool m_disablechecks;
  TQString m_signalName;
  TQString m_signalText;
  TQString m_whatToDoText;
  TQString m_errorDescriptionText;
  TQString m_execname;

  TQString m_debugger;
  TQString m_debuggerBatch;
  TQString m_tryExec;
  TQString m_backtraceCommand;
  TQString m_removeFromBacktraceRegExp;
  TQString m_invalidStackFrameRegExp;
  TQString m_frameRegExp;
  TQString m_neededInValidBacktraceRegExp;
  TQString m_kcrashRegExp;
  TQString m_kcrashRegExpSingle;
  TQString m_threadRegExp;
};

#endif
