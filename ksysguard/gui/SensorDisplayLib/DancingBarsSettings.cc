/*
    KSysGuard, the KDE System Guard

    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>

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

#include <kaccelmanager.h>
#include <kcolorbutton.h>
#include <klineedit.h>
#include <kinputdialog.h>
#include <klistview.h>
#include <klocale.h>
#include <knuminput.h>

#include <tqcheckbox.h>
#include <tqframe.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqwhatsthis.h>

#include "DancingBarsSettings.h"

DancingBarsSettings::DancingBarsSettings( TQWidget* parent, const char* name )
  : KDialogBase( Tabbed, i18n( "Edit BarGraph Preferences" ), 
    Ok | Apply | Cancel, Ok, parent, name, true, true )
{
  // Range page
  TQFrame *page = addPage( i18n( "Range" ) );
  TQGridLayout *pageLayout = new TQGridLayout( page, 3, 1, 0, spacingHint() );

  TQGroupBox *groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Title" ), page );
  TQGridLayout *boxLayout = new TQGridLayout( groupBox->layout(), 1, 1 );

  mTitle = new KLineEdit( groupBox );
  TQWhatsThis::add( mTitle, i18n( "Enter the title of the display here." ) );
  boxLayout->addWidget( mTitle, 0, 0 );

  pageLayout->addWidget( groupBox, 0, 0 );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Display Range" ), page );
  boxLayout = new TQGridLayout( groupBox->layout(), 1, 5 );
  boxLayout->setColStretch( 2, 1 );

  TQLabel *label = new TQLabel( i18n( "Minimum value:" ), groupBox );
  boxLayout->addWidget( label, 0, 0 );

  mMinValue = new KDoubleSpinBox( 0, 100, 0.5, 0, 2, groupBox );
  TQWhatsThis::add( mMinValue, i18n( "Enter the minimum value for the display here. If both values are 0, automatic range detection is enabled." ) );
  boxLayout->addWidget( mMinValue, 0, 1 );
  label->setBuddy( mMinValue );

  label = new TQLabel( i18n( "Maximum value:" ), groupBox );
  boxLayout->addWidget( label, 0, 3 );

  mMaxValue = new KDoubleSpinBox( 0, 10000, 0.5, 100, 2, groupBox );
  TQWhatsThis::add( mMaxValue, i18n( "Enter the maximum value for the display here. If both values are 0, automatic range detection is enabled." ) );
  boxLayout->addWidget( mMaxValue, 0, 4 );
  label->setBuddy( mMaxValue );

  pageLayout->addWidget( groupBox, 1, 0 );

  pageLayout->setRowStretch( 2, 1 );

  // Alarm page
  page = addPage( i18n( "Alarms" ) );
  pageLayout = new TQGridLayout( page, 3, 1, 0, spacingHint() );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Alarm for Minimum Value" ), page );
  boxLayout = new TQGridLayout( groupBox->layout(), 1, 4 );
  boxLayout->setColStretch( 1, 1 );

  mUseLowerLimit = new TQCheckBox( i18n( "Enable alarm" ), groupBox );
  TQWhatsThis::add( mUseLowerLimit, i18n( "Enable the minimum value alarm." ) );
  boxLayout->addWidget( mUseLowerLimit, 0, 0 );

  label = new TQLabel( i18n( "Lower limit:" ), groupBox );
  boxLayout->addWidget( label, 0, 2 );

  mLowerLimit = new KDoubleSpinBox( 0, 100, 0.5, 0, 2, groupBox );
  mLowerLimit->setEnabled( false );
  boxLayout->addWidget( mLowerLimit, 0, 3 );
  label->setBuddy( mLowerLimit );

  pageLayout->addWidget( groupBox, 0, 0 );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Alarm for Maximum Value" ), page );
  boxLayout = new TQGridLayout( groupBox->layout(), 1, 4 );
  boxLayout->setColStretch( 1, 1 );

  mUseUpperLimit = new TQCheckBox( i18n( "Enable alarm" ), groupBox );
  TQWhatsThis::add( mUseUpperLimit, i18n( "Enable the maximum value alarm." ) );
  boxLayout->addWidget( mUseUpperLimit, 0, 0 );

  label = new TQLabel( i18n( "Upper limit:" ), groupBox );
  boxLayout->addWidget( label, 0, 2 );

  mUpperLimit = new KDoubleSpinBox( 0, 100, 0.5, 0, 2, groupBox );
  mUpperLimit->setEnabled( false );
  boxLayout->addWidget( mUpperLimit, 0, 3 );
  label->setBuddy( mUpperLimit );

  pageLayout->addWidget( groupBox, 1, 0 );

  pageLayout->setRowStretch( 2, 1 );

  // Look page
  page = addPage( i18n( "Look" ) );
  pageLayout = new TQGridLayout( page, 5, 2, 0, spacingHint() );

  label = new TQLabel( i18n( "Normal bar color:" ), page );
  pageLayout->addWidget( label, 0, 0 );

  mForegroundColor = new KColorButton( page );
  pageLayout->addWidget( mForegroundColor, 0, 1 );
  label->setBuddy( mForegroundColor );  

  label = new TQLabel( i18n( "Out-of-range color:" ), page );
  pageLayout->addWidget( label, 1, 0 );

  mAlarmColor = new KColorButton( page );
  pageLayout->addWidget( mAlarmColor, 1, 1 );
  label->setBuddy( mAlarmColor );  

  label = new TQLabel( i18n( "Background color:" ), page );
  pageLayout->addWidget( label, 2, 0 );

  mBackgroundColor = new KColorButton( page );
  pageLayout->addWidget( mBackgroundColor, 2, 1 );
  label->setBuddy( mBackgroundColor );  

  label = new TQLabel( i18n( "Font size:" ), page );
  pageLayout->addWidget( label, 3, 0 );

  mFontSize = new KIntNumInput( 9, page );
  TQWhatsThis::add( mFontSize, i18n( "This determines the size of the font used to print a label underneath the bars. Bars are automatically suppressed if text becomes too large, so it is advisable to use a small font size here." ) );
  pageLayout->addWidget( mFontSize, 3, 1 );
  label->setBuddy( mFontSize );  

  pageLayout->setRowStretch( 4, 1 );

  // Sensor page
  page = addPage( i18n( "Sensors" ) );
  pageLayout = new TQGridLayout( page, 3, 2, 0, spacingHint() );
  pageLayout->setRowStretch( 2, 1 );

  mSensorView = new KListView( page );
  mSensorView->addColumn( i18n( "Host" ) );
  mSensorView->addColumn( i18n( "Sensor" ) );
  mSensorView->addColumn( i18n( "Label" ) );
  mSensorView->addColumn( i18n( "Unit" ) );
  mSensorView->addColumn( i18n( "Status" ) );
  mSensorView->setAllColumnsShowFocus( true );
  pageLayout->addMultiCellWidget( mSensorView, 0, 2, 0, 0 );

  mEditButton = new TQPushButton( i18n( "Edit..." ), page );
  mEditButton->setEnabled( false );
  TQWhatsThis::add( mEditButton, i18n( "Push this button to configure the label." ) );
  pageLayout->addWidget( mEditButton, 0, 1 );

  mRemoveButton = new TQPushButton( i18n( "Delete" ), page );
  mRemoveButton->setEnabled( false );
  TQWhatsThis::add( mRemoveButton, i18n( "Push this button to delete the sensor." ) );
  pageLayout->addWidget( mRemoveButton, 1, 1 );

  connect( mUseLowerLimit, TQT_SIGNAL( toggled( bool ) ),
           mLowerLimit, TQT_SLOT( setEnabled( bool ) ) );
  connect( mUseUpperLimit, TQT_SIGNAL( toggled( bool ) ),
           mUpperLimit, TQT_SLOT( setEnabled( bool ) ) );

  connect( mSensorView, TQT_SIGNAL( selectionChanged( TQListViewItem* ) ),
           TQT_SLOT( selectionChanged( TQListViewItem* ) ) );
  connect( mEditButton, TQT_SIGNAL( clicked() ), TQT_SLOT( editSensor() ) );
  connect( mRemoveButton, TQT_SIGNAL( clicked() ), TQT_SLOT( removeSensor() ) );

  KAcceleratorManager::manage( this );

  mTitle->setFocus();
}

DancingBarsSettings::~DancingBarsSettings()
{
}

void DancingBarsSettings::setTitle( const TQString& title )
{
  mTitle->setText( title );
}

TQString DancingBarsSettings::title() const
{
  return mTitle->text();
}

void DancingBarsSettings::setMinValue( double min )
{
  mMinValue->setValue( min );
}

double DancingBarsSettings::minValue() const
{
  return mMinValue->value();
}

void DancingBarsSettings::setMaxValue( double max )
{
  mMaxValue->setValue( max );
}

double DancingBarsSettings::maxValue() const
{
  return mMaxValue->value();
}

void DancingBarsSettings::setUseLowerLimit( bool value )
{
  mUseLowerLimit->setChecked( value );
}

bool DancingBarsSettings::useLowerLimit() const
{
  return mUseLowerLimit->isChecked();
}

void DancingBarsSettings::setLowerLimit( double limit )
{
  mLowerLimit->setValue( limit );
}

double DancingBarsSettings::lowerLimit() const
{
  return mLowerLimit->value();
}

void DancingBarsSettings::setUseUpperLimit( bool value )
{
  mUseUpperLimit->setChecked( value );
}

bool DancingBarsSettings::useUpperLimit() const
{
  return mUseUpperLimit->isChecked();
}

void DancingBarsSettings::setUpperLimit( double limit )
{
  mUpperLimit->setValue( limit );
}

double DancingBarsSettings::upperLimit() const
{
  return mUpperLimit->value();
}

void DancingBarsSettings::setForegroundColor( const TQColor &color )
{
  mForegroundColor->setColor( color );
}

TQColor DancingBarsSettings::foregroundColor() const
{
  return mForegroundColor->color();
}

void DancingBarsSettings::setAlarmColor( const TQColor &color )
{
  mAlarmColor->setColor( color );
}

TQColor DancingBarsSettings::alarmColor() const
{
  return mAlarmColor->color();
}

void DancingBarsSettings::setBackgroundColor( const TQColor &color )
{
  mBackgroundColor->setColor( color );
}

TQColor DancingBarsSettings::backgroundColor() const
{
  return mBackgroundColor->color();
}

void DancingBarsSettings::setFontSize( int size )
{
  mFontSize->setValue( size );
}

int DancingBarsSettings::fontSize() const
{
  return mFontSize->value();
}

void DancingBarsSettings::setSensors( const TQValueList< TQStringList > &list )
{
  mSensorView->clear();

  TQValueList< TQStringList >::ConstIterator it;
  for ( it = list.begin(); it != list.end(); ++it ) {
    new TQListViewItem( mSensorView,
                       (*it)[ 0 ],   // host name
                       (*it)[ 1 ],   // sensor name
                       (*it)[ 2 ],   // footer title
                       (*it)[ 3 ],   // unit
                       (*it)[ 4 ] ); // status
  }
}

TQValueList< TQStringList > DancingBarsSettings::sensors() const
{
  TQValueList< TQStringList > list;

  TQListViewItemIterator it( mSensorView );
  while ( it.current() && !it.current()->text( 0 ).isEmpty() ) {
    TQStringList entry;
    entry << it.current()->text( 0 );
    entry << it.current()->text( 1 );
    entry << it.current()->text( 2 );
    entry << it.current()->text( 3 );
    entry << it.current()->text( 4 );

    list.append( entry );
    ++it;
  }

  return list;
}

void DancingBarsSettings::editSensor()
{
  TQListViewItem *lvi = mSensorView->currentItem();

  if ( !lvi )
    return;

  bool ok;
  TQString str = KInputDialog::getText( i18n( "Label of Bar Graph" ),
    i18n( "Enter new label:" ), lvi->text( 2 ), &ok, this );
  if ( ok )
    lvi->setText( 2, str );
}

void DancingBarsSettings::removeSensor()
{
  TQListViewItem *lvi = mSensorView->currentItem();

  if ( lvi ) {
    /* Before we delete the currently selected item, we determine a
     * new item to be selected. That way we can ensure that multiple
     * items can be deleted without forcing the user to select a new
     * item between the deletes. If all items are deleted, the buttons
     * are disabled again. */
    TQListViewItem* newSelected = 0;
    if ( lvi->itemBelow() ) {
      lvi->itemBelow()->setSelected( true );
      newSelected = lvi->itemBelow();
    } else if ( lvi->itemAbove() ) {
      lvi->itemAbove()->setSelected( true );
      newSelected = lvi->itemAbove();
    } else
      selectionChanged( 0 );

    delete lvi;

    if ( newSelected )
      mSensorView->ensureItemVisible( newSelected );
  }
}

void DancingBarsSettings::selectionChanged( TQListViewItem* lvi )
{
  bool state = ( lvi != 0 );

  mEditButton->setEnabled( state );
  mRemoveButton->setEnabled( state );
}


#include "DancingBarsSettings.moc"
