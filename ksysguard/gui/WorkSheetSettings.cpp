/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999 - 2002 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
    Please do not commit any changes without consulting me first. Thanks!

*/

#include <tdeaccelmanager.h>
#include <klineedit.h>
#include <knuminput.h>
#include <tdelocale.h>

#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqspinbox.h>
#include <tqlayout.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>

#include "WorkSheetSettings.h"

WorkSheetSettings::WorkSheetSettings( TQWidget* parent, const char* name )
  : KDialogBase( parent, name, true, TQString::null, Ok|Cancel, Ok, true )
{
  setCaption( i18n( "Worksheet Properties" ) );

  TQWidget *page = new TQWidget( this );
  setMainWidget( page );

  TQVBoxLayout *topLayout = new TQVBoxLayout( page, 0, spacingHint() );

  TQGroupBox *group = new TQGroupBox( 0, Qt::Vertical, i18n( "Title" ), page );
  group->layout()->setMargin( marginHint() );
  group->layout()->setSpacing( spacingHint() );

  TQGridLayout *groupLayout = new TQGridLayout( group->layout(), 1, 1 );
  groupLayout->setAlignment( Qt::AlignTop );

  mSheetTitle = new KLineEdit( group );
  groupLayout->addWidget( mSheetTitle, 0, 0 );

  topLayout->addWidget( group );

  group = new TQGroupBox( 0, Qt::Vertical, i18n( "Properties" ), page );
  group->layout()->setMargin( marginHint() );
  group->layout()->setSpacing( spacingHint() );

  groupLayout = new TQGridLayout( group->layout(), 3, 2 );
  groupLayout->setAlignment( Qt::AlignTop );

  TQLabel *label = new TQLabel( i18n( "Rows:" ), group );
  groupLayout->addWidget( label, 0, 0 );

  mRows = new KIntNumInput( 1, group );
  mRows->setMaxValue( 42 );
  mRows->setMinValue( 1 );
  groupLayout->addWidget( mRows, 0, 1 );
  label->setBuddy( mRows );

  label = new TQLabel( i18n( "Columns:" ), group );
  groupLayout->addWidget( label, 1, 0 );

  mColumns = new KIntNumInput( 1, group );
  mColumns->setMaxValue( 42 );
  mColumns->setMinValue( 1 );
  groupLayout->addWidget( mColumns, 1, 1 );
  label->setBuddy( mColumns );

  label = new TQLabel( i18n( "Update interval:" ), group );
  groupLayout->addWidget( label, 2, 0 );

  mInterval = new KIntNumInput( 2, group );
  mInterval->setMaxValue( 300 );
  mInterval->setMinValue( 1 );
  mInterval->setSuffix( i18n( " sec" ) );
  groupLayout->addWidget( mInterval, 2, 1 );
  label->setBuddy( mInterval );

  topLayout->addWidget( group );

  TQWhatsThis::add( mRows, i18n( "Enter the number of rows the sheet should have." ) );
  TQWhatsThis::add( mColumns, i18n( "Enter the number of columns the sheet should have." ) );
  TQWhatsThis::add( mInterval, i18n( "All displays of the sheet are updated at the rate specified here." ) );
  TQToolTip::add( mSheetTitle, i18n( "Enter the title of the worksheet here." ) );

  TDEAcceleratorManager::manage( page );

  mSheetTitle->setFocus();

  resize( TQSize( 250, 230 ).expandedTo( minimumSizeHint() ) );
}

WorkSheetSettings::~WorkSheetSettings()
{
}

void WorkSheetSettings::setRows( int rows )
{
  mRows->setValue( rows );
}

int WorkSheetSettings::rows() const
{
  return mRows->value();
}

void WorkSheetSettings::setColumns( int columns )
{
  mColumns->setValue( columns );
}

int WorkSheetSettings::columns() const
{
  return mColumns->value();
}

void WorkSheetSettings::setInterval( int interval )
{
  mInterval->setValue( interval );
}

int WorkSheetSettings::interval() const
{
  return mInterval->value();
}

void WorkSheetSettings::setSheetTitle( const TQString &title )
{
  mSheetTitle->setText( title );
}

TQString WorkSheetSettings::sheetTitle() const
{
  return mSheetTitle->text();
}

#include "WorkSheetSettings.moc"
