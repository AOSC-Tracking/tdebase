/*

Conversation widget for kdm greeter

Copyright (C) 2008 Dirk Mueller <mueller@kde.org>


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


#ifndef KGREET_CLASSIC_H
#define KGREET_CLASSIC_H

#include "kgreeterplugin.h"

#include <tqobject.h>
#include <tqlayout.h>

class KLineEdit;
class KPasswordEdit;
class KSimpleConfig;
class TQGridLayout;
class TQLabel;

class KPamGreeter : public TQObject, public KGreeterPlugin {
	Q_OBJECT

  public:
	KPamGreeter( KGreeterPluginHandler *handler,
	                 KdmThemer *themer,
	                 TQWidget *parent, TQWidget *predecessor,
	                 const TQString &fixedEntitiy,
	                 Function func, Context ctx );
	~KPamGreeter();
	virtual void loadUsers( const TQStringList &users );
	virtual void presetEntity( const TQString &entity, int field );
	virtual TQString getEntity() const;
	virtual void setUser( const TQString &user );
	virtual void setPassword( const TQString &pass );
	virtual void setEnabled( bool on );
	virtual bool textMessage( const char *message, bool error );
	virtual void textPrompt( const char *prompt, bool echo, bool nonBlocking );
	virtual bool binaryPrompt( const char *prompt, bool nonBlocking );
	virtual void start();
	virtual void suspend();
	virtual void resume();
	virtual void next();
	virtual void abort();
	virtual void succeeded();
	virtual void failed();
	virtual void revive();
	virtual void clear();

        TQGridLayout *getLayoutItem() const { return static_cast<TQGridLayout*>(TQT_TQLAYOUT(layoutItem)); }

  public slots:
	void slotLoginLostFocus();
	void slotActivity();

  private:
	void setActive( bool enable );
	void setAllActive( bool enable );
	void returnData();

        TQLabel *loginLabel;
        TQValueList<TQLabel*> authLabel;
	KLineEdit *loginEdit;
        TQWidget* m_parentWidget;
        TQValueList<KPasswordEdit*> authEdit;
	KSimpleConfig *stsFile;
        KdmThemer *m_themer;
	TQString fixedUser, curUser;
	Function func;
	Context ctx;
	int exp, pExp, has;
        unsigned state;
	bool running, authTok;
};

#endif /* KGREET_CLASSIC_H */
