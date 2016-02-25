/* This file is part of the KDE Project
   Copyright (c) 2004-2005 Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>
   Copyright (c) 2006 Valentine Sinitsyn <e_val@inbox.ru>

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

#include "halbackend.h"
#include "linuxcdpolling.h"

#include <stdlib.h>
#include <locale.h>

#include <tdeapplication.h>
#include <tdemessagebox.h>
#include <tqeventloop.h>
#include <tqfile.h>
#include <tdelocale.h>
#include <kurl.h>
#include <kdebug.h>
#include <kprocess.h>
#include <tdeconfig.h>
#include <tqstylesheet.h>
#include <kmountpoint.h>
#include <tdemessagebox.h>
#include <tdeio/job.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <kprocess.h>

#define MOUNT_SUFFIX    (                                                                       \
    (medium->isMounted() ? TQString("_mounted") : TQString("_unmounted")) +   \
    (medium->isEncrypted() ? (halClearVolume ? "_decrypted" : "_encrypted") : "" )          \
    )
#define MOUNT_ICON_SUFFIX   (                                                              \
    (medium->isMounted() ? TQString("_mount") : TQString("_unmount")) +   \
    (medium->isEncrypted() ? (halClearVolume ? "_decrypt" : "_encrypt") : "" )      \
    )
#define MOUNTED_ICON_SUFFIX   (                                                              \
    (medium->isMounted() ? TQString("-mounted") : TQString("")) +   \
    (medium->isEncrypted() ? (halClearVolume ? "-decrypted" : "-encrypted") : "" )      \
    )

/* Static instance of this class, for static HAL callbacks */
static HALBackend* s_HALBackend;

/* A macro function to convert HAL string properties to TQString */
TQString libhal_device_get_property_QString(LibHalContext *ctx, const char* udi, const char *key)
{
    char*   _ppt_string;
    TQString _ppt_QString;
    _ppt_string = libhal_device_get_property_string(ctx, udi, key, NULL);
    if ( _ppt_string )
        _ppt_QString = _ppt_string;
    libhal_free_string(_ppt_string);
    return _ppt_QString;
}

/* Constructor */
HALBackend::HALBackend(MediaList &list, TQObject* parent)
    : TQObject()
    , BackendBase(list)
    , m_halContext(NULL)
    , m_halStoragePolicy(NULL)
    , m_parent(parent)
{
    s_HALBackend = this;
}

/* Destructor */
HALBackend::~HALBackend()
{
    /* Close HAL connection */
    if (m_halContext)
    {
        const TQPtrList<Medium> medlist = m_mediaList.list();
        TQPtrListIterator<Medium> it (medlist);
        for ( const Medium *current_medium = it.current(); current_medium; current_medium = ++it)
        {
            if( !current_medium->id().startsWith( "/org/kde" ))
                unmount(current_medium->id());
        }


        /* Remove all the registered media first */
        int numDevices;
        char** halDeviceList = libhal_get_all_devices( m_halContext, &numDevices, NULL );

        if ( halDeviceList )
        {
            for ( int i = 0; i < numDevices; i++ )
            {
                m_mediaList.removeMedium( halDeviceList[i], false );
            }
        }

        libhal_free_string_array( halDeviceList );

        DBusError error;
        dbus_error_init(&error);
        libhal_ctx_shutdown(m_halContext, &error);
        libhal_ctx_free(m_halContext);
    }

    if (m_halStoragePolicy)
        libhal_storage_policy_free(m_halStoragePolicy);
}

/* Connect to the HAL */
bool HALBackend::InitHal()
{
    kdDebug(1219) << "Context new" << endl;
    m_halContext = libhal_ctx_new();
    if (!m_halContext)
    {
        kdDebug(1219) << "Failed to initialize HAL!" << endl;
        return false;
    }

    // Main loop integration
    kdDebug(1219) << "Main loop integration" << endl;
    DBusError error;
    dbus_error_init(&error);
    dbus_connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);

    if (!dbus_connection || dbus_error_is_set(&error)) {
        dbus_error_free(&error);
        libhal_ctx_free(m_halContext);
        m_halContext = NULL;
        return false;
    }

    dbus_connection_set_exit_on_disconnect (dbus_connection, FALSE);

    MainLoopIntegration(dbus_connection);
    libhal_ctx_set_dbus_connection(m_halContext, dbus_connection);

    // HAL callback functions
    kdDebug(1219) << "Callback functions" << endl;
    libhal_ctx_set_device_added(m_halContext, HALBackend::hal_device_added);
    libhal_ctx_set_device_removed(m_halContext, HALBackend::hal_device_removed);
    libhal_ctx_set_device_new_capability (m_halContext, NULL);
    libhal_ctx_set_device_lost_capability (m_halContext, NULL);
    libhal_ctx_set_device_property_modified (m_halContext, HALBackend::hal_device_property_modified);
    libhal_ctx_set_device_condition(m_halContext, HALBackend::hal_device_condition);

    kdDebug(1219) << "Context Init" << endl;
    if (!libhal_ctx_init(m_halContext, &error))
    {
        if (dbus_error_is_set(&error))
            dbus_error_free(&error);
        libhal_ctx_free(m_halContext);
        m_halContext = NULL;
        kdDebug(1219) << "Failed to init HAL context!" << endl;
        return false;
    }

    /** @todo customize watch policy */
    kdDebug(1219) << "Watch properties" << endl;
    if (!libhal_device_property_watch_all(m_halContext, &error))
    {
        kdDebug(1219) << "Failed to watch HAL properties!" << endl;
        return false;
    }

    /* libhal-storage initialization */
    kdDebug(1219) << "Storage Policy" << endl;
    m_halStoragePolicy = libhal_storage_policy_new();
    /** @todo define libhal-storage icon policy */

    /* List devices at startup */
    return ListDevices();
}

/* List devices (at startup)*/
bool HALBackend::ListDevices()
{
    kdDebug(1219) << "ListDevices" << endl;

    int numDevices;
    char** halDeviceList = libhal_get_all_devices(m_halContext, &numDevices, NULL);

    if (!halDeviceList)
        return false;

    kdDebug(1219) << "HALBackend::ListDevices : " << numDevices << " devices found" << endl;
    for (int i = 0; i < numDevices; i++)
        AddDevice(halDeviceList[i], false);

    libhal_free_string_array( halDeviceList );

    return true;
}

/* Create a media instance for the HAL device "udi".
   This functions checks whether the device is worth listing */
