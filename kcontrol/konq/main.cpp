/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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


#include "rootopts.h"
#include "behaviour.h"
#include "fontopts.h"
#include "desktop.h"
#include "previews.h"
#include "browser.h"
#include "desktopbehavior_impl.h"

#include <tdeconfig.h>
#include <tdeapplication.h>

static TQCString configname()
{
	int desktop = TDEApplication::desktop()->primaryScreen();
	TQCString name;
	if (desktop == 0)
		name = "kdesktoprc";
    else
		name.sprintf("kdesktop-screen-%drc", desktop);

	return name;
}


extern "C"
{
  TDE_EXPORT TDECModule *create_browser(TQWidget *parent, const char *name)
  {
    TDEConfig *config = new TDEConfig("konquerorrc", false, true);
    return new KBrowserOptions(config, "FMSettings", parent, name);
  }

  TDE_EXPORT TDECModule *create_behavior(TQWidget *parent, const char *name)
  {
    TDEConfig *config = new TDEConfig("konquerorrc", false, true);
    return new KBehaviourOptions(config, "FMSettings", parent, name);
  }

  TDE_EXPORT TDECModule *create_appearance(TQWidget *parent, const char *name)
  {
    TDEConfig *config = new TDEConfig("konquerorrc", false, true);
    return new KonqFontOptions(config, "FMSettings", false, parent, name);
  }

  TDE_EXPORT TDECModule *create_previews(TQWidget *parent, const char *name)
  {
    return new KPreviewOptions(parent, name);
  }

  TDE_EXPORT TDECModule *create_dbehavior(TQWidget *parent, const char* /*name*/)
  {
    TDEConfig *config = new TDEConfig(configname(), false, false);
    return new DesktopBehaviorModule(config, parent);
  }

  TDE_EXPORT TDECModule *create_dappearance(TQWidget *parent, const char* /*name*/)
  {
    TDEConfig *config = new TDEConfig(configname(), false, false);
    return new KonqFontOptions(config, "FMSettings", true, parent);
  }

  TDE_EXPORT TDECModule *create_dpath(TQWidget *parent, const char* /*name*/)
  {
    //TDEConfig *config = new TDEConfig(configname(), false, false);
    return new DesktopPathConfig(parent);
  }

  TDE_EXPORT TDECModule *create_ddesktop(TQWidget *parent, const char* /*name*/)
  {
    return new KDesktopConfig(parent, "VirtualDesktops");
  }
}


