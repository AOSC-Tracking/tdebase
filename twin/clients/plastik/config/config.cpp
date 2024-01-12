/* Plastik KWin window decoration
  Copyright (C) 2003 Sandro Giessl <ceebx@users.sourceforge.net>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
 */

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqradiobutton.h>
#include <tqslider.h>
#include <tqspinbox.h>
#include <tqwhatsthis.h>

#include <tdeconfig.h>
#include <tdelocale.h>
#include <tdeglobal.h>

#include "config.h"
#include "configdialog.h"

PlastikConfig::PlastikConfig(TDEConfig* config, TQWidget* parent)
    : TQObject(parent), m_config(0), m_dialog(0)
{
    // create the configuration object
    m_config = new TDEConfig("twinplastikrc");
    TDEGlobal::locale()->insertCatalogue("twin_clients");

    // create and show the configuration dialog
    m_dialog = new ConfigDialog(parent);
    m_dialog->show();

    // load the configuration
    load(config);

    // setup the connections
    connect(m_dialog->titleAlign, TQ_SIGNAL(clicked(int)),
            this, TQ_SIGNAL(changed()));
    connect(m_dialog->animateButtons, TQ_SIGNAL(toggled(bool)),
            this, TQ_SIGNAL(changed()));
    connect(m_dialog->menuClose, TQ_SIGNAL(toggled(bool)),
            this, TQ_SIGNAL(changed()));
    connect(m_dialog->titleShadow, TQ_SIGNAL(toggled(bool)),
            this, TQ_SIGNAL(changed()));
    connect(m_dialog->coloredBorder, TQ_SIGNAL(toggled(bool)),
            this, TQ_SIGNAL(changed()));
}

PlastikConfig::~PlastikConfig()
{
    if (m_dialog) delete m_dialog;
    if (m_config) delete m_config;
}

void PlastikConfig::load(TDEConfig*)
{
    m_config->setGroup("General");


    TQString value = m_config->readEntry("TitleAlignment", "AlignLeft");
    TQRadioButton *button = (TQRadioButton*)m_dialog->titleAlign->child(value.latin1());
    if (button) button->setChecked(true);
    bool animateButtons = m_config->readBoolEntry("AnimateButtons", true);
    m_dialog->animateButtons->setChecked(animateButtons);
    bool menuClose = m_config->readBoolEntry("CloseOnMenuDoubleClick", true);
    m_dialog->menuClose->setChecked(menuClose);
    bool titleShadow = m_config->readBoolEntry("TitleShadow", true);
    m_dialog->titleShadow->setChecked(titleShadow);
    bool coloredBorder = m_config->readBoolEntry("ColoredBorder", true);
    m_dialog->coloredBorder->setChecked(coloredBorder);
}

void PlastikConfig::save(TDEConfig*)
{
    m_config->setGroup("General");

    TQRadioButton *button = (TQRadioButton*)m_dialog->titleAlign->selected();
    if (button) m_config->writeEntry("TitleAlignment", TQString(button->name()));
    m_config->writeEntry("AnimateButtons", m_dialog->animateButtons->isChecked() );
    m_config->writeEntry("CloseOnMenuDoubleClick", m_dialog->menuClose->isChecked() );
    m_config->writeEntry("TitleShadow", m_dialog->titleShadow->isChecked() );
    m_config->writeEntry("ColoredBorder", m_dialog->coloredBorder->isChecked() );
    m_config->sync();
}

void PlastikConfig::defaults()
{
    TQRadioButton *button =
        (TQRadioButton*)m_dialog->titleAlign->child("AlignLeft");
    if (button) button->setChecked(true);
    m_dialog->animateButtons->setChecked(true);
    m_dialog->menuClose->setChecked(false);
    m_dialog->titleShadow->setChecked(true);
    m_dialog->coloredBorder->setChecked(true);
}

//////////////////////////////////////////////////////////////////////////////
// Plugin Stuff                                                             //
//////////////////////////////////////////////////////////////////////////////

extern "C"
{
    KDE_EXPORT TQObject* allocate_config(TDEConfig* config, TQWidget* parent) {
        return (new PlastikConfig(config, parent));
    }
}

#include "config.moc"
