/*
 *  advancedDialog.cpp
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

#include <kcolorbutton.h>
#include <tdelocale.h>

#include "advancedDialog.h"
#include "advancedOptions.h"
#include "main.h"

advancedDialog::advancedDialog(TQWidget* parent, const char* name)
    : KDialogBase(KDialogBase::Plain,
                  i18n("Advanced Options"),
                  Ok|Apply|Cancel,
                  Cancel,
                  parent,
                  name,
                  false, false)
{
    connect(this, TQ_SIGNAL(applyClicked()),
            this, TQ_SLOT(save()));
    connect(this, TQ_SIGNAL(okClicked()),
            this, TQ_SLOT(save()));

    TQFrame* page = plainPage();
    TQVBoxLayout* layout = new TQVBoxLayout(page);
    m_advancedWidget = new advancedKickerOptions(page);
    layout->addWidget(m_advancedWidget);
    layout->addStretch();

    setMinimumSize( sizeHint() );

    connect(m_advancedWidget->handles, TQ_SIGNAL(clicked(int)),
            this, TQ_SLOT(changed()));
    connect(m_advancedWidget->hideButtonSize, TQ_SIGNAL(valueChanged(int)),
            this, TQ_SLOT(changed()));
    connect(m_advancedWidget->tintColorB, TQ_SIGNAL(clicked()),
            this, TQ_SLOT(changed()));
    connect(m_advancedWidget->tintSlider, TQ_SIGNAL(valueChanged(int)),
            this, TQ_SLOT(changed()));
    connect(m_advancedWidget->blurSlider, TQ_SIGNAL(valueChanged(int)),
            this, TQ_SLOT(changed()));
    connect(m_advancedWidget->menubarPanelTransparent, TQ_SIGNAL(clicked()),
            this, TQ_SLOT(changed()));
    connect(m_advancedWidget->kickerResizeHandle, TQ_SIGNAL(clicked()),
            this, TQ_SLOT(changed()));
    connect(m_advancedWidget->kickerDeepButtons, TQ_SIGNAL(clicked()),
            this, TQ_SLOT(changed()));
    load();
}

advancedDialog::~advancedDialog()
{
}

void advancedDialog::load()
{
    TDEConfig c(KickerConfig::the()->configName(), false, false);
    c.setGroup("General");

    bool fadedOut = c.readBoolEntry("FadeOutAppletHandles", true);
    bool hideHandles = c.readBoolEntry("HideAppletHandles", false);
    if (hideHandles)
        m_advancedWidget->hideHandles->setChecked(true);
    else if (fadedOut)
        m_advancedWidget->fadeOutHandles->setChecked(true);
    else
        m_advancedWidget->visibleHandles->setChecked(true);

    int defaultHideButtonSize = c.readNumEntry("HideButtonSize", 14);
    m_advancedWidget->hideButtonSize->setValue(defaultHideButtonSize);
    TQColor color = c.readColorEntry( "TintColor", &colorGroup().mid() );
    m_advancedWidget->tintColorB->setColor( color );
    int tintValue = c.readNumEntry( "TintValue", 33 );
    m_advancedWidget->tintSlider->setValue( tintValue );

    // Compatibility with deprecated MenubarPanelBlurred option
    int blurValue = 0;
    if (c.hasKey("MenubarPanelBlurred"))
    {
        if (c.readNumEntry("MenubarPanelBlurred", false))
        {
            blurValue = 4;
        }
        c.deleteEntry("MenubarPanelBlurred");
    }
    blurValue = c.readNumEntry("BlurValue", blurValue);
    m_advancedWidget->blurSlider->setValue(blurValue);

    bool transparentMenubarPanel = c.readBoolEntry("MenubarPanelTransparent", false);
    m_advancedWidget->menubarPanelTransparent->setChecked( transparentMenubarPanel );

    bool useKickerResizeHandle = c.readBoolEntry("UseResizeHandle", false);
    m_advancedWidget->kickerResizeHandle->setChecked( useKickerResizeHandle );
    bool usekickerDeepButtons = c.readBoolEntry("ShowDeepButtons", false);
    m_advancedWidget->kickerDeepButtons->setChecked( usekickerDeepButtons );

    enableButtonApply(false);
}

void advancedDialog::save()
{
    TDEConfig c(KickerConfig::the()->configName(), false, false);

    c.setGroup("General");
    c.writeEntry("FadeOutAppletHandles",
                 m_advancedWidget->fadeOutHandles->isChecked());
    c.writeEntry("HideAppletHandles",
                 m_advancedWidget->hideHandles->isChecked());
    c.writeEntry("HideButtonSize",
                 m_advancedWidget->hideButtonSize->value());
    c.writeEntry("TintColor",
                 m_advancedWidget->tintColorB->color());
    c.writeEntry("TintValue",
                 m_advancedWidget->tintSlider->value());
    c.writeEntry("BlurValue",
                 m_advancedWidget->blurSlider->value());
    c.writeEntry("MenubarPanelTransparent",
                 m_advancedWidget->menubarPanelTransparent->isChecked());
    c.writeEntry("UseResizeHandle",
                 m_advancedWidget->kickerResizeHandle->isChecked());
    c.writeEntry("ShowDeepButtons",
                 m_advancedWidget->kickerDeepButtons->isChecked());

    TQStringList elist = c.readListEntry("Extensions2");
    for (TQStringList::Iterator it = elist.begin(); it != elist.end(); ++it)
    {
        // extension id
        TQString group(*it);

        // is there a config group for this extension?
        if(!c.hasGroup(group) ||
           group.contains("Extension") < 1)
        {
            continue;
        }

        // set config group
        c.setGroup(group);
        TDEConfig extConfig(c.readEntry("ConfigFile"));
        extConfig.setGroup("General");
        extConfig.writeEntry("FadeOutAppletHandles",
                             m_advancedWidget->fadeOutHandles->isChecked());
        extConfig.writeEntry("HideAppletHandles",
                             m_advancedWidget->hideHandles->isChecked());
        extConfig.writeEntry("HideButtonSize",
                             m_advancedWidget->hideButtonSize->value());
        extConfig.writeEntry("TintColor",
                             m_advancedWidget->tintColorB->color());
        extConfig.writeEntry("TintValue",
                             m_advancedWidget->tintSlider->value());
        extConfig.writeEntry("BlurValue",
                             m_advancedWidget->blurSlider->value());
        extConfig.writeEntry("MenubarPanelTransparent",
                             m_advancedWidget->menubarPanelTransparent->isChecked());
        extConfig.writeEntry("UseResizeHandle",
                             m_advancedWidget->kickerResizeHandle->isChecked());
        extConfig.writeEntry("ShowDeepButtons",
                             m_advancedWidget->kickerDeepButtons->isChecked());

        extConfig.sync();
    }

    c.sync();

    KickerConfig::the()->notifyKicker();
    enableButtonApply(false);
}

void advancedDialog::changed()
{
    enableButtonApply(true);
}

#include "advancedDialog.moc"

