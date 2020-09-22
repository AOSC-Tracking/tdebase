/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2005, Anders Lund <anders@alweb.dk>
 */

#include "katesessionmenu.h"
#include "katesessionmenu.moc"

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klibloader.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

#include <tqvalidator.h>

class Validator : public TQValidator {
  public:
    Validator( TQObject *parent ) : TQValidator( parent, 0 ) {}
    ~Validator() {}
    virtual TQValidator::State validate( TQString &, int & ) const { return TQValidator::Acceptable; }
};

K_EXPORT_KICKER_MENUEXT(kate, KateSessionMenu)

KateSessionMenu::KateSessionMenu( TQWidget *parent, const char *name, const TQStringList& )
  : KPanelMenu( parent, name ),
    m_parent( parent )
{
}

KateSessionMenu::~KateSessionMenu()
{
}

// update the session list and rebuild the menu
void KateSessionMenu::initialize()
{
  if ( initialized() )
  {
    return;
  }

  m_sessions.clear();

  int id = 0;

  // no session - exec 'kate'
  insertItem( SmallIconSet("kate"), i18n("Start Kate (no arguments)"), id++ );

  // new session - prompt for a name and  exec 'kate --start NAME'
  insertItem( SmallIconSet("new"), i18n("New Kate Session"), id++ );

  // new anonymous session, 'kate --start ""'
  insertItem( SmallIconSet("new"), i18n("New Anonymous Session"), id++ );

  insertSeparator();

  TQStringList list = TDEGlobal::dirs()->findAllResources( "data", "kate/sessions/*.katesession", false, true);
  for (TQStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
  {
    KSimpleConfig config( *it, true );
    config.setGroup( "General" );
    m_sessions.append( config.readEntry( "Name" ) );
  }

  m_sessions.sort();

  for ( TQStringList::ConstIterator it1 = m_sessions.begin(); it1 != m_sessions.end(); ++it1 )
  {
    insertItem( *it1, id++ );
  }

  // means for updating, to let the user manually update if he/she added new sessions.
  insertSeparator();
  insertItem( SmallIconSet("reload"), i18n("Reload Session List"), this, TQT_SLOT(reinitialize()) );
}

void KateSessionMenu::slotExec( int id )
{
  if ( id < 0 )
    return;

  TQStringList args;
  if ( id > 0 )
    args << "--start";

  // If a new session is requested we try to ask for a name.
  if ( id == 1 )
  {
    bool ok (false);
    TQString name = KInputDialog::getText( i18n("Session Name"),
                                          i18n("Please enter a name for the new session"),
                                          TQString::null,
                                          &ok, 0, 0, new Validator( TQT_TQOBJECT(m_parent) ) );
    if ( ! ok )
      return;

    if ( name.isEmpty() && KMessageBox::questionYesNo( 0,
                          i18n("An unnamed session will not be saved automatically. "
                               "Do you want to create such a session?"),
                          i18n("Create anonymous session?"),
                          KStdGuiItem::yes(), KStdGuiItem::cancel(),
                          "kate_session_button_create_anonymous" ) == KMessageBox::No )
      return;

    if ( m_sessions.contains( name ) &&
         KMessageBox::warningYesNo( 0,
                                    i18n("You allready have a session named %1. Do you want to open that session?").arg( name ),
                                    i18n("Session exists") ) == KMessageBox::No )
      return;
    else
      // mark the menu as dirty so that it gets rebuild at next display
      // to show the new session
      setInitialized( false );

    args << name;
  }

  else if ( id == 2 )
    args << "";

  else if ( id > 2 )
    args << m_sessions[ id-3 ];

  kapp->tdeinitExec("kate", args);
}
