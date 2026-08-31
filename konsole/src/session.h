/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

#ifndef SESSION_H
#define SESSION_H

#include <tdeapplication.h>
#include <tdemainwindow.h>
#include <tqstrlist.h>

#include "TEPty.h"
#include "TEWidget.h"
#include "TEmuVt102.h"

#include "sessioniface.h"

class TDEProcIO;
class TDEProcess;

namespace Konsole
{

class ZModemDialog;

class TESession : public TQObject, virtual public SessionIface
{ TQ_OBJECT

public:

  TESession(TEWidget* w,
	    const TQString &term, ulong winId, const TQString &sessionId="session-1",
	    const TQString &initial_cwd = TQString::null);
  void changeWidget(TEWidget* w);
  void setPty( TEPty *_sh );
  TEWidget* widget() { return te; }
  ~TESession();

  void        setConnect(bool r);  // calls setListenToKeyPress(r)
  void        setListenToKeyPress(bool l);
  TEmulation* getEmulation();      // to control emulation
  bool        isSecure();
  bool        isMonitorActivity();
  bool        isMonitorSilence();
  bool        isMasterMode();
  int schemaNo();
  int encodingNo();
  int fontNo();
  const TQString& Term();
  const TQString& SessionId();
  const TQString& Title();
  const TQString& IconName();
  const TQString& IconText();
  TQString fullTitle() const;
  int keymapNo();
  TQString keymap();
  TQStrList getArgs();
  TQString getPgm();
  TQString getCwd();
  TQString getInitial_cwd() { return initial_cwd; }
  void setInitial_cwd(const TQString& _cwd) { initial_cwd=_cwd; }

  void setHistory(const HistoryType&);
  const HistoryType& history();

  void setMonitorActivity(bool);
  void setMonitorSilence(bool);
  void setMonitorSilenceSeconds(int seconds);
  void setMasterMode(bool);
  void setSchemaNo(int sn);
  void setEncodingNo(int index);
  void setKeymapNo(int kn);
  void setKeymap(const TQString& _id);
  void setFontNo(int fn);
  void setTitle(const TQString& _title);
  void setIconName(const TQString& _iconName);
  void setIconText(const TQString& _iconText);
  void setAddToUtmp(bool);
  void setXonXoff(bool);
  bool testAndSetStateIconName (const TQString& newname);
  bool sendSignal(int signal);

  void setAutoClose(bool b) { autoClose = b; }

  // Additional functions for DCOP
  bool closeSession();
  void clearHistory();
  void feedSession(const TQString &text);
  void sendSession(const TQString &text);
  void renameSession(const TQString &name);
  TQString sessionName() { return title; }
  int sessionPID() { return sh->pid(); }

  virtual bool processDynamic(const TQCString &fun, const TQByteArray &data, TQCString& replyType, TQByteArray &replyData);
  virtual QCStringList functionsDynamic();
  void enableFullScripting(bool b) { fullScripting = b; }

  void startZModem(const TQString &rz, const TQString &dir, const TQStringList &list);
  void cancelZModem();
  bool zmodemIsBusy() { return zmodemBusy; }

  void print(TQPainter &paint, bool friendly, bool exact);

  TQString schema();
  void setSchema(const TQString &schema);
  TQString encoding();
  void setEncoding(const TQString &encoding);
  TQString keytab();
  void setKeytab(const TQString &keytab);
  TQSize size();
  void setSize(TQSize size);
  void setFont(const TQString &font);
  TQString font();
  void setMetaAsAltMode(bool mode);

public slots:

  void run();
  void setProgram( const TQString &_pgm, const TQStrList &_args );
  void done();
  void done(int);
  void terminate();
  void setUserTitle( int, const TQString &caption );
  void changeTabTextColor( int );
  void ptyError();
  void slotZModemDetected();
  void emitZModemDetected();

  void zmodemStatus(TDEProcess *, char *data, int len);
  void zmodemSendBlock(TDEProcess *, char *data, int len);
  void zmodemRcvBlock(const char *data, int len);
  void zmodemDone();
  void zmodemContinue();

signals:

  void processExited(TDEProcess *);
  void forkedChild();
  void receivedData( const TQString& text );
  void done(TESession*);
  void updateTitle(TESession*);
  void notifySessionState(TESession* session, int state);
  void changeTabTextColor( TESession*, int );

  void disableMasterModeConnections();
  void enableMasterModeConnections();
  void renameSession(TESession* ses, const TQString &name);

  void openURLRequest(const TQString &cwd);

  void zmodemDetected(TESession *);
  void updateSessionConfig(TESession *);
  void resizeSession(TESession *session, TQSize size);
  void setSessionEncoding(TESession *session, const TQString &encoding);
  void getSessionSchema(TESession *session, TQString &schema);
  void setSessionSchema(TESession *session, const TQString &schema);
  void updateSessionKeytab(TESession *session, const TQString &keytab);

private slots:
  void onRcvBlock( const char* buf, int len );
  void monitorTimerDone();
  void notifySessionState(int state);
  void onContentSizeChange(int height, int width);
  void onFontMetricChange(int height, int width);

private:

  TEPty*         sh;
  TEWidget*      te;
  TEmulation*    em;

  bool           connected;
  bool           monitorActivity;
  bool           monitorSilence;
  bool           notifiedActivity;
  bool           masterMode;
  bool           autoClose;
  bool           wantedClose;
  TQTimer*        monitorTimer;

  //FIXME: using the indices here
  // is propably very bad. We should
  // use a persistent reference instead.
  int            schema_no;
  int            font_no;
  int            silence_seconds;

  int            font_h;
  int            font_w;

  TQString        title;
  TQString        userTitle;
  TQString        iconName;
  TQString        iconText; // as set by: echo -en '\033]1;IconText\007
  bool           add_to_utmp;
  bool           xon_xoff;
  bool           fullScripting;

  TQString	 stateIconName;

  TQString        pgm;
  TQStrList       args;

  TQString        term;
  ulong          winId;
  TQString        sessionId;

  TQString        cwd;
  TQString        initial_cwd;

  // ZModem
  bool           zmodemBusy;
  TDEProcIO*     zmodemProc;
  ZModemDialog*  zmodemProgress;

  // Color/Font Changes by ESC Sequences

  TQColor         modifiedBackground; // as set by: echo -en '\033]11;Color\007
  int            encoding_no;
};

}

#endif
