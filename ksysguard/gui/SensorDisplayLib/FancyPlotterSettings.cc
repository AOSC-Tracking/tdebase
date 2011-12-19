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
#include <kcolordialog.h>
#include <klineedit.h>
#include <klistview.h>
#include <klocale.h>
#include <knuminput.h>

#include <tqcheckbox.h>
#include <tqbuttongroup.h>
#include <tqgroupbox.h>
#include <tqimage.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpixmap.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>
#include <tqwhatsthis.h>
#include <tqheader.h>

#include "FancyPlotterSettings.h"

FancyPlotterSettings::FancyPlotterSettings( TQWidget* parent, const char* name )
  : KDialogBase( Tabbed, i18n( "Signal Plotter Settings" ), Ok | Apply | Cancel,
                 Ok, parent, name, false, true )
{
  TQFrame *page = 0;
  TQGridLayout *pageLayout = 0;
  TQGridLayout *boxLayout = 0;
  TQGroupBox *groupBox = 0;
  TQLabel *label = 0;

  // Style page
  page = addPage( i18n( "Style" ) );
  pageLayout = new TQGridLayout( page, 3, 2, 0, spacingHint() );

  label = new TQLabel( i18n( "Title:" ), page );
  pageLayout->addWidget( label, 0, 0 );

  mTitle = new KLineEdit( page );
  TQWhatsThis::add( mTitle, i18n( "Enter the title of the display here." ) );
  pageLayout->addWidget( mTitle, 0, 1 );
  label->setBuddy( mTitle );

  TQButtonGroup *buttonBox = new TQButtonGroup( 2, Qt::Vertical,
                                              i18n( "Graph Drawing Style" ), page );

  mUsePolygonStyle = new TQRadioButton( i18n( "Basic polygons" ), buttonBox );
  mUsePolygonStyle->setChecked( true );
  mUseOriginalStyle = new TQRadioButton( i18n( "Original - single line per data point" ), buttonBox );

  pageLayout->addMultiCellWidget( buttonBox, 1, 1, 0, 1 );

  // Scales page
  page = addPage( i18n( "Scales" ) );
  pageLayout = new TQGridLayout( page, 2, 1, 0, spacingHint() );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Vertical Scale" ), page );
  boxLayout = new TQGridLayout( groupBox->tqlayout(), 2, 5, spacingHint() );
  boxLayout->setColStretch( 2, 1 );

  mUseAutoRange = new TQCheckBox( i18n( "Automatic range detection" ), groupBox );
  TQWhatsThis::add( mUseAutoRange, i18n( "Check this box if you want the display range to adapt dynamically to the currently displayed values; if you do not check this, you have to specify the range you want in the fields below." ) );
  boxLayout->addMultiCellWidget( mUseAutoRange, 0, 0, 0, 4 );

  label = new TQLabel( i18n( "Minimum value:" ), groupBox );
  boxLayout->addWidget( label, 1, 0 );

  mMinValue = new KLineEdit( groupBox );
  mMinValue->setAlignment( AlignRight );
  mMinValue->setEnabled( false );
  TQWhatsThis::add( mMinValue, i18n( "Enter the minimum value for the display here. If both values are 0, automatic range detection is enabled." ) );
  boxLayout->addWidget( mMinValue, 1, 1 );
  label->setBuddy( mMinValue );

  label = new TQLabel( i18n( "Maximum value:" ), groupBox );
  boxLayout->addWidget( label, 1, 3 );

  mMaxValue = new KLineEdit( groupBox );
  mMaxValue->setAlignment( AlignRight );
  mMaxValue->setEnabled( false );
  TQWhatsThis::add( mMaxValue, i18n( "Enter the maximum value for the display here. If both values are 0, automatic range detection is enabled." ) );
  boxLayout->addWidget( mMaxValue, 1, 4 );
  label->setBuddy( mMaxValue );

  pageLayout->addWidget( groupBox, 0, 0 );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Horizontal Scale" ), page );
  boxLayout = new TQGridLayout( groupBox->tqlayout(), 2, 2, spacingHint() );
  boxLayout->setRowStretch( 1, 1 );

  mHorizontalScale = new KIntNumInput( 1, groupBox );
  mHorizontalScale->setMinValue( 1 );
  mHorizontalScale->setMaxValue( 50 );
  boxLayout->addWidget( mHorizontalScale, 0, 0 );

  label = new TQLabel( i18n( "pixel(s) per time period" ), groupBox );
  boxLayout->addWidget( label, 0, 1 );

  pageLayout->addWidget( groupBox, 1, 0 );

  // Grid page
  page = addPage( i18n( "Grid" ) );
  pageLayout = new TQGridLayout( page, 3, 2, 0, spacingHint() );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Lines" ), page );
  boxLayout = new TQGridLayout( groupBox->tqlayout(), 2, 5, spacingHint() );
  boxLayout->setColStretch( 1, 1 );

  mShowVerticalLines = new TQCheckBox( i18n( "Vertical lines" ), groupBox );
  TQWhatsThis::add( mShowVerticalLines, i18n( "Check this to activate the vertical lines if display is large enough." ) );
  boxLayout->addWidget( mShowVerticalLines, 0, 0 );

  label = new TQLabel( i18n( "Distance:" ), groupBox );
  boxLayout->addWidget( label, 0, 2 );

  mVerticalLinesDistance = new KIntNumInput( 0, groupBox );
  mVerticalLinesDistance->setMinValue( 10 );
  mVerticalLinesDistance->setMaxValue( 120 );
  TQWhatsThis::add( mVerticalLinesDistance, i18n( "Enter the distance between two vertical lines here." ) );
  boxLayout->addWidget( mVerticalLinesDistance , 0, 3 );
  label->setBuddy( mVerticalLinesDistance );

  mVerticalLinesScroll = new TQCheckBox( i18n( "Vertical lines scroll" ), groupBox );
  boxLayout->addWidget( mVerticalLinesScroll, 0, 4 );

  mShowHorizontalLines = new TQCheckBox( i18n( "Horizontal lines" ), groupBox );
  TQWhatsThis::add( mShowHorizontalLines, i18n( "Check this to enable horizontal lines if display is large enough." ) );
  boxLayout->addWidget( mShowHorizontalLines, 1, 0 );

  label = new TQLabel( i18n( "Count:" ), groupBox );
  boxLayout->addWidget( label, 1, 2 );

  mHorizontalLinesCount = new KIntNumInput( 0, groupBox );
  mHorizontalLinesCount->setMinValue( 1 );
  mHorizontalLinesCount->setMaxValue( 100 );
  TQWhatsThis::add( mHorizontalLinesCount, i18n( "Enter the number of horizontal lines here." ) );
  boxLayout->addWidget( mHorizontalLinesCount , 1, 3 );
  label->setBuddy( mHorizontalLinesCount );

  boxLayout->setRowStretch( 2, 1 );

  pageLayout->addMultiCellWidget( groupBox, 0, 0, 0, 1 );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Text" ), page );
  boxLayout = new TQGridLayout( groupBox->tqlayout(), 3, 4, spacingHint() );
  boxLayout->setColStretch( 1, 1 );

  mShowLabels = new TQCheckBox( i18n( "Labels" ), groupBox );
  TQWhatsThis::add( mShowLabels, i18n( "Check this box if horizontal lines should be decorated with the values they mark." ) );
  boxLayout->addWidget( mShowLabels, 0, 0 );

  label = new TQLabel( i18n( "Font size:" ), groupBox );
  boxLayout->addWidget( label, 0, 2 );

  mFontSize = new KIntNumInput( 9, groupBox );
  mFontSize->setMinValue( 5 );
  mFontSize->setMaxValue( 24 );
  boxLayout->addWidget( mFontSize, 0, 3 );
  label->setBuddy( mFontSize );

  mShowTopBar = new TQCheckBox( i18n( "Top bar" ), groupBox );
  TQWhatsThis::add( mShowTopBar, i18n( "Check this to active the display title bar. This is probably only useful for applet displays. The bar is only visible if the display is large enough." ) );
  boxLayout->addWidget( mShowTopBar, 1, 0 );

  boxLayout->setRowStretch( 2, 1 );

  pageLayout->addWidget( groupBox, 1, 0 );

  groupBox = new TQGroupBox( 0, Qt::Vertical, i18n( "Colors" ), page );
  boxLayout = new TQGridLayout( groupBox->tqlayout(), 4, 2, spacingHint() );

  label = new TQLabel( i18n( "Vertical lines:" ), groupBox );
  boxLayout->addWidget( label, 0, 0 );

  mVerticalLinesColor = new KColorButton( groupBox );
  boxLayout->addWidget( mVerticalLinesColor, 0, 1 );
  label->setBuddy( mVerticalLinesColor );

  label = new TQLabel( i18n( "Horizontal lines:" ), groupBox );
  boxLayout->addWidget( label, 1, 0 );

  mHorizontalLinesColor = new KColorButton( groupBox );
  boxLayout->addWidget( mHorizontalLinesColor, 1, 1 );
  label->setBuddy( mHorizontalLinesColor );

  label = new TQLabel( i18n( "Background:" ), groupBox );
  boxLayout->addWidget( label, 2, 0 );

  mBackgroundColor = new KColorButton( groupBox );
  boxLayout->addWidget( mBackgroundColor, 2, 1 );
  label->setBuddy( mBackgroundColor );

  boxLayout->setRowStretch( 3, 1 );

  pageLayout->addWidget( groupBox, 1, 1 );

  pageLayout->setRowStretch( 2, 1 );

  // Sensors page
  page = addPage( i18n( "Sensors" ) );
  pageLayout = new TQGridLayout( page, 6, 2, 0, spacingHint() );
  pageLayout->setRowStretch( 2, 1 );
  pageLayout->setRowStretch( 5, 1 );

  mSensorView = new KListView( page );
  mSensorView->addColumn("" , 0);
  mSensorView->addColumn( i18n( "Host" ) );
  mSensorView->addColumn( i18n( "Sensor" ) );
  mSensorView->addColumn( i18n( "Unit" ) );
  mSensorView->addColumn( i18n( "Status" ) );
  mSensorView->setResizeMode(TQListView::LastColumn);
  mSensorView->header()->setResizeEnabled(false, 0);
  mSensorView->hideColumn(0);
  mSensorView->header()->resizeSection(0, 0);
  mSensorView->setAllColumnsShowFocus( true );
  pageLayout->addMultiCellWidget( mSensorView, 0, 5, 0, 0 );
  mSensorView->setSortColumn ( -1 );
  mEditButton = new TQPushButton( i18n( "Set Color..." ), page );
  mEditButton->setEnabled( false );
  TQWhatsThis::add( mEditButton, i18n( "Push this button to configure the color of the sensor in the diagram." ) );
  pageLayout->addWidget( mEditButton, 0, 1 );

  mRemoveButton = new TQPushButton( i18n( "Delete" ), page );
  mRemoveButton->setEnabled( false );
  TQWhatsThis::add( mRemoveButton, i18n( "Push this button to delete the sensor." ) );
  pageLayout->addWidget( mRemoveButton, 1, 1 );

  mMoveUpButton = new TQPushButton( i18n( "Move Up" ), page );
  mMoveUpButton->setEnabled( false );
  pageLayout->addWidget( mMoveUpButton, 3, 1 );

  mMoveDownButton = new TQPushButton( i18n( "Move Down" ), page );
  mMoveDownButton->setEnabled( false );
  pageLayout->addWidget( mMoveDownButton, 4, 1 );

  connect( mUseAutoRange, TQT_SIGNAL( toggled( bool ) ), mMinValue,
           TQT_SLOT( setDisabled( bool ) ) );
  connect( mUseAutoRange, TQT_SIGNAL( toggled( bool ) ), mMaxValue,
           TQT_SLOT( setDisabled( bool ) ) );
  connect( mShowVerticalLines, TQT_SIGNAL( toggled( bool ) ), mVerticalLinesDistance,
           TQT_SLOT( setEnabled( bool ) ) );
  connect( mShowVerticalLines, TQT_SIGNAL( toggled( bool ) ), mVerticalLinesScroll,
           TQT_SLOT( setEnabled( bool ) ) );
  connect( mShowVerticalLines, TQT_SIGNAL( toggled( bool ) ), mVerticalLinesColor,
           TQT_SLOT( setEnabled( bool ) ) );
  connect( mShowHorizontalLines, TQT_SIGNAL( toggled( bool ) ), mHorizontalLinesCount,
           TQT_SLOT( setEnabled( bool ) ) );
  connect( mShowHorizontalLines, TQT_SIGNAL( toggled( bool ) ), mHorizontalLinesColor,
           TQT_SLOT( setEnabled( bool ) ) );
  connect( mShowHorizontalLines, TQT_SIGNAL( toggled( bool ) ), mShowLabels,
           TQT_SLOT( setEnabled( bool ) ) );
  connect( mSensorView, TQT_SIGNAL( selectionChanged( TQListViewItem* ) ),
           TQT_SLOT( selectionChanged( TQListViewItem* ) ) );

  connect( mEditButton, TQT_SIGNAL( clicked() ), TQT_SLOT( editSensor() ) );
  connect( mRemoveButton, TQT_SIGNAL( clicked() ), TQT_SLOT( removeSensor() ) );
  connect( mMoveUpButton, TQT_SIGNAL( clicked() ), TQT_SLOT( moveUpSensor() ) );
  connect( mMoveDownButton, TQT_SIGNAL( clicked() ), TQT_SLOT( moveDownSensor() ) );
  connect ( mSensorView, TQT_SIGNAL( doubleClicked( TQListViewItem *, const TQPoint &, int )), TQT_SLOT(editSensor()));

  KAcceleratorManager::manage( this );
}

