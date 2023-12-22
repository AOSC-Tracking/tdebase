/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

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

#include "trashbutton.h"

#include <tqpopupmenu.h>
#include <tqtooltip.h>

#include <tdelocale.h>
#include <krun.h>
#include <tdepopupmenu.h>

#include <tdeio/netaccess.h>

#include <konq_operations.h>
#include <konq_popupmenu.h>

TrashButton::TrashButton(TQWidget *parent)
	: PanelPopupButton(parent), mActions(this, this),
	  mFileItem(KFileItem::Unknown, KFileItem::Unknown, "trash:/")
{
	TDEIO::UDSEntry entry;
	TDEIO::NetAccess::stat("trash:/", entry, 0L);
	mFileItem.assign(KFileItem(entry, "trash:/"));

	TDEAction *a = KStdAction::paste(this, TQT_SLOT(slotPaste()),
	                               &mActions, "paste");
	a->setShortcut(0);

	move(0, 0);
	resize(20, 20);

	setTitle(i18n("Trash"));
	setIcon( "trashcan_empty" );

	setAcceptDrops(true);

	// Activate this code only if we find a way to have both an
	// action and a popup menu for the same kicker button
	//connect(this, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotClicked()));

	setPopup(new TQPopupMenu());
}

TrashButton::~TrashButton()
{
}

void TrashButton::setItemCount(int count)
{
    if (count==0)
    {
        setIcon( "trashcan_empty" );
        TQToolTip::add(this, i18n("Empty"));
    }
    else
    {
        setIcon( "trashcan_full" );
        TQToolTip::add(this, i18n("One item", "%n items", count));
    }
}

void TrashButton::initPopup()
{
	TQPopupMenu *old_popup = static_cast<TQPopupMenu*>(popup());

	KFileItemList items;
	items.append(&mFileItem);

	KonqPopupMenu::KonqPopupFlags kpf =
		  KonqPopupMenu::ShowProperties
		| KonqPopupMenu::ShowNewWindow;

	KParts::BrowserExtension::PopupFlags bef =
		  KParts::BrowserExtension::DefaultPopupItems;

	KonqPopupMenu *new_popup = new KonqPopupMenu(0L, items,
	                                   KURL("trash:/"), mActions, 0L,
	                                   this, kpf, bef);
	TDEPopupTitle *title = new TDEPopupTitle(new_popup);
	title->setTitle(i18n("Trash"));

	new_popup->insertItem(title, -1, 0);

	setPopup(new_popup);

	if (old_popup!=0L) delete old_popup;
}

// Activate this code only if we find a way to have both an
// action and a popup menu for the same kicker button
/*
void TrashButton::slotClicked()
{
	mFileItem.run();
}
*/

void TrashButton::slotPaste()
{
	KonqOperations::doPaste(this, mFileItem.url());
}

void TrashButton::dragEnterEvent(TQDragEnterEvent* e)
{
	e->accept(true);
}

void TrashButton::dropEvent(TQDropEvent *e)
{
	KonqOperations::doDrop(0L, mFileItem.url(), e, this);
}

TQString TrashButton::tileName()
{
	return mFileItem.name();
}

void TrashButton::setPanelPosition(KPanelApplet::Position position)
{
	switch(position)
	{
	case KPanelApplet::pBottom:
		setPopupDirection(KPanelApplet::Up);
		break;
	case KPanelApplet::pTop:
		setPopupDirection(KPanelApplet::Down);
		break;
	case KPanelApplet::pRight:
		setPopupDirection(KPanelApplet::Left);
		break;
	case KPanelApplet::pLeft:
		setPopupDirection(KPanelApplet::Right);
		break;
	}
}

#include "trashbutton.moc"
