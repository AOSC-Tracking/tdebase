/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <config.h>

#include <tqlayout.h>

#include <tdeaboutdata.h>
#include <kgenericfactory.h>
#include <kimageio.h>
#include <tdemessagebox.h>
#include <kurldrag.h>

#include "tdm-appear.h"
#include "tdm-font.h"
#include "tdm-users.h"
#include "tdm-shut.h"
#include "tdm-conv.h"

#include "main.h"
#include "background.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <pwd.h>
#include <grp.h>

typedef KGenericFactory<TDModule, TQWidget> TDMFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_tdm, TDMFactory("tdmconfig") )

KURL *decodeImgDrop(TQDropEvent *e, TQWidget *wdg)
{
    KURL::List uris;

    if (KURLDrag::decode(e, uris) && (uris.count() > 0)) {
	KURL *url = new KURL(uris.first());

	KImageIO::registerFormats();
	if( KImageIO::canRead(KImageIO::type(url->fileName())) )
	    return url;

	TQStringList qs = TQStringList::split('\n', KImageIO::pattern());
	qs.remove(qs.begin());

	TQString msg = i18n( "%1 "
			    "does not appear to be an image file.\n"
			    "Please use files with these extensions:\n"
			    "%2")
			    .arg(url->fileName())
			    .arg(qs.join("\n"));
	KMessageBox::sorry( wdg, msg);
	delete url;
    }
    return 0;
}

KSimpleConfig *config;

