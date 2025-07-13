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

#include "mediumbutton.h"

#include <tqapplication.h>
#include <tqclipboard.h>
#include <tqpainter.h>
#include <tqdrawutil.h>
#include <tqpopupmenu.h>
#include <tqstyle.h>
#include <tqtooltip.h>

#include <tdemessagebox.h>
#include <kmimetype.h>
#include <tdelocale.h>
#include <tdedesktopfile.h>
#include <krun.h>
#include <tdeglobalsettings.h>
#include <kcursor.h>
#include <tdeapplication.h>
#include <kipc.h>
#include <kiconloader.h>
#include <kurldrag.h>
#include <tdepopupmenu.h>

#include <konq_operations.h>
#include <konq_popupmenu.h>
#include <konq_drag.h>

MediumButton::MediumButton(TQWidget *parent, const KFileItem &fileItem)
	: PanelPopupButton(parent), mActions(this, this), mFileItem(fileItem), mOpenTimer(0,
                "MediumButton::mOpenTimer")
{
    TDEAction *a = KStdAction::paste(this, TQ_SLOT(slotPaste()),
                                    &mActions, "pasteto");
    a->setShortcut(0);
    a = KStdAction::copy(this, TQ_SLOT(slotCopy()), &mActions, "copy");
    a->setShortcut(0);
    
    setBackgroundOrigin(AncestorOrigin);
    
    resize(20, 20);
    
    setAcceptDrops(mFileItem.isWritable());
    
    setTitle(mFileItem.text());
    
    refreshType();
    
    connect(&mOpenTimer, TQ_SIGNAL(timeout()), TQ_SLOT(slotDragOpen()));
    
    // Activate this code only if we find a way to have both an
    // action and a popup menu for the same kicker button
    //connect(this, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotClicked()));
    
    setPopup(new TQPopupMenu());
}

MediumButton::~MediumButton()
{
	TQPopupMenu *menu = static_cast<TQPopupMenu*>(popup());
	setPopup(0);
	delete menu;
}

const KFileItem &MediumButton::fileItem() const
{
    return mFileItem;
}

void MediumButton::setFileItem(const KFileItem &fileItem)
{
    mFileItem.assign(fileItem);
    setAcceptDrops(mFileItem.isWritable());
    setTitle(mFileItem.text());
    refreshType();
}

void MediumButton::initPopup()
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
	                                   KURL("media:/"), mActions, 0L,
	                                   this, kpf, bef);
	TDEPopupTitle *title = new TDEPopupTitle(new_popup);
	title->setTitle(mFileItem.text());

	new_popup->insertItem(title, -1, 0);

	setPopup(new_popup);

	if (old_popup!=0L) delete old_popup;
}

void MediumButton::refreshType()
{
    TQToolTip::add(this, mFileItem.text());
    setIcon(mFileItem.iconName());
}

// Activate this code only if we find a way to have both an
// action and a popup menu for the same kicker button
/*
void MediumButton::slotClicked()
{
	mFileItem.run();
}
*/

void MediumButton::slotPaste()
{
    KonqOperations::doPaste(this, mFileItem.url());
}

void MediumButton::slotCopy()
{
    KonqDrag * obj = KonqDrag::newDrag(mFileItem.url(), false);
    
    TQApplication::clipboard()->setData( obj );
}

void MediumButton::dragEnterEvent(TQDragEnterEvent* e)
{
    if (mFileItem.isWritable())
    {
        mOpenTimer.start(1000, true);
        e->accept(true);
    }
}

void MediumButton::dragLeaveEvent(TQDragLeaveEvent* e)
{
    mOpenTimer.stop();
    
    PanelPopupButton::dragLeaveEvent( e );
}

void MediumButton::dropEvent(TQDropEvent *e)
{
    mOpenTimer.stop();
    
    KonqOperations::doDrop(&mFileItem, mFileItem.url(), e, this);
}

void MediumButton::slotDragOpen()
{
    mFileItem.run();
}

TQString MediumButton::tileName()
{
    return mFileItem.text();
}

void MediumButton::setPanelPosition(KPanelApplet::Position position)
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

#include "mediumbutton.moc"
