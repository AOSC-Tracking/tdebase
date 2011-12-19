/* This file is part of the KDE project
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dockviewbase.h"
#include "dockviewbase.moc"

#include <tqlabel.h>
#include <tqlayout.h>

//#include <kdebug.h>

namespace Kate {

// data storage
class DockViewBasePrivate {
  public:
  TQWidget *header;
  TQLabel *lTitle;
  TQLabel *lPrefix;
};

}

Kate::DockViewBase::DockViewBase( TQWidget* parent, const char* name )
  : TQVBox( parent, name ),
    d ( new Kate::DockViewBasePrivate )
{
  init( TQString::null, TQString::null );
}

Kate::DockViewBase::DockViewBase( const TQString &prefix, const TQString &title, TQWidget* parent, const char* name )
  : TQVBox( parent, name ),
    d ( new Kate::DockViewBasePrivate )
{
  init( prefix, title );
}

Kate::DockViewBase::~DockViewBase()
{
  delete d;
}

void Kate::DockViewBase::setTitlePrefix( const TQString &prefix )
{
    d->lPrefix->setText( prefix );
    d->lPrefix->show();
}

TQString Kate::DockViewBase::titlePrefix() const
{
  return d->lPrefix->text();
}

void Kate::DockViewBase::setTitle( const TQString &title )
{
  d->lTitle->setText( title );
  d->lTitle->show();
}

TQString Kate::DockViewBase::title() const
{
  return d->lTitle->text();
}

void Kate::DockViewBase::setTitle( const TQString &prefix, const TQString &title )
{
  setTitlePrefix( prefix );
  setTitle( title );
}

void Kate::DockViewBase::init( const TQString &prefix, const TQString &title )
{
  setSpacing( 4 );
  d->header = new TQWidget( this );
  d->header->setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Fixed, true ) );
  TQHBoxLayout *lo = new TQHBoxLayout( d->header );
  lo->setSpacing( 6 );
  lo->insertSpacing( 0, 6 ); 
  d->lPrefix = new TQLabel( title, d->header );
  lo->addWidget( d->lPrefix );
  d->lTitle = new TQLabel( title, d->header );
  lo->addWidget( d->lTitle );
  lo->setStretchFactor( d->lTitle, 1 );
  lo->insertSpacing( -1, 6 );
  if ( prefix.isEmpty() ) d->lPrefix->hide();
  if ( title.isEmpty() ) d->lTitle->hide();
}
