/*
 *  advancedTabDialog.cpp
 *
 *  Copyright (c) 2002 Aaron J. Seigo <aseigo@olympusproject.org>
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

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>
#include <tqslider.h>

#include <kapplication.h>
#include <dcopclient.h>
#include <kcolorbutton.h>
#include <klocale.h>
#include <kconfig.h>

#include "advancedTabDialog.h"
#include "advancedTabOptions.h"
#include "main.h"

advancedTabDialog::advancedTabDialog(TQWidget* parent, KConfig* config, const char* name)
    : KDialogBase(KDialogBase::Plain,
                  i18n("Advanced Options"),
                  KDialogBase::Ok |
                  KDialogBase::Apply |
                  KDialogBase::Cancel,
                  KDialogBase::Ok,
                  parent,
                  name,
                  true, true),
                  m_pConfig(config)
{
    connect(this, TQT_SIGNAL(applyClicked()),
            this, TQT_SLOT(save()));
    connect(this, TQT_SIGNAL(okClicked()),
            this, TQT_SLOT(save()));
    actionButton(Apply)->setEnabled(false);
    TQFrame* page = plainPage();
    TQVBoxLayout* layout = new TQVBoxLayout(page);
    m_advancedWidget = new advancedTabOptions(page);
    layout->addWidget(m_advancedWidget);
    layout->addSpacing( 20 );
    layout->addStretch();

    connect(m_advancedWidget->m_pNewTabsInBackground, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
    connect(m_advancedWidget->m_pOpenAfterCurrentPage, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
    connect(m_advancedWidget->m_pTabConfirm, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
    connect(m_advancedWidget->m_pTabCloseActivatePrevious, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
    connect(m_advancedWidget->m_pPermanentCloseButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
    connect(m_advancedWidget->m_pHoverCloseButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
    connect(m_advancedWidget->m_pKonquerorTabforExternalURL, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
    connect(m_advancedWidget->m_pPopupsWithinTabs, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));

    load();
}

advancedTabDialog::~advancedTabDialog()
{
}

void advancedTabDialog::load()
{
    m_pConfig->setGroup("FMSettings");
    m_advancedWidget->m_pNewTabsInBackground->setChecked( ! (m_pConfig->readBoolEntry( "NewTabsInFront", false )) );
    m_advancedWidget->m_pOpenAfterCurrentPage->setChecked( m_pConfig->readBoolEntry( "OpenAfterCurrentPage", false ) );
    m_advancedWidget->m_pPermanentCloseButton->setChecked( m_pConfig->readBoolEntry( "PermanentCloseButton", false ) );
    m_advancedWidget->m_pHoverCloseButton->setChecked( m_pConfig->readBoolEntry( "HoverCloseButton", false ) );
    m_advancedWidget->m_pKonquerorTabforExternalURL->setChecked( m_pConfig->readBoolEntry( "KonquerorTabforExternalURL", false ) );
    m_advancedWidget->m_pPopupsWithinTabs->setChecked( m_pConfig->readBoolEntry( "PopupsWithinTabs", false ) );
    m_advancedWidget->m_pTabCloseActivatePrevious->setChecked( m_pConfig->readBoolEntry( "TabCloseActivatePrevious", false ) );

    m_pConfig->setGroup("Notification Messages");
    m_advancedWidget->m_pTabConfirm->setChecked( !m_pConfig->hasKey("MultipleTabConfirm") );

    if ( m_advancedWidget->m_pPermanentCloseButton->isChecked() )
      m_advancedWidget->m_pHoverCloseButton->setEnabled(false);
    else
      m_advancedWidget->m_pHoverCloseButton->setEnabled(true);
    actionButton(Apply)->setEnabled(false);
}

void advancedTabDialog::save()
{
    m_pConfig->setGroup("FMSettings");
    m_pConfig->writeEntry( "NewTabsInFront", !(m_advancedWidget->m_pNewTabsInBackground->isChecked()) );
    m_pConfig->writeEntry( "OpenAfterCurrentPage", m_advancedWidget->m_pOpenAfterCurrentPage->isChecked() );
    m_pConfig->writeEntry( "PermanentCloseButton", m_advancedWidget->m_pPermanentCloseButton->isChecked() );
    m_pConfig->writeEntry( "HoverCloseButton", m_advancedWidget->m_pHoverCloseButton->isChecked() );
    m_pConfig->writeEntry( "KonquerorTabforExternalURL", m_advancedWidget->m_pKonquerorTabforExternalURL->isChecked() );
    m_pConfig->writeEntry( "PopupsWithinTabs", m_advancedWidget->m_pPopupsWithinTabs->isChecked() );
    m_pConfig->writeEntry( "TabCloseActivatePrevious", m_advancedWidget->m_pTabCloseActivatePrevious->isChecked() );
    m_pConfig->sync();

    // It only matters whether the key is present, its value has no meaning
    m_pConfig->setGroup("Notification Messages");
    if ( m_advancedWidget->m_pTabConfirm->isChecked() ) m_pConfig->deleteEntry( "MultipleTabConfirm" );
    else m_pConfig->writeEntry( "MultipleTabConfirm", true );

    TQByteArray data;
    if ( !KApplication::kApplication()->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
    KApplication::kApplication()->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

    if ( m_advancedWidget->m_pPermanentCloseButton->isChecked() )
      m_advancedWidget->m_pHoverCloseButton->setEnabled(false);
    else
      m_advancedWidget->m_pHoverCloseButton->setEnabled(true);
    actionButton(Apply)->setEnabled(false);
}

void advancedTabDialog::changed()
{
    if ( m_advancedWidget->m_pPermanentCloseButton->isChecked() )
      m_advancedWidget->m_pHoverCloseButton->setEnabled(false);
    else
      m_advancedWidget->m_pHoverCloseButton->setEnabled(true);
    actionButton(Apply)->setEnabled(true);
}

#include "advancedTabDialog.moc"