void HALBackend::AddDevice(const char *udi, bool allowNotification)
{
    /* We don't deal with devices that do not expose their capabilities.
       If we don't check this, we will get a lot of warning messages from libhal */
    if (!libhal_device_property_exists(m_halContext, udi, "info.capabilities", NULL))
        return;

    /* If the device is already listed, do not process.
       This should not happen, but who knows... */
    /** @todo : refresh properties instead ? */
    if (m_mediaList.findById(udi))
        return;

    if (libhal_device_get_property_bool(m_halContext, "/org/freedesktop/Hal/devices/computer", "storage.disable_volume_handling", NULL))
        allowNotification=false;

    /* Add volume block devices */
    if (libhal_device_query_capability(m_halContext, udi, "volume", NULL))
    {
        /* We only list volumes that...
         *  - are encrypted with LUKS or
         *  - have a filesystem or
         *  - have an audio track
         */
        if ( ( libhal_device_get_property_QString(m_halContext, udi, "volume.fsusage") != "crypto" ||
               libhal_device_get_property_QString(m_halContext, udi, "volume.fstype") != "crypto_LUKS"
             ) &&
             libhal_device_get_property_QString(m_halContext, udi, "volume.fsusage") != "filesystem" &&
             !libhal_device_get_property_bool(m_halContext, udi, "volume.disc.has_audio", NULL) &&
             !libhal_device_get_property_bool(m_halContext, udi, "volume.disc.is_blank", NULL) )
            return;

        /* Query drive udi */
        TQString driveUdi = libhal_device_get_property_QString(m_halContext, udi, "block.storage_device");
        if ( driveUdi.isNull() ) // no storage - no fun
            return;

        // if the device is locked do not act upon it
        if (libhal_device_get_property_bool(m_halContext, driveUdi.ascii(), "info.locked", NULL))
            allowNotification=false;

        // if the device is locked do not act upon it
        if (libhal_device_get_property_bool(m_halContext, driveUdi.ascii(), "storage.partition_table_changed", NULL))
            allowNotification=false;

        /** @todo check exclusion list **/

        /* Special handling for clear crypto volumes */
        LibHalVolume* halVolume = libhal_volume_from_udi(m_halContext, udi);
        if (!halVolume)
            return;
        const char* backingVolumeUdi = libhal_volume_crypto_get_backing_volume_udi(halVolume);
        if ( backingVolumeUdi != NULL )
        {
            /* The crypto drive was unlocked and may now be mounted... */
            kdDebug(1219) << "HALBackend::AddDevice : ClearVolume appeared for " << backingVolumeUdi << endl;
            ResetProperties(backingVolumeUdi, allowNotification);
            libhal_volume_free(halVolume);
            return;
        }
        libhal_volume_free(halVolume);

        /* Create medium */
        Medium* medium = new Medium(udi, udi, "");
        setVolumeProperties(medium);

        if ( isInFstab( medium ).isNull() )
        {
            // if it's not mountable by user and not by HAL, don't show it at all
            if ( ( libhal_device_get_property_QString(m_halContext, udi, "volume.fsusage") == "filesystem" &&
                   !libhal_device_get_property_bool(m_halContext, udi, "volume.is_mounted", NULL ) ) &&
                 ( libhal_device_get_property_bool(m_halContext, udi, "volume.ignore", NULL ) ) )
            {
                delete medium;
                return;
            }
        }
        
	// instert medium into list
	m_mediaList.addMedium(medium, allowNotification);

	// finally check for automount
        TQMap<TQString,TQString> options = MediaManagerUtils::splitOptions(mountoptions(udi));
        kdDebug() << "automount " << options["automount"] << endl;
        if (options["automount"] == "true" && allowNotification ) {
            TQString error = mount(medium);
            if (!error.isEmpty())
                kdDebug() << "error " << error << endl;
        }

        return;
    }

    /* Floppy & zip drives */
    if (libhal_device_query_capability(m_halContext, udi, "storage", NULL))
        if ((libhal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "floppy") ||
            (libhal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "zip") ||
            (libhal_device_get_property_QString(m_halContext, udi, "storage.drive_type") == "jaz"))
        {
            if (! libhal_device_get_property_bool(m_halContext, udi, "storage.removable.media_available", NULL) )
                allowNotification = false;
            /* Create medium */
            Medium* medium = new Medium(udi, udi, "");
            // if the storage has a volume, we ignore it
            if ( setFloppyProperties(medium) )
                m_mediaList.addMedium(medium, allowNotification);
            else
                delete medium;
            return;
        }

    /* Camera handled by gphoto2*/
    if (libhal_device_query_capability(m_halContext, udi, "camera", NULL) &&
        ((libhal_device_get_property_QString(m_halContext, udi, "camera.access_method")=="ptp") ||

         (libhal_device_property_exists(m_halContext, udi, "camera.libgphoto2.support", NULL) &&
          libhal_device_get_property_bool(m_halContext, udi, "camera.libgphoto2.support", NULL)))
        )
    {
        /* Create medium */
        Medium* medium = new Medium(udi, udi, "");
        setCameraProperties(medium);
        m_mediaList.addMedium(medium, allowNotification);
        return;
    }
}

void HALBackend::RemoveDevice(const char *udi)
{
    const Medium *medium = m_mediaList.findByClearUdi(udi);
    if (medium) {
        ResetProperties(medium->id().ascii());
    } else {
        m_mediaList.removeMedium(udi, true);
    }
}

void HALBackend::ModifyDevice(const char *udi, const char* key)
{
    kdDebug(1219) << "HALBackend::ModifyDevice for '" << udi << "' on '" << key << "'\n";

    const char* mediumUdi = findMediumUdiFromUdi(udi);
    if (!mediumUdi)
        return;
    bool allowNotification = false;
    if (strcmp(key, "storage.removable.media_available") == 0)
        allowNotification = libhal_device_get_property_bool(m_halContext, udi, key, NULL);
    ResetProperties(mediumUdi, allowNotification);
}

void HALBackend::DeviceCondition(const char* udi, const char* condition)
{
    TQString conditionName = TQString(condition);
    kdDebug(1219) << "Processing device condition " << conditionName << " for " << udi << endl;

    if (conditionName == "EjectPressed") {
        const Medium* medium = m_mediaList.findById(udi);
        if (!medium) {
	    /* the ejectpressed appears on the drive and we need to find the volume */
	    const TQPtrList<Medium> medlist = m_mediaList.list();
	    TQPtrListIterator<Medium> it (medlist);
	    for ( const Medium *current_medium = it.current(); current_medium; current_medium = ++it)
            {
                if( current_medium->id().startsWith( "/org/kde" ))
                    continue;
		TQString driveUdi = libhal_device_get_property_QString(m_halContext, current_medium->id().latin1(), "block.storage_device");
		if (driveUdi == udi)
                {
		    medium = current_medium;
		    break;
                }
            }
        }
        if (medium) {
	    TDEProcess p;
	    p << "tdeio_media_mounthelper" << "-e" << medium->name();
	    p.start(TDEProcess::DontCare);
        }
    }

    const char* mediumUdi = findMediumUdiFromUdi(udi);
    kdDebug() << "findMedumUdiFromUdi " << udi << " returned " << mediumUdi << endl;
    if (!mediumUdi)
        return;

    /* TODO: Warn the user that (s)he should unmount devices before unplugging */
    if (conditionName == "VolumeUnmountForced")
        ResetProperties(mediumUdi);

    /* Reset properties after mounting */
    if (conditionName == "VolumeMount")
        ResetProperties(mediumUdi);

    /* Reset properties after unmounting */
    if (conditionName == "VolumeUnmount")
        ResetProperties(mediumUdi);

}

void HALBackend::MainLoopIntegration(DBusConnection *dbusConnection)
{
    m_dBusQtConnection = new DBusQt::Connection(m_parent);
    m_dBusQtConnection->dbus_connection_setup_with_qt_main(dbusConnection);
}

/******************************************
 ** Properties attribution                **
 ******************************************/

/* Return the medium udi that should be updated when recieving a call for
   device udi */
const char* HALBackend::findMediumUdiFromUdi(const char* udi)
{
    /* Easy part : this Udi is already registered as a device */
    const Medium* medium = m_mediaList.findById(udi);
    if (medium)
        return medium->id().ascii();

    /* Hard part : this is a volume whose drive is registered */
    if (libhal_device_property_exists(m_halContext, udi, "info.capabilities", NULL))
        if (libhal_device_query_capability(m_halContext, udi, "volume", NULL))
        {
            /* check if this belongs to an encrypted volume */
            LibHalVolume* halVolume = libhal_volume_from_udi(m_halContext, udi);
            if (!halVolume) return NULL;
            const char* backingUdi = libhal_volume_crypto_get_backing_volume_udi(halVolume);
            if (backingUdi != NULL) {
                const char* result = findMediumUdiFromUdi(backingUdi);
                libhal_volume_free(halVolume);
                return result;
            }
            libhal_volume_free(halVolume);

            /* this is a volume whose drive is registered */
            TQString driveUdi = libhal_device_get_property_QString(m_halContext, udi, "block.storage_device");
            return findMediumUdiFromUdi(driveUdi.ascii());
        }

    return NULL;
}

void HALBackend::ResetProperties(const char* mediumUdi, bool allowNotification)
{
    kdDebug(1219) << "HALBackend::setProperties" << endl;
    if ( TQString::fromLatin1( mediumUdi ).startsWith( "/org/kde/" ) )
    {
        const Medium *cmedium = m_mediaList.findById(mediumUdi);
        if ( cmedium )
        {
            Medium m( *cmedium );
            if ( setFstabProperties( &m ) ) {
                kdDebug() << "setFstabProperties worked" << endl;
                m_mediaList.changeMediumState(m, allowNotification);
            }
            return;
       }
    }

    Medium* m = new Medium(mediumUdi, mediumUdi, "");

    if (libhal_device_query_capability(m_halContext, mediumUdi, "volume", NULL))
        setVolumeProperties(m);
    if (libhal_device_query_capability(m_halContext, mediumUdi, "storage", NULL))
        setFloppyProperties(m);
    if (libhal_device_query_capability(m_halContext, mediumUdi, "camera", NULL))
        setCameraProperties(m);

    m_mediaList.changeMediumState(*m, allowNotification);

    delete m;
}

