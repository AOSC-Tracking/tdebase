/*
 * screensaverInterfaceImpl.h
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
#ifndef KDESKTOP_SCREENSAVERINTERFACEIMPL_H_
#define KDESKTOP_SCREENSAVERINTERFACEIMPL_H_

#include <dcopref.h>
#include <tqpair.h>
#include <tqmap.h>
#include <tqdbusconnection.h>

#include "screensaverInterface.h"

#define DBUS_SCREENSAVER_SERVICE      "TDEDbusScreenSaver"
#define DBUS_SCREENSAVER_SERVICE_NAME "org.freedesktop.ScreenSaver"
#define DBUS_SCREENSAVER_SERVICE_PATH "/org/freedesktop/ScreenSaver"

class ScreenSaverInterfaceImpl : public org::freedesktop::ScreenSaverInterface
{
    /**
     *
     * Idle inhibition is achieved by the application calling an Inhibit method on a well-known D-Bus name.
     *
     * Inhibition will stop when the UnInhibit method is called, or the application disconnects from the
     * D-Bus session bus (which usually happens upon exit).
     * Implementations of this well-known bus name must have an object /org/freedesktop/ScreenSaver which
     * implements the org.freedesktop.ScreenSaver interface.
     *
     * https://specifications.freedesktop.org/idle-inhibit-spec/latest/ch03.html
     */

public:
    ScreenSaverInterfaceImpl(TQT_DBusConnection&);
    virtual ~ScreenSaverInterfaceImpl();

    void restoreState();

protected:

    /**
     * void Lock()
     *       This method gets called when the service daemon
     *       locks the screen
     */
    virtual bool Lock(TQT_DBusError& error);

    /**
     * Name:           SetActive
     * Args:           DBUS_TYPE_BOOLEAN state
     *                  state:  TRUE to request activation,
     *                          FALSE to request deactivation
     * Returns:        (nothing)
     * Description:    Request a change in the state of the screensaver.
     *                 Set to TRUE to request that the screensaver activate.
     *                 Active means that the screensaver has blanked the
     *                 screen and may run a graphical theme.  This does
     *                 not necessary mean that the screen is locked.
     *
     * Not implemented
     */
    virtual bool SetActive(bool& arg0, bool e, TQT_DBusError& error);

    /**
     *
     *  Name:           Inhibit
     *  Args:           DBUS_TYPE_STRING "application-name"
     *                      A unique identifier for the application, usually a reverse domain
     *                      (such as 'org.freedesktop.example').
     *                  DBUS_TYPE_STRING "reason for inhibit"
     *                      A human-readable and possibly translated string
     *                      explaining the reason why idleness is inhibited
     *                      (such as 'Playing a movie').
     *  Returns:        INT cookie
     *                  This is a random number used to identify the request.
     *                  To be passed to UnInhibit when done.
     *  Description:    Request that saving the screen due to system idleness
     *                  be blocked until UnInhibit is called or the
     *                  calling process exits.
     *
     * https://specifications.freedesktop.org/idle-inhibit-spec/latest/re01.html
     * https://lists.freedesktop.org/archives/xdg/2007-March/009187.html
     */

    virtual bool Inhibit(const TQString& application_name, const TQString& reason_for_inhibit, TQ_UINT32& cookie, TQT_DBusError& error);

    /**
     *
     * Name:           UnInhibit
     * Args:           DBUS_TYPE_UINT32 cookie
     *                      A cookie representing the inhibition request,
     *                      as returned by the 'Inhibit' function.
     * Returns:        (nothing)
     * Description:    Cancel a previous call to Inhibit() identified by the cookie.
     */
    virtual bool UnInhibit(TQ_UINT32 cookie, TQT_DBusError& error);

    virtual void handleMethodReply(const TQT_DBusMessage& reply);
    virtual bool handleSignalSend(const TQT_DBusMessage& reply);
    virtual TQString objectPath() const;

private:
    bool screenSaverIsEnabled();
    bool forceScreenSaver(bool);

    struct Pair {
        TQString name;
        TQString value;
    };

    TQT_DBusConnection *m_connection;
    TQMap<TQ_UINT32,Pair> m_cookies;
    TQ_UINT32 m_ccount;

    DCOPRef m_kdesktopdcoprefobj;
    bool isScreenSaverEnabled;
};

#endif /* KDESKTOP_SCREENSAVERINTERFACEIMPL_H_ */
