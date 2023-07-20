/***************************************************************************
                               konqsidebartest.h
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <tqlabel.h>
#include <tqlayout.h>

class SidebarTest : public KonqSidebarPlugin
	{
		TQ_OBJECT
		public:
		SidebarTest(TDEInstance *inst,TQObject *parent,TQWidget *widgetParent, TQString &desktopName_, const char* name=0):
                   KonqSidebarPlugin(inst,parent,widgetParent,desktopName_,name)
		{
			widget=new TQLabel("Init Value",widgetParent);			
		}
		~SidebarTest(){;}
                virtual TQWidget *getWidget(){return widget;}   
		virtual void *provides(const TQString &) {return 0;}  
		protected:
			TQLabel *widget;
			virtual void handleURL(const KURL &url)
				{
					widget->setText(url.url());
				}
	};

#endif