TDModule::TDModule(TQWidget *parent, const char *name, const TQStringList &)
  : TDECModule(TDMFactory::instance(), parent, name)
  , minshowuid(0)
  , maxshowuid(0)
  , updateOK(false)
{
  TDEAboutData *about =
    new TDEAboutData(I18N_NOOP("kcmtdm"), I18N_NOOP("TDE Login Manager Config Module"),
                0, 0, TDEAboutData::License_GPL,
                I18N_NOOP("(c) 1996 - 2005 The TDM Authors"));

  about->addAuthor("Thomas Tanghus", I18N_NOOP("Original author"), "tanghus@earthling.net");
	about->addAuthor("Steffen Hansen", 0, "hansen@kde.org");
	about->addAuthor("Oswald Buddenhagen", I18N_NOOP("Current maintainer"), "ossi@kde.org");

  setQuickHelp( i18n(    "<h1>Login Manager</h1> In this module you can configure the "
                    "various aspects of the TDE Login Manager. This includes "
                    "the look and feel as well as the users that can be "
                    "selected for login. Note that you can only make changes "
                    "if you run the module with superuser rights. If you have not started the TDE "
                    "Control Center with superuser rights (which is absolutely the right thing to "
                    "do, by the way), click on the <em>Modify</em> button to acquire "
                    "superuser rights. You will be asked for the superuser password."
                    "<h2>Appearance</h2> On this tab page, you can configure how "
                    "the Login Manager should look, which language it should use, and which "
                    "GUI style it should use. The language settings made here have no influence on "
                    "the user's language settings."
                    "<h2>Font</h2>Here you can choose the fonts that the Login Manager should use "
                    "for various purposes like greetings and user names. "
                    "<h2>Background</h2>If you want to set a special background for the login "
                    "screen, this is where to do it."
                    "<h2>Shutdown</h2> Here you can specify who is allowed to shutdown/reboot the machine "
                    "and whether a boot manager should be used."
                    "<h2>Users</h2>On this tab page, you can select which users the Login Manager "
                    "will offer you for logging in."
                    "<h2>Convenience</h2> Here you can specify a user to be logged in automatically, "
		    "users not needing to provide a password to log in, and other convenience features.<br>"
		    "Note, that these settings are security holes by their nature, so use them very carefully."));

  setAboutData( about );

  setlocale( LC_COLLATE, "C" );

  TDEGlobal::locale()->insertCatalogue("kcmbackground");

  TQStringList sl;
  TQMap<gid_t,TQStringList> tgmap;
  TQMap<gid_t,TQStringList>::Iterator tgmapi;
  TQMap<gid_t,TQStringList>::ConstIterator tgmapci;
  TQMap<TQString, TQPair<int,TQStringList> >::Iterator umapi;

  struct passwd *ps;
  for (setpwent(); (ps = getpwent()); ) {
    TQString un( TQFile::decodeName( ps->pw_name ) );
    if (usermap.find( un ) == usermap.end()) {
      usermap.insert( un, TQPair<int,TQStringList>( ps->pw_uid, sl ) );
      if ((tgmapi = tgmap.find( ps->pw_gid )) != tgmap.end())
        (*tgmapi).append( un );
      else
	tgmap[ps->pw_gid] = un;
    }
  }
  endpwent();

  struct group *grp;
  for (setgrent(); (grp = getgrent()); ) {
    TQString gn( TQFile::decodeName( grp->gr_name ) );
    bool delme = false;
    if ((tgmapi = tgmap.find( grp->gr_gid )) != tgmap.end()) {
      if ((*tgmapi).count() == 1 && (*tgmapi).first() == gn)
        delme = true;
      else
        for (TQStringList::ConstIterator it = (*tgmapi).begin();
             it != (*tgmapi).end(); ++it)
          usermap[*it].second.append( gn );
      tgmap.remove( tgmapi );
    }
    if (!*grp->gr_mem ||
        (delme && !grp->gr_mem[1] && gn == TQFile::decodeName( *grp->gr_mem )))
      continue;
    do {
      TQString un( TQFile::decodeName( *grp->gr_mem ) );
      if ((umapi = usermap.find( un )) != usermap.end()) {
        if ((*umapi).second.find( gn ) == (*umapi).second.end())
	  (*umapi).second.append( gn );
      } else
        kdWarning() << "group '" << gn << "' contains unknown user '" << un << "'" << endl;
    } while (*++grp->gr_mem);
  }
  endgrent();

  for (tgmapci = tgmap.begin(); tgmapci != tgmap.end(); ++tgmapci)
    kdWarning() << "user(s) '" << tgmapci.data().join(",")
	<< "' have unknown GID " << tgmapci.key() << endl;

  struct stat st;
  if( stat( KDE_CONFDIR "/tdm/tdmdistrc" ,&st ) == 0) {
    config = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdm/tdmdistrc" ));
  }
  else {
    config = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdm/tdmrc" ));
  }

  TQVBoxLayout *top = new TQVBoxLayout(this);
  tab = new TQTabWidget(this);

  // *****
  // _don't_ add a theme configurator until the theming engine is _really_ done!!
  // *****

  appearance = new TDMAppearanceWidget(this);
  tab->addTab(appearance, i18n("A&ppearance"));
  connect(appearance, TQ_SIGNAL(changed(bool)), TQ_SIGNAL( changed(bool)));

  font = new TDMFontWidget(this);
  tab->addTab(font, i18n("&Font"));
  connect(font, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));

  background = new KBackground(this);
  tab->addTab(background, i18n("&Background"));
  connect(background, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));

  sessions = new TDMSessionsWidget(this);
  tab->addTab(sessions, i18n("&Shutdown"));
  connect(sessions, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));

  users = new TDMUsersWidget(this, 0);
  tab->addTab(users, i18n("&Users"));
  connect(users, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));
  connect(users, TQ_SIGNAL(setMinMaxUID(int,int)), TQ_SLOT(slotMinMaxUID(int,int)));
  connect(this, TQ_SIGNAL(addUsers(const TQMap<TQString,int> &)), users, TQ_SLOT(slotAddUsers(const TQMap<TQString,int> &)));
  connect(this, TQ_SIGNAL(delUsers(const TQMap<TQString,int> &)), users, TQ_SLOT(slotDelUsers(const TQMap<TQString,int> &)));
  connect(this, TQ_SIGNAL(clearUsers()), users, TQ_SLOT(slotClearUsers()));

  convenience = new TDMConvenienceWidget(this, 0);
  tab->addTab(convenience, i18n("Con&venience"));
  connect(convenience, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));
  connect(this, TQ_SIGNAL(addUsers(const TQMap<TQString,int> &)), convenience, TQ_SLOT(slotAddUsers(const TQMap<TQString,int> &)));
  connect(this, TQ_SIGNAL(delUsers(const TQMap<TQString,int> &)), convenience, TQ_SLOT(slotDelUsers(const TQMap<TQString,int> &)));
  connect(this, TQ_SIGNAL(clearUsers()), convenience, TQ_SLOT(slotClearUsers()));

  load();
  if (getuid() != 0 || !config->checkConfigFilesWritable( true )) {
    appearance->makeReadOnly();
    font->makeReadOnly();
    background->makeReadOnly();
    users->makeReadOnly();
    sessions->makeReadOnly();
    convenience->makeReadOnly();
  }
  top->addWidget(tab);
}

