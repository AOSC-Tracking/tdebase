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

#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <tdeapplication.h>
#include <tdeaboutdata.h>
#include <tdeaboutapplication.h>
#include <kdebug.h>
#include <tdepopupmenu.h>
#include <kiconloader.h>

#include "trashapplet.h"

extern "C"
{
	KDE_EXPORT KPanelApplet* init( TQWidget *parent, const TQString& configFile)
	{
		TDEGlobal::locale()->insertCatalogue("trashapplet");
		return new TrashApplet(configFile, KPanelApplet::Normal,
			KPanelApplet::About, parent, "trashapplet");
	}
}

TrashApplet::TrashApplet(const TQString& configFile, Type type, int actions, TQWidget *parent, const char *name)
	: KPanelApplet(configFile, type, actions, parent, name), mButton(0)
{
	mButton = new TrashButton(this);

	if (!parent)
		setBackgroundMode(X11ParentRelative);

	mButton->setPanelPosition(position());
	
	setAcceptDrops(true);

	mpDirLister = new KDirLister();

	connect( mpDirLister, TQ_SIGNAL( clear() ),
	         this, TQ_SLOT( slotClear() ) );
	connect( mpDirLister, TQ_SIGNAL( completed() ),
	         this, TQ_SLOT( slotCompleted() ) );
	connect( mpDirLister, TQ_SIGNAL( deleteItem( KFileItem * ) ),
	         this, TQ_SLOT( slotDeleteItem( KFileItem * ) ) );

	mpDirLister->openURL("trash:/");
}

TrashApplet::~TrashApplet()
{
	// disconnect the dir lister before quitting so as not to crash
	// on kicker exit
	disconnect( mpDirLister, TQ_SIGNAL( clear() ),
	            this, TQ_SLOT( slotClear() ) );
	delete mpDirLister;
	TDEGlobal::locale()->removeCatalogue("trashapplet");
}

void TrashApplet::about()
{
	TDEAboutData data("trashapplet",
	                I18N_NOOP("Trash Applet"),
	                "1.0",
	                I18N_NOOP("\"trash:/\" ioslave frontend applet"),
	                TDEAboutData::License_GPL_V2,
	                "(c) 2004, Kevin Ottens");

	data.addAuthor("Kevin \'ervin\' Ottens",
	               I18N_NOOP("Maintainer"),
	               "ervin ipsquad net",
	               "http://ervin.ipsquad.net");

	TDEAboutApplication dialog(&data);
	dialog.exec();
}

int TrashApplet::widthForHeight( int height ) const
{
	if ( !mButton )
	{
		return height;
	}

	return mButton->widthForHeight( height );
}

int TrashApplet::heightForWidth( int width ) const
{
	if ( !mButton )
	{
		return width;
	}

	return mButton->heightForWidth( width );
}

void TrashApplet::resizeEvent( TQResizeEvent * )
{
	if (!mButton)
	{
		return;
	}

	int size = 1;

	size = std::max( size, 
			 orientation() == TQt::Vertical ?
			 	mButton->heightForWidth( width() ) :
				mButton->widthForHeight( height() ) );

	
	if(orientation() == TQt::Vertical)
	{
		mButton->resize( width(), size );
	}
	else
	{
		mButton->resize( size, height() );
	}
}

void TrashApplet::slotClear()
{
	kdDebug()<<"MediaApplet::slotClear"<<endl;

	mButton->setItemCount(0);
}

void TrashApplet::slotCompleted()
{
	mCount = mpDirLister->items(KDirLister::AllItems).count();
	mButton->setItemCount( mCount );
}

void TrashApplet::slotDeleteItem(KFileItem *)
{
	mCount--;
	mButton->setItemCount( mCount );
}


void TrashApplet::positionChange(Position p)
{
	mButton->setPanelPosition(p);
}


#include "trashapplet.moc"