FancyPlotterSettings::~FancyPlotterSettings()
{
}

void FancyPlotterSettings::setTitle( const TQString &title )
{
  mTitle->setText( title );
}

TQString FancyPlotterSettings::title() const
{
  return mTitle->text();
}

void FancyPlotterSettings::setUseAutoRange( bool value )
{
  mUseAutoRange->setChecked( value );
  mMinValue->setEnabled( !value );
  mMaxValue->setEnabled( !value );
}

bool FancyPlotterSettings::useAutoRange() const
{
  return mUseAutoRange->isChecked();
}

void FancyPlotterSettings::setMinValue( double min )
{
  mMinValue->setText( TQString::number( min ) );
}

double FancyPlotterSettings::minValue() const
{
  return mMinValue->text().toDouble();
}

void FancyPlotterSettings::setMaxValue( double max )
{
  mMaxValue->setText( TQString::number( max ) );
}

double FancyPlotterSettings::maxValue() const
{
  return mMaxValue->text().toDouble();
}

void FancyPlotterSettings::setUsePolygonStyle( bool value )
{
  if ( value )
    mUsePolygonStyle->setChecked( true );
  else
    mUseOriginalStyle->setChecked( true );
}

bool FancyPlotterSettings::usePolygonStyle() const
{
  return mUsePolygonStyle->isChecked();
}

