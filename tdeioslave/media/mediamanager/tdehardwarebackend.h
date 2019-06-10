/* This file is part of the TDE Project
   Copyright (c) 2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>

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

/**
* This is a media:/ backend for the builtin TDE hardware library
*
* @author Timothy Pearson <kb9vqf@pearsoncomputing.net>
* @short media:/ backend for the TDE hardware library
*/

#ifndef _TDEBACKEND_H_
#define _TDEBACKEND_H_

#include "backendbase.h"

#include <tqobject.h>
#include <tqstringlist.h>
#include <tqstring.h>

#include <config.h>

#include <tdehardwaredevices.h>

namespace TDEIO {
	class Job;
}

class Dialog;

class TDEBackend : public TQObject, public BackendBase
{
Q_OBJECT

public:
	/**
	* Constructor
	*/
	TDEBackend(MediaList &list, TQObject* parent);

	/**
	* Destructor
	*/
	~TDEBackend();

	/**
	* List all devices and append them to the media device list (called only once, at startup).
	*
	* @return true if succeded, false otherwise
	*/
	bool ListDevices();

	TQStringList mountoptions(const TQString &id);

	bool setMountoptions(const TQString &id, const TQStringList &options);

	TQStringVariantMap mount(const TQString &id);
	TQStringVariantMap mount(const Medium *medium);
	TQStringVariantMap unmount(const TQString &id);
	TQStringVariantMap unlock(const TQString &id, const TQString &password);
	TQStringVariantMap lock(const TQString &id);

private:
	/**
	* Append a device in the media list. This function will check if the device
	* is worth listing.
	*
	*  @param sdevice             A pointer to a TDEStorageDevice object
	*  @param allowNotification   Indicates if this event will be notified to the user
	*/
	void AddDevice(TDEStorageDevice * sdevice, bool allowNotification=true);

	/**
	* Remove a device from the device list
	*
	*  @param sdevice             A pointer to a TDEStorageDevice object
	*/
	void RemoveDevice(TDEStorageDevice * sdevice);

	/**
	* A device has changed, update it
	*
	*  @param sdevice             A pointer to a TDEStorageDevice object
	*/
	void ModifyDevice(TDEStorageDevice * sdevice);

private slots:
	void AddDeviceHandler(TDEGenericDevice* device);
	void RemoveDeviceHandler(TDEGenericDevice* device);
	void ModifyDeviceHandler(TDEGenericDevice* device);

	void slotPasswordReady();
	void slotPasswordCancel();

signals:
	void signalDecryptionPasswordError(TQString);

/* Set media properties */
private:
	/**
	* Reset properties for the given medium
	*
	*  @param sdevice             A pointer to a TDEStorageDevice objec
	*  @param allowNotification   Indicates if this event will be notified to the user
	*  @param overrideIgnoreList  If true, override event ignore requests for the current device node
	*/
	void ResetProperties(TDEStorageDevice * sdevice, bool allowNotification=false, bool overrideIgnoreList=false);

	/**
	* Find the medium that is concerned with device udi
	*/
// 	const char* findMediumUdiFromUdi(const char* udi);

	void setVolumeProperties(Medium* medium);
	bool setFloppyProperties(Medium* medium);
	void setFloppyMountState( Medium* medium );
// 	bool setFstabProperties(Medium* medium);
	void setCameraProperties(Medium* medium);
	TQString generateName(const TQString &devNode);
	static TQString isInFstab(const Medium *medium);
	static TQString listUsingProcesses(const Medium *medium);
	static TQString killUsingProcesses(const Medium *medium);

	TQString driveUDIFromDeviceUID(TQString uuid);

	// Decryption
	Dialog* m_unlockDialog;
	TQString m_decryptionPassword;
	bool m_decryptPasswordValid;

private slots:
	void slotResult(TDEIO::Job *job);

/* TDE structures */
private:
	/**
	* Object for the kded module
	*/
	TQObject* m_parent;

	/**
	* Data structure for fstab mount/unmount jobs
	*/
	struct mount_job_data {
		// [in] Medium, which is being mounted/unmounted by the job
		const Medium* medium;
		// [in,out] Should be set to true when the job completes
		bool completed;
		// [out] TDEIO::Error if an error occured during operation. Otherwise, 0
		int error;
		// [out] Error message to be displayed to the user
		TQString errorMessage;
	};

	TQMap<TDEIO::Job *, struct mount_job_data*> mount_jobs;

	TQStringList m_ignoreDeviceChangeEvents;
};

#endif /* _TDEBACKEND_H_ */
