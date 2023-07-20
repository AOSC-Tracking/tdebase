/*

Shutdown dialog

Copyright (C) 1997, 1998 Steffen Hansen <hansen@kde.org>
Copyright (C) 2000-2003,2005 Oswald Buddenhagen <ossi@kde.org>


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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/


#ifndef TDMSHUTDOWN_H
#define TDMSHUTDOWN_H

#include "tdmconfig.h" // for HAVE_VTS
#include "kgverify.h"

#include <kpushbutton.h>

#include <tqradiobutton.h>
#include <tqtoolbutton.h>
#include <tqpixmap.h>

class TQLabel;
class KPushButton;
class TQButtonGroup;
class TQGroupBox;
class TQComboBox;
class TQCheckBox;
class TQLineEdit;

enum { Authed = TQDialog::Accepted + 1, Schedule };

class TDMShutdownBase : public FDialog, public KGVerifyHandler {
	TQ_OBJECT
	typedef FDialog inherited;

  public:
	TDMShutdownBase( int _uid, TQWidget *_parent );
	virtual ~TDMShutdownBase();

  protected slots:
	virtual void accept();

  protected:
	virtual void accepted();

  protected:
	void updateNeedRoot();
	void complete( TQWidget *prevWidget );

	TQVBoxLayout *box;
#ifdef HAVE_VTS
	bool willShut;
#else
	static const bool willShut = true;
#endif
	bool mayNuke, doesNuke, mayOk, maySched;

  private slots:
	void slotSched();
	void slotActivatePlugMenu();

  private:
	KPushButton *okButton, *cancelButton;
	TQLabel *rootlab;
	KGStdVerify *verify;
	int needRoot, uid;

	static int curPlugin;
	static PluginList pluginList;

  public: // from KGVerifyHandler
	virtual void verifyPluginChanged( int id );
	virtual void verifyOk();
	virtual void verifyFailed();
	virtual void verifyRetry();
	virtual void verifySetUser( const TQString &user );
};


class TDMShutdown : public TDMShutdownBase {
	TQ_OBJECT
	typedef TDMShutdownBase inherited;

  public:
	TDMShutdown( int _uid, TQWidget *_parent = 0 );
	static void scheduleShutdown( TQWidget *_parent = 0 );

  protected slots:
	virtual void accept();

  protected:
	virtual void accepted();

  private slots:
	void slotTargetChanged();
	void slotWhenChanged();

  private:
	TQButtonGroup *howGroup;
	TQGroupBox *schedGroup;
	TQRadioButton *restart_rb;
	TQLineEdit *le_start, *le_timeout;
	TQCheckBox *cb_force;
	TQComboBox *targets;
	int oldTarget;
	int sch_st, sch_to;

};

class TDMRadioButton : public TQRadioButton {
	TQ_OBJECT
	typedef TQRadioButton inherited;

  public:
	TDMRadioButton( const TQString &label, TQWidget *parent );

  private:
	virtual void mouseDoubleClickEvent( TQMouseEvent * );

  signals:
	void doubleClicked();

};

class TDMDelayedPushButton : public KPushButton {
	TQ_OBJECT
	typedef KPushButton inherited;

  public:
	TDMDelayedPushButton( const KGuiItem &item, TQWidget *parent, const char *name = 0 );
	void setPopup( TQPopupMenu *pop );

  private slots:
	void slotTimeout();
	void slotPressed();
	void slotReleased();

  private:
	TQPopupMenu *pop;
	TQTimer popt;
};

class TDMSlimShutdown : public FDialog {
	TQ_OBJECT
	typedef FDialog inherited;

  public:
	TDMSlimShutdown( TQWidget *_parent = 0 );
	~TDMSlimShutdown();
	static void externShutdown( int type, const char *os, int uid );

  private slots:
	void slotHalt();
	void slotReboot();
	void slotReboot( int );
	void slotSched();

  private:
	bool checkShutdown( int type, const char *os );
	char **targetList;

};

class TDMConfShutdown : public TDMShutdownBase {
	TQ_OBJECT
	typedef TDMShutdownBase inherited;

  public:
	TDMConfShutdown( int _uid, struct dpySpec *sess, int type, const char *os,
	                 TQWidget *_parent = 0 );
};

class TDMCancelShutdown : public TDMShutdownBase {
	TQ_OBJECT
	typedef TDMShutdownBase inherited;

  public:
	TDMCancelShutdown( int how, int start, int timeout, int force, int uid,
	                   const char *os, TQWidget *_parent );
};

class KSMPushButton : public KPushButton
{
  TQ_OBJECT

public:

  KSMPushButton( const KGuiItem &item, TQWidget *parent, const char *name = 0 );

protected:
  virtual void keyPressEvent(TQKeyEvent*e);
  virtual void keyReleaseEvent(TQKeyEvent*e);

private:

 bool m_pressed;

};

class FlatButton : public TQToolButton
{
  TQ_OBJECT

 public:

  FlatButton( TQWidget *parent = 0, const char *name = 0 );
  ~FlatButton();

 protected:
  virtual void keyPressEvent(TQKeyEvent*e);
  virtual void keyReleaseEvent(TQKeyEvent*e);

 private slots:
  
 private:
  void init();
  
  bool m_pressed;
  TQString m_text;
  TQPixmap m_pixmap;
 
};

#endif /* TDMSHUTDOWN_H */
