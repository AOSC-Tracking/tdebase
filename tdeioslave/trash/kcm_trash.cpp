/*
   This file is part of the TDE Project

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

#include "kcm_trash.h"
#include "ktrashpropswidget.h"

#include <tqlayout.h>
#include <tqtabwidget.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdeaboutdata.h>
#include <kdialog.h>


extern "C"
{
  KDE_EXPORT TDECModule *create_trash(TQWidget *parent, const char *)
  {
    return new TrashModule(parent, "trash");
  }
}


TrashModule::TrashModule(TQWidget *parent, const char *name)
	: TDECModule(parent, name)
{
	TDEAboutData * about = new TDEAboutData("trash",
	                                    I18N_NOOP("Trash"), "1",
	                                    I18N_NOOP("Trash Control Panel Module"),
	                                    TDEAboutData::License_GPL_V2,
	                                    I18N_NOOP("(c) 2019 Michele Calgaro"));
	setAboutData( about );
  TDEGlobal::locale()->insertCatalogue("trash");

  tab = new TQTabWidget(this);

	policyWidget = new KTrashPropsWidget(this);
  tab->addTab(policyWidget, i18n("Trash Polic&y"));
  connect(policyWidget, TQT_SIGNAL(changed(bool)), TQT_SIGNAL(changed(bool)));

  TQVBoxLayout *top = new TQVBoxLayout(this);
  top->addWidget(tab);
}

void TrashModule::load()
{
  policyWidget->load();
}

void TrashModule::save()
{
  policyWidget->save();
}

void TrashModule::defaults()
{
  policyWidget->setDefaultValues();
}

TQString TrashModule::quickHelp() const
{
  return i18n("<h1>Trash</h1> Here you can choose the settings "
              "for your Trash Bin size and clean up policy. ");
}

#include "kcm_trash.moc"
