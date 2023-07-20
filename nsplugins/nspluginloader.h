/*

  This is an encapsulation of the  Netscape plugin API.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
  Copyright (c) 2002-2005 George Staikos <staikos@kde.org>

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


#ifndef __NS_PLUGINLOADER_H__
#define __NS_PLUGINLOADER_H__


#include <tqstring.h>
#include <tqstringlist.h>
#include <tqdict.h>
#include <tqobject.h>
#include <tqwidget.h>
#include <qxembed.h>

#include "NSPluginClassIface_stub.h"

#define EMBEDCLASS QXEmbed

class TDEProcess;
class TQPushButton;
class TQGridLayout;

class NSPluginInstance : public EMBEDCLASS
{
  TQ_OBJECT

public:
    NSPluginInstance(TQWidget *parent);
    void init( const TQCString& app, const TQCString& obj );
    ~NSPluginInstance();
public: // wrappers
    void javascriptResult( int id, TQString result ) { stub->javascriptResult( id, result ); }

private slots:
    void loadPlugin();
    void doLoadPlugin();

protected:
    void resizeEvent(TQResizeEvent *event);
    void showEvent  (TQShowEvent *);
    void windowChanged(WId w);
    virtual void focusInEvent( TQFocusEvent* event );
    virtual void focusOutEvent( TQFocusEvent* event );
    class NSPluginLoader *_loader;
    bool shown;
    bool inited;
    int resize_count;
    TQPushButton *_button;
    TQGridLayout *_layout;
    NSPluginInstanceIface_stub* stub;
private: // wrappers
    void displayPlugin();
    void resizePlugin( int w, int h );
    void shutdown();
};


class NSPluginLoader : public TQObject
{
  TQ_OBJECT

public:
  NSPluginLoader();
  ~NSPluginLoader();

  NSPluginInstance *newInstance(TQWidget *parent,
                                TQString url, TQString mimeType, bool embed,
                                TQStringList argn, TQStringList argv,
                                TQString appId, TQString callbackId, bool reload,
                                bool doPost, TQByteArray postData);

  static NSPluginLoader *instance();
  void release();

protected:
  void scanPlugins();

  TQString lookup(const TQString &mimeType);
  TQString lookupMimeType(const TQString &url);

  bool loadViewer(const TQString &mimeType);
  void unloadViewer();

protected slots:
  void applicationRegistered( const TQCString& appId );
  void processTerminated( TDEProcess *proc );

private:
  TQStringList _searchPaths;
  TQDict<TQString> _mapping, _filetype;

  TDEProcess *_process;
  bool _running;
  TQCString _dcopid;
  NSPluginViewerIface_stub *_viewer;
  bool _useArtsdsp;

  static NSPluginLoader *s_instance;
  static int s_refCount;
};


#endif
