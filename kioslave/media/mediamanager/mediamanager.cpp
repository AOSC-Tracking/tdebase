/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "mediamanager.h"

#include <config.h>
#include <tqtimer.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include <kdirnotify_stub.h>
#include <kstandarddirs.h>

#include "mediamanagersettings.h"

#include "fstabbackend.h"

#ifdef COMPILE_HALBACKEND
#include "halbackend.h"
#endif //COMPILE_HALBACKEND

#ifdef COMPILE_LINUXCDPOLLING
#include "linuxcdpolling.h"
#endif //COMPILE_LINUXCDPOLLING


MediaManager::MediaManager(const TQCString &obj)
    : KDEDModule(obj), m_dirNotify(m_mediaList)
{
    connect( &m_mediaList, TQT_SIGNAL(mediumAdded(const TQString&, const TQString&, bool)),
             TQT_SLOT(slotMediumAdded(const TQString&, const TQString&, bool)) );
    connect( &m_mediaList, TQT_SIGNAL(mediumRemoved(const TQString&, const TQString&, bool)),
             TQT_SLOT(slotMediumRemoved(const TQString&, const TQString&, bool)) );
    connect( &m_mediaList,
             TQT_SIGNAL(mediumStateChanged(const TQString&, const TQString&, bool, bool)),
             TQT_SLOT(slotMediumChanged(const TQString&, const TQString&, bool, bool)) );

    TQTimer::singleShot( 10, this, TQT_SLOT( loadBackends() ) );
}

MediaManager::~MediaManager()
{
    while ( !m_backends.isEmpty() )
    {
        BackendBase *b = m_backends.first();
        m_backends.remove( b );
        delete b;
    }
}

void MediaManager::loadBackends()
{
    m_mediaList.blockSignals(true);

    while ( !m_backends.isEmpty() )
    {
        BackendBase *b = m_backends.first();
        m_backends.remove( b );
        delete b;
    }

    mp_removableBackend = 0L;
    m_halbackend = 0L;
    m_fstabbackend = 0L;

#ifdef COMPILE_HALBACKEND
    if ( MediaManagerSettings::self()->halBackendEnabled() )
    {
        m_mediaList.blockSignals(false);
        m_halbackend = new HALBackend(m_mediaList, this);
        if (m_halbackend->InitHal())
        {
            m_backends.append( m_halbackend );
            m_fstabbackend = new FstabBackend(m_mediaList, true);
            m_backends.append( m_fstabbackend );
            // No need to load something else...
            m_mediaList.blockSignals(false);
            return;
        }
        else
        {
            delete m_halbackend;
            m_halbackend = 0;
            m_mediaList.blockSignals(true);
        }
    }
#endif // COMPILE_HALBACKEND

    mp_removableBackend = new RemovableBackend(m_mediaList);
    m_backends.append( mp_removableBackend );

#ifdef COMPILE_LINUXCDPOLLING
    if ( MediaManagerSettings::self()->cdPollingEnabled() )
    {
        m_backends.append( new LinuxCDPolling(m_mediaList) );
    }
#endif //COMPILE_LINUXCDPOLLING

    m_fstabbackend = new FstabBackend(m_mediaList);
    m_backends.append( m_fstabbackend );
    m_mediaList.blockSignals(false);
}


TQStringList MediaManager::fullList()
{
    TQPtrList<Medium> list = m_mediaList.list();

    TQStringList result;

    TQPtrList<Medium>::const_iterator it = list.begin();
    TQPtrList<Medium>::const_iterator end = list.end();
    for (; it!=end; ++it)
    {
        result+= (*it)->properties();
        result+= Medium::SEPARATOR;
    }

    return result;
}

TQStringList MediaManager::properties(const TQString &name)
{
    const Medium *m = m_mediaList.findByName(name);

    if (!m)
    {
        KURL u(name);
        kdDebug() << "Media::prop " << name << " " << u.isValid() << endl;
        if (u.isValid())
        {
            if (u.protocol() == "system")
            {
                TQString path = u.path();
                if (path.startsWith("/media/"))
                    path = path.mid(strlen("/media/"));
                m = m_mediaList.findByName(path);
                kdDebug() << "findByName " << path << m << endl;
            }
            else if (u.protocol() == "media")
            {
                m = m_mediaList.findByName(u.filename());
                kdDebug() << "findByName " << u.filename() << m << endl;
            }
            else if (u.protocol() == "file")
            {
                // look for the mount point
                TQPtrList<Medium> list = m_mediaList.list();
                TQPtrList<Medium>::const_iterator it = list.begin();
                TQPtrList<Medium>::const_iterator end = list.end();
                TQString path;

                for (; it!=end; ++it)
                {
                    path = KStandardDirs::realFilePath(u.path());
                    kdDebug() << "comparing " << (*it)->mountPoint()  << " " << path << " " <<  (*it)->deviceNode() << endl;
                    if ((*it)->mountPoint() == path || (*it)->deviceNode() == path) {
			m = *it;
			break;
                    }
                }
            }
        }
    }

    if (m)
        return m->properties();
    else
        return TQStringList();
}

