/*
 *  Copyright (c) 2005      Stefan Nikolaus <stefan.nikolaus@kdemail.net>
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

#include <tqlayout.h>
#include <tqtimer.h>

#include <tdelocale.h>
#include <kdebug.h>

#include "userInterOpts_impl.h"
#include "main.h"

#include "userInterConfig.h"
#include "userInterConfig.moc"

userInterConfig::userInterConfig(TDEConfig *config, TQString groupName,
                                 TQWidget *parent, const char *name)
  : TDECModule(parent, "kcmkonqhtml")
{
    TQVBoxLayout *layout = new TQVBoxLayout(this);
    m_widget = new userInterOpts(config, groupName, this, name);
    layout->addWidget(m_widget);
    layout->addStretch();

    connect(m_widget, TQ_SIGNAL(changed()),
            this, TQ_SLOT(changed()));

    load();
    TQTimer::singleShot(0, this, TQ_SLOT(notChanged()));
}

void userInterConfig::notChanged()
{
    emit changed(false);
}

void userInterConfig::load()
{
    m_widget->load();
    TDECModule::load();
}

void userInterConfig::save()
{
    m_widget->save();
    TDECModule::save();
}

void userInterConfig::defaults()
{
    m_widget->defaults();
    TDECModule::defaults();

    // TDEConfigDialogManager may queue an changed(false) signal,
    // so we make sure, that the module is labeled as changed,
    // while we manage some of the widgets ourselves
    TQTimer::singleShot(0, this, TQ_SLOT(changed()));
}
