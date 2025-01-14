/*****************************************************************

Copyright (c) 2000 the kicker authors. See file AUTHORS.

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

#include <time.h>

#include <tqregexp.h>
#include <tqstringlist.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdelocale.h>

#include "kickerSettings.h"

#include "recentapps.h"

RecentlyLaunchedApps& RecentlyLaunchedApps::the()
{
    static RecentlyLaunchedApps obj;
    return obj;
}

RecentlyLaunchedApps::RecentlyLaunchedApps()
{
    // set defaults
    m_nNumMenuItems = 0;
    m_bNeedToUpdate = false;
    m_bInitialised = false;
    init();
}

void RecentlyLaunchedApps::init()
{
    if (m_bInitialised)
    {
       return;
    }

    m_nNumMenuItems = 0;
    m_appInfos.clear();

    configChanged();

    TQStringList recentApps = KickerSettings::recentAppsStat();

    for (TQStringList::ConstIterator it = recentApps.begin(); 
         it != recentApps.end(); ++it )
    {
        TQRegExp re( "(\\d*) (\\d*) (.*)" );
        if (re.search(*it) != -1)
        {
            int nCount = re.cap(1).toInt();
            long lTime = re.cap(2).toLong();
            TQString szPath = re.cap(3);
            m_appInfos.append(RecentlyLaunchedAppInfo(
                szPath, nCount, time_t(lTime)));
        }
    }

    qHeapSort(m_appInfos);

    m_bInitialised = true;
}

void RecentlyLaunchedApps::configChanged()
{
    qHeapSort(m_appInfos);
}

void RecentlyLaunchedApps::save()
{
    TQStringList recentApps;

    for (TQValueList<RecentlyLaunchedAppInfo>::const_iterator it = 
            m_appInfos.constBegin(); it != m_appInfos.constEnd(); ++it)
    {
        recentApps.append(TQString("%1 %2 %3").arg((*it).getLaunchCount())
                                             .arg((*it).getLastLaunchTime())
                                             .arg((*it).getDesktopPath()));
    }

    KickerSettings::setRecentAppsStat(recentApps);
    KickerSettings::writeConfig();
}

void RecentlyLaunchedApps::appLaunched(const TQString& strApp)
{
    // Inform other applications (like the quickstarter applet)
    // that an application was started
    TQByteArray params;
    TQDataStream stream(params, IO_WriteOnly);
    stream << launchDCOPSignalSource() << strApp;
    tdeApp->dcopClient()->emitDCOPSignal("appLauncher",
        "serviceStartedByStorageId(TQString,TQString)", params);

    for (TQValueList<RecentlyLaunchedAppInfo>::iterator it = m_appInfos.begin();
         it != m_appInfos.end(); ++it)
    {
        if ((*it).getDesktopPath() == strApp)
        {
            (*it).increaseLaunchCount();
            (*it).setLastLaunchTime(time(0));
            qHeapSort(m_appInfos);
            return;
        }
    }

    m_appInfos.append(RecentlyLaunchedAppInfo(strApp, 1, time(0)));
    qHeapSort(m_appInfos);
}

void RecentlyLaunchedApps::getRecentApps(TQStringList& recentApps)
{
    recentApps.clear();

    int maximumNum = KickerSettings::numVisibleEntries();
    int i = 0;
    for (TQValueList<RecentlyLaunchedAppInfo>::const_iterator it =
            m_appInfos.constBegin();
         it != m_appInfos.constEnd() && i < maximumNum;
         ++it, ++i)
    {
        recentApps.append((*it).getDesktopPath());
    }
}

void RecentlyLaunchedApps::removeItem( const TQString& strName )
{
    for (TQValueList<RecentlyLaunchedAppInfo>::iterator it = m_appInfos.begin();
         it != m_appInfos.end(); ++it)
    {
        if ((*it).getDesktopPath() == strName)
        {
            m_appInfos.erase(it);
            return;
        }
    }
}

void RecentlyLaunchedApps::clearRecentApps()
{
    m_appInfos.clear();
}

TQString RecentlyLaunchedApps::caption() const
{
    return KickerSettings::recentVsOften() ?
           i18n("Recently Used Applications") :
           i18n("Most Used Applications");
}
