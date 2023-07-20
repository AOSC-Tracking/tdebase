/*
  This is an encapsulation of the  Netscape plugin API.

  Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
*/                                                                            


#ifndef __TESTNSPLUGIN_H__
#define __TESTNSPLUGIN_H__

#include <tqstring.h>
#include <tqwidget.h>
#include <tdemainwindow.h>
#include <tqlayout.h>
#include <tqptrlist.h>

class NSPluginLoader;
class NSPluginInstance;

class TestNSPlugin : public TDEMainWindow
{
  TQ_OBJECT

public:
  TestNSPlugin();
  virtual ~TestNSPlugin();

public slots:
  void newView();
  void closeView();
  void viewDestroyed( NSPluginInstance *inst );

protected:
  NSPluginLoader *m_loader;
  TQPtrList<TQWidget> m_plugins;
  TQWidget *m_client;
  TQBoxLayout *m_layout;
};


#endif
