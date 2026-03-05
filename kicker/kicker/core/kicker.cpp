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
#include <tdeconfig.h>
#include <tdecmdlineargs.h>
#include <kcmultidialog.h>
#include <tdecrash.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <tdeglobal.h>
#include <tdeglobalaccel.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdestandarddirs.h>
#include <twin.h>
#include <twinmodule.h>

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

Kicker* Kicker::the() { return static_cast<Kicker*>(tdeApp); }

Kicker::Kicker()
    : TDEUniqueApplication(),
      keys(0),
      m_twinModule(0),
      m_configDialog(0),
      m_canAddContainers(true),
      m_reloadingConfigDialog(false)
{
    // initialize the configuration object
    KickerSettings::instance(instanceName() + "rc");

    if (TDECrash::crashHandler() == 0 )
    {
        // this means we've most likely crashed once. so let's see if we
        // stay up for more than 2 minutes time, and if so reset the
        // crash handler since the crash isn't a frequent offender
        TQTimer::singleShot(120000, this, TQ_SLOT(setCrashHandler()));
    }
    else
    {
        // See if a crash handler was installed. It was if the -nocrashhandler
        // argument was given, but the app eats the kde options so we can't
        // check that directly. If it wasn't, don't install our handler either.
        setCrashHandler();
    }

    // Make kicker immutable if configuration modules have been marked immutable
    if (isKioskImmutable() && tdeApp->authorizeControlModules(Kicker::configModules(true)).isEmpty())
    {
        config()->setReadOnly(true);
        config()->reparseConfiguration();
    }

    dcopClient()->setDefaultObject("Panel");
    disableSessionManagement();
    TQString dataPathBase = TDEStandardDirs::kde_default("data").append("kicker/");
    TDEGlobal::dirs()->addResourceType("mini", dataPathBase + "pics/mini");
    TDEGlobal::dirs()->addResourceType("icon", dataPathBase + "pics");
    TDEGlobal::dirs()->addResourceType("builtinbuttons", dataPathBase + "builtins");
    TDEGlobal::dirs()->addResourceType("specialbuttons", dataPathBase + "menuext");
    TDEGlobal::dirs()->addResourceType("applets", dataPathBase + "applets");
    TDEGlobal::dirs()->addResourceType("tiles", dataPathBase + "tiles");
    TDEGlobal::dirs()->addResourceType("extensions", dataPathBase +  "extensions");

    KImageIO::registerFormats();

    TDEGlobal::iconLoader()->addExtraDesktopThemes();

    TDEGlobal::locale()->insertCatalogue("tdmgreet");
    TDEGlobal::locale()->insertCatalogue("libkonq");
    TDEGlobal::locale()->insertCatalogue("libdmctl");
    TDEGlobal::locale()->insertCatalogue("libtaskbar");

    // initialize our keys
    // note that this creates the KMenu by calling MenuManager::the()
    keys = new TDEGlobalAccel( this );
#define KICKER_ALL_BINDINGS
#include "kickerbindings.cpp"
    keys->readSettings();
    keys->updateConnections();

    // set up our global settings
    configure();

    connect(this, TQ_SIGNAL(settingsChanged(int)), TQ_SLOT(slotSettingsChanged(int)));
    connect(this, TQ_SIGNAL(tdedisplayPaletteChanged()), TQ_SLOT(paletteChanged()));
    connect(this, TQ_SIGNAL(tdedisplayStyleChanged()), TQ_SLOT(slotStyleChanged()));

#if (TQT_VERSION-0 >= 0x030200) // XRANDR support
    connect(desktop(), TQ_SIGNAL(resized(int)), TQ_SLOT(slotDesktopResized()));
#endif

    // the panels, aka extensions
    TQTimer::singleShot(0, ExtensionManager::the(), TQ_SLOT(initialize()));

    connect(ExtensionManager::the(), TQ_SIGNAL(desktopIconsAreaChanged(const TQRect &, int)),
            this, TQ_SLOT(slotDesktopIconsAreaChanged(const TQRect &, int)));
}

Kicker::~Kicker()
{
    // order of deletion here is critical to avoid crashes
    delete ExtensionManager::the();
    delete MenuManager::the();
}

void Kicker::setCrashHandler()
{
    TDECrash::setEmergencySaveFunction(Kicker::crashHandler);
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
    if (category == (int)TDEApplication::SETTINGS_SHORTCUTS)
    {
        keys->readSettings();
        keys->updateConnections();
    }
}

void Kicker::paletteChanged()
{
    TDEConfigGroup c(TDEGlobal::config(), "General");
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

    TDEConfig* c = TDEGlobal::config();
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
    TQTimer::singleShot(0, this, TQ_SLOT(slotRestart()));
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
    execv(TQFile::encodeName(locate("exe", "tdeinit_wrapper")), o_argv);

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
        args << "tde-panel.desktop";
    }
    else
    {
        args << "tde-kicker_config_arrangement.desktop"
             << "tde-kicker_config_hiding.desktop"
             << "tde-kicker_config_menus.desktop"
             << "tde-kicker_config_appearance.desktop";
    }
    args << "tde-kcmtaskbar.desktop";
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


void Kicker::showConfig(const TQString& configPath, const TQString& configFile, int page)
{
    if (!m_configDialog)
    {
         m_configDialog = new KCMultiDialog(0);

         TQStringList modules = configModules(false);
         TQStringList::ConstIterator end(modules.end());
         int moduleNumber = 0;
         for (TQStringList::ConstIterator it = modules.begin(); it != end; ++it)
         {
            if (configFile == "")
            {
                m_configDialog->addModule(*it);
            }
            else
            {
                if (moduleNumber == page)
                {
                    TQStringList argList;
                    argList << configFile;
                    m_configDialog->addModule(*it, true, argList);
                }
                else
                {
                    m_configDialog->addModule(*it);
                }
            }
            moduleNumber++;
         }

         connect(m_configDialog, TQ_SIGNAL(finished()), TQ_SLOT(configDialogFinished()));
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
        if ((configFile == "") && (page != 4))
        {
            m_configDialog->showPage(0);
        }
        else {
            m_configDialog->showPage(page);
        }
    }
}

void Kicker::showTaskBarConfig()
{
    showConfig(TQString(), TQString(), 4);
}

void Kicker::hideTaskBarConfig()
{
    if (m_configDialog) {
        m_configDialog->close();
    }
}

void Kicker::reshowTaskBarConfig()
{
    m_reloadingConfigDialog = true;
    hideTaskBarConfig();
}

void Kicker::showTaskBarConfig(const TQString& configFile)
{
    showConfig(TQString(), configFile, 4);
}

void Kicker::configureMenubar()
{
    ExtensionManager::the()->configureMenubar(false);
}

void Kicker::configDialogFinished()
{
    m_configDialog->delayedDestruct();
    m_configDialog = 0;
    if (m_reloadingConfigDialog) {
        TQByteArray data;
        m_reloadingConfigDialog = false;
        tdeApp->dcopClient()->send("kicker", "kicker", "showTaskBarConfig()", data);
    }
}

void Kicker::slotDesktopResized()
{
    configure(); // reposition on the desktop
}

void Kicker::clearQuickStartMenu()
{
    MenuManager::the()->kmenu()->clearRecentMenuItems();
}

TWinModule* Kicker::twinModule()
{
    if (!m_twinModule)
    {
        m_twinModule = new TWinModule();
    }

    return m_twinModule;
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
