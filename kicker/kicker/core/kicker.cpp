/*****************************************************************

  Copyright (c) 1996-2001 the kicker authors. See file AUTHORS.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 ******************************************************************/

#include <stdlib.h>
#include <unistd.h>

#include <tqfile.h>
#include <tqtimer.h>
#include <tqtooltip.h>

#include <dcopclient.h>
#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kcmultidialog.h>
#include <kcrash.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <kglobal.h>
#include <kglobalaccel.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kwin.h>
#include <kwinmodule.h>

#include "extensionmanager.h"
#include "pluginmanager.h"
#include "menumanager.h"
#include "k_new_mnu.h"
#include "k_mnu_stub.h"
#include "k_mnu.h"
#include "showdesktop.h"
#include "panelbutton.h"

#include "kicker.h"
#include "kickerSettings.h"

#include "kicker.moc"

Kicker* Kicker::the() { return static_cast<Kicker*>(kapp); }

Kicker::Kicker()
    : KUniqueApplication(),
      keys(0),
      m_kwinModule(0),
      m_configDialog(0),
      m_canAddContainers(true)
{
    // initialize the configuration object
    KickerSettings::instance(instanceName() + "rc");

    if (KCrash::crashHandler() == 0 )
    {
        // this means we've most likely crashed once. so let's see if we
        // stay up for more than 2 minutes time, and if so reset the
        // crash handler since the crash isn't a frequent offender
        TQTimer::singleShot(120000, this, TQT_SLOT(setCrashHandler()));
    }
    else
    {
        // See if a crash handler was installed. It was if the -nocrashhandler
        // argument was given, but the app eats the kde options so we can't
        // check that directly. If it wasn't, don't install our handler either.
        setCrashHandler();
    }

    // Make kicker immutable if configuration modules have been marked immutable
    if (isKioskImmutable() && kapp->authorizeControlModules(Kicker::configModules(true)).isEmpty())
    {
        config()->setReadOnly(true);
        config()->reparseConfiguration();
    }

    dcopClient()->setDefaultObject("Panel");
    disableSessionManagement();
    TQString dataPathBase = KStandardDirs::kde_default("data").append("kicker/");
    KGlobal::dirs()->addResourceType("mini", dataPathBase + "pics/mini");
    KGlobal::dirs()->addResourceType("icon", dataPathBase + "pics");
    KGlobal::dirs()->addResourceType("builtinbuttons", dataPathBase + "builtins");
    KGlobal::dirs()->addResourceType("specialbuttons", dataPathBase + "menuext");
    KGlobal::dirs()->addResourceType("applets", dataPathBase + "applets");
    KGlobal::dirs()->addResourceType("tiles", dataPathBase + "tiles");
    KGlobal::dirs()->addResourceType("extensions", dataPathBase +  "extensions");

    KImageIO::registerFormats();

    KGlobal::iconLoader()->addExtraDesktopThemes();

    KGlobal::locale()->insertCatalogue("kdmgreet");
    KGlobal::locale()->insertCatalogue("libkonq");
    KGlobal::locale()->insertCatalogue("libdmctl");
    KGlobal::locale()->insertCatalogue("libtaskbar");

    // initialize our keys
    // note that this creates the KMenu by calling MenuManager::the()
    keys = new KGlobalAccel( TQT_TQOBJECT(this) );
#define KICKER_ALL_BINDINGS
#include "kickerbindings.cpp"
    keys->readSettings();
    keys->updateConnections();

    // set up our global settings
    configure();

    connect(this, TQT_SIGNAL(settingsChanged(int)), TQT_SLOT(slotSettingsChanged(int)));
    connect(this, TQT_SIGNAL(kdisplayPaletteChanged()), TQT_SLOT(paletteChanged()));
    connect(this, TQT_SIGNAL(kdisplayStyleChanged()), TQT_SLOT(slotStyleChanged()));

#if (QT_VERSION-0 >= 0x030200) // XRANDR support
    connect(desktop(), TQT_SIGNAL(resized(int)), TQT_SLOT(slotDesktopResized()));
#endif

    // the panels, aka extensions
    TQTimer::singleShot(0, ExtensionManager::the(), TQT_SLOT(initialize()));

    connect(ExtensionManager::the(), TQT_SIGNAL(desktopIconsAreaChanged(const TQRect &, int)),
            this, TQT_SLOT(slotDesktopIconsAreaChanged(const TQRect &, int)));
}

Kicker::~Kicker()
{
    // order of deletion here is critical to avoid crashes
    delete ExtensionManager::the();
    delete MenuManager::the();
}

void Kicker::setCrashHandler()
{
    KCrash::setEmergencySaveFunction(Kicker::crashHandler);
}

void Kicker::crashHandler(int /* signal */)
{
    fprintf(stderr, "kicker: crashHandler called\n");

    DCOPClient::emergencyClose();
    sleep(1);
    system("kicker --nocrashhandler &"); // try to restart
}

void Kicker::slotToggleShowDesktop()
{
    // don't connect directly to the ShowDesktop::toggle() slot
    // so that the ShowDesktop object doesn't get created if
    // this feature is never used, and isn't created until after
    // startup even if it is
    ShowDesktop::the()->toggle();
}

void Kicker::toggleLock()
{
    KickerSettings::self()->setLocked(!KickerSettings::locked());
    KickerSettings::self()->writeConfig();
    emit immutabilityChanged(isImmutable());
}

void Kicker::toggleShowDesktop()
{
    ShowDesktop::the()->toggle();
}

