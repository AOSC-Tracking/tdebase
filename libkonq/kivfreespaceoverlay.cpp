/*  This file is part of the TDE libraries
    Copyright (C) 2013 Timothy Pearson
    Based on kivdirectoryoverlay.cpp

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#include <tqdict.h>
#include <tqpixmap.h>
#include <tqpainter.h>
#include <tqbitmap.h>
#include <tqimage.h>
#include <tqfile.h>
#include <tqtimer.h>

#include <tdefileivi.h>
#include <tdefileitem.h>
#include <tdeapplication.h>
#include <kdirlister.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <konq_settings.h>
#include <tdelocale.h>
#include <kdebug.h>

#ifdef HAVE_STATVFS
# include <sys/statvfs.h>
#else
# include <sys/mount.h>
# define statvfs statfs
# define f_frsize f_bsize
#endif

#include "kivfreespaceoverlay.h"

KIVFreeSpaceOverlay::KIVFreeSpaceOverlay(KFileIVI* freespace)
{
    m_freespace = freespace;
}

KIVFreeSpaceOverlay::~KIVFreeSpaceOverlay()
{
    //
}

void KIVFreeSpaceOverlay::start()
{
    if ( !m_freespace->item()->isReadable() ) {
        emit finished();
    }
    TQTimer::singleShot(0, this, TQ_SLOT(slotDisplay()));
}

void KIVFreeSpaceOverlay::timerEvent(TQTimerEvent *)
{
    //
}

void KIVFreeSpaceOverlay::slotDisplay()
{
    KFileItem* item = m_freespace->item();
    if (item) {
        if (item->mimetype().contains("_mounted")) {
            bool isLocal = false;
            KURL localURL = item->mostLocalURL(isLocal);
            if (!isLocal) {
                m_freespace->setOverlayProgressBar(-1);
            }
            else {
                TQString localPath = localURL.path();
                if (localPath != "") {
                    TDEIO::filesize_t m_total = 0;
                    TDEIO::filesize_t m_used = 0;
                    TDEIO::filesize_t m_free = 0;

                    struct statvfs vfs;
                    memset(&vfs, 0, sizeof(vfs));

                    if ( ::statvfs(TQFile::encodeName(localPath), &vfs) != -1 )
                    {
                        m_total = static_cast<TDEIO::filesize_t>(vfs.f_blocks) * static_cast<TDEIO::filesize_t>(vfs.f_frsize);
                        m_free = static_cast<TDEIO::filesize_t>(vfs.f_bavail) * static_cast<TDEIO::filesize_t>(vfs.f_frsize);
                        m_used = m_total - m_free;
                        m_freespace->setOverlayProgressBar((m_used/(m_total*1.0))*100.0);
                    }
                    else {
                        m_freespace->setOverlayProgressBar(-1);
                    }
                }
                else {
                    m_freespace->setOverlayProgressBar(-1);
                }
            }
        }
    }
    else {
        m_freespace->setOverlayProgressBar(-1);
    }

    emit finished();
}

#include "kivfreespaceoverlay.moc"