void HALBackend::setVolumeProperties(Medium* medium)
{
    kdDebug(1219) << "HALBackend::setVolumeProperties for " << medium->id() << endl;

    const char* udi = medium->id().ascii();
    /* Check if the device still exists */
    if (!libhal_device_exists(m_halContext, udi, NULL))
        return;

    /* Get device information from libhal-storage */
    LibHalVolume* halVolume = libhal_volume_from_udi(m_halContext, udi);
    if (!halVolume)
        return;
    TQString driveUdi = libhal_volume_get_storage_device_udi(halVolume);
    LibHalDrive*  halDrive  = 0;
    if ( !driveUdi.isNull() )
        halDrive = libhal_drive_from_udi(m_halContext, driveUdi.ascii());
    if (!halDrive) {
        // at times HAL sends an UnmountForced event before the device is removed
        libhal_volume_free(halVolume);
        return;
    }

    medium->setName(
        generateName(libhal_volume_get_device_file(halVolume)) );

    LibHalVolume* halClearVolume = NULL;
    if ( libhal_device_get_property_QString(m_halContext, udi, "volume.fsusage") == "crypto" )
    {
        kdDebug(1219) << "HALBackend::setVolumeProperties : crypto volume" << endl;

        medium->setEncrypted(true);
        char* clearUdi = libhal_volume_crypto_get_clear_volume_udi(m_halContext, halVolume);
	TQString clearUdiString;
        if (clearUdi != NULL) {
            kdDebug(1219) << "HALBackend::setVolumeProperties : crypto clear volume avail - " << clearUdi << endl;
            halClearVolume = libhal_volume_from_udi(m_halContext, clearUdi);
            // ignore if halClearVolume is NULL -> just not decrypted in this case
	    clearUdiString = clearUdi;
	    libhal_free_string(clearUdi);
        }

        if (halClearVolume)
            medium->mountableState(
                libhal_volume_get_device_file(halVolume),		/* Device node */
                clearUdiString,
                libhal_volume_get_mount_point(halClearVolume),		/* Mount point */
                libhal_volume_get_fstype(halClearVolume),		/* Filesystem type */
                libhal_volume_is_mounted(halClearVolume) );		/* Mounted ? */
        else
            medium->mountableState(
                libhal_volume_get_device_file(halVolume),		/* Device node */
                TQString::null,
                TQString::null,		/* Mount point */
                TQString::null,		/* Filesystem type */
                false );		/* Mounted ? */
    }
    else
    {
        kdDebug(1219) << "HALBackend::setVolumeProperties : normal volume" << endl;
        medium->mountableState(
            libhal_volume_get_device_file(halVolume),		/* Device node */
            TQString::fromUtf8(libhal_volume_get_mount_point(halVolume)),	/* Mount point */
            libhal_volume_get_fstype(halVolume),		/* Filesystem type */
            libhal_volume_is_mounted(halVolume) );		/* Mounted ? */
    }


    char* name = libhal_volume_policy_compute_display_name(halDrive, halVolume, m_halStoragePolicy);
    TQString volume_name = TQString::fromUtf8(name);
    TQString media_name = volume_name;
    /* media_name contains something like "501M Removable Media" or "Blank CD-R"
       The former needs special handling for correct translation
    */
    if (media_name.find(TQRegExp("^[0-9]+\\.?[0-9]*[KMGT] (Removable )?Media$")) > -1) {
        TQString pattern = media_name.section(" ", 1);
        media_name.replace(pattern, i18n(pattern.utf8()));
        medium->setLabel(media_name);
    } else {
        medium->setLabel(i18n(media_name.utf8()));
    }

    free(name);

    TQString mimeType;
    if (libhal_volume_is_disc(halVolume))
    {
        mimeType = "media/cdrom" + MOUNT_SUFFIX;

        LibHalVolumeDiscType discType = libhal_volume_get_disc_type(halVolume);
        if ((discType == LIBHAL_VOLUME_DISC_TYPE_CDROM) ||
            (discType == LIBHAL_VOLUME_DISC_TYPE_CDR) ||
            (discType == LIBHAL_VOLUME_DISC_TYPE_CDRW))
            if (libhal_volume_disc_is_blank(halVolume))
            {
                mimeType = "media/blankcd";
                medium->unmountableState("");
            }
            else
                mimeType = "media/cdwriter" + MOUNT_SUFFIX;

        if ((discType == LIBHAL_VOLUME_DISC_TYPE_DVDROM) || (discType == LIBHAL_VOLUME_DISC_TYPE_DVDRAM) ||
            (discType == LIBHAL_VOLUME_DISC_TYPE_DVDR) || (discType == LIBHAL_VOLUME_DISC_TYPE_DVDRW) ||
            (discType == LIBHAL_VOLUME_DISC_TYPE_DVDPLUSR) || (discType == LIBHAL_VOLUME_DISC_TYPE_DVDPLUSRW) )
            if (libhal_volume_disc_is_blank(halVolume))
            {
                mimeType = "media/blankdvd";
                medium->unmountableState("");
            }
            else
                mimeType = "media/dvd" + MOUNT_SUFFIX;

        if (libhal_volume_disc_has_audio(halVolume) && !libhal_volume_disc_has_data(halVolume))
        {
            mimeType = "media/audiocd";
            medium->unmountableState( "audiocd:/?device=" + TQString(libhal_volume_get_device_file(halVolume)) );
        }

        medium->setIconName(TQString::null);

        /* check if the disc id a vcd or a video dvd */
        if (libhal_device_get_property_bool(m_halContext, udi, "volume.disc.is_vcd", NULL)) {
            mimeType = "media/vcd";
        }
        else if (libhal_device_get_property_bool(m_halContext, udi, "volume.disc.is_svcd", NULL)) {
            mimeType = "media/svcd";
        }
        else if (libhal_device_get_property_bool(m_halContext, udi, "volume.disc.is_videodvd", NULL)) {
            mimeType = "media/dvdvideo";
        }

    }
    else
    {
        mimeType = "media/hdd" + MOUNT_SUFFIX;
        medium->setIconName(TQString::null); // reset icon
        if (libhal_drive_is_hotpluggable(halDrive))
        {
            mimeType = "media/removable" + MOUNT_SUFFIX;
            medium->needMounting();
            switch (libhal_drive_get_type(halDrive)) {
            case LIBHAL_DRIVE_TYPE_COMPACT_FLASH:
                medium->setIconName("media-flash-compact_flash" + MOUNTED_ICON_SUFFIX);
                break;
            case LIBHAL_DRIVE_TYPE_MEMORY_STICK:
                medium->setIconName("media-flash-memory_stick" + MOUNTED_ICON_SUFFIX);
                break;
            case LIBHAL_DRIVE_TYPE_SMART_MEDIA:
                medium->setIconName("media-flash-smart_media" + MOUNTED_ICON_SUFFIX);
                break;
            case LIBHAL_DRIVE_TYPE_SD_MMC:
                medium->setIconName("media-flash-sd_mmc" + MOUNTED_ICON_SUFFIX);
                break;
            case LIBHAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER:
            {
                medium->setIconName("ipod" + MOUNT_ICON_SUFFIX);

                if (libhal_device_get_property_QString(m_halContext, driveUdi.latin1(), "info.product") == "iPod" &&
		         KProtocolInfo::isKnownProtocol( TQString("ipod") ) )
                {
                    medium->unmountableState( "ipod:/" );
                    medium->mountableState(  libhal_volume_is_mounted(halVolume) );
                }
                break;
            }
            case LIBHAL_DRIVE_TYPE_CAMERA:
            {
                mimeType = "media/camera" + MOUNT_SUFFIX;
                const char *physdev = libhal_drive_get_physical_device_udi(halDrive);
                // get model from camera
                if (physdev && libhal_device_query_capability(m_halContext, physdev, "camera", NULL))
                {
                    if (libhal_device_property_exists(m_halContext, physdev, "usb_device.product", NULL))
                        medium->setLabel(libhal_device_get_property_QString(m_halContext, physdev, "usb_device.product"));
                    else if (libhal_device_property_exists(m_halContext, physdev, "usb.product", NULL))
                        medium->setLabel(libhal_device_get_property_QString(m_halContext, physdev, "usb.product"));
                }
                break;
            }
            case LIBHAL_DRIVE_TYPE_TAPE:
                medium->setIconName("media-tape" + MOUNTED_ICON_SUFFIX);
                break;
            default:
                medium->setIconName(TQString::null);
            }

            if (medium->isMounted() && TQFile::exists(medium->mountPoint() + "/dcim"))
            {
                mimeType = "media/camera" + MOUNT_SUFFIX;
            }
        }
    }
    medium->setMimeType(mimeType);

    libhal_drive_free(halDrive);
    libhal_volume_free(halVolume);
}

