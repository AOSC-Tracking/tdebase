/*
 * dbusscreensaverservice.h
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
#ifndef KDESKTOP_LOCK_DBUS_SCREENSAVER_DBUSSCREENSAVERSERVICE_H_
#define KDESKTOP_LOCK_DBUS_SCREENSAVER_DBUSSCREENSAVERSERVICE_H_

#include <tqmap.h>
#include <tqdbusconnection.h>

#include "screensaverNode.h"
#include "dbusbaseNode.h"
#include "screensaverInterfaceImpl.h"

class ScreenSaverService: public org::freedesktop::screensaverNode
{
    friend class TDEDbusScreenSaver;

public:
    ScreenSaverService(TQT_DBusConnection&);
    virtual ~ScreenSaverService();

protected:
    virtual TQT_DBusObjectBase* createInterface(const TQString&);
    ScreenSaverInterfaceImpl *screenSaverInterface;

private:
    TQMap<TQString, TQT_DBusObjectBase*> m_interfaces;
    TQT_DBusConnection m_connection;
};

class RootNodeService: public DBusBaseNode
{
public:
    RootNodeService(TQT_DBusConnection&);
    virtual ~RootNodeService();
protected:
    virtual TQT_DBusObjectBase* createInterface(const TQString&);
private:
    TQMap<TQString, TQT_DBusObjectBase*> m_interfaces;
    TQT_DBusConnection m_connection;
};

class OrgNodeService: public DBusBaseNode
{
public:
    OrgNodeService(TQT_DBusConnection&);
    virtual ~OrgNodeService();
protected:
    virtual TQT_DBusObjectBase* createInterface(const TQString&);
private:
    TQMap<TQString, TQT_DBusObjectBase*> m_interfaces;
    TQT_DBusConnection m_connection;
};

class FreeDesktopNodeService: public DBusBaseNode
{
public:
    FreeDesktopNodeService(TQT_DBusConnection&);
    virtual ~FreeDesktopNodeService();
protected:
    virtual TQT_DBusObjectBase* createInterface(const TQString&);
private:
    TQMap<TQString, TQT_DBusObjectBase*> m_interfaces;
    TQT_DBusConnection m_connection;
};

class TDEDbusScreenSaver
{

public:
    TDEDbusScreenSaver();
    virtual ~TDEDbusScreenSaver();

    /*!
     * This function return information about connection status to the DBUS daemon.
     * \return boolean with the state of the connection to D-Bus
     * \retval true if connected
     * \retval false if disconnected
     */
    bool isConnectedToDBUS();

private:
    /*!
     * This function initialize the connection to the D-Bus daemon.
     * \return boolean with the result of the operation
     * \retval true if successfully configured D-Bus connection
     * \retval false if unsuccessful
     */
    bool configureService();

    /*!
     * This function is to close the connection to D-Bus
     * \return boolean with the result of the operation
     * \retval true if successfully unset D-Bus connection
     * \retval false if unsuccessful
     */
    bool unconfigureService();

private:
    TQT_DBusConnection m_connection;

    RootNodeService *rootService;
    OrgNodeService   *orgService;
    FreeDesktopNodeService *freeDesktopNodeService;
    ScreenSaverService *screenSaverService;

};

#endif /* KDESKTOP_LOCK_DBUS_SCREENSAVER_DBUSSCREENSAVERSERVICE_H_ */
