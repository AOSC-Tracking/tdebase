/*  Copyright (C) 2003 Lukas Tinkl <lukas@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "knewthemedlg.h"
#include "newthemewidget.h"

#include <tqlineedit.h>
#include <tqtextedit.h>

#include <tdelocale.h>

KNewThemeDlg::KNewThemeDlg( TQWidget * parent, const char * name )
    : KDialogBase(parent, name, true, i18n("New Theme"), Ok|Cancel, Ok)
{
    m_base = new NewThemeWidget( this, "new_theme_base" );
    setMainWidget( m_base );
    connect( m_base->leName, TQ_SIGNAL( textChanged ( const TQString & ) ), this, TQ_SLOT( slotThemeNameChanged( const TQString & ) ) );
    slotThemeNameChanged( m_base->leName->text() );
}

KNewThemeDlg::~KNewThemeDlg()
{
    delete m_base;
}

void KNewThemeDlg::slotThemeNameChanged( const TQString &_text )
{
     enableButtonOK( !_text.isEmpty() );
}

TQString KNewThemeDlg::getName() const
{
    return m_base->leName->text();
}

TQString KNewThemeDlg::getAuthor() const
{
    return m_base->leAuthor->text();
}

TQString KNewThemeDlg::getEmail() const
{
    return m_base->leEmail->text();
}

TQString KNewThemeDlg::getHomepage() const
{
    return m_base->leHomepage->text();
}

TQString KNewThemeDlg::getComment() const
{
    return m_base->teComment->text();
}

TQString KNewThemeDlg::getVersion() const
{
    return m_base->leVersion->text();
}

void KNewThemeDlg::setAuthor( const TQString & author )
{
    m_base->leAuthor->setText( author );
}

void KNewThemeDlg::setEmail( const TQString & email )
{
    m_base->leEmail->setText( email );
}

void KNewThemeDlg::setVersion( const TQString & version )
{
    m_base->leVersion->setText( version );
}

void KNewThemeDlg::setName( const TQString & name )
{
    m_base->leName->setText( name );
}

#include "knewthemedlg.moc"
