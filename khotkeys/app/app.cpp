/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#define _KHOTKEYS_APP_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "app.h"

#include <dcopclient.h>
#include <tdecmdlineargs.h>
#include <tdeconfig.h>
#include <tdelocale.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include <settings.h>
#include <input.h>
#include <action_data.h>
#include <gestures.h>
#include <voices.h>

namespace KHotKeys
{

// KhotKeysApp

KHotKeysApp::KHotKeysApp()
    :   TDEUniqueApplication( false, true ), // no styles
        delete_helper( new TQObject )
    {
    init_global_data( true, delete_helper ); // grab keys
    actions_root = NULL;
    reread_configuration();
    }

KHotKeysApp::~KHotKeysApp()
    {
    delete actions_root;
    // Many global data should be destroyed while the TQApplication object still
    // exists, and therefore 'this' cannot be the parent, as ~Object()
    // for 'this' would be called after ~TQApplication() - use proxy object
    delete delete_helper;
    }

void KHotKeysApp::reread_configuration()
    { // TODO
    kdDebug( 1217 ) << "reading configuration" << endl;
    delete actions_root;
    khotkeys_set_active( false );
    Settings settings;
    settings.read_settings( false );
    gesture_handler->set_mouse_button( settings.gesture_mouse_button );
    gesture_handler->set_timeout( settings.gesture_timeout );
    gesture_handler->enable( !settings.gestures_disabled_globally );
    gesture_handler->set_exclude( settings.gestures_exclude );
    voice_handler->set_shortcut( settings.voice_shortcut );
#if 0 // TEST CHECKME
    settings.write_settings();
#endif
    actions_root = settings.actions;
    khotkeys_set_active( true );
    actions_root->update_triggers();
    }

void KHotKeysApp::quit()
    {
    kapp->quit();
    }

} // namespace KHotKeys



using namespace KHotKeys;

// for multihead
static int khotkeys_screen_number = 0;

extern "C"
int TDE_EXPORT kdemain( int argc, char** argv )
{
  // Check if khotkeys is already running as a kded module.
  // In such case just exit.
  DCOPClient *dcopClient = new DCOPClient;
  if (!dcopClient->isAttached())
  {
    if (!dcopClient->attach())
    {
      kdWarning(1217) << "khotkeys [application]: could not register with DCOP. Exiting." << endl;
      delete dcopClient;
      return 1;
    }
  }
  TQCString replyType;
  TQByteArray replyData;
  if (dcopClient->call("kded", "kded", "loadedModules()",
        TQByteArray(), replyType, replyData))
  {
    if (replyType == "QCStringList")
    {
      TQDataStream reply(replyData, IO_ReadOnly);
      QCStringList modules;
      reply >> modules;
      if (modules.contains("khotkeys"))
      {
        // khotkeys is already running as a service, do nothing
        kdWarning(1217) << "khotkeys is already running as a kded module. Exiting." << endl;
        delete dcopClient;
        return 2;
      }
    }
  }
  delete dcopClient;

  // no need to i18n these, no GUI
  TDECmdLineArgs::init( argc, argv, "khotkeys", I18N_NOOP( "KHotKeys" ),
      I18N_NOOP( "KHotKeys daemon" ), KHOTKEYS_VERSION );
  TDEUniqueApplication::addCmdLineOptions();
  if( !KHotKeysApp::start()) // already running
  {
    return 0;
  }
  KHotKeysApp app;
  app.disableSessionManagement();
  return app.exec();
}


#include "app.moc"
