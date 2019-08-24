/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>
   Linux CD/DVD detection
   Copyright (c) 2005 Bernhard Rosenkraenzer <bero arklinux org>

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

#include "fstabbackend.h"

#ifdef __linux__
// For CD/DVD drive detection
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#define CDROM_GET_CAPABILITY    0x5331
#define CDSL_CURRENT            ((int) (~0U>>1))
#define CDC_DVD_R               0x10000 /* drive can write DVD-R */
#define CDC_DVD_RAM             0x20000 /* drive can write DVD-RAM */
#define CDC_CD_R                0x2000  /* drive is a CD-R */
#define CDC_CD_RW               0x4000  /* drive is a CD-RW */
#define CDC_DVD                 0x8000  /* drive is a DVD */
#include <tqfile.h>
#endif

#include <tdelocale.h>
#include <tdeio/job.h>
#include <tdeio/netaccess.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <kurl.h>
#include <kmountpoint.h>
#include <kstandarddirs.h>

#ifdef _OS_SOLARIS_
#define FSTAB "/etc/vfstab"
#define MTAB "/etc/mnttab"
#else
#define FSTAB "/etc/fstab"
#define MTAB "/etc/mtab"
#endif



FstabBackend::FstabBackend(MediaList &list, bool networkSharesOnly)
	: TQObject(), BackendBase(list), m_networkSharesOnly(networkSharesOnly)
{
	KDirWatch::self()->addFile(MTAB);
	KDirWatch::self()->addFile(FSTAB);

	connect( KDirWatch::self(), TQT_SIGNAL( dirty(const TQString&) ),
	         this, TQT_SLOT( slotDirty(const TQString&) ) );

	handleFstabChange(false);
	handleMtabChange(false);

	KDirWatch::self()->startScan();

#if defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
	connect( &m_mtabTimer, TQT_SIGNAL( timeout() ),
	         this, TQT_SLOT( handleMtabChange() ) );
	m_mtabTimer.start(250);
#endif
}

FstabBackend::~FstabBackend()
{
	TQStringList::iterator it = m_mtabIds.begin();
	TQStringList::iterator end = m_mtabIds.end();

	for (; it!=end; ++it)
	{
		m_mediaList.removeMedium(*it, false);
	}

	it = m_fstabIds.begin();
	end = m_fstabIds.end();

	for (; it!=end; ++it)
	{
		m_mediaList.removeMedium(*it, false);
	}
        KDirWatch::self()->removeFile(FSTAB);
        KDirWatch::self()->removeFile(MTAB);
}

TQStringVariantMap FstabBackend::mount(const TQString &id)
{
		TQStringVariantMap result;
    const Medium *medium = m_mediaList.findById(id);
    if (!medium) {
			result["errStr"] = i18n("No such medium: %1").arg(id);
			result["result"] = false;
			return result;
		}
    TDEIO::Job *job = TDEIO::mount(false, 0, medium->deviceNode(), medium->mountPoint());
    TDEIO::NetAccess::synchronousRun(job, 0);
		result["result"] = true;
		return result;
}

TQStringVariantMap FstabBackend::unmount(const TQString &id)
{
		TQStringVariantMap result;
    const Medium *medium = m_mediaList.findById(id);
    if (!medium) {
			result["errStr"] = i18n("No such medium: %1").arg(id);
			result["result"] = false;
			return result;
		}
    TDEIO::Job *job = TDEIO::unmount(medium->mountPoint(), false);
    TDEIO::NetAccess::synchronousRun(job, 0);
		result["result"] = true;
		return result;
}

void FstabBackend::slotDirty(const TQString &path)
{
	if (path==MTAB)
	{
		handleMtabChange();
	}
	else if (path==FSTAB)
	{
		handleFstabChange();
	}
}

bool inExclusionPattern(KMountPoint *mount, bool networkSharesOnly)
{
	if ( mount->mountType() == "swap"
	  || mount->mountType() == "tmpfs"
	  || mount->mountType() == "sysfs"
	  || mount->mountType() == "fdescfs"
	  || mount->mountType() == "kernfs"
	  || mount->mountType() == "usbfs"
	  || mount->mountType().contains( "proc" )
	  || mount->mountType() == "unknown"
	  || mount->mountType() == "none"
	  || mount->mountType() == "sunrpc"
	  || mount->mountedFrom() == "none"
	  || mount->mountedFrom() == "tmpfs"
	  || mount->mountedFrom().find("shm") != -1
	  || mount->mountPoint() == "/dev/swap"
	  || mount->mountPoint() == "/dev/pts"
	  || mount->mountPoint().find("/proc") == 0
	  || mount->mountPoint().find("/sys") == 0

	  // We might want to display only network shares
	  // since HAL doesn't handle them
	  || ( networkSharesOnly
	    && mount->mountType().find( "smb" ) == -1
	    && mount->mountType().find( "cifs" ) == -1
	    && mount->mountType().find( "nfs" ) == -1
	     )
	   )
	{
		return true;
	}
	else
	{
		return false;
	}
}


