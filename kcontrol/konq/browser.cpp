/* This file is part of the KDE project
   Copyright (C) 2002 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <tqlayout.h>
#include <tqtabwidget.h>
#include <tqfile.h>

#include <tdelocale.h>
#include <kdialog.h>
#include <fixx11h.h>
#include <tdecmoduleloader.h>

#include "behaviour.h"
#include "fontopts.h"
#include "previews.h"
#include "browser.h"

KBrowserOptions::KBrowserOptions(TDEConfig *config, TQString group, TQWidget *parent, const char *name)
    : TDECModule( parent, "kcmkonq" ) 
{
  TQVBoxLayout *layout = new TQVBoxLayout(this);
  TQTabWidget *tab = new TQTabWidget(this);
  layout->addWidget(tab);

  appearance = new KonqFontOptions(config, group, false, tab, name);
  appearance->layout()->setMargin( KDialog::marginHint() );

  behavior = new KBehaviourOptions(config, group, tab, name);
  behavior->layout()->setMargin( KDialog::marginHint() );

  previews = new KPreviewOptions(tab, name);
  previews->layout()->setMargin( KDialog::marginHint() );

  kuick = TDECModuleLoader::loadModule("kcmkuick", tab);

  tab->addTab(appearance, i18n("&Appearance"));
  tab->addTab(behavior, i18n("&Behavior"));
  tab->addTab(previews, i18n("&Previews && Meta-Data"));
  if (kuick)
  {
    kuick->layout()->setMargin( KDialog::marginHint() );
    tab->addTab(kuick, i18n("&Quick Copy && Move"));
  }

  connect(appearance, TQ_SIGNAL(changed(bool)), this, TQ_SIGNAL(changed(bool)));
  connect(behavior, TQ_SIGNAL(changed(bool)), this, TQ_SIGNAL(changed(bool)));
  connect(previews, TQ_SIGNAL(changed(bool)), this, TQ_SIGNAL(changed(bool)));
  if (kuick)
     connect(kuick, TQ_SIGNAL(changed(bool)), this, TQ_SIGNAL(changed(bool)));

  connect(tab, TQ_SIGNAL(currentChanged(TQWidget *)), 
          this, TQ_SIGNAL(quickHelpChanged()));
  m_tab = tab;
}

void KBrowserOptions::load()
{
  appearance->load();
  behavior->load();
  previews->load();
  if (kuick)
     kuick->load();
}

void KBrowserOptions::defaults()
{
  appearance->defaults();
  behavior->defaults();
  previews->defaults();
  if (kuick)
     kuick->defaults();
}

void KBrowserOptions::save()
{
  appearance->save();
  behavior->save();
  previews->save();
  if (kuick)
     kuick->save();
}

TQString KBrowserOptions::handbookDocPath() const
{
 	int index = m_tab->currentPageIndex();
	if (kuick && index == 3)
		return "konq-plugins/kuick/index.html";
 	else
 		return TQString::null;
}

TQString KBrowserOptions::handbookSection() const
{
 	int index = m_tab->currentPageIndex();
 	if (index == 0)
		return "fileman-appearance";
	else if (index == 1)
		return "fileman-behav";
	else if (index == 2)
		return "fileman-previews";
 	else
 		return TQString::null;
}

TQString KBrowserOptions::quickHelp() const
{
  TQWidget *w = m_tab->currentPage();
  if (w->inherits("TDECModule"))
  {
     TDECModule *m = static_cast<TDECModule *>(w);
     return m->quickHelp();
  }
  return TQString::null;
}

#include "browser.moc"
