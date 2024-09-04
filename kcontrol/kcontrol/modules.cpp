/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

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

#include <unistd.h>
#include <sys/types.h>


#include <tqlabel.h>
#include <tqlayout.h>
#include <tqvbox.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kservicegroup.h>
#include <kprocess.h>
#include <qxembed.h>
#include <tdelocale.h>
#include <kstandarddirs.h>


#include "modules.h"
#include "modules.moc"
#include "global.h"
#include "proxywidget.h"
#include <tdecmoduleloader.h>
#include "kcrootonly.h"

#include <X11/Xlib.h>


template class TQPtrList<ConfigModule>;


ConfigModule::ConfigModule(const KService::Ptr &s)
  : TDECModuleInfo(s), _changed(false), _module(0), _embedWidget(0),
    _rootProcess(0), _embedFrame(0), _embedStack(0)
{
}

ConfigModule::~ConfigModule()
{
  deleteClient();
}

ProxyWidget *ConfigModule::module()
{
  if (_module)
    return _module;

  bool run_as_root = needsRootPrivileges() && (getuid() != 0);

  TDECModule *modWidget = 0;

  if (run_as_root && isHiddenByDefault())
     modWidget = new KCRootOnly(0, "root_only");
  else
      modWidget = TDECModuleLoader::loadModule(*this);

  if (modWidget)
    {

      _module = new ProxyWidget(modWidget, moduleName(), "", run_as_root);
      connect(_module, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(clientChanged(bool)));
      connect(_module, TQ_SIGNAL(closed()), this, TQ_SLOT(clientClosed()));
      connect(_module, TQ_SIGNAL(handbookRequest()), this, TQ_SIGNAL(handbookRequest()));
      connect(_module, TQ_SIGNAL(helpRequest()), this, TQ_SIGNAL(helpRequest()));
      connect(_module, TQ_SIGNAL(runAsRoot()), this, TQ_SLOT(runAsRoot()));

      return _module;
    }

  return 0;
}

void ConfigModule::deleteClient()
{
  if (_embedWidget)
    XKillClient(tqt_xdisplay(), _embedWidget->embeddedWinId());

  delete _rootProcess;
  _rootProcess = 0;

  delete _embedWidget;
  _embedWidget = 0;
  delete _embedStack;
  _embedStack = 0;
  delete _embedFrame;
  _embedFrame = 0;
  kapp->syncX();

  if(_module)
    _module->close(true);
  _module = 0;

  TDECModuleLoader::unloadModule(*this);
  _changed = false;
}

void ConfigModule::clientClosed()
{
  deleteClient();

  emit changed(this);
  emit childClosed();
}


void ConfigModule::clientChanged(bool state)
{
  setChanged(state);
  emit changed(this);
}


void ConfigModule::runAsRoot()
{
  if (!_module)
    return;

  delete _rootProcess;
  delete _embedWidget;
  delete _embedStack;

  // create an embed widget that will embed the
  // tdecmshell running as root
  TQWidget* parentWidget = _module->parentWidget();
  _embedFrame = new TQVBox( parentWidget );
  _embedFrame->setFrameStyle( TQFrame::Box | TQFrame::Raised );
  TQPalette pal( red );
  pal.setColor( TQColorGroup::Background, parentWidget->colorGroup().background() );
  _embedFrame->setPalette( pal );
  _embedFrame->setLineWidth( 2 );
  _embedFrame->setMidLineWidth( 2 );
  // cannot reparent anything else inside QXEmbed, so put the busy label separately
  _embedStack = new TQWidgetStack(_embedFrame);
  _embedWidget = new KControlEmbed(_embedStack);
  _module->hide();
  _embedFrame->show();
  TQLabel *_busy = new TQLabel(i18n("<big>Loading...</big>"), _embedStack);
  _busy->setAlignment(AlignCenter);
  _busy->setTextFormat(RichText);
  _busy->setGeometry(0,0, _module->width(), _module->height());
  _busy->show();
  _embedStack->raiseWidget(_busy);
  connect(_embedWidget, TQ_SIGNAL( windowEmbedded(WId)), TQ_SLOT( embedded()));

  // prepare the process to run the tdecmshell
  TQString cmd = service()->exec().stripWhiteSpace();
  bool kdeshell = false;
  if (cmd.left(5) == "tdesu")
    {
      cmd = TQString(cmd.remove(0,5)).stripWhiteSpace();
      // remove all tdesu switches
      while( cmd.length() > 1 && cmd[ 0 ] == '-' )
        {
          int pos = cmd.find( ' ' );
          cmd = TQString(cmd.remove( 0, pos )).stripWhiteSpace();
        }
    }

  if (cmd.left(8) == "tdecmshell")
    {
      cmd = TQString(cmd.remove(0,8)).stripWhiteSpace();
      kdeshell = true;
    }

  // run the process
  TQString tdesu = TDEStandardDirs::findExe("tdesu");
  if (!tdesu.isEmpty())
    {
      _rootProcess = new TDEProcess;
      *_rootProcess << tdesu;
      *_rootProcess << "--nonewdcop";
      // We have to disable the keep-password feature because
      // in that case the modules is started through tdesud and tdesu
      // returns before the module is running and that doesn't work.
      // We also don't have a way to close the module in that case.
      *_rootProcess << "--n"; // Don't keep password.
      if (kdeshell) {
         *_rootProcess << TQString("%1 %2 --embed %3 --lang %4").arg(locate("exe", "tdecmshell")).arg(cmd).arg(_embedWidget->winId()).arg(TDEGlobal::locale()->language());
      }
      else {
         *_rootProcess << TQString("%1 --embed %2 --lang %3").arg(cmd).arg(_embedWidget->winId()).arg( TDEGlobal::locale()->language() );
      }

      connect(_rootProcess, TQ_SIGNAL(processExited(TDEProcess*)), this, TQ_SLOT(rootExited(TDEProcess*)));

      if ( !_rootProcess->start(TDEProcess::NotifyOnExit) )
      {
          delete _rootProcess;
          _rootProcess = 0L;
      }

      return;
    }

  // clean up in case of failure
  delete _embedStack;
  _embedStack = 0;
  delete _embedFrame;
  _embedWidget = 0;
  _module->show();
}


