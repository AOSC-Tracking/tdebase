/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

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

#ifndef __dockcontainer_h__
#define __dockcontainer_h__

#include <tqwidgetstack.h>
#include <tqvbox.h>

class ConfigModule;
class ModuleTitle;
class ProxyWidget;
class TQLabel;

class ModuleWidget : public TQVBox
{
  TQ_OBJECT

  public:
    ModuleWidget( TQWidget *parent, const char *name );
    ~ModuleWidget() {}

    ProxyWidget* load( ConfigModule *module );

  signals:
    void helpRequest();

  protected:
    ModuleTitle *m_title;
    TQVBox *m_body;
};

class DockContainer : public TQWidgetStack
{
  TQ_OBJECT

public:
  DockContainer(TQWidget *parent=0);
  virtual ~DockContainer();

  void setBaseWidget(TQWidget *widget);
  TQWidget *baseWidget() { return _basew; }

  bool dockModule(ConfigModule *module);
  ConfigModule *module() { return _module; }

public slots:
  void removeModule();

protected slots:
  void quickHelpChanged();
  void slotHelpRequest();

protected:
  void deleteModule();
  ProxyWidget* loadModule( ConfigModule *module );

signals:
  void newModule(const TQString &name, const TQString& docPath, const TQString &quickhelp);
  void changedModule(ConfigModule *module);

private:
  TQWidget      *_basew;
  TQLabel       *_busyw;
  ModuleWidget *_modulew;
  ConfigModule *_module;

};

#endif