bool HALBackend::setFstabProperties( Medium *medium )
{
    TQString mp = isInFstab(medium);

    if (!mp.isNull() && !medium->id().startsWith( "/org/kde" ) )
    {
        // now that we know it's in fstab, we have to find out if it's mounted
        KMountPoint::List mtab = KMountPoint::currentMountPoints();

        KMountPoint::List::iterator it = mtab.begin();
        KMountPoint::List::iterator end = mtab.end();

        bool mounted = false;

        for (; it!=end; ++it)
        {
            if ((*it)->mountedFrom() == medium->deviceNode() && (*it)->mountPoint() == mp )
            {
                mounted = true;
                break;
            }
        }

        kdDebug() << mp << " " << mounted << " " << medium->deviceNode() << " " <<  endl;
        TQString fstype = medium->fsType();
        if ( fstype.isNull() )
            fstype = "auto";

        medium->mountableState(
            medium->deviceNode(),
            mp,							/* Mount point */
            fstype,						/* Filesystem type */
            mounted );						/* Mounted ? */

        return true;
    }

    return false;

}

// Handle floppies and zip drives
bool HALBackend::setFloppyProperties(Medium* medium)
{
    kdDebug(1219) << "HALBackend::setFloppyProperties for " << medium->id() << endl;

    const char* udi = medium->id().ascii();
    /* Check if the device still exists */
    if (!libhal_device_exists(m_halContext, udi, NULL))
        return false;

    LibHalDrive*  halDrive  = libhal_drive_from_udi(m_halContext, udi);
    if (!halDrive)
        return false;

    TQString drive_type = libhal_device_get_property_QString(m_halContext, udi, "storage.drive_type");

    if (drive_type == "zip") {
        int numVolumes;
        char** volumes = libhal_drive_find_all_volumes(m_halContext, halDrive, &numVolumes);
        libhal_free_string_array(volumes);
        kdDebug(1219) << " found " << numVolumes << " volumes" << endl;
        if (numVolumes)
        {
            libhal_drive_free(halDrive);
            return false;
        }
    }

    medium->setName( generateName(libhal_drive_get_device_file(halDrive)) );
    medium->setLabel(i18n("Unknown Drive"));

    // HAL hates floppies - so we have to do it twice ;(
    medium->mountableState(libhal_drive_get_device_file(halDrive), TQString::null, TQString::null, false);
    setFloppyMountState(medium);

    if (drive_type == "floppy")
    {
        if (medium->isMounted()) // don't use _SUFFIX here as it accesses the volume
            medium->setMimeType("media/floppy_mounted" );
        else
            medium->setMimeType("media/floppy_unmounted");
        medium->setLabel(i18n("Floppy Drive"));
    }
    else if (drive_type == "zip") 
    {
        if (medium->isMounted())
            medium->setMimeType("media/zip_mounted" );
        else
            medium->setMimeType("media/zip_unmounted");
        medium->setLabel(i18n("Zip Drive"));
    }

    /** @todo And mimtype for JAZ drives ? */

    medium->setIconName(TQString::null);

    libhal_drive_free(halDrive);

    return true;
}

void HALBackend::setFloppyMountState( Medium *medium )
{
    if ( !medium->id().startsWith( "/org/kde" ) )
    {
        KMountPoint::List mtab = KMountPoint::currentMountPoints();
        KMountPoint::List::iterator it = mtab.begin();
        KMountPoint::List::iterator end = mtab.end();

        TQString fstype;
        TQString mountpoint;
        for (; it!=end; ++it)
        {
            if ((*it)->mountedFrom() == medium->deviceNode() )
            {
                fstype = (*it)->mountType().isNull() ? (*it)->mountType() : "auto";
                mountpoint = (*it)->mountPoint();
                medium->mountableState( medium->deviceNode(), mountpoint, fstype, true );
                return;
            }
        }
    }
}

void HALBackend::setCameraProperties(Medium* medium)
{
    kdDebug(1219) << "HALBackend::setCameraProperties for " << medium->id() << endl;

    const char* udi = medium->id().ascii();
    /* Check if the device still exists */
    if (!libhal_device_exists(m_halContext, udi, NULL))
        return;

    /** @todo find name */
    medium->setName("camera");

    TQString device = "camera:/";

    char *cam = libhal_device_get_property_string(m_halContext, udi, "camera.libgphoto2.name", NULL);
    DBusError error;
    dbus_error_init(&error);
    if (cam &&
        libhal_device_property_exists(m_halContext, udi, "usb.linux.device_number", NULL) &&
        libhal_device_property_exists(m_halContext, udi, "usb.bus_number", NULL))
        device.sprintf("camera://%s@[usb:%03d,%03d]/", cam,
                       libhal_device_get_property_int(m_halContext, udi, "usb.bus_number", &error),
                       libhal_device_get_property_int(m_halContext, udi, "usb.linux.device_number", &error));

    libhal_free_string(cam);

    /** @todo find the rest of this URL */
    medium->unmountableState(device);
    medium->setMimeType("media/gphoto2camera");
    medium->setIconName(TQString::null);
    if (libhal_device_property_exists(m_halContext, udi, "usb_device.product", NULL))
        medium->setLabel(libhal_device_get_property_QString(m_halContext, udi, "usb_device.product"));
    else if (libhal_device_property_exists(m_halContext, udi, "usb.product", NULL))
        medium->setLabel(libhal_device_get_property_QString(m_halContext, udi, "usb.product"));
    else
        medium->setLabel(i18n("Camera"));
}

TQString HALBackend::generateName(const TQString &devNode)
{
    return KURL(devNode).fileName();
}

/******************************************
 ** HAL CALL-BACKS                        **
 ******************************************/

void HALBackend::hal_device_added(LibHalContext *ctx, const char *udi)
{
    kdDebug(1219) << "HALBackend::hal_device_added " << udi <<  endl;
    Q_UNUSED(ctx);
    s_HALBackend->AddDevice(udi);
}

void HALBackend::hal_device_removed(LibHalContext *ctx, const char *udi)
{
    kdDebug(1219) << "HALBackend::hal_device_removed " << udi << endl;
    Q_UNUSED(ctx);
    s_HALBackend->RemoveDevice(udi);
}

void HALBackend::hal_device_property_modified(LibHalContext *ctx, const char *udi,
                                              const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
    kdDebug(1219) << "HALBackend::hal_property_modified " << udi << " -- " << key << endl;
    Q_UNUSED(ctx);
    Q_UNUSED(is_removed);
    Q_UNUSED(is_added);
    s_HALBackend->ModifyDevice(udi, key);
}

void HALBackend::hal_device_condition(LibHalContext *ctx, const char *udi,
                                      const char *condition_name,
                                      const char* message
    )
{
    kdDebug(1219) << "HALBackend::hal_device_condition " << udi << " -- " << condition_name << endl;
    Q_UNUSED(ctx);
    Q_UNUSED(message);
    s_HALBackend->DeviceCondition(udi, condition_name);
}

TQStringList HALBackend::getHALmountoptions(TQString udi)
{
    const char*   _ppt_string;
    LibHalVolume* volume;
    LibHalDrive* drive;

    TQString _ppt_TQString;

    volume = libhal_volume_from_udi( m_halContext, udi.latin1() );
    if( volume )
        drive = libhal_drive_from_udi( m_halContext, libhal_volume_get_storage_device_udi( volume ) );
    else
        drive = libhal_drive_from_udi( m_halContext, udi.latin1() );

    if( !drive )
           return TQString::null;

    if( volume )
       _ppt_string = libhal_volume_policy_get_mount_options ( drive, volume, NULL );
    else
       _ppt_string = libhal_drive_policy_get_mount_options ( drive, NULL );

    _ppt_TQString = TQString(_ppt_string ? _ppt_string : "");

   return TQStringList::split(",",_ppt_TQString);
}

