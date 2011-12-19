/*
    KSysGuard, the KDE System Guard
   
    Copyright (c) 1999 - 2001 Chris Schlaeger <cs@kde.org>
    
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

#include <tqimage.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqpixmap.h>
#include <tqpushbutton.h>
#include <tqspinbox.h>
#include <tqtabwidget.h>

#include <kaccelmanager.h>
#include <kcolorbutton.h>
#include <kcolordialog.h>
#include <klocale.h>

#include "StyleSettings.h"

StyleSettings::StyleSettings( TQWidget *parent, const char *name )
  : KDialogBase( Tabbed, i18n( "Global Style Settings" ), Help | Ok | Apply |
                 Cancel, Ok, parent, name, true, true )
{
  TQFrame *page = addPage( i18n( "Display Style" ) );
  TQGridLayout *layout = new TQGridLayout( page, 6, 2, 0, spacingHint() );

  TQLabel *label = new TQLabel( i18n( "First foreground color:" ), page );
  layout->addWidget( label, 0, 0 );

  mFirstForegroundColor = new KColorButton( page );
  layout->addWidget( mFirstForegroundColor, 0, 1 );
  label->setBuddy( mFirstForegroundColor );

  label = new TQLabel( i18n( "Second foreground color:" ), page );
  layout->addWidget( label, 1, 0 );

  mSecondForegroundColor = new KColorButton( page );
  layout->addWidget( mSecondForegroundColor, 1, 1 );
  label->setBuddy( mSecondForegroundColor );

  label = new TQLabel( i18n( "Alarm color:" ), page );
  layout->addWidget( label, 2, 0 );

  mAlarmColor = new KColorButton( page );
  layout->addWidget( mAlarmColor, 2, 1 );
  label->setBuddy( mAlarmColor );

  label = new TQLabel( i18n( "Background color:" ), page );
  layout->addWidget( label, 3, 0 );

  mBackgroundColor = new KColorButton( page );
  layout->addWidget( mBackgroundColor, 3, 1 );
  label->setBuddy( mBackgroundColor );

  label = new TQLabel( i18n( "Font size:" ), page );
  layout->addWidget( label, 4, 0 );

  mFontSize = new TQSpinBox( 7, 48, 1, page );
  mFontSize->setValue( 8 );
  layout->addWidget( mFontSize, 4, 1 );
  label->setBuddy( mFontSize );

  layout->setRowStretch( 5, 1 );

  page = addPage( i18n( "Sensor Colors" ) );
  layout = new TQGridLayout( page, 1, 2, 0, spacingHint() );

  mColorListBox = new TQListBox( page );
  layout->addWidget( mColorListBox, 0, 0 );

  mEditColorButton = new TQPushButton( i18n( "Change Color..." ), page );
  mEditColorButton->setEnabled( false );
  layout->addWidget( mEditColorButton, 0, 1, Qt::AlignTop );

  connect( mColorListBox, TQT_SIGNAL( selectionChanged( TQListBoxItem* ) ),
           TQT_SLOT( selectionChanged( TQListBoxItem* ) ) );
  connect( mColorListBox, TQT_SIGNAL( doubleClicked( TQListBoxItem* ) ),
           TQT_SLOT( editSensorColor() ) );
  connect( mEditColorButton, TQT_SIGNAL( clicked() ),
           TQT_SLOT( editSensorColor() ) );

  KAcceleratorManager::manage( this );
}

StyleSettings::~StyleSettings()
{
}

void StyleSettings::setFirstForegroundColor( const TQColor &color )
{
  mFirstForegroundColor->setColor( color );
}

TQColor StyleSettings::firstForegroundColor() const
{
  return mFirstForegroundColor->color();
}

void StyleSettings::setSecondForegroundColor( const TQColor &color )
{
  mSecondForegroundColor->setColor( color );
}

TQColor StyleSettings::secondForegroundColor() const
{
  return mSecondForegroundColor->color();
}

void StyleSettings::setAlarmColor( const TQColor &color )
{
  mAlarmColor->setColor( color );
}

TQColor StyleSettings::alarmColor() const
{
  return mAlarmColor->color();
}

void StyleSettings::setBackgroundColor( const TQColor &color )
{
  mBackgroundColor->setColor( color );
}

TQColor StyleSettings::backgroundColor() const
{
  return mBackgroundColor->color();
}

void StyleSettings::setFontSize( uint size )
{
  mFontSize->setValue( size );
}

uint StyleSettings::fontSize() const
{
  return mFontSize->value();
}

void StyleSettings::setSensorColors( const TQValueList<TQColor> &list )
{
  mColorListBox->clear();

  for ( uint i = 0; i < list.count(); ++i ) {
    TQPixmap pm( 12, 12 );
		pm.fill( *list.at( i ) );
    mColorListBox->insertItem( pm, i18n( "Color %1" ).arg( i ) );
	}
}

TQValueList<TQColor> StyleSettings::sensorColors()
{
  TQValueList<TQColor> list;

  for ( uint i = 0; i < mColorListBox->count(); ++i )
    list.append( TQColor( mColorListBox->pixmap( i )->convertToImage().pixel( 1, 1 ) ) );

  return list;
}

void StyleSettings::editSensorColor()
{
  int pos = mColorListBox->currentItem();

  if ( pos < 0 )
    return;

  TQColor color = mColorListBox->pixmap( pos )->convertToImage().pixel( 1, 1 );

  if ( KColorDialog::getColor( color ) == KColorDialog::Accepted ) {
    TQPixmap pm( 12, 12 );
		pm.fill( color );
    mColorListBox->changeItem( pm, mColorListBox->text( pos ), pos );
	}
}

void StyleSettings::selectionChanged( TQListBoxItem *item )
{
  mEditColorButton->setEnabled( item != 0 );
}

#include "StyleSettings.moc"