TDModule::~TDModule()
{
  delete config;
}

void TDModule::load()
{
  appearance->load();
  font->load();
  background->load();
  users->load();
  sessions->load();
  convenience->load();
  propagateUsers();
}


void TDModule::save()
{
  appearance->save();
  font->save();
  background->save();
  users->save();
  sessions->save();
  convenience->save();
  config->sync();
}


void TDModule::defaults()
{
    if ( getuid() == 0 )
    {
        appearance->defaults();
        font->defaults();
        background->defaults();
        users->defaults();
        sessions->defaults();
        convenience->defaults();
        propagateUsers();
    }
}

TQString TDModule::handbookSection() const
{
 	int index = tab->currentPageIndex();
 	if (index == 0)
		return "tdmconfig-appearance";
	else if (index == 1)
		return "tdmconfig-font";
	else if (index == 2)
		return "tdmconfig-background";
	else if (index == 3)
		return "tdmconfig-shutdown";
	else if (index == 4)
		return "tdmconfig-users";
	else if (index == 5)
		return "tdmconfig-convenience";
 	else
 		return TQString::null;
}

void TDModule::propagateUsers()
{
  groupmap.clear();
  emit clearUsers();
  TQMap<TQString,int> lusers;
  TQMapConstIterator<TQString, TQPair<int,TQStringList> > it;
  TQStringList::ConstIterator jt;
  TQMap<TQString,int>::Iterator gmapi;
  for (it = usermap.begin(); it != usermap.end(); ++it) {
    int uid = it.data().first;
    if (!uid || (uid >= minshowuid && uid <= maxshowuid)) {
      lusers[it.key()] = uid;
      for (jt = it.data().second.begin(); jt != it.data().second.end(); ++jt)
	if ((gmapi = groupmap.find( *jt )) == groupmap.end()) {
	  groupmap[*jt] = 1;
	  lusers['@' + *jt] = -uid;
	} else
	  (*gmapi)++;
    }
  }
  emit addUsers(lusers);
  updateOK = true;
}

void TDModule::slotMinMaxUID(int min, int max)
{
  if (updateOK) {
    TQMap<TQString,int> alusers, dlusers;
    TQMapConstIterator<TQString, TQPair<int,TQStringList> > it;
    TQStringList::ConstIterator jt;
    TQMap<TQString,int>::Iterator gmapi;
    for (it = usermap.begin(); it != usermap.end(); ++it) {
      int uid = it.data().first;
      if (!uid) continue;
      if ((uid >= minshowuid && uid <= maxshowuid) &&
	  !(uid >= min && uid <= max)) {
        dlusers[it.key()] = uid;
	for (jt = it.data().second.begin();
	     jt != it.data().second.end(); ++jt) {
	  gmapi = groupmap.find( *jt );
	  if (!--(*gmapi)) {
	    groupmap.remove( gmapi );
	    dlusers['@' + *jt] = -uid;
	  }
	}
      } else
      if ((uid >= min && uid <= max) &&
	  !(uid >= minshowuid && uid <= maxshowuid)) {
        alusers[it.key()] = uid;
	for (jt = it.data().second.begin();
	     jt != it.data().second.end(); ++jt)
	  if ((gmapi = groupmap.find( *jt )) == groupmap.end()) {
	    groupmap[*jt] = 1;
	    alusers['@' + *jt] = -uid;
	  } else
	    (*gmapi)++;
      }
    }
    emit delUsers(dlusers);
    emit addUsers(alusers);
  }
  minshowuid = min;
  maxshowuid = max;
}

#include "main.moc"
