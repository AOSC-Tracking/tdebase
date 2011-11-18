/*

Conversation widget for kdm greeter

Copyright (C) 1997, 1998, 2000 Steffen Hansen <hansen@kde.org>
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

#include "kgreet_winbind.h"
#include "themer/kdmthemer.h"
#include "themer/kdmitem.h"

#include <klocale.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kpassdlg.h>
#include <kuser.h>
#include <kprocio.h>

#include <tqregexp.h>
#include <tqlayout.h>
#include <tqlabel.h>

#include <stdlib.h>

class KDMPasswordEdit : public KPasswordEdit {
public:
	KDMPasswordEdit( TQWidget *parent ) : KPasswordEdit( parent, 0 ) {}
	KDMPasswordEdit( KPasswordEdit::EchoModes echoMode, TQWidget *parent ) : KPasswordEdit( echoMode, parent, 0 ) {}
protected:
	virtual void contextMenuEvent( TQContextMenuEvent * ) {}
};

static int echoMode;
static char separator;
static TQStringList staticDomains;
static TQString defaultDomain;

static void
splitEntity( const TQString &ent, TQString &dom, TQString &usr )
{
	int pos = ent.find( separator );
	if (pos < 0)
		dom = "<local>", usr = ent;
	else
		dom = ent.left( pos ), usr = ent.mid( pos + 1 );
}

KWinbindGreeter::KWinbindGreeter( KGreeterPluginHandler *_handler,
                                  KdmThemer *themer,
                                  TQWidget *parent, TQWidget *pred,
                                  const TQString &_fixedEntity,
                                  Function _func, Context _ctx ) :
	TQObject(),
	KGreeterPlugin( _handler ),
	func( _func ),
	ctx( _ctx ),
	exp( -1 ),
	pExp( -1 ),
	running( false )
{
	KdmItem *user_entry = 0, *pw_entry = 0, *domain_entry = 0;
	TQGridLayout *grid = 0;

	int line = 0;
	layoutItem = 0;

	if (themer &&
	    (!(user_entry = themer->findNode( "user-entry" )) ||
	     !(pw_entry = themer->findNode( "pw-entry" )) ||
	     !(domain_entry = themer->findNode( "domain-entry" ))))
		themer = 0;

	if (!themer)
		grid = new TQGridLayout( 0, 0, 10 );
		layoutItem = TQT_TQLAYOUTITEM(grid);

	domainLabel = loginLabel = passwdLabel = passwd1Label = passwd2Label = 0;
	domainCombo = 0;
	loginEdit = 0;
	passwdEdit = passwd1Edit = passwd2Edit = 0;
	m_domainLister = 0;
	if (ctx == ExUnlock || ctx == ExChangeTok)
		splitEntity( KUser().loginName(), fixedDomain, fixedUser );
	else
		splitEntity( _fixedEntity, fixedDomain, fixedUser );
	if (func != ChAuthTok) {
		if (fixedUser.isEmpty()) {
			domainCombo = new KComboBox( parent );
			connect( domainCombo, TQT_SIGNAL(activated( const TQString & )),
			         TQT_SLOT(slotChangedDomain( const TQString & )) );
			connect( domainCombo, TQT_SIGNAL(activated( const TQString & )),
			         TQT_SLOT(slotLoginLostFocus()) );
			connect( domainCombo, TQT_SIGNAL(activated( const TQString & )),
			         TQT_SLOT(slotActivity()) );
			// should handle loss of focus
			loginEdit = new KLineEdit( parent );
			loginEdit->setContextMenuEnabled( false );

			if (pred) {
				parent->setTabOrder( pred, domainCombo );
				parent->setTabOrder( domainCombo, loginEdit );
				pred = loginEdit;
			}
			if (!grid) {
				loginEdit->adjustSize();
				domainCombo->adjustSize();
				user_entry->setWidget( loginEdit );
				domain_entry->setWidget( domainCombo );
			} else {
				domainLabel = new TQLabel( domainCombo, i18n("&Domain:"), parent );
				loginLabel = new TQLabel( loginEdit, i18n("&Username:"), parent );
				grid->addWidget( domainLabel, line, 0 );
				grid->addWidget( domainCombo, line++, 1 );
				grid->addWidget( loginLabel, line, 0 );
				grid->addWidget( loginEdit, line++, 1 );
			}
			connect( loginEdit, TQT_SIGNAL(lostFocus()), TQT_SLOT(slotLoginLostFocus()) );
			connect( loginEdit, TQT_SIGNAL(lostFocus()), TQT_SLOT(slotActivity()) );
			connect( loginEdit, TQT_SIGNAL(textChanged( const TQString & )), TQT_SLOT(slotActivity()) );
			connect( loginEdit, TQT_SIGNAL(selectionChanged()), TQT_SLOT(slotActivity()) );
			connect(&mDomainListTimer, TQT_SIGNAL(timeout()), TQT_SLOT(slotStartDomainList()));
			domainCombo->insertStringList( staticDomains );
			TQTimer::singleShot(0, this, TQT_SLOT(slotStartDomainList()));
		} else if (ctx != Login && ctx != Shutdown && grid) {
			domainLabel = new TQLabel( i18n("Domain:"), parent );
			grid->addWidget( domainLabel, line, 0 );
			grid->addWidget( new TQLabel( fixedDomain, parent ), line++, 1 );
			loginLabel = new TQLabel( i18n("Username:"), parent );
			grid->addWidget( loginLabel, line, 0 );
			grid->addWidget( new TQLabel( fixedUser, parent ), line++, 1 );
		}
		if (echoMode == -1)
			passwdEdit = new KDMPasswordEdit( parent );
		else
			passwdEdit = new KDMPasswordEdit( (KPasswordEdit::EchoModes)echoMode,
			                                  parent );
		connect( passwdEdit, TQT_SIGNAL(textChanged( const TQString & )),
		         TQT_SLOT(slotActivity()) );
		connect( passwdEdit, TQT_SIGNAL(lostFocus()), TQT_SLOT(slotActivity()) );

		if (!grid) {
			passwdEdit->adjustSize();
			pw_entry->setWidget( passwdEdit );
		} else {
			passwdLabel = new TQLabel( passwdEdit,
			                          func == Authenticate ?
			                          i18n("&Password:") :
			                          i18n("Current &password:"),
			                          parent );
			if (pred) {
				parent->setTabOrder( pred, passwdEdit );
				pred = passwdEdit;
			}
			grid->addWidget( passwdLabel, line, 0 );
			grid->addWidget( passwdEdit, line++, 1 );
		}

		if (loginEdit)
			loginEdit->setFocus();
		else
			passwdEdit->setFocus();
	}
	if (func != Authenticate) {
		if (echoMode == -1) {
			passwd1Edit = new KDMPasswordEdit( (KPasswordEdit::EchoModes)echoMode, parent );
			passwd2Edit = new KDMPasswordEdit( (KPasswordEdit::EchoModes)echoMode, parent );
		} else {
			passwd1Edit = new KDMPasswordEdit( parent );
			passwd2Edit = new KDMPasswordEdit( parent );
		}
		passwd1Label = new TQLabel( passwd1Edit, i18n("&New password:"), parent );
		passwd2Label = new TQLabel( passwd2Edit, i18n("Con&firm password:"), parent );
		if (pred) {
			parent->setTabOrder( pred, passwd1Edit );
			parent->setTabOrder( passwd1Edit, passwd2Edit );
		}
		if (grid) {
			grid->addWidget( passwd1Label, line, 0 );
			grid->addWidget( passwd1Edit, line++, 1 );
			grid->addWidget( passwd2Label, line, 0 );
			grid->addWidget( passwd2Edit, line, 1 );
		}
		if (!passwdEdit)
			passwd1Edit->setFocus();
	}
}

// virtual
KWinbindGreeter::~KWinbindGreeter()
{
	abort();
	if (!layoutItem) {
		delete loginEdit;
		delete passwdEdit;
		delete domainCombo;
		return;
	}
	TQLayoutIterator it = TQT_TQLAYOUT(layoutItem)->iterator();
	for (TQLayoutItem *itm = it.current(); itm; itm = ++it)
		delete itm->widget();
	delete layoutItem;
        delete m_domainLister;
}

void
KWinbindGreeter::slotChangedDomain( const TQString &dom )
{
	if (!loginEdit->completionObject())
		return;
	TQStringList users;
	if (dom == "<local>") {
		for (TQStringList::ConstIterator it = allUsers.begin(); it != allUsers.end(); ++it)
			if ((*it).find( separator ) < 0)
				users << *it;
	} else {
		TQString st( dom + separator );
		for (TQStringList::ConstIterator it = allUsers.begin(); it != allUsers.end(); ++it)
			if ((*it).startsWith( st ))
				users << (*it).mid( st.length() );
	}
	loginEdit->completionObject()->setItems( users );
}

void // virtual
KWinbindGreeter::loadUsers( const TQStringList &users )
{
	allUsers = users;
	KCompletion *userNamesCompletion = new KCompletion;
	loginEdit->setCompletionObject( userNamesCompletion );
	loginEdit->setAutoDeleteCompletionObject( true );
	loginEdit->setCompletionMode( KGlobalSettings::CompletionAuto );
	slotChangedDomain( defaultDomain );
}

void // virtual
KWinbindGreeter::presetEntity( const TQString &entity, int field )
{
	TQString dom, usr;
	splitEntity( entity, dom, usr );
	domainCombo->setCurrentItem( dom, true );
	slotChangedDomain( dom );
	loginEdit->setText( usr );
	if (field > 1)
		passwdEdit->setFocus();
	else if (field == 1 || field == -1) {
		if (field == -1) {
			passwdEdit->setText( "     " );
			passwdEdit->setEnabled( false );
			authTok = false;
		}
		loginEdit->setFocus();
		loginEdit->selectAll();
	}
	curUser = entity;
}

TQString // virtual
KWinbindGreeter::getEntity() const
{
	TQString dom, usr;
	if (fixedUser.isEmpty())
		dom = domainCombo->currentText(), usr = loginEdit->text();
	else
		dom = fixedDomain, usr = fixedUser;
	return dom == "<local>" ? usr : dom + separator + usr;
}

void // virtual
KWinbindGreeter::setUser( const TQString &user )
{
	// assert (fixedUser.isEmpty());
	curUser = user;
	TQString dom, usr;
	splitEntity( user, dom, usr );
	domainCombo->setCurrentItem( dom, true );
	slotChangedDomain( dom );
	loginEdit->setText( usr );
	passwdEdit->setFocus();
	passwdEdit->selectAll();
}

void // virtual
KWinbindGreeter::setPassword( const TQString &pass )
{
	passwdEdit->erase();
	passwdEdit->insert( pass );
}

void // virtual
KWinbindGreeter::setEnabled( bool enable )
{
	// assert( !passwd1Label );
	// assert( func == Authenticate && ctx == Shutdown );
//	if (domainCombo)
//		domainCombo->setEnabled( enable );
//	if (loginLabel)
//		loginLabel->setEnabled( enable );
	passwdLabel->setEnabled( enable );
	setActive( enable );
	if (enable)
		passwdEdit->setFocus();
}

void // private
KWinbindGreeter::returnData()
{
	switch (exp) {
	case 0:
		handler->gplugReturnText( getEntity().local8Bit(),
		                          KGreeterPluginHandler::IsUser );
		break;
	case 1:
		handler->gplugReturnText( passwdEdit->password(),
		                          KGreeterPluginHandler::IsPassword |
		                          KGreeterPluginHandler::IsSecret );
		break;
	case 2:
		handler->gplugReturnText( passwd1Edit->password(),
		                          KGreeterPluginHandler::IsSecret );
		break;
	default: // case 3:
		handler->gplugReturnText( passwd2Edit->password(),
		                          KGreeterPluginHandler::IsNewPassword |
		                          KGreeterPluginHandler::IsSecret );
		break;
	}
}

bool // virtual
KWinbindGreeter::textMessage( const char *text, bool err )
{
	if (!err &&
	    TQString( text ).find( TQRegExp( "^Changing password for [^ ]+$" ) ) >= 0)
		return true;
	return false;
}

void // virtual
KWinbindGreeter::textPrompt( const char *prompt, bool echo, bool nonBlocking )
{
	pExp = exp;
	if (echo)
		exp = 0;
	else if (!authTok)
		exp = 1;
	else {
		TQString pr( prompt );
		if (pr.find( TQRegExp( "\\b(old|current)\\b", false ) ) >= 0) {
			handler->gplugReturnText( "",
			                          KGreeterPluginHandler::IsOldPassword |
			                          KGreeterPluginHandler::IsSecret );
			return;
		} else if (pr.find( TQRegExp( "\\b(re-?(enter|type)|again|confirm|repeat)\\b",
		                             false ) ) >= 0)
			exp = 3;
		else if (pr.find( TQRegExp( "\\bnew\\b", false ) ) >= 0)
			exp = 2;
		else {
			handler->gplugMsgBox( TQMessageBox::Critical,
			                      i18n("Unrecognized prompt \"%1\"")
			                      .arg( prompt ) );
			handler->gplugReturnText( 0, 0 );
			exp = -1;
			return;
		}
	}

	if (pExp >= 0 && pExp >= exp) {
		revive();
		has = -1;
	}

	if (has >= exp || nonBlocking)
		returnData();
}

bool // virtual
KWinbindGreeter::binaryPrompt( const char *, bool )
{
	// this simply cannot happen ... :}
	return true;
}

void // virtual
KWinbindGreeter::start()
{
	authTok = !(passwdEdit && passwdEdit->isEnabled());
	exp = has = -1;
	running = true;
}

void // virtual
KWinbindGreeter::suspend()
{
}

void // virtual
KWinbindGreeter::resume()
{
}

void // virtual
KWinbindGreeter::next()
{
	// assert( running );
	if (domainCombo && domainCombo->hasFocus())
		loginEdit->setFocus();
	else if (loginEdit && loginEdit->hasFocus()) {
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
	else if (has >= exp)
		returnData();
}

void // virtual
KWinbindGreeter::abort()
{
	running = false;
	if (exp >= 0) {
		exp = -1;
		handler->gplugReturnText( 0, 0 );
	}
}

void // virtual
KWinbindGreeter::succeeded()
{
	// assert( running || timed_login );
	if (!authTok) {
		setActive( false );
		if (passwd1Edit) {
			authTok = true;
			return;
		}
	} else
		setActive2( false );
	exp = -1;
	running = false;
}

void // virtual
KWinbindGreeter::failed()
{
	// assert( running || timed_login );
	setActive( false );
	setActive2( false );
	exp = -1;
	running = false;
}

void // virtual
KWinbindGreeter::revive()
{
	// assert( !running );
	setActive2( true );
	if (authTok) {
		passwd1Edit->erase();
		passwd2Edit->erase();
		passwd1Edit->setFocus();
	} else {
		passwdEdit->erase();
		if (loginEdit && loginEdit->isEnabled())
			passwdEdit->setEnabled( true );
		else {
			setActive( true );
			if (loginEdit && loginEdit->text().isEmpty())
				loginEdit->setFocus();
			else
				passwdEdit->setFocus();
		}
	}
}

void // virtual
KWinbindGreeter::clear()
{
	// assert( !running && !passwd1Edit );
	passwdEdit->erase();
	if (loginEdit) {
		domainCombo->setCurrentItem( defaultDomain );
		slotChangedDomain( defaultDomain );
		loginEdit->clear();
		loginEdit->setFocus();
		curUser = TQString::null;
	} else
		passwdEdit->setFocus();
}


// private

void
KWinbindGreeter::setActive( bool enable )
{
	if (domainCombo)
		domainCombo->setEnabled( enable );
	if (loginEdit)
		loginEdit->setEnabled( enable );
	if (passwdEdit)
		passwdEdit->setEnabled( enable );
}

void
KWinbindGreeter::setActive2( bool enable )
{
	if (passwd1Edit) {
		passwd1Edit->setEnabled( enable );
		passwd2Edit->setEnabled( enable );
	}
}

void
KWinbindGreeter::slotLoginLostFocus()
{
	if (!running)
		return;
	TQString ent( getEntity() );
	if (exp > 0) {
		if (curUser == ent)
			return;
		exp = -1;
		handler->gplugReturnText( 0, 0 );
	}
	curUser = ent;
	handler->gplugSetUser( curUser );
}

void
KWinbindGreeter::slotActivity()
{
	if (running)
		handler->gplugActivity();
}

void
KWinbindGreeter::slotStartDomainList()
{
    mDomainListTimer.stop();
    mDomainListing.clear();

    m_domainLister = new KProcIO;
    connect(m_domainLister, TQT_SIGNAL(readReady(KProcIO*)), TQT_SLOT(slotReadDomainList()));
    connect(m_domainLister, TQT_SIGNAL(processExited(KProcess*)), TQT_SLOT(slotEndDomainList()));

    (*m_domainLister) << "wbinfo" << "--own-domain" << "--trusted-domains";
    m_domainLister->setComm (KProcess::Stdout);
    m_domainLister->start();
}

void
KWinbindGreeter::slotReadDomainList()
{
    TQString line;

    while ( m_domainLister->readln( line ) != -1 ) {
        mDomainListing.append(line);
    }
}

void
KWinbindGreeter::slotEndDomainList()
{
    delete m_domainLister;
    m_domainLister = 0;

    TQStringList domainList;
    domainList = staticDomains;

    for (TQStringList::const_iterator it = mDomainListing.begin();
         it != mDomainListing.end(); ++it) {

        if (!domainList.contains(*it))
            domainList.append(*it);
    }

    TQString current = domainCombo->currentText();

    for (int i = 0; i < domainList.count(); ++i) {
        if (i < domainCombo->count())
            domainCombo->changeItem(domainList[i], i);
        else
            domainCombo->insertItem(domainList[i], i);
    }

    while (domainCombo->count() > domainList.count())
        domainCombo->removeItem(domainCombo->count()-1);

    domainCombo->setCurrentItem( current );

    if (domainCombo->currentText() != current)
        domainCombo->setCurrentItem( defaultDomain );

    mDomainListTimer.start(5 * 1000);
}

// factory

static bool init( const TQString &,
                  TQVariant (*getConf)( void *, const char *, const TQVariant & ),
                  void *ctx )
{
	echoMode = getConf( ctx, "EchoMode", TQVariant( -1 ) ).toInt();
	staticDomains = TQStringList::split( ':', getConf( ctx, "winbind.Domains", TQVariant( "" ) ).toString() );
	if (!staticDomains.contains("<local>"))
		staticDomains << "<local>";

	defaultDomain = getConf( ctx, "winbind.DefaultDomain", TQVariant( staticDomains.first() ) ).toString();
	TQString sepstr = getConf( ctx, "winbind.Separator", TQVariant( TQString::null ) ).toString();
	if (sepstr.isNull()) {
		FILE *sepfile = popen( "wbinfo --separator 2>/dev/null", "r" );
		if (sepfile) {
			TQTextIStream( sepfile ) >> sepstr;
			if (pclose( sepfile ))
				sepstr = "\\";
		} else
			sepstr = "\\";
	}
	separator = sepstr[0].latin1();
	KGlobal::locale()->insertCatalogue( "kgreet_winbind" );
	return true;
}

static void done( void )
{
	KGlobal::locale()->removeCatalogue( "kgreet_winbind" );
	// avoid static deletion problems ... hopefully
	staticDomains.clear();
	defaultDomain = TQString::null;
}

static KGreeterPlugin *
create( KGreeterPluginHandler *handler, KdmThemer *themer,
        TQWidget *parent, TQWidget *predecessor,
        const TQString &fixedEntity,
        KGreeterPlugin::Function func,
        KGreeterPlugin::Context ctx )
{
	return new KWinbindGreeter( handler, themer, parent, predecessor, fixedEntity, func, ctx );
}

KDE_EXPORT kgreeterplugin_info kgreeterplugin_info = {
	I18N_NOOP("Winbind / Samba"), "classic",
	kgreeterplugin_info::Local | kgreeterplugin_info::Fielded | kgreeterplugin_info::Presettable,
	init, done, create
};

#include "kgreet_winbind.moc"
