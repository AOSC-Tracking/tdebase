/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "klistdebugdialog.h"
#include <kconfig.h>
#include <kapplication.h>
#include <kdebug.h>
#include <tqlayout.h>
#include <tqscrollview.h>
#include <tqvbox.h>
#include <klocale.h>
#include <tqpushbutton.h>
#include <klineedit.h>
#include <dcopclient.h>

KListDebugDialog::KListDebugDialog( TQStringList areaList, TQWidget *parent, const char *name, bool modal )
  : KAbstractDebugDialog( parent, name, modal ),
  m_areaList( areaList )
{
  setCaption(i18n("Debug Settings"));

  TQVBoxLayout *lay = new TQVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );

  m_incrSearch = new KLineEdit( this );
  lay->addWidget( m_incrSearch );
  connect( m_incrSearch, TQT_SIGNAL( textChanged( const TQString& ) ),
           TQT_SLOT( generateCheckBoxes( const TQString& ) ) );

  TQScrollView * scrollView = new TQScrollView( this );
  scrollView->setResizePolicy( TQScrollView::AutoOneFit );
  lay->addWidget( scrollView );

  m_box = new TQVBox( scrollView->viewport() );
  scrollView->addChild( m_box );

  generateCheckBoxes( TQString::null );

  TQHBoxLayout* selectButs = new TQHBoxLayout( lay );
  TQPushButton* all = new TQPushButton( i18n("&Select All"), this );
  TQPushButton* none = new TQPushButton( i18n("&Deselect All"), this );
  selectButs->addWidget( all );
  selectButs->addWidget( none );

  connect( all, TQT_SIGNAL( clicked() ), this, TQT_SLOT( selectAll() ) );
  connect( none, TQT_SIGNAL( clicked() ), this, TQT_SLOT( deSelectAll() ) );

  buildButtons( lay );
  resize( 350, 400 );
}

void KListDebugDialog::generateCheckBoxes( const TQString& filter )
{
  TQPtrListIterator<TQCheckBox> cb_it ( boxes );
  for( ; cb_it.current() ; ++cb_it )
  {
    if( (*cb_it)->state() != TQButton::NoChange )
      m_changes.insert( (*cb_it)->name(), (*cb_it)->isChecked() ? 2 : 4 );
  }

  boxes.setAutoDelete( true );
  boxes.clear();
  boxes.setAutoDelete( false );

  TQWidget* taborder = m_incrSearch;
  TQStringList::Iterator it = m_areaList.begin();
  for ( ; it != m_areaList.end() ; ++it )
  {
    TQString data = (*it).simplifyWhiteSpace();
    if ( filter.isEmpty() || data.lower().contains( filter.lower() ) )
    {
      int space = data.find(" ");
      if (space == -1)
        kdError() << "No space:" << data << endl;

      TQString areaNumber = data.left(space);
      //kdDebug() << areaNumber << endl;
      TQCheckBox * cb = new TQCheckBox( data, m_box, areaNumber.latin1() );
      cb->show();
      boxes.append( cb );
      setTabOrder( taborder, cb );
      taborder = cb;
    }
  }

  load();
}

void KListDebugDialog::selectAll()
{
  TQPtrListIterator<TQCheckBox> it ( boxes );
  for ( ; it.current() ; ++it ) {
    (*it)->setChecked( true );
    m_changes.insert( (*it)->name(), 2 );
  }
}

void KListDebugDialog::deSelectAll()
{
  TQPtrListIterator<TQCheckBox> it ( boxes );
  for ( ; it.current() ; ++it ) {
    (*it)->setChecked( false );
    m_changes.insert( (*it)->name(), 4 );
  }
}

void KListDebugDialog::load()
{
  TQPtrListIterator<TQCheckBox> it ( boxes );
  for ( ; it.current() ; ++it )
  {
      pConfig->setGroup( (*it)->name() ); // Group name = debug area code = cb's name

      int setting = pConfig->readNumEntry( "InfoOutput", 2 );
      // override setting if in m_changes
      if( m_changes.find( (*it)->name() ) != m_changes.end() ) {
        setting = m_changes[ (*it)->name() ];
      }

      switch (setting) {
        case 4: // off
          (*it)->setChecked(false);
          break;
        case 2: //shell
          (*it)->setChecked(true);
          break;
        case 3: //syslog
        case 1: //msgbox
        case 0: //file
        default:
          (*it)->setNoChange();
          /////// Uses the triState capability of checkboxes
          ////// Note: it seems some styles don't draw that correctly (BUG)
          break;
      }
  }
}

void KListDebugDialog::save()
{
  TQPtrListIterator<TQCheckBox> it ( boxes );
  for ( ; it.current() ; ++it )
  {
      pConfig->setGroup( (*it)->name() ); // Group name = debug area code = cb's name
      if ( (*it)->state() != TQButton::NoChange )
      {
          int setting = (*it)->isChecked() ? 2 : 4;
          pConfig->writeEntry( "InfoOutput", setting );
      }
  }
  //sync done by main.cpp

  // send DCOP message to all clients
  TQByteArray data;
  if (!kapp->dcopClient()->send("*", "KDebug", "notifyKDebugConfigChanged()", data))
  {
    kdError() << "Unable to send DCOP message" << endl;
  }

  m_changes.clear();
}

void KListDebugDialog::activateArea( TQCString area, bool activate )
{
  TQPtrListIterator<TQCheckBox> it ( boxes );
  for ( ; it.current() ; ++it )
  {
      if ( area == (*it)->name()  // debug area code = cb's name
          || (*it)->text().find( TQString::fromLatin1(area) ) != -1 ) // area name included in cb text
      {
          (*it)->setChecked( activate );
          return;
      }
  }
}

#include "klistdebugdialog.moc"
