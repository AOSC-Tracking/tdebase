/*
  Netscape Plugin Loader KPart

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


#ifndef __plugin_part_h__
#define __plugin_part_h__

#include <tdeparts/browserextension.h>
#include <tdeparts/factory.h>
#include <tdeparts/part.h>
#include <klibloader.h>
#include <tqwidget.h>
#include <tqguardedptr.h>

class TDEAboutData;
class TDEInstance;
class PluginBrowserExtension;
class PluginLiveConnectExtension;
class TQLabel;
class NSPluginInstance;
class PluginPart;


#include "NSPluginCallbackIface.h"


class NSPluginCallback : public NSPluginCallbackIface
{
public:
  NSPluginCallback(PluginPart *part);

  ASYNC reloadPage();
  ASYNC requestURL(TQString url, TQString target);
  ASYNC postURL(TQString url, TQString target, TQByteArray data, TQString mime);
  ASYNC statusMessage( TQString msg );
  ASYNC evalJavaScript( int id, TQString script );

private:
  PluginPart *_part;
};


class PluginFactory : public KParts::Factory
{
  TQ_OBJECT

public:
  PluginFactory();
  virtual ~PluginFactory();

  virtual KParts::Part * createPartObject(TQWidget *parentWidget = 0, const char *widgetName = 0,
  		            	    TQObject *parent = 0, const char *name = 0,
  			            const char *classname = "KParts::Part",
   			            const TQStringList &args = TQStringList());

  static TDEInstance *instance();
  static TDEAboutData *aboutData();

private:

  static TDEInstance *s_instance;
  class NSPluginLoader *_loader;
};


class PluginCanvasWidget : public TQWidget
{
  TQ_OBJECT
public:

  PluginCanvasWidget(TQWidget *parent=0, const char *name=0)
    : TQWidget(parent,name) {}

protected:
  void resizeEvent(TQResizeEvent *e);

signals:
  void resized(int,int);
};


class PluginPart: public KParts::ReadOnlyPart
{
  TQ_OBJECT
public:
  PluginPart(TQWidget *parentWidget, const char *widgetName, TQObject *parent,
             const char *name, const TQStringList &args = TQStringList());
  virtual ~PluginPart();

  void postURL(const TQString& url, const TQString& target, const TQByteArray& data, const TQString& mime);
  void requestURL(const TQString& url, const TQString& target);
  void statusMessage( TQString msg );
  void evalJavaScript( int id, const TQString& script );
  void reloadPage();

  void changeSrc(const TQString& url);

protected:
  virtual bool openURL(const KURL &url);
  virtual bool closeURL();
  virtual bool openFile() { return false; };

protected slots:
  void pluginResized(int,int);
  void saveAs();

private:
  TQGuardedPtr<TQWidget> _widget;
  PluginCanvasWidget *_canvas;
  PluginBrowserExtension *_extension;
  PluginLiveConnectExtension *_liveconnect;
  NSPluginCallback *_callback;
  TQStringList _args;
  class NSPluginLoader *_loader;
  bool *_destructed;
};


class PluginLiveConnectExtension : public KParts::LiveConnectExtension
{
TQ_OBJECT
public:
    PluginLiveConnectExtension(PluginPart* part);
    virtual ~PluginLiveConnectExtension();
    virtual bool put(const unsigned long, const TQString &field, const TQString &value);
    virtual bool get(const unsigned long, const TQString&, Type&, unsigned long&, TQString&);
    virtual bool call(const unsigned long, const TQString&, const TQStringList&, Type&, unsigned long&, TQString&);

    TQString evalJavaScript( const TQString & script );

signals:
    virtual void partEvent( const unsigned long objid, const TQString & event, const KParts::LiveConnectExtension::ArgList & args );

private:
    PluginPart *_part;
    TQString *_retval;
};


#endif
