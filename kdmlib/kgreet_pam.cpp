/*

Conversation widget for kdm greeter

Copyright (C) 2008 Dirk Mueller <mueller@kde.org>

based on classic kdm greeter:

  Copyright (C) 1997, 1998, 2000 Steffen Hansen <hansen@kde.org>
  Copyright (C) 2000-2003 Oswald Buddenhagen <ossi@kde.org>


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

#include "kgreet_pam.h"
#include "themer/kdmthemer.h"
#include "themer/kdmlabel.h"

#include <klocale.h>
#include <klineedit.h>
#include <kpassdlg.h>
#include <kuser.h>

#include <tqregexp.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqtimer.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

//#define PAM_GREETER_DEBUG

class KDMPasswordEdit : public KPasswordEdit {
public:
	KDMPasswordEdit( TQWidget *parent ) : KPasswordEdit( parent, 0 ) {}
	KDMPasswordEdit( KPasswordEdit::EchoModes echoMode, TQWidget *parent ) : KPasswordEdit( echoMode, parent, 0 ) {}
protected:
	virtual void contextMenuEvent( TQContextMenuEvent * ) {}
};

static FILE* log;
static void kg_debug(const char* fmt, ...)
{
    va_list lst;
    va_start(lst, fmt);

#ifdef PAM_GREETER_DEBUG
#if 0
    vfprintf(log, fmt, lst);
    fflush(log);
#else
    char buf[6000];
    sprintf(buf, "*** %s\n", fmt);
    vsyslog(LOG_WARNING, buf, lst);
#endif
#endif
    va_end(lst);
}

static KPasswordEdit::EchoModes echoMode;

KPamGreeter::KPamGreeter( KGreeterPluginHandler *_handler,
                                  KdmThemer *themer,
                                  TQWidget *parent, TQWidget *pred,
                                  const TQString &_fixedEntity,
                                  Function _func, Context _ctx ) :
	TQObject(),
	KGreeterPlugin( _handler ),
	fixedUser( _fixedEntity ),
	func( _func ),
	ctx( _ctx ),
	exp( -1 ),
	pExp( -1 ),
	running( false )
{
        ctx = Login;

        kg_debug("KPamGreeter constructed\n");

	m_parentWidget = parent;

	KdmItem *user_entry = 0, *pw_entry = 0;
	int line = 0;

	layoutItem = 0;

	if (themer &&
	    (!(user_entry = themer->findNode( "user-entry" )) ||
	     !(pw_entry = themer->findNode( "pw-entry" ))))
		themer = 0;

        m_themer = themer;

	if (!themer)
		layoutItem = TQT_TQLAYOUTITEM(new TQGridLayout( 0, 0, 10 ));

        loginLabel = 0;
        authLabel.clear();
        authEdit.clear();
	loginLabel = 0;
	loginEdit = 0;
	if (ctx == ExUnlock || ctx == ExChangeTok)
		fixedUser = KUser().loginName();
	if (func != ChAuthTok) {
		kg_debug("func != ChAuthTok\n");
		kg_debug("fixedUser: *%s*\n", fixedUser.latin1());

		if (fixedUser.isEmpty()) {
			loginEdit = new KLineEdit( parent );
			loginEdit->setContextMenuEnabled( false );
			connect( loginEdit, TQT_SIGNAL(lostFocus()), TQT_SLOT(slotLoginLostFocus()) );
			connect( loginEdit, TQT_SIGNAL(lostFocus()), TQT_SLOT(slotActivity()) );
			connect( loginEdit, TQT_SIGNAL(textChanged( const TQString & )), TQT_SLOT(slotActivity()) );
			connect( loginEdit, TQT_SIGNAL(selectionChanged()), TQT_SLOT(slotActivity()) );
			if (pred) {
				parent->setTabOrder( pred, loginEdit );
				pred = loginEdit;
			}
			if (!getLayoutItem()) {
				loginEdit->adjustSize();
				user_entry->setWidget( loginEdit );
			} else {
				loginLabel = new TQLabel( loginEdit, i18n("Username:"), parent );
				getLayoutItem()->addWidget( loginLabel, line, 0 );
				getLayoutItem()->addWidget( loginEdit, line++, 1 );
			}
		} else if (ctx != Login && ctx != Shutdown && getLayoutItem()) {
			loginLabel = new TQLabel( i18n("Username:"), parent );
			getLayoutItem()->addWidget( loginLabel, line, 0 );
			getLayoutItem()->addWidget( new TQLabel( fixedUser, parent ), line++, 1 );
		}
#if 0
		if (echoMode == -1)
			passwdEdit = new KDMPasswordEdit( parent );
		else
			passwdEdit = new KDMPasswordEdit( echoMode,
			                                  parent );
		connect( passwdEdit, TQT_SIGNAL(textChanged( const TQString & )),
		         TQT_SLOT(slotActivity()) );
		connect( passwdEdit, TQT_SIGNAL(lostFocus()), TQT_SLOT(slotActivity()) );
		if (pred) {
			parent->setTabOrder( pred, passwdEdit );
			pred = passwdEdit;
		}
		if (!getLayoutItem()) {
			passwdEdit->adjustSize();
			pw_entry->setWidget( passwdEdit );
		} else {
			passwdLabel = new TQLabel( passwdEdit,
			                          func == Authenticate ?
			                            i18n("hello &Password:") :
			                            i18n("Current &password:"),
			                          parent );
			getLayoutItem()->addWidget( passwdLabel, line, 0 );
			getLayoutItem()->addWidget( passwdEdit, line++, 1 );
		}
#endif
		if (loginEdit)
			loginEdit->setFocus();
	}
	if (func != Authenticate) {
		if (echoMode == -1) {
                        authEdit << new KDMPasswordEdit( echoMode, parent );
			authEdit << new KDMPasswordEdit( echoMode, parent );
		} else {
			authEdit << new KDMPasswordEdit( parent );
			authEdit << new KDMPasswordEdit( parent );
		}
		authLabel << new TQLabel( authEdit[0], i18n("&New password:"), parent );
		authLabel << new TQLabel( authEdit[1], i18n("Con&firm password:"), parent );
		if (pred) {
			parent->setTabOrder( pred, authEdit[0] );
			parent->setTabOrder( authEdit[0], authEdit[1] );
		}
		if (getLayoutItem()) {
			getLayoutItem()->addWidget( authLabel[0], line, 0 );
			getLayoutItem()->addWidget( authEdit[0], line++, 1 );
			getLayoutItem()->addWidget( authLabel[1], line, 0 );
			getLayoutItem()->addWidget( authEdit[1], line, 1 );
		}
		if (authEdit.size() >= 2)
                        authEdit[1]->setFocus();
	}
}

// virtual
KPamGreeter::~KPamGreeter()
{
        kg_debug("KPamGreeter::~KPamGreeter");
	abort();
	if (!layoutItem) {
		delete loginEdit;
		return;
	}
	TQLayoutIterator it = TQT_TQLAYOUT(layoutItem)->iterator();
	for (TQLayoutItem *itm = it.current(); itm; itm = ++it)
		 delete itm->widget();
	delete layoutItem;
        kg_debug("destructor finished, good bye");
}

void // virtual
KPamGreeter::loadUsers( const TQStringList &users )
{
	KCompletion *userNamesCompletion = new KCompletion;
	userNamesCompletion->setItems( users );
	loginEdit->setCompletionObject( userNamesCompletion );
	loginEdit->setAutoDeleteCompletionObject( true );
	loginEdit->setCompletionMode( KGlobalSettings::CompletionAuto );
}

void // virtual
KPamGreeter::presetEntity( const TQString &entity, int field )
{
        kg_debug("presetEntity(%s,%d) called!\n", entity.latin1(), field);
	loginEdit->setText( entity );
	if (field == 1 && authEdit.size() >= 1)
		authEdit[0]->setFocus();
	else {
		loginEdit->setFocus();
		loginEdit->selectAll();
		if (field == -1 && authEdit.size() >= 1) {
			authEdit[0]->setText( "     " );
			authEdit[0]->setEnabled( false );
			authTok = false;
		}
	}
	curUser = entity;
}

TQString // virtual
KPamGreeter::getEntity() const
{
	return fixedUser.isEmpty() ? loginEdit->text() : fixedUser;
}

void // virtual
KPamGreeter::setUser( const TQString &user )
{
	// assert( fixedUser.isEmpty() );
	curUser = user;
	loginEdit->setText( user );
        if (authEdit.size() >= 1) {
	    authEdit[0]->setFocus();
 	    authEdit[0]->selectAll();
        }
}

void // virtual
KPamGreeter::setPassword( const TQString &pass )
{
	authEdit[0]->erase();
	authEdit[0]->insert( pass );
}

void // virtual
KPamGreeter::setEnabled(bool enable)
{
	// assert( !passwd1Label );
	// assert( func == Authenticate && ctx == Shutdown );
//	if (loginLabel)
//		loginLabel->setEnabled( enable );
		authEdit[0]->setEnabled( enable );
		setActive( enable );
		if (enable)
			authEdit[0]->setFocus();
	}

void // private
KPamGreeter::returnData()
{
       kg_debug("*************** returnData called with exp %d\n", exp);


	switch (exp) {
	case 0:
		handler->gplugReturnText( (loginEdit ? loginEdit->text() :
						       fixedUser).local8Bit(),
					  KGreeterPluginHandler::IsUser );
		break;
	case 1:
		handler->gplugReturnText( authEdit[0]->password(),
					  KGreeterPluginHandler::IsPassword |
					  KGreeterPluginHandler::IsSecret );
		break;
	case 2:
		handler->gplugReturnText( authEdit[1]->password(),
					  KGreeterPluginHandler::IsSecret );
		break;
	default: // case 3:
		handler->gplugReturnText( authEdit[2]->password(),
					  KGreeterPluginHandler::IsNewPassword |
					  KGreeterPluginHandler::IsSecret );
		break;
	}
}

bool // virtual
KPamGreeter::textMessage( const char *text, bool err )
{
    kg_debug(" ************** textMessage(%s, %d)\n", text, err);

    if (!authEdit.size())
	    return false;

    if (getLayoutItem()) {
      TQLabel* label = new TQLabel(TQString::fromUtf8(text), m_parentWidget);
      getLayoutItem()->addWidget(label, state+1, 0, 0);
    }

    return true;
}

void // virtual
KPamGreeter::textPrompt( const char *prompt, bool echo, bool nonBlocking )
{
    kg_debug("textPrompt called with prompt %s echo %d nonBlocking %d", prompt, echo, nonBlocking);
    kg_debug("state is %d, authEdit.size is %d\n", state, authEdit.size());

    if (state == 0 && echo) {
        if (loginLabel)
	    loginLabel->setText(TQString::fromUtf8(prompt));
        else if (m_themer) {
            KdmLabel *kdmlabel = static_cast<KdmLabel*>(m_themer->findNode("user-label"));
            if (kdmlabel) {
                //userLabel->setText(TQString::fromUtf8(prompt));
                kdmlabel->label.text = TQString::fromUtf8(prompt);
                TQTimer::singleShot(0, kdmlabel, TQT_SLOT(update()));
            }
        }
    }
    else if (state >= authEdit.size()) {
	if (getLayoutItem()) {
   	    TQLabel* label = new TQLabel(TQString::fromUtf8(prompt), m_parentWidget);
	    getLayoutItem()->addWidget(label, state+1, 0, 0);
            kg_debug("added label widget to layout");
        }
        else if (m_themer) {
            kg_debug("themer found!");
	    KdmItem *pw_label = 0;

            KdmLabel *kdmlabel = static_cast<KdmLabel*>(m_themer->findNode("pw-label"));
            if (kdmlabel) {
                //userLabel->setText(TQString::fromUtf8(prompt));
                TQString str = TQString::fromUtf8(prompt);
                kdmlabel->label.text = str;
                TQTimer::singleShot(0, kdmlabel, TQT_SLOT(update()));
            }
        }

	KDMPasswordEdit* passwdEdit;

	if (echoMode == -1)
	    passwdEdit = new KDMPasswordEdit( m_parentWidget );
	else
	    passwdEdit = new KDMPasswordEdit( echoMode, m_parentWidget);
	connect( passwdEdit, TQT_SIGNAL(textChanged( const TQString & )),
		TQT_SLOT(slotActivity()) );
	connect( passwdEdit, TQT_SIGNAL(lostFocus()), TQT_SLOT(slotActivity()) );
	authEdit << passwdEdit;

#if 1
       for(TQValueList<KPasswordEdit*>::iterator it = authEdit.begin();
        it != authEdit.end();
        ++it) {
       if ((*it)->isEnabled() && (*it)->text().isEmpty()) {
          (*it)->setFocus();
          break;
        }
       }
#endif
       if (getLayoutItem())
	   getLayoutItem()->addWidget(passwdEdit, state+1, 1, 0);

       if (m_themer) {
           kg_debug("themer found!");
	   KdmItem *pw_entry = 0;

	   pw_entry = m_themer->findNode("pw-entry");

	   if (pw_entry && passwdEdit)
	       pw_entry->setWidget(passwdEdit);

           if (0) {
                //userLabel->setText(TQString::fromUtf8(prompt));
                //kdmlabel->label.text = TQString::fromUtf8(prompt);
                //TQTimer::singleShot(0, kdmlabel, TQT_SLOT(update()));
           }
       } 
       else
           kg_debug("no themer found!");
    }
    ++state;
    pExp = exp;

    exp = authEdit.size();
    kg_debug("state %d exp: %d, has %d\n", state, exp, has);

    if (has >= exp || nonBlocking)
	returnData();
}

bool // virtual
KPamGreeter::binaryPrompt( const char *, bool )
{
	// this simply cannot happen ... :}
	return true;
}

void // virtual
KPamGreeter::start()
{
   kg_debug("******* start() called\n");

   while(authEdit.begin() != authEdit.end()) {
       KPasswordEdit* item = *authEdit.remove(authEdit.begin());
       delete item;
   }

   while(authLabel.begin() != authLabel.end()) {
       TQLabel* item = *authLabel.remove(authLabel.begin());
       delete item;
   }

   authTok = !(authEdit.size() >= 2 && authEdit[1]->isEnabled());
   exp = has = -1;
   state = 0;
   running = true;
   handler->gplugStart();
}

void // virtual
KPamGreeter::suspend()
{
}

void // virtual
KPamGreeter::resume()
{
}

void // virtual
KPamGreeter::next()
{
        kg_debug("********* next() called state %d\n", state);

	if (state == 0 && running && handler) {
                kg_debug(" **** returned text!\n");
		handler->gplugReturnText( (loginEdit ? loginEdit->text() :
		                  fixedUser).local8Bit(),
		                          KGreeterPluginHandler::IsUser );
                setActive(false);
        }
	
        has = 0;

    for(TQValueList<KPasswordEdit*>::iterator it = authEdit.begin();
        it != authEdit.end();
        ++it) {

          has++;
        if ((*it)->hasFocus()) {
          ++it;
          if (it != authEdit.end())
              (*it)->setFocus();
          break;
        }
        if (it == authEdit.end())
           has = -1;
   }

   kg_debug(" has %d and exp %d\n", has, exp);

#if 0
	// assert( running );
	if (loginEdit && loginEdit->hasFocus()) {
		passwdEdit->setFocus(); // will cancel running login if necessary
		has = 0;
	} else if (passwdEdit && passwdEdit->hasFocus()) {
		if (passwd1Edit)
			passwd1Edit->setFocus();
		has = 1;
	} else if (passwd1Edit) {
		if (passwd1Edit->hasFocus()) {
			passwd2Edit->setFocus();
			has = 1; // sic!
		} else
			has = 3;
	} else
		has = 1;
	if (exp < 0)
		handler->gplugStart();
#endif
	if (has >= exp)
		returnData();
}

void // virtual
KPamGreeter::abort()
{
	kg_debug("***** abort() called\n");

	running = false;
	if (exp >= 0) {
		exp = -1;
		handler->gplugReturnText( 0, 0 );
	}
}

void // virtual
KPamGreeter::succeeded()
{
        kg_debug("**** succeeded() called\n");

	// assert( running || timed_login );
	if (!authTok)
		setActive( false );
	else
		setAllActive( false );
	exp = -1;
	running = false;
}

void // virtual
KPamGreeter::failed()
{
	// assert( running || timed_login );
	setActive( false );
	setAllActive( false );
	exp = -1;
	running = false;
}

#include<assert.h>
void // virtual
KPamGreeter::revive()
{
	// assert( !running );
	setAllActive( true );

#if 1
        if (authEdit.size()  < 1)
                return;
#endif

        assert(authEdit.size() >= 1);
	if (authTok) {
		authEdit[0]->erase();
                if(authEdit.size() >= 2)
			authEdit[1]->erase();
		authEdit[0]->setFocus();
	} else {
		authEdit[0]->erase();
		if (loginEdit && loginEdit->isEnabled())
			authEdit[0]->setEnabled( true );
		else {
			setActive( true );
			if (loginEdit && loginEdit->text().isEmpty())
				loginEdit->setFocus();
			else
				authEdit[0]->setFocus();
		}
	}
}

void // virtual
KPamGreeter::clear()
{
	// assert( !running && !passwd1Edit );
	authEdit[0]->erase();
	if (loginEdit) {
		loginEdit->clear();
		loginEdit->setFocus();
		curUser = TQString::null;
	} else
		authEdit[0]->setFocus();
}


// private

void
KPamGreeter::setActive( bool enable )
{
	if (loginEdit)
		loginEdit->setEnabled( enable );
}

void
KPamGreeter::setAllActive( bool enable )
{
    for(TQValueList<KPasswordEdit*>::iterator it = authEdit.begin();
        it != authEdit.end();
        ++it)
        (*it)->setEnabled( enable );
}

void
KPamGreeter::slotLoginLostFocus()
{
	if (!running)
		return;
	if (exp > 0) {
		if (curUser == loginEdit->text())
			return;
		exp = -1;
		handler->gplugReturnText( 0, 0 );
	}
	curUser = loginEdit->text();
        kg_debug("curUser is %s", curUser.latin1());
	handler->gplugSetUser( curUser );
}

void
KPamGreeter::slotActivity()
{  
        kg_debug("slotActivity");

	if (running)
		handler->gplugActivity();
}

// factory

static bool init( const TQString &,
                  TQVariant (*getConf)( void *, const char *, const TQVariant & ),
                  void *ctx )
{
	echoMode = (KPasswordEdit::EchoModes) getConf( ctx, "EchoMode", TQVariant( -1 ) ).toInt();
	KGlobal::locale()->insertCatalogue( "kgreet_pam" );
	return true;
}

static void done( void )
{
	KGlobal::locale()->removeCatalogue( "kgreet_pam" );
        if (log && log != stderr)
	    fclose(log);
        log = 0;
}

static KGreeterPlugin *
create( KGreeterPluginHandler *handler, KdmThemer *themer,
        TQWidget *parent, TQWidget *predecessor,
        const TQString &fixedEntity,
        KGreeterPlugin::Function func,
        KGreeterPlugin::Context ctx )
{
	return new KPamGreeter( handler, themer, parent, predecessor, fixedEntity, func, ctx );
}

KDE_EXPORT kgreeterplugin_info kgreeterplugin_info = {
	I18N_NOOP("Pam conversation plugin"), "pam",
	kgreeterplugin_info::Local | kgreeterplugin_info::Presettable,
	init, done, create
};

#include "kgreet_pam.moc"
