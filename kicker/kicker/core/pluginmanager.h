/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

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

#ifndef __PLUGIN_MANAGER_H__
#define __PLUGIN_MANAGER_H__

#include <tqmap.h>
#include <tqobject.h>
#include <tqstringlist.h>
#include <tdemacros.h>
#include <kstaticdeleter.h>

#include "appletinfo.h"

class AppletContainer;
class ExtensionContainer;
class KPanelApplet;
class KPanelExtension;
class TQPopupMenu;

class TDE_EXPORT PluginManager : public TQObject
{
    TQ_OBJECT

public:
    static PluginManager* the();
    static AppletInfo::List applets(bool sort = true, AppletInfo::List* list = 0);
    static AppletInfo::List extensions(bool sort = true, AppletInfo::List* list = 0);
    static AppletInfo::List builtinButtons(bool sort = true, AppletInfo::List* list = 0);
    static AppletInfo::List specialButtons(bool sort = true, AppletInfo::List* list = 0);

    AppletContainer* createAppletContainer(const TQString& desktopFile,
                                           bool isStartup,
                                           const TQString& configFile,
                                           TQPopupMenu* opMenu,
                                           TQWidget* parent,
                                           bool isImmutable = false);
    ExtensionContainer* createExtensionContainer(const TQString& desktopFile,
                                                 bool isStartup,
                                                 const TQString& configFile,
                                                 const TQString& extensionId);

    KPanelApplet* loadApplet(const AppletInfo& info, TQWidget* parent);
    KPanelExtension* loadExtension(const AppletInfo& info, TQWidget* parent);

    bool hasInstance(const AppletInfo&) const;

public slots:
    void clearUntrustedLists();

protected:
    static AppletInfo::List plugins(const TQStringList& desktopFiles,
                                    AppletInfo::AppletType,
                                    bool sort,
                                    AppletInfo::List* list);

private slots:
    void slotPluginDestroyed(TQObject* plugin);

private:
    friend class KStaticDeleter<PluginManager>;
    PluginManager();
    virtual ~PluginManager();

    AppletInfo::Dict _dict;
    static PluginManager* m_self;
    TQStringList m_untrustedApplets;
    TQStringList m_untrustedExtensions;
};

class LibUnloader : public TQObject
{
    TQ_OBJECT
public:
    static void unload( const TQString &libName );

private slots:
    void unload();

private:
    LibUnloader( const TQString &libName, TQObject *parent );

    TQString _libName;
};

#endif
