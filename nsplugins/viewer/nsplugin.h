/*

  This is an encapsulation of the  Netscape plugin API.

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
  Copyright (c) 2003-2005 George Staikos <staikos@kde.org>

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


#ifndef __NS_PLUGIN_H__
#define __NS_PLUGIN_H__


#include <dcopobject.h>
#include "NSPluginClassIface.h"
#include "NSPluginCallbackIface_stub.h"


#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqptrqueue.h>
#include <tqdict.h>
#include <tqmap.h>
#include <tqintdict.h>
#include <tqguardedptr.h>

#include <tdeparts/browserextension.h>  // for URLArgs
#include <tdeio/job.h>


#define XP_UNIX
#define MOZ_X11
#include "sdk/npupp.h"

typedef char* NP_GetMIMEDescriptionUPP(void);
typedef NPError NP_InitializeUPP(NPNetscapeFuncs*, NPPluginFuncs*);
typedef NPError NP_ShutdownUPP(void);


#include <X11/Intrinsic.h>


void quitXt();

class KLibrary;
class TQTimer;


class NSPluginStreamBase : public TQObject
{
TQ_OBJECT
friend class NSPluginInstance;
public:
  NSPluginStreamBase( class NSPluginInstance *instance );
  ~NSPluginStreamBase();

  KURL url() { return _url; }
  int pos() { return _pos; }
  void stop();

signals:
  void finished( NSPluginStreamBase *strm );

protected:
  void finish( bool err );
  bool pump();
  bool error() { return _error; }
  void queue( const TQByteArray &data );
  bool create( const TQString& url, const TQString& mimeType, void *notify, bool forceNotify = false );
  int tries() { return _tries; }
  void inform( );
  void updateURL( const KURL& newURL );

  class NSPluginInstance *_instance;
  uint16 _streamType;
  NPStream *_stream;
  void *_notifyData;
  KURL _url;
  TQString _fileURL;
  TQString _mimeType;
  TQByteArray _headers;
  TQByteArray _data;
  class KTempFile *_tempFile;

private:
  int process( const TQByteArray &data, int start );

  unsigned int _pos;
  TQByteArray _queue;
  unsigned int _queuePos;
  int _tries;
  bool _onlyAsFile;
  bool _error;
  bool _informed;
  bool _forceNotify;
};


class NSPluginStream : public NSPluginStreamBase
{
  TQ_OBJECT

public:
  NSPluginStream( class NSPluginInstance *instance );
  ~NSPluginStream();

  bool get(const TQString& url, const TQString& mimeType, void *notifyData, bool reload = false);
  bool post(const TQString& url, const TQByteArray& data, const TQString& mimeType, void *notifyData, const KParts::URLArgs& args);

protected slots:
  void data(TDEIO::Job *job, const TQByteArray &data);
  void totalSize(TDEIO::Job *job, TDEIO::filesize_t size);
  void mimetype(TDEIO::Job * job, const TQString &mimeType);
  void result(TDEIO::Job *job);
  void redirection(TDEIO::Job *job, const KURL& url);
  void resume();

protected:
  TQGuardedPtr<TDEIO::TransferJob> _job;
  TQTimer *_resumeTimer;
};


class NSPluginBufStream : public NSPluginStreamBase
{
  TQ_OBJECT

public:
  NSPluginBufStream( class NSPluginInstance *instance );
  ~NSPluginBufStream();

  bool get( const TQString& url, const TQString& mimeType, const TQByteArray &buf, void *notifyData, bool singleShot=false );

protected slots:
  void timer();

protected:
  TQTimer *_timer;
  bool _singleShot;
};


class NSPluginInstance : public TQObject, public virtual NSPluginInstanceIface
{
  TQ_OBJECT

public:

  // constructor, destructor
  NSPluginInstance( NPP privateData, NPPluginFuncs *pluginFuncs, KLibrary *handle,
		    int width, int height, TQString src, TQString mime,
                    TQString appId, TQString callbackId, bool embed, WId xembed,
		    TQObject *parent, const char* name=0 );
  ~NSPluginInstance();

  // DCOP functions
  void shutdown();
  int winId() { return _form != 0 ? XtWindow(_form) : 0; }
  int setWindow(TQ_INT8 remove=0);
  void resizePlugin(TQ_INT32 w, TQ_INT32 h);
  void javascriptResult(TQ_INT32 id, TQString result);
  void displayPlugin();
  void gotFocusIn();
  void gotFocusOut();

  // value handling
  NPError NPGetValue(NPPVariable variable, void *value);
  NPError NPSetValue(NPNVariable variable, void *value);

  // window handling
  NPError NPSetWindow(NPWindow *window);

  // stream functions
  NPError NPDestroyStream(NPStream *stream, NPReason reason);
  NPError NPNewStream(NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype);
  void NPStreamAsFile(NPStream *stream, const char *fname);
  int32 NPWrite(NPStream *stream, int32 offset, int32 len, void *buf);
  int32 NPWriteReady(NPStream *stream);

  // URL functions
  void NPURLNotify(TQString url, NPReason reason, void *notifyData);

  // Event handling
  uint16 HandleEvent(void *event);

  // signal emitters
  void emitStatus( const TQString &message);
  void requestURL( const TQString &url, const TQString &mime,
		   const TQString &target, void *notify, bool forceNotify = false, bool reload = false );
  void postURL( const TQString &url, const TQByteArray& data, const TQString &mime,
             const TQString &target, void *notify, const KParts::URLArgs& args, bool forceNotify = false );

  TQString normalizedURL(const TQString& url) const;

public slots:
  void streamFinished( NSPluginStreamBase *strm );

private slots:
  void timer();

private:
  friend class NSPluginStreamBase;

  static void forwarder(Widget, XtPointer, XEvent *, Boolean*);

  void destroy();

  bool _destroyed;
  bool _visible;
  void addTempFile(KTempFile *tmpFile);
  TQPtrList<KTempFile> _tempFiles;
  NSPluginCallbackIface_stub *_callback;
  TQPtrList<NSPluginStreamBase> _streams;
  KLibrary *_handle;
  TQTimer *_timer;

  NPP      _npp;
  NPPluginFuncs _pluginFuncs;

  Widget _area, _form, _toplevel;
  WId _xembed_window;
  TQString _baseURL;
  int _width, _height;

  struct Request
  {
      // A GET request
      Request( const TQString &_url, const TQString &_mime,
	       const TQString &_target, void *_notify, bool _forceNotify = false,
               bool _reload = false)
	  { url=_url; mime=_mime; target=_target; notify=_notify; post=false; forceNotify = _forceNotify; reload = _reload; }

      // A POST request
      Request( const TQString &_url, const TQByteArray& _data,
               const TQString &_mime, const TQString &_target, void *_notify,
               const KParts::URLArgs& _args, bool _forceNotify = false)
	  { url=_url; mime=_mime; target=_target;
            notify=_notify; post=true; data=_data; args=_args;
            forceNotify = _forceNotify; }

      TQString url;
      TQString mime;
      TQString target;
      TQByteArray data;
      bool post;
      bool forceNotify;
      bool reload;
      void *notify;
      KParts::URLArgs args;
  };

  NPWindow _win;
  NPSetWindowCallbackStruct _win_info;
  TQPtrQueue<Request> _waitingRequests;
  TQMap<int, Request*> _jsrequests;
};


class NSPluginClass : public TQObject, virtual public NSPluginClassIface
{
  TQ_OBJECT
public:

  NSPluginClass( const TQString &library, TQObject *parent, const char *name=0 );
  ~NSPluginClass();

  TQString getMIMEDescription();
  DCOPRef newInstance(TQString url, TQString mimeType, TQ_INT8 embed,
                      TQStringList argn, TQStringList argv,
                      TQString appId, TQString callbackId, TQ_INT8 reload, TQ_INT8 post,
                      TQByteArray postData, TQ_UINT32 xembed );
  void destroyInstance( NSPluginInstance* inst );
  bool error() { return _error; }

  void setApp(const TQCString& app) { _app = app; }
  const TQCString& app() const { return _app; }

protected slots:
  void timer();

private:
  int initialize();
  void shutdown();

  KLibrary *_handle;
  TQString  _libname;
  bool _constructed;
  bool _error;
  TQTimer *_timer;

  NP_GetMIMEDescriptionUPP *_NP_GetMIMEDescription;
  NP_InitializeUPP *_NP_Initialize;
  NP_ShutdownUPP *_NP_Shutdown;

  NPPluginFuncs _pluginFuncs;
  NPNetscapeFuncs _nsFuncs;

  TQPtrList<NSPluginInstance> _instances;
  TQPtrList<NSPluginInstance> _trash;

  TQCString _app;

  // If plugins use gtk, we call the gtk_init function for them ---
  // but only do it once.
  static bool s_initedGTK;
};


class NSPluginViewer : public TQObject, virtual public NSPluginViewerIface
{
    TQ_OBJECT
public:
   NSPluginViewer( TQCString dcopId, TQObject *parent, const char *name=0 );
   virtual ~NSPluginViewer();

   void shutdown();
   DCOPRef newClass( TQString plugin );

private slots:
   void appUnregistered(const TQCString& id);

private:
   TQDict<NSPluginClass> _classes;
};


#endif