TQStringList MediaManager::mountoptions(const TQString &name)
{
#ifdef COMPILE_HALBACKEND
    if (!m_halbackend)
        return TQStringList();
    return m_halbackend->mountoptions(name);
#else
    return TQStringList();
#endif
}

bool MediaManager::setMountoptions(const TQString &name, const TQStringList &options)
{
#ifdef COMPILE_HALBACKEND
    if (!m_halbackend)
        return false;
    return m_halbackend->setMountoptions(name, options);
#else
    return false;
#endif
}

TQString MediaManager::mount(const TQString &name)
{
#ifdef COMPILE_HALBACKEND
    if (!m_halbackend)
        return i18n("Feature only available with HAL");
    return m_halbackend->mount(name);
#else
    if ( !m_fstabbackend ) // lying :)
        return i18n("Feature only available with HAL");
    return m_fstabbackend->mount( name );
#endif
}

TQString MediaManager::unmount(const TQString &name)
{
#ifdef COMPILE_HALBACKEND
    if (!m_halbackend)
        return i18n("Feature only available with HAL");
    return m_halbackend->unmount(name);
#else
    if ( !m_fstabbackend ) // lying :)
        return i18n("Feature only available with HAL");
    return m_fstabbackend->unmount( name );
#endif
}

TQString MediaManager::decrypt(const TQString &name, const TQString &password)
{
#ifdef COMPILE_HALBACKEND
    if (!m_halbackend)
        return i18n("Feature only available with HAL");
    return m_halbackend->decrypt(name, password);
#else
    return i18n("Feature only available with HAL");
#endif
}

TQString MediaManager::undecrypt(const TQString &name)
{
#ifdef COMPILE_HALBACKEND
    if (!m_halbackend)
        return i18n("Feature only available with HAL");
    return m_halbackend->undecrypt(name);
#else
    return i18n("Feature only available with HAL");
#endif
}

TQString MediaManager::nameForLabel(const TQString &label)
{
    const TQPtrList<Medium> media = m_mediaList.list();

    TQPtrList<Medium>::const_iterator it = media.begin();
    TQPtrList<Medium>::const_iterator end = media.end();
    for (; it!=end; ++it)
    {
        const Medium *m = *it;

        if (m->prettyLabel()==label)
        {
            return m->name();
        }
    }

    return TQString::null;
}

ASYNC MediaManager::setUserLabel(const TQString &name, const TQString &label)
{
    m_mediaList.setUserLabel(name, label);
}

ASYNC MediaManager::reloadBackends()
{
    MediaManagerSettings::self()->readConfig();
    loadBackends();
}

bool MediaManager::removablePlug(const TQString &devNode, const TQString &label)
{
    if (mp_removableBackend)
    {
        return mp_removableBackend->plug(devNode, label);
    }
    return false;
}

bool MediaManager::removableUnplug(const TQString &devNode)
{
    if (mp_removableBackend)
    {
        return mp_removableBackend->unplug(devNode);
    }
    return false;
}

bool MediaManager::removableCamera(const TQString &devNode)
{
    if (mp_removableBackend)
    {
        return mp_removableBackend->camera(devNode);
    }
    return false;
}


void MediaManager::slotMediumAdded(const TQString &/*id*/, const TQString &name,
                                   bool allowNotification)
{
    kdDebug(1219) << "MediaManager::slotMediumAdded: " << name << endl;

    KDirNotify_stub notifier("*", "*");
    notifier.FilesAdded( KURL("media:/") );

    emit mediumAdded(name, allowNotification);
    emit mediumAdded(name);
}

void MediaManager::slotMediumRemoved(const TQString &/*id*/, const TQString &name,
                                     bool allowNotification)
{
    kdDebug(1219) << "MediaManager::slotMediumRemoved: " << name << endl;

    KDirNotify_stub notifier("*", "*");
    notifier.FilesRemoved( KURL("media:/"+name) );

    emit mediumRemoved(name, allowNotification);
    emit mediumRemoved(name);
}

void MediaManager::slotMediumChanged(const TQString &/*id*/, const TQString &name,
                                     bool mounted, bool allowNotification)
{
    kdDebug(1219) << "MediaManager::slotMediumChanged: " << name << endl;

    KDirNotify_stub notifier("*", "*");
    if (!mounted)
    {
        notifier.FilesRemoved( KURL("media:/"+name) );
    }
    notifier.FilesChanged( KURL("media:/"+name) );

    emit mediumChanged(name, allowNotification);
    emit mediumChanged(name);
}


extern "C" {
    KDE_EXPORT KDEDModule *create_mediamanager(const TQCString &obj)
    {
        KGlobal::locale()->insertCatalogue("kio_media");
        return new MediaManager(obj);
    }
}

#include "mediamanager.moc"
