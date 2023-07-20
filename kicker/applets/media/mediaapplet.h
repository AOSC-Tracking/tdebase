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

#ifndef MEDIAAPPLET_H
#define MEDIAAPPLET_H

#ifdef HAVE_CONFIG_H
        #include <config.h>
#endif

#include <kpanelapplet.h>
#include <tqstring.h>
#include <tdeconfig.h>
#include <kurl.h>
#include <tdefileitem.h>
#include <kdirlister.h>

#include <tqptrlist.h>
#include "mediumbutton.h"
typedef TQValueList<MediumButton*> MediumButtonList;


class MediaApplet : public KPanelApplet
{
TQ_OBJECT

public:
	MediaApplet(const TQString& configFile, Type t = Normal, int actions = 0,
	              TQWidget *parent = 0, const char *name = 0);
	~MediaApplet();

	int widthForHeight(int height) const;
	int heightForWidth(int width) const;
	void about();
	void preferences();

protected:
	void arrangeButtons();
	void resizeEvent(TQResizeEvent *e);
	void positionChange(Position p);
	void reloadList();
	void loadConfig();
	void saveConfig();
	void mousePressEvent(TQMouseEvent *e);

protected slots:
	void slotClear();
	void slotStarted(const KURL &url);
	void slotCompleted();
	void slotNewItems(const KFileItemList &entries);
	void slotDeleteItem(KFileItem *fileItem);
	void slotRefreshItems(const KFileItemList &entries);

private:
	KDirLister *mpDirLister;
	MediumButtonList mButtonList;
	TQStringList mExcludedTypesList;
	TQStringList mExcludedList;
	KFileItemList mMedia;
        int mButtonSizeSum;
};

#endif
