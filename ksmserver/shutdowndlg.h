/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2012 Serghei Amelian <serghei.amelian@gmail.com>
Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#ifndef SHUTDOWNDLG_H
#define SHUTDOWNDLG_H

#include <tqpixmap.h>
#include <tqimage.h>
#include <tqdatetime.h>
#include <kdialog.h>
#include <kpushbutton.h>
#include <tqpushbutton.h>
#include <tqframe.h>
#include <kguiitem.h>
#include <tqtoolbutton.h>
#include <krootpixmap.h>
#include <ntqprogressbar.h>

class TQPushButton;
class TQVButtonGroup;
class TQPopupMenu;
class TQTimer;
class TQPainter;
class TQString;
class TDEAction;

#include "timed.h"
#include <tdeapplication.h>
#include <kpixmapio.h>

#include <config.h>

namespace SuspendType {
enum SuspendType {
	NotSpecified = 0,
	Freeze,
	Standby,
	Suspend,
	Hibernate,
	HybridSuspend
};
};

// The (singleton) widget that makes/fades the desktop gray.
class KSMShutdownFeedback : public TQWidget
{
	TQ_OBJECT

public:
	static void start() { s_pSelf = new KSMShutdownFeedback(); }
	static void stop() { if ( s_pSelf != 0L ) s_pSelf->fadeBack(); delete s_pSelf; s_pSelf = 0L; }
	static KSMShutdownFeedback * self() { return s_pSelf; }

protected:
	~KSMShutdownFeedback() {}

private slots:
	void slotPaintEffect();

private:
	static KSMShutdownFeedback * s_pSelf;
	KSMShutdownFeedback();
	int m_currentY;
	TQPixmap m_root;
	void fadeBack( void );
	float  m_grayOpacity;
	float  m_compensation;
	bool   m_fadeBackwards;
	bool   m_readDelayComplete;
	TQImage m_unfadedImage;
	TQImage m_grayImage;
	TQTime  m_fadeTime;
	int    m_rowsDone;
	KPixmapIO m_pmio;
	bool m_greyImageCreated;
};

// The (singleton) widget that shows either pretty pictures or a black screen during logout
class KSMShutdownIPFeedback : public TQWidget
{
	TQ_OBJECT

public:
	static void start() { s_pSelf = new KSMShutdownIPFeedback(); }
	static void showit() { if ( s_pSelf != 0L ) s_pSelf->showNow(); }
	static void stop() { if ( s_pSelf != 0L ) s_pSelf->fadeBack(); delete s_pSelf; s_pSelf = 0L; }
	static KSMShutdownIPFeedback * self() { return s_pSelf; }
	static bool ispainted() { if ( s_pSelf != 0L ) return s_pSelf->m_isPainted; else return false; }

protected:
	~KSMShutdownIPFeedback();
	virtual void resizeEvent(TQResizeEvent* re);

public slots:
	void slotPaintEffect();
	void slotSetBackgroundPixmap(const TQPixmap &);

private:
	/**
	* Asks KDesktop to export the desktop background as a TDESharedPixmap.
	* This method uses DCOP to call KBackgroundIface/setExport(int).
	*/
	void enableExports();

private:
	static KSMShutdownIPFeedback * s_pSelf;
	KSMShutdownIPFeedback();
	int m_currentY;
	TQPixmap m_root;
	void fadeBack( void );
	void showNow( void );
	int m_timeout;
	bool m_isPainted;
	bool m_paintedFromSharedRootPixmap;
	KRootPixmap* m_sharedRootPixmap;
	TQPixmap m_rootPixmap;
	int mPixmapTimeout;
	TQRect m_screenGeometry;
};

// The confirmation dialog
class KSMShutdownDlg : public TQDialog
{
	TQ_OBJECT

public:
	static bool confirmShutdown( bool maysd, bool mayrb, TDEApplication::ShutdownType& sdtype, TQString& bopt, int* selection=0 );

public slots:
	void slotLogout();
	void slotHalt();
	void slotReboot();
	void slotReboot(int);
	void slotSuspend();
	void slotHibernate();
	void slotFreeze();
	void slotHybridSuspend();

protected:
	~KSMShutdownDlg();

private:
	KSMShutdownDlg( TQWidget* parent, bool maysd, bool mayrb, TDEApplication::ShutdownType sdtype, int* selection=0 );
	TDEApplication::ShutdownType m_shutdownType;
	TQString m_bootOption;
	TQPopupMenu *targets;
	TQStringList rebootOptions;
	bool m_lockOnResume;
	int* m_selection;
};

// The shutdown-in-progress dialog
class KSMShutdownIPDlg : public KSMModalDialog
{
	TQ_OBJECT

public:
	static TQWidget* showShutdownIP();

	void showNotificationActionButtons();
	void hideNotificationActionButtons();
	void setNotificationActionButtonsSkipText(TQString text);
	void setProgressBarTotalSteps(int total_steps);
	void setProgressBarProgress(int step);

signals:
	void abortLogoutClicked();
	void skipNotificationClicked();

protected:
	~KSMShutdownIPDlg();

private:
	KSMShutdownIPDlg( TQWidget* parent );

	TQProgressBar *m_progressbar;
};

class KSMDelayedPushButton : public KPushButton
{
  TQ_OBJECT

public:

  KSMDelayedPushButton( const KGuiItem &item, TQWidget *parent, const char *name = 0 );
  void setPopup( TQPopupMenu *pop);

private slots:
  void slotTimeout();
  void slotPressed();
  void slotReleased();

private:
  TQPopupMenu *pop;
  TQTimer *popt;
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




class TQLabel;

class  KSMDelayedMessageBox : public TimedLogoutDlg
{
	TQ_OBJECT

public:
	KSMDelayedMessageBox( TDEApplication::ShutdownType sdtype, const TQString &bootOption, int confirmDelay );
	static bool showTicker( TDEApplication::ShutdownType sdtype, const TQString &bootOption, int confirmDelay );

protected slots:
	void updateText();

private:
	TQString m_template;
	int m_remaining;
};

#endif
