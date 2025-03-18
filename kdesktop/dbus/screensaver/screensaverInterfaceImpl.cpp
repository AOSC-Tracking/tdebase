/*
 * screensaverInterfaceImpl.cpp
 *
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

#include "screensaverInterfaceImpl.h"

ScreenSaverInterfaceImpl::ScreenSaverInterfaceImpl(TQT_DBusConnection &conn) :
        m_connection(&conn), m_cookieCount(1), m_kdesktopdcoprefobj("kdesktop", "KScreensaverIface")
{
    m_screenSaverEnabled = screenSaverIsEnabled();
    tqDebug(TQString("ScreenSaver isEnabled = %1").arg(
            (m_screenSaverEnabled) ? "yes" : "no"));
}

ScreenSaverInterfaceImpl::~ScreenSaverInterfaceImpl()
{
}

void ScreenSaverInterfaceImpl::restoreState()
{
    // restore the state
    setScreenSaverState(m_screenSaverEnabled);
}

/*!
 * Implement virtual methods
 *
 */
void ScreenSaverInterfaceImpl::handleMethodReply(const TQT_DBusMessage &reply)
{
    m_connection->send(reply);
}

bool ScreenSaverInterfaceImpl::handleSignalSend(const TQT_DBusMessage &reply)
{
    return true;
}

TQString ScreenSaverInterfaceImpl::objectPath() const
{
    return TQString(DBUS_SCREENSAVER_SERVICE_PATH);
}

bool ScreenSaverInterfaceImpl::GetActive(bool &retval, TQT_DBusError &dbuserror)
{

    DCOPReply reply = m_kdesktopdcoprefobj.call("isBlanked");
    retval = false;
    if (!reply.isValid())
    {
        TQString e("ScreenSaverInterfaceImpl::GetActive(): there was some error using DCOP.");
        tqDebug(e);
        dbuserror = TQT_DBusError::stdFailed(e);
        return false;
    }
    else
    {
        if (!reply.get(retval))
        {
            TQString e("ScreenSaverInterfaceImpl::GetActive(): there was some error getting the value from DCOPReply");
            tqDebug(e);
            dbuserror = TQT_DBusError::stdFailed(e);
            return false;
        }
    }
    return true;
}

bool ScreenSaverInterfaceImpl::Lock(TQT_DBusError &dbuserror)
{

    DCOPReply reply = m_kdesktopdcoprefobj.call("lock");
    if (!reply.isValid())
    {
        TQString e("ScreenSaverInterfaceImpl::Lock: there was some error using DCOP.");
        tqDebug(e);
        dbuserror = TQT_DBusError::stdFailed(e);
        return false;
    }
    return true;
}

bool ScreenSaverInterfaceImpl::SetActive(bool &retval, bool b, TQT_DBusError &dbuserror)
{

    DCOPReply reply = m_kdesktopdcoprefobj.call("setBlankOnly", b);
    if (!reply.isValid())
    {
        retval = false;
        TQString e("ScreenSaverInterfaceImpl::SetActive: there was some error using DCOP.");
        tqDebug(e);
        dbuserror = TQT_DBusError::stdFailed(e);
        return false;
    }
    emitActiveChanged(true);
    retval = true;
    return true;
}

bool ScreenSaverInterfaceImpl::Inhibit(const TQString &application_name,
        const TQString &reason_for_inhibit, TQ_UINT32 &cookie, TQT_DBusError &dbuserror)
{

    //this is to make sure we have the actual state - it could have been changed meanwhile
    //however some other application like kplayer may have disabled the screensaver
    //when we call this function
    //m_screenSaverEnabled = screenSaverIsEnabled();
    if (m_screenSaverEnabled && m_cookies.isEmpty()) // disable only once
    {
        if (!setScreenSaverState(false))
        {
            dbuserror = TQT_DBusError::stdFailed(TQString("Failed to disable the screen saver"));
        }
        emitActiveChanged(true);
    }

    cookie = m_cookieCount++;
    ScreenSaverInterfaceImpl::Pair v;
    v.name = application_name;
    v.value = reason_for_inhibit;
    m_cookies[cookie] = v;
    tqDebug(TQString("Inhibit: cookie(%1), application(%2), reason(%3)")
            .arg(cookie).arg(m_cookies[cookie].name).arg(m_cookies[cookie].value).local8Bit());

    return true;
}

bool ScreenSaverInterfaceImpl::UnInhibit(TQ_UINT32 cookie, TQT_DBusError &dbuserror)
{
    tqDebug(TQString("UnInhibit: cookie(%1), application(%2), reason(%3)")
            .arg(cookie).arg(m_cookies[cookie].name).arg(m_cookies[cookie].value).local8Bit());

    m_cookies.remove(cookie);
    if (m_cookies.isEmpty())
    {
        // restore states when all applications finished
        if (!setScreenSaverState(m_screenSaverEnabled))
        {
            dbuserror = TQT_DBusError::stdFailed(TQString("Failed to switch ScreenSaver %1!")
                    .arg((m_screenSaverEnabled) ? "on" : "off"));
        }
    }
    return true;
}

bool ScreenSaverInterfaceImpl::screenSaverIsEnabled()
{
    DCOPReply reply = m_kdesktopdcoprefobj.call("isEnabled");
    bool on = false;
    if (!reply.isValid())
    {
        tqDebug("ScreenSaverInterfaceImpl::screenSaverIsEnabled(): there was some error using DCOP.");
    }
    else
    {
        if (!reply.get(on))
        {
            tqDebug("ScreenSaverInterfaceImpl::screenSaverIsEnabled(): there was some error getting the value from DCOPReply");
        }
    }
    return on;
}

bool ScreenSaverInterfaceImpl::setScreenSaverState(bool on)
{
    DCOPReply reply = m_kdesktopdcoprefobj.call("enable", on);
    if (!reply.isValid())
    {
        tqDebug(TQString("Failed to switch ScreenSaver %1!").arg(
                (on) ? "on" : "off"));
        return false;
    }
    return true;
}
// End of File
