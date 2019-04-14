/*
   This file is part of the TDE project

   Copyright (C) 2008 Tobias Koenig <tokoe@kde.org>
   Copyright (C) 2016 Emanoil Kotsev <deloptes@yahoo.com>

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

#include "ktrashpropsdlgplugin.h"
#include "ktrashpropswidget.h"
#include "discspaceutil.h"
#include "trash_constant.h"
#include "trashimpl.h"

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqradiobutton.h>
#include <tqspinbox.h>

#include <kdesktopfile.h>
#include <kgenericfactory.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <knuminput.h>

#include <kdebug.h>

using namespace TrashConstant;

typedef KGenericFactory<KTrashPropsDlgPlugin, KPropertiesDialog> Factory;
K_EXPORT_COMPONENT_FACTORY( ktrashpropsdlgplugin, Factory( "ktrashpropsdlgplugin" ) )

KTrashPropsDlgPlugin::KTrashPropsDlgPlugin( KPropertiesDialog *dialog, const char*, const TQStringList& )
  : KPropsDlgPlugin( dialog )
{
  if ( dialog->items().count() != 1 )
    return;

  KFileItem *item = dialog->items().first();
  KURL itemUrl = item->url();
	if (!(itemUrl.protocol() == "trash" && item->name() == "."))
	{
	  // Check for a desktop file in case the protocol is not "trash"
		if (!KPropsDlgPlugin::isDesktopFile(item))
			return;

		KDesktopFile deskFile( itemUrl.path(), true /* readonly */ );
		if ( deskFile.readURL() != "trash:/" )
			return;
	}

  TDEGlobal::locale()->insertCatalogue( "tdeio_trash" );

  mTrashImpl = new TrashImpl();
  mTrashImpl->init();

  readConfig();

  TQFrame *frame = dialog->addPage(i18n("Polic&y"));
	policyWidget = NULL;

  setupGui( frame );

  mUseTimeLimit->setChecked( mConfigMap[ mCurrentTrash ].useTimeLimit );
  mUseSizeLimit->setChecked( mConfigMap[ mCurrentTrash ].useSizeLimit );
  mSizeLimitType = mConfigMap[ mCurrentTrash ].sizeLimitType;
	if ( mSizeLimitType == SIZE_LIMIT_FIXED )
	{
		mRbFixedSize->setChecked( true );
	}
	else
	{
		mRbPercentSize->setChecked( true );
	}
	
  mDays->setValue( mConfigMap[ mCurrentTrash ].days );
  mPercentSize->setValue( mConfigMap[ mCurrentTrash ].percent );
  mFixedSize->setValue( mConfigMap[ mCurrentTrash ].fixedSize );
  mFixedSizeUnit->setCurrentItem( mConfigMap[ mCurrentTrash ].fixedSizeUnit );
  mLimitReachedAction->setCurrentItem( mConfigMap[ mCurrentTrash ].actionType );
  percentSizeChanged(mPercentSize->value());
  fixedSizeChanged(mFixedSize->value());

  useTypeChanged();

  connect( mUseTimeLimit, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mUseTimeLimit, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( useTypeChanged() ) );
  connect( mDays, TQT_SIGNAL( valueChanged( int ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mUseSizeLimit, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mUseSizeLimit, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( useTypeChanged() ) );
  connect( mPercentSize, TQT_SIGNAL( valueChanged( double ) ),
           this, TQT_SLOT( percentSizeChanged( double ) ) );
  connect( mFixedSize, TQT_SIGNAL( valueChanged( double ) ),
           this, TQT_SLOT( fixedSizeChanged( double ) ) );
  connect( mPercentSize, TQT_SIGNAL( valueChanged( double ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mFixedSize, TQT_SIGNAL( valueChanged( double ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mFixedSizeUnit, TQT_SIGNAL( activated( int ) ),
           this, TQT_SLOT( fixedSizeUnitActivated( int ) ) );
  connect( mRbPercentSize, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( rbPercentSizeToggled( bool ) ) );
  connect( mRbPercentSize, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mRbFixedSize, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( rbFixedSizeToggled( bool ) ) );
  connect( mRbFixedSize, TQT_SIGNAL( toggled( bool ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mLimitReachedAction, TQT_SIGNAL( activated( int ) ),
           this, TQT_SLOT( setDirty() ) );

  trashChanged( 0 );
}

KTrashPropsDlgPlugin::~KTrashPropsDlgPlugin()
{
}

void KTrashPropsDlgPlugin::setupGui( TQFrame *frame )
{
  TQVBoxLayout *layout = new TQVBoxLayout( frame, 11, 6 );

  TrashImpl::TrashDirMap map = mTrashImpl->trashDirectories();
  if ( map.count() != 1 ) {
    // If we have multiple trashes, we setup a widget to choose
    // which trash to configure
    TQListBox *mountPoints = new TQListBox( frame );
    layout->addWidget( mountPoints );

    const TQPixmap folderPixmap = TDEGlobal::iconLoader()->loadIcon( "folder", TDEIcon::Small );
    TQMapConstIterator<int, TQString> it;
    for ( it = map.begin(); it != map.end(); ++it ) {
      DiscSpaceUtil util( it.data() );
      mountPoints->insertItem( folderPixmap, util.mountPoint(), it.key() ); 
    }

    mountPoints->setCurrentItem( 0 );

    connect( mountPoints, TQT_SIGNAL( highlighted( int ) ), TQT_SLOT( trashChanged( int ) ) );
  } else {
    mCurrentTrash = map[0];
  }

  TQHBoxLayout *daysLayout = new TQHBoxLayout( layout );

  mUseTimeLimit = new TQCheckBox( i18n( "Delete files older than:" ), frame );
  daysLayout->addWidget( mUseTimeLimit );
  mDays = new TQSpinBox( 1, 365, 1, frame );
  mDays->setSuffix( " days" );
  daysLayout->addWidget( mDays );

  TQGridLayout *sizeLayout = new TQGridLayout( layout, 2, 2 );
  mUseSizeLimit = new TQCheckBox( i18n( "Limit to maximum size" ), frame );
  sizeLayout->addMultiCellWidget( mUseSizeLimit, 0, 0, 0, 1 );

  mSizeWidget = new TQWidget( frame );
  sizeLayout->setColSpacing( 0, 30 );
  sizeLayout->addWidget( mSizeWidget, 1, 1 );

  TQGridLayout *sizeWidgetLayout = new TQGridLayout( mSizeWidget, 3, 3, 11, 6 );
  
 	mRbPercentSize = new TQRadioButton( i18n("&Percentage:"), mSizeWidget );
 	mRbFixedSize = new TQRadioButton( i18n("&Fixed size:"), mSizeWidget );
 	sizeWidgetLayout->addWidget( mRbPercentSize, 0, 0 );
 	sizeWidgetLayout->addWidget( mRbFixedSize, 1, 0 );

  mPercentSize = new KDoubleSpinBox( 0, 100, 0.1, 10, 2, mSizeWidget );
  mPercentSize->setSuffix( " %" );
  sizeWidgetLayout->addWidget( mPercentSize, 0, 1 );
  mSizeLabel = new TQLabel( mSizeWidget );
  sizeWidgetLayout->addWidget( mSizeLabel, 0, 2 );

  mFixedSize = new KDoubleSpinBox( 0, 1024*1024, 1, 500, 2, mSizeWidget );
  sizeWidgetLayout->addWidget( mFixedSize, 1, 1 );
  mFixedSizeUnit = new TQComboBox( mSizeWidget );
  mFixedSizeUnit->setEditable( false );
  mFixedSizeUnit->insertItem( i18n( "Bytes" ), SIZE_ID_B );
  mFixedSizeUnit->insertItem( i18n( "KBytes" ), SIZE_ID_KB );
  mFixedSizeUnit->insertItem( i18n( "MBytes" ), SIZE_ID_MB );
  mFixedSizeUnit->insertItem( i18n( "GBytes" ), SIZE_ID_GB );
  mFixedSizeUnit->insertItem( i18n( "TBytes" ), SIZE_ID_TB );
  mFixedSizeUnit->setCurrentItem( 2 );
  sizeWidgetLayout->addWidget( mFixedSizeUnit, 1, 2 );
  
  TQLabel *label = new TQLabel( i18n( "When limit reached:" ), mSizeWidget );
  sizeWidgetLayout->addWidget( label, 2, 0 );

  mLimitReachedAction = new TQComboBox( mSizeWidget );
  mLimitReachedAction->insertItem( i18n( "Warn me" ) );
  mLimitReachedAction->insertItem( i18n( "Delete oldest files from trash" ) );
  mLimitReachedAction->insertItem( i18n( "Delete biggest files from trash" ) );
  sizeWidgetLayout->addMultiCellWidget( mLimitReachedAction, 2, 2, 1, 2 );

  layout->addStretch();
}

void KTrashPropsDlgPlugin::applyChanges()
{
  if ( !mCurrentTrash.isEmpty() ) {
    ConfigEntry entry;
    entry.useTimeLimit = mUseTimeLimit->isChecked();
    entry.days = mDays->value();
    entry.useSizeLimit = mUseSizeLimit->isChecked();
    if (mRbFixedSize->isChecked())
    {
    	entry.sizeLimitType = SIZE_LIMIT_FIXED;
    }
    else
    {
    	entry.sizeLimitType = SIZE_LIMIT_PERCENT;
    }
    entry.percent = mPercentSize->value();
    entry.fixedSize = mFixedSize->value();
    entry.fixedSizeUnit = mFixedSizeUnit->currentItem();
    entry.actionType = mLimitReachedAction->currentItem();
    mConfigMap.insert( mCurrentTrash, entry );
  }

  writeConfig();

  // Adjust trash size if necessary
  TrashImpl::TrashDirMap trashDirs = mTrashImpl->trashDirectories();
	for (TrashImpl::TrashDirMap::ConstIterator it = trashDirs.begin(); it != trashDirs.end(); ++it)
	{
		mTrashImpl->resizeTrash(it.key());
	}
}

void KTrashPropsDlgPlugin::percentSizeChanged( double percent )
{
  DiscSpaceUtil util( mCurrentTrash );
  double partitionSize = util.size() * 1024.0; // convert to byte
  double size = partitionSize*(percent/100.0);

  TQString unit = i18n( "Bytes" );
  if ( size >= 1024 ) {
    unit = i18n( "KBytes" );
    size = size/1024.0;
  }
  if ( size >= 1024 ) {
    unit = i18n( "MBytes" );
    size = size/1024.0;
  }
  if ( size >= 1024 ) {
    unit = i18n( "GBytes" );
    size = size/1024.0;
  }
  if ( size >= 1024 ) {
    unit = i18n( "TBytes" );
    size = size/1024.0;
  }

  mSizeLabel->setText( i18n( "(%1 %2)" ).arg( TQString::number( size, 'f', 2 ) ).arg( unit ) );
}

void KTrashPropsDlgPlugin::fixedSizeChanged( double value )
{
	int currItem = mFixedSizeUnit->currentItem();
	if (value > 1023.999 && currItem >= SIZE_ID_TB)
	{
	  // Max limit 1024 TBytes
		mFixedSizeUnit->setCurrentItem(SIZE_ID_TB);
		mFixedSize->setValue(1024.0);
	}
	else if (value > 1023.999 && currItem < SIZE_ID_TB)
	{
		// Scale up to higher measure unit
	  while (value > 1023.999 && currItem < SIZE_ID_TB)
	  {
			++currItem;
			value /= 1024.0;
		}
		mFixedSizeUnit->setCurrentItem(currItem);
		mFixedSize->setValue(value);
	}
	else if (value < 0.001)
	{
		// Scale down to lower measure unit
	  int currItem = mFixedSizeUnit->currentItem();		
	  if (currItem > SIZE_ID_B)
	  {
	  	mFixedSizeUnit->setCurrentItem(currItem - 1);
	  	mFixedSize->setValue(1023.0);
	  }
	}
	// Need to call this manually because "activated" is not emitted by "mFixedSizeUnit"
	// when the index is changed programmatically (see TQComboBox API docs)
	fixedSizeUnitActivated(mFixedSizeUnit->currentItem());
}

void KTrashPropsDlgPlugin::fixedSizeUnitActivated(int index)
{
	// Bytes can not be split into fractions
	if (index == SIZE_ID_B)
	{
		mFixedSize->setPrecision(0);
	}
	else
	{
		mFixedSize->setPrecision(2);
	}
}
    
void KTrashPropsDlgPlugin::rbPercentSizeToggled( bool buttonOn)
{
	if (buttonOn)
	{
		mRbFixedSize->setChecked(false);
		mSizeLimitType = SIZE_LIMIT_PERCENT;
	}
	else if (!mRbFixedSize->isChecked())
	{
		// Set the button back on if the user clicked it twice
		mRbPercentSize->setChecked(true);
	}
}
    
void KTrashPropsDlgPlugin::rbFixedSizeToggled( bool buttonOn)
{
	if (buttonOn)
	{
		mRbPercentSize->setChecked(false);
		mSizeLimitType = SIZE_LIMIT_FIXED;
	}
	else if (!mRbPercentSize->isChecked())
	{
		// Set the button back on if the user clicked it twice
		mRbFixedSize->setChecked(true);
	}
}

void KTrashPropsDlgPlugin::trashChanged( int value )
{
  const TrashImpl::TrashDirMap map = mTrashImpl->trashDirectories();

  if ( !mCurrentTrash.isEmpty() ) {
    ConfigEntry entry;
    entry.useTimeLimit = mUseTimeLimit->isChecked();
    entry.days = mDays->value();
    entry.useSizeLimit = mUseSizeLimit->isChecked();
    entry.sizeLimitType = mSizeLimitType;
    entry.percent = mPercentSize->value(),
    entry.fixedSize = mFixedSize->value();
    entry.fixedSizeUnit = mFixedSizeUnit->currentItem();
    entry.actionType = mLimitReachedAction->currentItem();
    mConfigMap.insert( mCurrentTrash, entry );
  }

  mCurrentTrash = map[ value ];

  if ( mConfigMap.contains( mCurrentTrash ) ) {
    const ConfigEntry entry = mConfigMap[ mCurrentTrash ];
    mUseTimeLimit->setChecked( entry.useTimeLimit );
    mDays->setValue( entry.days );
    mUseSizeLimit->setChecked( entry.useSizeLimit );
		if ( entry.sizeLimitType == SIZE_LIMIT_FIXED )
		{
			mRbFixedSize->setChecked( true );
		}
		else
		{
			mRbPercentSize->setChecked( true );
		}
    mPercentSize->setValue( entry.percent );
		mFixedSize->setValue( entry.fixedSize );
		mFixedSizeUnit->setCurrentItem( entry.fixedSizeUnit );
    mLimitReachedAction->setCurrentItem( entry.actionType );
  } else {
    mUseTimeLimit->setChecked( false );
    mDays->setValue( 7 );
    mUseSizeLimit->setChecked( true );
    mRbPercentSize->setChecked( true );
    mPercentSize->setValue( 10.0 );
		mFixedSize->setValue( 500 );
		mFixedSizeUnit->setCurrentItem( SIZE_ID_MB );
    mLimitReachedAction->setCurrentItem( 0 );
  }

  percentSizeChanged( mPercentSize->value() );
  fixedSizeChanged( mFixedSize->value() );
}

void KTrashPropsDlgPlugin::useTypeChanged()
{
  mDays->setEnabled( mUseTimeLimit->isChecked() );
  mSizeWidget->setEnabled( mUseSizeLimit->isChecked() );
}

void KTrashPropsDlgPlugin::readConfig()
{
  TDEConfig config( "trashrc" );
  mConfigMap.clear();

  const TQStringList groups = config.groupList();
  for ( uint i = 0; i < groups.count(); ++i ) {
    if ( groups[ i ].startsWith( "/" ) ) {
      config.setGroup( groups[ i ] );
      ConfigEntry entry;
      entry.useTimeLimit = config.readBoolEntry( "UseTimeLimit", false );
      entry.days = config.readNumEntry( "Days", 32000 );
      entry.useSizeLimit = config.readBoolEntry( "UseSizeLimit", true );
	    entry.sizeLimitType = config.readNumEntry( "SizeLimitType", SIZE_LIMIT_PERCENT );
      entry.percent = config.readDoubleNumEntry( "Percent", 10 );
      entry.fixedSize = config.readDoubleNumEntry( "FixedSize", 500 );
      entry.fixedSizeUnit = config.readNumEntry( "FixedSizeUnit", SIZE_ID_MB );
      entry.actionType = config.readNumEntry( "LimitReachedAction", TrashConstant::ACTION_WARNING );
      mConfigMap.insert( groups[ i ], entry );
    }
  }
}

void KTrashPropsDlgPlugin::writeConfig()
{
  TDEConfig config( "trashrc" );

  // first delete all existing groups
  const TQStringList groups = config.groupList();
  for ( uint i = 0; i < groups.count(); ++i )
    if ( groups[ i ].startsWith( "/" ) )
      config.deleteGroup( groups[ i ] );

  TQMapConstIterator<TQString, ConfigEntry> it;
  for ( it = mConfigMap.begin(); it != mConfigMap.end(); ++it ) {
    config.setGroup( it.key() );
    config.writeEntry( "UseTimeLimit", it.data().useTimeLimit );
    config.writeEntry( "Days", it.data().days );
    config.writeEntry( "UseSizeLimit", it.data().useSizeLimit );
    config.writeEntry( "SizeLimitType", it.data().sizeLimitType );
    config.writeEntry( "Percent", it.data().percent );
    config.writeEntry( "FixedSize", it.data().fixedSize );
    config.writeEntry( "FixedSizeUnit", it.data().fixedSizeUnit );
    config.writeEntry( "LimitReachedAction", it.data().actionType );
  }

  config.sync();
}

#include "ktrashpropsdlgplugin.moc"