bool Kicker::desktopShowing()
{
    return ShowDesktop::the()->desktopShowing();
}

void Kicker::slotSettingsChanged(int category)
{
    if (category == (int)KApplication::SETTINGS_SHORTCUTS)
    {
        keys->readSettings();
        keys->updateConnections();
    }
}

void Kicker::paletteChanged()
{
    KConfigGroup c(KGlobal::config(), "General");
    KickerSettings::setTintColor(c.readColorEntry("TintColor",
                                           &palette().active().mid()));
    KickerSettings::self()->writeConfig();
}

void Kicker::slotStyleChanged()
{
	restart();
}

bool Kicker::highlightMenuItem(const TQString &menuId)
{
    return MenuManager::the()->kmenu()->highlightMenuItem( menuId );
}

void Kicker::showKMenu()
{
    MenuManager::the()->kmenuAccelActivated();
}

void Kicker::popupKMenu(const TQPoint &p)
{
    MenuManager::the()->popupKMenu(p);
}

void Kicker::configure()
{
    static bool notFirstConfig = false;

    KConfig* c = KGlobal::config();
    c->reparseConfiguration();
    c->setGroup("General");
    m_canAddContainers = !c->entryIsImmutable("Applets2");

    KickerSettings::self()->readConfig();

    TQToolTip::setGloballyEnabled(KickerSettings::showToolTips());

    if (notFirstConfig)
    {
        emit configurationChanged();
        {
            TQByteArray data;
            emitDCOPSignal("configurationChanged()", data);
        }
    }

    notFirstConfig = true;
//    kdDebug(1210) << "tooltips " << ( _showToolTips ? "enabled" : "disabled" ) << endl;
}

void Kicker::quit()
{
    exit(1);
}

void Kicker::restart()
{
    // do this on a timer to give us time to return true
    TQTimer::singleShot(0, this, TQT_SLOT(slotRestart()));
}

void Kicker::slotRestart()
{
    // since the child will awaken before we do, we need to
    // clear the untrusted list manually; can't rely on the
    // dtor's to this for us.
    PluginManager::the()->clearUntrustedLists();

    char ** o_argv = new char*[2];
    o_argv[0] = strdup("kicker");
    o_argv[1] = 0L;
    execv(TQFile::encodeName(locate("exe", "kdeinit_wrapper")), o_argv);

    exit(1);
}

bool Kicker::isImmutable() const
{
    return config()->isImmutable() || KickerSettings::locked();
}

bool Kicker::isKioskImmutable() const
{
    return config()->isImmutable();
}

void Kicker::addExtension( const TQString &desktopFile )
{
   ExtensionManager::the()->addExtension( desktopFile );
}

TQStringList Kicker::configModules(bool controlCenter)
{
    TQStringList args;

    if (controlCenter)
    {
        args << "kde-panel.desktop";
    }
    else
    {
        args << "kde-kicker_config_arrangement.desktop"
             << "kde-kicker_config_hiding.desktop"
             << "kde-kicker_config_menus.desktop"
             << "kde-kicker_config_appearance.desktop";
    }
    args << "kde-kcmtaskbar.desktop";
    return args;
}

TQPoint Kicker::insertionPoint()
{
    return m_insertionPoint;
}

void Kicker::setInsertionPoint(const TQPoint &p)
{
    m_insertionPoint = p;
}


void Kicker::showConfig(const TQString& configPath, int page)
{
    if (!m_configDialog)
    {
         m_configDialog = new KCMultiDialog(0);

         TQStringList modules = configModules(false);
         TQStringList::ConstIterator end(modules.end());
         for (TQStringList::ConstIterator it = modules.begin(); it != end; ++it)
         {
            m_configDialog->addModule(*it);
         }

         connect(m_configDialog, TQT_SIGNAL(finished()), TQT_SLOT(configDialogFinished()));
    }

    if (!configPath.isEmpty())
    {
        TQByteArray data;
        TQDataStream stream(data, IO_WriteOnly);
        stream << configPath;
        emitDCOPSignal("configSwitchToPanel(TQString)", data);
    }

    KWin::setOnDesktop(m_configDialog->winId(), KWin::currentDesktop());
    m_configDialog->show();
    m_configDialog->raise();
    if (page > -1)
    {
        m_configDialog->showPage(page);
    }
}

void Kicker::showTaskBarConfig()
{
    showConfig(TQString(), 4);
}

void Kicker::configureMenubar()
{
    ExtensionManager::the()->configureMenubar(false);
}

void Kicker::configDialogFinished()
{
    m_configDialog->delayedDestruct();
    m_configDialog = 0;
}

void Kicker::slotDesktopResized()
{
    configure(); // reposition on the desktop
}

void Kicker::clearQuickStartMenu()
{
    MenuManager::the()->kmenu()->clearRecentMenuItems();
}

KWinModule* Kicker::kwinModule()
{
    if (!m_kwinModule)
    {
        m_kwinModule = new KWinModule();
    }

    return m_kwinModule;
}

TQRect Kicker::desktopIconsArea(int screen) const
{
    return ExtensionManager::the()->desktopIconsArea(screen);
}

void Kicker::slotDesktopIconsAreaChanged(const TQRect &area, int screen)
{
    TQByteArray params;
    TQDataStream stream(params, IO_WriteOnly);
    stream << area;
    stream << screen;
    emitDCOPSignal("desktopIconsAreaChanged(TQRect, int)", params);
}