void FancyPlotterSettings::setHorizontalScale( int scale )
{
  mHorizontalScale->setValue( scale );
}

int FancyPlotterSettings::horizontalScale() const
{
  return mHorizontalScale->value();
}

void FancyPlotterSettings::setShowVerticalLines( bool value )
{
  mShowVerticalLines->setChecked( value );
  mVerticalLinesDistance->setEnabled(  value );
  mVerticalLinesScroll->setEnabled( value );
  mVerticalLinesColor->setEnabled( value );
}

bool FancyPlotterSettings::showVerticalLines() const
{
  return mShowVerticalLines->isChecked();
}

void FancyPlotterSettings::setVerticalLinesColor( const TQColor &color )
{
  mVerticalLinesColor->setColor( color );
}

TQColor FancyPlotterSettings::verticalLinesColor() const
{
  return mVerticalLinesColor->color();
}

void FancyPlotterSettings::setVerticalLinesDistance( int distance )
{
  mVerticalLinesDistance->setValue( distance );
}

int FancyPlotterSettings::verticalLinesDistance() const
{
  return mVerticalLinesDistance->value();
}

void FancyPlotterSettings::setVerticalLinesScroll( bool value )
{
  mVerticalLinesScroll->setChecked( value );
}

bool FancyPlotterSettings::verticalLinesScroll() const
{
  return mVerticalLinesScroll->isChecked();
}