TQStringList HALBackend::mountoptions(const TQString &name)
{
    const Medium* medium = m_mediaList.findById(name);
    if (!medium)
    	return TQStringList(); // we don't know about that one
    if (!isInFstab(medium).isNull())
        return TQStringList(); // not handled by HAL - fstab entry

    TQString volume_udi = name;
    if (medium->isEncrypted()) {
        // see if we have a clear volume
        LibHalVolume* halVolume = libhal_volume_from_udi(m_halContext, medium->id().latin1());
        if (halVolume) {
            char* clearUdi = libhal_volume_crypto_get_clear_volume_udi(m_halContext, halVolume);
            if (clearUdi != NULL) {
	        volume_udi = clearUdi;
		libhal_free_string(clearUdi);
	    } else {
	        // if not decrypted yet then no mountoptions
		return TQStringList();
	    }
            libhal_volume_free(halVolume);
        } else {
	    // strange...
	    return TQStringList();
	}
    }

    TDEConfig config("mediamanagerrc");
    
    bool use_defaults = true;    
    if (config.hasGroup(name)) 
    {
	config.setGroup(name);
	use_defaults = config.readBoolEntry("use_defaults", false);
    }
    
    if (use_defaults)
	config.setGroup("DefaultOptions");    

    char ** array = libhal_device_get_property_strlist(m_halContext, volume_udi.latin1(), "volume.mount.valid_options", NULL);
    TQMap<TQString,bool> valids;

    for (int index = 0; array && array[index]; ++index) {
        TQString t = array[index];
        if (t.endsWith("="))
            t = t.left(t.length() - 1);
        valids[t] = true;
        kdDebug() << "valid " << t << endl;
    }
    libhal_free_string_array(array);
    TQStringList result;
    TQString tmp;
    
    result << TQString("use_defaults=%1").arg(use_defaults ? "true" : "false");    

    TQString fstype = libhal_device_get_property_QString(m_halContext, volume_udi.latin1(), "volume.fstype");
    if (fstype.isNull())
        fstype = libhal_device_get_property_QString(m_halContext, volume_udi.latin1(), "volume.policy.mount_filesystem");

    TQString drive_udi = libhal_device_get_property_QString(m_halContext, volume_udi.latin1(), "block.storage_device");

    bool removable = false;
    if ( !drive_udi.isNull() )
        removable = libhal_device_get_property_bool(m_halContext, drive_udi.latin1(), "storage.removable", NULL)
                     || libhal_device_get_property_bool(m_halContext, drive_udi.latin1(), "storage.hotpluggable", NULL);

    bool value;
    if (use_defaults)
    {
	value = config.readBoolEntry("automount", false);
    }
    else
    {
	QString current_group = config.group();
	config.setGroup(drive_udi);
	value = config.readBoolEntry("automount", false);
	config.setGroup(current_group);
    }

    if (libhal_device_get_property_bool(m_halContext, volume_udi.latin1(), "volume.disc.is_blank", NULL)
        || libhal_device_get_property_bool(m_halContext, volume_udi.latin1(), "volume.disc.is_vcd", NULL)
        || libhal_device_get_property_bool(m_halContext, volume_udi.latin1(), "volume.disc.is_svcd", NULL)
        || libhal_device_get_property_bool(m_halContext, volume_udi.latin1(), "volume.disc.is_videodvd", NULL)
        || libhal_device_get_property_bool(m_halContext, volume_udi.latin1(), "volume.disc.has_audio", NULL))
        value = false;

    result << TQString("automount=%1").arg(value ? "true" : "false");

    if (valids.contains("ro"))
    {
        value = config.readBoolEntry("ro", false);
        tmp = TQString("ro=%1").arg(value ? "true" : "false");
        if (fstype != "iso9660") // makes no sense
            result << tmp;
    }

    if (valids.contains("quiet"))
    {
        value = config.readBoolEntry("quiet", false);
        tmp = TQString("quiet=%1").arg(value ? "true" : "false");
        if (fstype != "iso9660") // makes no sense
            result << tmp;
    }

    if (valids.contains("flush"))
    {
        value = config.readBoolEntry("flush", fstype.endsWith("fat"));
        tmp = TQString("flush=%1").arg(value ? "true" : "false");
        result << tmp;
    }

    if (valids.contains("uid"))
    {
        value = config.readBoolEntry("uid", true);
        tmp = TQString("uid=%1").arg(value ? "true" : "false");
        result << tmp;
    }

    if (valids.contains("utf8"))
    {
        value = config.readBoolEntry("utf8", true);
        tmp = TQString("utf8=%1").arg(value ? "true" : "false");
        result << tmp;
    }

    if (valids.contains("shortname"))
    {
        TQString svalue = config.readEntry("shortname", "lower").lower();
        if (svalue == "windows nt")
            result << "shortname=winnt";
        else if (svalue == "windows 95")
            result << "shortname=win95";
        else if (svalue == "mixed")
            result << "shortname=mixed";
        else
            result << "shortname=lower";
    }

    // pass our locale to the ntfs-3g driver so it can translate local characters
    if (valids.contains("locale") && fstype == "ntfs-3g")
    {
        // have to obtain LC_CTYPE as returned by the `locale` command
        // check in the same order as `locale` does
        char *cType;
        if ( (cType = getenv("LC_ALL")) || (cType = getenv("LC_CTYPE")) || (cType = getenv("LANG")) ) {
            result << TQString("locale=%1").arg(cType);
        }
    }

    if (valids.contains("sync"))
    {
        value = config.readBoolEntry("sync", ( valids.contains("flush") && !fstype.endsWith("fat") ) && removable);
        tmp = TQString("sync=%1").arg(value ? "true" : "false");
        if (fstype != "iso9660") // makes no sense
            result << tmp;
    }

    if (valids.contains("noatime"))
    {
        value = config.readBoolEntry("atime", !fstype.endsWith("fat"));
        tmp = TQString("atime=%1").arg(value ? "true" : "false");
        if (fstype != "iso9660") // makes no sense
            result << tmp;
    }

    TQString mount_point = libhal_device_get_property_QString(m_halContext, volume_udi.latin1(), "volume.mount_point");
    if (mount_point.isEmpty())
        mount_point = libhal_device_get_property_QString(m_halContext, volume_udi.latin1(), "volume.policy.desired_mount_point");

    mount_point = config.readEntry("mountpoint", mount_point);

    if (!mount_point.startsWith("/"))
        mount_point = "/media/" + mount_point;

    result << TQString("mountpoint=%1").arg(mount_point);
    result << TQString("filesystem=%1").arg(fstype);

    if (valids.contains("data"))
    {
        TQString svalue = config.readEntry("journaling").lower();
        if (svalue == "ordered")
            result << "journaling=ordered";
        else if (svalue == "writeback")
            result << "journaling=writeback";
        else if (svalue == "data")
            result << "journaling=data";
        else
            result << "journaling=ordered";
    }

    return result;
}

bool HALBackend::setMountoptions(const TQString &name, const TQStringList &options )
{
    kdDebug() << "setMountoptions " << name << " " << options << endl;

    TDEConfig config("mediamanagerrc");
    config.setGroup(name);

    TQMap<TQString,TQString> valids = MediaManagerUtils::splitOptions(options);

    const char *names[] = { "use_defaults", "ro", "quiet", "atime", "uid", "utf8", "flush", "sync", 0 };
    for (int index = 0; names[index]; ++index)
        if (valids.contains(names[index]))
            config.writeEntry(names[index], valids[names[index]] == "true");

    if (valids.contains("shortname"))
        config.writeEntry("shortname", valids["shortname"]);

    if (valids.contains("journaling"))
        config.writeEntry("journaling", valids["journaling"]);

    if (!mountoptions(name).contains(TQString("mountpoint=%1").arg(valids["mountpoint"])))
        config.writeEntry("mountpoint", valids["mountpoint"]);

    if (valids.contains("automount")) {
        TQString drive_udi = libhal_device_get_property_QString(m_halContext, name.latin1(), "block.storage_device");
        config.setGroup(drive_udi);
        config.writeEntry("automount", valids["automount"]);
    }

    return true;
}

