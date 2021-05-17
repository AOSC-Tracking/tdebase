/*

Shell for tdm conversation plugins

Copyright (C) 1997, 1998 Steffen Hansen <hansen@kde.org>
Copyright (C) 2000-2004 Oswald Buddenhagen <ossi@kde.org>


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


#ifndef KGVERIFY_H
#define KGVERIFY_H

#include "kgreeterplugin.h"
#include "kfdialog.h"

#include <tqlayout.h>
#include <tqtimer.h>
#include <tqvaluevector.h>

#include <sys/time.h>
#include <time.h>

// helper class, nuke when qt supports suspend()/resume()
class QXTimer : public TQObject {
	Q_OBJECT
	typedef TQObject inherited;

  public:
	QXTimer();
	void start( int msec );
	void stop();
	void suspend();
	void resume();

  signals:
	void timeout();

  private slots:
	void slotTimeout();

  private:
	TQTimer timer;
	struct timeval stv;
	long left;
};

class KGVerifyHandler {
  public:
	virtual void verifyPluginChanged( int id ) = 0;
	virtual void verifyClear();
	virtual void verifyOk() = 0;
	virtual void verifyFailed() = 0;
	virtual void verifyRetry() = 0;
	virtual void verifySetUser( const TQString &user ) = 0;
	virtual void updateStatus( bool fail, bool caps, int left ); // for themed only
};

class TQWidget;
class TQLabel;
class TQPopupMenu;
class TQTimer;
class KPushButton;
class KLibrary;
#ifdef WITH_TDEHWLIB
class TDECryptographicCardDevice;
#endif

struct GreeterPluginHandle {
	KLibrary *library;
	kgreeterplugin_info *info;
};

typedef TQValueVector<int> PluginList;

class KGVerify : public TQObject, public KGreeterPluginHandler {
	Q_OBJECT
	typedef TQObject inherited;

  public:
	KGVerify( KGVerifyHandler *handler, KdmThemer *themer,
	          TQWidget *parent, TQWidget *predecessor,
	          const TQString &fixedEntity, const PluginList &pluginList,
	          KGreeterPlugin::Function func, KGreeterPlugin::Context ctx );
	virtual ~KGVerify();
	TQPopupMenu *getPlugMenu();
	void loadUsers( const TQStringList &users );
	void presetEntity( const TQString &entity, int field );
	TQString getEntity() const;
	void setUser( const TQString &user );
	void lockUserEntry( const bool lock );
	void setPassword( const TQString &pass );
	void setInfoMessageDisplay( bool on );
	void setPasswordPrompt(const TQString &prompt);
	/* virtual */ void selectPlugin( int id );
	bool entitiesLocal() const;
	bool entitiesFielded() const;
	bool entityPresettable() const;
	bool isClassic() const;
	TQString pluginName() const;
	void setEnabled( bool on );
	void abort();
	void suspend();
	void resume();
	void accept();
	void reject();
	void requestAbort();

	int coreLock;

	static bool handleFailVerify( TQWidget *parent );
	static PluginList init( const TQStringList &plugins );
	static void done();

  public slots:
	void start();

  protected:
	bool eventFilter( TQObject *, TQEvent * );
	void MsgBox( TQMessageBox::Icon typ, const TQString &msg );
	void setTimer();
	void updateLockStatus();
	virtual void updateStatus() = 0;
	void handleVerify();

	QXTimer timer;
	TQString fixedEntity, presEnt, curUser;
	PluginList pluginList;
	KGVerifyHandler *handler;
	KdmThemer *themer;
	TQWidget *parent, *predecessor;
	KGreeterPlugin *greet;
	TQPopupMenu *plugMenu;
	int curPlugin, presFld, timedLeft, deadTicks;
	TQCString pName;
	KGreeterPlugin::Function func;
	KGreeterPlugin::Context ctx;
	bool capsLocked;
	bool enabled, running, suspended, failed, delayed, cont;
	bool authTok, isClear, timeable;
	bool inGreeterPlugin;
	bool abortRequested;

	static void VMsgBox( TQWidget *parent, const TQString &user, TQMessageBox::Icon type, const TQString &mesg );
	static void VErrBox( TQWidget *parent, const TQString &user, const char *msg );
	static void VInfoBox( TQWidget *parent, const TQString &user, const char *msg );

	static TQValueVector<GreeterPluginHandle> greetPlugins;

  private:
	bool applyPreset();
	void performAutoLogin();
	bool scheduleAutoLogin( bool initial );
	void doReject( bool initial );
	void setPassPromptText(TQString text, bool use_default_text=false);

  private slots:
	//virtual void slotPluginSelected( int id ) = 0;
	void slotTimeout();
	void slotActivity();

  public: // from KGreetPluginHandler
	virtual void gplugReturnText( const char *text, int tag );
	virtual void gplugReturnBinary( const char *data );
	virtual void gplugSetUser( const TQString &user );
	virtual void gplugStart();
	virtual void gplugActivity();
	virtual void gplugMsgBox( TQMessageBox::Icon type, const TQString &text );

	static TQVariant getConf( void *ctx, const char *key, const TQVariant &dflt );

#ifdef WITH_TDEHWLIB
	bool cardLoginInProgress;
	TDECryptographicCardDevice* cardLoginDevice;
#endif
};

class KGStdVerify : public KGVerify {
	Q_OBJECT
	typedef KGVerify inherited;

  public:
	KGStdVerify( KGVerifyHandler *handler, TQWidget *parent,
	             TQWidget *predecessor, const TQString &fixedEntity,
	             const PluginList &pluginList,
	             KGreeterPlugin::Function func, KGreeterPlugin::Context ctx );
	virtual ~KGStdVerify();
	TQLayout *getLayout() const { return TQT_TQLAYOUT(grid); }
	void selectPlugin( int id );

  protected:
	void updateStatus();

  private:
	TQGridLayout *grid;
	TQLabel *failedLabel;
	int failedLabelState;

  private slots:
	void slotPluginSelected( int id );
};

class KGThemedVerify : public KGVerify {
	Q_OBJECT
	typedef KGVerify inherited;

  public:
	KGThemedVerify( KGVerifyHandler *handler, KdmThemer *themer,
	                TQWidget *parent, TQWidget *predecessor,
	                const TQString &fixedEntity,
	                const PluginList &pluginList,
	                KGreeterPlugin::Function func,
	                KGreeterPlugin::Context ctx );
	virtual ~KGThemedVerify();
	void selectPlugin( int id );

  protected:
	void updateStatus();

  private slots:
	void slotPluginSelected( int id );
};

class KGChTok : public FDialog, public KGVerifyHandler {
	Q_OBJECT
	typedef FDialog inherited;

  public:
	KGChTok( TQWidget *parent, const TQString &user,
	         const PluginList &pluginList, int curPlugin,
	         KGreeterPlugin::Function func, KGreeterPlugin::Context ctx );
	~KGChTok();

  public slots:
	void accept();

  private:
	KPushButton *okButton, *cancelButton;
	KGStdVerify *verify;

  public: // from KGVerifyHandler
	virtual void verifyPluginChanged( int id );
	virtual void verifyOk();
	virtual void verifyFailed();
	virtual void verifyRetry();
	virtual void verifySetUser( const TQString &user );
};

#endif /* KGVERIFY_H */
