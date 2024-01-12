/*
 * main.cpp
 *
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
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

#include <unistd.h>

#include <tqlayout.h>
#include <tqmap.h>
#include <tqtabwidget.h>

#include <dcopclient.h>
#include <kdialog.h>
#include <kurifilter.h>
#include <kgenericfactory.h>

#include "filteropts.h"
#include "main.h"

typedef KGenericFactory<KURIFilterModule, TQWidget> KURIFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kurifilt, KURIFactory("kcmkurifilt") )

class FilterOptions;

KURIFilterModule::KURIFilterModule(TQWidget *parent, const char *name, const TQStringList &)
                 :TDECModule(KURIFactory::instance(), parent, name)
{

    filter = KURIFilter::self();

    setQuickHelp( i18n("<h1>Enhanced Browsing</h1> In this module you can configure some enhanced browsing"
      " features of TDE. <h2>Internet Keywords</h2>Internet Keywords let you"
      " type in the name of a brand, a project, a celebrity, etc... and go to the"
      " relevant location. For example you can just type"
      " \"TDE\" or \"Trinity Desktop Environment\" in Konqueror to go to TDE's homepage."
      "<h2>Web Shortcuts</h2>Web Shortcuts are a quick way of using Web search engines. For example, type \"altavista:frobozz\""
      " or \"av:frobozz\" and Konqueror will do a search on AltaVista for \"frobozz\"."
      " Even easier: just press Alt+F2 (if you have not"
      " changed this shortcut) and enter the shortcut in the TDE Run Command dialog."));

    TQVBoxLayout *layout = new TQVBoxLayout(this);

#if 0
    opts = new FilterOptions(this);
    tab->addTab(opts, i18n("&Filters"));
    connect(opts, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));
#endif

    modules.setAutoDelete(true);

    TQMap<TQString,TDECModule*> helper;
    TQPtrListIterator<KURIFilterPlugin> it = filter->pluginsIterator();
    for (; it.current(); ++it)
    {
        TDECModule *module = it.current()->configModule(this, 0);
        if (module)
        {
            modules.append(module);
            helper.insert(it.current()->configName(), module);
            connect(module, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));
        }
    }

    if (modules.count() > 1)
    {
        TQTabWidget *tab = new TQTabWidget(this);

        TQMapIterator<TQString,TDECModule*> it2;
        for (it2 = helper.begin(); it2 != helper.end(); ++it2)
        {
            tab->addTab(it2.data(), it2.key());
        }

        tab->showPage(modules.first());
        widget = tab;
    }
    else if (modules.count() == 1)
    {
        widget = modules.first();
        layout->setMargin(-KDialog::marginHint());
    }

    layout->addWidget(widget);
}

void KURIFilterModule::load()
{
    TQPtrListIterator<TDECModule> it(modules);
    for (; it.current(); ++it)
    {
	  it.current()->load();
    }
}

void KURIFilterModule::save()
{
    TQPtrListIterator<TDECModule> it(modules);
    for (; it.current(); ++it)
    {
	  it.current()->save();
    }
}

void KURIFilterModule::defaults()
{
    TQPtrListIterator<TDECModule> it(modules);
    for (; it.current(); ++it)
    {
	  it.current()->defaults();
    }
}

#include "main.moc"