void FstabBackend::handleMtabChange(bool allowNotification)
{
	TQStringList new_mtabIds;
	KMountPoint::List mtab = KMountPoint::currentMountPoints(KMountPoint::NeedRealDeviceName);

	KMountPoint::List::iterator it = mtab.begin();
	KMountPoint::List::iterator end = mtab.end();

	for (; it!=end; ++it)
	{
		TQString dev = (*it)->mountedFrom();
		TQString mp = (*it)->mountPoint();
		TQString fs = (*it)->mountType();

		if ( ::inExclusionPattern(*it, m_networkSharesOnly) ) continue;

		/* Did we know this already before ? If yes, then
		   nothing has changed, do not stat the mount point. Avoids
		   hang if network shares are stalling */
		TQString mtabEntry = dev + "*" + mp + "*" + fs;
		if(m_mtabEntries.contains(mtabEntry)) {
		        new_mtabIds += m_mtabEntries[mtabEntry];
			continue;
		}

		TQString id = generateId(dev, mp);
		new_mtabIds+=id;
		m_mtabEntries[mtabEntry] = id;

		if ( !m_mtabIds.contains(id) && m_fstabIds.contains(id) )
		{
			TQString mime, icon, label;
			guess(dev, mp, fs, true, mime, icon, label);
			m_mediaList.changeMediumState(id, true, false,
			                              mime, icon, label);
		}
#if 0
		else if ( !m_mtabIds.contains(id) )
		{
			TQString name = generateName(dev, fs);

			Medium *m = new Medium(id, name);

			m->setMountable(true);
			m->setDeviceNode(dev);
			m->setMountPoint(mp);
			m->setFsType(fs);
			m->setMounted(true);

			TQString mime, icon, label;
			guess(dev, mp, fs, true, mime, icon, label);

			m->setMimeType(mime);
			m->setIconName(icon);
			m->setLabel(label);

			m_mediaList.addMedium(m, notificationAllowed);
		}
#endif
	}

	TQStringList::iterator it2 = m_mtabIds.begin();
	TQStringList::iterator end2 = m_mtabIds.end();

	for (; it2!=end2; ++it2)
	{
		if ( !new_mtabIds.contains(*it2) && m_fstabIds.contains(*it2) )
		{
			const Medium *medium = m_mediaList.findById(*it2);

			TQString dev = medium->deviceNode();
			TQString mp = medium->mountPoint();
			TQString fs = medium->fsType();


			TQString mtabEntry = dev + "*" + mp + "*" + fs;
			m_mtabEntries.remove(mtabEntry);

			TQString mime, icon, label;
			guess(dev, mp, fs, false, mime, icon, label);

			m_mediaList.changeMediumState(*it2, false, false,
			                              mime, icon, label);
		}
#if 0
		else if ( !new_mtabIds.contains(*it2) )
		{
			m_mediaList.removeMedium(*it2, allowNotification);
		}
#endif
	}

	m_mtabIds = new_mtabIds;
}

void FstabBackend::handleFstabChange(bool allowNotification)
{
	TQStringList new_fstabIds;
	KMountPoint::List fstab = KMountPoint::possibleMountPoints();

	KMountPoint::List::iterator it = fstab.begin();
	KMountPoint::List::iterator end = fstab.end();

	for (; it!=end; ++it)
	{
		TQString dev = (*it)->mountedFrom();
		TQString mp = (*it)->mountPoint();
		TQString fs = (*it)->mountType();

		if ( ::inExclusionPattern(*it, m_networkSharesOnly) ) continue;

		TQString id = generateId(dev, mp);
		new_fstabIds+=id;

		if ( !m_fstabIds.contains(id) )
		{
			TQString name = generateName(dev, fs);

			Medium *m = new Medium(id, id, name);

			m->setMountable(true);
			m->setDeviceNode(dev);
			m->setMountPoint(mp);
			m->setFsType(fs);
			m->setMounted(false);

			TQString mime, icon, label;
			guess(dev, mp, fs, false, mime, icon, label);

			m->setMimeType(mime);
			m->setIconName(icon);
			m->setLabel(label);

			m_mediaList.addMedium(m, allowNotification);
		}
	}

	TQStringList::iterator it2 = m_fstabIds.begin();
	TQStringList::iterator end2 = m_fstabIds.end();

	for (; it2!=end2; ++it2)
	{
		if ( !new_fstabIds.contains(*it2) )
		{
			m_mediaList.removeMedium(*it2, allowNotification);
		}
	}

	m_fstabIds = new_fstabIds;
}

TQString FstabBackend::generateId(const TQString &devNode,
                                 const TQString &mountPoint)
{
	TQString d = TDEStandardDirs::realFilePath(devNode);
	TQString m = TDEStandardDirs::realPath(mountPoint);

	return "/org/kde/mediamanager/fstab/"
	      +d.replace("/", "")
	      +m.replace("/", "");
}

TQString FstabBackend::generateName(const TQString &devNode, const TQString &fsType)
{
	KURL url( devNode );

	if ( url.isValid() )
	{
		return url.fileName();
	}
	else // surely something nfs or samba based
	{
		return fsType;
	}
}

