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

tde_create_translated_desktop(
  SOURCE
    linkProgram.desktop linkURL.desktop linkFloppy.desktop
    linkHD.desktop linkCDROM.desktop Directory.desktop
    TextFile.desktop HTMLFile.desktop linkZIP.desktop
    linkDVDROM.desktop linkCAMERA.desktop linkNFS.desktop
    linkCDWRITER.desktop linkMO.desktop
  DESTINATION ${TEMPLATES_INSTALL_DIR}
  PO_DIR kdesktop-icons
)

tde_create_translated_desktop(
  SOURCE
    Program.desktop URL.desktop NFS.desktop Floppy.desktop HD.desktop
    CDROM-Device.desktop CDWRITER-Device.desktop DVDROM-Device.desktop
    ZIP-Device.desktop CAMERA-Device.desktop MO-Device.desktop
  DESTINATION ${TEMPLATES_INSTALL_DIR}/.source
  PO_DIR kdesktop-icons
)

install(
  FILES TextFile.txt HTMLFile.html
  DESTINATION ${TEMPLATES_INSTALL_DIR}/.source
)

tde_install_empty_directory( ${TEMPLATES_INSTALL_DIR}/.source/emptydir )