void FancyPlotterSettings::setShowHorizontalLines( bool value )
{
  mShowHorizontalLines->setChecked( value );
  mHorizontalLinesCount->setEnabled( value );
  mHorizontalLinesColor->setEnabled( value );
  mShowLabels->setEnabled( value );

}

bool FancyPlotterSettings::showHorizontalLines() const
{
  return mShowHorizontalLines->isChecked();
}

void FancyPlotterSettings::setHorizontalLinesColor( const TQColor &color )
{
  mHorizontalLinesColor->setColor( color );
}

TQColor FancyPlotterSettings::horizontalLinesColor() const
{
  return mHorizontalLinesColor->color();
}

void FancyPlotterSettings::setHorizontalLinesCount( int count )
{
  mHorizontalLinesCount->setValue( count );
}

int FancyPlotterSettings::horizontalLinesCount() const
{
  return mHorizontalLinesCount->value();
}

void FancyPlotterSettings::setShowLabels( bool value )
{
  mShowLabels->setChecked( value );
}

bool FancyPlotterSettings::showLabels() const
{
  return mShowLabels->isChecked();
}

void FancyPlotterSettings::setShowTopBar( bool value )
{
  mShowTopBar->setChecked( value );
}

bool FancyPlotterSettings::showTopBar() const
{
  return mShowTopBar->isChecked();
}

void FancyPlotterSettings::setFontSize( int size )
{
  mFontSize->setValue( size );
}

int FancyPlotterSettings::fontSize() const
{
  return mFontSize->value();
}

void FancyPlotterSettings::setBackgroundColor( const TQColor &color )
{
  mBackgroundColor->setColor( color );
}

