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

#ifndef MEDIUMBUTTON_H
#define MEDIUMBUTTON_H

#include <panelbutton.h>
#include <tqpoint.h>
#include <tqstring.h>
#include <tqpixmap.h>
#include <tqcursor.h>
#include <tqtimer.h>
#include <tdefileitem.h>
#include <kpanelapplet.h>
#include <tdeactioncollection.h>

class MediumButton : public PanelPopupButton
{
TQ_OBJECT

public:
	MediumButton(TQWidget *parent, const KFileItem &fileItem);
	~MediumButton();
	const KFileItem &fileItem() const;
	void setFileItem(const KFileItem &fileItem);
	void setPanelPosition(KPanelApplet::Position position);

protected:
	TQString tileName();
	void refreshType();
	void initPopup();
	void dragEnterEvent( TQDragEnterEvent* );
	void dragLeaveEvent( TQDragLeaveEvent* );
	void dropEvent(TQDropEvent *e);

protected slots:
	// Activate this code only if we find a way to have both an
	// action and a popup menu for the same kicker button
	//void slotClicked();
	void slotPaste();
	void slotCopy();
	void slotDragOpen();

private:
	TDEActionCollection mActions;
	KFileItem mFileItem;
	TQTimer mOpenTimer;
};

#endif
