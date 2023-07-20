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

#ifndef MODULES_H
#define MODULES_H

#include <tdecmoduleinfo.h>
#include <tqobject.h>
#include <tqdict.h>
#include <qxembed.h>

template<class ConfigModule> class TQPtrList;
class TQStringList;
class TDEAboutData;
class TDECModule;
class ProxyWidget;
class TDEProcess;
class QXEmbed;
class TQVBoxLayout;
class TQVBox;
class TQWidgetStack;

class ConfigModule : public TQObject, public TDECModuleInfo
{
  TQ_OBJECT

public:

  ConfigModule(const KService::Ptr &s);
  ~ConfigModule();

  bool isChanged() { return _changed; };
  void setChanged(bool changed) { _changed = changed; };

  bool isActive() { return _module != 0; };
  ProxyWidget *module();
  const TDEAboutData *aboutData() const;


public slots:

  void deleteClient();


private slots:

  void clientClosed();
  void clientChanged(bool state);
  void runAsRoot();
  void rootExited(TDEProcess *proc);
  void embedded();


signals:

  void changed(ConfigModule *module);
  void childClosed();
  void handbookRequest();
  void helpRequest();


private:
  
  bool         _changed;
  ProxyWidget *_module;
  QXEmbed     *_embedWidget;
  TDEProcess    *_rootProcess;
  TQVBox       *_embedFrame;
  TQWidgetStack *_embedStack;

};

class ConfigModuleList : public TQPtrList<ConfigModule>
{
public:

  ConfigModuleList();

  void readDesktopEntries();
  bool readDesktopEntriesRecursive(const TQString &path);

  /**
   * Returns all submenus of the submenu identified by path
   */
  TQPtrList<ConfigModule> modules(const TQString &path);
  
  /**
   * Returns all modules of the submenu identified by path
   */
  TQStringList submenus(const TQString &path);

  /**
   * Returns the path of the submenu the module is in
   */
  TQString findModule(ConfigModule *module);
 
protected:

  class Menu
  {
  public:
    TQPtrList<ConfigModule> modules;
    TQStringList submenus;
  };

  TQDict<Menu> subMenus;
};

class KControlEmbed : public QXEmbed
    {
    TQ_OBJECT
    public:
        KControlEmbed( TQWidget* w ) : QXEmbed( w ) {}
        virtual void windowChanged( WId w ) { if( w ) emit windowEmbedded( w ); }
    signals:
        void windowEmbedded( WId w );
    };

#endif
