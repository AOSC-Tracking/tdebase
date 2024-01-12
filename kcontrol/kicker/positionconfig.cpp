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

#include "positiontab_impl.h"
#include "kickerSettings.h"
#include "main.h"

#include "positionconfig.h"
#include "positionconfig.moc"

PositionConfig::PositionConfig(TQWidget *parent, const char *name)
  : TDECModule(parent, name)
{
    TQVBoxLayout *layout = new TQVBoxLayout(this);
    m_widget = new PositionTab(this);
    layout->addWidget(m_widget);
    layout->addStretch();

    setQuickHelp(KickerConfig::the()->quickHelp());
    setAboutData(KickerConfig::the()->aboutData());

    //addConfig(KickerSettings::self(), m_widget);

    connect(m_widget, TQ_SIGNAL(changed()),
            this, TQ_SLOT(changed()));
    connect(KickerConfig::the(), TQ_SIGNAL(aboutToNotifyKicker()),
            this, TQ_SLOT(aboutToNotifyKicker()));

    load();
    TQTimer::singleShot(0, this, TQ_SLOT(notChanged()));
}

void PositionConfig::notChanged()
{
    emit changed(false);
}

void PositionConfig::load()
{
    m_widget->load();
    TDECModule::load();
}

void PositionConfig::aboutToNotifyKicker()
{
    kdDebug() << "PositionConfig::aboutToNotifyKicker()" << endl;

    // This slot is triggered by the signal,
    // which is send before Kicker is notified.
    // See comment in save().
    m_widget->save();
    TDECModule::save();
}

void PositionConfig::save()
{
    // As we don't want to notify Kicker multiple times
    // we do not save the settings here. Instead the
    // KickerConfig object sends a signal before the
    // notification. On this signal all existing modules,
    // including this object, save their settings.
    KickerConfig::the()->notifyKicker();
}

void PositionConfig::defaults()
{
    m_widget->defaults();
    TDECModule::defaults();

    // TDEConfigDialogManager may queue an changed(false) signal,
    // so we make sure, that the module is labeled as changed,
    // while we manage some of the widgets ourselves
    TQTimer::singleShot(0, this, TQ_SLOT(changed()));
}

TQString PositionConfig::handbookSection() const
{
    return "panel-arrangement";
}
