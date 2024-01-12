/*
   This file is part of the TDE project

   Copyright (C) 2008 Tobias Koenig <tokoe@kde.org>
   Copyright (C) 2016 Emanoil Kotsev <deloptes@yahoo.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "ktrashpropsdlgplugin.h"
#include "ktrashpropswidget.h"
#include "discspaceutil.h"
#include "trash_constant.h"
#include "trashimpl.h"

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqradiobutton.h>
#include <tqspinbox.h>

#include <kdesktopfile.h>
#include <kgenericfactory.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <knuminput.h>

#include <kdebug.h>

using namespace TrashConstant;

typedef KGenericFactory<KTrashPropsDlgPlugin, KPropertiesDialog> Factory;
K_EXPORT_COMPONENT_FACTORY( ktrashpropsdlgplugin, Factory( "ktrashpropsdlgplugin" ) )

KTrashPropsDlgPlugin::KTrashPropsDlgPlugin( KPropertiesDialog *dialog, const char*, const TQStringList& )
  : KPropsDlgPlugin( dialog )
{
  if ( dialog->items().count() != 1 )
    return;

  KFileItem *item = dialog->items().first();
  KURL itemUrl = item->url();
	if (!(itemUrl.protocol() == "trash" && item->name() == "."))
	{
	  // Check for a desktop file in case the protocol is not "trash"
		if (!KPropsDlgPlugin::isDesktopFile(item))
			return;

		KDesktopFile deskFile( itemUrl.path(), true /* readonly */ );
		if ( deskFile.readURL() != "trash:/" )
			return;
	}

  TDEGlobal::locale()->insertCatalogue( "tdeio_trash" );

  TQFrame *frame = dialog->addPage(i18n("&Trash Policy"));
	policyWidget = new KTrashPropsWidget(frame);
  TQVBoxLayout *vLayout = new TQVBoxLayout(frame, 0, 0);
  vLayout->addWidget(policyWidget);
  connect(policyWidget, TQ_SIGNAL(changed(bool)), TQ_SLOT(setDirty()));
}

KTrashPropsDlgPlugin::~KTrashPropsDlgPlugin()
{
}

void KTrashPropsDlgPlugin::applyChanges()
{
	policyWidget->save();
}

#include "ktrashpropsdlgplugin.moc"