TQString startKdeSudoProcess(const TQString& tdesudoPath, const TQString& command,
        const TQString& dialogCaption, const TQString& dialogComment)
{
    TDEProcess tdesudoProcess;

    tdesudoProcess << tdesudoPath
		<< "-d"
		<< "--noignorebutton"
		<< "--caption" << dialogCaption
		<< "--comment" << dialogComment
		<< "-c" << command;

    // @todo handle tdesudo output
    tdesudoProcess.start(TDEProcess::Block);

    return TQString();
}

TQString startKdeSuProcess(const TQString& tdesuPath, const TQString& command,
        const TQString& dialogCaption)
{
    TDEProcess tdesuProcess;

    tdesuProcess << tdesuPath
		<< "-d"
		<< "--noignorebutton"
		<< "--caption" << dialogCaption
		<< "-c" << command;

    // @todo handle tdesu output
    tdesuProcess.start(TDEProcess::Block);

    return TQString();
}

TQString startPrivilegedProcess(const TQString& command, const TQString& dialogCaption, const TQString& dialogComment)
{
    TQString error;

    TQString tdesudoPath = TDEStandardDirs::findExe("tdesudo");

    if (!tdesudoPath.isEmpty())
        error = startKdeSudoProcess(tdesudoPath, command, dialogCaption, dialogComment);
    else {
        TQString tdesuPath = TDEStandardDirs::findExe("tdesu");

        if (!tdesuPath.isEmpty())
            error = startKdeSuProcess(tdesuPath, command, dialogCaption);
    }

    return error;
}

TQString privilegedMount(const char* udi, const char* mountPoint, const char** options, int numberOfOptions)
{
    TQString error;
 
    kdDebug() << "run privileged mount for " << udi << endl;

    TQString dbusSendPath = TDEStandardDirs::findExe("dbus-send");

    // @todo return error message
    if (dbusSendPath.isEmpty())
        return TQString();

    TQString mountOptions;
    TQTextOStream optionsStream(&mountOptions);
    for (int optionIndex = 0; optionIndex < numberOfOptions; optionIndex++) {
        optionsStream << options[optionIndex];
        if (optionIndex < numberOfOptions - 1)
            optionsStream << ",";
    }

    TQString command;
    TQTextOStream(&command) << dbusSendPath
            << " --system --print-reply --dest=org.freedesktop.Hal " << udi
            << " org.freedesktop.Hal.Device.Volume.Mount string:" << mountPoint
            << " string: array:string:" << mountOptions;

    kdDebug() << "command: " << command << endl;

    error = startPrivilegedProcess(command,
            i18n("Authenticate"),
            i18n("<big><b>System policy prevents mounting internal media</b></big><br/>Authentication is required to perform this action. Please enter your password to verify."));

    return error;
}

TQString privilegedUnmount(const char* udi)
{
    TQString error;
 
    kdDebug() << "run privileged unmount for " << udi << endl;

    TQString dbusSendPath = TDEStandardDirs::findExe("dbus-send");

    // @todo return error message
    if (dbusSendPath.isEmpty())
        return TQString();

    TQString command;
    TQTextOStream(&command) << dbusSendPath
            << " --system --print-reply --dest=org.freedesktop.Hal " << udi
            << " org.freedesktop.Hal.Device.Volume.Unmount array:string:force";

    kdDebug() << "command: " << command << endl;

    error = startPrivilegedProcess(command,
            i18n("Authenticate"),
            i18n("<big><b>System policy prevents unmounting media mounted by other users</b></big><br/>Authentication is required to perform this action. Please enter your password to verify."));

    return error;
}

