#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

##### other data ################################

install( FILES
    floppy_mounted.desktop floppy_unmounted.desktop
    floppy5_unmounted.desktop floppy5_mounted.desktop
    zip_mounted.desktop zip_unmounted.desktop
    hdd_mounted.desktop hdd_unmounted.desktop
    hdd_encrypted_locked.desktop
    hdd_encrypted_unlocked.desktop removable_mounted.desktop
    removable_unmounted.desktop
    removable_encrypted_locked.desktop removable_encrypted_unlocked.desktop
    bluray_mounted.desktop bluray_unmounted.desktop
    bluray_encrypted_locked.desktop bluray_encrypted_unlocked.desktop
    cdrom_mounted.desktop cdrom_unmounted.desktop
    cdrom_encrypted_locked.desktop cdrom_encrypted_unlocked.desktop
    dvd_mounted.desktop dvd_unmounted.desktop
    dvd_encrypted_locked.desktop dvd_encrypted_unlocked.desktop
    cd-r_mounted.desktop cd-r_unmounted.desktop
    cd-r_encrypted_locked.desktop cd-r_encrypted_unlocked.desktop
    cd-rw_mounted.desktop cd-rw_unmounted.desktop
    cd-rw_encrypted_locked.desktop cd-rw_encrypted_unlocked.desktop
    smb_mounted.desktop smb_unmounted.desktop
    nfs_mounted.desktop nfs_unmounted.desktop
    audiocd.desktop
    blankbluray.desktop blankcd.desktop blankdvd.desktop
    blurayvideo.desktop dvdvideo.desktop svcd.desktop vcd.desktop
    gphoto2camera.desktop
    camera_mounted.desktop camera_unmounted.desktop
    mydocuments.desktop mycomputer.desktop mynetworkplaces.desktop
    printers.desktop trash.desktop webbrowser.desktop
  DESTINATION ${MIME_INSTALL_DIR}/media )
