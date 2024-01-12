/*
 *  userInterOpts.cpp
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

#include <tdeapplication.h>
#include <dcopclient.h>
#include <kcolorbutton.h>
#include <tdelocale.h>
#include <tdeconfig.h>

#include "main.h"
#include "userInterOpts_impl.h"
#include "userInterOpts_impl.moc"

userInterOpts::userInterOpts(TDEConfig *config, TQString groupName,
                             TQWidget* parent, const char* name)
    : userInterOptsBase(parent, name), m_pConfig(config), m_groupName(groupName)
{
    // connections
    connect(m_pShowMMBInTabs, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pDynamicTabbarHide, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pDynamicTabbarCycle, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pNewTabsInBackground, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pOpenAfterCurrentPage, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pTabConfirm, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pPermanentCloseButton, TQ_SIGNAL(toggled(bool)),  TQ_SLOT(slotChanged()));
    connect(m_pHoverCloseButton, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pKonquerorTabforExternalURL, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pPopupsWithinTabs, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
    connect(m_pTabCloseActivatePrevious, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChanged()));
}

void userInterOpts::load()
{
  load(false);
}

void userInterOpts::load(bool useDefaults)
{
    m_pConfig->setReadDefaults(useDefaults);
    m_pConfig->setGroup(m_groupName);
    
    m_pShowMMBInTabs->setChecked( m_pConfig->readBoolEntry( "MMBOpensTab", false ) );
    m_pDynamicTabbarHide->setChecked( !(m_pConfig->readBoolEntry( "AlwaysTabbedMode", false )) );
    m_pDynamicTabbarCycle->setChecked( m_pConfig->readBoolEntry( "TabsCycleWheel", true ) );
    m_pNewTabsInBackground->setChecked( ! (m_pConfig->readBoolEntry( "NewTabsInFront", false )) );
    m_pOpenAfterCurrentPage->setChecked( m_pConfig->readBoolEntry( "OpenAfterCurrentPage", false ) );
    m_pPermanentCloseButton->setChecked( m_pConfig->readBoolEntry( "PermanentCloseButton", false ) );
    m_pHoverCloseButton->setChecked( m_pConfig->readBoolEntry( "HoverCloseButton", false ) );
    m_pKonquerorTabforExternalURL->setChecked( m_pConfig->readBoolEntry( "KonquerorTabforExternalURL", false ) );
    m_pPopupsWithinTabs->setChecked( m_pConfig->readBoolEntry( "PopupsWithinTabs", false ) );
    m_pTabCloseActivatePrevious->setChecked( m_pConfig->readBoolEntry( "TabCloseActivatePrevious", false ) );

    m_pConfig->setGroup("Notification Messages");
    m_pTabConfirm->setChecked( !m_pConfig->hasKey("MultipleTabConfirm") );

    if ( m_pPermanentCloseButton->isChecked() )
      m_pHoverCloseButton->setEnabled(false);
    else
      m_pHoverCloseButton->setEnabled(true);
}

void userInterOpts::save()
{
    m_pConfig->setGroup(m_groupName);
    
    m_pConfig->writeEntry( "MMBOpensTab", (m_pShowMMBInTabs->isChecked()) );
    m_pConfig->writeEntry( "AlwaysTabbedMode", ( !(m_pDynamicTabbarHide->isChecked())) );
    m_pConfig->writeEntry( "TabsCycleWheel", (m_pDynamicTabbarCycle->isChecked()) );
    m_pConfig->writeEntry( "NewTabsInFront", !(m_pNewTabsInBackground->isChecked()) );
    m_pConfig->writeEntry( "OpenAfterCurrentPage", m_pOpenAfterCurrentPage->isChecked() );
    m_pConfig->writeEntry( "PermanentCloseButton", m_pPermanentCloseButton->isChecked() );
    m_pConfig->writeEntry( "HoverCloseButton", m_pHoverCloseButton->isChecked() );
    m_pConfig->writeEntry( "KonquerorTabforExternalURL", m_pKonquerorTabforExternalURL->isChecked() );
    m_pConfig->writeEntry( "PopupsWithinTabs", m_pPopupsWithinTabs->isChecked() );
    m_pConfig->writeEntry( "TabCloseActivatePrevious", m_pTabCloseActivatePrevious->isChecked() );
    m_pConfig->sync();

    // It only matters whether the key is present, its value has no meaning
    m_pConfig->setGroup("Notification Messages");
    if ( m_pTabConfirm->isChecked() ) m_pConfig->deleteEntry( "MultipleTabConfirm" );
    else m_pConfig->writeEntry( "MultipleTabConfirm", true );

    TQByteArray data;
    if ( !TDEApplication::kApplication()->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
    TDEApplication::kApplication()->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

    if ( m_pPermanentCloseButton->isChecked() )
      m_pHoverCloseButton->setEnabled(false);
    else
      m_pHoverCloseButton->setEnabled(true);
}

void userInterOpts::defaults()
{
   load(true);
}

void userInterOpts::slotChanged()
{
    emit changed();
}


