/*
 *  Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "system.h"

#include <tdeconfig.h>
#include <tqwhatsthis.h>
#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tdelocale.h>

namespace KCMPerformance
{

SystemWidget::SystemWidget( TQWidget* parent_P )
    : System_ui( parent_P )
    {
    TQString tmp =
        i18n( "<p>During startup TDE needs to perform a check of its system configuration"
              " (mimetypes, installed applications, etc.), and in case the configuration"
              " has changed since the last time, the system configuration cache (TDESyCoCa)"
              " needs to be updated.</p>"
              "<p>This option delays the check, which avoid scanning all directories containing"
              " files describing the system during TDE startup, thus"
              " making TDE startup faster. However, in the rare case the system configuration"
              " has changed since the last time, and the change is needed before this"
              " delayed check takes place, this option may lead to various problems"
              " (missing applications in the TDE Menu, reports from applications about missing"
              " required mimetypes, etc.).</p>"
              "<p>Changes of system configuration mostly happen by (un)installing applications."
              " It is therefore recommended to turn this option temporarily off while"
              " (un)installing applications.</p>"
              "<p>For this reason, usage of this option is not recommended. The TDE crash"
              " handler will refuse to provide backtrace for the bugreport with this option"
              " turned on (you will need to reproduce it again with this option turned off,"
              " or turn on the developer mode for the crash handler).</p>" );
    TQWhatsThis::add( cb_disable_tdebuildsycoca, tmp );
    TQWhatsThis::add( label_tdebuildsycoca, tmp );
    connect( cb_disable_tdebuildsycoca, TQ_SIGNAL( clicked()), TQ_SIGNAL( changed()));
    defaults();
    }

void SystemWidget::load(bool useDefaults )
    {
    TDEConfig cfg( "kdedrc", true );
	 cfg.setReadDefaults( useDefaults );
    cfg.setGroup( "General" );
    cb_disable_tdebuildsycoca->setChecked( cfg.readBoolEntry( "DelayedCheck", false ));
    }

void SystemWidget::save()
    {
    TDEConfig cfg( "kdedrc" );
    cfg.setGroup( "General" );
    cfg.writeEntry( "DelayedCheck", cb_disable_tdebuildsycoca->isChecked());
    }

void SystemWidget::defaults()
    {
		 load( true );
    }

} // namespace

#include "system.moc"