void ConfigModule::rootExited(TDEProcess *)
{
  if (_embedWidget->embeddedWinId())
    XDestroyWindow(tqt_xdisplay(), _embedWidget->embeddedWinId());

  delete _embedWidget;
  _embedWidget = 0;

  delete _rootProcess;
  _rootProcess = 0;

  delete _module;
  _module=0;

  _changed = false;
  emit changed(this);
  emit childClosed();
}

void ConfigModule::embedded()
{
  _embedStack->raiseWidget(_embedWidget); // put it above the busy label
}

const TDEAboutData *ConfigModule::aboutData() const
{
  if (!_module) return 0;
  return _module->aboutData();
}


ConfigModuleList::ConfigModuleList()
{
  setAutoDelete(true);
  subMenus.setAutoDelete(true);
}

void ConfigModuleList::readDesktopEntries()
{
  readDesktopEntriesRecursive( KCGlobal::baseGroup() );
}

bool ConfigModuleList::readDesktopEntriesRecursive(const TQString &path)
{

  KServiceGroup::Ptr group = KServiceGroup::group(path);

  if (!group || !group->isValid()) return false;

  KServiceGroup::List list = group->entries(true, true);

  if( list.isEmpty() )
	  return false;

  Menu *menu = new Menu;
  subMenus.insert(path, menu);

  for( KServiceGroup::List::ConstIterator it = list.begin();
       it != list.end(); it++)
  {
     KSycocaEntry *p = (*it);
     if (p->isType(KST_KService))
     {
        KService *s = static_cast<KService*>(p);
        if (!kapp->authorizeControlModule(s->menuId()))
           continue;

        ConfigModule *module = new ConfigModule(s);
        if (module->library().isEmpty() || (module->needsTest() && !TDECModuleLoader::testModule(*module)))
        {
           delete module;
           continue;
        }

        append(module);
        menu->modules.append(module);
     }
     else if (p->isType(KST_KServiceGroup) && 
		     readDesktopEntriesRecursive(p->entryPath()) )
        	menu->submenus.append(p->entryPath());

  }
  return true;
}

TQPtrList<ConfigModule> ConfigModuleList::modules(const TQString &path)
{
  Menu *menu = subMenus.find(path);
  if (menu)
     return menu->modules;

  return TQPtrList<ConfigModule>();
}

TQStringList ConfigModuleList::submenus(const TQString &path)
{
  Menu *menu = subMenus.find(path);
  if (menu)
     return menu->submenus;

  return TQStringList();
}

TQString ConfigModuleList::findModule(ConfigModule *module)
{
  TQDictIterator<Menu> it(subMenus);
  Menu *menu;
  for(;(menu = it.current());++it)
  {
     if (menu->modules.containsRef(module))
        return it.currentKey();
  }
  return TQString::null;
}
