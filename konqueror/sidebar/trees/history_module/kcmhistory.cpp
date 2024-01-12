/*
 *  kcmhistory.cpp
 *  Copyright (c) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>
 *  Copyright (c) 2002 Stephan Binner <binner@kde.org>
 *
 *  based on kcmtaskbar.cpp
 *  Copyright (c) 2000 Kurt Granroth <granroth@kde.org>
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
 */

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>

#include <dcopclient.h>
#include <dcopref.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdialog.h>
#include <tdefontdialog.h>
#include <kgenericfactory.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <knuminput.h>
#include "history_dlg.h"

#include "konq_historymgr.h"

#include "kcmhistory.h"
#include "history_settings.h"

typedef KGenericFactory<HistorySidebarConfig, TQWidget > KCMHistoryFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_history, KCMHistoryFactory("kcmhistory") )

HistorySidebarConfig::HistorySidebarConfig( TQWidget *parent, const char* name, const TQStringList & )
    : TDECModule (KCMHistoryFactory::instance(), parent, name)
{
    TDEGlobal::locale()->insertCatalogue("konqueror");

    m_settings = new KonqSidebarHistorySettings( 0, "history settings" );
    m_settings->readSettings( false );

    TQVBoxLayout *topLayout = new TQVBoxLayout(this, 0, KDialog::spacingHint());
    dialog = new KonqSidebarHistoryDlg(this);

    dialog->spinEntries->setRange( 0, INT_MAX, 1, false );
    dialog->spinExpire->setRange(  0, INT_MAX, 1, false );

    dialog->spinNewer->setRange( 0, INT_MAX, 1, false );
    dialog->spinOlder->setRange( 0, INT_MAX, 1, false );

    dialog->comboNewer->insertItem( i18n("Minutes"),
                                    KonqSidebarHistorySettings::MINUTES );
    dialog->comboNewer->insertItem( i18n("Days"),
                                    KonqSidebarHistorySettings::DAYS );

    dialog->comboOlder->insertItem( i18n("Minutes"),
                                    KonqSidebarHistorySettings::MINUTES );
    dialog->comboOlder->insertItem( i18n("Days"),
                                    KonqSidebarHistorySettings::DAYS );

    connect( dialog->cbExpire, TQ_SIGNAL( toggled( bool )),
	     dialog->spinExpire, TQ_SLOT( setEnabled( bool )));
    connect( dialog->spinExpire, TQ_SIGNAL( valueChanged( int )),
	     this, TQ_SLOT( slotExpireChanged( int )));

    connect( dialog->spinNewer, TQ_SIGNAL( valueChanged( int )),
	     TQ_SLOT( slotNewerChanged( int )));
    connect( dialog->spinOlder, TQ_SIGNAL( valueChanged( int )),
	     TQ_SLOT( slotOlderChanged( int )));

    connect( dialog->btnFontNewer, TQ_SIGNAL( clicked() ),
             TQ_SLOT( slotGetFontNewer() ));
    connect( dialog->btnFontOlder, TQ_SIGNAL( clicked() ),
             TQ_SLOT( slotGetFontOlder() ));
    connect( dialog->btnClearHistory, TQ_SIGNAL( clicked() ),
             TQ_SLOT( slotClearHistory() ));

    connect( dialog->cbDetailedTips, TQ_SIGNAL( toggled( bool )),
             TQ_SLOT( configChanged() ));
    connect( dialog->cbExpire, TQ_SIGNAL( toggled( bool )),
             TQ_SLOT( configChanged() ));
    connect( dialog->spinEntries, TQ_SIGNAL( valueChanged( int )),
             TQ_SLOT( configChanged() ));
    connect( dialog->comboNewer, TQ_SIGNAL( activated( int )),
             TQ_SLOT( configChanged() ));
    connect( dialog->comboOlder, TQ_SIGNAL( activated( int )),
             TQ_SLOT( configChanged() ));

    dialog->show();
    topLayout->add(dialog);
    load();
}

void HistorySidebarConfig::configChanged()
{
    emit changed(true);
}

void HistorySidebarConfig::load()
{
    TDEConfig config("konquerorrc");
    config.setGroup("HistorySettings");
    dialog->spinExpire->setValue( config.readNumEntry( "Maximum age of History entries", 90) );
    dialog->spinEntries->setValue( config.readNumEntry( "Maximum of History entries", 500 ) );
    dialog->cbExpire->setChecked( dialog->spinExpire->value() > 0 );

    dialog->spinNewer->setValue( m_settings->m_valueYoungerThan );
    dialog->spinOlder->setValue( m_settings->m_valueOlderThan );

    dialog->comboNewer->setCurrentItem( m_settings->m_metricYoungerThan );
    dialog->comboOlder->setCurrentItem( m_settings->m_metricOlderThan );

    dialog->cbDetailedTips->setChecked( m_settings->m_detailedTips );

    m_fontNewer = m_settings->m_fontYoungerThan;
    m_fontOlder = m_settings->m_fontOlderThan;

    // enable/disable widgets
    dialog->spinExpire->setEnabled( dialog->cbExpire->isChecked() );

    slotExpireChanged( dialog->spinExpire->value() );
    slotNewerChanged( dialog->spinNewer->value() );
    slotOlderChanged( dialog->spinOlder->value() );

    emit changed(false);
}

