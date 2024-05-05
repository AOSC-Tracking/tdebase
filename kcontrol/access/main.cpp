
#include "kaccess.h"
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <kdebug.h>

extern "C" TDE_EXPORT int kdemain(int argc, char * argv[] )
{
  TDEAboutData about(I18N_NOOP("kaccess"), I18N_NOOP("TDE Accessibility Tool"),
                  0, 0, TDEAboutData::License_GPL,
                  I18N_NOOP("(c) 2000, Matthias Hoelzer-Kluepfel"));

  about.addAuthor("Matthias Hoelzer-Kluepfel", I18N_NOOP("Author") , "hoelzer@kde.org");

  TDECmdLineArgs::init( argc, argv, &about );

  if (!KAccessApp::start())
    return 0;

  // verify the Xlib has matching XKB extension
  int major = XkbMajorVersion;
  int minor = XkbMinorVersion;
  if (!XkbLibraryVersion(&major, &minor))
    {
      kdError() << "[kcontrol-access] Xlib XKB extension does not match" << endl;
      return 1;
    }
  kdDebug() << "[kcontrol-access] Xlib XKB extension major=" << major << " minor=" << minor << endl;

  // we need an application object for tqt_xdisplay()
  KAccessApp app;

  // verify the X server has matching XKB extension
  // if yes, the XKB extension is initialized
  int opcode_rtrn;
  int error_rtrn;
  int xkb_opcode;
  if (!XkbQueryExtension(tqt_xdisplay(), &opcode_rtrn, &xkb_opcode, &error_rtrn,
			 &major, &minor))
    {
      kdError() << "[kcontrol-access] X server has not matching XKB extension" << endl;
      return 1;
    }
  kdDebug() << "[kcontrol-access] X server XKB extension major=" << major << " minor=" << minor << endl;

  app.setXkbOpcode(xkb_opcode);
  app.disableSessionManagement();
  return app.exec();
}