static TQString mount_priv(const char *udi, const char *mount_point, const char **poptions, int noptions,
			  DBusConnection *dbus_connection)
{
    DBusMessage *dmesg, *reply;
    DBusError error;

    const char *fstype = "";
    if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.Hal", udi,
                                                "org.freedesktop.Hal.Device.Volume",
                                                "Mount"))) {
        kdDebug() << "mount failed for " << udi << ": could not create dbus message\n";
        return i18n("Internal Error");
    }

    if (!dbus_message_append_args (dmesg, DBUS_TYPE_STRING, &mount_point, DBUS_TYPE_STRING, &fstype,
                                   DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &poptions, noptions,
                                   DBUS_TYPE_INVALID))
    {
        kdDebug() << "mount failed for " << udi << ": could not append args to dbus message\n";
        dbus_message_unref (dmesg);
        return i18n("Internal Error");
    }

    TQString qerror;

    dbus_error_init (&error);
    if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error)))
    {
        TQString qerror = error.message;
        kdError() << "mount failed for " << udi << ": " << error.name << " - " << qerror << endl;
        if ( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.UnknownFilesystemType"))
            qerror = i18n("Invalid filesystem type");
        else if ( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.PermissionDenied"))
            qerror = i18n("Permission denied<p>Please ensure that:<br>1. You have permission to access this device.<br>2. This device node is not listed in /etc/fstab.</p>");
        else if ( !strcmp(error.name, "org.freedesktop.Hal.Device.PermissionDeniedByPolicy"))
            qerror = privilegedMount(udi, mount_point, poptions, noptions);
        else if ( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.AlreadyMounted"))
            qerror = i18n("Device is already mounted.");
        else if ( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.InvalidMountpoint") && strlen(mount_point)) {
            dbus_message_unref (dmesg);
            dbus_error_free (&error);
            return mount_priv(udi, "", poptions, noptions, dbus_connection);
        }
        dbus_message_unref (dmesg);
        dbus_error_free (&error);
        return qerror;
    }

    kdDebug() << "mount queued for " << udi << endl;

    dbus_message_unref (dmesg);
    dbus_message_unref (reply);

    return qerror;

}

TQString HALBackend::listUsingProcesses(const Medium* medium)
{
    TQString proclist, fullmsg;
    TQString fuserpath = TDEStandardDirs::findExe("fuser", TQString("/sbin:/usr/sbin:") + getenv( "PATH" ));
    FILE *fuser = NULL;

    uint counter = 0;
    if (!fuserpath.isEmpty()) {
        TQString cmdline = TQString("/usr/bin/env %1 -vm %2 2>&1").arg(fuserpath, TDEProcess::quote(medium->mountPoint()));
        fuser = popen(cmdline.latin1(), "r");
    }
    if (fuser) {
        proclist += "<pre>";
        TQTextIStream is(fuser);
        TQString tmp;
        while (!is.atEnd()) {
            tmp = is.readLine();
            tmp = TQStyleSheet::escape(tmp) + "\n";

            proclist += tmp;
            if (counter++ > 10)
            {
                proclist += "...";
                break;
            }
        }
        proclist += "</pre>";
        (void)pclose( fuser );
    }
    if (counter) {
        fullmsg = i18n("Moreover, programs still using the device "
            "have been detected. They are listed below. You have to "
            "close them or change their working directory before "
            "attempting to unmount the device again.");
        fullmsg += "<br>" + proclist;
        return fullmsg;
    } else {
        return TQString::null;
    }
}

TQString HALBackend::killUsingProcesses(const Medium* medium)
{
    TQString proclist, fullmsg;
    TQString fuserpath = TDEStandardDirs::findExe("fuser", TQString("/sbin:/usr/sbin:") + getenv( "PATH" ));
    FILE *fuser = NULL;

    uint counter = 0;
    if (!fuserpath.isEmpty()) {
        TQString cmdline = TQString("/usr/bin/env %1 -vmk %2 2>&1").arg(fuserpath, TDEProcess::quote(medium->mountPoint()));
        fuser = popen(cmdline.latin1(), "r");
    }
    if (fuser) {
        proclist += "<pre>";
        TQTextIStream is(fuser);
        TQString tmp;
        while (!is.atEnd()) {
            tmp = is.readLine();
            tmp = TQStyleSheet::escape(tmp) + "\n";

            proclist += tmp;
            if (counter++ > 10)
            {
                proclist += "...";
                break;
            }
        }
        proclist += "</pre>";
        (void)pclose( fuser );
    }
    if (counter) {
        fullmsg = i18n("Programs that were still using the device "
            "have been forcibly terminated. They are listed below.");
        fullmsg += "<br>" + proclist;
        return fullmsg;
    } else {
        return TQString::null;
    }
}

void HALBackend::slotResult(TDEIO::Job *job)
{
    kdDebug() << "slotResult " << mount_jobs[job] << endl;

    struct mount_job_data *data = mount_jobs[job];
    TQString& qerror = data->errorMessage;
    const Medium* medium = data->medium;

    if (job->error() == TDEIO::ERR_COULD_NOT_UNMOUNT) {
        TQString proclist(listUsingProcesses(medium));

        qerror = "<qt>";
        qerror += "<p>" + i18n("Unfortunately, the device <b>%1</b> (%2) named <b>'%3'</b> and "
                       "currently mounted at <b>%4</b> could not be unmounted. ").arg(
                          "system:/media/" + medium->name(),
                          medium->deviceNode(),
                          medium->prettyLabel(),
                          medium->prettyBaseURL().pathOrURL()) + "</p>";
        qerror += "<p>" + i18n("The following error was returned by umount command:");
        qerror += "</p><pre>" + job->errorText() + "</pre>";

        if (!proclist.isEmpty()) {
            qerror += proclist;
        }
        qerror += "</qt>";
    } else if (job->error()) {
        qerror = job->errorText();
    }

    ResetProperties( medium->id().latin1() );
    mount_jobs.remove(job);

    /* Job completed. Notify the caller */
    data->error = job->error();
    data->completed = true;
    kapp->eventLoop()->exitLoop();
}

TQString HALBackend::isInFstab(const Medium *medium)
{
    KMountPoint::List fstab = KMountPoint::possibleMountPoints(KMountPoint::NeedMountOptions|KMountPoint::NeedRealDeviceName);

    KMountPoint::List::iterator it = fstab.begin();
    KMountPoint::List::iterator end = fstab.end();

    for (; it!=end; ++it)
    {
        TQString reald = (*it)->realDeviceName();
        if ( reald.endsWith( "/" ) )
            reald = reald.left( reald.length() - 1 );
        kdDebug() << "isInFstab -" << medium->deviceNode() << "- -" << reald << "- -" << (*it)->mountedFrom() << "-" << endl;
        if ((*it)->mountedFrom() == medium->deviceNode() || ( !medium->deviceNode().isEmpty() && reald == medium->deviceNode() ) )
	{
            TQStringList opts = (*it)->mountOptions();
            if (opts.contains("user") || opts.contains("users"))
                return (*it)->mountPoint();
        }
    }

    return TQString::null;
}

TQString HALBackend::mount(const Medium *medium)
{
    if (medium->isMounted())
        return TQString(); // that was easy

    TQString mountPoint = isInFstab(medium);
    if (!mountPoint.isNull())
    {
        struct mount_job_data data;
        data.completed = false;
        data.medium = medium;

        kdDebug() << "triggering user mount " << medium->deviceNode() << " " << mountPoint << " " << medium->id() << endl;
        TDEIO::Job *job = TDEIO::mount( false, 0, medium->deviceNode(), mountPoint );
        connect(job, TQT_SIGNAL( result (TDEIO::Job *)),
                TQT_SLOT( slotResult( TDEIO::Job *)));
        mount_jobs[job] = &data;
        // The caller expects the device to be mounted when the function
        // completes. Thus block until the job completes.
        while (!data.completed) {
            kapp->eventLoop()->enterLoop();
        }
        // Return the error message (if any) to the caller
        return (data.error) ? data.errorMessage : TQString::null;

    } else if (medium->id().startsWith("/org/kde/") )
	    return i18n("Permission denied");

    TQStringList soptions;

    kdDebug() << "mounting " << medium->id() << "..." << endl;

    TQMap<TQString,TQString> valids = MediaManagerUtils::splitOptions(mountoptions(medium->id()));
    if (valids["flush"] == "true")
        soptions << "flush";

    if ((valids["uid"] == "true") && (medium->fsType() != "ntfs"))
    {
        soptions << TQString("uid=%1").arg(getuid());
    }

    if (valids["ro"] == "true")
        soptions << "ro";

    if (valids["atime"] != "true")
        soptions << "noatime";

    if (valids["quiet"] == "true")
        soptions << "quiet";

    if (valids["utf8"] == "true")
        soptions << "utf8";

    if (valids["sync"] == "true")
        soptions << "sync";

    if (medium->fsType() == "ntfs") {
        TQString fsLocale("locale=");
        fsLocale += setlocale(LC_ALL, "");

        soptions << fsLocale;
    }

    TQString mount_point = valids["mountpoint"];
    if (mount_point.startsWith("/media/"))
        mount_point = mount_point.mid(7);

    if (valids.contains("shortname"))
    {
        soptions << TQString("shortname=%1").arg(valids["shortname"]);
    }

    if (valids.contains("locale"))
    {
        soptions << TQString("locale=%1").arg(valids["locale"]);
    }

    if (valids.contains("journaling"))
    {
        TQString option = valids["journaling"];
        if (option == "data")
            soptions << TQString("data=journal");
        else if (option == "writeback")
            soptions << TQString("data=writeback");
        else
            soptions << TQString("data=ordered");
    }

    TQStringList hal_mount_options = getHALmountoptions(medium->id());
    for (TQValueListIterator<TQString> it=hal_mount_options.begin();it!=hal_mount_options.end();it++)
    {
       soptions << *it;
       kdDebug()<<"HALOption: "<<*it<<endl;
       if ((*it).startsWith("iocharset="))
       {
           soptions.remove("utf8");
           kdDebug()<<"\"iocharset=\" found. Removing \"utf8\" from options."<<endl;
       }
    }


    const char **options = new const char*[soptions.size() + 1];
    uint noptions = 0;
    for (TQStringList::ConstIterator it = soptions.begin(); it != soptions.end(); ++it, ++noptions)
    {
        options[noptions] = (*it).latin1();
        kdDebug()<<"Option: "<<*it<<endl;
    }
    options[noptions] = NULL;

    TQString qerror = i18n("Cannot mount encrypted drives!");

    if (!medium->isEncrypted()) {
        // normal volume
    	qerror = mount_priv(medium->id().latin1(), mount_point.utf8(), options, noptions, dbus_connection);
    } else {
        // see if we have a clear volume
        LibHalVolume* halVolume = libhal_volume_from_udi(m_halContext, medium->id().latin1());
        if (halVolume) {
            char* clearUdi = libhal_volume_crypto_get_clear_volume_udi(m_halContext, halVolume);
            if (clearUdi != NULL) {
                qerror = mount_priv(clearUdi, mount_point.utf8(), options, noptions, dbus_connection);
                libhal_free_string(clearUdi);
            }
            libhal_volume_free(halVolume);
        }
    }

    if (!qerror.isEmpty()) {
        kdError() << "mounting " << medium->id() << " returned " << qerror << endl;
        return qerror;
    }

    medium->setHalMounted(true);
    ResetProperties(medium->id().latin1());

    return TQString();
}

TQString HALBackend::mount(const TQString &_udi)
{
    const Medium* medium = m_mediaList.findById(_udi);
    if (!medium)
        return i18n("No such medium: %1").arg(_udi);

    return mount(medium);
}

TQString HALBackend::unmount(const TQString &_udi)
{
    const Medium* medium = m_mediaList.findById(_udi);
    if (!medium)
    {   // now we get fancy: if the udi is no volume, it _might_ be a device with only one
        // volume on it (think CDs) - so we're so nice to the caller to unmount that volume
        LibHalDrive*  halDrive  = libhal_drive_from_udi(m_halContext, _udi.latin1());
        if (halDrive)
        {
            int numVolumes;
            char** volumes = libhal_drive_find_all_volumes(m_halContext, halDrive, &numVolumes);
            if (numVolumes == 1)
                medium = m_mediaList.findById( volumes[0] );
        }
    }

    if ( !medium )
        return i18n("No such medium: %1").arg(_udi);

    if (!medium->isMounted())
        return TQString(); // that was easy

    TQString mountPoint = isInFstab(medium);
    if (!mountPoint.isNull())
    {
        struct mount_job_data data;
        data.completed = false;
        data.medium = medium;

        kdDebug() << "triggering user unmount " << medium->deviceNode() << " " << mountPoint << endl;
        TDEIO::Job *job = TDEIO::unmount( medium->mountPoint(), false );
        connect(job, TQT_SIGNAL( result (TDEIO::Job *)),
                TQT_SLOT( slotResult( TDEIO::Job *)));
        mount_jobs[job] = &data;
        // The caller expects the device to be unmounted when the function
        // completes. Thus block until the job completes.
        while (!data.completed) {
            kapp->eventLoop()->enterLoop();
        }
        // Return the error message (if any) to the caller
        return (data.error) ? data.errorMessage : TQString::null;
    }

    DBusMessage *dmesg, *reply;
    DBusError error;
    const char *options[2];
    TQString udi = TQString::null;

    if (!medium->isEncrypted()) {
        // normal volume
        udi = medium->id();
    } else {
        // see if we have a clear volume
        LibHalVolume* halVolume = libhal_volume_from_udi(m_halContext, medium->id().latin1());
        if (halVolume) {
            char *clearUdi = libhal_volume_crypto_get_clear_volume_udi(m_halContext, halVolume);
	    udi = clearUdi;
	    libhal_free_string(clearUdi);
            libhal_volume_free(halVolume);
        }
    }
    if (udi.isNull()) {
        kdDebug() << "unmount failed: no udi" << endl;
        return i18n("Internal Error");
    }

    kdDebug() << "unmounting " << udi << "..." << endl;

    dbus_error_init(&error);
    DBusConnection *dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set(&error))
    {
        dbus_error_free(&error);
        return false;
    }

    if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.Hal", udi.latin1(),
                                                "org.freedesktop.Hal.Device.Volume",
                                                "Unmount"))) {
        kdDebug() << "unmount failed for " << udi << ": could not create dbus message\n";
        return i18n("Internal Error");
    }

    options[0] = "force";
    options[1] = 0;

    if (!dbus_message_append_args (dmesg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &options, 0,
                                   DBUS_TYPE_INVALID))
    {
        kdDebug() << "unmount failed for " << udi << ": could not append args to dbus message\n";
        dbus_message_unref (dmesg);
        return i18n("Internal Error");
    }

    char thisunmounthasfailed = 0;
    dbus_error_init (&error);
    if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error)))
    {
        thisunmounthasfailed = 1;
        TQString qerror, reason, origqerror;

        if (!strcmp(error.name, "org.freedesktop.Hal.Device.PermissionDeniedByPolicy")) {
            qerror = privilegedUnmount(udi.latin1());

            if (qerror.isEmpty()) {
                dbus_message_unref(dmesg);
                dbus_error_free(&error);
                return TQString();
            }

            // @todo handle unmount error message
        }
        
        kdDebug() << "unmount failed for " << udi << ": " << error.name << " " << error.message << endl;
        qerror = "<qt>";
        qerror += "<p>" + i18n("Unfortunately, the device <b>%1</b> (%2) named <b>'%3'</b> and "
                       "currently mounted at <b>%4</b> could not be unmounted. ").arg(
                          "system:/media/" + medium->name(),
                          medium->deviceNode(),
                          medium->prettyLabel(),
                          medium->prettyBaseURL().pathOrURL()) + "</p>";
        qerror += "<p>" + i18n("Unmounting failed due to the following error:") + "</p>";
        if (!strcmp(error.name, "org.freedesktop.Hal.Device.Volume.Busy")) {
            reason = i18n("Device is Busy:");
            thisunmounthasfailed = 2;
        } else if (!strcmp(error.name, "org.freedesktop.Hal.Device.Volume.NotMounted")) {
            // this is faking. The error is that the device wasn't mounted by hal (but by the system)
            reason = i18n("Permission denied<p>Please ensure that:<br>1. You have permission to access this device.<br>2. This device was originally mounted using TDE.</p>");
        } else {
            reason = error.message;
        }
        qerror += "<p><b>" + reason + "</b></p>";
        origqerror = qerror;

        // Include list of processes (if any) using the device in the error message
        reason = listUsingProcesses(medium);
        if (!reason.isEmpty()) {
            qerror += reason;
            if (thisunmounthasfailed == 2) {	// Failed as BUSY
                if (KMessageBox::warningYesNo(0, i18n("%1<p><b>Would you like to forcibly terminate these processes?</b><br><i>All unsaved data would be lost</i>").arg(qerror)) == KMessageBox::Yes) {
                    qerror = origqerror;
                    reason = killUsingProcesses(medium);
                    qerror = HALBackend::unmount(udi);
                    if (qerror.isNull()) {
                        thisunmounthasfailed = 0;
                    }
                }
            }
        }

        if (thisunmounthasfailed != 0) {
            dbus_message_unref (dmesg);
            dbus_error_free (&error);
            return qerror;
        }
    }

    kdDebug() << "unmount queued for " << udi << endl;

    dbus_message_unref (dmesg);
    if (reply) {
      dbus_message_unref (reply);
    }

    medium->setHalMounted(false);
    ResetProperties(medium->id().latin1());

    while (dbus_connection_dispatch(dbus_connection) == DBUS_DISPATCH_DATA_REMAINS) ;

    return TQString();
}