void FstabBackend::guess(const TQString &devNode, const TQString &mountPoint,
                         const TQString &fsType, bool mounted,
                         TQString &mimeType, TQString &iconName, TQString &label)
{
	enum { UNKNOWN, CD, CDWRITER, DVD, DVDWRITER } devType = UNKNOWN;
#ifdef __linux__
	// Guessing device types by mount point is not exactly accurate...
	// Do something accurate first, and fall back if necessary.
	int device=open(TQFile::encodeName(devNode), O_RDONLY|O_NONBLOCK);
	if(device>=0)
	{
		bool isCd=false;
		TQString devname=devNode.section('/', -1);
		if(devname.startsWith("scd") || devname.startsWith("sr"))
		{
			// SCSI CD/DVD drive
			isCd=true;
		}
		else if(devname.startsWith("hd"))
		{
			// IDE device -- we can't tell if this is a
			// CD/DVD drive or harddisk by just looking at the
			// filename
			TQFile m(TQString("/proc/ide/") + devname + "/media");
			if(m.open(IO_ReadOnly))
			{
				TQString buf;
				m.readLine(buf, 1024);
				if(buf.contains("cdrom"))
					isCd=true;
				m.close();
			}
		}
		if(isCd)
		{
			int drv=ioctl(device, CDROM_GET_CAPABILITY, CDSL_CURRENT);
			if(drv>=0)
			{
				if((drv & CDC_DVD_R) || (drv & CDC_DVD_RAM))
					devType = DVDWRITER;
				else if((drv & CDC_CD_R) || (drv & CDC_CD_RW))
					devType = CDWRITER;
				else if(drv & CDC_DVD)
					devType = DVD;
				else
					devType = CD;
			}
		}
		close(device);
	}
#endif
	if ( devType == CDWRITER
	  || devNode.find("cdwriter")!=-1 || mountPoint.find("cdwriter")!=-1
	  || devNode.find("cdrecorder")!=-1 || mountPoint.find("cdrecorder")!=-1
	  || devNode.find("cdburner")!=-1 || mountPoint.find("cdburner")!=-1
	  || devNode.find("cdrw")!=-1 || mountPoint.find("cdrw")!=-1
	  || devNode.find("graveur")!=-1
	   )
	{
		mimeType = "media/cdwriter";
		label = i18n("CD Recorder");
	}
	else if ( devType == DVD || devType == DVDWRITER
	       || devNode.find("dvd")!=-1 || mountPoint.find("dvd")!=-1 )
	{
		mimeType = "media/dvd";
		label = i18n("DVD");
	}
	else if ( devType == CD
	       || devNode.find("cdrom")!=-1 || mountPoint.find("cdrom")!=-1
	       // LINUX SPECIFIC
	       || devNode.find("/dev/scd")!=-1 || devNode.find("/dev/sr")!=-1
	       // FREEBSD SPECIFIC
	       || devNode.find("/acd")!=-1 || devNode.find("/scd")!=-1
	       // NETBSD SPECIFIC
	       || devNode.find("/cd")!=-1 || devNode.find("/cd")!=-1
	        )
	{
		mimeType = "media/cdrom";
		label = i18n("CD-ROM");
	}
	else if ( devNode.find("fd")!=-1 || mountPoint.find("fd")!=-1
	       || devNode.find("floppy")!=-1 || mountPoint.find("floppy")!=-1 )
	{
		if ( devNode.find("360")!=-1 || devNode.find("1200")!=-1 )
		{
			mimeType = "media/floppy5";
		}
		else
		{
			mimeType = "media/floppy";
		}
		label = i18n("Floppy");
	}
	else if ( mountPoint.find("zip")!=-1
	       // FREEBSD SPECIFIC
	       || devNode.find("/afd")!=-1
	        )
	{
		mimeType = "media/zip";
		label = i18n("Zip Disk");
	}
	else if ( mountPoint.find("removable")!=-1
	       || mountPoint.find("hotplug")!=-1
	       || mountPoint.find("usb")!=-1
	       || mountPoint.find("firewire")!=-1
	       || mountPoint.find("ieee1394")!=-1
	       || devNode.find("/usb/")!= -1
	        )
	{
		mimeType = "media/removable";
		label = i18n("Removable Device");
	}
	else if ( fsType.find("nfs")!=-1 )
	{
		mimeType = "media/nfs";
		label = i18n("Remote Share");
	}
	else if ( fsType.find("smb")!=-1 || fsType.find("cifs")!=-1
	       || devNode.find("//")!=-1 )
	{
		mimeType = "media/smb";
		label = i18n("Remote Share");
	}
	else
	{
		mimeType = "media/hdd";
		label = i18n("Hard Disk");
	}

	if ( mimeType=="media/nfs" || mimeType=="media/smb" )
	{
		label+= " (" + devNode + ")";
	}
	else
	{
		TQString tmp = devNode;
		if ( tmp.startsWith("/dev/") )
		{
			tmp = tmp.mid(5);
		}
		label+= " (" + tmp + ")";
	}
	mimeType+= (mounted ? "_mounted" : "_unmounted");
	iconName = TQString::null;
}

#include "fstabbackend.moc"
