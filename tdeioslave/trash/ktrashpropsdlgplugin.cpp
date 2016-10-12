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
#include "discspaceutil.h"
#include "trashimpl.h"

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqspinbox.h>

#include <kdesktopfile.h>
#include <kgenericfactory.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <knuminput.h>

#include <kdebug.h>

typedef KGenericFactory<KTrashPropsDlgPlugin, KPropertiesDialog> Factory;
K_EXPORT_COMPONENT_FACTORY( ktrashpropsdlgplugin, Factory( "ktrashpropsdlgplugin" ) )

KTrashPropsDlgPlugin::KTrashPropsDlgPlugin( KPropertiesDialog *dialog, const char*, const TQStringList& )
  : KPropsDlgPlugin( dialog )
{
  if ( dialog->items().count() != 1 )
    return;

  KFileItem *item = dialog->items().first();

  if ( !KPropsDlgPlugin::isDesktopFile( item ) )
    return;

  KDesktopFile deskFile( item->url().path(), true /* readonly */ );

  if ( deskFile.readURL() != "trash:/" )
    return;

  TDEGlobal::locale()->insertCatalogue( "tdeio_trash" );

  mTrashImpl = new TrashImpl();
  mTrashImpl->init();

  readConfig();

  TQFrame *frame = dialog->addPage( i18n( "Size Limits" ) );
  setupGui( frame );


  mUseTimeLimit->setChecked( mConfigMap[ mCurrentTrash ].useTimeLimit );
  mUseSizeLimit->setChecked( mConfigMap[ mCurrentTrash ].useSizeLimit );
  mDays->setValue( mConfigMap[ mCurrentTrash ].days );
  mPercent->setValue( mConfigMap[ mCurrentTrash ].percent );
  mPercent->setPrecision(3);
  mLimitReachedAction->setCurrentItem( mConfigMap[ mCurrentTrash ].actionType );
  percentChanged(mPercent->value());

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
  connect( mPercent, TQT_SIGNAL( valueChanged( double ) ),
           this, TQT_SLOT( percentChanged( double ) ) );
  connect( mPercent, TQT_SIGNAL( valueChanged( double ) ),
           this, TQT_SLOT( setDirty() ) );
  connect( mLimitReachedAction, TQT_SIGNAL( activated( int ) ),
           this, TQT_SLOT( setDirty() ) );

 trashChanged( 0 );
}

KTrashPropsDlgPlugin::~KTrashPropsDlgPlugin()
{
}

void KTrashPropsDlgPlugin::applyChanges()
{
  if ( !mCurrentTrash.isEmpty() ) {
    ConfigEntry entry;
    entry.useTimeLimit = mUseTimeLimit->isChecked();
    entry.days = mDays->value();
    entry.useSizeLimit = mUseSizeLimit->isChecked();
    entry.percent = mPercent->value(),
    entry.actionType = mLimitReachedAction->currentItem();
    mConfigMap.insert( mCurrentTrash, entry );
  }

  writeConfig();
}

void KTrashPropsDlgPlugin::percentChanged( double percent )
{
  DiscSpaceUtil util( mCurrentTrash );

  double partitionSize = util.size() * 1024.0; // convert to byte

  double size = partitionSize*(percent/100.0);

	// FIXME It would be good if precision step depended on max HD size
  mPercent->setPrecision(3);

  TQString unit = i18n( "Byte" );
  if ( size > 1024 ) {
    unit = i18n( "KByte" );
    size = size/1024.0;
    mPercent->setLineStep(0.001);
  }
  if ( size > 1024 ) {
    unit = i18n( "MByte" );
    size = size/1024.0;
    mPercent->setLineStep(0.01);
  }
  if ( size > 1024 ) {
    unit = i18n( "GByte" );
    size = size/1024.0;
    mPercent->setLineStep(0.1);
  }
  if ( size > 1024 ) {
    unit = i18n( "TByte" );
    size = size/1024.0;
    mPercent->setLineStep(1);
  }

  mSizeLabel->setText( i18n( "(%1 %2)" ).arg( TQString::number( size, 'f', 2 ) ).arg( unit ) );

}

void KTrashPropsDlgPlugin::trashChanged( int value )
{
  const TrashImpl::TrashDirMap map = mTrashImpl->trashDirectories();

  if ( !mCurrentTrash.isEmpty() ) {
    ConfigEntry entry;
    entry.useTimeLimit = mUseTimeLimit->isChecked();
    entry.days = mDays->value();
    entry.useSizeLimit = mUseSizeLimit->isChecked();
    entry.percent = mPercent->value(),
    entry.actionType = mLimitReachedAction->currentItem();
    mConfigMap.insert( mCurrentTrash, entry );
  }

  mCurrentTrash = map[ value ];

  if ( mConfigMap.contains( mCurrentTrash ) ) {
    const ConfigEntry entry = mConfigMap[ mCurrentTrash ];
    mUseTimeLimit->setChecked( entry.useTimeLimit );
    mDays->setValue( entry.days );
    mUseSizeLimit->setChecked( entry.useSizeLimit );
    mPercent->setValue( entry.percent );
    mLimitReachedAction->setCurrentItem( entry.actionType );
  } else {
    mUseTimeLimit->setChecked( false );
    mDays->setValue( 7 );
    mUseSizeLimit->setChecked( true );
    mPercent->setValue( 10.0 );
    mLimitReachedAction->setCurrentItem( 0 );
  }

  percentChanged( mPercent->value() );
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
      entry.days = config.readNumEntry( "Days", 7 );
      entry.useSizeLimit = config.readBoolEntry( "UseSizeLimit", true );
      entry.percent = config.readDoubleNumEntry( "Percent", 10 );
      entry.actionType = config.readNumEntry( "LimitReachedAction", 0 );
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
    config.writeEntry( "Percent", it.data().percent );
    config.writeEntry( "LimitReachedAction", it.data().actionType );
  }

  config.sync();
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

  TQGridLayout *sizeWidgetLayout = new TQGridLayout( mSizeWidget, 2, 3, 11, 6 );

  TQLabel *label = new TQLabel( i18n( "Maximum Size:" ), mSizeWidget );
  sizeWidgetLayout->addWidget( label, 0, 0 );

  mPercent = new KDoubleSpinBox( 0.001, 100, 1, 10, 3, mSizeWidget );
  mPercent->setSuffix( " %" );
  sizeWidgetLayout->addWidget( mPercent, 0, 1 );

  mSizeLabel = new TQLabel( mSizeWidget );
  sizeWidgetLayout->addWidget( mSizeLabel, 0, 2 );

  label = new TQLabel( i18n( "When limit reached:" ), mSizeWidget );
  sizeWidgetLayout->addWidget( label, 1, 0 );

  mLimitReachedAction = new TQComboBox( mSizeWidget );
  mLimitReachedAction->insertItem( i18n( "Warn me" ) );
  mLimitReachedAction->insertItem( i18n( "Delete oldest files from trash" ) );
  mLimitReachedAction->insertItem( i18n( "Delete biggest files from trash" ) );
  sizeWidgetLayout->addMultiCellWidget( mLimitReachedAction, 1, 1, 1, 2 );

  layout->addStretch();

}

#include "ktrashpropsdlgplugin.moc"