TQString HALBackend::decrypt(const TQString &_udi, const TQString &password)
{
    const Medium* medium = m_mediaList.findById(_udi);
    if (!medium)
        return i18n("No such medium: %1").arg(_udi);

    if (!medium->isEncrypted() || !medium->clearDeviceUdi().isNull())
        return TQString();

    const char *udi = medium->id().latin1();
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    DBusError error;

    kdDebug() << "Setting up " << udi << " for crypto\n" <<endl;
	
    msg = dbus_message_new_method_call ("org.freedesktop.Hal", udi,
                                        "org.freedesktop.Hal.Device.Volume.Crypto",
                                        "Setup");
    if (msg == NULL) {
        kdDebug() << "decrypt failed for " << udi << ": could not create dbus message\n";
        return i18n("Internal Error");
    }

    TQCString pwdUtf8 = password.utf8();
    const char *pwd_utf8 = pwdUtf8;
    if (!dbus_message_append_args (msg, DBUS_TYPE_STRING, &pwd_utf8, DBUS_TYPE_INVALID)) {
        kdDebug() << "decrypt failed for " << udi << ": could not append args to dbus message\n";
        dbus_message_unref (msg);
        return i18n("Internal Error");
    }

    dbus_error_init (&error);
    if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, msg, -1, &error)) || 
        dbus_error_is_set (&error))
    {
        TQString qerror = i18n("Internal Error");
        kdDebug() << "decrypt failed for " << udi << ": " << error.name << " " << error.message << endl;
        if (strcmp (error.name, "org.freedesktop.Hal.Device.Volume.Crypto.SetupPasswordError") == 0) {
            qerror = i18n("Wrong password");
        }
        dbus_error_free (&error);
        dbus_message_unref (msg);
        while (dbus_connection_dispatch(dbus_connection) == DBUS_DISPATCH_DATA_REMAINS) ;
        return qerror;
    }

    dbus_message_unref (msg);
    dbus_message_unref (reply);

    while (dbus_connection_dispatch(dbus_connection) == DBUS_DISPATCH_DATA_REMAINS) ;

    return TQString();
}

TQString HALBackend::undecrypt(const TQString &_udi)
{
    const Medium* medium = m_mediaList.findById(_udi);
    if (!medium)
        return i18n("No such medium: %1").arg(_udi);

    if (!medium->isEncrypted() || medium->clearDeviceUdi().isNull())
        return TQString();

    const char *udi = medium->id().latin1();
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    DBusError error;

    kdDebug() << "Tear down " << udi << "\n" <<endl;
	
    msg = dbus_message_new_method_call ("org.freedesktop.Hal", udi,
                                        "org.freedesktop.Hal.Device.Volume.Crypto",
                                        "Teardown");
    if (msg == NULL) {
        kdDebug() << "teardown failed for " << udi << ": could not create dbus message\n";
        return i18n("Internal Error");
    }

    if (!dbus_message_append_args (msg, DBUS_TYPE_INVALID)) {
        kdDebug() << "teardown failed for " << udi << ": could not append args to dbus message\n";
        dbus_message_unref (msg);
        return i18n("Internal Error");
    }

    dbus_error_init (&error);
    if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, msg, -1, &error)) || 
        dbus_error_is_set (&error))
    {
        TQString qerror = i18n("Internal Error");
        kdDebug() << "teardown failed for " << udi << ": " << error.name << " " << error.message << endl;
        dbus_error_free (&error);
        dbus_message_unref (msg);
        while (dbus_connection_dispatch(dbus_connection) == DBUS_DISPATCH_DATA_REMAINS) ;
        return qerror;
    }

    dbus_message_unref (msg);
    dbus_message_unref (reply);

    ResetProperties(udi);

    while (dbus_connection_dispatch(dbus_connection) == DBUS_DISPATCH_DATA_REMAINS) ;

    return TQString();
}

#include "halbackend.moc"
