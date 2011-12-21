/**
 * Copyright (c) 2001 Dawit Alemayehu <adawit@kde.org>
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

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqwhatsthis.h>
#include <tqpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kurllabel.h>

#include "fakeuaprovider.h"
#include "uagentproviderdlg.h"
#include "uagentproviderdlg_ui.h"

UALineEdit::UALineEdit( TQWidget *parent, const char *name )
           :KLineEdit( parent, name )
{
  // For now do not accept any drops since they might contain
  // characters we do not accept.
  // TODO: Re-implement ::dropEvent to allow acceptable formats...
  setAcceptDrops( false );
}

void UALineEdit::keyPressEvent( TQKeyEvent* e )
{
  int key = e->key();
  TQString keycode = e->text();
  if ( (key >= Qt::Key_Escape && key <= Qt::Key_Help) || key == Qt::Key_Period ||
       (cursorPosition() > 0 && key == Qt::Key_Minus) ||
       (!keycode.isEmpty() && keycode.unicode()->isLetterOrNumber()) )
  {
    KLineEdit::keyPressEvent(e);
    return;
  }
  e->accept();
}

UAProviderDlg::UAProviderDlg( const TQString& caption, TQWidget *parent,
                              FakeUASProvider* provider, const char *name )
              :KDialog(parent, name, true), m_provider(provider)
{
  setCaption ( caption );

  TQVBoxLayout* mainLayout = new TQVBoxLayout(this, 0, 0);

  dlg = new UAProviderDlgUI (this);
  mainLayout->addWidget(dlg);
  //dlg->leIdentity->setEnableSqueezedText( true );

  if (!m_provider)
  {
    setEnabled( false );
    return;
  }

  init();
}

UAProviderDlg::~UAProviderDlg()
{
}

void UAProviderDlg::init()
{
  connect( dlg->pbOk, TQT_SIGNAL(clicked()), TQT_SLOT(accept()) );
  connect( dlg->pbCancel, TQT_SIGNAL(clicked()), TQT_SLOT(reject()) );

  connect( dlg->leSite, TQT_SIGNAL(textChanged(const TQString&)),
                TQT_SLOT(slotTextChanged( const TQString&)) );

  connect( dlg->cbAlias, TQT_SIGNAL(activated(const TQString&)),
                TQT_SLOT(slotActivated(const TQString&)) );

  dlg->cbAlias->clear();
  dlg->cbAlias->insertStringList( m_provider->userAgentAliasList() );
  dlg->cbAlias->insertItem( "", 0 );
  dlg->cbAlias->listBox()->sort();

  dlg->leSite->setFocus();
}

void UAProviderDlg::slotActivated( const TQString& text )
{
  if ( text.isEmpty() )
    dlg->leIdentity->setText( "" );
  else
    dlg->leIdentity->setText( m_provider->agentStr(text) );

  dlg->pbOk->setEnabled( (!dlg->leSite->text().isEmpty() && !text.isEmpty()) );
}

void UAProviderDlg::slotTextChanged( const TQString& text )
{
  dlg->pbOk->setEnabled( (!text.isEmpty() && !dlg->cbAlias->currentText().isEmpty()) );
}

void UAProviderDlg::setSiteName( const TQString& text )
{
  dlg->leSite->setText( text );
}

void UAProviderDlg::setIdentity( const TQString& text )
{
  int id = dlg->cbAlias->listBox()->index( dlg->cbAlias->listBox()->findItem(text) );
  dlg->cbAlias->setCurrentItem( id );
  slotActivated( dlg->cbAlias->currentText() );
  if ( !dlg->leSite->isEnabled() )
    dlg->cbAlias->setFocus();
}

TQString UAProviderDlg::siteName()
{
  TQString site_name=dlg->leSite->text().lower();
  site_name = site_name.remove( "https://" );
  site_name = site_name.remove( "http://" );
  return site_name;
}

TQString UAProviderDlg::identity()
{
  return dlg->cbAlias->currentText();
}

TQString UAProviderDlg::alias()
{
  return dlg->leIdentity->text();
}

#include "uagentproviderdlg.moc"
