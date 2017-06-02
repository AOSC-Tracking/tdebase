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

include( ConfigureChecks.cmake )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${SMBCLIENT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${SMBCLIENT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES smb.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES smb-network.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/dirtree/remote )
install( FILES smb-network.desktop DESTINATION ${DATA_INSTALL_DIR}/remoteview )
install( FILES x-smb-workgroup.desktop x-smb-server.desktop DESTINATION ${MIME_INSTALL_DIR}/application )


##### tdeio_smb (module) ##########################

tde_add_kpart( tdeio_smb AUTOMOC
  SOURCES
    tdeio_smb.cpp tdeio_smb_auth.cpp tdeio_smb_browse.cpp tdeio_smb_config.cpp
    tdeio_smb_dir.cpp tdeio_smb_file.cpp tdeio_smb_internal.cpp tdeio_smb_mount.cpp
  LINK tdeio-shared ${SMBCLIENT_LIBRARIES}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