void HistorySidebarConfig::save()
{
    TQ_UINT32 age   = dialog->cbExpire->isChecked() ? dialog->spinExpire->value() : 0;
    TQ_UINT32 count = dialog->spinEntries->value();

    TDEConfig config("konquerorrc");
    config.setGroup("HistorySettings");
    config.writeEntry( "Maximum of History entries", count );
    config.writeEntry( "Maximum age of History entries", age );

    TQByteArray dataAge;
    TQDataStream streamAge( dataAge, IO_WriteOnly );
    streamAge << age << "foo";
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxAge(TQ_UINT32, TQCString)", dataAge );

    TQByteArray dataCount;
    TQDataStream streamCount( dataCount, IO_WriteOnly );
    streamCount << count << "foo";
    kapp->dcopClient()->send( "konqueror*", "KonqHistoryManager",
			      "notifyMaxCount(TQ_UINT32, TQCString)", dataCount );

    m_settings->m_valueYoungerThan = dialog->spinNewer->value();
    m_settings->m_valueOlderThan   = dialog->spinOlder->value();

    m_settings->m_metricYoungerThan = dialog->comboNewer->currentItem();
    m_settings->m_metricOlderThan   = dialog->comboOlder->currentItem();

    m_settings->m_detailedTips = dialog->cbDetailedTips->isChecked();

    m_settings->m_fontYoungerThan = m_fontNewer;
    m_settings->m_fontOlderThan   = m_fontOlder;

    m_settings->applySettings();

    emit changed(false);
}

void HistorySidebarConfig::defaults()
{
    dialog->spinEntries->setValue( 500 );
    dialog->cbExpire->setChecked( true );
    dialog->spinExpire->setValue( 90 );

    dialog->spinNewer->setValue( 1 );
    dialog->spinOlder->setValue( 2 );

    dialog->comboNewer->setCurrentItem( KonqSidebarHistorySettings::DAYS );
    dialog->comboOlder->setCurrentItem( KonqSidebarHistorySettings::DAYS );

    dialog->cbDetailedTips->setChecked( true );

    m_fontNewer = TQFont();
    m_fontNewer.setItalic( true );
    m_fontOlder = TQFont();

    emit changed(true);
}

TQString HistorySidebarConfig::quickHelp() const
{
    return i18n("<h1>History Sidebar</h1>"
                " You can configure the history sidebar here.");
}

void HistorySidebarConfig::slotExpireChanged( int value )
{
    dialog->spinExpire->setSuffix( i18n(" day", " days", value) );
    configChanged();
}

// change hour to days, minute to minutes and the other way round,
// depending on the value of the spinbox, and synchronize the two spinBoxes
// to enfore newer <= older.
void HistorySidebarConfig::slotNewerChanged( int value )
{
    dialog->comboNewer->changeItem( i18n ( "Day", "Days", value),
                                    KonqSidebarHistorySettings::DAYS);
    dialog->comboNewer->changeItem( i18n ( "Minute", "Minutes", value),
                                    KonqSidebarHistorySettings::MINUTES);

    if ( dialog->spinNewer->value() > dialog->spinOlder->value() )
	dialog->spinOlder->setValue( dialog->spinNewer->value() );
    configChanged();
}

void HistorySidebarConfig::slotOlderChanged( int value )
{
    dialog->comboOlder->changeItem( i18n ( "Day", "Days", value),
                                    KonqSidebarHistorySettings::DAYS);
    dialog->comboOlder->changeItem( i18n ( "Minute", "Minutes", value),
                                    KonqSidebarHistorySettings::MINUTES);

    if ( dialog->spinNewer->value() > dialog->spinOlder->value() )
	dialog->spinNewer->setValue( dialog->spinOlder->value() );

    configChanged();
}

void HistorySidebarConfig::slotGetFontNewer()
{
    int result = TDEFontDialog::getFont( m_fontNewer, false, this );
    if ( result == TDEFontDialog::Accepted )
        configChanged();
}

void HistorySidebarConfig::slotGetFontOlder()
{
    int result = TDEFontDialog::getFont( m_fontOlder, false, this );
    if ( result == TDEFontDialog::Accepted )
        configChanged();
}

void HistorySidebarConfig::slotClearHistory()
{
    KGuiItem guiitem = KStdGuiItem::clear();
    guiitem.setIconSet( SmallIconSet("history_clear"));
    if ( KMessageBox::warningContinueCancel( this,
				     i18n("Do you really want to clear "
					  "the entire history?"),
				     i18n("Clear History?"), guiitem )
	 == KMessageBox::Continue ) {
        DCOPRef dcopHistManager( "konqueror*", "KonqHistoryManager" );
        dcopHistManager.send( "notifyClear", "KonqHistoryManager" );
    }
}

#include "kcmhistory.moc"
