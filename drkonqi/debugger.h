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

#ifndef DEBUGGER_H
#define DEBUGGER_H

class TQLabel;
class TQPushButton;
class KrashConfig;
class KTextBrowser;
class BackTrace;

#include <tqwidget.h>

class KrashDebugger : public TQWidget
{
  TQ_OBJECT

public:
  KrashDebugger(const KrashConfig *krashconf, TQWidget *parent = 0, const char *name = 0);
  ~KrashDebugger();

public slots:
  void slotAppend(const TQString &);
  void slotDone(const TQString&);
  void slotSomeError();

protected:
  void startDebugger();
  bool performChecks( TQString* msg );

  virtual void showEvent(TQShowEvent *e);

protected slots:
  void slotCopy();
  void slotSave();

private:
  const KrashConfig *m_krashconf;
  BackTrace *m_proctrace;
  TQLabel *m_status;
  KTextBrowser *m_backtrace;
  TQPushButton * m_copyButton;
  TQPushButton * m_saveButton;
  TQString m_prependText;
};

#endif