TQColor FancyPlotterSettings::backgroundColor() const
{
  return mBackgroundColor->color();
}
void FancyPlotterSettings::clearDeleted() 
{
  mDeleted.clear();
}
TQValueList<int> FancyPlotterSettings::deleted() const
{
  return mDeleted;
}

TQValueList<int> FancyPlotterSettings::order() const
{
  TQValueList<int> newOrder;

  TQListViewItemIterator it( mSensorView );
  for ( ; it.current(); ++it ) {
    newOrder.prepend(it.current()->text(0).toInt());
  }
  return newOrder;
}

void FancyPlotterSettings::resetOrder()
{
  int i = mSensorView->childCount()-1;
  TQListViewItemIterator it( mSensorView );
  for ( ; it.current(); ++it, --i) {
    it.current()->setText(0, TQString::number(i));
  }
}

void FancyPlotterSettings::setSensors( const TQValueList< TQStringList > &list )
{
  mSensorView->clear();

  TQValueList< TQStringList >::ConstIterator it;
  for ( it = list.begin(); it != list.end(); ++it ) {
    TQListViewItem* lvi = new TQListViewItem( mSensorView,
                                            (*it)[ 0 ],   // id
                                            (*it)[ 1 ],   // host name
                                            (*it)[ 2 ],   // sensor name
                                            (*it)[ 3 ],   // unit
                                            (*it)[ 4 ] ); // status
    TQPixmap pm( 12, 12 );
    pm.fill( TQColor( (*it)[ 5 ] ) );
    lvi->setPixmap( 2, pm );
    mSensorView->insertItem( lvi );
  }
}

TQValueList< TQStringList > FancyPlotterSettings::sensors() const
{
  TQValueList< TQStringList > list;

  TQListViewItemIterator it( mSensorView );

  for ( ; it.current(); ++it ) {
    TQStringList entry;
    entry << it.current()->text( 0 );
    entry << it.current()->text( 1 );
    entry << it.current()->text( 2 );
    entry << it.current()->text( 3 );
    entry << it.current()->text( 4 );
    QRgb rgb = it.current()->pixmap( 2 )->convertToImage().pixel( 1, 1 );
    TQColor color( tqRed( rgb ), tqGreen( rgb ), tqBlue( rgb ) );
    entry << ( color.name() );

    list.prepend( entry );
  }

  return list;
}

void FancyPlotterSettings::editSensor()
{
  TQListViewItem* lvi = mSensorView->currentItem();

  if ( !lvi )
    return;

  TQColor color = lvi->pixmap( 2 )->convertToImage().pixel( 1, 1 );
  int result = KColorDialog::getColor( color, parentWidget() );
  if ( result == KColorDialog::Accepted ) {
    TQPixmap newPm( 12, 12 );
    newPm.fill( color );
    lvi->setPixmap( 2, newPm );
  }
}

void FancyPlotterSettings::removeSensor()
{
  TQListViewItem* lvi = mSensorView->currentItem();

  if ( lvi ) {
    //Note down the id of the one we are deleting
    int id = lvi->text(0).toInt();
    mDeleted.append(id);

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

    TQListViewItemIterator it( mSensorView );
    for ( ; it.current(); ++it ) {
      if(it.current()->text(0).toInt() > id)
        it.current()->setText(0, TQString::number(it.current()->text(0).toInt() -1));
    }


    if ( newSelected )
      mSensorView->ensureItemVisible( newSelected );
  }
}

void FancyPlotterSettings::moveUpSensor()
{
  if ( mSensorView->currentItem() != 0 ) {
    TQListViewItem* item = mSensorView->currentItem()->itemAbove();
    if ( item ) {
      if ( item->itemAbove() )
      {
        mSensorView->currentItem()->moveItem( item->itemAbove() );
      }
      else
      {
        item->moveItem( mSensorView->currentItem() );
      }
    }

    selectionChanged( mSensorView->currentItem() );
  }
}

void FancyPlotterSettings::moveDownSensor()
{
  if ( mSensorView->currentItem() != 0 ) {
    if ( mSensorView->currentItem()->itemBelow() )
      mSensorView->currentItem()->moveItem( mSensorView->currentItem()->itemBelow() );

    selectionChanged( mSensorView->currentItem() );
  }
}

void FancyPlotterSettings::selectionChanged( TQListViewItem *item )
{
  bool state = ( item != 0 );

  mEditButton->setEnabled( state );
  mRemoveButton->setEnabled( state );
  mMoveUpButton->setEnabled( state && item->itemAbove() );
  mMoveDownButton->setEnabled( state && item->itemBelow() );
}

#include "FancyPlotterSettings.moc"
