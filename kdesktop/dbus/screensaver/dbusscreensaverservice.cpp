/*
 * dbusscreensaverservice.cpp
 *
 *  (C) 2024 Emanoil Kotsev
 *  deloptes (AT) gmail.com
 *
 * tdebase Copyright (C) 2009 tdebase development team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <tdelocale.h>
#include <tqdbusobjectpath.h>

#include "dbusscreensaverservice.h"

RootNodeService::RootNodeService(TQT_DBusConnection &connection) :
        DBusBaseNode(), m_connection(connection)
{
    addChildNode("org");
    registerObject(m_connection, "/");
}

RootNodeService::~RootNodeService()
{
}

TQT_DBusObjectBase* RootNodeService::createInterface(const TQString& interfaceName)
{
    return (TQT_DBusObjectBase*) m_interfaces[interfaceName];
}

OrgNodeService::OrgNodeService(TQT_DBusConnection &connection) :
        DBusBaseNode(), m_connection(connection)
{
    addChildNode("freedesktop");
    registerObject(m_connection, "/org");
}

OrgNodeService::~OrgNodeService()
{
}

TQT_DBusObjectBase* OrgNodeService::createInterface(const TQString& interfaceName)
{
    return (TQT_DBusObjectBase*) m_interfaces[interfaceName];
}

FreeDesktopNodeService::FreeDesktopNodeService(TQT_DBusConnection &connection) :
        DBusBaseNode(), m_connection(connection)
{
    addChildNode("ScreenSaver");
    registerObject(m_connection, "/org/freedesktop");
}

FreeDesktopNodeService::~FreeDesktopNodeService()
{
}

TQT_DBusObjectBase* FreeDesktopNodeService::createInterface(const TQString& interfaceName)
{
    return (TQT_DBusObjectBase*) m_interfaces[interfaceName];
}

ScreenSaverService::ScreenSaverService(TQT_DBusConnection &conn) :
        org::freedesktop::screensaverNode(),
        m_connection(conn),
        screenSaverInterface(new ScreenSaverInterfaceImpl(conn))
{
    m_interfaces.insert("org.freedesktop.DBus.Introspectable", this);
    m_interfaces.insert("org.freedesktop.ScreenSaver", screenSaverInterface);
    registerObject(m_connection, DBUS_SCREENSAVER_SERVICE_PATH);
}

ScreenSaverService::~ScreenSaverService()
{
    if(screenSaverInterface)
    {
        screenSaverInterface->restoreState();
        delete screenSaverInterface;
    }
}

TQT_DBusObjectBase* ScreenSaverService::createInterface(const TQString& interfaceName)
{
    return (TQT_DBusObjectBase*) m_interfaces[interfaceName];
}



TDEDbusScreenSaver::TDEDbusScreenSaver()
{
    // open connection to DBus and configure the service
    if (!configureService())
    {
        tqDebug("Failed to configure the screen saver service");
    }
}

TDEDbusScreenSaver::~TDEDbusScreenSaver()
{
    // unconfigure the DBus service and close connection
    if (!unconfigureService())
    {
        tqDebug("Failed to properly close the screen saver service");
    }

    delete screenSaverService;
    delete freeDesktopNodeService;
    delete orgService;
    delete rootService;
}

bool TDEDbusScreenSaver::isConnectedToDBUS()
{
    return m_connection.isConnected();
}

bool TDEDbusScreenSaver::configureService()
{
      m_connection = TQT_DBusConnection::addConnection(TQT_DBusConnection::SessionBus, DBUS_SCREENSAVER_SERVICE);

        if (!m_connection.isConnected())
        {
            tqDebug(i18n("Failed to open connection to system message bus: %1").arg(m_connection.lastError().message()));
            return false;
        }

        // try to get a specific service name
        if (!m_connection.requestName(DBUS_SCREENSAVER_SERVICE_NAME))
        {
            tqWarning(i18n("Requesting name %1 failed. "
                    "The object will only be addressable through unique name '%2'").arg(
                            DBUS_SCREENSAVER_SERVICE_NAME).arg(m_connection.uniqueName()));
            return false;
        }

        rootService = new RootNodeService(m_connection);
        orgService = new OrgNodeService(m_connection);
        freeDesktopNodeService = new FreeDesktopNodeService(m_connection);
        screenSaverService = new ScreenSaverService(m_connection);
    return true;
}

bool TDEDbusScreenSaver::unconfigureService()
{
    screenSaverService->screenSaverInterface->restoreState(); // will restore the original state

    screenSaverService=nullptr;
    freeDesktopNodeService=nullptr;
    orgService=nullptr;
    rootService=nullptr;
    // close D-Bus connection
    m_connection.closeConnection(DBUS_SCREENSAVER_SERVICE);

    return true;
}
