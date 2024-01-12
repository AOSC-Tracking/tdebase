/*
    Copyright (c) 2004 Jan Schaefer <j_schaef@informatik.uni-kl.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <tqstring.h>
#include <tqvbox.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqtimer.h>

#include <kgenericfactory.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <tdefileshare.h>
#include <tdemessagebox.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <dcopref.h>

#include "propertiespage.h"
#include "propsdlgshareplugin.h"
#include "../libmediacommon/medium.h"

typedef KGenericFactory<PropsDlgSharePlugin, KPropertiesDialog> PropsDlgSharePluginFactory;

K_EXPORT_COMPONENT_FACTORY( media_propsdlgplugin,
                            PropsDlgSharePluginFactory("media_propsdlgplugin") )

class PropsDlgSharePlugin::Private
{
  public:
    PropertiesPage* page;
};

PropsDlgSharePlugin::PropsDlgSharePlugin( KPropertiesDialog *dlg,
					  const char *, const TQStringList & )
  : KPropsDlgPlugin(dlg), d(0)
{
  if (properties->items().count() != 1)
    return;

  KFileItem *item = properties->items().first();

  DCOPRef mediamanager("kded", "mediamanager");
  kdDebug() << "properties " << item->url() << endl;
  DCOPReply reply = mediamanager.call( "properties", item->url().url() );

  if ( !reply.isValid() )
    return;

  TQVBox* vbox = properties->addVBoxPage(i18n("&Mounting"));

  d = new Private();

  d->page = new PropertiesPage(vbox, Medium::create(reply).id());
  connect(d->page, TQ_SIGNAL(changed()),
	  TQ_SLOT(slotChanged()));

  //  TQTimer::singleShot(100, this, TQ_SLOT(slotChanged()));

}

void PropsDlgSharePlugin::slotChanged()
{
  kdDebug() << "slotChanged()\n";
  setDirty(true);
}

PropsDlgSharePlugin::~PropsDlgSharePlugin()
{
  delete d;
}

void PropsDlgSharePlugin::applyChanges()
{
  kdDebug() << "applychanges\n";
  if (!d->page->save()) {
    properties->abortApplying();
  }
}


#include "propsdlgshareplugin.moc"

